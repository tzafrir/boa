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

inline void setBufferCoef(LinearProblem &p, const Buffer &b, double base) {
  glp_set_obj_coef(p.lp_, p.varToCol_[b.NameExpression(VarLiteral::MIN, VarLiteral::USED )],  base);
  glp_set_obj_coef(p.lp_, p.varToCol_[b.NameExpression(VarLiteral::MAX, VarLiteral::USED )], -base);
  glp_set_obj_coef(p.lp_, p.varToCol_[b.NameExpression(VarLiteral::MIN, VarLiteral::ALLOC)],  base);
  glp_set_obj_coef(p.lp_, p.varToCol_[b.NameExpression(VarLiteral::MAX, VarLiteral::ALLOC)], -base);
}

LinearProblem ConstraintProblem::MakeFeasableProblem() const {
  set<string> vars = CollectVars();
  LinearProblem lp;
  MapVarToCol(vars, lp.varToCol_, lp.colToVar_);

  glp_set_obj_dir(lp.lp_, GLP_MAX);
  glp_add_cols(lp.lp_, vars.size());
  glp_add_rows(lp.lp_, constraints_.size());
  {
    // Fill matrix
    int row = 1;
    for (vector<Constraint>::const_iterator c = constraints_.begin(); c != constraints_.end(); ++c) {
      if (c->GetType() == Constraint::STRUCTURAL) {
        c->AddToLPP(lp.lp_, row, lp.varToCol_);
        ++row;
      }     
    }
    lp.structuralRows_ = row - 1;
    for (vector<Constraint>::const_iterator c = constraints_.begin(); c != constraints_.end(); ++c) {
      if (c->GetType() == Constraint::ALIASING) {
        c->AddToLPP(lp.lp_, row, lp.varToCol_);
        ++row;
      }     
    }    
    lp.aliasingRows_ = (row - 1) - lp.structuralRows_;
    for (vector<Constraint>::const_iterator c = constraints_.begin(); c != constraints_.end(); ++c) {
      if (c->GetType() == Constraint::NORMAL) {
        c->AddToLPP(lp.lp_, row, lp.varToCol_);
        ++row;
      }     
    }    
    lp.realRows_ = row - lp.structuralRows_ - 1;
  }

  for (size_t i = 1; i <= vars.size(); ++i) {
    glp_set_col_bnds(lp.lp_, i, GLP_FR, 0.0, 0.0);
  }

  for (set<Buffer>::const_iterator b = buffers_.begin(); b != buffers_.end(); ++b) {
    // Set objective coeficients
    setBufferCoef(lp, *b, 1.0);
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
        setBufferCoef(lp, *b, 0.0);
      }

      for (set<Buffer>::const_iterator b = buffers_.begin(); b != buffers_.end(); ++b) {
        setBufferCoef(lp, *b, 1.0);
        status = lp.Solve();
        if (status == GLP_UNBND) {
          setBufferCoef(lp, *b, 0.0);
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
        lp.lp_, lp.varToCol_[buffer->NameExpression(VarLiteral::MIN, VarLiteral::USED)]) << endl;
    LOG << " Used  max\t = " << glp_get_col_prim(
        lp.lp_, lp.varToCol_[buffer->NameExpression(VarLiteral::MAX, VarLiteral::USED)]) << endl;
    LOG << " Alloc min\t = " << glp_get_col_prim(
        lp.lp_, lp.varToCol_[buffer->NameExpression(VarLiteral::MIN, VarLiteral::ALLOC)]) << endl;
    LOG << " Alloc max\t = " << glp_get_col_prim(
        lp.lp_, lp.varToCol_[buffer->NameExpression(VarLiteral::MAX, VarLiteral::ALLOC)]) << endl;

    LOG << endl;
    if ((glp_get_col_prim(
         lp.lp_, lp.varToCol_[buffer->NameExpression(VarLiteral::MAX, VarLiteral::USED)]) >=
         glp_get_col_prim(
         lp.lp_, lp.varToCol_[buffer->NameExpression(VarLiteral::MIN, VarLiteral::ALLOC)])) ||
         (glp_get_col_prim(
         lp.lp_, lp.varToCol_[buffer->NameExpression(VarLiteral::MIN, VarLiteral::USED)]) < 0)) {
      unsafeBuffers.push_back(*buffer);
    }
  }

  return unsafeBuffers;
}

vector<string> ConstraintProblem::Blame(LinearProblem lp, Buffer &buffer) const {
  vector<string> result;

  double minAlloc = glp_get_col_prim(lp.lp_, 
                       lp.varToCol_[buffer.NameExpression(VarLiteral::MIN, VarLiteral::ALLOC)]) - 1;
  glp_set_col_bnds(lp.lp_, lp.varToCol_[buffer.NameExpression(VarLiteral::MAX, VarLiteral::USED)],
                   GLP_UP, minAlloc, minAlloc);
  glp_set_col_bnds(lp.lp_, lp.varToCol_[buffer.NameExpression(VarLiteral::MIN, VarLiteral::USED)],
                   GLP_LO, 0.0, 0.0);
   
  lp.Solve();

  // blame the interesting rows first
  lp.structuralRows_ += lp.aliasingRows_;
  lp.realRows_ = glp_get_num_rows(lp.lp_) - lp.structuralRows_;
  vector<int> rows = lp.ElasticFilter();
  for (size_t i = 0; i < rows.size(); ++i) {
    char const *row = glp_get_row_name(lp.lp_, rows[i]);
    if (row) {
      result.push_back(row);
    }
  }
  
  // then aliasing rows too
  lp.structuralRows_ -= lp.aliasingRows_;
  lp.realRows_ = lp.aliasingRows_;
  rows = lp.ElasticFilter();
  for (size_t i = 0; i < rows.size(); ++i) {
    char const *row = glp_get_row_name(lp.lp_, rows[i]);
    if (row) {
      result.push_back(row);
    }
  }
  return result;
}

map<Buffer, vector<string> > ConstraintProblem::SolveAndBlame() const {
  LinearProblem lp = MakeFeasableProblem();
  vector<Buffer> unsafe = SolveProblem(lp);
  map<Buffer, vector<string> > result;
  for (size_t i = 0; i < unsafe.size(); ++i) {
    result[unsafe[i]] = Blame(lp, unsafe[i]);
  }
  return result;
}
} // namespace boa
