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
#include "pointer.h"

using namespace clang;

namespace boa {

class ConstraintGenerator : public RecursiveASTVisitor<ConstraintGenerator> {
  SourceManager &sm_;
  ConstraintProblem &cp_;
  
  Constraint::Expressions GenerateIntegerExpression(Expr *expr) {
    Constraint::Expressions retval;
    if (IntegerLiteral *literal = dyn_cast<IntegerLiteral>(expr)) {
      retval.addConst(literal->getValue().getLimitedValue());
    } // TODO - else (not a literal)    
    return retval;  
  }

  bool GenerateArraySubscriptConstraints(ArraySubscriptExpr* expr) {
    Constraint::Expressions indexExpr = GenerateIntegerExpression(expr->getIdx());

    // Base is a static array
    if (ImplicitCastExpr *implicitCast = dyn_cast<ImplicitCastExpr>(expr->getBase())) {
      if (implicitCast->getSubExpr()->getType().getTypePtr()->isArrayType()) {
        if (DeclRefExpr *declRef = dyn_cast<DeclRefExpr>(implicitCast->getSubExpr())) {
          ArrayType* arr = dyn_cast<ArrayType>(declRef->getDecl()->getType().getTypePtr());
          if (arr->getElementType().getTypePtr()->isAnyCharacterType()) {
            Buffer buf(declRef->getDecl());
            Constraint usedMax, usedMin;

            usedMax.AddBigExpression(buf.NameExpression(Buffer::USED, Buffer::MAX));
            usedMax.AddSmall(indexExpr);
            cp_.AddConstraint(usedMax);
            llvm::errs() << "Adding - " << buf.NameExpression(Buffer::USED, Buffer::MAX) << " >= " << indexExpr.toString() << "\n";

            usedMin.AddSmallExpression(buf.NameExpression(Buffer::USED, Buffer::MIN));
            usedMin.AddBig(indexExpr);
            llvm::errs() << "Adding - " << buf.NameExpression(Buffer::USED, Buffer::MIN) << " <= " << indexExpr.toString() << "\n";
            cp_.AddConstraint(usedMin);
          }
        }
      }
    }
    // base is a pointer
    else if (DeclRefExpr *declRef = dyn_cast<DeclRefExpr>(expr->getBase())) {
      PointerType* pType = dyn_cast<PointerType>(declRef->getDecl()->getType().getTypePtr());
      if (pType->getPointeeType()->isAnyCharacterType()) {
        Pointer ptr(declRef->getDecl());
        llvm::errs() << "Should dispatch access through pointer " << (void*)declRef->getDecl() << " at " <<  indexExpr.toString() << "\n";
        // TODO - dispatch
      }
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

