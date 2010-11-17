#include "constraint.h"
#include <iostream>
#include <glpk.h>


using std::cout;
using std::endl;

void ConstraintProblem::Solve() {
	if (buffers.empty()) {
		cout << "No buffers" << endl;
		return; 
	}

//	Expression goal = 0;
	set<string> vars;
	for (set<string>::iterator buffer = buffers.begin(); buffer != buffers.end(); ++buffer) {
		using namespace NameBufferExpression;
		vars.insert(Name(*buffer, USED, MIN));
		vars.insert(Name(*buffer, USED, MAX));
		vars.insert(Name(*buffer, ALLOC, MIN));
		vars.insert(Name(*buffer, ALLOC, MAX));		
	}
	
	for (list<Constraint>::iterator constraint = constraints.begin(); constraint != constraints.end(); ++constraint) {
		constraint->GetVars(vars);
	}
	
	map<string, int> varToCol;
	{ // Fill varToCol map
		int col = 1;
		for (set<string>::iterator var = vars.begin(); var != vars.end(); ++var, ++col) {
			varToCol[*var] = col;
		}
	}
	
	glp_prob *lp;
	lp = glp_create_prob();
	glp_set_obj_dir(lp, GLP_MAX);
	glp_add_rows(lp, constraints.size());
	glp_add_cols(lp, vars.size());	
	{ // Fill matrix
		int row = 1;
		for (list<Constraint>::iterator constraint = constraints.begin(); constraint != constraints.end(); ++constraint, ++row) {
			constraint->AddToLPP(lp, row, varToCol);
		}
	}

	for (set<string>::iterator buffer = buffers.begin(); buffer != buffers.end(); ++buffer) {
		// Set objective coeficients
		using namespace NameBufferExpression;
		glp_set_obj_coef(lp, varToCol[Name(*buffer, USED, MIN)], 1.0);
		glp_set_obj_coef(lp, varToCol[Name(*buffer, USED, MAX)], -1.0);
		glp_set_obj_coef(lp, varToCol[Name(*buffer, ALLOC, MIN)], 1.0);
		glp_set_obj_coef(lp, varToCol[Name(*buffer, ALLOC, MAX)], -1.0);
	}

	for (int i = 1; i <= vars.size(); ++i) {
		glp_set_col_bnds(lp, i, GLP_FR, 0.0, 0.0);
	}

	glp_smcp params;
	glp_init_smcp(&params);
	params.msg_lev = GLP_MSG_ERR;
	glp_simplex(lp, &params);

	for (set<string>::iterator buffer = buffers.begin(); buffer != buffers.end(); ++buffer) {
		// Print result
		using namespace NameBufferExpression;
		cout << Name(*buffer, USED, MIN) << "\t = " << glp_get_col_prim(lp, varToCol[Name(*buffer, USED, MIN)]) << endl;
		cout << Name(*buffer, USED, MAX) << "\t = " << glp_get_col_prim(lp, varToCol[Name(*buffer, USED, MAX)]) << endl;
		cout << Name(*buffer, ALLOC, MIN) << "\t = " << glp_get_col_prim(lp, varToCol[Name(*buffer, ALLOC, MIN)]) << endl;
		cout << Name(*buffer, ALLOC, MAX) << "\t = " << glp_get_col_prim(lp, varToCol[Name(*buffer, ALLOC, MAX)]) << endl;
		
		if (glp_get_col_prim(lp, varToCol[Name(*buffer, USED, MAX)]) > glp_get_col_prim(lp, varToCol[Name(*buffer, ALLOC, MIN)])) {
			cout << endl << "  !! POSSIBLE BUFFER OVERRUN ON " << *buffer << endl << endl; 
		}
	}	

	glp_delete_prob(lp);
}

