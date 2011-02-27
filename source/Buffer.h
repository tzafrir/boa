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
    unsigned offset_;

   public:
    // Default constructor, so we can create a map of Buffers
    Buffer() : VarLiteral(NULL), offset_(0) {}
    
    Buffer(const void* ValueNode, const string& readableName, const string& filename, unsigned offset = 0) :
      VarLiteral(ValueNode), readableName_(readableName), filename_(filename), offset_(offset) {}

    Buffer(const void* ValueNode, unsigned offset = 0) :
      VarLiteral(ValueNode), offset_(offset) {}

    virtual bool IsNull() {
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
    
    bool operator<(const Buffer& other) const {
       return (this->ValueNode_ < other.ValueNode_) ||
           ((this->ValueNode_ == other.ValueNode_) && (this->offset_ < other.offset_));
     }
  };
}

#endif // __BOA_BUFFER_H

