#include "ConstraintGenerator.h"

#include <map>
#include <vector>
#include <sstream>

#include "Integer.h"
#include "LinearProblem.h"
#include "Pointer.h"
#include "PointerAnalyzer.h"

#include "llvm/Constants.h"
#include "llvm/LLVMContext.h"
#include "llvm/User.h"

using std::pair;
using std::stringstream;

typedef boa::Constraint::Expression Expression;

using namespace llvm;

namespace boa {

void ConstraintGenerator::AddBuffer(const Buffer& buf) {
  cp_.AddBuffer(buf);
  buffers_.insert(buf);

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

void ConstraintGenerator::VisitInstruction(const Instruction *I, const Function *F) {
  if (const DbgDeclareInst *D = dyn_cast<const DbgDeclareInst>(I)) {
    SaveDbgDeclare(D);
    return;
  }

  switch (I->getOpcode()) {
  // Terminators
  case Instruction::Ret:
    GenerateReturnConstraint(dyn_cast<const ReturnInst>(I), F);
    break;
  case Instruction::Br:
  case Instruction::Switch:
  case Instruction::IndirectBr:
  case Instruction::Invoke:
  case Instruction::Unwind:
  case Instruction::Unreachable:
    break;
  // Standard binary operators...
  case Instruction::Add:
  case Instruction::FAdd:
    GenerateAddConstraint(dyn_cast<const BinaryOperator>(I));
    break;
  case Instruction::Sub:
  case Instruction::FSub:
    GenerateSubConstraint(dyn_cast<const BinaryOperator>(I));
    break;
  case Instruction::Mul:
  case Instruction::FMul:
    GenerateMulConstraint(dyn_cast<const BinaryOperator>(I));
    break;
//  case Instruction::UDiv:
  case Instruction::SDiv:
  case Instruction::FDiv:
    GenerateDivConstraint(dyn_cast<const BinaryOperator>(I));
    break;
//  case Instruction::URem:
//  case Instruction::SRem:
//  case Instruction::FRem:

  // Logical operators...
  case Instruction::And:
    GenerateAndConstraint(dyn_cast<const BinaryOperator>(I));
    break;
  case Instruction::Or:    
  case Instruction::Xor:
    GenerateOrXorConstraint(I);
    break;
    
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
  case Instruction::Trunc:
  case Instruction::ZExt:
  case Instruction::SExt:
    GenerateCastConstraint(dyn_cast<const CastInst>(I), "Int sign extend");
//  case Instruction::FPToUI:
  case Instruction::FPExt:
    GenerateCastConstraint(dyn_cast<const CastInst>(I), "Float sign extend");
    break;
  case Instruction::FPToSI:
    GenerateCastConstraint(dyn_cast<const CastInst>(I), "Float to int cast");
    break;
  case Instruction::UIToFP:
    GenerateCastConstraint(dyn_cast<const CastInst>(I), "Uint to float cast");
    break;
  case Instruction::SIToFP:
    GenerateCastConstraint(dyn_cast<const CastInst>(I), "Int to float cast");
    break;

  // Exploiting case fall-through.
  case Instruction::IntToPtr:
  case Instruction::PtrToInt:
  case Instruction::BitCast:
    GenerateCastConstraint(dyn_cast<const CastInst>(I), "Arbitrary cast");
    break;

  // Other instructions...

  // Fallthrough.
  case Instruction::ICmp:
  case Instruction::FCmp:
    GenerateBooleanConstraint(I);
    break;
//  case Instruction::PHI:
//  case Instruction::Select:
  case Instruction::Call:
    GenerateCallConstraint(dyn_cast<const CallInst>(I));
    break;
  case Instruction::Shl:
  case Instruction::AShr:
  case Instruction::LShr:
    GenerateShiftConstraint(dyn_cast<const BinaryOperator>(I));
    break;
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
    string s;
    if (const GlobalVariable *GV = dyn_cast<const GlobalVariable>(G)) {
      if (const ConstantArray *CA = dyn_cast<const ConstantArray>(GV->getInitializer())) {
        if (CA->isCString()) {
          s = CA->getAsString();
        }
      }
    }
    Buffer buf(G, "string literal \"" + s + "\"", ""); // TODO - file? line?
    LOG << "Adding string literal. Len - " << len <<  " at " << (void*)G << endl;

    GenerateAllocConstraint(G, ar);

    Constraint lenMax, lenMin;

    lenMax.addBig(buf.NameExpression(VarLiteral::MAX, VarLiteral::LEN_WRITE));
    lenMax.addSmall(len - 1);
    cp_.AddConstraint(lenMax);
    LOG << "Adding - " << buf.NameExpression(VarLiteral::MAX, VarLiteral::LEN_WRITE) << " >= " <<
                  (len - 1) << "\n";

    lenMin.addSmall(buf.NameExpression(VarLiteral::MIN, VarLiteral::LEN_WRITE));
    lenMin.addBig(len - 1);
    cp_.AddConstraint(lenMin);
    LOG << "Adding - " << buf.NameExpression(VarLiteral::MIN, VarLiteral::LEN_WRITE) << " <= " <<
                 (len - 1) << "\n";

    AddBuffer(buf);
  }
}

void ConstraintGenerator::GenerateReturnConstraint(const ReturnInst* I, const Function *F) {
  if (I->getReturnValue()) { // non void
    if (F->getReturnType()->isPointerTy()) {
      Pointer from(makePointer(I->getReturnValue())), to(F);
      GenerateBufferAliasConstraint(from, to);
    }
    else {
      Integer intLiteral(F);
      GenerateGenericConstraint(intLiteral, I->getReturnValue(), "return value", VarLiteral::USED);
    }
  }
}

void ConstraintGenerator::GenerateAndConstraint(const BinaryOperator* I) {
  Integer res(I);
  GenerateGenericConstraint(res, I->getOperand(0), "bitwise and", VarLiteral::USED);
  GenerateGenericConstraint(res, I->getOperand(1), "bitwise and", VarLiteral::USED);
}


void ConstraintGenerator::GenerateAddConstraint(const BinaryOperator* I) {
  Expression maxResult, minResult;
  maxResult.add(GenerateIntegerExpression(I->getOperand(0), VarLiteral::MAX));
  minResult.add(GenerateIntegerExpression(I->getOperand(0), VarLiteral::MIN));
  maxResult.add(GenerateIntegerExpression(I->getOperand(1), VarLiteral::MAX));
  minResult.add(GenerateIntegerExpression(I->getOperand(1), VarLiteral::MIN));

  Integer intLiteral(I);

  Constraint maxCons("Addition"), minCons("Addition");
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

  Constraint maxCons("Subtraction"), minCons("Subtraction");
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

  Constraint maxCons1("Multiplication"), minCons1("Multiplication"), 
             maxCons2("Multiplication"), minCons2("Multiplication");
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

  Constraint maxCons1("Division"), minCons1("Division"), maxCons2("Division"), minCons2("Division");
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

void ConstraintGenerator::GenerateCastConstraint(const CastInst* I, const string& blame) {
  Integer intLiteral(I);
  GenerateGenericConstraint(intLiteral, I->getOperand(0), blame,
                            VarLiteral::USED);
}

void ConstraintGenerator::GeneratePointerDerefConstraint(const Value* I) {
  Buffer buf(I);
  Constraint cMax("Pointer Dereference"), cMin("Pointer Dereference");

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
      GenerateGenericConstraint(intLiteral, I->getValueOperand(), "store instruction",
                                VarLiteral::USED);
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

void ConstraintGenerator::AnalyzePointers() {
  PointerAnalyzer analyzer;
  analyzer.SetBuffers(buffers_);
  for (set<Pointer>::iterator pi = unknownPointers_.begin(); pi != unknownPointers_.end(); ++pi) {
    set<Buffer> buffers = analyzer.PointsTo(*pi);
    for (set<Buffer>::iterator bi = buffers.begin(); bi != buffers.end(); ++bi) {
      GenerateBufferAliasConstraint(*pi, *bi);
      LOG << "Pointer Analyzer" << endl;
    }
  }
}

void ConstraintGenerator::GenerateLoadConstraint(const LoadInst* I) {
  if (const PointerType *pType = dyn_cast<const PointerType>(I->getPointerOperand()->getType())) {
    if (!(pType->getElementType()->isPointerTy())) {
      // load from a pointer - load int value
      Integer intLiteral(I);
      GenerateGenericConstraint(intLiteral, I->getPointerOperand(), "load instruction",
                                VarLiteral::USED);
      GeneratePointerDerefConstraint(I->getPointerOperand());
    } else {
      Pointer pFrom(I->getPointerOperand()), pTo(I);
      GenerateBufferAliasConstraint(pFrom, pTo);
  
      if (const PointerType *ppType = dyn_cast<const PointerType>(pType->getElementType())) {
        if (ppType->getElementType()->isPointerTy()) {
          unknownPointers_.insert(pTo);
        }
      }
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


void ConstraintGenerator::GenerateShiftConstraint(const BinaryOperator* I) {
  Integer intLiteral(I);
  Expression operand1 = GenerateIntegerExpression(I->getOperand(1), VarLiteral::MAX);
  if (!operand1.IsConst()) {
    GenerateUnboundConstraint(intLiteral, "Non-const shift factor.");
    return;
  }
  double shiftFactor = 1 << (int)operand1.GetConst();
  Expression maxOperand = GenerateIntegerExpression(I->getOperand(0), VarLiteral::MAX);
  Expression minOperand = GenerateIntegerExpression(I->getOperand(0), VarLiteral::MIN);
  
  switch (I->getOpcode()) {
    case Instruction::Shl:
      minOperand.mul(shiftFactor);
      maxOperand.mul(shiftFactor);
      break;
    case Instruction::AShr:
      minOperand.div(shiftFactor);
      maxOperand.div(shiftFactor);
      break;
    default:
      GenerateUnboundConstraint(intLiteral, "Logical Shr - unbound.");
      return;
  }
  
  Constraint maxCons1("Shift operation"), minCons1("Shift operation"), 
             maxCons2("Shift operation"), minCons2("Shift operation");
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

void ConstraintGenerator::GenerateOrXorConstraint(const Instruction* I) {
    Integer intLiteral(I);
    GenerateUnboundConstraint(intLiteral, "(X)OR operation");
}

void ConstraintGenerator::SaveDbgDeclare(const DbgDeclareInst* D) {
  // FIXME - magic numbers!
  if (const MDString *S = dyn_cast<const MDString>(D->getVariable()->getOperand(2))) {
    if (const MDNode *node = dyn_cast<const MDNode>(D->getVariable()->getOperand(3))) {
      if (const MDString *file = dyn_cast<const MDString>(node->getOperand(1))) {
        LOG << (void*)D->getAddress() << " name = " << S->getString().str() << " Source location - "
            << file->getString().str() << ":" << D->getDebugLoc().getLine() << endl;

        stringstream ss;
        ss << file->getString().str() << ":" << D->getDebugLoc().getLine();
        Buffer b(D->getAddress(), S->getString().str(), ss.str());

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

  Pointer ptr(I);
  GenerateBufferAliasConstraint(b, ptr, I->getOperand(I->getNumOperands()-1));
}

void ConstraintGenerator::GenerateCallConstraint(const CallInst* I) {
  Function* f = I->getCalledFunction();
  if (f == NULL) {
    return;
  }

  string functionName = f->getNameStr();

  if (functionName == "malloc") {
    // malloc calls are of the form:
    //   %2 = call i8* @malloc(i64 4)
    //   ...
    //   store i8* %2, i8** %buf1, align 8
    //
    // This method generates an Alloc expression for the malloc call, and the store instruction will
    // generate a BufferAlias.
    LOG << I << " malloc call" << endl;
    Buffer buf(I, "malloc", GetInstructionFilename(I));
    GenerateGenericConstraint(buf, I->getArgOperand(0), "malloc call", VarLiteral::ALLOC);
    AddBuffer(buf);
    return;
  }

  if (functionName == "strlen") {
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

  if (functionName == "strcpy") {
    GenerateStringCopyConstraint(I);
    return;
  }

  if (functionName == "strncpy") {
    Pointer to(makePointer(I->getArgOperand(0)));
    Expression minExp = GenerateIntegerExpression(I->getArgOperand(2), VarLiteral::MIN);
    Expression maxExp = GenerateIntegerExpression(I->getArgOperand(2), VarLiteral::MAX);

    Constraint cMax;
    cMax.addBig(to.NameExpression(VarLiteral::MAX, VarLiteral::LEN_WRITE));
    cMax.addSmall(maxExp);
    cMax.addSmall(-1);
    cMax.SetBlame("strncpy call");
    cp_.AddConstraint(cMax);
    LOG << "Adding - " << to.NameExpression(VarLiteral::MAX, VarLiteral::LEN_WRITE) << " <= "
              << maxExp.toString() << endl;

    Constraint cMin;
    cMin.addSmall(to.NameExpression(VarLiteral::MIN, VarLiteral::LEN_WRITE));
    cMin.addBig(minExp);
    cMin.addBig(-1);
    cMin.SetBlame("strncpy call");
    cp_.AddConstraint(cMin);
    LOG << "Adding - " << to.NameExpression(VarLiteral::MIN, VarLiteral::LEN_WRITE) << " <= "
              << minExp.toString() << endl;
    return;
  }

  if (functionName == "sprintf") {
    if (I->getNumOperands() != 3) {
      Pointer to(makePointer(I->getArgOperand(0)));
      GenerateUnboundConstraint(to, "sprintf with unknown length format string");
    }
    else {
      GenerateStringCopyConstraint(I);
    }
    return;
  }

  // General function call
  if (f->isDeclaration()) {
    // Has no body, assuming overrun in each buffer, and unbound return value.
    const unsigned params = I->getNumOperands() - 1; // The last operand is the called function.
    string blame("unknown function call " + functionName);
    for (unsigned i = 0; i< params; ++i) {
      if (I->getOperand(i)->getType()->isPointerTy()) {
        Pointer p(makePointer(I->getOperand(i)));
        GenerateUnboundConstraint(p, blame);
      }
    }
    if (I->getType()->isPointerTy()) {
      GenerateUnboundConstraint(makePointer(I), blame);
    }
    else {
      Integer intLiteral(I);
      GenerateUnboundConstraint(intLiteral, blame);
    }
  }
  else {
    //has body, pass arguments
    int i = 0;
    for (Function::const_arg_iterator it = f->arg_begin(); it != f->arg_end(); ++it, ++i) {
      if (it->getType()->isPointerTy()) {
        Pointer from(I->getOperand(i)), to(it);
        GenerateBufferAliasConstraint(from, to);
      }
      else {
        Integer to(it);
        GenerateGenericConstraint(to, I->getOperand(i), "pass integer parameter to a function",
                                  VarLiteral::LEN_WRITE);
      }
    }
    // get return value
    if (I->getType()->isPointerTy()) {
      GenerateBufferAliasConstraint(makePointer(f), makePointer(I));
    }
    else {
      Integer intLiteral(I);
      GenerateGenericConstraint(intLiteral, f, "user function call", VarLiteral::USED);
    }
  }
}

void ConstraintGenerator::GenerateStringCopyConstraint(const CallInst* I) {
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

void ConstraintGenerator::GenerateGenericConstraint(const VarLiteral &var,
                                                    const Value *integerExpression,
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
  if (const ConstantFP *fpLiteral = dyn_cast<const ConstantFP>(expr)) {
    APFloat apFloat(fpLiteral->getValueAPF());
    bool ignore;
    apFloat.convert(APFloat::IEEEdouble, APFloat::rmTowardPositive, &ignore);

    result.add(apFloat.convertToDouble());
    return result;
  }

  // Otherwise, this is a reference to another var definition
  Integer intLiteral(expr);
  result.add(intLiteral.NameExpression(dir));
  return result;
}

void ConstraintGenerator::GenerateUnboundConstraint(const VarLiteral &var, const string &blame) {
  // FIXME - is VarLiteral::MAX_INT enough?
  Constraint maxV, minV;
  maxV.addBig(var.NameExpression(VarLiteral::MAX, VarLiteral::LEN_WRITE));
  maxV.addSmall(std::numeric_limits<int>::max());
  maxV.SetBlame(blame);
  cp_.AddConstraint(maxV);
  minV.addSmall(var.NameExpression(VarLiteral::MIN, VarLiteral::LEN_WRITE));
  minV.addBig(std::numeric_limits<int>::min());
  minV.SetBlame(blame);
  cp_.AddConstraint(minV);
}

void ConstraintGenerator::GenerateBooleanConstraint(const Value *I) {
  // Assuming result is of type i1, not [N x i1].
  Constraint minB, maxB;
  Integer boolean(I);
  maxB.addBig(boolean.NameExpression(VarLiteral::MAX));
  maxB.addSmall(0.0);
  minB.addSmall(boolean.NameExpression(VarLiteral::MIN));
  minB.addBig(1.0);
  maxB.SetBlame("Boolean operation");
  minB.SetBlame("Boolean operation");
  cp_.AddConstraint(minB);
  cp_.AddConstraint(maxB);
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
            stringstream ss;
            ss << filename->getString().str() << ":" << I->getDebugLoc().getLine();
            return ss.str();
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
