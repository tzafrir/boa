#ifndef __BOA_VARLITERAL_H
#define __BOA_VARLITERAL_H /* */

#include "llvm/Value.h"
#include <string>
#include <sstream>

using std::string;
using std::stringstream;

using namespace llvm;

namespace boa {

  class VarLiteral {
   protected:
    const Value* ValueNode_;

    VarLiteral(const Value* ValueNode) : ValueNode_(ValueNode) {}
    enum ExpressionDir  {MIN, MAX};
    enum ExpressionType {USED, ALLOC, LEN_READ, LEN_WRITE};

    static inline string DirToString(ExpressionDir dir) {
      switch (dir) {
        case MIN:
          return "min";
        case MAX:
          return "max";
        default:
          return "";
      }
    }

    static inline string TypeToString(ExpressionType type) {
      switch (type) {
        case USED:
          return "used";
        case ALLOC:
          return "alloc";
        case LEN_READ:
          return "len-read";
        case LEN_WRITE:
          return "len-write";
        default:
          return "";
      }
    }

   public:

    virtual string getUniqueName() const {
      stringstream ss;
      ss << "v@" << ValueNode_;
      return ss.str();
    }

    virtual string NameExpression(ExpressionDir dir, ExpressionType type) const {
      return getUniqueName() + "!" + TypeToString(type) + "!" + DirToString(dir);
    }

    bool operator<(const VarLiteral& other) const {
      return this->ValueNode_ < other.ValueNode_;
    }
  };
}

#endif // __BOA_VARLITERAL_H

