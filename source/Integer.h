#ifndef __BOA_INTEGER_H
#define __BOA_INTEGER_H /* */

#include <string>
#include <sstream>
#include "VarLiteral.h"

using std::string;
using std::stringstream;

namespace boa {
  class Integer : public VarLiteral {
   public:

    Integer(void* ASTNode) : VarLiteral(ASTNode) {}

    string NameExpression(ExpressionDir dir, ExpressionType type = USED) const {
      return getUniqueName() + "!" + (dir == MIN ? "min" : "max");
    }
  };
}

#endif // __BOA_INTEGER_H

