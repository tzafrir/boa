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
#include "Integer.h"
#include "log.h"

using namespace llvm;

namespace boa {

class ConstraintGenerator {
  ConstraintProblem &cp_;
  map<const Value*, Buffer> buffers;

  void GenerateUnboundConstraint(const Integer &var, const string &blame);

  void GenerateGenericConstraint(const Buffer &buf, const Value *integerExpression,
                                 const string &blame,
                                 VarLiteral::ExpressionType type = VarLiteral::ALLOC);

  void GenerateGenericConstraint(const VarLiteral &var, const Value *integerExpression,
                                 const string &blame,
                                 VarLiteral::ExpressionType type = VarLiteral::ALLOC);

  /**
   * TODO(gai/tzafrir): Document this recursive method.
   */
  Constraint::Expression GenerateIntegerExpression(const Value *expr, VarLiteral::ExpressionDir dir);

//  void GenerateVarDeclConstraints(VarDecl *var);

//  void GenerateStringLiteralConstraints(StringLiteral *stringLiteral);
  void GenerateArraySubscriptConstraint(const GetElementPtrInst *I);

  void GenerateAllocConstraint(const AllocaInst *I);
  
  void GenerateStoreConstraint(const StoreInst* I);
  
  void GenerateLoadConstraint(const LoadInst* I);

  void GenerateAddConstraint(const BinaryOperator* I);
  
  void GenerateSubConstraint(const BinaryOperator* I);
  
  void GenerateMulConstraint(const BinaryOperator* I);
  
  void GenerateDivConstraint(const BinaryOperator* I);
  
  void SaveDbgDeclare(const DbgDeclareInst* D);  

 public:
  ConstraintGenerator(ConstraintProblem &CP) : cp_(CP) {}

  void VisitInstruction(const Instruction *I);

//  bool VisitStmt(Stmt* S);

//  bool VisitStmt(Stmt* S, FunctionDecl* context);
};

}  // namespace boa

#endif  // __BOA_CONSTRAINTGENERATOR_H

