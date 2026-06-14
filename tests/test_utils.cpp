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
