#ifndef __BOA_BUFFER_H
#define __BOA_BUFFER_H /* */

#include <string>

using std::string;

namespace boa {
	class Buffer {
	 public:
   	enum ExpressionType {USED, ALLOC};
   	enum ExpressionDir  {MIN, MAX};

	  virtual string getUniqueName();

	  string NameExpression(ExpressionType type, ExpressionDir dir) {
		  return getUniqueName() + "!" + (type == USED ? "used" : "alloc") + "!" + (dir == MIN ? "min" : "max");
		}
	};
}

#endif // __BOA_BUFFER_H

