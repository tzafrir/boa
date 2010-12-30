#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/AST.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/raw_ostream.h"

#include "buffer.h"
#include "constraint.h"
#include "log.h"

#include "ConstraintGenerator.h"
#include "PointerAnalyzer.h"

using namespace boa;

#include <vector>

using std::vector;


// DEBUG
#include <iostream>
using std::cerr;
using std::endl;

using namespace clang;

namespace {
static const string SEPARATOR("---");

class boaConsumer : public ASTConsumer {
 private:
  SourceManager &sm_;
  PointerASTVisitor pointerAnalyzer_;
  ConstraintProblem constraintProblem_;
  ConstraintGenerator constraintGenerator_;
 public:
  boaConsumer(SourceManager &SM) : sm_(SM), pointerAnalyzer_(SM), constraintGenerator_(SM, constraintProblem_) {}

  virtual void HandleTopLevelDecl(DeclGroupRef DG) {
    for (DeclGroupRef::iterator i = DG.begin(), e = DG.end(); i != e; ++i) {
      Decl *D = *i;
      pointerAnalyzer_.TraverseDecl(D);
      pointerAnalyzer_.findVarDecl(D);
      constraintGenerator_.TraverseDecl(D);
    }
  }

  virtual ~boaConsumer() {
    const map<Pointer, vector<Buffer>* >& mapping = pointerAnalyzer_.getMapping();
    for (map<Pointer, vector<Buffer>* >::const_iterator pointerIt = mapping.begin(); pointerIt != mapping.end(); ++pointerIt) {
      const Pointer &ptr = pointerIt->first;
      vector<Buffer>* buffers = pointerIt->second;

      for (vector<Buffer>::const_iterator it = buffers->begin(); it != buffers->end(); ++it) {        
        Constraint usedMax, usedMin;

        usedMax.addBig(it->NameExpression(MAX, USED));
        usedMax.addSmall(ptr.NameExpression(MAX, USED));
        constraintProblem_.AddConstraint(usedMax);
        log::os() << "Adding - " << it->NameExpression(MAX, USED) << " >= " << ptr.NameExpression(MAX, USED) << "\n";
        
        usedMin.addBig(ptr.NameExpression(MIN, USED));
        usedMin.addSmall(it->NameExpression(MIN, USED));
        constraintProblem_.AddConstraint(usedMin);
        log::os() << "Adding - " << ptr.NameExpression(MIN, USED) << " >= " << it->NameExpression(MIN, USED) << "\n";
      }
    }

    log::os() << "The buffers we have found - " << endl;
    const vector<Buffer> &Buffers = pointerAnalyzer_.getBuffers();
    for (vector<Buffer>::const_iterator buf = Buffers.begin(); buf != Buffers.end(); ++buf) {
      log::os() << buf->getUniqueName() << endl;
      constraintProblem_.AddBuffer(*buf);
    }
    log::os() << "Constraint solver output - " << endl;
    vector<Buffer> unsafeBuffers = constraintProblem_.Solve();
    if (unsafeBuffers.empty()) {
      cerr << "boa[0]" << endl;
      cerr << endl << "No overruns possible" << endl;
      cerr << SEPARATOR << endl;
      cerr << SEPARATOR << endl;
    }
    else {
      cerr << "boa[1]" << endl;
      cerr << endl << "Possible buffer overruns on - " << endl;
      cerr << SEPARATOR << endl;
      for (vector<Buffer>::iterator buff = unsafeBuffers.begin(); buff != unsafeBuffers.end(); ++buff) {
        cerr << buff->getReadableName() << " " << buff->getSourceLocation() << endl;
      }
      cerr << SEPARATOR << endl;
    }
  }
};

class boaPlugin : public PluginASTAction {
 protected:
  ASTConsumer *CreateASTConsumer(CompilerInstance &CI, llvm::StringRef) {
    return new boaConsumer(CI.getSourceManager());
  }

  bool ParseArgs(const CompilerInstance &CI, const std::vector<std::string>& args) {
    for (unsigned i = 0; i < args.size(); ++i) {
      if (args[i] == "log") {
        log::set(std::cout);
       }
    }
    return true;
  }
};

}

static FrontendPluginRegistry::Add<boaPlugin>
X("boa", "boa - Buffer Overrun Analyzer");

