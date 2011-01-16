#include "ConstraintProblem.h"

#include <iostream>
#include <glpk.h>
#include "log.h"

using std::endl;

namespace boa {

set<string> ConstraintProblem::CollectVars() const {
  set<string> vars;
  for (set<Buffer>::const_iterator buffer = buffers.begin(); buffer != buffers.end(); ++buffer) {
    vars.insert(buffer->NameExpression(MIN, USED));
    vars.insert(buffer->NameExpression(MAX, USED));
    vars.insert(buffer->NameExpression(MIN, ALLOC));
    vars.insert(buffer->NameExpression(MAX, ALLOC));
    vars.insert(buffer->NameExpression(MIN, LEN));
    vars.insert(buffer->NameExpression(MAX, LEN));
  }

  for (vector<Constraint>::const_iterator constraint = constraints.begin();
       constraint != constraints.end();
       ++constraint) {
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
  map<string, int> varToCol = MapVarToCol(vars);

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

  for (set<Buffer>::const_iterator buffer = inputBuffers.begin();
       buffer != inputBuffers.end();
       ++buffer) {
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

  for (set<Buffer>::const_iterator buffer = inputBuffers.begin();
      buffer != inputBuffers.end();
      ++buffer) {
    // Print result
    LOG << buffer->NameExpression(MIN, USED) <<
        "\t = " << glp_get_col_prim(lp, varToCol[buffer->NameExpression(MIN, USED)]) << endl;
    LOG << buffer->NameExpression(MAX, USED) <<
        "\t = " << glp_get_col_prim(lp, varToCol[buffer->NameExpression(MAX, USED)]) << endl;
    LOG << buffer->NameExpression(MIN, ALLOC) <<
        "\t = " << glp_get_col_prim(lp, varToCol[buffer->NameExpression(MIN, ALLOC)]) << endl;
    LOG << buffer->NameExpression(MAX, ALLOC) <<
        "\t = " << glp_get_col_prim(lp, varToCol[buffer->NameExpression(MAX, ALLOC)]) << endl;
    LOG << buffer->NameExpression(MIN, LEN) <<
        "\t = " << glp_get_col_prim(lp, varToCol[buffer->NameExpression(MIN, LEN)]) << endl;
    LOG << buffer->NameExpression(MAX, LEN) <<
        "\t = " << glp_get_col_prim(lp, varToCol[buffer->NameExpression(MAX, LEN)]) << endl;

    LOG << endl;
    if ((glp_get_col_prim(lp, varToCol[buffer->NameExpression(MAX, USED)]) >=
         glp_get_col_prim(lp, varToCol[buffer->NameExpression(MIN, ALLOC)])) ||
        (glp_get_col_prim(lp, varToCol[buffer->NameExpression(MIN, USED)]) < 0)) {
      unsafeBuffers.push_back(*buffer);
      LOG << endl << "  !! POSSIBLE BUFFER OVERRUN ON " << buffer->getUniqueName() << endl << endl;
    }
  }

  glp_delete_prob(lp);
  return unsafeBuffers;
}

vector<Constraint> ConstraintProblem::Blame(const vector<Constraint> &input, const set<Buffer> &buffer) const {
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
