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

#include "constraint.h"

using namespace clang;

namespace boa {

class ConstraintGenerator : public RecursiveASTVisitor<ConstraintGenerator> {
  SourceManager &sm_;
 public:
  ConstraintGenerator(SourceManager &SM) : sm_(SM) {}

  bool VisitStmt(Stmt* S) {
    if (BinaryOperator* op = dyn_cast<BinaryOperator>(S)) {
      if (!op->isAssignmentOp()) {
        return true;
      }

      op->dump();
      if (op->getLHS()->getType()->isIntegerType() && op->getRHS()->getType()->isIntegerType()) {
        if (DeclRefExpr* dre = dyn_cast<DeclRefExpr>(op->getLHS())) {
          llvm::errs() << "Is at " << (void*)(dre->getDecl()) << "\n";
        }
      }
    }
    return true;
  }
};

}  // namespace boa

#endif  // __BOA_CONSTRAINTGENERATOR_H

