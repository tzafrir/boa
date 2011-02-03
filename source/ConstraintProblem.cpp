#include "ConstraintProblem.h"

#include "LinearProblem.h"

#include <iostream>
#include <glpk.h>
#include "log.h"

using std::endl;

namespace boa {


/**
 * A printing function for GLPK.
 *
 * @see { glpk.pdf / glp_term_hook }
 */
static int printToLog(void *info, const char *s) {
  log::os() << s;
  return 1;  // Non zero.
}

set<string> ConstraintProblem::CollectVars() const {
  set<string> vars;
  for (set<Buffer>::const_iterator buffer = buffers.begin(); buffer != buffers.end(); ++buffer) {
    vars.insert(buffer->NameExpression(VarLiteral::MIN, VarLiteral::USED));
    vars.insert(buffer->NameExpression(VarLiteral::MAX, VarLiteral::USED));
    vars.insert(buffer->NameExpression(VarLiteral::MIN, VarLiteral::ALLOC));
    vars.insert(buffer->NameExpression(VarLiteral::MAX, VarLiteral::ALLOC));
  }

  for (vector<Constraint>::const_iterator constraint = constraints.begin();
       constraint != constraints.end();
       ++constraint) {
    constraint->GetVars(vars);
  }
  return vars;
}


inline static void MapVarToCol(const set<string>& vars, map<string, int>& varToCol /* out */,
                               map<int, string>& colToVar /* out */) {
  int col = 1;
  for (set<string>::const_iterator var = vars.begin(); var != vars.end(); ++var, ++col) {
    varToCol[*var] = col;
    colToVar[col] = *var;
  }
}

inline bool IsFeasable(int status) {
  return ((status != GLP_INFEAS) && (status != GLP_NOFEAS));
}

inline int max(int a, int b) {
  return (a > b) ? a : b;
}

vector<int> ElasticFilter(const LinearProblem &lp) {
  LOG << "running elastic filter" << endl;
  set<int> S; // suspect rows
  LinearProblem tmp(lp);

  int realCols = glp_get_num_cols(tmp.lp_);
  for (int i = realCols; i > 0; --i) { // set old coefs to zero
    glp_set_obj_coef(tmp.lp_, i,  0);
  }

  int newCols = glp_get_num_rows(tmp.lp_);
  glp_add_cols(tmp.lp_, newCols);
  for (int i = 1; i <= newCols; ++i) {
    int indices[10];
    double values[10];
    int nonZeros = glp_get_mat_row(tmp.lp_, i, indices, values);

    nonZeros++;
    indices[nonZeros] = realCols + i;
    values[nonZeros] = 1.0;

    glp_set_mat_row(tmp.lp_, i, nonZeros, indices, values);
    glp_set_obj_coef(tmp.lp_, realCols + i,  1);
    glp_set_col_bnds(tmp.lp_, realCols + i, GLP_UP, 0.0, 0.0);
  }

  glp_std_basis(tmp.lp_);
  int status = tmp.Solve();
  while ((status != GLP_INFEAS) && (status != GLP_NOFEAS)) {

    for (int i = 1; i <= newCols; ++i) {
      if (glp_get_col_prim(tmp.lp_, realCols + i) < 0) {
        S.insert(i);
        glp_set_col_bnds(tmp.lp_, realCols + i, GLP_FX, 0.0, 0.0);
      }
    }
    status = tmp.Solve();
  }

  vector<int> result;
  for (set<int>::iterator it = S.begin(); it != S.end(); ++it) {
    result.push_back(*it);
  }
  return result;
}

/**
  Remove a minimal set of constraints that makes the lp inFeasable.

  Each of the variables in the removed rows will become "unbound" (e.g. - ...!max >= MAX_INT  or
  ...!min <= MIN_INT).

  The removed set is *A* minimal in the sense that these lines alone form an inFeasable problem, and
  each subset of them does not. It is not nessecerily *THE* minimal set in the sense that there is
  no such set which is smaller in size.
*/
inline void removeInFeasable(LinearProblem &lp, map<int, string>& colToVar) {
  LOG << "No Feasable solution, running taint analysis - " << endl;

  vector<int> rows = ElasticFilter(lp);
  int numRows = rows.size();

  LinearProblem tmp(lp);

  map<int, bool> irrelevant;
  for (int i = 0; i < numRows; ++i) {
    irrelevant[rows[i]] = true;
  }
  for (int i = 1; i <= lp.realRows_; ++i) {
    if (!irrelevant[i]) {
      tmp.RemoveRow(i, colToVar);
    }
  }

  vector<int> removedRows;

  for (int i = 0; i < numRows; /* empty */) {
    int half = i + max((numRows - i) / 2, 1);

    bool oneByOne = false;

    while ((half > i) || oneByOne) {
      LinearProblem tmp2(tmp);
      for (int j = i; j < half; ++j) {
        tmp.RemoveRow(rows[j], colToVar);
      }

      int status = tmp.Solve();

      if (IsFeasable(status)) {
        tmp = tmp2; // get the row(s) back to tmp
        if (half == i + 1) {
          lp.RemoveRow(rows[i], colToVar);
          removedRows.push_back(rows[i]);
          LOG << " removing row " << rows[i] << endl;
          i = half;
          oneByOne = false;
        } else { // proccess rows one by one
          oneByOne = true;
          half = i + 1;
        }
      } else {
        i = half;
        if (oneByOne) {
          half++;
        }
      }
    }

    if (removedRows.size() >= 3) {
      LinearProblem tmp2(tmp);
      for (int j = i; j < numRows; ++j) {
        tmp2.RemoveRow(rows[j], colToVar);
      }
      int status = tmp2.Solve();
      if (!IsFeasable(status)) {
        i = numRows + 1;
      }
    }
  }

  int ind[2], removed = removedRows.size();
  for (int i = 0; i < removed; ++i) {
    ind[1] = removedRows[i] - i;
    glp_del_rows(lp.lp_, 1, ind);
  }
  glp_std_basis(lp.lp_);
  lp.realRows_ -= removed;
}

vector<Buffer> ConstraintProblem::Solve() const {
  return Solve(constraints, buffers);
}

inline void setBufferCoef(LinearProblem &lp, const Buffer &b, double base, map<string, int> varToCol) {
  glp_set_obj_coef(lp.lp_, varToCol[b.NameExpression(VarLiteral::MIN, VarLiteral::USED )],  base);
  glp_set_obj_coef(lp.lp_, varToCol[b.NameExpression(VarLiteral::MAX, VarLiteral::USED )], -base);
  glp_set_obj_coef(lp.lp_, varToCol[b.NameExpression(VarLiteral::MIN, VarLiteral::ALLOC)],  base);
  glp_set_obj_coef(lp.lp_, varToCol[b.NameExpression(VarLiteral::MAX, VarLiteral::ALLOC)], -base);
}

vector<Buffer> ConstraintProblem::Solve(
    const vector<Constraint> &inputConstraints, const set<Buffer> &inputBuffers) const {
  vector<Buffer> unsafeBuffers;
  if (inputBuffers.empty()) {
    LOG << "No buffers" << endl;
    return unsafeBuffers;
  }
  if (inputConstraints.empty()) {
    LOG << "No constraints" << endl;
    return unsafeBuffers;
  }

  set<string> vars = CollectVars();
  map<string, int> varToCol;
  map<int, string> colToVar;
  MapVarToCol(vars, varToCol, colToVar);

  LinearProblem lp;
  glp_set_obj_dir(lp.lp_, GLP_MAX);
  glp_add_cols(lp.lp_, vars.size());
  glp_add_rows(lp.lp_, inputConstraints.size());
  lp.realRows_ = inputConstraints.size();
  {
    // Fill matrix
    int row = 1;
    for (vector<Constraint>::const_iterator constraint = inputConstraints.begin();
         constraint != inputConstraints.end();
         ++constraint, ++row) {
      constraint->AddToLPP(lp.lp_, row, varToCol);
    }
  }

  for (size_t i = 1; i <= vars.size(); ++i) {
    glp_set_col_bnds(lp.lp_, i, GLP_FR, 0.0, 0.0);
  }

  for (set<Buffer>::const_iterator b = inputBuffers.begin(); b != inputBuffers.end(); ++b) {
    // Set objective coeficients
    setBufferCoef(lp, *b, 1.0, varToCol);
  }

  glp_smcp params;
  glp_init_smcp(&params);
  glp_term_hook(&printToLog, NULL);
  if (outputGlpk) {
    params.msg_lev = GLP_MSG_ALL;
  } else {
    params.msg_lev = GLP_MSG_ERR;
  }
  lp.SetParams(params);

  int status = lp.Solve();
  while (status != GLP_OPT) {
    while (status == GLP_UNBND) {
      for (set<Buffer>::const_iterator b = inputBuffers.begin(); b != inputBuffers.end(); ++b) {
        setBufferCoef(lp, *b, 0.0, varToCol);
      }

      for (set<Buffer>::const_iterator b = inputBuffers.begin(); b != inputBuffers.end(); ++b) {
        setBufferCoef(lp, *b, 1.0, varToCol);
        status = lp.Solve();
        if (status == GLP_UNBND) {
          setBufferCoef(lp, *b, 0.0, varToCol);
        }
      }
      status = lp.Solve();
    }
    while ((status == GLP_INFEAS) || (status == GLP_NOFEAS)) {
      removeInFeasable(lp, colToVar);
      status = lp.Solve();
    }
  }

  for (set<Buffer>::const_iterator buffer = inputBuffers.begin();
      buffer != inputBuffers.end();
      ++buffer) {
    // Print result
    LOG << buffer->NameExpression(VarLiteral::MIN, VarLiteral::USED) <<
        "\t = " << glp_get_col_prim(
        lp.lp_, varToCol[buffer->NameExpression(VarLiteral::MIN, VarLiteral::USED)]) << endl;
    LOG << buffer->NameExpression(VarLiteral::MAX, VarLiteral::USED) <<
        "\t = " << glp_get_col_prim(
        lp.lp_, varToCol[buffer->NameExpression(VarLiteral::MAX, VarLiteral::USED)]) << endl;
    LOG << buffer->NameExpression(VarLiteral::MIN, VarLiteral::ALLOC) <<
        "\t = " << glp_get_col_prim(
        lp.lp_, varToCol[buffer->NameExpression(VarLiteral::MIN, VarLiteral::ALLOC)]) << endl;
    LOG << buffer->NameExpression(VarLiteral::MAX, VarLiteral::ALLOC) <<
        "\t = " << glp_get_col_prim(
        lp.lp_, varToCol[buffer->NameExpression(VarLiteral::MAX, VarLiteral::ALLOC)]) << endl;

    LOG << endl;
    if ((glp_get_col_prim(
         lp.lp_, varToCol[buffer->NameExpression(VarLiteral::MAX, VarLiteral::USED)]) >=
         glp_get_col_prim(
         lp.lp_, varToCol[buffer->NameExpression(VarLiteral::MIN, VarLiteral::ALLOC)])) ||
         (glp_get_col_prim(
         lp.lp_, varToCol[buffer->NameExpression(VarLiteral::MIN, VarLiteral::USED)]) < 0)) {
      unsafeBuffers.push_back(*buffer);
      LOG << endl << "  !! POSSIBLE BUFFER OVERRUN ON " << buffer->getUniqueName() << endl << endl;
    }
  }

  return unsafeBuffers;
}

vector<Constraint> ConstraintProblem::Blame(
    const vector<Constraint> &input, const set<Buffer> &buffer) const {
  vector<Constraint> result(input);
  // super naive algorithm
  for (size_t i = 0; i < result.size(); ) {
    Constraint tmp = result[i];
    result[i] = result.back();
    result.pop_back();
    if (Solve(result, buffer).empty()) {
      result.push_back(tmp);
      tmp = result[i];
      result[i] = result.back();
      result.back() = tmp;
      ++i;
    }
  }
  return result;
}

map<Buffer, vector<Constraint> > ConstraintProblem::SolveAndBlame() const {
  vector<Buffer> unsafe = Solve();
  map<Buffer, vector<Constraint> > result;
  for (size_t i = 0; i < unsafe.size(); ++i) {
    set<Buffer> buf;
    buf.insert(unsafe[i]);
    result[unsafe[i]] = Blame(constraints, buf);
  }
  return result;
}
} // namespace boa
