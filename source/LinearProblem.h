#ifndef __BOA_LINEAR_PROBLEM_H__
#define __BOA_LINEAR_PROBLEM_H__ /* */

#include <glpk.h>
#include <limits.h>

#define MINUS_INFTY (std::numeric_limits<int>::min())

namespace boa {

class LinearProblem {
  // because of llvm command structure, each constraint affect 3(?) variables at the most
  static const int MAX_VARS = 10;

  glp_smcp params_;

  static bool isMax(string s) {
    return (s.substr(s.length() - 3) == "max");
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

  LinearProblem& operator=(const LinearProblem &old) {
    if (&old != this) {
      glp_delete_prob(this->lp_);
      copyFrom(old);
    }
    return *this;
  }

  ~LinearProblem() {
    glp_delete_prob(this->lp_);
  }

  void SetParams(const glp_smcp& params) {
    params_ = params;
  }

  int Solve() {
    glp_simplex(lp_, &params_);
    return glp_get_status(lp_);
  }

/**
  Remove a row from a linear problem matrix, create "unbound constraints" instead

  The new constraints created at the end of the matrix.
*/
  void RemoveRow(int row, map<int, string>& colToVar) {
    static int indices[MAX_VARS];
    static double values[MAX_VARS];

    int nonZeros = glp_get_mat_row(lp_, row, indices, values);
    glp_set_row_bnds(lp_, row, GLP_FR, 0.0, 0.0);

    int ind[2];
    double val[2];
    for (int i = 1; i <= nonZeros; ++i) {
      ind[1] = indices[i];
      val[1] = (isMax(colToVar[indices[i]]) ? -1 : 1);
      int r = glp_add_rows(lp_, 1);
      glp_set_row_bnds(lp_, r, GLP_UP, 0.0, MINUS_INFTY);
      glp_set_mat_row(lp_, r, 1, ind, val);
    }
  }

  int NumCols() const {
    return glp_get_num_cols(lp_);
  }
};

} // namespace boa

#endif // __BOA_LINEAR_PROBLEM_H__
