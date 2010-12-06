#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/AST.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/raw_ostream.h"

#include <iostream>
using std::cout;
using std::endl;

using namespace clang;

namespace {

class PointerASTVisitor : public RecursiveASTVisitor<PointerASTVisitor> {
public:
  bool VisitStmt(Stmt* S) {  	
    if (DeclStmt* dec = dyn_cast<DeclStmt>(S)) {
      for (DeclGroupRef::iterator i = dec->decl_begin(), end = dec->decl_end(); i != end; ++i) {
				MyVisitDeclStmt(*i);
      }
    }
    return true;
  }
  
  void MyVisitDeclStmt(Decl *d) {
		if (VarDecl* var = dyn_cast<VarDecl>(d)) {
      var->dump();
			cout << " boa >> " << var->getStorageClassSpecifierString() << " << boa " << endl;
		}  
  }
};

class PointerAnalysisConsumer : public ASTConsumer {
public: 
  virtual void HandleTopLevelDecl(DeclGroupRef DG) {
    for (DeclGroupRef::iterator i = DG.begin(), e = DG.end(); i != e; ++i) {
      Decl *D = *i;
      PointerASTVisitor iav;
      iav.TraverseDecl(D);
      iav.MyVisitDeclStmt(D);
    }
  }
};

class PointerAnalyzer : public PluginASTAction {
protected:
  ASTConsumer *CreateASTConsumer(CompilerInstance &CI, llvm::StringRef) {
    return new PointerAnalysisConsumer();
  }
  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string>& args) {
		return true;
	}    
};

}

static FrontendPluginRegistry::Add<PointerAnalyzer>
X("PointerAnalyzer", "boa pointer analyzer");

