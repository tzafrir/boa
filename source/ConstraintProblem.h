#ifndef __BOA_CONSTRAINT_PROBLEM_H__
#define __BOA_CONSTRAINT_PROBLEM_H__

#include <vector>

#include "Constraint.h"
#include "LinearProblem.h"

using std::vector;

namespace boa {

class ConstraintProblem {
 private:
  const vector<Constraint> NO_CONSTRAINTS;
  vector<Constraint> constraints_;
  set<Buffer> buffers_;
  bool outputGlpk_;

  set<string> CollectVars() const;

  vector<Buffer> SolveProblem(LinearProblem lp) const;

  vector<string> Blame(LinearProblem lp, Buffer &buffer) const;
  
  LinearProblem MakeFeasableProblem() const;
 public:
  ConstraintProblem(bool output_glpk) : outputGlpk_(output_glpk) {}

  void AddBuffer(const Buffer& buffer) {
    buffers_.insert(buffer);
  }

  void AddConstraint(const Constraint& c) {
    constraints_.push_back(c);
  }

  void Clear() {
    buffers_.clear();
    constraints_.clear();
  }

  /**
    Solve the constriant problem defined by the constraints.

    Return a set of buffers in which buffer overrun may occur.
  */
  vector<Buffer> Solve() const;

  /**
    Solve the constraint problem and generate a minimal set of constraints which cause each overrun

    Return a map - the keys are possibly overrun buffers, the corresponding value is a small set of
    constraints which cause the overrun. The set is minimal in the sense that no subset of these
    constraint will cause the specific buffer overrun, there might be other (smaller) set which will
    also cause the overrun.
  */
  map<Buffer, vector<string> > SolveAndBlame() const;
};

}  // namespace boa

#endif /* __BOA_CONSTRAINT_PROBLEM_H__ */
