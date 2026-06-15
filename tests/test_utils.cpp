#include "utils.hpp"

#include <gtest/gtest.h>

#include <regex>

TEST(TimestampTest, HasCorrectFormat) {
  std::regex pattern(R"(\d{8}_\d{6})");
  EXPECT_TRUE(std::regex_match(getTimestampString(), pattern));
}

TEST(TimestampTest, HasCorrectLength) {
  // YYYYMMDD_HHMMSS = 15 characters
  EXPECT_EQ(getTimestampString().length(), 15u);
}

// ---------------------------------------------------------------------------
// parseCommaSeparated
// ---------------------------------------------------------------------------

TEST(ParseCommaSeparatedTest, SingleCode) {
  auto result = parseCommaSeparated("ARN");
  ASSERT_EQ(result.size(), 1u);
  EXPECT_EQ(result[0], "ARN");
}

TEST(ParseCommaSeparatedTest, MultipleCodes) {
  auto result = parseCommaSeparated("ARN,LEA,MIR");
  ASSERT_EQ(result.size(), 3u);
  EXPECT_EQ(result[0], "ARN");
  EXPECT_EQ(result[1], "LEA");
  EXPECT_EQ(result[2], "MIR");
}

TEST(ParseCommaSeparatedTest, EmptyStringReturnsEmpty) {
  EXPECT_TRUE(parseCommaSeparated("").empty());
}

TEST(ParseCommaSeparatedTest, TrailingCommaIgnored) {
  auto result = parseCommaSeparated("ARN,LEA,");
  EXPECT_EQ(result.size(), 2u);
}

TEST(ParseCommaSeparatedTest, LeadingCommaIgnored) {
  auto result = parseCommaSeparated(",ARN");
  ASSERT_EQ(result.size(), 1u);
  EXPECT_EQ(result[0], "ARN");
}

TEST(ParseCommaSeparatedTest, ConsecutiveCommasIgnored) {
  auto result = parseCommaSeparated("ARN,,LEA");
  ASSERT_EQ(result.size(), 2u);
  EXPECT_EQ(result[0], "ARN");
  EXPECT_EQ(result[1], "LEA");
}
