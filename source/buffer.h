#ifndef __BOA_BUFFER_H
#define __BOA_BUFFER_H /* */

#include <string>
#include <sstream>

using std::string;
using std::stringstream;
using std::hex;

namespace boa {
	class Buffer {
	 private:
	  void* ASTNode_;
	  
	 public:
   	enum ExpressionType {USED, ALLOC};
   	enum ExpressionDir  {MIN, MAX};

	  virtual string getUniqueName() const {
	    stringstream ss;
	    ss << ASTNode_;
	    string retval;
	    ss >> retval;
	    return retval;
	  }

	  string NameExpression(ExpressionType type, ExpressionDir dir) {
		  return getUniqueName() + "!" + (type == USED ? "used" : "alloc") + "!" + (dir == MIN ? "min" : "max");
		}
		
		Buffer(void* ASTNode) : ASTNode_(ASTNode) {}
	};
}

#endif // __BOA_BUFFER_H

