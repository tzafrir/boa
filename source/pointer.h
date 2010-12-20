#ifndef __BOA_POINTER_H
#define __BOA_POINTER_H /* */

#include <string>
#include <sstream>

using std::string;
using std::stringstream;

namespace boa {
  class Pointer {
   private:
    void* ASTNode_;

  // TODO(tzafrir): Disallow copying and assignment.

   public:

    virtual string getUniqueName() const {
      stringstream ss;
      ss << ASTNode_;
      string retval;
      ss >> retval;
      return retval;
    }

    Pointer(void* ASTNode) : ASTNode_(ASTNode) {}

    bool operator<(const Pointer& other) const {
      return this->ASTNode_ < other.ASTNode_;
    }
  };
}

#endif // __BOA_POINTER_H

