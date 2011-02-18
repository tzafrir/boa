#include "Helpers.h"

#include <sstream>

namespace boa {

namespace Helpers {
  void ReplaceInString(string &s, const char* chars, const char* replaceWith, int num) {
    size_t n = s.find_first_of(chars, 0);
    while (n != string::npos) {
      s.replace(n, num, replaceWith);
      n = s.find_first_of(chars, n+1);
    }
  }

  string DoubleToString(double d) {
    std::ostringstream buffer;
    buffer << d;
    return buffer.str();
  }
}  // namespace Helpers

}  // namespace boa
