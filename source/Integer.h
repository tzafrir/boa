#ifndef __BOA_INTEGER_H
#define __BOA_INTEGER_H /* */

#include <string>
#include <sstream>
#include "VarLiteral.h"
#include "llvm/Value.h"

namespace boa {
  class Integer : public VarLiteral {
   public:

    Integer(const Value* node) : VarLiteral(node) {}

    string NameExpression(ExpressionDir dir, ExpressionType type = USED) const {
      return getUniqueName() + "!" + DirToString(dir);
    }
  };
}

#endif // __BOA_INTEGER_H

