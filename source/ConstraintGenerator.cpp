#include "ConstraintGenerator.h"

#include <vector>
#include <limits>
#include <string>

using std::vector;
using std::string;

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
    ce.add(intLiteral.NameExpression(max ? VarLiteral::MAX : VarLiteral::MIN));
    result.push_back(ce);
    return result;
  }

  if (CallExpr* funcCall = dyn_cast<CallExpr>(expr)) {
    if (FunctionDecl* funcDec = funcCall->getDirectCallee()) {
      string funcName = funcDec->getNameInfo().getAsString();
      if (funcName == "strlen") {
        Expr* argument = funcCall->getArg(0);
        while (CastExpr *cast = dyn_cast<CastExpr>(argument)) {
          argument = cast->getSubExpr();
        }
        if (DeclRefExpr *declRef = dyn_cast<DeclRefExpr>(argument)) {
          Pointer p(declRef->getDecl()); // Treat all args as pointers. A buffer is cast to char*
          ce.add(p.NameExpression(max ? VarLiteral::MAX : VarLiteral::MIN, VarLiteral::LEN_READ));
          result.push_back(ce);
          return result;
        }
      }
    }
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
        } else if ((RHS.size() == 1) && (RHS[0].IsConst())) {
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
        LOG << "Multiplication of two non-const integer expressions " << getStmtLoc(expr) << endl;
        return result;
      }
      case BO_Div : {
        vector<Constraint::Expression> LHS = GenerateIntegerExpression(op->getLHS(), max);
        vector<Constraint::Expression> RHS = GenerateIntegerExpression(op->getRHS(), max);

        // We can only handle linear constraints, so this method ignores RHS expressions that are
        // non const.
        if ((RHS.size() == 1) && (RHS[0].IsConst())) {
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

        LOG << "Non linear RHS expression " << getStmtLoc(expr) << endl;
        return result;
      }
      default : break;
    }
  }

  expr->dump();
  LOG << "Can't generate integer expression " << getStmtLoc(expr) << endl;
  return result;
}

bool ConstraintGenerator::GenerateArraySubscriptConstraints(ArraySubscriptExpr* expr) {
  VarLiteral* varLiteral = NULL;

  Expr* base = expr->getBase();
  while (dyn_cast<CastExpr>(base)) {
    base = dyn_cast<CastExpr>(base)->getSubExpr();
  }

  if (DeclRefExpr *declRef = dyn_cast<DeclRefExpr>(base)) {
    if (ArrayType* arr = dyn_cast<ArrayType>(declRef->getDecl()->getType().getTypePtr())) {
      if (arr->getElementType()->isAnyCharacterType()) {
        varLiteral = new Buffer(declRef->getDecl());
      }
    } else if (PointerType* pType =
        dyn_cast<PointerType>(declRef->getDecl()->getType().getTypePtr())) {
      if (pType->getPointeeType()->isAnyCharacterType()) {
        varLiteral = new Pointer(declRef->getDecl());
      }
    }
  } else {
    // TODO: Any other cases?
  }

  if (NULL == varLiteral) {
    expr->dump();
    return false;
  }

  GenerateGenericConstraint(*varLiteral, expr->getIdx(),
      "array subscript " + getStmtLoc(expr), VarLiteral::USED);

  delete varLiteral;
  return true;
}

void ConstraintGenerator::GenerateVarDeclConstraints(VarDecl *var) {
  if (ConstantArrayType* arr = dyn_cast<ConstantArrayType>(var->getType().getTypePtr())) {
    if (arr->getElementType()->isAnyCharacterType()) {
      Buffer buf(var);
      Constraint allocMax, allocMin;

      allocMax.addBig(buf.NameExpression(VarLiteral::MAX, VarLiteral::ALLOC));
      allocMax.addSmall(arr->getSize().getLimitedValue());
      allocMax.SetBlame("static char buffer declaration " + getStmtLoc(var));
      cp_.AddConstraint(allocMax);
      LOG << "Adding - " << buf.NameExpression(VarLiteral::MAX, VarLiteral::ALLOC) << " >= " <<
                    arr->getSize().getLimitedValue() << "\n";

      allocMin.addSmall(buf.NameExpression(VarLiteral::MIN, VarLiteral::ALLOC));
      allocMin.addBig(arr->getSize().getLimitedValue());
      allocMin.SetBlame("static char buffer declaration " + getStmtLoc(var));
      LOG << "Adding - " << buf.NameExpression(VarLiteral::MIN, VarLiteral::ALLOC) << " <= " <<
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
    LOG << "Integer definition without initializer on " << getStmtLoc(var) << endl;
  }
}

void ConstraintGenerator::GenerateStringLiteralConstraints(StringLiteral *stringLiteral) {
  Buffer buf(stringLiteral);
  Constraint allocMax, allocMin, lenMax, lenMin;

  allocMax.addBig(buf.NameExpression(VarLiteral::MAX, VarLiteral::ALLOC));
  allocMax.addSmall(stringLiteral->getByteLength() + 1);
  allocMax.SetBlame("string literal buffer declaration " + getStmtLoc(stringLiteral));
  cp_.AddConstraint(allocMax);
  LOG << "Adding - " << buf.NameExpression(VarLiteral::MAX, VarLiteral::ALLOC) << " >= " <<
                stringLiteral->getByteLength() + 1 << "\n";

  allocMin.addSmall(buf.NameExpression(VarLiteral::MIN, VarLiteral::ALLOC));
  allocMin.addBig(stringLiteral->getByteLength() + 1);
  allocMin.SetBlame("string literal buffer declaration " + getStmtLoc(stringLiteral));
  LOG << "Adding - " << buf.NameExpression(VarLiteral::MIN, VarLiteral::ALLOC) << " <= " <<
               stringLiteral->getByteLength() + 1 << "\n";
  cp_.AddConstraint(allocMin);

  lenMax.addBig(buf.NameExpression(VarLiteral::MAX, VarLiteral::LEN_READ));
  lenMax.addSmall(stringLiteral->getByteLength());
  lenMax.SetBlame("string literal buffer declaration " + getStmtLoc(stringLiteral));
  cp_.AddConstraint(lenMax);
  LOG << "Adding - " << buf.NameExpression(VarLiteral::MAX, VarLiteral::LEN_READ) << " >= " <<
                stringLiteral->getByteLength() << "\n";

  lenMin.addSmall(buf.NameExpression(VarLiteral::MIN, VarLiteral::LEN_READ));
  lenMin.addBig(stringLiteral->getByteLength());
  lenMin.SetBlame("string literal buffer declaration " + getStmtLoc(stringLiteral));
  LOG << "Adding - " << buf.NameExpression(VarLiteral::MIN, VarLiteral::LEN_READ) << " <= " <<
               stringLiteral->getByteLength() << "\n";
  cp_.AddConstraint(allocMin);
}

void ConstraintGenerator::GenerateUnboundConstraint(const Integer &var, const string &blame) {
  // FIXME - is VarLiteral::MAX_INT enough?
  Constraint maxV, minV;
  maxV.addBig(var.NameExpression(VarLiteral::MAX, VarLiteral::USED));
  maxV.addSmall(std::numeric_limits<int>::max());
  maxV.SetBlame(blame);
  cp_.AddConstraint(maxV);
  minV.addSmall(var.NameExpression(VarLiteral::MIN, VarLiteral::USED));
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

  if (StringLiteral* stringLiteral = dyn_cast<StringLiteral>(S)) {
    GenerateStringLiteralConstraints(stringLiteral);
  }

  if (CallExpr* funcCall = dyn_cast<CallExpr>(S)) {
    if (FunctionDecl* funcDec = funcCall->getDirectCallee()) {
      string funcName = funcDec->getNameInfo().getAsString();
      if (funcName == "malloc") {
        Buffer buf(funcCall);
        Expr* argument = funcCall->getArg(0);
        while (CastExpr *cast = dyn_cast<CastExpr>(argument)) {
          argument = cast->getSubExpr();
        }

        GenerateGenericConstraint(buf, argument, "malloc " + getStmtLoc(S));
        return true;
      }
    }
  }


  if (BinaryOperator* op = dyn_cast<BinaryOperator>(S)) {
    if (DeclRefExpr* declRef = dyn_cast<DeclRefExpr>(op->getLHS())) {
      if (declRef->getType()->isIntegerType() && op->getRHS()->getType()->isIntegerType()) {
        Integer intLiteral(declRef->getDecl());
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
                                                    const string &blame,
                                                    VarLiteral::ExpressionType type) {
  vector<Constraint::Expression> maxExprs = GenerateIntegerExpression(integerExpression, true);
  for (size_t i = 0; i < maxExprs.size(); ++i) {
    Constraint allocMax;
    allocMax.addBig(var.NameExpression(VarLiteral::MAX, type));
    allocMax.addSmall(maxExprs[i]);
    allocMax.SetBlame(blame);
    cp_.AddConstraint(allocMax);
    LOG << "Adding - " << var.NameExpression(VarLiteral::MAX, type) << " >= "
              << maxExprs[i].toString() << endl;
  }

  vector<Constraint::Expression> minExprs = GenerateIntegerExpression(integerExpression, false);
  for (size_t i = 0; i < minExprs.size(); ++i) {
    Constraint allocMin;
    allocMin.addSmall(var.NameExpression(VarLiteral::MIN, type));
    allocMin.addBig(minExprs[i]);
    allocMin.SetBlame(blame);
    cp_.AddConstraint(allocMin);
    LOG << "Adding - " << var.NameExpression(VarLiteral::MIN, type) << " <= "
              << minExprs[i].toString() << endl;
  }
}

} // namespace boa
