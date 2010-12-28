#ifndef __BOA_CONSTRAINTGENERATOR_H
#define __BOA_CONSTRAINTGENERATOR_H

#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/AST.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/raw_ostream.h"

#include <string>
#include <sstream>

using std::string;
using std::stringstream;

#include "constraint.h"
#include "pointer.h"
#include "log.h"

using namespace clang;

namespace boa {

class ConstraintGenerator : public RecursiveASTVisitor<ConstraintGenerator> {
  SourceManager &sm_;
  ConstraintProblem &cp_;

  string getStmtLoc(Stmt *stmt); // for debug
  
  Constraint::Expression GenerateIntegerExpression(Expr *expr, bool max);

  bool GenerateArraySubscriptConstraints(ArraySubscriptExpr* expr);

 public:
  ConstraintGenerator(SourceManager &SM, ConstraintProblem &CP) : sm_(SM), cp_(CP) {}

  bool VisitStmt(Stmt* S);
};

}  // namespace boa

#endif  // __BOA_CONSTRAINTGENERATOR_H

