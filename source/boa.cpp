#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/InstIterator.h"

#include "Buffer.h"
#include "ConstraintGenerator.h"
#include "ConstraintProblem.h"
#include "Helpers.h"
#include "log.h"

#include <fstream>
#include <iostream>
#include <unistd.h>
#include <vector>
#include <set>

using std::cerr;
using std::ios_base;
using std::ofstream;
using std::set;
using std::vector;

using namespace llvm;

cl::opt<string> LogFile("logfile", cl::desc("Log to filename"), cl::value_desc("filename"));
cl::opt<bool> OutputGlpk("output_glpk", cl::desc("Show GLPK Output"), cl::value_desc(""));
cl::opt<bool> Blame("blame", cl::desc("Calculate and show Blame information"), cl::value_desc(""));
cl::opt<bool> NoPointerAnalysis("no_pointer_analysis",
                   cl::desc("Do not generate pointer analysis constraints"), cl::value_desc(""));
cl::opt<bool> IgnoreLiterals("ignore_literals",
                   cl::desc("Don't report buffer overruns on string literals"), cl::value_desc(""));
cl::opt<bool> Verbose("v", cl::desc("Verbose output format"), cl::value_desc(""));
cl::opt<string> SafeFunctions("", cl::desc("Names of safe functions"), cl::value_desc(""));
cl::opt<string> UnsafeFunctions("", cl::desc("Names of unsafe functions"), cl::value_desc(""));

namespace boa {
static const string SEPARATOR("---");

namespace Colors {
  static string Red, Green, Normal, Bold;
  static void Setup() {
    Red     = "\033[0;31m";
    Green   = "\033[0;32m";
    Normal  = "\033[0m";
    Bold    = "\033[1m";
  }
}

class boa : public ModulePass {
 private:
  ConstraintProblem constraintProblem_;
  set<string> safeFunctions, unsafeFunctions;

 public:
  static char ID;


  boa() : ModulePass(ID), constraintProblem_(OutputGlpk) {
    if (LogFile != "") {
      ofstream* logfile = new ofstream();
      logfile->open(LogFile.c_str());
      log::set(*logfile);
    }
    if (isatty(2)) {
      // use colors only if stderror is a tty
      Colors::Setup();
    }
   }

  virtual bool runOnModule(Module &M) {
    ConstraintGenerator constraintGenerator(constraintProblem_, IgnoreLiterals, safeFunctions,
                                            unsafeFunctions);

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

    if (!NoPointerAnalysis) {
      constraintGenerator.AnalyzePointers();
    }
    return false;
  }

  virtual ~boa() {
    if (constraintProblem_.BuffersCount() == 0) {
      cerr << "no buffers detected" << endl;
      cerr << SEPARATOR << endl;
      cerr << SEPARATOR << endl;
      cerr << SEPARATOR << endl;
      return;
    }
    LOG << "Constraint solver output - " << endl;
    vector<Buffer> unsafeBuffers = constraintProblem_.Solve();
    cerr << Colors::Bold << "boa" << Colors::Normal << " found "
         << constraintProblem_.BuffersCount() << " buffers. ";
    if (unsafeBuffers.empty()) {
      cerr << endl << Colors::Green << "No overruns detected" << Colors::Normal << "." << endl;
      cerr << SEPARATOR << endl;
      cerr << SEPARATOR << endl;
      cerr << SEPARATOR << endl;
    } else {
      cerr << Colors::Red << unsafeBuffers.size() << " possible buffer overruns found"
           << Colors::Normal << "." << endl;
      cerr << SEPARATOR << endl;
      if (Blame) {
        if (Verbose) {
          cerr << Colors::Bold << "Blames section" << Colors::Normal << " Each of the overrunning "
                  "buffers appear here with a small list of constraints which cause an overrun in "
                  "this buffer. A buffer is described by its name and the source location where it is "
                  "defined, a constraint consist of a brief desctiption and the source line where "
                  "it originates." << endl << endl;
        }
        map<Buffer, vector<string> > blames = constraintProblem_.SolveAndBlame();
        for (map<Buffer, vector<string> >::iterator it = blames.begin();
             it != blames.end();
             ++it) {
          cerr << Colors::Red << it->first.getReadableName() << Colors ::Normal << " " <<
              it->first.getSourceLocation() << endl;
          for (size_t i = 0; i < it->second.size(); ++i) {
            cerr << "  - " << it->second[i] << endl;
          }
        }
      }
      cerr << SEPARATOR << endl;
      if (Verbose) {
        cerr << Colors::Bold << "Buffers section" << Colors::Normal << " Each of the overrunning "
                "buffers appear here, one in each line. A buffer is described by its name and the "
                "source location where it is defined" << endl << endl;
      }
      for (vector<Buffer>::iterator buff = unsafeBuffers.begin();
           buff != unsafeBuffers.end();
           ++buff) {
        cerr << Colors::Red << buff->getReadableName() << Colors::Normal << " " <<
                buff->getSourceLocation() << endl;
      }
      cerr << SEPARATOR << endl;
    }
  }
};
}

char boa::boa::ID = 0;
static RegisterPass<boa::boa> X("boa", "boa - buffer overrun analyzer");
