#ifndef __BOA_POINTER_H
#define __BOA_POINTER_H /* */

#include "Integer.h"
#include "llvm/Value.h"

namespace boa {
  class Pointer : public VarLiteral {

  public:
    Pointer(Value* node) : VarLiteral(node) {}
  };
}

#endif // __BOA_POINTER_H

