#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/AST.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/raw_ostream.h"

#include "buffer.h"
using boa::Buffer;
#include <list>
using std::list;
#include "constraint.h"

// DEBUG
#include <iostream>
using std::cerr;
using std::endl;

using namespace clang;

namespace boa {

class PointerASTVisitor : public RecursiveASTVisitor<PointerASTVisitor> {
private:
  list<Buffer> Buffers_;
  SourceManager &sm_;
public:
  PointerASTVisitor(SourceManager &SM)
    : sm_(SM) {}

  bool VisitStmt(Stmt* S) {
    if (DeclStmt* dec = dyn_cast<DeclStmt>(S)) {
      for (DeclGroupRef::iterator i = dec->decl_begin(), end = dec->decl_end(); i != end; ++i) {
        MyVisitDeclStmt(*i);
      }
    }
    else if (CallExpr* funcCall = dyn_cast<CallExpr>(S)) {
      if (FunctionDecl* funcDec = funcCall->getDirectCallee())
      {
         //if (funcDec->getNameInfo()->getAsString() == "malloc")
         {
            addMallocToSet(funcCall,funcDec);
         }
      }
    }
    return true;
  }

  void MyVisitDeclStmt(Decl *d) {
    if (VarDecl* var = dyn_cast<VarDecl>(d)) {
      // FIXME - This code only detects an array of chars
      // Array of array of chars will probably NOT detected
      // As well as "array of MyChar" When using "typedef MyChar char".
      if (ArrayType* arr = dyn_cast<ArrayType>(var->getType().getTypePtr())) {
        if (BuiltinType* arrType = dyn_cast<BuiltinType>(arr->getElementType())) {
          switch (arrType->getKind()) {
            case BuiltinType::Char_U:
            case BuiltinType::UChar: 
            case BuiltinType::Char16:
            case BuiltinType::Char32: 
            case BuiltinType::Char_S: 
            case BuiltinType::SChar:
            case BuiltinType::WChar:
              addBufferToSet(var);
              break;
            default:
              // Not a char array
              break;
          }
        }
      }
    }
  }

  void addBufferToSet(VarDecl* var) {
    var->dump();
    cerr << " was added to buffers set" << endl;

    Buffer b((void*)var);
    cerr << " code name  = " << var->getNameAsString() << endl;
    cerr << " \"clang ID\" = " << (void*)var << endl;
    // TODO - get var's code location (Tzafrir?)
    Buffers_.push_back(b);
  }

  void addMallocToSet(CallExpr* funcCall, FunctionDecl* func) {
    cerr << func->getNameInfo().getAsString() << endl;
    cerr << "malloc_" << sm_.getSpellingLineNumber(funcCall->getExprLoc()) << endl;
    
    Buffer b((void*)funcCall);
    Buffers_.push_back(b);
  }
  
  const list<Buffer>& getBuffers() const {
    return Buffers_;
  }
};

}

