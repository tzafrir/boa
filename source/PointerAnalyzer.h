#ifndef __BOA_POINTERANALYZER_H
#define __BOA_POINTERANALYZER_H

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
#include "pointer.h"
using boa::Pointer;

#include <list>
using std::list;
#include <map>
using std::map;

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
  list<Pointer> Pointers_;
  map< Pointer, list<Buffer>& > Pointer2Buffers_;
  SourceManager &sm_;
public:
  PointerASTVisitor(SourceManager &SM)
    : sm_(SM) {}

  bool VisitStmt(Stmt* S) {
    if (DeclStmt* dec = dyn_cast<DeclStmt>(S)) {
      for (DeclGroupRef::iterator i = dec->decl_begin(), end = dec->decl_end(); i != end; ++i) {
        findStaticBufferDecl(*i);
      }
    }
    else if (CallExpr* funcCall = dyn_cast<CallExpr>(S)) {
      if (FunctionDecl* funcDec = funcCall->getDirectCallee())
      {
         if (funcDec->getNameInfo().getAsString() == "malloc")
         {
            addMallocToSet(funcCall,funcDec);
         }
      }
    }
    return true;
  }

  void findStaticBufferDecl(Decl *d) {
    if (VarDecl* var = dyn_cast<VarDecl>(d)) {
      Type* varType = var->getType().getTypePtr();
      // FIXME - This code only detects an array of chars
      // Array of array of chars will probably NOT detected
      // As well as "array of MyChar" When using "typedef MyChar char".
      if (ArrayType* arr = dyn_cast<ArrayType>(varType)) {
        if (arr->getElementType().getTypePtr()->isAnyCharacterType())
        {
          addBufferToSet(var);
          
        }
      }
      else if (PointerType* pType = dyn_cast<PointerType>(varType))
      {
        if (pType->getPointeeType()->isAnyCharacterType())
        {
           addPointerToSet(var);
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
    cerr << " line number = " << sm_.getSpellingLineNumber(var->getLocation()) << endl;
    Buffers_.push_back(b);
  }

  void addMallocToSet(CallExpr* funcCall, FunctionDecl* func) {
    cerr << "malloc on line " << sm_.getSpellingLineNumber(funcCall->getExprLoc()) << endl;
    
    Buffer b((void*)funcCall);
    Buffers_.push_back(b);
  }

  void addPointerToSet(VarDecl* var) {
    var->dump();
    cerr << " was added to pointers set" << endl;

    Pointer p((void*)var);
    cerr << " code name  = " << var->getNameAsString() << endl;
    cerr << " \"clang ID\" = " << (void*)var << endl;
    cerr << " line number = " << sm_.getSpellingLineNumber(var->getLocation()) << endl;
    Pointers_.push_back(p);
  }
  
  const list<Buffer>& getBuffers() const {
    return Buffers_;
  }

  const list<Pointer>& getPointers() const {
    return Pointers_;
  }
};

}  // namespace boa

#endif  // __BOA_POINTERANALYZER_H
