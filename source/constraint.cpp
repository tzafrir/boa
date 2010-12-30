#include "constraint.h"
#include <iostream>
#include <glpk.h>
#include "log.h"

using std::endl;

namespace boa {

set<string> ConstraintProblem::CollectVars() const {
  set<string> vars;
  for (vector<Buffer>::const_iterator buffer = buffers.begin(); buffer != buffers.end(); ++buffer) {
    vars.insert(buffer->NameExpression(MIN, USED));
    vars.insert(buffer->NameExpression(MAX, USED));
    vars.insert(buffer->NameExpression(MIN, ALLOC));
    vars.insert(buffer->NameExpression(MAX, ALLOC));
  }

  for (vector<Constraint>::const_iterator constraint = constraints.begin(); constraint != constraints.end(); ++constraint) {
    constraint->GetVars(vars);
  }
  return vars;
}

inline static map<string, int> MapVarToCol(const set<string>& vars) {
  map<string, int> varToCol;
  int col = 1;
  for (set<string>::const_iterator var = vars.begin(); var != vars.end(); ++var, ++col) {
    varToCol[*var] = col;
  }
  return varToCol;
}

vector<Buffer> ConstraintProblem::Solve() const {
  return Solve(constraints, buffers);
}

vector<Buffer> ConstraintProblem::Solve(const vector<Constraint> &inputConstraints, const vector<Buffer> &inputBuffers) const {
  vector<Buffer> unsafeBuffers;
  if (inputBuffers.empty()) {
    log::os() << "No buffers" << endl;
    return unsafeBuffers;
  }
  if (inputConstraints.empty()) {
    log::os() << "No constraints" << endl;
    return unsafeBuffers;
  }

  set<string> vars = CollectVars();
  map<string, int> varToCol = MapVarToCol(vars);

  glp_prob *lp;
  lp = glp_create_prob();
  glp_set_obj_dir(lp, GLP_MAX);
  glp_add_rows(lp, inputConstraints.size());
  glp_add_cols(lp, vars.size());
  {
    // Fill matrix
    int row = 1;
    for (vector<Constraint>::const_iterator constraint = inputConstraints.begin(); constraint != inputConstraints.end(); ++constraint, ++row) {
      constraint->AddToLPP(lp, row, varToCol);
    }
  }

  for (vector<Buffer>::const_iterator buffer = inputBuffers.begin(); buffer != inputBuffers.end(); ++buffer) {
    // Set objective coeficients
    glp_set_obj_coef(lp, varToCol[buffer->NameExpression(MIN, USED)], 1.0);
    glp_set_obj_coef(lp, varToCol[buffer->NameExpression(MAX, USED)], -1.0);
    glp_set_obj_coef(lp, varToCol[buffer->NameExpression(MIN, ALLOC)], 1.0);
    glp_set_obj_coef(lp, varToCol[buffer->NameExpression(MAX, ALLOC)], -1.0);
  }

  for (size_t i = 1; i <= vars.size(); ++i) {
    glp_set_col_bnds(lp, i, GLP_FR, 0.0, 0.0);
  }

  glp_smcp params;
  glp_init_smcp(&params);
  params.msg_lev = GLP_MSG_ERR;
  glp_simplex(lp, &params);

  // TODO - what if no solution can be found?

  for (vector<Buffer>::const_iterator buffer = inputBuffers.begin(); buffer != inputBuffers.end(); ++buffer) {
    // Print result
    log::os() << buffer->NameExpression(MIN, USED) << "\t = " << glp_get_col_prim(lp, varToCol[buffer->NameExpression(MIN, USED)]) << endl;
    log::os() << buffer->NameExpression(MAX, USED) << "\t = " << glp_get_col_prim(lp, varToCol[buffer->NameExpression(MAX, USED)]) << endl;
    log::os() << buffer->NameExpression(MIN, ALLOC) << "\t = " << glp_get_col_prim(lp, varToCol[buffer->NameExpression(MIN, ALLOC)]) << endl;
    log::os() << buffer->NameExpression(MAX, ALLOC) << "\t = " << glp_get_col_prim(lp, varToCol[buffer->NameExpression(MAX, ALLOC)]) << endl;

    if (glp_get_col_prim(lp, varToCol[buffer->NameExpression(MAX, USED)]) >= glp_get_col_prim(lp, varToCol[buffer->NameExpression(MIN, ALLOC)])) {
      unsafeBuffers.push_back(*buffer);
      log::os() << endl << "  !! POSSIBLE BUFFER OVERRUN ON " << buffer->getUniqueName() << endl << endl;
    }
  }

  glp_delete_prob(lp);
  return unsafeBuffers;
}

vector<Constraint> ConstraintProblem::Blame(vector<Constraint> &input, const vector<Buffer> &buffer, 
                                            const vector<Constraint> &output) const {
  static const int SLICE = 10;

  if (input.size() > SLICE) { 
    // try first slice 
    vector<Constraint> slice;
    for (size_t i = 0; i < SLICE; ++i) {
      slice.push_back(input[i]);
    }
    if (! Solve(slice, buffer).empty()) {
      return Blame(slice, buffer, NO_CONSTRAINTS);
    }
    // try the rest of the input
    for (size_t i = SLICE; i < input.size(); ++i) {
      //result.push_back(
    }
  }
  
}

map<Buffer, vector<Constraint> > ConstraintProblem::SolveAndBlame() const {
  vector<Buffer> unsafe = Solve();
  map<Buffer, vector<Constraint> > result;
  for (size_t i = 0; i < unsafe.size(); ++i) {
    vector<Buffer> buf;
    buf.push_back(unsafe[i]);
    vector<Constraint> c(constraints);
    result[unsafe[i]] = Blame(c, buf, NO_CONSTRAINTS);
  } 
  return result;
}
} // namespace boa
