#ifndef __BOA_VARLITERAL_H
#define __BOA_VARLITERAL_H /* */

#include <string>
#include <sstream>

using std::string;
using std::stringstream;

namespace boa {
  enum ExpressionDir  {MIN, MAX};
  enum ExpressionType {USED, ALLOC};

  class VarLiteral {  
   protected:
    void* ASTNode_;

    VarLiteral(void* ASTNode) : ASTNode_(ASTNode) {}

   public:    
    
    virtual string getUniqueName() const {
      stringstream ss;
      ss << "V" << ASTNode_;
      return ss.str();
    }

    virtual string NameExpression(ExpressionDir dir, ExpressionType type) const {
      return getUniqueName() + "!" + (type == USED ? "used" : "alloc") + "!" + (dir == MIN ? "min" : "max");
    }

    bool operator<(const VarLiteral& other) const {
      return this->ASTNode_ < other.ASTNode_;
    }
  };
}

#endif // __BOA_VARLITERAL_H

