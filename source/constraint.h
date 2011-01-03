#ifndef __BOA_CONSTRAINT_H
#define __BOA_CONSTRAINT_H /* */

#include <string>
#include <set>
#include <vector>
#include <map>
#include <glpk.h>
#include "buffer.h"

using std::string;
using std::set;
using std::vector;
using std::map;

// DEBUG
#include <sstream>

namespace boa {
/**
  Model a single constraint.

  A constraint is a simple linear inequality of the form BIG >= SMALL
  Use addBig in order to add an expression, literal or a constant
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
  map<string, int> literals_;
  string blame_;

  void addLiteral(int num, string var) {
    literals_[var] += num;
  }

  void addLeft(int left) {
    left_ += left;
  }

  // TODO(tzafrir): Disallow copying and assignment.

 public:
  class Expression {
    int val_;
    map<string, int> vars_;
   public:
    friend class Constraint;
    Expression() : val_(0) {}
    void add(const Expression& expr) {
      for (map<string, int>::const_iterator it = expr.vars_.begin(); it != expr.vars_.end(); ++it) {
        add(it->first, it->second);
      }
      add(expr.val_);
    }
    void add(const string& var, int num = 1) {vars_[var] += num;}
    void add(int num) {val_ += num;}

    void sub(const Expression& expr) {
      for (map<string, int>::const_iterator it = expr.vars_.begin(); it != expr.vars_.end(); ++it) {
        add(it->first, -it->second);
      }
      add(-expr.val_);
    }
    
    void mul(int num) {
      for (map<string, int>::iterator it = vars_.begin(); it != vars_.end(); ++it) {
        it->second *= num;
      }
      val_ *= num;        
    }

    void div(int num) {
      for (map<string, int>::iterator it = vars_.begin(); it != vars_.end(); ++it) {
        it->second /= num;
      }
      val_ /= num;        
    }

    /**
      Does the expression contain only a free element (no literals)?    
    */
    bool IsConst() {
      for (map<string, int>::const_iterator it = vars_.begin(); it != vars_.end(); ++it) {
        if (it->second != 0) {
          return false;
        }
      }
      return true;
    }
    
    int GetConst() {
      return val_;
    }

    // DEBUG
    static string int2str(int i) {
      std::ostringstream buffer;
      buffer << i;
      return buffer.str();
    }
    string toString() {
      string s;
      for (map<string, int>::const_iterator it = vars_.begin(); it != vars_.end(); ++it) {
        if ((!s.empty()) && (it->second >= 0)) s += "+ ";
        s += int2str(it->second) + "*" + it->first + " ";
      }
      if (s.empty() || (val_ != 0)) {
        if ((!s.empty()) && (val_ >= 0)) s += "+ ";
        s += int2str(val_);
      }
      return s;
    }
  };

  Constraint() : left_(0) {}
  
  void SetBlame(const string &blame) {
    blame_ = blame;
  }
  
  string Blame() {
    return blame_;
  }

  void addBig(const Expression& expr) {
    for (map<string, int>::const_iterator it = expr.vars_.begin(); it != expr.vars_.end(); ++it) {
      addBig(it->first, it->second);
    }
    addBig(expr.val_);
  }

  void addBig(const string& var, int num = 1) {
    addLiteral(-num, var);
  }

  void addBig(int num) {
    addLeft(num);
  }

  void addSmall(const Expression& expr) {
    for (map<string, int>::const_iterator it = expr.vars_.begin(); it != expr.vars_.end(); ++it) {
      addSmall(it->first, it->second);
    }
    addSmall(expr.val_);
  }

  void addSmall(const string& var, int num = 1) {
    addLiteral(num, var);
  }

  void addSmall(int num) {
    addLeft(-num);
  }

  void Clear() {
   left_ = 0;
   literals_.clear();
  }

  void GetVars(set<string>& vars) const{
    for (map<string, int>::const_iterator it = literals_.begin(); it != literals_.end(); ++it) {
      vars.insert(it->first);
    }
  }

  void AddToLPP(glp_prob *lp, int row, map<string, int>& colNumbers) const {
    int indices[MAX_SIZE + 1];
    double values[MAX_SIZE + 1];

    // TODO if size > MAX_SIZE...

    int count = 1;
    for (map<string, int>::const_iterator it = literals_.begin(); it != literals_.end(); ++it, ++count) {
      indices[count] = colNumbers[it->first];
      values[count] = it->second;
    }
    glp_set_row_bnds(lp, row, GLP_UP, 0.0, left_);
    glp_set_mat_row(lp, row, literals_.size(), indices, values);
  }

};

class ConstraintProblem {
 private:
  const vector<Constraint> NO_CONSTRAINTS;
  vector<Constraint> constraints;
  set<Buffer> buffers;

  set<string> CollectVars() const;

  vector<Buffer> Solve(const vector<Constraint> &inputConstraints, const set<Buffer> &inputBuffers) const;

  vector<Constraint> Blame(const vector<Constraint> &input, const set<Buffer> &buffer) const;
 public:
  void AddBuffer(const Buffer& buffer) {
    buffers.insert(buffer);
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
  vector<Buffer> Solve() const;

  /**
    Solve the constraint problem and generate a minimal set of constraints which cause each overrun

    Return a map - the keys are possibly overrun buffers, the corresponding value is a small set of
    constraints which cause the overrun. The set is minimal in the sense that no subset of these
    constraint will cause the specific buffer overrun, there might be other (smaller) set which will
    also cause the overrun.
  */
  map<Buffer, vector<Constraint> > SolveAndBlame() const;
};
} //namespace boa
#endif /* __BOA_CONSTRAINT_H */

