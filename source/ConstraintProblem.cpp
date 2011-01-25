#include "ConstraintProblem.h"

#include <iostream>
#include <glpk.h>
#include "log.h"

using std::endl;

namespace boa {

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

inline bool IsFeasble(int status) {
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
inline RowData removeRowFromLP(glp_prob* lp, int row, map<int, string>& colToVar) {
  RowData res;

  res.nonZeros = glp_get_mat_row(lp, row, res.indices, res.values);
  res.bound = glp_get_row_ub(lp, row);

  glp_set_row_bnds(lp, row, GLP_FR, 0.0, 0.0);        
  
  int ind[2];
  double val[2];
  for (int i = 1; i <= res.nonZeros; ++i) {
    ind[1] = res.indices[i];
    val[1] = (isMax(colToVar[res.indices[i]]) ? -1 : 1);
    int r = glp_add_rows(lp, 1);    
    glp_set_row_bnds(lp, r, GLP_UP, 0.0, std::numeric_limits<int>::min());
    glp_set_mat_row(lp, r, 1, ind, val);
  }
  return res;  
}

/**
  Return a removed row back to the LP and remove the related "unbound constraints"
  
  This function assumes that it is called right after "row" was removed using "removeRowFromLP",
  if there wasany other manipulation on the matrix rows between the two calls - the result may be 
  unexpected
*/
inline void returnRowToLP(glp_prob* lp, int row, RowData data) {
  glp_set_row_bnds(lp, row, GLP_UP, 0.0, data.bound);      

  int ind[RowData::MAX_VARS]; // rows to be deleted from lp
  ind[0] = glp_get_num_rows(lp) - data.nonZeros; // last row to stay
  for (int i = 1; i <= data.nonZeros; ++i) {
    ind[i] = ind[0] + i;
  }
  glp_del_rows(lp, data.nonZeros, ind);
}

/** 
  Remove a minimal set of constraints that makes the lp infeasble.
  
  Each of the variables in the removed rows will become "unbound" (e.g. - ...!max >= MAX_INT  or 
  ...!min <= MIN_INT). 
  
  The removed set is *A* minimal in the sense that these lines alone form an infeasble problem, and
  each subset of them does not. It is not nessecerily *THE* minimal set in the sense that there is 
  no such set which is smaller in size.
*/
inline void removeInfeasble(glp_prob* lp, const glp_smcp &params, map<int, string>& colToVar) {
  LOG << "No feasble solution, running taint analysis - " << endl;
  glp_prob *tmp;
  tmp = glp_create_prob();
  glp_copy_prob(tmp, lp, GLP_OFF);

  for (int i = 1, rows = glp_get_num_rows(tmp); i <= rows; ++i) {
    RowData data = removeRowFromLP(tmp, i, colToVar);
    glp_simplex(tmp, &params);
    if (IsFeasble(glp_get_status(tmp))) {
      removeRowFromLP(lp, i, colToVar);
      returnRowToLP(tmp, i, data);
      LOG << " removing row " << i << endl;
    }
  }

  glp_delete_prob(tmp);
}

vector<Buffer> ConstraintProblem::Solve() const {
  return Solve(constraints, buffers);
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

  glp_prob *lp;
  lp = glp_create_prob();
  glp_set_obj_dir(lp, GLP_MAX);
  glp_add_rows(lp, inputConstraints.size());
  glp_add_cols(lp, vars.size());
  {
    // Fill matrix
    int row = 1;
    for (vector<Constraint>::const_iterator constraint = inputConstraints.begin();
         constraint != inputConstraints.end();
         ++constraint, ++row) {
      constraint->AddToLPP(lp, row, varToCol);
    }
  }

  for (size_t i = 1; i <= vars.size(); ++i) {
    glp_set_col_bnds(lp, i, GLP_FR, 0.0, 0.0);
  }

  for (set<Buffer>::const_iterator b = inputBuffers.begin(); b != inputBuffers.end(); ++b) {
    // Set objective coeficients
    glp_set_obj_coef(lp, varToCol[b->NameExpression(VarLiteral::MIN, VarLiteral::USED )],  1.0);
    glp_set_obj_coef(lp, varToCol[b->NameExpression(VarLiteral::MAX, VarLiteral::USED )], -1.0);
    glp_set_obj_coef(lp, varToCol[b->NameExpression(VarLiteral::MIN, VarLiteral::ALLOC)],  1.0);
    glp_set_obj_coef(lp, varToCol[b->NameExpression(VarLiteral::MAX, VarLiteral::ALLOC)], -1.0);
  }

  glp_smcp params;
  glp_init_smcp(&params);
  params.msg_lev = GLP_MSG_OFF;
  glp_simplex(lp, &params);
  
  while (!IsFeasble(glp_get_status(lp))) {
    removeInfeasble(lp, params, colToVar);
    glp_simplex(lp, &params);
  }
  
  // TODO - what if no solution can be found? (glp_get_status(lp) != GLP_OPT)

  for (set<Buffer>::const_iterator buffer = inputBuffers.begin();
      buffer != inputBuffers.end();
      ++buffer) {
    // Print result
    LOG << buffer->NameExpression(VarLiteral::MIN, VarLiteral::USED) <<
        "\t = " << glp_get_col_prim(
        lp, varToCol[buffer->NameExpression(VarLiteral::MIN, VarLiteral::USED)]) << endl;
    LOG << buffer->NameExpression(VarLiteral::MAX, VarLiteral::USED) <<
        "\t = " << glp_get_col_prim(
        lp, varToCol[buffer->NameExpression(VarLiteral::MAX, VarLiteral::USED)]) << endl;
    LOG << buffer->NameExpression(VarLiteral::MIN, VarLiteral::ALLOC) <<
        "\t = " << glp_get_col_prim(
        lp, varToCol[buffer->NameExpression(VarLiteral::MIN, VarLiteral::ALLOC)]) << endl;
    LOG << buffer->NameExpression(VarLiteral::MAX, VarLiteral::ALLOC) <<
        "\t = " << glp_get_col_prim(
        lp, varToCol[buffer->NameExpression(VarLiteral::MAX, VarLiteral::ALLOC)]) << endl;

    LOG << endl;
    if ((glp_get_col_prim(
         lp, varToCol[buffer->NameExpression(VarLiteral::MAX, VarLiteral::USED)]) >=
         glp_get_col_prim(
         lp, varToCol[buffer->NameExpression(VarLiteral::MIN, VarLiteral::ALLOC)])) ||
         (glp_get_col_prim(
         lp, varToCol[buffer->NameExpression(VarLiteral::MIN, VarLiteral::USED)]) < 0)) {
      unsafeBuffers.push_back(*buffer);
      LOG << endl << "  !! POSSIBLE BUFFER OVERRUN ON " << buffer->getUniqueName() << endl << endl;
    }
  }

  glp_delete_prob(lp);
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
