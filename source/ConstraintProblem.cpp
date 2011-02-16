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
  for (set<Buffer>::const_iterator buffer = buffers_.begin(); buffer != buffers_.end(); ++buffer) {
    vars.insert(buffer->NameExpression(VarLiteral::MIN, VarLiteral::USED));
    vars.insert(buffer->NameExpression(VarLiteral::MAX, VarLiteral::USED));
    vars.insert(buffer->NameExpression(VarLiteral::MIN, VarLiteral::ALLOC));
    vars.insert(buffer->NameExpression(VarLiteral::MAX, VarLiteral::ALLOC));
  }

  for (vector<Constraint>::const_iterator constraint = constraints_.begin();
       constraint != constraints_.end();
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
  
 vector<Buffer> emptySet;
 if (buffers_.empty()) {
    LOG << "No buffers" << endl;
    return emptySet;
  }
  if (constraints_.empty()) {
    LOG << "No constraints" << endl;
    return emptySet;
  }

  return SolveProblem(MakeFeasableProblem());
}

inline void setBufferCoef(LinearProblem &lp,const Buffer &b, double base,
                          map<string, int> varToCol) {
  glp_set_obj_coef(lp.lp_, varToCol[b.NameExpression(VarLiteral::MIN, VarLiteral::USED )],  base);
  glp_set_obj_coef(lp.lp_, varToCol[b.NameExpression(VarLiteral::MAX, VarLiteral::USED )], -base);
  glp_set_obj_coef(lp.lp_, varToCol[b.NameExpression(VarLiteral::MIN, VarLiteral::ALLOC)],  base);
  glp_set_obj_coef(lp.lp_, varToCol[b.NameExpression(VarLiteral::MAX, VarLiteral::ALLOC)], -base);
}

LinearProblem ConstraintProblem::MakeFeasableProblem() const {
  set<string> vars = CollectVars();
  LinearProblem lp;
  MapVarToCol(vars, lp.varToCol, lp.colToVar);

  glp_set_obj_dir(lp.lp_, GLP_MAX);
  glp_add_cols(lp.lp_, vars.size());
  glp_add_rows(lp.lp_, constraints_.size());
  lp.realRows_ = constraints_.size();
  {
    // Fill matrix
    int row = 1;
    for (vector<Constraint>::const_iterator constraint = constraints_.begin();
         constraint != constraints_.end();
         ++constraint, ++row) {
      constraint->AddToLPP(lp.lp_, row, lp.varToCol);
    }
  }

  for (size_t i = 1; i <= vars.size(); ++i) {
    glp_set_col_bnds(lp.lp_, i, GLP_FR, 0.0, 0.0);
  }

  for (set<Buffer>::const_iterator b = buffers_.begin(); b != buffers_.end(); ++b) {
    // Set objective coeficients
    setBufferCoef(lp, *b, 1.0, lp.varToCol);
  }

  glp_smcp params;
  glp_init_smcp(&params);
  glp_term_hook(&printToLog, NULL);
  if (outputGlpk_) {
    params.msg_lev = GLP_MSG_ALL;
  } else {
    params.msg_lev = GLP_MSG_ERR;
  }
  lp.SetParams(params);

  int status = lp.Solve();
  while (status != GLP_OPT) {
    while (status == GLP_UNBND) {
      for (set<Buffer>::const_iterator b = buffers_.begin(); b != buffers_.end(); ++b) {
        setBufferCoef(lp, *b, 0.0, lp.varToCol);
      }

      for (set<Buffer>::const_iterator b = buffers_.begin(); b != buffers_.end(); ++b) {
        setBufferCoef(lp, *b, 1.0, lp.varToCol);
        status = lp.Solve();
        if (status == GLP_UNBND) {
          setBufferCoef(lp, *b, 0.0, lp.varToCol);
        }
      }
      status = lp.Solve();
    }
    while ((status == GLP_INFEAS) || (status == GLP_NOFEAS)) {
      lp.RemoveInfeasable();
      status = lp.Solve();
    }
  }

  return lp;
}

vector<Buffer> ConstraintProblem::SolveProblem(LinearProblem lp) const {  
  vector<Buffer> unsafeBuffers;
  
  for (set<Buffer>::const_iterator buffer = buffers_.begin(); buffer != buffers_.end(); ++buffer) {
    // Print result
    LOG << buffer->getReadableName() << " " << buffer->getSourceLocation() << endl;
    LOG << " Used  min\t = " << glp_get_col_prim(
        lp.lp_, lp.varToCol[buffer->NameExpression(VarLiteral::MIN, VarLiteral::USED)]) << endl;
    LOG << " Used  max\t = " << glp_get_col_prim(
        lp.lp_, lp.varToCol[buffer->NameExpression(VarLiteral::MAX, VarLiteral::USED)]) << endl;
    LOG << " Alloc min\t = " << glp_get_col_prim(
        lp.lp_, lp.varToCol[buffer->NameExpression(VarLiteral::MIN, VarLiteral::ALLOC)]) << endl;
    LOG << " Alloc max\t = " << glp_get_col_prim(
        lp.lp_, lp.varToCol[buffer->NameExpression(VarLiteral::MAX, VarLiteral::ALLOC)]) << endl;

    LOG << endl;
    if ((glp_get_col_prim(
         lp.lp_, lp.varToCol[buffer->NameExpression(VarLiteral::MAX, VarLiteral::USED)]) >=
         glp_get_col_prim(
         lp.lp_, lp.varToCol[buffer->NameExpression(VarLiteral::MIN, VarLiteral::ALLOC)])) ||
         (glp_get_col_prim(
         lp.lp_, lp.varToCol[buffer->NameExpression(VarLiteral::MIN, VarLiteral::USED)]) < 0)) {
      unsafeBuffers.push_back(*buffer);
    }
  }

  return unsafeBuffers;
}

vector<Constraint> ConstraintProblem::Blame(
    const vector<Constraint> &input, const set<Buffer> &buffer) const {
  vector<Constraint> result(input);

  // TODO
  return result;
}

map<Buffer, vector<Constraint> > ConstraintProblem::SolveAndBlame() const {
  vector<Buffer> unsafe = Solve();
  map<Buffer, vector<Constraint> > result;
  for (size_t i = 0; i < unsafe.size(); ++i) {
    set<Buffer> buf;
    buf.insert(unsafe[i]);
    result[unsafe[i]] = Blame(constraints_, buf);
  }
  return result;
}
} // namespace boa
