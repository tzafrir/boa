#ifndef __BOA_HELPERS_H
#define __BOA_HELPERS_H

#include <sstream>

namespace boa {

/**
 * Helper functions.
 *
 * This class should only have static methods.
 */
class Helpers {
 public:
  /**
   * Replaces each occurence of each character in chars with num characters from replaceWith.
   * Note - this mutates the string s.
   */
  static void ReplaceInString(string &s, const char* chars, const char* replaceWith, int num) {
    size_t n = s.find_first_of(chars, 0);
    while (n != string::npos) {
      s.replace(n, num, replaceWith);
      n = s.find_first_of(chars, n+1);
    }
  }

  static string DoubleToString(double i) {
    std::ostringstream buffer;
    buffer << i;
    return buffer.str();
  }
};

}  // namespace boa

#endif  // __BOA_HELPERS_H

