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
#include <vector>

using std::string;
using std::stringstream;
using std::vector;

#include "Constraint.h"
#include "Pointer.h"

#include "log.h"

using namespace clang;

namespace boa {

class ConstraintGenerator : public RecursiveASTVisitor<ConstraintGenerator> {
  SourceManager &sm_;
  ConstraintProblem &cp_;

  /**
   * Generic method to get source code location of either a clang::Stmt or a clang::Decl.
   */
  template <class T> string getStmtLoc(T *stmt) {
    stringstream buff;
    buff << sm_.getBufferName(stmt->getLocStart()) << ":" <<
            sm_.getSpellingLineNumber(stmt->getLocStart());
    return buff.str();
  }

  void GenerateUnboundConstraint(const VarLiteral &var, const string &blame);

  void GenerateGenericConstraint(const VarLiteral &var, Expr *integerExpression,
                                 const string &blame, ExpressionType type=ALLOC);

  vector<Constraint::Expression> GenerateIntegerExpression(Expr *expr, bool max);

  void GenerateVarDeclConstraints(VarDecl *var);

  bool GenerateArraySubscriptConstraints(ArraySubscriptExpr* expr);

 public:
  ConstraintGenerator(SourceManager &SM, ConstraintProblem &CP) : sm_(SM), cp_(CP) {}

  bool VisitStmt(Stmt* S);
};

}  // namespace boa

#endif  // __BOA_CONSTRAINTGENERATOR_H

