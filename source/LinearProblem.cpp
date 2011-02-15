#include "LinearProblem.h"

#include <vector>
#include <map>
#include <algorithm>

using std::vector;
using std::map;
using std::sort;

namespace {
inline int max(int a, int b) {
  return (a > b) ? a : b;
}
}

namespace boa {
vector<int> LinearProblem::ElasticFilter() const {
  LinearProblem tmp(*this);

  int realCols = glp_get_num_cols(tmp.lp_);
  for (int i = realCols; i > 0; --i) { // set old coefs to zero
    glp_set_obj_coef(tmp.lp_, i,  0);
  }

  int elasticCols = glp_get_num_rows(tmp.lp_);
  glp_add_cols(tmp.lp_, elasticCols);
  for (int i = 1; i <= elasticCols; ++i) {
    int indices[MAX_VARS];
    double values[MAX_VARS];
    int nonZeros = glp_get_mat_row(tmp.lp_, i, indices, values);

    indices[nonZeros + 1] = realCols + i;
    values[nonZeros + 1] = 1.0;

    glp_set_mat_row(tmp.lp_, i, nonZeros + 1, indices, values);
    glp_set_obj_coef(tmp.lp_, realCols + i,  1);
    glp_set_col_bnds(tmp.lp_, realCols + i, GLP_UP, 0.0, 0.0);
  }

  vector<int> suspects;
  glp_std_basis(tmp.lp_);
  int status = tmp.Solve();
  while ((status != GLP_INFEAS) && (status != GLP_NOFEAS)) {
    for (int i = 1; i <= elasticCols; ++i) {
      if (glp_get_col_prim(tmp.lp_, realCols + i) < 0) {
        suspects.push_back(i);
        glp_set_col_bnds(tmp.lp_, realCols + i, GLP_FX, 0.0, 0.0);
      }
    }
    status = tmp.Solve();
  }

  return suspects;
}

void LinearProblem::RemoveRow(int row, map<int, string>& colToVar) {
  static int indices[MAX_VARS];
  static double values[MAX_VARS];

  int nonZeros = glp_get_mat_row(lp_, row, indices, values);
  glp_set_row_bnds(lp_, row, GLP_FR, 0.0, 0.0);

  // glpk ignores the 0's index of the array
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

LinearProblem& LinearProblem::operator=(const LinearProblem &old) {
  if (&old != this) {
    glp_delete_prob(this->lp_);
    copyFrom(old);
  }
  return *this;
}

void LinearProblem::RemoveInfeasable(map<int, string>& colToVar) {
  LOG << "No Feasable solution, running elastic filter - " << endl;

  vector<int> rows = ElasticFilter();
  sort(rows.begin(), rows.end());

  int ind[2], removed = rows.size();
  realRows_ -= removed;
  LOG << "removing " << removed << " rows" << endl;  
  for (int i = 0; i < removed; ++i) {
    int cur = rows[i] - i;
    RemoveRow(cur, colToVar);
    ind[1] = cur;
    glp_del_rows(lp_, 1, ind);
  }
  glp_std_basis(lp_);
  realRows_ -= removed;
}

} // namespace boa
