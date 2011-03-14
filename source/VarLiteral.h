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
    const void* ValueNode_;
    // isTmp allows a temporary buffer to be aliased with llvm node without being used later when
    // reffering the same node by generating two different buffer names
    bool isTmp_;

    VarLiteral(const void* ValueNode, bool isTmp = false) : ValueNode_(ValueNode), isTmp_(isTmp) {}

  public:
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
   protected:

    virtual string GetUniqueName() const {
      stringstream ss;
      if (isTmp_) {
        ss << "temp@" << ValueNode_;
      } else {
        ss << "v@" << ValueNode_;
      }
      return ss.str();
    }

   public:

    virtual string NameExpression(ExpressionDir dir, ExpressionType type) const {
      return GetUniqueName() + "!" + TypeToString(type) + "!" + DirToString(dir);
    }

    virtual bool IsBuffer() const { return false; }

    virtual bool operator<(const VarLiteral& other) const {
      if (this->ValueNode_ == other.ValueNode_) {
        return this->isTmp_ < other.isTmp_;
      }
      return this->ValueNode_ < other.ValueNode_;
    }
  };
}

#endif // __BOA_VARLITERAL_H

