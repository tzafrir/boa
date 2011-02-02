#include "ConstraintGenerator.h"
#include <vector>
#include <map>
#include "Integer.h"
#include "Pointer.h"
#include "llvm/Constants.h"
#include "llvm/LLVMContext.h"
#include "llvm/User.h"

using std::pair;
typedef boa::Constraint::Expression Expression;

using namespace llvm;


namespace boa {

void ConstraintGenerator::AddBuffer(const Buffer& buf) {
  cp_.AddBuffer(buf);

  Constraint cReadMax, cWriteMax;
  cReadMax.addBig(buf.NameExpression(VarLiteral::MAX, VarLiteral::LEN_READ));
  cReadMax.addSmall(buf.NameExpression(VarLiteral::MAX, VarLiteral::USED));
  cp_.AddConstraint(cReadMax);

  cWriteMax.addBig(buf.NameExpression(VarLiteral::MAX, VarLiteral::USED));
  cWriteMax.addSmall(buf.NameExpression(VarLiteral::MAX, VarLiteral::LEN_WRITE));
  cp_.AddConstraint(cWriteMax);

  Constraint cReadMin, cWriteMin;
  cReadMin.addSmall(buf.NameExpression(VarLiteral::MIN, VarLiteral::LEN_READ));
  cReadMin.addBig(buf.NameExpression(VarLiteral::MIN, VarLiteral::USED));
  cp_.AddConstraint(cReadMin);

  cWriteMin.addSmall(buf.NameExpression(VarLiteral::MIN, VarLiteral::USED));
  cWriteMin.addBig(buf.NameExpression(VarLiteral::MIN, VarLiteral::LEN_WRITE));
  cp_.AddConstraint(cWriteMin);
  // TODO - LOG
}

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
    GenerateAllocaConstraint(dyn_cast<const AllocaInst>(I));
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
  case Instruction::Call:
    GenerateCallConstraint(dyn_cast<const CallInst>(I));
    break;
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

void ConstraintGenerator::VisitGlobal(const GlobalValue *G) {
  const Type *t = G->getType();
  if (const PointerType *p = dyn_cast<const PointerType>(t)) {
    t = p->getElementType();
  } else {
    LOG << "can't handle global variable" << endl;
    return;
  }
  if (const ArrayType *ar = dyn_cast<const ArrayType>(t)) {
    // string literals are global arrays
    unsigned len = ar->getNumElements();
    Buffer buf(G, "string literal", "", 0); // TODO - file? line?
    LOG << "Adding string literal. Len - " << len <<  " at " << (void*)G << endl;

    GenerateAllocConstraint(G, ar);

    Constraint lenMax, lenMin;

    lenMax.addBig(buf.NameExpression(VarLiteral::MAX, VarLiteral::LEN_WRITE));
    lenMax.addSmall(len - 1);
//    lenMax.SetBlame("string literal buffer declaration " + getStmtLoc(stringLiteral));
    cp_.AddConstraint(lenMax);
    LOG << "Adding - " << buf.NameExpression(VarLiteral::MAX, VarLiteral::LEN_WRITE) << " >= " <<
                  (len - 1) << "\n";

    lenMin.addSmall(buf.NameExpression(VarLiteral::MIN, VarLiteral::LEN_WRITE));
    lenMin.addBig(len - 1);
//    lenMin.SetBlame("string literal buffer declaration " + getStmtLoc(stringLiteral));
    cp_.AddConstraint(lenMin);
    LOG << "Adding - " << buf.NameExpression(VarLiteral::MIN, VarLiteral::LEN_WRITE) << " <= " <<
                 (len - 1) << "\n";

    AddBuffer(buf);
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
  maxResult.sub(GenerateIntegerExpression(I->getOperand(1), VarLiteral::MIN));
  minResult.sub(GenerateIntegerExpression(I->getOperand(1), VarLiteral::MAX));

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
  } else if (operand1Max.IsConst()) {
    constOperand = operand1Max.GetConst();
    minOperand = &operand0Min;
    maxOperand = &operand0Max;
  } else {
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

void ConstraintGenerator::GeneratePointerDerefConstraint(const Value* I) {
  Buffer buf(I);
  Constraint cMax, cMin;

  cMax.addBig(buf.NameExpression(VarLiteral::MAX, VarLiteral::LEN_WRITE));
  cp_.AddConstraint(cMax);
  LOG << "Adding - " << buf.NameExpression(VarLiteral::MAX, VarLiteral::LEN_WRITE) << " >= 0 \n";

  cMin.addSmall(buf.NameExpression(VarLiteral::MIN, VarLiteral::LEN_WRITE));
  cp_.AddConstraint(cMin);
  LOG << "Adding - " << buf.NameExpression(VarLiteral::MIN, VarLiteral::LEN_WRITE) << " <= 0 \n";
}

void ConstraintGenerator::GenerateStoreConstraint(const StoreInst* I) {
  if (const PointerType *pType = dyn_cast<const PointerType>(I->getPointerOperand()->getType())) {
    if (!(pType->getElementType()->isPointerTy())) {
      // store into a pointer - store int value
      Integer intLiteral(I->getPointerOperand());
      GenerateGenericConstraint(intLiteral, I->getValueOperand(), "store instruction");
      GeneratePointerDerefConstraint(I->getPointerOperand());
    } else {
      Pointer pFrom(makePointer(I->getValueOperand())), pTo(makePointer(I->getPointerOperand()));
      GenerateBufferAliasConstraint(pFrom, pTo);
//      GeneratePointerDerefConstraint(I->getPointerOperand());
    }
  } else {
    LOG << "Error - Trying to store into a non pointer type" << endl;
  }
}

void ConstraintGenerator::GenerateLoadConstraint(const LoadInst* I) {
  if (const PointerType *pType = dyn_cast<const PointerType>(I->getPointerOperand()->getType())) {
    if (!(pType->getElementType()->isPointerTy())) {
      // load from a pointer - load int value
      Integer intLiteral(I);
      GenerateGenericConstraint(intLiteral, I->getPointerOperand(), "load instruction");
      GeneratePointerDerefConstraint(I->getPointerOperand());
    } else {
      Pointer pFrom(I->getPointerOperand()), pTo(I);
      GenerateBufferAliasConstraint(pFrom, pTo);
    }
  } else {
    LOG << "Error - Trying to load from a non pointer type" << endl;
  }
}

void ConstraintGenerator::GenerateBufferAliasConstraint(VarLiteral from, VarLiteral to,
                                                        const Value *offset) {
  static const VarLiteral::ExpressionDir dirs[2] = {VarLiteral::MIN, VarLiteral::MAX};
  static const int dirCoef[2] = {1, -1};
  static const VarLiteral::ExpressionType types[2] = {VarLiteral::LEN_READ, VarLiteral::LEN_WRITE};
  static const int typeCoef[2] = {-1, 1};

  for (int type = 0; type < 2; ++type) {
    Constraint::Expression offsets[2];
    if (offset) {
      offsets[0] = GenerateIntegerExpression(offset, dirs[0]);
      offsets[0].mul(dirCoef[0] * typeCoef[type]);
      offsets[1] = GenerateIntegerExpression(offset, dirs[1]);
      offsets[1].mul(dirCoef[1] * typeCoef[type]);
    }

    for (int dir = 0; dir < 2; ++ dir) {
      Constraint c;
      c.addBig(to.NameExpression(dirs[dir], types[type]), dirCoef[dir] * typeCoef[type]);
      c.addBig(offsets[dir]);
      c.addSmall(from.NameExpression(dirs[dir], types[type]), dirCoef[dir] * typeCoef[type]);
      cp_.AddConstraint(c);
      LOG << "Adding - " << to.NameExpression(dirs[dir], types[type]) << " * " <<
             dirCoef[dir] << " * " << typeCoef[type] << " >= " << offsets[dir].toString() <<
             " + " << from.NameExpression(dirs[dir], types[type]) << " * " <<  dirCoef[dir] <<
             " *  " << typeCoef[type] << "\n";
    }
  }
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

        if (allocedBuffers[D->getAddress()]) {
          AddBuffer(b);
        }

        return;
      }
    }
  }
  // else
  LOG << "Can't extract debug info\n";
}

void ConstraintGenerator::GenerateAllocConstraint(const Value *I, const ArrayType *aType) {
  Buffer buf(I);
  double allocSize = aType->getNumElements();
  Constraint allocMax, allocMin;
  allocedBuffers[I] = true;

  allocMax.addBig(buf.NameExpression(VarLiteral::MAX, VarLiteral::ALLOC));
  allocMax.addSmall(allocSize);
  cp_.AddConstraint(allocMax);
  LOG << "Adding - " << buf.NameExpression(VarLiteral::MAX, VarLiteral::ALLOC) << " >= " <<
                allocSize << "\n";

  allocMin.addSmall(buf.NameExpression(VarLiteral::MIN, VarLiteral::ALLOC));
  allocMin.addBig(allocSize);
  LOG << "Adding - " << buf.NameExpression(VarLiteral::MIN, VarLiteral::ALLOC) << " <= " <<
               allocSize << "\n";
  cp_.AddConstraint(allocMin);
}

void ConstraintGenerator::GenerateAllocaConstraint(const AllocaInst *I) {
  if (const PointerType *pType = dyn_cast<const PointerType>(I->getType())) {
    if (const ArrayType *aType = dyn_cast<const ArrayType>(pType->getElementType())) {
      GenerateAllocConstraint(I, aType);
    }
  }
}

void ConstraintGenerator::GenerateArraySubscriptConstraint(const GetElementPtrInst *I) {
  Buffer b(I->getPointerOperand());
  if (b.IsNull()) {
    LOG << " ERROR - trying to add a buffer before buffer definition" << endl;
    return;
  }

  LOG << " Adding buffer to problem" << endl;

  Pointer ptr(I);
  GenerateBufferAliasConstraint(b, ptr, I->getOperand(I->getNumOperands()-1));
}

void ConstraintGenerator::GenerateCallConstraint(const CallInst* I) {
  Function* f = I->getCalledFunction();
  if (f == NULL) {
    return;
  }

  if (f->getNameStr() == "malloc") {
    // malloc calls are of the form:
    //   %2 = call i8* @malloc(i64 4)
    //   ...
    //   store i8* %2, i8** %buf1, align 8
    //
    // This method generates an Alloc expression for the malloc call, and the store instruction will
    // generate a BufferAlias.
    LOG << I << " malloc call" << endl;
    Buffer buf(I, "malloc", GetInstructionFilename(I), I->getDebugLoc().getLine());
    GenerateGenericConstraint(buf, I->getArgOperand(0), "malloc call", VarLiteral::ALLOC);
    AddBuffer(buf);
    return;
  }

  if (f->getNameStr() == "strlen") {
    Pointer p(makePointer(I->getArgOperand(0)));
    Integer var(I);

    Constraint cMax;
    cMax.addBig(var.NameExpression(VarLiteral::MAX));
    cMax.addSmall(p.NameExpression(VarLiteral::MAX, VarLiteral::LEN_READ));
    cMax.addSmall(1);
    cMax.SetBlame("strlen call");
    cp_.AddConstraint(cMax);
    LOG << "Adding - " << var.NameExpression(VarLiteral::MAX) << " >= "
              << p.NameExpression(VarLiteral::MAX, VarLiteral::LEN_READ) << " + 1" << endl;

    Constraint cMin;
    cMin.addSmall(var.NameExpression(VarLiteral::MIN));
    cMin.addBig(p.NameExpression(VarLiteral::MIN, VarLiteral::LEN_READ));
    cMax.addBig(1);
    cMin.SetBlame("strlen call");
    cp_.AddConstraint(cMin);
    LOG << "Adding - " << var.NameExpression(VarLiteral::MIN) << " <= "
              << p.NameExpression(VarLiteral::MIN, VarLiteral::LEN_READ) << " + 1" << endl;
    return;
  }

  if (f->getNameStr() == "strcpy") {
    Pointer from(makePointer(I->getArgOperand(1))), to(makePointer(I->getArgOperand(0)));

    Constraint cMax;
    cMax.addBig(to.NameExpression(VarLiteral::MAX, VarLiteral::LEN_WRITE));
    cMax.addSmall(from.NameExpression(VarLiteral::MAX, VarLiteral::LEN_READ));
    cMax.SetBlame("strcpy call");
    cp_.AddConstraint(cMax);
    LOG << "Adding - " << to.NameExpression(VarLiteral::MAX, VarLiteral::LEN_WRITE) << " >= "
              << from.NameExpression(VarLiteral::MAX, VarLiteral::LEN_READ) << endl;

    Constraint cMin;
    cMin.addSmall(to.NameExpression(VarLiteral::MIN, VarLiteral::LEN_WRITE));
    cMin.addBig(from.NameExpression(VarLiteral::MIN, VarLiteral::LEN_READ));
    cMin.SetBlame("strcpy call");
    cp_.AddConstraint(cMin);
    LOG << "Adding - " << to.NameExpression(VarLiteral::MIN, VarLiteral::LEN_WRITE) << " <= "
              << from.NameExpression(VarLiteral::MIN, VarLiteral::LEN_READ) << endl;

  }

//      // General function call
//      if (funcDec->hasBody()) {
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
}

void ConstraintGenerator::GenerateGenericConstraint(const VarLiteral &var, const Value *integerExpression,
                                                    const string &blame,
                                                    VarLiteral::ExpressionType type) {
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
    result.add(literal->getSExtValue());
    return result;
  }

  // Otherwise, this is a reference to another var definition
  Integer intLiteral(expr);
  result.add(intLiteral.NameExpression(dir));
  return result;
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

// Static.
string ConstraintGenerator::GetInstructionFilename(const Instruction* I) {
  // Magic numbers that lead us through the various debug nodes to where the filename is.
  if (const MDNode* n1 =
      dyn_cast<const MDNode>(I->getMetadata(LLVMContext::MD_dbg)->getOperand(2))) {
    if (const MDNode* n2 = dyn_cast<const MDNode>(n1->getOperand(1))) {
      if (const MDNode* n3 = dyn_cast<const MDNode>(n2->getOperand(2))) {
        if (const MDNode* filenamenode = dyn_cast<const MDNode>(n3->getOperand(3))) {
          if (const MDString* filename =
              dyn_cast<const MDString>(filenamenode->getOperand(3))) {
            return filename->getString().str();
          }
        }
      }
    }
  }
  return "";
}

//Static.
Pointer ConstraintGenerator::makePointer(const Value *I) {
  if (const ConstantExpr* G = dyn_cast<const ConstantExpr>(I)) {
    return G->getOperand(0);
  }
  return I;
}

// return stmnt
//bool ConstraintGenerator::VisitStmt(Stmt* S, FunctionDecl* context) {
//  if (ReturnStmt *ret = dyn_cast<ReturnStmt>(S)) {
//    if (/*(ret->getRetValue()->getType()->isIntegerType()) &&*/ (context != NULL)) {
//      Integer intLiteral(context);
//      GenerateGenericConstraint(intLiteral, ret->getRetValue(), "Integer return " + getStmtLoc(S));
//    }
//  }
//}

} // namespace boa
