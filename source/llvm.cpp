#define DEBUG_TYPE "hello"
#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/ADT/Statistic.h"

#include "Buffer.h"
#include "Constraint.h"
#include "log.h"

using namespace llvm;

namespace boa {
class ConstraintGenerator : public ModulePass {
 private:
  void GenerateArraySubscriptConstraint(const GetElementPtrInst *I) {
    errs() << "PtrElementInstr " << (void*)(I->getPointerOperand()) << '\n';  
  }

  void GenerateAllocConstraint(const AllocaInst *I) {
    LOG << "here" << endl;
    I->getType()->print(errs());
    
    if (const PointerType *pType = dyn_cast<const PointerType>(I->getType())) {
      if (const ArrayType *aType = dyn_cast<const ArrayType>(pType->getElementType())) {
        Buffer buf(I);
        double allocSize = aType->getNumElements();
        Constraint allocMax, allocMin;

        allocMax.addBig(buf.NameExpression(VarLiteral::MAX, VarLiteral::ALLOC));
        allocMax.addSmall(allocSize);
//        allocMax.SetBlame("static char buffer declaration " + getStmtLoc(var));
        //cp_.AddConstraint(allocMax);
        LOG << "Adding - " << buf.NameExpression(VarLiteral::MAX, VarLiteral::ALLOC) << " >= " <<
                      allocSize << "\n";

        allocMin.addSmall(buf.NameExpression(VarLiteral::MIN, VarLiteral::ALLOC));
        allocMin.addBig(allocSize);
//        allocMin.SetBlame("static char buffer declaration " + getStmtLoc(var));
        LOG << "Adding - " << buf.NameExpression(VarLiteral::MIN, VarLiteral::ALLOC) << " <= " <<
                     allocSize << "\n";
        //cp_.AddConstraint(allocMin);
      }
    }
/*************************************************    
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
// *************************************************/
  }
   
  void VisitInstruction(const Instruction *I) {
    switch (I->getOpcode()) {
      case Instruction::GetElementPtr : 
        GenerateArraySubscriptConstraint(dyn_cast<const GetElementPtrInst>(I));
        break;
      case Instruction::Alloca :
        GenerateAllocConstraint(dyn_cast<const AllocaInst>(I));
        break;
      default : break;//TODO
    }
    errs() << I->getOpcodeName() << '\n';  
  }
 public:
  static char ID; // Pass identification, replacement for typeid
  ConstraintGenerator() : ModulePass(ID) {        
    log::set(std::cout);
  }

  virtual bool runOnModule(Module &M) {
    for (Module::const_iterator it = M.begin(); it != M.end(); ++it) {
      const Function *F = it; 
      for (const_inst_iterator ii = inst_begin(F); ii != inst_end(F); ++ii) {
        VisitInstruction(&(*ii));           
      }
    }
  }
  
  
};
}

char boa::ConstraintGenerator::ID = 0;
static RegisterPass<boa::ConstraintGenerator> X("ConstraintGenerator", "boa ConstraintGenerator");

