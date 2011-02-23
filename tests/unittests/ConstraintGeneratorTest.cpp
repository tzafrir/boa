#include "gtest/gtest.h"

#include "ConstraintGenerator.h"

namespace boa {

TEST(ConstraintGeneratorTest, SafeFunctionTest) {
  ASSERT_TRUE(ConstraintGenerator::IsSafeFunction("puts"));
  ASSERT_TRUE(ConstraintGenerator::IsSafeFunction("strtok"));
  ASSERT_FALSE(ConstraintGenerator::IsSafeFunction("gets"));
}

TEST(ConstraintGeneratorTest, UnsafeFunctionTest) {
  ASSERT_TRUE(ConstraintGenerator::IsUnsafeFunction("gets"));
  ASSERT_TRUE(ConstraintGenerator::IsUnsafeFunction("scanf"));
  ASSERT_FALSE(ConstraintGenerator::IsUnsafeFunction("puts"));
}

}  // namespace boa
