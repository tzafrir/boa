#include "gtest/gtest.h"

#include "Helpers.h"
#include <string>

using std::string;
using boa::Helpers::ReplaceInString;

TEST(HelpersTest, StringReplaceTest) {
  string a = "Hello World";
  string a_(a);
  ReplaceInString(a, 'q', "$");
  ASSERT_EQ(a_, a) << "No q in the string a";
  string b = "abcd";
  string c(b), d(b);
  ReplaceInString(b, 'b', "^");
  ASSERT_EQ("a^cd", b) << "Only b should be replaced";
  ReplaceInString(c, 'a', "Lorem ipsum ");
  ASSERT_EQ("Lorem ipsum bcd", c) << "character a should expand to a long string";
  ReplaceInString(d, 'c', "");
  ASSERT_EQ("abd", d) << "character c should be removed";
}
