#ifndef __BOA_VARLITERAL_H
#define __BOA_VARLITERAL_H /* */

#include <string>
#include <sstream>

using std::string;
using std::stringstream;

namespace boa {
  enum ExpressionDir  {MIN, MAX};
  enum ExpressionType {USED, ALLOC, LEN};  

#define Type2Str(type) ((type) == USED ? "used" : ((type) == ALLOC ? "alloc" : "len"))
#define Dir2Str(dir) ((dir) == MIN ? "min" : "max")

  class VarLiteral {  
   protected:
    void* ASTNode_;

    VarLiteral(void* ASTNode) : ASTNode_(ASTNode) {}

   public:       
    
    virtual string getUniqueName() const {
      stringstream ss;
      ss << "v@" << ASTNode_;
      return ss.str();
    }

    virtual string NameExpression(ExpressionDir dir, ExpressionType type) const {
      return getUniqueName() + "!" + Type2Str(type) + "!" + Dir2Str(dir);
    }

    bool operator<(const VarLiteral& other) const {
      return this->ASTNode_ < other.ASTNode_;
    }
  };
}

#endif // __BOA_VARLITERAL_H

