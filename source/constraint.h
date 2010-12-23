#ifndef __BOA_CONSTRAINT_H
#define __BOA_CONSTRAINT_H /* */

#include <string>
#include <set>
#include <list>
#include <map>
#include <glpk.h>
#include "buffer.h"

using std::string;
using std::set;
using std::list;
using std::map;
using boa::Buffer;

/**
  Model a single constraint.

  A constraint is a simple linear inequality of the form BIG >= SMALL
  Use AddBigExpression, AddBigConst in order to add an expression or a constant
  to the BIG side of the inequality, or the equivalent functions for the SMALL
  side.

  For internal use, the constraint is stored in the form of -

  C >= aX + bY ...

  Where C is a constant (referred as "left"), a small letter (a, b...) is an
  integer value ("num") and a capital letter (X, Y...) is a string name of variable.
*/
class Constraint {
 private:
  const static int MAX_SIZE = 100;
  int left_;
  map<string, int> expressions_;

  void AddExpression(int num, string var) {
    expressions_[var] += num;
  }

  void AddLeft(int left) {
    left_ += left;
  }

  // TODO(tzafrir): Disallow copying and assignment.

 public:
  class Expressions {
    int val_;
    map<string, int> vars_;
   public:
    Expressions() : val_(0) {}
    void AddExpression(const string& var, int num = 1) {vars_[var] += num;}
    void addConst(int num) {val_ += num;}
  };

  Constraint() : left_(0) {}
  
  void AddBig(const Expressions& expr) {
    for (map<string, int>::const_iterator it = expr.vars_.begin(); it != expr.vars_.end(); ++it) {
      AddBigExpression(it->first, it->second);
    }
    AddBigConst(expr.val_);
  }

  void AddBigExpression(const string& var, int num = 1) {
    AddExpression(-num, var);
  }

  void AddBigConst(int num) {
    AddLeft(num);
  }

  void AddSmall(const Expressions& expr) {
    for (map<string, int>::const_iterator it = expr.vars_.begin(); it != expr.vars_.end(); ++it) {
      AddSmallExpression(it->first, it->second);
    }
    AddSmallConst(expr.val_);
  }

  void AddSmallExpression(const string& var, int num = 1) {
    AddExpression(num, var);
  }

  void AddSmallConst(int num) {
    AddLeft(-num);
  }

  void Clear() {
   left_ = 0;
   expressions_.clear();
  }

  void GetVars(set<string>& vars) {
    for (map<string, int>::iterator it = expressions_.begin(); it != expressions_.end(); ++it) {
      vars.insert(it->first);
    }
  }

  void AddToLPP(glp_prob *lp, int row, map<string, int>& colNumbers) {
    int indices[MAX_SIZE + 1];
    double values[MAX_SIZE + 1];

    // TODO if size > MAX_SIZE...

    int count = 1;
    for (map<string, int>::iterator it = expressions_.begin(); it != expressions_.end(); ++it, ++count) {
      indices[count] = colNumbers[it->first];
      values[count] = it->second;
    }
    glp_set_row_bnds(lp, row, GLP_UP, 0.0, left_);
    glp_set_mat_row(lp, row, expressions_.size(), indices, values);
  }

};

class ConstraintProblem {
 private:
  list<Constraint> constraints;
  list<Buffer> buffers;

  set<string> CollectVars();
 public:
  void AddBuffer(const Buffer& buffer) {
    buffers.push_back(buffer);
  }

  void AddConstraint(const Constraint& c) {
    constraints.push_back(c);
  }

  void Clear() {
    buffers.clear();
    constraints.clear();
  }

  /**
    Solve the constriant problem defined by the constraints.

    Return a set of buffers in which buffer overrun may occur.
  */
  list<Buffer> Solve();
};

#endif /* __BOA_CONSTRAINT_H */

