#ifndef __BOA_LINEAR_PROBLEM_H__
#define __BOA_LINEAR_PROBLEM_H__ /* */

#include <glpk.h>
#include <limits>
#include <vector>
#include <map>

using std::vector;
using std::map;

#include "log.h"

#define MINUS_INFTY (std::numeric_limits<int>::min())

namespace boa {

class LinearProblem {
  // because of llvm command structure, each constraint affect 3(?) variables at the most
  static const int MAX_VARS = 10;

  glp_smcp params_;

  static bool isMax(string s) {
    return (s.substr(s.length() - 3) == "max");
  }

  static bool IsFeasable(int status) {
    return ((status != GLP_INFEAS) && (status != GLP_NOFEAS));
  }

  void copyFrom(const LinearProblem &old) {
    this->lp_ = glp_create_prob();
    glp_copy_prob(this->lp_, old.lp_, GLP_OFF);
    this->params_ = old.params_;
  }

 public:
  glp_prob *lp_;
  int realRows_;

  LinearProblem() {
    lp_ = glp_create_prob();
  }

  LinearProblem(const LinearProblem &old) {
    copyFrom(old);
  }

  LinearProblem& operator=(const LinearProblem &old);

  ~LinearProblem() {
    glp_delete_prob(this->lp_);
  }

  /**
    Efficiently identify a small group of infeasble constraints using elastic fileter algorithm
  */
  vector<int> ElasticFilter() const;

  void SetParams(const glp_smcp& params) {
    params_ = params;
  }


  /**
    Solve the linear problem and return the glpk status
  */
  int Solve() {
    glp_simplex(lp_, &params_);
    return glp_get_status(lp_);
  }

  /**
    Remove a row from a linear problem matrix, create "unbound constraints" instead

    The new constraints created at the end of the matrix.
  */
  void RemoveRow(int row, map<int, string>& colToVar);

  int NumCols() const {
    return glp_get_num_cols(lp_);
  }

  /**
    Remove a minimal set of constraints that makes the lp inFeasable.

    Each of the variables in the removed rows will become "unbound" (e.g. - ...!max >= MAX_INT  or
    ...!min <= MIN_INT).

    The removed set is *A* minimal in the sense that these lines alone form an inFeasable problem, and
    each subset of them does not. It is not nessecerily *THE* minimal set in the sense that there is
    no such set which is smaller in size.
  */
  void RemoveInfeasable(map<int, string>& colToVar);
};

} // namespace boa

#endif // __BOA_LINEAR_PROBLEM_H__
