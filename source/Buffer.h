#ifndef __BOA_BUFFER_H
#define __BOA_BUFFER_H /* */

#include <string>
#include <sstream>

#include "VarLiteral.h"
#include "llvm/Value.h"

using std::string;
using std::stringstream;

using namespace llvm;

namespace boa {

  class Buffer : public VarLiteral {
   private:
    string readableName_, filename_;

   public:
    // Default constructor, so we can create a map of Buffers
    Buffer() : VarLiteral(NULL) {}
    
    Buffer(const Value* ValueNode, const string& readableName, const string& filename) :
      VarLiteral(ValueNode), readableName_(readableName), filename_(filename) {}

    Buffer(const Value* ValueNode) : VarLiteral(ValueNode) {}

    bool IsNull() {
      return ValueNode_ == NULL;
    }

    const string& getReadableName() const {
      return readableName_;
    }

    string getSourceLocation() const {
      stringstream ss;
      ss << filename_;
      return ss.str();
    }
    
    bool inline IsBuffer() const { return true; }
    
  };
}

#endif // __BOA_BUFFER_H

