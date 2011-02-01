#ifndef __BOA_CONSTRAINTGENERATOR_H
#define __BOA_CONSTRAINTGENERATOR_H

#include "llvm/Module.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/Support/InstIterator.h"

#include <map>
#include <string>

using std::string;
using std::stringstream;
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
  /**
    Mark buffers that was allocated, so they can be added to the constraint problem once the debug
    info is availble.
  */
  map<const Value*, bool> allocedBuffers;

  /**
    Set the bounds of an integer variable to be [-infinity , infinity]
  */
  void GenerateUnboundConstraint(const Integer &var, const string &blame);

  /**
    Generate a constraint on "var" according to the the integer value of "integerExpression"
  */
  void GenerateGenericConstraint(const VarLiteral &var, const Value *integerExpression,
                                 const string &blame,
                                 VarLiteral::ExpressionType type = VarLiteral::ALLOC);

  /**
    Generate buffer aliasing constraint - "to" is aliased to "from" + "offset"
  */
  void GenerateBufferAliasConstraint(VarLiteral from, VarLiteral to, const Value *offset = NULL);

  /**
    Make a boa::Pointer instance out of an instruction parameter. This function should be used in
    order to deal with getElementPtr that might appear as a constantExpr (and not a reference to
    another instruction) in an instruction parameter.
  */
  static Pointer makePointer(const Value *I);

  /**
    Extract variable declration data from debug information
  */
  void SaveDbgDeclare(const DbgDeclareInst* D);

  /**
    Extract instruction filename from debug information
  */
  static string GetInstructionFilename(const Instruction* I);

  /**
    Add a buffer to the constraint problem, together with the nessesary constraints. This method
    shouldbe used instead of adding buffer directly to the constraintProblem.
  */
  void AddBuffer(const Buffer& buf);

  /**
    Generate the Constraint::Expression reflected by "expr". The result will be a number in a case
    of a constant, or a named literal when the expr is a reference to another Instruction.
   */
  Constraint::Expression GenerateIntegerExpression(const Value *expr, VarLiteral::ExpressionDir dir);

  /**
    Generate the aliasing reflected by getElementPtr instruction
  */
  void GenerateArraySubscriptConstraint(const GetElementPtrInst *I);

  /**
    Generate the constraints reflecting defreference of a pointer (usually accesing a buffer through
    alias)
  */
  void GeneratePointerDerefConstraint(const Value* I);

  /**
    Generate buffer allocation constraints and register the buffer as allocated
  */
  void GenerateAllocConstraint(const Value *I, const ArrayType *aType);

  /*
    Generate the constraints reflecting llvm memory access instructions
  */
  void GenerateAllocaConstraint(const AllocaInst *I);
  void GenerateStoreConstraint(const StoreInst* I);
  void GenerateLoadConstraint(const LoadInst* I);

  void GenerateCallConstraint(const CallInst* I);

  /*
    Generate the constraints reflecting llvm arithmetic access instructions
  */
  void GenerateAddConstraint(const BinaryOperator* I);
  void GenerateSubConstraint(const BinaryOperator* I);
  void GenerateMulConstraint(const BinaryOperator* I);
  void GenerateDivConstraint(const BinaryOperator* I);
  void GenerateSExtConstraint(const SExtInst* I);

 public:
  ConstraintGenerator(ConstraintProblem &CP) : cp_(CP) {}

  /**
    Generate constraints out of a specific instruction
  */
  void VisitInstruction(const Instruction *I);

  /**
    Generate constraints out of global variable declerations/definitions
  */
  void VisitGlobal(const GlobalValue *G);
};

}  // namespace boa

#endif  // __BOA_CONSTRAINTGENERATOR_H

