#include "constraint.h"

int main() {
	ConstraintProblem cp;

	cp.AddBuffer("buf");

	{ //add constraints
		using namespace NameBufferExpression;
		Constraint c1, c2, c3, c4, c5, c6;
		
		// buf.alloc.min <= 10
		c1.AddExpression(1, Name("buf", ALLOC, MIN));
		c1.SetLeft(10);
		cp.AddConstraint(c1);
		
		// buf.alloc.max >= 10
		c2.AddExpression(-1, Name("buf", ALLOC, MAX));
		c2.SetLeft(-10);
		cp.AddConstraint(c2);
		
		// buf.used.min <= 0
		c3.AddExpression(1, Name("buf", USED, MIN));
		c3.SetLeft(0);
		cp.AddConstraint(c3);
		
		// buf.used.max >= 0
		c4.AddExpression(-1, Name("buf", USED, MAX));
		c4.SetLeft(0);
		cp.AddConstraint(c4);

		// i.max >= 11
		c5.AddExpression(-1, "i.max");
		c5.SetLeft(-11);
		cp.AddConstraint(c5);

		// i.max <= buf.used.max
		c6.AddExpression(1, "i.max");
		c6.AddExpression(-1, Name("buf", USED, MAX));
		c6.SetLeft(0);
		//cp.AddConstraint(c6); // uncomment to add this constraint and wtach buffer overrun detection
	}
	cp.Solve();
	return 0;
}

