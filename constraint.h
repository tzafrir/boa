#ifndef __BOA_CONSTRAINT_H
#define __BOA_CONSTRAINT_H /* */

#include <string>
#include <set>
#include <list>
#include <map>
#include <glpk.h>

using std::string;
using std::set;
using std::list;
using std::map;

namespace NameBufferExpression {
 	enum Type {USED, ALLOC};
 	enum Dir  {MIN, MAX};
	static string Name(const string& buffer, Type type, Dir dir) {
		return buffer + "!" + (type == USED ? "used" : "alloc") + "!" + (dir == MIN ? "min" : "max");
	}
};

/**
	Model a singel constraint.
	
	The constrint form is - 
	
	C >= aX + bY ...
		
	Where C is a constant (referred as "left"),  a small letter (a, b...) is an 
	integer value ("num") and a capital letter (X, Y...) is a string name of variable
*/
class Constraint {
 private:
 	const static int MAX_SIZE = 1000;
	int left_;
	map<string, int> expressions_;
 public:
 	Constraint() : left_(0) {};
 	
	void AddExpression(int num, string var) {
		expressions_[var] = num;
	}
	
	void SetLeft(int left) {
		left_ = left;
	}
	
	void GetVars(set<string>& vars) {
		for (map<string, int>::iterator it = expressions_.begin(); it != expressions_.end(); ++it) {
			vars.insert(it->first);
		}
	}
	
	void AddToLPP(glp_prob *lp, int row, map<string, int>& colNumbers) {
		int indices[MAX_SIZE];
		double values[MAX_SIZE];
		
		/* TODO if size > MAX_SIZE...  */
		
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
 	set<string> buffers;
 public:
 	void AddBuffer(const string& name) {
 		buffers.insert(name);
 	}
 	
 	void AddConstraint(const Constraint& c) {
 		constraints.push_back(c);
 	}
 	
 	void Clear() {
 		buffers.clear();
 		constraints.clear();
 	}
 	
 	void Solve();
 	
};

#endif /* __BOA_CONSTRAINT_H */

