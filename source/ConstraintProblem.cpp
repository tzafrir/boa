#include "ConstraintProblem.h"

#include <iostream>
#include <glpk.h>
#include "log.h"

using std::endl;

namespace boa {

struct LinearProblem {
  glp_prob *lp;
  int realRows;
  set<string> removedVars;

  LinearProblem clone() {
    LinearProblem res;
    res.lp = glp_create_prob();
    glp_copy_prob(res.lp, this->lp, GLP_OFF);
    res.realRows = this->realRows;
    res.removedVars = this->removedVars;
    return res;
  }

  void init() {
    lp = glp_create_prob();
    realRows = 0;
    removedVars.clear();
  }

  LinearProblem() : lp(NULL), realRows(0) {}

  void free() {
    if (lp) {
      glp_delete_prob(lp);
    }
  }
};

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

inline bool isMax(string s) {
  return (s.substr(s.length() - 3) == "max");
}


/**
  RowData store all the data of a single lp matrix row, so the row can be removed and returned later
*/
struct RowData {
  // because of llvm command structure, each constraint affect 3(?) variables at the most
  static const int MAX_VARS = 10;

  int indices[MAX_VARS];
  double values[MAX_VARS];
  int nonZeros;
  double bound;
};

/**
  Remove a row from a linear problem matrix, create "unbound constraints" instead

  The new constraints created at the end of the matrix, and all the data regarding the removed row
  retured. Use returnRowToLP with the returned RowData in order to get the lp back to the original
  state (note you must call "returnRowToLP" before removing any other rows from the LP, or else the
  wrong "unbound" constraints will be removed.
*/
inline RowData removeRowFromLP(LinearProblem &lp, int row, map<int, string>& colToVar) {
  RowData res;

  res.nonZeros = glp_get_mat_row(lp.lp, row, res.indices, res.values);
  res.bound = glp_get_row_ub(lp.lp, row);

  glp_set_row_bnds(lp.lp, row, GLP_FR, 0.0, 0.0);

  int ind[2];
  double val[2];
  for (int i = 1; i <= res.nonZeros; ++i) {
    ind[1] = res.indices[i];
    val[1] = (isMax(colToVar[res.indices[i]]) ? -1 : 1);
    int r = glp_add_rows(lp.lp, 1);
    glp_set_row_bnds(lp.lp, r, GLP_UP, 0.0, std::numeric_limits<int>::min());
    glp_set_mat_row(lp.lp, r, 1, ind, val);
  }
  return res;
}

/**
  Return a removed row back to the LP and remove the related "unbound constraints"

  This function assumes that it is called right after "row" was removed using "removeRowFromLP",
  if there was any other manipulation on the matrix rows between the two calls - the result may be
  unexpected
*/
//inline void returnRowToLP(LinearProblem &lp, int row, RowData data) {
//  glp_set_row_bnds(lp, row, GLP_UP, 0.0, data.bound);

//  int ind[RowData::MAX_VARS]; // rows to be deleted from lp
//  ind[0] = glp_get_num_rows(lp) - data.nonZeros; // last row to stay
//  for (int i = 1; i <= data.nonZeros; ++i) {
//    ind[i] = ind[0] + i;
//  }
//  glp_del_rows(lp, data.nonZeros, ind);
//}

inline int max(int a, int b) {
  return (a > b) ? a : b;
}

inline void swap(LinearProblem *a, LinearProblem *b) {
  LinearProblem tmp = *a;
  *a = *b;
  *b = tmp;
}

/**
  Remove a minimal set of constraints that makes the lp inFeasable.

  Each of the variables in the removed rows will become "unbound" (e.g. - ...!max >= MAX_INT  or
  ...!min <= MIN_INT).

  The removed set is *A* minimal in the sense that these lines alone form an inFeasable problem, and
  each subset of them does not. It is not nessecerily *THE* minimal set in the sense that there is
  no such set which is smaller in size.
*/
inline void removeInFeasable(LinearProblem &lp, const glp_smcp &params, map<int, string>& colToVar) {
  LOG << "No Feasable solution, running taint analysis - " << endl;
  LinearProblem tmp = lp.clone();

  int rows = lp.realRows;
  int i = 1;
  vector<int> removedRows;

  while (i <= rows) {
    int half = i + max((rows - i) / 2, 1);
    
    bool oneByOne = false;
    
    while ((half > i) || oneByOne) {
      LinearProblem tmp2 = tmp.clone();
      for (int j = i; j < half; ++j) {
        removeRowFromLP(tmp, j, colToVar);
      }

      glp_simplex(tmp.lp, &params);

      if (IsFeasable(glp_get_status(tmp.lp))) {
        swap(&tmp, &tmp2); // get the row(s) back to tmp
        if (half == i + 1) {
          removeRowFromLP(lp, i, colToVar);
          removedRows.push_back(i);
          LOG << " removing row " << i << endl;
          i = half;
          oneByOne = false;
        } else { // proccess rows one by one
          oneByOne = true;
          half = i + 1;
        }
      } else {
        i = half;
        if (oneByOne) {half++;}
      }
    tmp2.free();
    }
    
    if (removedRows.size() >= 3) {
      LinearProblem tmp2 = tmp.clone();
      for (int j = i; j <= rows; ++j) {
        removeRowFromLP(tmp2, j, colToVar);
      }
      glp_simplex(tmp2.lp, &params); 
      if (!IsFeasable(glp_get_status(tmp2.lp))) {
        i = rows + 1;
      }
      tmp2.free();
    }
  }
  tmp.free();

  int ind[101], removed = -max(-removedRows.size(), -100);
  for (int i = 1; i <= removed; ++i) {
    ind[i] = removedRows[i-1];
  }
  glp_del_rows(lp.lp, removed, ind);
  glp_std_basis(lp.lp);
  lp.realRows -= removed;
}

vector<Buffer> ConstraintProblem::Solve() const {
  return Solve(constraints, buffers);
}

inline void setBufferCoef(LinearProblem &lp, const Buffer &b, double base, map<string, int> varToCol) {
  glp_set_obj_coef(lp.lp, varToCol[b.NameExpression(VarLiteral::MIN, VarLiteral::USED )],  base);
  glp_set_obj_coef(lp.lp, varToCol[b.NameExpression(VarLiteral::MAX, VarLiteral::USED )], -base);
  glp_set_obj_coef(lp.lp, varToCol[b.NameExpression(VarLiteral::MIN, VarLiteral::ALLOC)],  base);
  glp_set_obj_coef(lp.lp, varToCol[b.NameExpression(VarLiteral::MAX, VarLiteral::ALLOC)], -base);
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
  lp.init();
  glp_set_obj_dir(lp.lp, GLP_MAX);
  glp_add_cols(lp.lp, vars.size());
  glp_add_rows(lp.lp, inputConstraints.size());
  lp.realRows = inputConstraints.size();
  {
    // Fill matrix
    int row = 1;
    for (vector<Constraint>::const_iterator constraint = inputConstraints.begin();
         constraint != inputConstraints.end();
         ++constraint, ++row) {
      constraint->AddToLPP(lp.lp, row, varToCol);
    }
  }

  for (size_t i = 1; i <= vars.size(); ++i) {
    glp_set_col_bnds(lp.lp, i, GLP_FR, 0.0, 0.0);
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
  glp_simplex(lp.lp, &params);

  int status = glp_get_status(lp.lp);
  while (status != GLP_OPT) {
    while (status == GLP_UNBND) {
      for (set<Buffer>::const_iterator b = inputBuffers.begin(); b != inputBuffers.end(); ++b) {
        setBufferCoef(lp, *b, 0.0, varToCol);
      }

      for (set<Buffer>::const_iterator b = inputBuffers.begin(); b != inputBuffers.end(); ++b) {
        setBufferCoef(lp, *b, 1.0, varToCol);
        glp_simplex(lp.lp, &params);
        if (glp_get_status(lp.lp) == GLP_UNBND) {
          setBufferCoef(lp, *b, 0.0, varToCol);
        }
      }
      glp_simplex(lp.lp, &params);
      status = glp_get_status(lp.lp);
    }
    while ((status == GLP_INFEAS) || (status == GLP_NOFEAS)) {
      removeInFeasable(lp, params, colToVar);
      glp_simplex(lp.lp, &params);
      status = glp_get_status(lp.lp);
    }
  }

  for (set<Buffer>::const_iterator buffer = inputBuffers.begin();
      buffer != inputBuffers.end();
      ++buffer) {
    // Print result
    LOG << buffer->NameExpression(VarLiteral::MIN, VarLiteral::USED) <<
        "\t = " << glp_get_col_prim(
        lp.lp, varToCol[buffer->NameExpression(VarLiteral::MIN, VarLiteral::USED)]) << endl;
    LOG << buffer->NameExpression(VarLiteral::MAX, VarLiteral::USED) <<
        "\t = " << glp_get_col_prim(
        lp.lp, varToCol[buffer->NameExpression(VarLiteral::MAX, VarLiteral::USED)]) << endl;
    LOG << buffer->NameExpression(VarLiteral::MIN, VarLiteral::ALLOC) <<
        "\t = " << glp_get_col_prim(
        lp.lp, varToCol[buffer->NameExpression(VarLiteral::MIN, VarLiteral::ALLOC)]) << endl;
    LOG << buffer->NameExpression(VarLiteral::MAX, VarLiteral::ALLOC) <<
        "\t = " << glp_get_col_prim(
        lp.lp, varToCol[buffer->NameExpression(VarLiteral::MAX, VarLiteral::ALLOC)]) << endl;

    LOG << endl;
    if ((glp_get_col_prim(
         lp.lp, varToCol[buffer->NameExpression(VarLiteral::MAX, VarLiteral::USED)]) >=
         glp_get_col_prim(
         lp.lp, varToCol[buffer->NameExpression(VarLiteral::MIN, VarLiteral::ALLOC)])) ||
         (glp_get_col_prim(
         lp.lp, varToCol[buffer->NameExpression(VarLiteral::MIN, VarLiteral::USED)]) < 0)) {
      unsafeBuffers.push_back(*buffer);
      LOG << endl << "  !! POSSIBLE BUFFER OVERRUN ON " << buffer->getUniqueName() << endl << endl;
    }
  }

  lp.free();

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
