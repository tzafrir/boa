#include "Helpers.h"

#include <set>
#include <sstream>
#include <string>

using std::set;
using std::string;
using std::stringstream;

namespace boa {

namespace Helpers {
  void ReplaceInString(string &s, char c, const string &replaceWith) {
    size_t n = s.find_first_of(c, 0);
    while (n != string::npos) {
      s.replace(n, 1, replaceWith);
      n = s.find_first_of(c, n + replaceWith.size());
    }
  }

  string DoubleToString(double d) {
    std::ostringstream buffer;
    buffer << d;
    return buffer.str();
  }

  bool IsPrefix(string str1, string str2) {
    return str2.find(str1) == 0;
  }

  set<string> SplitString(const string &str, char c) {
    stringstream ss(str);
    string out;
    set<string> result;
    while (getline(ss, out, c)) {
      result.insert(out);
    }
    return result;
  }
}  // namespace Helpers

}  // namespace boa
