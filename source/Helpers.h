#ifndef __BOA_HELPERS_H
#define __BOA_HELPERS_H

#include <set>
#include <sstream>
#include <string>

using std::set;
using std::string;

namespace boa {

/**
 * Helper functions.
 *
 * This namespace should only have static methods.
 */
namespace Helpers {
  /**
   * Replaces each occurence of the character c with the string replaceWith.
   * Note - this mutates the string s.
   */
  void ReplaceInString(string &s, char c, const string& replaceWith);

  /**
   * Returns a string representation of the input.
   */
  string DoubleToString(double d);

  /**
   * Return true iff str1 is a prefix of str2
   */
  bool IsPrefix(string str1, string str2);

  /**
   * Splits str by the character c.
   */
  set<string> SplitString(const string &str, char c);
}  // namespace Helpers

}  // namespace boa

#endif  // __BOA_HELPERS_H

