#ifndef __BOA_BUFFER_H
#define __BOA_BUFFER_H /* */

#include <string>
#include <sstream>

#include "VarLiteral.h"

using std::string;
using std::stringstream;

namespace boa {

  class Buffer : public VarLiteral {
   private:    
    string readableName_, filename_;
    int line_;

   public:

    Buffer(void* ASTNode, const string& readableName, const string& filename, int line) :
      VarLiteral(ASTNode), readableName_(readableName), filename_(filename), line_(line) {}
      
    Buffer(void* ASTNode) : VarLiteral(ASTNode) {}

    const string& getReadableName() const {
      return readableName_;
    }

    string getSourceLocation() const {
      stringstream ss;
      ss << filename_ << ":" << line_;
      return ss.str();
    }
  };
}

#endif // __BOA_BUFFER_H

