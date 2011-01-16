#ifndef __BOA_VARLITERAL_H
#define __BOA_VARLITERAL_H /* */

#include <string>
#include <sstream>

using std::string;
using std::stringstream;

namespace boa {

  class VarLiteral {
   protected:
    void* ASTNode_;

    VarLiteral(void* ASTNode) : ASTNode_(ASTNode) {}
    enum ExpressionDir  {MIN, MAX};
    enum ExpressionType {USED, ALLOC, LEN};

    static inline string TypeToString(ExpressionType type) {
      switch (type) {
        case USED:
          return "used";
        case ALLOC:
          return "alloc";
        case LEN:
          return "len";
        default:
          return "";
      }
    }

#define Dir2Str(dir) ((dir) == MIN ? "min" : "max")

   public:

    virtual string getUniqueName() const {
      stringstream ss;
      ss << "v@" << ASTNode_;
      return ss.str();
    }

    virtual string NameExpression(ExpressionDir dir, ExpressionType type) const {
      return getUniqueName() + "!" + TypeToString(type) + "!" + Dir2Str(dir);
    }

    bool operator<(const VarLiteral& other) const {
      return this->ASTNode_ < other.ASTNode_;
    }
  };
}

#endif // __BOA_VARLITERAL_H

