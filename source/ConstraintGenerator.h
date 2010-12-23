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
  ConstraintProblem &cp_;

  bool GenerateArraySubscriptConstraints(ArraySubscriptExpr* expr) {
    if (ImplicitCastExpr *implicitCast = dyn_cast<ImplicitCastExpr>(expr->getBase())) {
      if (implicitCast->getSubExpr()->getType().getTypePtr()->isArrayType()) {
        if (DeclRefExpr *declRef = dyn_cast<DeclRefExpr>(implicitCast->getSubExpr())) {
          ArrayType* arr = dyn_cast<ArrayType>(declRef->getDecl()->getType().getTypePtr());
          if (arr->getElementType().getTypePtr()->isAnyCharacterType()) {
            Buffer buf(declRef->getDecl());
            if (IntegerLiteral *literal = dyn_cast<IntegerLiteral>(expr->getIdx())) {
              Constraint usedMax, usedMin;
              int intVal = literal->getValue().getLimitedValue();

              usedMax.AddBigExpression(buf.NameExpression(Buffer::USED, Buffer::MAX));
              usedMax.AddSmallConst(intVal);
              cp_.AddConstraint(usedMax);
              llvm::errs() << "Adding - " << buf.NameExpression(Buffer::USED, Buffer::MAX) << " >= " << intVal << "\n";

              usedMin.AddSmallExpression(buf.NameExpression(Buffer::USED, Buffer::MIN));
              usedMin.AddBigConst(intVal);
              llvm::errs() << "Adding - " << buf.NameExpression(Buffer::USED, Buffer::MIN) << " <= " << intVal << "\n";
              cp_.AddConstraint(usedMin);

            } // TODO - else (not a literal)
          }
        }
      } //TODO - else (pointer?)
    }
    return true;
  }

 public:
  ConstraintGenerator(SourceManager &SM, ConstraintProblem CP) : sm_(SM), cp_(CP) {}

  bool VisitStmt(Stmt* S) {
    if (ArraySubscriptExpr* expr = dyn_cast<ArraySubscriptExpr>(S)) {
      return GenerateArraySubscriptConstraints(expr);
    }

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

