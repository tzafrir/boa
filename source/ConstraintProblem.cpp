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

vector<Buffer> ConstraintProblem::Solve() const {
  LOG << "Solving constraint problem (" << constraints_.size() << " constraints)" << endl;
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
      lp.RemoveInfeasable(colToVar);
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
