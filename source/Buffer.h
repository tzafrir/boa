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
    int line_;

   public:

    Buffer(const Value* ValueNode, const string& readableName, const string& filename, int line) :
      VarLiteral(ValueNode), readableName_(readableName), filename_(filename), line_(line) {}

    Buffer(const Value* ValueNode) : VarLiteral(ValueNode) {}

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

