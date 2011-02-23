#include "ConstraintGenerator.h"

#include <map>
#include <vector>
#include <sstream>

#include "Helpers.h"
#include "Integer.h"
#include "LinearProblem.h"
#include "Pointer.h"
#include "PointerAnalyzer.h"

#include "llvm/Constants.h"
#include "llvm/LLVMContext.h"
#include "llvm/User.h"

using std::pair;
using std::stringstream;

using namespace llvm;

namespace boa {

void ConstraintGenerator::AddBuffer(const Buffer& buf, const string &location) {
  cp_.AddBuffer(buf);
  buffers_.insert(buf);

  GenerateConstraint(buf.NameExpression(VarLiteral::MAX, VarLiteral::LEN_READ),
                     buf.NameExpression(VarLiteral::MAX, VarLiteral::USED),
                     VarLiteral::MAX, "Buffer Addition", location, Constraint::STRUCTURAL);
  GenerateConstraint(buf.NameExpression(VarLiteral::MAX, VarLiteral::USED),
                     buf.NameExpression(VarLiteral::MAX, VarLiteral::LEN_WRITE),
                     VarLiteral::MAX, "Buffer Addition", location, Constraint::STRUCTURAL);

  GenerateConstraint(buf.NameExpression(VarLiteral::MIN, VarLiteral::LEN_READ),
                     buf.NameExpression(VarLiteral::MIN, VarLiteral::USED),
                     VarLiteral::MIN, "Buffer Addition", location, Constraint::STRUCTURAL);
  GenerateConstraint(buf.NameExpression(VarLiteral::MIN, VarLiteral::USED),
                     buf.NameExpression(VarLiteral::MIN, VarLiteral::LEN_WRITE),
                     VarLiteral::MIN, "Buffer Addition", location, Constraint::STRUCTURAL);
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
  case Instruction::PHI:
    GeneratePhiConstraint(dyn_cast<const PHINode>(I));
    break;
  case Instruction::Select:
    GenerateSelectConstraint(dyn_cast<const SelectInst>(I));
    break;
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
    unsigned len = ar->getNumElements() - 1;
    string s;
    if (const GlobalVariable *GV = dyn_cast<const GlobalVariable>(G)) {
      if (const ConstantArray *CA = dyn_cast<const ConstantArray>(GV->getInitializer())) {
        if (CA->isCString()) {
          s = CA->getAsString();
          Helpers::ReplaceInString(s, '\n', "\\n");
          Helpers::ReplaceInString(s, '\t', "\\t");
          Helpers::ReplaceInString(s, '\r', "");
        }
      }
    }
    s = "string literal \"" + s + "\"";
    Buffer buf(G, s, ""); // TODO - file? line?
    LOG << "Adding string literal. Len - " << len <<  " at " << (void*)G << endl;

    GenerateAllocConstraint(G, ar, "(literal)");
    GenerateConstraint(buf, len, VarLiteral::LEN_WRITE, VarLiteral::MAX, s, "(literal)");
    GenerateConstraint(buf, len, VarLiteral::LEN_WRITE, VarLiteral::MIN, s, "(literal)");
    AddBuffer(buf, "(literal)");
  }
}

void ConstraintGenerator::GenerateReturnConstraint(const ReturnInst* I, const Function *F) {
  if (I->getReturnValue()) { // non void
    if (F->getReturnType()->isPointerTy()) {
      Pointer from(makePointer(I->getReturnValue())), to(F);
      GenerateBufferAliasConstraint(from, to, GetInstructionFilename(I));
    } else {
      Integer intLiteral(F);
      GenerateGenericConstraint(intLiteral, I->getReturnValue(), VarLiteral::USED, "return value", 
                                GetInstructionFilename(I));
    }
  }
}

void ConstraintGenerator::GenerateAndConstraint(const BinaryOperator* I) {
  Integer res(I);
  GenerateGenericConstraint(res, I->getOperand(0), VarLiteral::USED, "bitwise and", 
                            GetInstructionFilename(I));
  GenerateGenericConstraint(res, I->getOperand(1), VarLiteral::USED, "bitwise and",
                            GetInstructionFilename(I));
}


void ConstraintGenerator::GenerateAddConstraint(const BinaryOperator* I) {
  string blame = "Addition";
  Expression maxResult, minResult;
  maxResult.add(GenerateIntegerExpression(I->getOperand(0), VarLiteral::MAX));
  minResult.add(GenerateIntegerExpression(I->getOperand(0), VarLiteral::MIN));
  maxResult.add(GenerateIntegerExpression(I->getOperand(1), VarLiteral::MAX));
  minResult.add(GenerateIntegerExpression(I->getOperand(1), VarLiteral::MIN));

  Integer intLiteral(I);
  GenerateConstraint(intLiteral, maxResult, VarLiteral::USED, VarLiteral::MAX, blame,
                     GetInstructionFilename(I));
  GenerateConstraint(intLiteral, minResult, VarLiteral::USED, VarLiteral::MIN, blame, 
                     GetInstructionFilename(I));
}


void ConstraintGenerator::GenerateSubConstraint(const BinaryOperator* I) {
  string blame = "Subtraction";
  Expression maxResult, minResult;
  maxResult.add(GenerateIntegerExpression(I->getOperand(0), VarLiteral::MAX));
  minResult.add(GenerateIntegerExpression(I->getOperand(0), VarLiteral::MIN));
  maxResult.sub(GenerateIntegerExpression(I->getOperand(1), VarLiteral::MIN));
  minResult.sub(GenerateIntegerExpression(I->getOperand(1), VarLiteral::MAX));

  Integer intLiteral(I);
  GenerateConstraint(intLiteral, maxResult, VarLiteral::USED, VarLiteral::MAX, blame, 
                     GetInstructionFilename(I));
  GenerateConstraint(intLiteral, minResult, VarLiteral::USED, VarLiteral::MIN, blame,
                     GetInstructionFilename(I));
}

void ConstraintGenerator::GenerateMulConstraint(const BinaryOperator* I) {
  string blame = "Multiplication";
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

  string loc = GetInstructionFilename(I);

  GenerateConstraint(intLiteral, *maxOperand, VarLiteral::USED, VarLiteral::MAX, blame, loc);
  GenerateConstraint(intLiteral, *minOperand, VarLiteral::USED, VarLiteral::MAX, blame, loc);
  GenerateConstraint(intLiteral, *maxOperand, VarLiteral::USED, VarLiteral::MIN, blame, loc);
  GenerateConstraint(intLiteral, *minOperand, VarLiteral::USED, VarLiteral::MIN, blame, loc);
}

void ConstraintGenerator::GenerateDivConstraint(const BinaryOperator* I) {
  string blame = "Division";
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

  string loc = GetInstructionFilename(I);

  GenerateConstraint(intLiteral, maxOperand, VarLiteral::USED, VarLiteral::MAX, blame, loc);
  GenerateConstraint(intLiteral, minOperand, VarLiteral::USED, VarLiteral::MAX, blame, loc);
  GenerateConstraint(intLiteral, maxOperand, VarLiteral::USED, VarLiteral::MIN, blame, loc);
  GenerateConstraint(intLiteral, minOperand, VarLiteral::USED, VarLiteral::MIN, blame, loc);
}

void ConstraintGenerator::GenerateCastConstraint(const CastInst* I, const string& blame) {
  Integer intLiteral(I);
  GenerateGenericConstraint(intLiteral, I->getOperand(0), VarLiteral::USED, blame, 
                            GetInstructionFilename(I));
}

void ConstraintGenerator::GeneratePointerDerefConstraint(const Value* I, const string &location) {
  Buffer buf(I);
  Constraint cMax("Pointer Dereference", location), cMin("Pointer Dereference", location);

  cMax.addBig(buf.NameExpression(VarLiteral::MAX, VarLiteral::LEN_WRITE));
  cp_.AddConstraint(cMax);
  LOG << "Adding - " << buf.NameExpression(VarLiteral::MAX, VarLiteral::LEN_WRITE) << " >= 0 \n";

  cMin.addSmall(buf.NameExpression(VarLiteral::MIN, VarLiteral::LEN_WRITE));
  cp_.AddConstraint(cMin);
  LOG << "Adding - " << buf.NameExpression(VarLiteral::MIN, VarLiteral::LEN_WRITE) << " <= 0 \n";
}

void ConstraintGenerator::GenerateStoreConstraint(const StoreInst* I) {
  string loc = GetInstructionFilename(I);

  if (const PointerType *pType = dyn_cast<const PointerType>(I->getPointerOperand()->getType())) {
    if (!(pType->getElementType()->isPointerTy())) {
      // store into a pointer - store int value
      Integer intLiteral(I->getPointerOperand());
      GenerateGenericConstraint(intLiteral, I->getValueOperand(), VarLiteral::USED, 
                                "store instruction", loc);
      GeneratePointerDerefConstraint(I->getPointerOperand(), loc);
    } else {
      Pointer pFrom(makePointer(I->getValueOperand())), pTo(makePointer(I->getPointerOperand()));
      GenerateBufferAliasConstraint(pFrom, pTo, loc);
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
      GenerateBufferAliasConstraint(*pi, *bi, "(boa pointer analyzer)");
      LOG << "Pointer Analyzer" << endl;
    }
  }
}

void ConstraintGenerator::GenerateLoadConstraint(const LoadInst* I) {
  string loc = GetInstructionFilename(I);
  if (const PointerType *pType = dyn_cast<const PointerType>(I->getPointerOperand()->getType())) {
    if (!(pType->getElementType()->isPointerTy())) {
      // load from a pointer - load int value
      Integer intLiteral(I);
      GenerateGenericConstraint(intLiteral, I->getPointerOperand(),VarLiteral::USED, 
                                "load instruction", loc);
      GeneratePointerDerefConstraint(I->getPointerOperand(), loc);
    } else {
      Pointer pFrom(I->getPointerOperand()), pTo(I);
      GenerateBufferAliasConstraint(pFrom, pTo, loc);
  
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
                                                        const string& location, 
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
      Expression bigExp, smallExp;
      bigExp.add(to.NameExpression(dirs[dir], types[type]), dirCoef[dir] * typeCoef[type]);
      bigExp.add(offsets[dir]);
      smallExp.add(from.NameExpression(dirs[dir], types[type]), dirCoef[dir] * typeCoef[type]);
      GenerateConstraint(bigExp, smallExp, VarLiteral::MAX, "buffer alias", location);
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
  
  string blame = "Shift operation", loc = GetInstructionFilename(I);
  
  GenerateConstraint(intLiteral, maxOperand, VarLiteral::USED, VarLiteral::MAX, blame, loc);
  GenerateConstraint(intLiteral, minOperand, VarLiteral::USED, VarLiteral::MAX, blame, loc);
  GenerateConstraint(intLiteral, maxOperand, VarLiteral::USED, VarLiteral::MIN, blame, loc);
  GenerateConstraint(intLiteral, minOperand, VarLiteral::USED, VarLiteral::MIN, blame, loc);
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
          AddBuffer(b, ss.str());
        }

        return;
      }
    }
  }
  // else
  LOG << "Can't extract debug info\n";
}

void ConstraintGenerator::GenerateAllocConstraint(const Value *I, const ArrayType *aType, 
                                                  const string& location) {
  Buffer buf(I);
  double allocSize = aType->getNumElements();
  Constraint allocMax, allocMin;
  allocedBuffers[I] = true;

  string blame = "Buffer allocation";

  GenerateConstraint(buf, allocSize, VarLiteral::ALLOC, VarLiteral::MAX, blame, location);
  GenerateConstraint(buf, allocSize, VarLiteral::ALLOC, VarLiteral::MIN, blame, location);
}

void ConstraintGenerator::GenerateAllocaConstraint(const AllocaInst *I) {
  if (const PointerType *pType = dyn_cast<const PointerType>(I->getType())) {
    if (const ArrayType *aType = dyn_cast<const ArrayType>(pType->getElementType())) {
      GenerateAllocConstraint(I, aType, GetInstructionFilename(I));
    }
  }
}

void ConstraintGenerator::GenerateArraySubscriptConstraint(const GetElementPtrInst *I) {
  Buffer b(I->getPointerOperand());

  Pointer ptr(I);
  GenerateBufferAliasConstraint(b, ptr, GetInstructionFilename(I),
      I->getOperand(I->getNumOperands()-1));
}

void ConstraintGenerator::GenerateCallConstraint(const CallInst* I) {
  Function* f = I->getCalledFunction();
  if (f == NULL) {
    return;
  }

  string functionName = f->getNameStr(), location = GetInstructionFilename(I);

  if (functionName == "malloc") {
    GenerateMallocConstraint(I, location);
    return;
  }

  if (functionName == "strdup") {
    GenerateStrdupConstraint(I, location);
    return;
  }

  if (functionName == "strlen") {
    GenerateStrlenConstraint(I, location);
    return;
  }

  if (functionName == "strcpy") {
    GenerateStringCopyConstraint(I);
    return;
  }

  if (functionName == "strncpy") {
    GenerateStrNCopyConstraint(I, "strncpy call", location);
    return;
  }

  if (functionName == "strxfrm") {
    GenerateStrNCopyConstraint(I, "strxfrm call", location);
    return;
  }

  const static string memcpyStr("llvm.memcpy.");
  if (functionName.substr(0, memcpyStr.length()) == memcpyStr) {
    Pointer dest(makePointer(I->getArgOperand(0))), src(makePointer(I->getArgOperand(1)));
    Pointer to(makePointer(I));
    
    Expression minExp = GenerateIntegerExpression(I->getArgOperand(2), VarLiteral::MIN);
    minExp.add(-1.0);
    Expression maxExp = GenerateIntegerExpression(I->getArgOperand(2), VarLiteral::MAX);
    maxExp.add(-1.0);

    GenerateConstraint(dest, maxExp, VarLiteral::LEN_WRITE, VarLiteral::MAX, "memcpy", location);
    GenerateConstraint(dest, minExp, VarLiteral::LEN_WRITE, VarLiteral::MIN, "memcpy", location);
    GenerateConstraint(src, maxExp, VarLiteral::LEN_WRITE, VarLiteral::MAX, "memcpy", location);
    GenerateConstraint(src, minExp, VarLiteral::LEN_WRITE, VarLiteral::MIN, "memcpy", location);
    GenerateBufferAliasConstraint(dest, to, location);
    return;
  }

  if (functionName == "pipe") {
    Expression one(1.0);
    Pointer arr(makePointer(I->getArgOperand(0)));
    GenerateConstraint(arr, one, VarLiteral::LEN_WRITE, VarLiteral::MAX, "pipe call", 
                       GetInstructionFilename(I));
    GenerateConstraint(arr, one, VarLiteral::LEN_WRITE, VarLiteral::MIN, "pipe call", 
                       GetInstructionFilename(I));
    return;
  }
  
  if (functionName == "sprintf") {
    if (I->getNumOperands() != 3) {
      Pointer to(makePointer(I->getArgOperand(0)));
      GenerateUnboundConstraint(to, "sprintf with unknown length format string");
    } else {
      GenerateStringCopyConstraint(I);
    }
    return;
  }
  
  if (functionName == "write") {
    Pointer to(makePointer(I->getArgOperand(0)));
    Expression minExp = GenerateIntegerExpression(I->getArgOperand(1), VarLiteral::MIN);
    minExp.add(-1.0);
    Expression maxExp = GenerateIntegerExpression(I->getArgOperand(1), VarLiteral::MAX);
    maxExp.add(-1.0);

    GenerateConstraint(to, maxExp, VarLiteral::LEN_WRITE, VarLiteral::MAX, "write call", location);
    GenerateConstraint(to, minExp, VarLiteral::LEN_WRITE, VarLiteral::MIN, "write call", location);
    return;
  }

  // General function call
  if (f->isDeclaration()) {
    // Has no body, assuming overrun in each buffer, and unbound return value.
    const unsigned params = I->getNumOperands() - 1; // The last operand is the called function.
    string blame;

    Constraint::Type priority = Constraint::NORMAL;

    // Set blame and priority according to the function's level of safety.
    // A function can be either safe, not safe, or unsafe.
    if (IsUnsafeFunction(functionName)) {
      blame = "unsafe function call " + functionName;
      priority = Constraint::INTERESTING;
    } else if (IsSafeFunction(functionName)) {
      blame = "safe function call " + functionName;
    } else {
      blame = "unknown function call " + functionName;
    }

    // Not safe and unsafe functions.
    if (!IsSafeFunction(functionName)) {
      for (unsigned i = 0; i< params; ++i) {
        if (I->getOperand(i)->getType()->isPointerTy()) {
          Pointer p(makePointer(I->getOperand(i)));
          GenerateUnboundConstraint(p, blame, location, priority);
        }
      }
    }

    // Safe function, return by pointer.
    if (I->getType()->isPointerTy()) {
      GenerateUnboundConstraint(makePointer(I), blame, location);
    } else {
      // Return by value.
      Integer intLiteral(I);
      GenerateUnboundConstraint(intLiteral, blame, location);
    }
  } else {
    //has body, pass arguments
    int i = 0;
    for (Function::const_arg_iterator it = f->arg_begin(); it != f->arg_end(); ++it, ++i) {
      if (it->getType()->isPointerTy()) {
        Pointer from(I->getOperand(i)), to(it);
        GenerateBufferAliasConstraint(from, to, GetInstructionFilename(I));
      } else {
        Integer to(it);
        GenerateGenericConstraint(to, I->getOperand(i), VarLiteral::LEN_WRITE,
                                  "pass integer parameter to a function", location);
      }
    }
    // get return value
    if (I->getType()->isPointerTy()) {
      GenerateBufferAliasConstraint(makePointer(f), makePointer(I), GetInstructionFilename(I));
    } else {
      Integer intLiteral(I);
      GenerateGenericConstraint(intLiteral, f, VarLiteral::USED, "user function call", location);
    }
  }
}

void ConstraintGenerator::GenerateStrNCopyConstraint(const CallInst* I, const string &blame,
                                                     const string &location) {
  Pointer to(makePointer(I->getArgOperand(0)));
  Expression minExp = GenerateIntegerExpression(I->getArgOperand(2), VarLiteral::MIN);
  minExp.add(-1.0);
  Expression maxExp = GenerateIntegerExpression(I->getArgOperand(2), VarLiteral::MAX);
  maxExp.add(-1.0);

  GenerateConstraint(to, maxExp, VarLiteral::LEN_WRITE, VarLiteral::MAX, blame, location);
  GenerateConstraint(to, minExp, VarLiteral::LEN_WRITE, VarLiteral::MIN, blame, location);
}


bool ConstraintGenerator::IsSafeFunction(const string& name) {
  static string safeFunctions[] = { "execv",
                                    "fdopen",
                                    "getopt",
                                    "openlog",
                                    "puts",
                                    "setenv",
                                    "strtok",
                                    "syslog",
                                    "vsyslog",
                                    "strstr",
                                    "strspn" };
  for (size_t i = 0; i < (sizeof(safeFunctions) / sizeof(string)); ++i) {
    if (name == safeFunctions[i]) {
      return true;
    }
  }
  return false;
}

bool ConstraintGenerator::IsUnsafeFunction(const string& name) {
  static string unsafeFunctions[] = { "gets",
                                      "scanf",
                                      "strcat" };
  for (size_t i = 0; i < (sizeof(unsafeFunctions) / sizeof(string)); ++i) {
    if (name == unsafeFunctions[i]) {
      return true;
    }
  }
  return false;
}

void ConstraintGenerator::GenerateStringCopyConstraint(const CallInst* I) {
    Pointer from(makePointer(I->getArgOperand(1)));
    Pointer to(makePointer(I->getArgOperand(0)));
    string loc = GetInstructionFilename(I);

    GenerateConstraint(to.NameExpression(VarLiteral::MAX, VarLiteral::LEN_WRITE),
                       from.NameExpression(VarLiteral::MAX, VarLiteral::LEN_READ),
                       VarLiteral::MAX, "strcpy call", loc);
    GenerateConstraint(to.NameExpression(VarLiteral::MIN, VarLiteral::LEN_WRITE),
                       from.NameExpression(VarLiteral::MIN, VarLiteral::LEN_READ),
                       VarLiteral::MIN, "strcpy call", loc);
}

void ConstraintGenerator::GenerateGenericConstraint(const VarLiteral &var,
                                                    const Value *integerExpression,
                                                    VarLiteral::ExpressionType type,
                                                    const string &blame,
                                                    const string &location) {
  Expression maxExpr = GenerateIntegerExpression(integerExpression, VarLiteral::MAX);
  Expression minExpr = GenerateIntegerExpression(integerExpression, VarLiteral::MIN);
  GenerateConstraint(var, maxExpr, type, VarLiteral::MAX, blame, location);
  GenerateConstraint(var, minExpr, type, VarLiteral::MIN, blame, location);
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

void ConstraintGenerator::GenerateUnboundConstraint(const VarLiteral &var, const string &blame,
                                                    const string& location /* = "" */,
                                                    Constraint::Type prio /* = NORMAL */) {
  GenerateConstraint(var, Expression::PosInfinity, VarLiteral::LEN_WRITE,
                     VarLiteral::MAX, blame, location, prio);
  GenerateConstraint(var, Expression::NegInfinity, VarLiteral::LEN_WRITE,
                     VarLiteral::MIN, blame, location, prio);
}


void ConstraintGenerator::GenerateConstraint(const VarLiteral &var,
                                             const Expression &integerExpression,
                                             VarLiteral::ExpressionType type,
                                             VarLiteral::ExpressionDir direction,
                                             const string &blame,
                                             const string &location,
                                             Constraint::Type prio) {
  GenerateConstraint(var.NameExpression(direction, type), integerExpression,
    direction, blame, location, prio);
}

void ConstraintGenerator::GenerateConstraint(const Expression &lhs, const Expression &rhs,
                                             VarLiteral::ExpressionDir direction,
                                             const string &blame, const string &location, 
                                             Constraint::Type prio) {
  Constraint constraint(lhs, rhs, direction);
  constraint.SetBlame(blame, location, prio);
  cp_.AddConstraint(constraint);
  LOG << "Adding - " << lhs.toString() << (direction == VarLiteral::MAX ? " >= " : " <= ") <<
      rhs.toString() << " - " + blame << endl;
}

void ConstraintGenerator::GenerateBooleanConstraint(const Instruction *I) {
  // Assuming result is of type i1, not [N x i1].
  Integer boolean(I);
  string loc = GetInstructionFilename(I);
  
  GenerateConstraint(boolean, 1.0, VarLiteral::USED,
                     VarLiteral::MAX, "Boolean Operation", loc);
  GenerateConstraint(boolean, 0.0, VarLiteral::USED,
                     VarLiteral::MIN, "Boolean Operation", loc);
}

void ConstraintGenerator::GeneratePhiConstraint(const PHINode *I) {
  Integer phiNode(I);
  string blame = "Ternary operator", loc = GetInstructionFilename(I) ;
  LOG << "Phi Node at " << I << " (" << blame << ")" << endl;
  for (unsigned i = 0; i < I->getNumIncomingValues(); i++) {
    GenerateGenericConstraint(phiNode, I->getIncomingValue(i), VarLiteral::USED, blame, loc);
  }
}

void ConstraintGenerator::GenerateSelectConstraint(const SelectInst *I) {
  Integer select(I);
  string blame = "Ternary operator at ", loc = GetInstructionFilename(I);
  LOG << "Select Node at " << I << " (" << blame << ")" << endl;
  GenerateGenericConstraint(select, I->getTrueValue(), VarLiteral::USED, blame, loc);
  GenerateGenericConstraint(select, I->getFalseValue(), VarLiteral::USED, blame, loc);
}

void ConstraintGenerator::GenerateMallocConstraint(const CallInst* I, const string& location) {
  // malloc calls are of the form:
  //   %2 = call i8* @malloc(i64 4)
  //   ...
  //   store i8* %2, i8** %buf1, align 8
  //
  // This method generates an Alloc expression for the malloc call, and the store instruction will
  // generate a BufferAlias.
  LOG << I << " malloc call" << endl;
  Buffer buf(I, "malloc", location);
  GenerateGenericConstraint(buf, I->getArgOperand(0), VarLiteral::ALLOC, "malloc call", location);
  AddBuffer(buf, location);
}

void ConstraintGenerator::GenerateStrdupConstraint(const CallInst* I, const string &location) {
  static const string blame = "strdup call";
  Buffer buf(I, "strdup", GetInstructionFilename(I));
  AddBuffer(buf, location);
  Pointer from(I->getArgOperand(0));

  Expression maxExp(from.NameExpression(VarLiteral::MAX, VarLiteral::LEN_READ));
  Expression minExp(from.NameExpression(VarLiteral::MIN, VarLiteral::LEN_READ));
  Expression allocMax(maxExp), allocMin(minExp);
  allocMax.add(1.0);
  allocMin.add(1.0);

  GenerateConstraint(buf, allocMax, VarLiteral::ALLOC, VarLiteral::MAX, blame, location);
  GenerateConstraint(buf, allocMin, VarLiteral::ALLOC, VarLiteral::MIN, blame, location);

  GenerateConstraint(buf, maxExp, VarLiteral::LEN_WRITE, VarLiteral::MAX, blame, location);
  GenerateConstraint(buf, minExp, VarLiteral::LEN_WRITE, VarLiteral::MIN, blame, location);
}

void ConstraintGenerator::GenerateStrlenConstraint(const CallInst* I, const string &location) {
  static const string blame = "strlen call";
  Pointer p(makePointer(I->getArgOperand(0)));
  Integer var(I);

  Expression pMax(p.NameExpression(VarLiteral::MAX, VarLiteral::LEN_READ));

  Expression pMin(p.NameExpression(VarLiteral::MIN, VarLiteral::LEN_READ));

  GenerateConstraint(var, pMax, VarLiteral::LEN_READ, VarLiteral::MAX, blame, location);
  GenerateConstraint(var, pMin, VarLiteral::LEN_READ, VarLiteral::MIN, blame, location);
}

// Static.
string ConstraintGenerator::GetInstructionFilename(const Instruction* I) {
  // Magic numbers that lead us through the various debug nodes to where the filename is.
  if (I->getMetadata(LLVMContext::MD_dbg)) {
    if (const MDNode* n1 =
        dyn_cast<const MDNode>(I->getMetadata(LLVMContext::MD_dbg)->getOperand(2))) {
      if (const MDNode* n2 = dyn_cast<const MDNode>(n1->getOperand(4))) {
        if (const MDNode* filenamenode = dyn_cast<const MDNode>(n2->getOperand(3))) {
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

} // namespace boa
