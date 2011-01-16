#ifndef __BOA_POINTER_H
#define __BOA_POINTER_H /* */

#include "Integer.h"

namespace boa {
  class Pointer : public VarLiteral {

  public:
    Pointer(void* ASTNode) : VarLiteral(ASTNode) {}

  };
}

#endif // __BOA_POINTER_H

