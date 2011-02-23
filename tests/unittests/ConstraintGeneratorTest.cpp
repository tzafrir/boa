#include "gtest/gtest.h"

#include "ConstraintGenerator.h"

#include <string>

using std::string;

namespace boa {

class MockConstraintProblem : public ConstraintProblem {
  double left_;
  string blame_;
  string location_;
  Constraint::Type type_;
 public:
  MockConstraintProblem() : ConstraintProblem(false), left_(-333.333) {}

  void SetExpectedConstraintParams(double left, string blame, string location,
                                   Constraint::Type type) {
    left_ = left;
    blame_ = blame;
    location_ = location;
    type_ = type;
  }

  virtual void AddConstraint(const Constraint& constraint) {
    EXPECT_DOUBLE_EQ(left_, constraint.left_);
    EXPECT_EQ(Constraint::TypeToChar(type_) + blame_ + " [" + location_ + "]", constraint.blame_);
    EXPECT_EQ(type_, constraint.type_);
  }
};

class ConstraintGeneratorTest : public ::testing::Test {
 public:
  ConstraintGeneratorTest() : cp(NULL), cg(NULL), blame("blame"), location("location") {}
 protected:
  string blame;
  string location;
  MockConstraintProblem* cp;
  ConstraintGenerator* cg;

  // Runs before each test.
  void SetUp() {
    delete cg;
    delete cp;
    cp = new MockConstraintProblem();
    cg = new ConstraintGenerator(*cp);
  }
};

TEST_F(ConstraintGeneratorTest, SafeFunctionTest) {
  ASSERT_TRUE(ConstraintGenerator::IsSafeFunction("puts"));
  ASSERT_TRUE(ConstraintGenerator::IsSafeFunction("strtok"));
  ASSERT_FALSE(ConstraintGenerator::IsSafeFunction("gets"));
}

TEST_F(ConstraintGeneratorTest, UnsafeFunctionTest) {
  ASSERT_TRUE(ConstraintGenerator::IsUnsafeFunction("gets"));
  ASSERT_TRUE(ConstraintGenerator::IsUnsafeFunction("scanf"));
  ASSERT_FALSE(ConstraintGenerator::IsUnsafeFunction("puts"));
}

TEST_F(ConstraintGeneratorTest, GenerateConstraintMax) {
  string s = "_Max";
  cp->SetExpectedConstraintParams(10.0 - 12.7, blame+s, location+s, Constraint::INTERESTING);
  cg->GenerateConstraint(10.0, 12.7, VarLiteral::MAX, blame+s, location+s, Constraint::INTERESTING);
}

TEST_F(ConstraintGeneratorTest, GenerateConstraintMin) {
  string s = "_Min";
  cp->SetExpectedConstraintParams(34.2 - 99.0, blame+s, location+s, Constraint::STRUCTURAL);
  cg->GenerateConstraint(99.0, 34.2, VarLiteral::MIN, blame+s, location+s, Constraint::STRUCTURAL);
}

}  // namespace boa
