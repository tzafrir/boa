#ifndef __BOA_CONSTRAINTGENERATOR_H
#define __BOA_CONSTRAINTGENERATOR_H

#include "llvm/Module.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/Support/InstIterator.h"

#include <map>
#include <vector>
#include <string>

using std::string;
using std::stringstream;
using std::vector;
using std::map;

#include "llvm/IntrinsicInst.h"
#include "ConstraintProblem.h"
#include "Buffer.h"
#include "Integer.h"
#include "Pointer.h"
#include "log.h"

using namespace llvm;

namespace boa {

class ConstraintGenerator {
  ConstraintProblem &cp_;
  map<const Value*, Buffer> buffers;

  void GenerateUnboundConstraint(const Integer &var, const string &blame);

  void GenerateGenericConstraint(const VarLiteral &var, const Value *integerExpression,
                                 const string &blame,
                                 VarLiteral::ExpressionType type = VarLiteral::ALLOC);

  void GenerateBufferAliasConstraint(VarLiteral from, VarLiteral to, const Value *offset = NULL);

  static Pointer makePointer(const Value *I) {
    if (const ConstantExpr* G = dyn_cast<const ConstantExpr>(I)) {
      return G->getOperand(0);
    }
    return I;
  }

  void AddBuffer(const Buffer& buf);

  /**
   * TODO(gai/tzafrir): Document this recursive method.
   */
  Constraint::Expression GenerateIntegerExpression(const Value *expr, VarLiteral::ExpressionDir dir);

//  void GenerateVarDeclConstraints(VarDecl *var);

  void GenerateArraySubscriptConstraint(const GetElementPtrInst *I);

  void GeneratePointerDerefConstraint(const Value* I);

  void GenerateAllocaConstraint(const AllocaInst *I);

  void GenerateStoreConstraint(const StoreInst* I);

  void GenerateLoadConstraint(const LoadInst* I);

  void GenerateAddConstraint(const BinaryOperator* I);

  void GenerateSubConstraint(const BinaryOperator* I);

  void GenerateCallConstraint(const CallInst* I);

  void GenerateMulConstraint(const BinaryOperator* I);

  void GenerateDivConstraint(const BinaryOperator* I);

  void SaveDbgDeclare(const DbgDeclareInst* D);

  void GenerateSExtConstraint(const SExtInst* I);

  void GenerateAllocConstraint(const Value *I, const ArrayType *aType);


 public:
  ConstraintGenerator(ConstraintProblem &CP) : cp_(CP) {}

  void VisitInstruction(const Instruction *I);

  void VisitGlobal(const GlobalValue *G);
};

}  // namespace boa

#endif  // __BOA_CONSTRAINTGENERATOR_H

