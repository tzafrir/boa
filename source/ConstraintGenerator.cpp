#include "ConstraintGenerator.h"
#include <vector>

using std::vector;

namespace boa {

vector<Constraint::Expression> ConstraintGenerator::GenerateIntegerExpression(Expr *expr, bool max) {
  vector<Constraint::Expression> result;
  Constraint::Expression ce;

  while (dyn_cast<ImplicitCastExpr>(expr)) {
    expr = dyn_cast<ImplicitCastExpr>(expr)->getSubExpr();
  }

  if (SizeOfAlignOfExpr *sizeOfExpr = dyn_cast<SizeOfAlignOfExpr>(expr)) {
    if (sizeOfExpr->isSizeOf()) {
      ce.add(1); // sizeof(ANYTHING) == 1
      result.push_back(ce);
      return result;
    }
  }

  if (IntegerLiteral *literal = dyn_cast<IntegerLiteral>(expr)) {
    ce.add(literal->getValue().getLimitedValue());
    result.push_back(ce);
    return result;
  }

  if (dyn_cast<DeclRefExpr>(expr)) {
    Integer intLiteral(expr);
    ce.add(intLiteral.NameExpression(max ? MAX : MIN));
    result.push_back(ce);
    return result;
  }

  if (BinaryOperator *op = dyn_cast<BinaryOperator>(expr)) {
    switch (op->getOpcode()) {
      case BO_Add : {
        vector<Constraint::Expression> LHExpressions = GenerateIntegerExpression(op->getLHS(), max);
        vector<Constraint::Expression> RHExpressions = GenerateIntegerExpression(op->getRHS(), max);
        for (unsigned i = 0; i < LHExpressions.size(); ++i) {
          for (unsigned j = 0; j < RHExpressions.size(); ++j) {
            Constraint::Expression loopCE;
            loopCE.add(LHExpressions[i]);
            loopCE.add(RHExpressions[j]);
            result.push_back(loopCE);
          }
        }
        return result;
      }
      case BO_Sub : {
        vector<Constraint::Expression> LHExpressions = GenerateIntegerExpression(op->getLHS(), max);
        vector<Constraint::Expression> RHExpressions = GenerateIntegerExpression(op->getRHS(), !max);
        for (unsigned i = 0; i < LHExpressions.size(); ++i) {
          for (unsigned j = 0; j < RHExpressions.size(); ++j) {
            Constraint::Expression loopCE;
            loopCE.add(LHExpressions[i]);
            loopCE.sub(RHExpressions[j]);
            result.push_back(loopCE);
          }
        }
        return result;
      }
      case BO_Mul : {
        vector<Constraint::Expression> LHS = GenerateIntegerExpression(op->getLHS(), max);
        vector<Constraint::Expression> RHS = GenerateIntegerExpression(op->getRHS(), max);
        if ((LHS.size() == 1) && (LHS[0].IsConst())) {
          for (unsigned i = 0; i < RHS.size(); ++i) {
            RHS[i].mul(LHS[0].GetConst());
            result.push_back(RHS[i]);
          }
          RHS = GenerateIntegerExpression(op->getRHS(), !max);
          for (unsigned i = 0; i < RHS.size(); ++i) {
            RHS[i].mul(LHS[0].GetConst());
            result.push_back(RHS[i]);
          }
          return result;
        }
        else if ((RHS.size() == 1) && (RHS[0].IsConst())) {
          for (unsigned i = 0; i < LHS.size(); ++i) {
            LHS[i].mul(RHS[0].GetConst());
            result.push_back(LHS[i]);
          }
          LHS = GenerateIntegerExpression(op->getLHS(), !max);
          for (unsigned i = 0; i < LHS.size(); ++i) {
            LHS[i].mul(RHS[0].GetConst());
            result.push_back(LHS[i]);
          }
          return result;
        }
        // TODO - return infinity/NaN instead of empty vector?
        log::os() << "Multiplaction of two non const integer expressions " << getStmtLoc(expr) << endl;
        return result;
      }
      default : break;
    }
  }

  expr->dump();
  log::os() << "Can't generate integer expression " << getStmtLoc(expr) << endl;
  return result;
}

bool ConstraintGenerator::GenerateArraySubscriptConstraints(ArraySubscriptExpr* expr) {
  VarLiteral* varLiteral = NULL;

  Expr* base = expr->getBase();
  while (dyn_cast<ImplicitCastExpr>(base)) {
    base = dyn_cast<ImplicitCastExpr>(base)->getSubExpr();
  }

  if (DeclRefExpr *declRef = dyn_cast<DeclRefExpr>(base)) {
    if (ArrayType* arr = dyn_cast<ArrayType>(declRef->getDecl()->getType().getTypePtr())) {
      if (arr->getElementType()->isAnyCharacterType()) {
        varLiteral = new Buffer(declRef->getDecl());
      }
    }
    else if (PointerType* pType = dyn_cast<PointerType>(declRef->getDecl()->getType().getTypePtr())) {
      if (pType->getPointeeType()->isAnyCharacterType()) {
        varLiteral = new Pointer(declRef->getDecl());
      }
    }
  }
  else {
    // TODO: Any other cases?
  }

  if (NULL == varLiteral) {
    expr->dump();
    return false;
  }

  vector<Constraint::Expression> maxIndices = GenerateIntegerExpression(expr->getIdx(), true);
  for (unsigned i = 0; i < maxIndices.size(); ++i) {
    Constraint usedMax;
    usedMax.addBig(varLiteral->NameExpression(MAX, USED));
    usedMax.addSmall(maxIndices[i]);
    cp_.AddConstraint(usedMax);
    log::os() << "Adding - " << varLiteral->NameExpression(MAX, USED) << " >= " <<
                 maxIndices[i].toString() << "\n";
  }
  vector<Constraint::Expression> minIndices = GenerateIntegerExpression(expr->getIdx(), false);
  for (unsigned i = 0; i < minIndices.size(); ++i) {
    Constraint usedMin;
    usedMin.addSmall(varLiteral->NameExpression(MIN, USED));
    usedMin.addSmall(minIndices[i]);
    cp_.AddConstraint(usedMin);
    log::os() << "Adding - " << varLiteral->NameExpression(MIN, USED) << " <= " <<
                 minIndices[i].toString() << "\n";
  }

  delete varLiteral;
  return true;
}

void ConstraintGenerator::GenerateVarDeclConstraints(VarDecl *var) {
  if (ConstantArrayType* arr = dyn_cast<ConstantArrayType>(var->getType().getTypePtr())) {
    if (arr->getElementType()->isAnyCharacterType()) {
      Buffer buf(var);
      Constraint allocMax, allocMin;

      allocMax.addBig(buf.NameExpression(MAX, ALLOC));
      allocMax.addSmall(arr->getSize().getLimitedValue());
      cp_.AddConstraint(allocMax);
      log::os() << "Adding - " << buf.NameExpression(MAX, ALLOC) << " >= " <<
                    arr->getSize().getLimitedValue() << "\n";

      allocMin.addSmall(buf.NameExpression(MIN, ALLOC));
      allocMin.addBig(arr->getSize().getLimitedValue());
      log::os() << "Adding - " << buf.NameExpression(MIN, ALLOC) << " <= " <<
                   arr->getSize().getLimitedValue() << "\n";
      cp_.AddConstraint(allocMin);
    }
  }
  
  if (var->getType()->isIntegerType()) {
    Integer intLiteral(var);
    if (var->hasInit()) {
      vector<Constraint::Expression> maxInits  = GenerateIntegerExpression(var->getInit(), true); 
      if (!maxInits.empty()) {
        for (unsigned i = 0; i < maxInits.size(); ++i) {
          Constraint c;
          c.addBig(intLiteral.NameExpression(MAX));
          c.addSmall(maxInits[i]);
          cp_.AddConstraint(c);
          log::os() << "Adding - " << intLiteral.NameExpression(MAX) << " >= " 
                    << maxInits[i].toString() << endl;
        }
        vector<Constraint::Expression> minInits  = GenerateIntegerExpression(var->getInit(), false); 
        for (unsigned i = 0; i < minInits.size(); ++i) {
          Constraint c;
          c.addSmall(intLiteral.NameExpression(MIN));
          c.addBig(minInits[i]);
          cp_.AddConstraint(c);
          log::os() << "Adding - " << intLiteral.NameExpression(MIN) << " <= " 
                    << minInits[i].toString() << endl;
        }              
        return;
      }
    }
    log::os() << "Integer definition without initializer on " << getStmtLoc(var) << endl;          
  }
}

bool ConstraintGenerator::VisitStmt(Stmt* S) {
  if (ArraySubscriptExpr* expr = dyn_cast<ArraySubscriptExpr>(S)) {
    return GenerateArraySubscriptConstraints(expr);
  }

  if (DeclStmt* dec = dyn_cast<DeclStmt>(S)) {
    for (DeclGroupRef::iterator decIt = dec->decl_begin(); decIt != dec->decl_end(); ++decIt) {
      if (VarDecl* var = dyn_cast<VarDecl>(*decIt)) {
        GenerateVarDeclConstraints(var);
      }
    }
  }

  if (CallExpr* funcCall = dyn_cast<CallExpr>(S)) {
    if (FunctionDecl* funcDec = funcCall->getDirectCallee()) {
      if (funcDec->getNameInfo().getAsString() == "malloc") {
        Buffer buf(funcCall);
        Expr* argument = funcCall->getArg(0);
        while (ImplicitCastExpr *implicitCast = dyn_cast<ImplicitCastExpr>(argument)) {
          argument = implicitCast->getSubExpr();
        }

        vector<Constraint::Expression> maxArgs = GenerateIntegerExpression(argument, true);
        for (unsigned i = 0; i < maxArgs.size(); ++i) {
          Constraint allocMax;

          allocMax.addBig(buf.NameExpression(MAX, ALLOC));
          allocMax.addSmall(maxArgs[i]);
          cp_.AddConstraint(allocMax);
          log::os() << "Adding - " << buf.NameExpression(MAX, ALLOC) << " >= "
                    << maxArgs[i].toString() << "\n";
        }
        vector<Constraint::Expression> minArgs = GenerateIntegerExpression(argument, false);
        for (unsigned i = 0; i < minArgs.size(); ++i) {
          Constraint allocMin;

          allocMin.addSmall(buf.NameExpression(MIN, ALLOC));
          allocMin.addBig(minArgs[i]);
          cp_.AddConstraint(allocMin);
          log::os() << "Adding - " << buf.NameExpression(MIN, ALLOC) << " <= "
                    << minArgs[i].toString() << "\n";
        }
        return true;
      }
    }
  }


//    if (BinaryOperator* op = dyn_cast<BinaryOperator>(S)) {
//      //if (!op->isAssignmentOp()) {
//        return true;
//      //}

//      op->dump();
//      if (op->getLHS()->getType()->isIntegerType() && op->getRHS()->getType()->isIntegerType()) {
//        if (DeclRefExpr* dre = dyn_cast<DeclRefExpr>(op->getLHS())) {
//          log::os() << "Is at " << (void*)(dre->getDecl()) << "\n";
//        }
//      }
//    }

  return true;
}

} // namespace boa
