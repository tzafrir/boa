#include "gtest/gtest.h"

#include "Helpers.h"

#include <set>
#include <string>

using std::set;
using std::string;

using boa::Helpers::IsPrefix;
using boa::Helpers::ReplaceInString;
using boa::Helpers::SplitString;

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
  string selfContaining("aba");
  ReplaceInString(selfContaining, 'a', "aa");
  ASSERT_EQ(selfContaining, "aabaa");
}

TEST(HelpersTest, IsPrefixTest) {
  ASSERT_TRUE(IsPrefix("ab", "abcd"));
  ASSERT_FALSE(IsPrefix("ab", "cabd"));
}

TEST(HelpersTest, SplitStringTest) {
  string s("a,b,c");
  set<string> split(SplitString(s, ','));
  ASSERT_EQ(3, split.size());
  ASSERT_EQ(1, split.count("a"));
  ASSERT_EQ(1, split.count("b"));
  ASSERT_EQ(1, split.count("c"));
}
