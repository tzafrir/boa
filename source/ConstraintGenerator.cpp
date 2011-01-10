#include "ConstraintGenerator.h"
#include <vector>
#include <limits>

using std::vector;

namespace boa {

vector<Constraint::Expression>
    ConstraintGenerator::GenerateIntegerExpression(Expr *expr, bool max) {
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

  if (DeclRefExpr *declRef = dyn_cast<DeclRefExpr>(expr)) {
    Integer intLiteral(declRef->getDecl());
    ce.add(intLiteral.NameExpression(max ? MAX : MIN));
    result.push_back(ce);
    return result;
  }

  if (BinaryOperator *op = dyn_cast<BinaryOperator>(expr)) {

    // TODO(tzafrir): Factor out each case's block to a separate method.
    switch (op->getOpcode()) {
      case BO_Add : {
        vector<Constraint::Expression> LHExpressions = GenerateIntegerExpression(op->getLHS(), max);
        vector<Constraint::Expression> RHExpressions = GenerateIntegerExpression(op->getRHS(), max);
        for (size_t i = 0; i < LHExpressions.size(); ++i) {
          for (size_t j = 0; j < RHExpressions.size(); ++j) {
            Constraint::Expression loopCE;
            loopCE.add(LHExpressions[i]);
            loopCE.add(RHExpressions[j]);
            result.push_back(loopCE);
          }
        }
        return result;
      }
      case BO_Sub : {
        vector<Constraint::Expression> LHS = GenerateIntegerExpression(op->getLHS(), max);
        vector<Constraint::Expression> RHS = GenerateIntegerExpression(op->getRHS(), !max);
        for (size_t i = 0; i < LHS.size(); ++i) {
          for (size_t j = 0; j < RHS.size(); ++j) {
            Constraint::Expression loopCE;
            loopCE.add(LHS[i]);
            loopCE.sub(RHS[j]);
            result.push_back(loopCE);
          }
        }
        return result;
      }
      case BO_Mul : {
        vector<Constraint::Expression> LHS = GenerateIntegerExpression(op->getLHS(), max);
        vector<Constraint::Expression> RHS = GenerateIntegerExpression(op->getRHS(), max);
        if ((LHS.size() == 1) && (LHS[0].IsConst())) {
          for (size_t i = 0; i < RHS.size(); ++i) {
            RHS[i].mul(LHS[0].GetConst());
            result.push_back(RHS[i]);
          }
          RHS = GenerateIntegerExpression(op->getRHS(), !max);
          for (size_t i = 0; i < RHS.size(); ++i) {
            RHS[i].mul(LHS[0].GetConst());
            result.push_back(RHS[i]);
          }
          return result;
        }
        else if ((RHS.size() == 1) && (RHS[0].IsConst())) {
          for (size_t i = 0; i < LHS.size(); ++i) {
            LHS[i].mul(RHS[0].GetConst());
            result.push_back(LHS[i]);
          }
          LHS = GenerateIntegerExpression(op->getLHS(), !max);
          for (size_t i = 0; i < LHS.size(); ++i) {
            LHS[i].mul(RHS[0].GetConst());
            result.push_back(LHS[i]);
          }
          return result;
        }
        // TODO - return infinity/NaN instead of empty vector?
        log::os() << "Multiplication of two non-const integer expressions " << getStmtLoc(expr) << endl;
        return result;
      }
      case BO_Div : {
        vector<Constraint::Expression> LHS = GenerateIntegerExpression(op->getLHS(), max);
        vector<Constraint::Expression> RHS = GenerateIntegerExpression(op->getRHS(), max);
        if ((LHS.size() == 1) && (LHS[0].IsConst())) {
          for (size_t i = 0; i < RHS.size(); ++i) {
            RHS[i].beDividedBy(LHS[0].GetConst());
            result.push_back(RHS[i]);
          }
          RHS = GenerateIntegerExpression(op->getRHS(), !max);
          for (size_t i = 0; i < RHS.size(); ++i) {
            RHS[i].beDividedBy(LHS[0].GetConst());
            result.push_back(RHS[i]);
          }
          return result;
        }
        else  if ((RHS.size() == 1) && (RHS[0].IsConst())) {
          for (size_t i = 0; i < LHS.size(); ++i) {
            LHS[i].div(RHS[0].GetConst());
            result.push_back(LHS[i]);
          }
          LHS = GenerateIntegerExpression(op->getLHS(), !max);
          for (size_t i = 0; i < LHS.size(); ++i) {
            LHS[i].div(RHS[0].GetConst());
            result.push_back(LHS[i]);
          }
          return result;
        }
        // TODO - return infinity/NaN instead of empty vector?
        log::os() << "Division of two non-const integer expressions " << getStmtLoc(expr) << endl;
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

  GenerateGenericConstraint(*varLiteral, expr->getIdx(), "array subscript " + getStmtLoc(expr), USED);

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
      allocMax.SetBlame("static char buffer declaration " + getStmtLoc(var));
      cp_.AddConstraint(allocMax);
      log::os() << "Adding - " << buf.NameExpression(MAX, ALLOC) << " >= " <<
                    arr->getSize().getLimitedValue() << "\n";

      allocMin.addSmall(buf.NameExpression(MIN, ALLOC));
      allocMin.addBig(arr->getSize().getLimitedValue());
      allocMin.SetBlame("static char buffer declaration " + getStmtLoc(var));
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
        GenerateGenericConstraint(intLiteral, var->getInit(), "int declaration " + getStmtLoc(var));
        return;
      }
    }

    GenerateUnboundConstraint(intLiteral, "int without initializer " + getStmtLoc(var));
    log::os() << "Integer definition without initializer on " << getStmtLoc(var) << endl;
  }
}

void ConstraintGenerator::GenerateUnboundConstraint(const VarLiteral &var, const string &blame) {
  // FIXME - is MAX_INT enough?
  Constraint maxV, minV;
  maxV.addBig(var.NameExpression(MAX, USED));
  maxV.addSmall(std::numeric_limits<int>::max());
  maxV.SetBlame(blame);
  cp_.AddConstraint(maxV);
  minV.addSmall(var.NameExpression(MIN, USED));
  minV.addBig(std::numeric_limits<int>::min());
  minV.SetBlame(blame);
  cp_.AddConstraint(minV);
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

        GenerateGenericConstraint(buf, argument, "malloc " + getStmtLoc(S));
        return true;
      }
    }
  }


  if (BinaryOperator* op = dyn_cast<BinaryOperator>(S)) {
    if (DeclRefExpr* declRef = dyn_cast<DeclRefExpr>(op->getLHS())) {
      Integer intLiteral(declRef->getDecl());
      if (op->getLHS()->getType()->isIntegerType() && op->getRHS()->getType()->isIntegerType()) {
        if (op->isAssignmentOp()) {
          GenerateGenericConstraint(intLiteral, op->getRHS(), "int assignment " + getStmtLoc(S));
        }
        if (op-> isCompoundAssignmentOp()) {
          GenerateUnboundConstraint(intLiteral, "compound int assignment " + getStmtLoc(S));
        }
      }
    }
  }

  if (UnaryOperator* op = dyn_cast<UnaryOperator>(S)) {
    if (op->getSubExpr()->getType()->isIntegerType() && op->isIncrementDecrementOp()) {
      if (DeclRefExpr* declRef = dyn_cast<DeclRefExpr>(op->getSubExpr())) {
        Integer intLiteral(declRef->getDecl());
        GenerateUnboundConstraint(intLiteral, "int inc/dec " + getStmtLoc(S));
      }
    }
  }

  return true;
}

void ConstraintGenerator::GenerateGenericConstraint(const VarLiteral &var, Expr *integerExpression,
                                                    const string &blame, ExpressionType type) {
  vector<Constraint::Expression> maxExprs = GenerateIntegerExpression(integerExpression, true);
  for (size_t i = 0; i < maxExprs.size(); ++i) {
    Constraint allocMax;
    allocMax.addBig(var.NameExpression(MAX, type));
    allocMax.addSmall(maxExprs[i]);
    allocMax.SetBlame(blame);
    cp_.AddConstraint(allocMax);
    log::os() << "Adding - " << var.NameExpression(MAX, type) << " >= "
              << maxExprs[i].toString() << endl;
  }

  vector<Constraint::Expression> minExprs = GenerateIntegerExpression(integerExpression, false);
  for (size_t i = 0; i < minExprs.size(); ++i) {
    Constraint allocMin;
    allocMin.addSmall(var.NameExpression(MIN, type));
    allocMin.addBig(minExprs[i]);
    allocMin.SetBlame(blame);
    cp_.AddConstraint(allocMin);
    log::os() << "Adding - " << var.NameExpression(MIN, type) << " <= "
              << minExprs[i].toString() << endl;
  }
}

} // namespace boa
