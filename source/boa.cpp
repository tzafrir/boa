#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/AST.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/raw_ostream.h"

#include "PointerAnalyzer.h"
#include "buffer.h"
using namespace boa;
#include <list>
using std::list;
#include "constraint.h"

// DEBUG
#include <iostream>
using std::cerr;
using std::endl;

using namespace clang;

namespace {

class boaConsumer : public ASTConsumer {
public:
  boaConsumer(SourceManager &SM) : sm_(SM), PointerAnalyzer_(SM) {}

  virtual void HandleTopLevelDecl(DeclGroupRef DG) {
    for (DeclGroupRef::iterator i = DG.begin(), e = DG.end(); i != e; ++i) {
      Decl *D = *i;
      PointerAnalyzer_.TraverseDecl(D);
      PointerAnalyzer_.MyVisitDeclStmt(D);
      // TODO - call constraint generator here
    }
  }
  
  virtual ~boaConsumer() {
    // TODO - call constraint dispach here
    
    cerr << endl << "The buffers we have found - " << endl;
    const list<Buffer> &Buffers = PointerAnalyzer_.getBuffers();
    for (list<Buffer>::const_iterator buf = Buffers.begin(); buf != Buffers.end(); ++buf) {
      cerr << buf->getUniqueName() << endl;
      constriantProb_.AddBuffer(*buf);
    }
    cerr << endl << "Constraint solver output - " << endl;
    constriantProb_.Solve();  
  }

private:
  SourceManager &sm_;
  PointerASTVisitor PointerAnalyzer_;
  ConstraintProblem constriantProb_;
};

class boaPlugin : public PluginASTAction {
protected:
  ASTConsumer *CreateASTConsumer(CompilerInstance &CI, llvm::StringRef) {
    return new boaConsumer(CI.getSourceManager());
  }
  
  bool ParseArgs(const CompilerInstance &CI, const std::vector<std::string>& args) {
    return true;
  }
};

}

static FrontendPluginRegistry::Add<boaPlugin>
X("boa", "boa - Buffer Overrun Analyzer");

