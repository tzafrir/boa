#ifndef __BOA_CONSTRAINTGENERATOR_H
#define __BOA_CONSTRAINTGENERATOR_H

//#include "clang/Frontend/FrontendPluginRegistry.h"
//#include "clang/AST/ASTConsumer.h"
//#include "clang/AST/ASTContext.h"
//#include "clang/AST/AST.h"
//#include "clang/AST/RecursiveASTVisitor.h"
//#include "clang/Basic/SourceManager.h"
//#include "clang/Frontend/CompilerInstance.h"
//#include "llvm/Support/raw_ostream.h"

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
#include "log.h"

using namespace llvm;

namespace boa {

class ConstraintGenerator {
  ConstraintProblem &cp_;
  map<const Value*, Buffer> buffers;

//  void GenerateUnboundConstraint(const Integer &var, const string &blame);

  void GenerateGenericConstraint(const VarLiteral &var, const Value *integerExpression,
                                 const string &blame,
                                 VarLiteral::ExpressionType type = VarLiteral::ALLOC);

  /**
   * TODO(gai/tzafrir): Document this recursive method.
   */
  vector<Constraint::Expression> GenerateIntegerExpression(const Value *expr, bool max);

//  void GenerateVarDeclConstraints(VarDecl *var);

//  void GenerateStringLiteralConstraints(StringLiteral *stringLiteral);
  void GenerateArraySubscriptConstraint(const GetElementPtrInst *I);

  void GenerateAllocConstraint(const AllocaInst *I);

  void GenerateStoreConstraint(const StoreInst* I);

  void GenerateLoadConstraint(const LoadInst* I);

  void GenerateAddConstraint(const BinaryOperator* I);

  void GenerateCallConstraint(const CallInst* I);

  void SaveDbgDeclare(const DbgDeclareInst* D);

 public:
  ConstraintGenerator(ConstraintProblem &CP) : cp_(CP), needToHandleMallocCall(false) {}

  void VisitInstruction(const Instruction *I);

//  bool VisitStmt(Stmt* S);

//  bool VisitStmt(Stmt* S, FunctionDecl* context);

 private:

  /**
   * malloc calls are of the form:
   *   %2 = call i8* @malloc(i64 4)
   *   store i8* %2, i8** %buf1, align 8
   *
   * This temporarily holds the value of the parameter to malloc until processing the next
   * instruction, when a Buffer instance can be created.
   */
  Value* lastMallocParameter;
  bool needToHandleMallocCall;

};

}  // namespace boa

#endif  // __BOA_CONSTRAINTGENERATOR_H

