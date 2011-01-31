#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/InstIterator.h"

//#include "Buffer.h"
//#include "Constraint.h"
#include "ConstraintGenerator.h"
#include "ConstraintProblem.h"
#include "log.h"

#include <fstream>
#include <iostream>

//#include <vector>

//using std::vector;

using std::cerr;
using std::ofstream;
using std::ios_base;

using namespace llvm;

cl::opt<string> LogFile("logfile", cl::desc("Log to filename"), cl::value_desc("filename"));

namespace boa {
static const string SEPARATOR("---");

class boa : public ModulePass {
 private:
  ConstraintProblem constraintProblem_;

 public:
  static char ID;


  boa() : ModulePass(ID) {
    if (LogFile != "") {
      ofstream* logfile = new ofstream();
      logfile->open(LogFile.c_str());
      log::set(*logfile);
    } else {
      log::set(cerr);
    }
   }

  virtual bool runOnModule(Module &M) {
    ConstraintGenerator constraintGenerator(constraintProblem_);

    for (Module::const_global_iterator it = M.global_begin(); it != M.global_end(); ++it) {
      const GlobalValue *g = it;
      constraintGenerator.VisitGlobal(g);
    }
    for (Module::const_iterator it = M.begin(); it != M.end(); ++it) {
      const Function *F = it;
      for (const_inst_iterator ii = inst_begin(F); ii != inst_end(F); ++ii) {
        constraintGenerator.VisitInstruction(&(*ii));
      }
    }
    return false;
  }

  virtual ~boa() {
//    const map<Pointer, vector<Buffer>* >& mapping = pointerAnalyzer_.getMapping();
//    for (map<Pointer, vector<Buffer>* >::const_iterator pointerIt = mapping.begin();
//         pointerIt != mapping.end();
//         ++pointerIt) {
//      const Pointer &ptr = pointerIt->first;
//      vector<Buffer>* buffers = pointerIt->second;

//      Constraint usedLenMax;
//      usedLenMax.addBig(ptr.NameExpression(VarLiteral::MAX, VarLiteral::USED));
//      usedLenMax.addSmall(ptr.NameExpression(VarLiteral::MAX, VarLiteral::LEN_WRITE));
//      usedLenMax.SetBlame("Length constraint");
//      constraintProblem_.AddConstraint(usedLenMax);
//      LOG << "Adding - " << ptr.NameExpression(VarLiteral::MAX, VarLiteral::USED) << " >= " <<
//          ptr.NameExpression(VarLiteral::MAX, VarLiteral::LEN_WRITE) << "\n";

//      for (vector<Buffer>::const_iterator it = buffers->begin(); it != buffers->end(); ++it) {
//        Constraint usedMax, usedMin, lenMax;

//        usedMax.addBig(it->NameExpression(VarLiteral::MAX, VarLiteral::USED));
//        usedMax.addSmall(ptr.NameExpression(VarLiteral::MAX, VarLiteral::USED));
//        usedMax.SetBlame("Pointer analyzer constraint");
//        constraintProblem_.AddConstraint(usedMax);
//        LOG << "Adding - " << it->NameExpression(VarLiteral::MAX, VarLiteral::USED) << " >= " <<
//            ptr.NameExpression(VarLiteral::MAX, VarLiteral::USED) << "\n";

//        usedMin.addBig(ptr.NameExpression(VarLiteral::MIN, VarLiteral::USED));
//        usedMin.addSmall(it->NameExpression(VarLiteral::MIN, VarLiteral::USED));
//        usedMin.SetBlame("Pointer analyzer constraint");
//        constraintProblem_.AddConstraint(usedMin);
//        LOG << "Adding - " << ptr.NameExpression(VarLiteral::MIN, VarLiteral::USED) << " >= " <<
//            it->NameExpression(VarLiteral::MIN, VarLiteral::USED) << "\n";

//        lenMax.addBig(it->NameExpression(VarLiteral::MAX, VarLiteral::LEN_READ));
//        lenMax.addSmall(ptr.NameExpression(VarLiteral::MAX, VarLiteral::LEN_READ));
//        lenMax.SetBlame("Pointer analyzer constraint");
//        constraintProblem_.AddConstraint(lenMax);
//        LOG << "Adding - " << it->NameExpression(VarLiteral::MAX, VarLiteral::LEN_READ) << " >= " <<
//            ptr.NameExpression(VarLiteral::MAX, VarLiteral::LEN_READ) << "\n";
//      }
//    }

//    const vector<Buffer> &Buffers = pointerAnalyzer_.getBuffers();
//    for (vector<Buffer>::const_iterator buf = Buffers.begin(); buf != Buffers.end(); ++buf) {
//      Constraint UsedMin, UsedMax, LenLen;
//      UsedMax.addBig(buf->NameExpression(VarLiteral::MAX, VarLiteral::USED));
//      UsedMax.addSmall(buf->NameExpression(VarLiteral::MAX, VarLiteral::LEN_READ));
//      UsedMax.SetBlame("Length constraint");
//      constraintProblem_.AddConstraint(UsedMax);
//      LOG << "Adding - " << buf->NameExpression(VarLiteral::MAX, VarLiteral::USED) << " >= " <<
//          buf->NameExpression(VarLiteral::MAX, VarLiteral::LEN_READ) << "\n";

//      UsedMin.addSmall(buf->NameExpression(VarLiteral::MIN, VarLiteral::USED));
//      UsedMin.addBig(buf->NameExpression(VarLiteral::MAX, VarLiteral::LEN_READ));
//      UsedMin.SetBlame("Length constraint");
//      constraintProblem_.AddConstraint(UsedMin);
//      LOG << "Adding - " << buf->NameExpression(VarLiteral::MIN, VarLiteral::USED) << " <= " <<
//          buf->NameExpression(VarLiteral::MAX, VarLiteral::LEN_READ) << "\n";

//      LenLen.addBig(buf->NameExpression(VarLiteral::MAX, VarLiteral::LEN_READ));
//      LenLen.addSmall(buf->NameExpression(VarLiteral::MAX, VarLiteral::LEN_WRITE));
//      LenLen.SetBlame("Length constraint");
//      constraintProblem_.AddConstraint(LenLen);
//      LOG << "Adding - " << buf->NameExpression(VarLiteral::MAX, VarLiteral::LEN_READ) << " >= " <<
//          buf->NameExpression(VarLiteral::MAX, VarLiteral::LEN_WRITE) << "\n";
//    }

//    LOG << "The buffers we have found - " << endl;
//    for (vector<Buffer>::const_iterator buf = Buffers.begin(); buf != Buffers.end(); ++buf) {
//      LOG << buf->getUniqueName() << endl;
//      constraintProblem_.AddBuffer(*buf);
//    }
    LOG << "Constraint solver output - " << endl;
    vector<Buffer> unsafeBuffers = constraintProblem_.Solve();
    if (unsafeBuffers.empty()) {
      cerr << endl << "No overruns detected" << endl;
      cerr << SEPARATOR << endl;
      cerr << SEPARATOR << endl;
    } else {
      cerr << endl << "Possible buffer overruns on - " << endl;
//      if (blameOverruns_) {
//        map<Buffer, vector<Constraint> > blames = constraintProblem_.SolveAndBlame();
//        for (map<Buffer, vector<Constraint> >::iterator it = blames.begin();
//             it != blames.end();
//             ++it) {
//          cerr << endl << it->first.getReadableName() << " " <<
//              it->first.getSourceLocation() << endl;
//          for (size_t i = 0; i < it->second.size(); ++i) {
//            cerr << "  - " << it->second[i].Blame() << endl;
//          }
//        }
//      }
      cerr << SEPARATOR << endl;
      for (vector<Buffer>::iterator buff = unsafeBuffers.begin();
           buff != unsafeBuffers.end();
           ++buff) {
        cerr << buff->getReadableName() << " " << buff->getSourceLocation() << endl;
      }
      cerr << SEPARATOR << endl;
    }
  }

};
}

char boa::boa::ID = 0;
static RegisterPass<boa::boa> X("boa", "boa - buffer overrun analyzer");

//// DEBUG
//#include <iostream>
//using std::cerr;
//using std::endl;

//using namespace clang;

//namespace {

//class boaConsumer : public ASTConsumer {
// private:
//  SourceManager &sm_;
//  PointerASTVisitor pointerAnalyzer_;
//  ConstraintProblem constraintProblem_;
//  ConstraintGenerator constraintGenerator_;
//  bool blameOverruns_;

// public:
//  boaConsumer(SourceManager &SM, bool blameOverruns) : sm_(SM), pointerAnalyzer_(SM),
//                                                       constraintGenerator_(SM, constraintProblem_),
//                                                       blameOverruns_(blameOverruns) {}

//  virtual void HandleTopLevelDecl(DeclGroupRef DG) {
//    for (DeclGroupRef::iterator i = DG.begin(), e = DG.end(); i != e; ++i) {
//      Decl *D = *i;
//      pointerAnalyzer_.TraverseDecl(D);
//      pointerAnalyzer_.findVarDecl(D);
//      constraintGenerator_.TraverseDecl(D);
//    }
//  }

//  }
//};

//class boaPlugin : public PluginASTAction {
// protected:
//  bool blameOverruns_;

//  ASTConsumer *CreateASTConsumer(CompilerInstance &CI, llvm::StringRef) {
//    return new boaConsumer(CI.getSourceManager(), blameOverruns_);
//  }

//  bool ParseArgs(const CompilerInstance &CI, const std::vector<std::string>& args) {
//    blameOverruns_ = false;
//    for (unsigned i = 0; i < args.size(); ++i) {
//      if (args[i] == "log") {
//        log::set(std::cout);
//      } else if (args[i] == "blame") {
//        blameOverruns_ = true;
//      }
//    }
//    return true;
//  }
//};

//}
