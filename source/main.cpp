#include <iostream>
#include <set>
#include <string>
#include "constraint.h"
#include "buffer.h"

using std::set;
using std::string;
using boa::Buffer;

int main(int argc, char* argv[]) {
  ConstraintProblem cp;
  // call pointer analysis
  // ...

  // generage constraints
  // ...

  // solve constraint problem
  list<Buffer> unsafeBuffers = cp.Solve();
  
  if (unsafeBuffers.empty()) {
	  std::cout << "Safe!" << std::endl;
  	return 0;
  }
  else {
	  std::cout << "Buffer overruns are possible!" << std::endl;
  	return 1; // non zero return value means fail => buffer overruns are possible
  }
}

