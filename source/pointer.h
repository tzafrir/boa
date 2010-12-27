#ifndef __BOA_POINTER_H
#define __BOA_POINTER_H /* */

#include <string>
#include <sstream>
#include "varLiteral.h"

using std::string;
using std::stringstream;

namespace boa {
  class Pointer : public VarLiteral {
   public:

    Pointer(void* ASTNode) : VarLiteral(ASTNode) {}

    string NameExpression(ExpressionDir dir, ExpressionType type) const {
      return getUniqueName() + "!" + (dir == MIN ? "min" : "max");
    }
  };
}

#endif // __BOA_POINTER_H

