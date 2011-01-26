#include "ConstraintGenerator.h"
#include <vector>
#include <map>
#include "Integer.h"

//#include <limits>
//#include <string>

using std::pair;
typedef boa::Constraint::Expression Expression;

//using std::string;

#include "llvm/User.h"

using namespace llvm;


namespace boa {

void ConstraintGenerator::VisitInstruction(const Instruction *I) {
  if (const DbgDeclareInst *D = dyn_cast<const DbgDeclareInst>(I)) {
    SaveDbgDeclare(D);
    return;
  }

  switch (I->getOpcode()) {
  // Terminators
//  case Instruction::Ret:
//  case Instruction::Br:
//  case Instruction::Switch:
//  case Instruction::IndirectBr:
//  case Instruction::Invoke:
//  case Instruction::Unwind:
//  case Instruction::Unreachable:

  // Standard binary operators...
  case Instruction::Add:
    GenerateAddConstraint(dyn_cast<const BinaryOperator>(I));
    break;
//  case Instruction::FAdd:
  case Instruction::Sub:
    GenerateSubConstraint(dyn_cast<const BinaryOperator>(I));
    break;
//  case Instruction::FSub:
  case Instruction::Mul:
    GenerateMulConstraint(dyn_cast<const BinaryOperator>(I));
    break;
//  case Instruction::FMul:
//  case Instruction::UDiv:
  case Instruction::SDiv:
    GenerateDivConstraint(dyn_cast<const BinaryOperator>(I));
    break;
//  case Instruction::FDiv:
//  case Instruction::URem:
//  case Instruction::SRem:
//  case Instruction::FRem:

  // Logical operators...
//  case Instruction::And:
//  case Instruction::Or :
//  case Instruction::Xor:

  // Memory instructions...
  case Instruction::Alloca:
    GenerateAllocConstraint(dyn_cast<const AllocaInst>(I));
    break;
  case Instruction::Load:
    GenerateLoadConstraint(dyn_cast<const LoadInst>(I));
    break;
  case Instruction::Store:
    GenerateStoreConstraint(dyn_cast<const StoreInst>(I));
    break;
  case Instruction::GetElementPtr:
    GenerateArraySubscriptConstraint(dyn_cast<const GetElementPtrInst>(I));
    break;

  // Convert instructions...
//  case Instruction::Trunc:
//  case Instruction::ZExt:
  case Instruction::SExt:
    GenerateSExtConstraint(dyn_cast<const SExtInst>(I));
    break;
//  case Instruction::FPTrunc:
//  case Instruction::FPExt:
//  case Instruction::FPToUI:
//  case Instruction::FPToSI:
//  case Instruction::UIToFP:
//  case Instruction::SIToFP:
//  case Instruction::IntToPtr:
//  case Instruction::PtrToInt:
//  case Instruction::BitCast:

  // Other instructions...
//  case Instruction::ICmp:
//  case Instruction::FCmp:
//  case Instruction::PHI:
//  case Instruction::Select:
//  case Instruction::Call:
//  case Instruction::Shl:
//  case Instruction::LShr:
//  case Instruction::AShr:
//  case Instruction::VAArg:
//  case Instruction::ExtractElement:
//  case Instruction::InsertElement:
//  case Instruction::ShuffleVector:
//  case Instruction::ExtractValue:
//  case Instruction::InsertValue:

  default :
    LOG << "unhandled instruction - " << endl;
    I->dump();
    break; //TODO
  }
}

void ConstraintGenerator::GenerateAddConstraint(const BinaryOperator* I) {
  Expression maxResult, minResult;
  maxResult.add(GenerateIntegerExpression(I->getOperand(0), VarLiteral::MAX));
  minResult.add(GenerateIntegerExpression(I->getOperand(0), VarLiteral::MIN));
  maxResult.add(GenerateIntegerExpression(I->getOperand(1), VarLiteral::MAX));
  minResult.add(GenerateIntegerExpression(I->getOperand(1), VarLiteral::MIN));

  Integer intLiteral(I);

  Constraint maxCons, minCons;
  maxCons.addBig(intLiteral.NameExpression(VarLiteral::MAX));
  maxCons.addSmall(maxResult);
  minCons.addSmall(intLiteral.NameExpression(VarLiteral::MIN));
  minCons.addBig(minResult);
  cp_.AddConstraint(maxCons);
  LOG << "Adding - " << intLiteral.NameExpression(VarLiteral::MAX) << " >= "
            << maxResult.toString() << endl;

  cp_.AddConstraint(minCons);
  LOG << "Adding - " << intLiteral.NameExpression(VarLiteral::MIN) << " <= "
          << minResult.toString() << endl;
}


void ConstraintGenerator::GenerateSubConstraint(const BinaryOperator* I) {

  Expression maxResult, minResult;
  maxResult.add(GenerateIntegerExpression(I->getOperand(0), VarLiteral::MAX));
  minResult.add(GenerateIntegerExpression(I->getOperand(0), VarLiteral::MIN));
  maxResult.add(GenerateIntegerExpression(I->getOperand(1), VarLiteral::MIN));
  minResult.add(GenerateIntegerExpression(I->getOperand(1), VarLiteral::MAX));

  Integer intLiteral(I);

  Constraint maxCons, minCons;
  maxCons.addBig(intLiteral.NameExpression(VarLiteral::MAX));
  maxCons.addSmall(maxResult);
  minCons.addSmall(intLiteral.NameExpression(VarLiteral::MIN));
  minCons.addBig(minResult);
  cp_.AddConstraint(maxCons);
  LOG << "Adding - " << intLiteral.NameExpression(VarLiteral::MAX) << " >= "
            << maxResult.toString() << endl;
  cp_.AddConstraint(minCons);
  LOG << "Adding - " << intLiteral.NameExpression(VarLiteral::MIN) << " <= "
        << minResult.toString() << endl;
}

void ConstraintGenerator::GenerateMulConstraint(const BinaryOperator* I) {
  Integer intLiteral(I);
  Expression operand0Max = GenerateIntegerExpression(I->getOperand(0), VarLiteral::MAX);
  Expression operand1Max = GenerateIntegerExpression(I->getOperand(1), VarLiteral::MAX);
  Expression operand0Min = GenerateIntegerExpression(I->getOperand(0), VarLiteral::MIN);
  Expression operand1Min = GenerateIntegerExpression(I->getOperand(1), VarLiteral::MIN);

  Expression *minOperand, *maxOperand;
  double constOperand;

  if (operand0Max.IsConst()) {
    constOperand = operand0Max.GetConst();
    minOperand = &operand1Min;
    maxOperand = &operand1Max;
  }
  else if (operand1Max.IsConst()) {
    constOperand = operand1Max.GetConst();
    minOperand = &operand0Min;
    maxOperand = &operand0Max;
  }
  else {
    GenerateUnboundConstraint(intLiteral, "Unconst multiplication.");
    return;
  }

  minOperand->mul(constOperand);
  maxOperand->mul(constOperand);

  Constraint maxCons1, minCons1, maxCons2, minCons2;
  maxCons1.addBig(intLiteral.NameExpression(VarLiteral::MAX));
  maxCons1.addSmall(*maxOperand);
  maxCons2.addBig(intLiteral.NameExpression(VarLiteral::MAX));
  maxCons2.addSmall(*minOperand);
  minCons1.addSmall(intLiteral.NameExpression(VarLiteral::MIN));
  minCons1.addBig(*minOperand);
  minCons2.addSmall(intLiteral.NameExpression(VarLiteral::MIN));
  minCons2.addBig(*maxOperand);
  cp_.AddConstraint(maxCons1);
  LOG << "Adding - " << intLiteral.NameExpression(VarLiteral::MAX) << " >= "
            << maxOperand->toString() << endl;
  cp_.AddConstraint(maxCons2);
  LOG << "Adding - " << intLiteral.NameExpression(VarLiteral::MAX) << " >= "
            << minOperand->toString() << endl;
  cp_.AddConstraint(minCons1);
  LOG << "Adding - " << intLiteral.NameExpression(VarLiteral::MIN) << " <= "
            << maxOperand->toString() << endl;
  cp_.AddConstraint(minCons2);
  LOG << "Adding - " << intLiteral.NameExpression(VarLiteral::MIN) << " <= "
            << minOperand->toString() << endl;
}

void ConstraintGenerator::GenerateDivConstraint(const BinaryOperator* I) {
  Integer intLiteral(I);
  Expression operand1 = GenerateIntegerExpression(I->getOperand(1), VarLiteral::MAX);
  if (!operand1.IsConst()) {
    GenerateUnboundConstraint(intLiteral, "Non-const denominator.");
    return;
  }
  double constOperand = operand1.GetConst();
  Expression minOperand = GenerateIntegerExpression(I->getOperand(0), VarLiteral::MAX);
  Expression maxOperand = GenerateIntegerExpression(I->getOperand(0), VarLiteral::MIN);

  minOperand.div(constOperand);
  maxOperand.div(constOperand);

  Constraint maxCons1, minCons1, maxCons2, minCons2;
  maxCons1.addBig(intLiteral.NameExpression(VarLiteral::MAX));
  maxCons1.addSmall(maxOperand);
  maxCons2.addBig(intLiteral.NameExpression(VarLiteral::MAX));
  maxCons2.addSmall(minOperand);
  minCons1.addSmall(intLiteral.NameExpression(VarLiteral::MIN));
  minCons1.addBig(minOperand);
  minCons2.addSmall(intLiteral.NameExpression(VarLiteral::MIN));
  minCons2.addBig(maxOperand);
  cp_.AddConstraint(maxCons1);
  LOG << "Adding - " << intLiteral.NameExpression(VarLiteral::MAX) << " >= "
            << maxOperand.toString() << endl;
  cp_.AddConstraint(maxCons2);
  LOG << "Adding - " << intLiteral.NameExpression(VarLiteral::MAX) << " >= "
            << minOperand.toString() << endl;
  cp_.AddConstraint(minCons1);
  LOG << "Adding - " << intLiteral.NameExpression(VarLiteral::MIN) << " <= "
            << maxOperand.toString() << endl;
  cp_.AddConstraint(minCons2);
  LOG << "Adding - " << intLiteral.NameExpression(VarLiteral::MIN) << " <= "
            << minOperand.toString() << endl;
}

void ConstraintGenerator::GenerateSExtConstraint(const SExtInst* I) {
  Integer intLiteral(I);
  GenerateGenericConstraint(intLiteral, I->getOperand(0), "sign extension instruction");
}

void ConstraintGenerator::GenerateStoreConstraint(const StoreInst* I) {
  Integer intLiteral(I->getPointerOperand());
  GenerateGenericConstraint(intLiteral, I->getValueOperand(), "store instruction");
}

void ConstraintGenerator::GenerateLoadConstraint(const LoadInst* I) {
  Integer intLiteral(I);
  GenerateGenericConstraint(intLiteral, I->getPointerOperand(), "load instruction");
}

void ConstraintGenerator::SaveDbgDeclare(const DbgDeclareInst* D) {
  // FIXME - magic numbers!
  if (const MDString *S = dyn_cast<const MDString>(D->getVariable()->getOperand(2))) {
    if (const MDNode *node = dyn_cast<const MDNode>(D->getVariable()->getOperand(3))) {
      if (const MDString *file = dyn_cast<const MDString>(node->getOperand(1))) {
        LOG << (void*)D->getAddress() << " name = " << S->getString().str() << " Source location - "
            << file->getString().str() << ":" << D->getDebugLoc().getLine() << endl;

        Buffer b(D->getAddress(), S->getString().str(), file->getString().str(),
                 D->getDebugLoc().getLine());

        buffers[D->getAddress()] = b;
        return;
      }
    }
  }
  // else
  LOG << "Can't extract debug info\n";
}

void ConstraintGenerator::GenerateAllocConstraint(const AllocaInst *I) {
  if (const PointerType *pType = dyn_cast<const PointerType>(I->getType())) {
    if (const ArrayType *aType = dyn_cast<const ArrayType>(pType->getElementType())) {
      Buffer buf(I);
      double allocSize = aType->getNumElements();
      Constraint allocMax, allocMin;

      allocMax.addBig(buf.NameExpression(VarLiteral::MAX, VarLiteral::ALLOC));
      allocMax.addSmall(allocSize);
//        allocMax.SetBlame("static char buffer declaration " + getStmtLoc(var));
      cp_.AddConstraint(allocMax);
      LOG << "Adding - " << buf.NameExpression(VarLiteral::MAX, VarLiteral::ALLOC) << " >= " <<
                    allocSize << "\n";

      allocMin.addSmall(buf.NameExpression(VarLiteral::MIN, VarLiteral::ALLOC));
      allocMin.addBig(allocSize);
//        allocMin.SetBlame("static char buffer declaration " + getStmtLoc(var));
      LOG << "Adding - " << buf.NameExpression(VarLiteral::MIN, VarLiteral::ALLOC) << " <= " <<
                   allocSize << "\n";
      cp_.AddConstraint(allocMin);
    }
  }
}

void ConstraintGenerator::GenerateArraySubscriptConstraint(const GetElementPtrInst *I) {
  Buffer b = buffers[I->getPointerOperand()];
  if (b.IsNull()) {
    LOG << " ERROR - trying to add a buffer before buffer definition" << endl;
    return;
  }
  GenerateGenericConstraint(b, *(I->idx_begin()+1), "array subscript", VarLiteral::USED);
}


void ConstraintGenerator::GenerateGenericConstraint(const VarLiteral &var, const Value *integerExpression,
                                                    const string &blame,
                                                    VarLiteral::ExpressionType type) {
  if (type == VarLiteral::USED && var.IsBuffer()) {
    Buffer& buf = (Buffer&)var;
    cp_.AddBuffer(buf);
    LOG << " Adding buffer to problem" << endl;
  }

  Expression maxExpr = GenerateIntegerExpression(integerExpression, VarLiteral::MAX);
  Constraint allocMax;
  allocMax.addBig(var.NameExpression(VarLiteral::MAX, type));
  allocMax.addSmall(maxExpr);
  allocMax.SetBlame(blame);
  cp_.AddConstraint(allocMax);
  LOG << "Adding - " << var.NameExpression(VarLiteral::MAX, type) << " >= "
            << maxExpr.toString() << endl;

  Expression minExpr = GenerateIntegerExpression(integerExpression, VarLiteral::MIN);
  Constraint allocMin;
  allocMin.addSmall(var.NameExpression(VarLiteral::MIN, type));
  allocMin.addBig(minExpr);
  allocMin.SetBlame(blame);
  cp_.AddConstraint(allocMin);
  LOG << "Adding - " << var.NameExpression(VarLiteral::MIN, type) << " <= "
            << minExpr.toString() << endl;
}

Constraint::Expression ConstraintGenerator::GenerateIntegerExpression(const Value *expr,
                                                            VarLiteral::ExpressionDir dir) {
  Expression result;

  if (const ConstantInt *literal = dyn_cast<const ConstantInt>(expr)) {
    result.add(literal->getLimitedValue());
    return result;
  }

//  if (CallExpr* funcCall = dyn_cast<CallExpr>(expr)) {
//    if (FunctionDecl* funcDec = funcCall->getDirectCallee()) {
//      string funcName = funcDec->getNameInfo().getAsString();
//      if (funcName == "strlen") {
//        Expr* argument = funcCall->getArg(0);
//        while (CastExpr *cast = dyn_cast<CastExpr>(argument)) {
//          argument = cast->getSubExpr();
//        }
//        if (DeclRefExpr *declRef = dyn_cast<DeclRefExpr>(argument)) {
//          Pointer p(declRef->getDecl()); // Treat all args as pointers. A buffer is cast to char*
//          ce.add(p.NameExpression(max ? VarLiteral::MAX : VarLiteral::MIN, VarLiteral::LEN_READ));
//          result.push_back(ce);
//          return result;
//        }
//      }
//    }
//  }

  // Otherwise, this is a reference to another var definition
  Integer intLiteral(expr);
  result.add(intLiteral.NameExpression(dir));
  return result;
}


//void ConstraintGenerator::GenerateVarDeclConstraints(VarDecl *var) {
//  if (ConstantArrayType* arr = dyn_cast<ConstantArrayType>(var->getType().getTypePtr())) {
//    if (arr->getElementType()->isAnyCharacterType()) {
//      Buffer buf(var);
//      Constraint allocMax, allocMin;

//      allocMax.addBig(buf.NameExpression(VarLiteral::MAX, VarLiteral::ALLOC));
//      allocMax.addSmall(arr->getSize().getLimitedValue());
//      allocMax.SetBlame("static char buffer declaration " + getStmtLoc(var));
//      cp_.AddConstraint(allocMax);
//      LOG << "Adding - " << buf.NameExpression(VarLiteral::MAX, VarLiteral::ALLOC) << " >= " <<
//                    arr->getSize().getLimitedValue() << "\n";

//      allocMin.addSmall(buf.NameExpression(VarLiteral::MIN, VarLiteral::ALLOC));
//      allocMin.addBig(arr->getSize().getLimitedValue());
//      allocMin.SetBlame("static char buffer declaration " + getStmtLoc(var));
//      LOG << "Adding - " << buf.NameExpression(VarLiteral::MIN, VarLiteral::ALLOC) << " <= " <<
//                   arr->getSize().getLimitedValue() << "\n";
//      cp_.AddConstraint(allocMin);
//    }
//  }

//  if (var->getType()->isIntegerType()) {
//    Integer intLiteral(var);
//    if (var->hasInit()) {
//      vector<Expression> maxInits  = GenerateIntegerExpression(var->getInit(), true);
//      if (!maxInits.empty()) {
//        GenerateGenericConstraint(intLiteral, var->getInit(), "int declaration " + getStmtLoc(var));
//        return;
//      }
//    }

//    GenerateUnboundConstraint(intLiteral, "int without initializer " + getStmtLoc(var));
//    LOG << "Integer definition without initializer on " << getStmtLoc(var) << endl;
//  }
//}

//void ConstraintGenerator::GenerateStringLiteralConstraints(StringLiteral *stringLiteral) {
//  Buffer buf(stringLiteral);
//  Constraint allocMax, allocMin, lenMax, lenMin;

//  allocMax.addBig(buf.NameExpression(VarLiteral::MAX, VarLiteral::ALLOC));
//  allocMax.addSmall(stringLiteral->getByteLength() + 1);
//  allocMax.SetBlame("string literal buffer declaration " + getStmtLoc(stringLiteral));
//  cp_.AddConstraint(allocMax);
//  LOG << "Adding - " << buf.NameExpression(VarLiteral::MAX, VarLiteral::ALLOC) << " >= " <<
//                stringLiteral->getByteLength() + 1 << "\n";

//  allocMin.addSmall(buf.NameExpression(VarLiteral::MIN, VarLiteral::ALLOC));
//  allocMin.addBig(stringLiteral->getByteLength() + 1);
//  allocMin.SetBlame("string literal buffer declaration " + getStmtLoc(stringLiteral));
//  LOG << "Adding - " << buf.NameExpression(VarLiteral::MIN, VarLiteral::ALLOC) << " <= " <<
//               stringLiteral->getByteLength() + 1 << "\n";
//  cp_.AddConstraint(allocMin);

//  lenMax.addBig(buf.NameExpression(VarLiteral::MAX, VarLiteral::LEN_READ));
//  lenMax.addSmall(stringLiteral->getByteLength());
//  lenMax.SetBlame("string literal buffer declaration " + getStmtLoc(stringLiteral));
//  cp_.AddConstraint(lenMax);
//  LOG << "Adding - " << buf.NameExpression(VarLiteral::MAX, VarLiteral::LEN_READ) << " >= " <<
//                stringLiteral->getByteLength() << "\n";

//  lenMin.addSmall(buf.NameExpression(VarLiteral::MIN, VarLiteral::LEN_READ));
//  lenMin.addBig(stringLiteral->getByteLength());
//  lenMin.SetBlame("string literal buffer declaration " + getStmtLoc(stringLiteral));
//  LOG << "Adding - " << buf.NameExpression(VarLiteral::MIN, VarLiteral::LEN_READ) << " <= " <<
//               stringLiteral->getByteLength() << "\n";
//  cp_.AddConstraint(allocMin);
//}

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

//bool ConstraintGenerator::VisitStmt(Stmt* S) {
//  return VisitStmt(S, NULL);
//}

//bool ConstraintGenerator::VisitStmt(Stmt* S, FunctionDecl* context) {
//  if (ArraySubscriptExpr* expr = dyn_cast<ArraySubscriptExpr>(S)) {
//    return GenerateArraySubscriptConstraints(expr);
//  }

//  if (ReturnStmt *ret = dyn_cast<ReturnStmt>(S)) {
//    LOG << "RETRUN!! 1" << endl;

//    if (/*(ret->getRetValue()->getType()->isIntegerType()) &&*/ (context != NULL)) {
//      Integer intLiteral(context);
//      LOG << "RETRUN!! 2" << endl;
//      GenerateGenericConstraint(intLiteral, ret->getRetValue(), "Integer return " + getStmtLoc(S));
//    }
//  }

//  if (DeclStmt* dec = dyn_cast<DeclStmt>(S)) {
//    for (DeclGroupRef::iterator decIt = dec->decl_begin(); decIt != dec->decl_end(); ++decIt) {
//      if (VarDecl* var = dyn_cast<VarDecl>(*decIt)) {
//        GenerateVarDeclConstraints(var);
//      }
//    }
//  }

//  if (StringLiteral* stringLiteral = dyn_cast<StringLiteral>(S)) {
//    GenerateStringLiteralConstraints(stringLiteral);
//  }

//  if (CallExpr* funcCall = dyn_cast<CallExpr>(S)) {
//    if (FunctionDecl* funcDec = funcCall->getDirectCallee()) {
//      string funcName = funcDec->getNameInfo().getAsString();
//      if (funcName == "malloc") {
//        Buffer buf(funcCall);
//        Expr* argument = funcCall->getArg(0);
//        while (CastExpr *cast = dyn_cast<CastExpr>(argument)) {
//          argument = cast->getSubExpr();
//        }

//        GenerateGenericConstraint(buf, argument, "malloc " + getStmtLoc(S));
//        return true;
//      }

//      // General function call
//      if (funcDec->hasBody()) {
//        LOG << "GO!" << endl;
//        VisitStmt(funcDec->getBody(), funcDec);
//      }
//      for (unsigned i = 0; i < funcDec->param_size(); ++i) {
//        VarDecl *var = funcDec->getParamDecl(i);
//        if (var->getType()->isIntegerType()) {
//          Integer intLiteral(var);
//          GenerateGenericConstraint(intLiteral, funcCall->getArg(i), "int parameter " + getStmtLoc(var));
//        }
//      }
//    }
//  }


//  if (BinaryOperator* op = dyn_cast<BinaryOperator>(S)) {
//    if (DeclRefExpr* declRef = dyn_cast<DeclRefExpr>(op->getLHS())) {
//      if (declRef->getType()->isIntegerType() && op->getRHS()->getType()->isIntegerType()) {
//        Integer intLiteral(declRef->getDecl());
//        if (op->isAssignmentOp()) {
//          GenerateGenericConstraint(intLiteral, op->getRHS(), "int assignment " + getStmtLoc(S));
//        }
//        if (op-> isCompoundAssignmentOp()) {
//          GenerateUnboundConstraint(intLiteral, "compound int assignment " + getStmtLoc(S));
//        }
//      }
//    }
//  }

//  if (UnaryOperator* op = dyn_cast<UnaryOperator>(S)) {
//    if (op->getSubExpr()->getType()->isIntegerType() && op->isIncrementDecrementOp()) {
//      if (DeclRefExpr* declRef = dyn_cast<DeclRefExpr>(op->getSubExpr())) {
//        Integer intLiteral(declRef->getDecl());
//        GenerateUnboundConstraint(intLiteral, "int inc/dec " + getStmtLoc(S));
//      }
//    }
//  }

//  if (CompoundStmt *comp = dyn_cast<CompoundStmt>(S)) {
//    for (Stmt** it = comp->body_begin(); it != comp->body_end(); ++it) {
//      VisitStmt(*it, context);
//    }
//  }

//  return true;
//}

} // namespace boa
