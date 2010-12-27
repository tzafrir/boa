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

  string getStmtLoc(Stmt *stmt) { 
    stringstream buff;
    buff << sm_.getBufferName(stmt->getLocStart()) << ":" << sm_.getSpellingLineNumber(stmt->getLocStart());
    return buff.str();
  }
  
  Constraint::Expression GenerateIntegerExpression(Expr *expr, bool max) {
    Constraint::Expression retval;
    if (IntegerLiteral *literal = dyn_cast<IntegerLiteral>(expr)) {
      retval.add(literal->getValue().getLimitedValue());
      return retval;
    }
    
    if (BinaryOperator *op = dyn_cast<BinaryOperator>(expr)) {
      switch (op->getOpcode()) {
        case BO_Add :
          retval.add(GenerateIntegerExpression(op->getLHS(), max));
          retval.add(GenerateIntegerExpression(op->getRHS(), max));
          return retval;
       case BO_Sub :
          retval.add(GenerateIntegerExpression(op->getLHS(), max));
          retval.sub(GenerateIntegerExpression(op->getRHS(), !max));
          return retval;
        default : break;
      }
    }
    expr->dump();
    log::os() << "Can't generate integer expression " << getStmtLoc(expr) << endl;
    return retval;
  }

  bool GenerateArraySubscriptConstraints(ArraySubscriptExpr* expr) {
    VarLiteral* varLiteral = NULL;
  
    // Base is a static array
    if (ImplicitCastExpr *implicitCast = dyn_cast<ImplicitCastExpr>(expr->getBase())) {
      if (implicitCast->getSubExpr()->getType().getTypePtr()->isArrayType()) {
        if (DeclRefExpr *declRef = dyn_cast<DeclRefExpr>(implicitCast->getSubExpr())) {
          ArrayType* arr = dyn_cast<ArrayType>(declRef->getDecl()->getType().getTypePtr());
          if (arr->getElementType().getTypePtr()->isAnyCharacterType()) {
            varLiteral = new Buffer(declRef->getDecl());           
          }
        }
      }
    }
    // base is a pointer
    else if (DeclRefExpr *declRef = dyn_cast<DeclRefExpr>(expr->getBase())) {
      PointerType* pType = dyn_cast<PointerType>(declRef->getDecl()->getType().getTypePtr());
      if (pType->getPointeeType()->isAnyCharacterType()) {
        varLiteral = new Pointer(declRef->getDecl());       
      }
    }
    else {
      // TODO: Any other cases?
    }
    
    if (NULL == varLiteral) {
      return false;
    }
    
    Constraint usedMax, usedMin;

    usedMax.addBig(varLiteral->NameExpression(MAX, USED));
    usedMax.addSmall(GenerateIntegerExpression(expr->getIdx(), true));
    cp_.AddConstraint(usedMax);
    log::os() << "Adding - " << varLiteral->NameExpression(MAX, USED) << " >= " << GenerateIntegerExpression(expr->getIdx(), true).toString() << "\n";

    usedMin.addSmall(varLiteral->NameExpression(MIN, USED));
    usedMin.addBig(GenerateIntegerExpression(expr->getIdx(), false));
    log::os() << "Adding - " << varLiteral->NameExpression(MIN, USED) << " <= " << GenerateIntegerExpression(expr->getIdx(), false).toString() << "\n";
    cp_.AddConstraint(usedMin);

    delete varLiteral;
    return true;
  }

 public:
  ConstraintGenerator(SourceManager &SM, ConstraintProblem &CP) : sm_(SM), cp_(CP) {}

  bool VisitStmt(Stmt* S) {
    if (ArraySubscriptExpr* expr = dyn_cast<ArraySubscriptExpr>(S)) {
      return GenerateArraySubscriptConstraints(expr);
    }

    if (DeclStmt* dec = dyn_cast<DeclStmt>(S)) {
      for (DeclGroupRef::iterator i = dec->decl_begin(), end = dec->decl_end(); i != end; ++i) {
        if (VarDecl* var = dyn_cast<VarDecl>(*i)) {
          if (ConstantArrayType* arr = dyn_cast<ConstantArrayType>(var->getType().getTypePtr())) {
            if (arr->getElementType().getTypePtr()->isAnyCharacterType()) {
              Buffer buf(var);
              Constraint allocMax, allocMin;

              allocMax.addBig(buf.NameExpression(MAX, ALLOC));
              allocMax.addSmall(arr->getSize().getLimitedValue());
              cp_.AddConstraint(allocMax);
              log::os() << "Adding - " << buf.NameExpression(MAX, ALLOC) << " >= " << arr->getSize().getLimitedValue() << "\n";

              allocMin.addSmall(buf.NameExpression(MIN, ALLOC));
              allocMin.addBig(arr->getSize().getLimitedValue());
              log::os() << "Adding - " << buf.NameExpression(MIN, ALLOC) << " <= " << arr->getSize().getLimitedValue() << "\n";
              cp_.AddConstraint(allocMin);
            }
          }
        }
      }
    }


//    if (BinaryOperator* op = dyn_cast<BinaryOperator>(S)) {
//      //if (!op->isAssignmentOp()) {
//        return true;
//      //}

//      op->dump();
//      if (op->getLHS()->getType()->isIntegerType() && op->getRHS()->getType()->isIntegerType()) {
//        if (DeclRefExpr* dre = dyn_cast<DeclRefExpr>(op->getLHS())) {
//          log::os() << "Is at " << (void*)(dre->getDecl()) << "\n";
//        }
//      }
//    }

    return true;
  }
};

}  // namespace boa

#endif  // __BOA_CONSTRAINTGENERATOR_H

