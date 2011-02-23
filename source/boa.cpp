#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/InstIterator.h"

#include "Buffer.h"
#include "ConstraintGenerator.h"
#include "ConstraintProblem.h"
#include "log.h"

#include <fstream>
#include <iostream>
#include <vector>

using std::vector;
using std::cerr;
using std::ofstream;
using std::ios_base;

using namespace llvm;

cl::opt<string> LogFile("logfile", cl::desc("Log to filename"), cl::value_desc("filename"));
cl::opt<bool> OutputGlpk("output_glpk", cl::desc("Show GLPK Output"), cl::value_desc(""));
cl::opt<bool> Blame("blame", cl::desc("Calculate and show Blame information"), cl::value_desc(""));

namespace boa {
static const string SEPARATOR("---");

class boa : public ModulePass {
 private:
  ConstraintProblem constraintProblem_;

 public:
  static char ID;


  boa() : ModulePass(ID), constraintProblem_(OutputGlpk) {
    if (LogFile != "") {
      ofstream* logfile = new ofstream();
      logfile->open(LogFile.c_str());
      log::set(*logfile);
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
        constraintGenerator.VisitInstruction(&(*ii), F);
      }
    }
    
    constraintGenerator.AnalyzePointers();
    return false;
  }

  virtual ~boa() {
    LOG << "Constraint solver output - " << endl;
    vector<Buffer> unsafeBuffers = constraintProblem_.Solve();
    cerr << constraintProblem_.BuffersCount() << " buffers found" << endl;
    if (unsafeBuffers.empty()) {
      cerr << endl << "No overruns detected" << endl;
      cerr << SEPARATOR << endl;
      cerr << SEPARATOR << endl;
      cerr << SEPARATOR << endl;
    } else {
      cerr << endl << "Possible buffer overruns on - " << endl;
      cerr << SEPARATOR << endl;
      if (Blame) {
        map<Buffer, vector<string> > blames = constraintProblem_.SolveAndBlame();
        for (map<Buffer, vector<string> >::iterator it = blames.begin();
             it != blames.end();
             ++it) {
          cerr << it->first.getReadableName() << " " <<
              it->first.getSourceLocation() << endl;
          for (size_t i = 0; i < it->second.size(); ++i) {
            cerr << "  - " << it->second[i] << endl;
          }
        }
      }
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
