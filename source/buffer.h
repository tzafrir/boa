#ifndef __BOA_BUFFER_H
#define __BOA_BUFFER_H /* */

#include <string>
#include <sstream>

using std::string;
using std::stringstream;

namespace boa {
  class Buffer {
   private:
    void* ASTNode_;
    string readableName_, filename_;
    int line_;

  // TODO(tzafrir): Disallow copying and assignment.

   public:
    enum ExpressionType {USED, ALLOC};
    enum ExpressionDir  {MIN, MAX};

    virtual string getUniqueName() const {
      stringstream ss;
      ss << ASTNode_;
      return ss.str();
    }

    const string& getReadableName() const {
      return readableName_;
    }

    string getSourceLocation() const {
      stringstream ss;
      ss << filename_ << ":" << line_;
      return ss.str();
    }

    string NameExpression(ExpressionType type, ExpressionDir dir) {
      return getUniqueName() + "!" + (type == USED ? "used" : "alloc") + "!" + (dir == MIN ? "min" : "max");
    }

    Buffer(void* ASTNode, const string& readableName, const string& filename, int line) :
      ASTNode_(ASTNode), readableName_(readableName), filename_(filename), line_(line) {}

    Buffer(void* ASTNode) :
      ASTNode_(ASTNode) {}

  };
}

#endif // __BOA_BUFFER_H

