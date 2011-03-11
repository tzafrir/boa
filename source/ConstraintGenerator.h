#ifndef __BOA_CONSTRAINTGENERATOR_H
#define __BOA_CONSTRAINTGENERATOR_H

#include "llvm/Module.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/Support/InstIterator.h"

#include <map>
#include <string>

using std::string;
using std::stringstream;
using std::map;

#include "ConstraintProblem.h"
#include "Buffer.h"
#include "Integer.h"
#include "Pointer.h"
#include "log.h"

#include "gtest_prod.h"

using namespace llvm;
typedef boa::Constraint::Expression Expression;

namespace boa {

class ConstraintGenerator {
  ConstraintProblem &cp_;
  /**
    Mark buffers that were allocated, so they can be added to the constraint problem once the debug
    info is availble.
  */
  map<const Value *, bool> allocedBuffers_;
  set<const StructType*> structsVisited_;
  const set<string> safeFunctions_;
  const set<string> unsafeFunctions_;
  set<Buffer> buffers_;
  set<Pointer> unknownPointers_;
  bool IgnoreLiterals_;

  /**
    Set the bounds of an integer variable to be [-infinity , infinity]
  */
  void GenerateUnboundConstraint(const VarLiteral &var, const string &blame,
                                 const string &location,
                                 Constraint::Type prio = Constraint::NORMAL);

  /**
    Generate a constraint on "var" according to the the integer value of "integerExpression"
  */
  void GenerateGenericConstraint(const VarLiteral &var, const Value *integerExpression,
                                 VarLiteral::ExpressionType type, const string &blame,
                                 const string &location, const Expression &offset = 0.0);

  void GenerateConstraint(const VarLiteral &var, const Expression &integerExpression,
                          VarLiteral::ExpressionType type, VarLiteral::ExpressionDir direction,
                          const string &blame, const string &location, 
                          Constraint::Type prio = Constraint::NORMAL);

  void GenerateConstraint(const Expression &lhs, const Expression &rhs,
                          VarLiteral::ExpressionDir direction,
                          const string &blame, const string &location, 
                          Constraint::Type prio = Constraint::NORMAL);

  /**
    Generate buffer aliasing constraint - "to" is aliased to "from" + "offset"
  */
  void GenerateBufferAliasConstraint(VarLiteral from, VarLiteral to, const string& location,
                                     const Value *offset = NULL,
                                     const Constraint::Expression *offsetExp = NULL,
                                     const string& blame = "");

  /**
    Make a boa::Pointer instance out of an instruction parameter. This function should be used in
    order to deal with getElementPtr that might appear as a constantExpr (and not a reference to
    another instruction) in an instruction parameter.
  */
  Pointer makePointer(const Value *I, const string& location = "");

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
    should be used instead of adding buffer directly to the constraintProblem.
  */
  void AddBuffer(const Buffer& buf, const string& location, bool literal = false);

  /**
    Generate the Constraint::Expression reflected by "expr". The result will be a number in a case
    of a constant, or a named literal when the expr is a reference to another Instruction.
   */
  Constraint::Expression GenerateIntegerExpression(const Value *expr, VarLiteral::ExpressionDir dir);

  /**
      Generate the aliasing reflected by getElementPtr instruction
    */
  void GenerateGetElementPtrConstraint(const GetElementPtrInst *I);

  /**
    Generate the constraints reflecting defreference of a pointer (usually accesing a buffer through
    alias)
  */
  void GeneratePointerDerefConstraint(const Value* I, const string& location);

  /**
    Generate buffer allocation constraints and register the buffer as allocated
  */
  void GenerateAllocConstraint(const Value *I, const ArrayType *aType, const string& location);

  /**
   * Generate buffer allocation constraints and register buffers as allocated
   * for all buffers contained in the struct tree
   */
  void AddContainedBuffers(const StructType *structType, const MDNode *node);

  /**
    Generate a constraint for the result of a boolean comparison.
  */
  void GenerateBooleanConstraint(const Instruction *I);

  /*
    Generate the constraints reflecting llvm memory access instructions
  */
  void GenerateAllocaConstraint(const AllocaInst *I);
  void GenerateStoreConstraint(const StoreInst* I);
  void GenerateLoadConstraint(const LoadInst* I);

  void GenerateCallConstraint(const CallInst* I);
  void GenerateReturnConstraint(const ReturnInst* I, const Function *F);

  void GenerateStringCopyConstraint(const CallInst* I);
  void GenerateStrNCopyConstraint(const CallInst* I, const string &blame, const string &location);
  void GenerateMallocConstraint(const CallInst* I, const string &location);
  void GenerateStrdupConstraint(const CallInst* I, const string &location);
  void GenerateStrlenConstraint(const CallInst* I, const string &location);
  void GenerateMemchrConstraint(const CallInst* I);
  void GenerateMemmoveConstraint(const CallInst* I);
  void GenerateMemcmpConstraint(const CallInst* I);
  void GenerateMemsetConstraint(const CallInst* I);
  void GenerateMemcpyConstraint(const CallInst* I);

  /*
    Generate the constraints reflecting llvm arithmetic access instructions
  */
  void GenerateAddConstraint(const BinaryOperator* I);
  void GenerateSubConstraint(const BinaryOperator* I);
  void GenerateMulConstraint(const BinaryOperator* I);
  void GenerateDivConstraint(const BinaryOperator* I);

  void GenerateCastConstraint(const CastInst* I, const string& blame);

  void GenerateAndConstraint(const BinaryOperator* I);
  void GenerateOrXorConstraint(const Instruction* I);
  void GenerateShiftConstraint(const BinaryOperator* I);

  void GeneratePhiConstraint(const PHINode* I);
  void GenerateSelectConstraint(const SelectInst* I);

  bool IsSafeFunction(const string& name);
  bool IsUnsafeFunction(const string& name);

 public:
  ConstraintGenerator(ConstraintProblem &CP, bool ignoreLiterals, const set<string> &safeFunctions,
                      const set<string> &unsafeFunctions) : cp_(CP), safeFunctions_(safeFunctions),
                      unsafeFunctions_(unsafeFunctions), IgnoreLiterals_(ignoreLiterals) {}

  void AnalyzePointers();

  /**
    Generate constraints out of a specific instruction
  */
  void VisitInstruction(const Instruction *I, const Function *F);

  /**
    Generate constraints out of global variable declerations/definitions
  */
  void VisitGlobal(const GlobalValue *G);

  // Friendships for testing.
  FRIEND_TEST(ConstraintGeneratorTest, SafeFunctionTest);
  FRIEND_TEST(ConstraintGeneratorTest, UnsafeFunctionTest);
  FRIEND_TEST(ConstraintGeneratorTest, GenerateConstraintMax);
  FRIEND_TEST(ConstraintGeneratorTest, GenerateConstraintMin);
};

}  // namespace boa

#endif  // __BOA_CONSTRAINTGENERATOR_H

