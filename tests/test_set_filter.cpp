#include "card_utils.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <fstream>
#include <sstream>

static auto readFile(const std::string &path) -> std::string {
  std::ifstream f(path);
  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

static auto loadSet(const std::string &filename) -> json {
  return json::parse(
      readFile(std::string(TEST_DATA_DIR) + "/mock_data/" + filename));
}

static auto contains(const std::vector<std::string> &v, const std::string &s)
    -> bool {
  return std::find(v.begin(), v.end(), s) != v.end();
}

// SetList_minimal.json has:
//   TST  – expansion – 2025-01-01
//   TST2 – expansion – 2025-06-01
//   TCMD – commander – 2025-03-01

TEST(FilterSetCodesTest, NoFiltersReturnsAll) {
  auto data = loadSet("SetList_minimal.json");
  auto codes = filterSetCodes(data["data"], "", "", "", "", "");
  EXPECT_EQ(codes.size(), 3u);
}

TEST(FilterSetCodesTest, EmptyArrayReturnsEmpty) {
  auto codes = filterSetCodes(json::array(), "", "", "", "", "");
  EXPECT_TRUE(codes.empty());
}

// ---------------------------------------------------------------------------
// Type filter
// ---------------------------------------------------------------------------

TEST(FilterSetCodesTest, FilterByTypeExpansion) {
  auto data = loadSet("SetList_minimal.json");
  auto codes = filterSetCodes(data["data"], "expansion", "", "", "", "");
  EXPECT_EQ(codes.size(), 2u);
  EXPECT_TRUE(contains(codes, "TST"));
  EXPECT_TRUE(contains(codes, "TST2"));
  EXPECT_FALSE(contains(codes, "TCMD"));
}

TEST(FilterSetCodesTest, FilterByTypeCommander) {
  auto data = loadSet("SetList_minimal.json");
  auto codes = filterSetCodes(data["data"], "commander", "", "", "", "");
  ASSERT_EQ(codes.size(), 1u);
  EXPECT_EQ(codes[0], "TCMD");
}

TEST(FilterSetCodesTest, FilterByNonexistentTypeReturnsEmpty) {
  auto data = loadSet("SetList_minimal.json");
  auto codes = filterSetCodes(data["data"], "core", "", "", "", "");
  EXPECT_TRUE(codes.empty());
}

// ---------------------------------------------------------------------------
// Date range filter
// ---------------------------------------------------------------------------

TEST(FilterSetCodesTest, FromDateExcludesEarlierSets) {
  auto data = loadSet("SetList_minimal.json");
  // Only TST2 (2025-06-01) is on or after 2025-05-01
  auto codes = filterSetCodes(data["data"], "", "2025-05-01", "", "", "");
  ASSERT_EQ(codes.size(), 1u);
  EXPECT_EQ(codes[0], "TST2");
}

TEST(FilterSetCodesTest, FromDateIsInclusive) {
  auto data = loadSet("SetList_minimal.json");
  // TST2 exactly matches 2025-06-01
  auto codes = filterSetCodes(data["data"], "", "2025-06-01", "", "", "");
  ASSERT_EQ(codes.size(), 1u);
  EXPECT_EQ(codes[0], "TST2");
}

TEST(FilterSetCodesTest, ToDateExcludesLaterSets) {
  auto data = loadSet("SetList_minimal.json");
  // TST (2025-01-01) and TCMD (2025-03-01) are on or before 2025-03-01
  auto codes = filterSetCodes(data["data"], "", "", "2025-03-01", "", "");
  EXPECT_EQ(codes.size(), 2u);
  EXPECT_TRUE(contains(codes, "TST"));
  EXPECT_TRUE(contains(codes, "TCMD"));
  EXPECT_FALSE(contains(codes, "TST2"));
}

TEST(FilterSetCodesTest, ToDateIsInclusive) {
  auto data = loadSet("SetList_minimal.json");
  auto codes = filterSetCodes(data["data"], "", "", "2025-01-01", "", "");
  ASSERT_EQ(codes.size(), 1u);
  EXPECT_EQ(codes[0], "TST");
}

TEST(FilterSetCodesTest, DateRangeNarrowsToSingleSet) {
  auto data = loadSet("SetList_minimal.json");
  // Between 2025-02-01 and 2025-04-01: only TCMD (2025-03-01)
  auto codes =
      filterSetCodes(data["data"], "", "2025-02-01", "2025-04-01", "", "");
  ASSERT_EQ(codes.size(), 1u);
  EXPECT_EQ(codes[0], "TCMD");
}

TEST(FilterSetCodesTest, TypeAndDateRangeCombined) {
  auto data = loadSet("SetList_minimal.json");
  // expansion sets between 2025-01-01 and 2025-03-01: only TST
  auto codes = filterSetCodes(data["data"], "expansion", "2025-01-01",
                              "2025-03-01", "", "");
  ASSERT_EQ(codes.size(), 1u);
  EXPECT_EQ(codes[0], "TST");
}

// ---------------------------------------------------------------------------
// Set-code range filter (fromSet / toSet)
// ---------------------------------------------------------------------------

TEST(FilterSetCodesTest, FromSetResolvesToItsReleaseDate) {
  auto data = loadSet("SetList_minimal.json");
  // fromSet=TST2 → resolved to 2025-06-01 → only TST2 qualifies
  auto codes = filterSetCodes(data["data"], "", "", "", "TST2", "");
  ASSERT_EQ(codes.size(), 1u);
  EXPECT_EQ(codes[0], "TST2");
}

TEST(FilterSetCodesTest, ToSetResolvesToItsReleaseDate) {
  auto data = loadSet("SetList_minimal.json");
  // toSet=TCMD → resolved to 2025-03-01 → TST + TCMD qualify
  auto codes = filterSetCodes(data["data"], "", "", "", "", "TCMD");
  EXPECT_EQ(codes.size(), 2u);
  EXPECT_TRUE(contains(codes, "TST"));
  EXPECT_TRUE(contains(codes, "TCMD"));
  EXPECT_FALSE(contains(codes, "TST2"));
}

TEST(FilterSetCodesTest, SetRangeIncludesBothBounds) {
  auto data = loadSet("SetList_minimal.json");
  // fromSet=TST (2025-01-01) toSet=TST2 (2025-06-01) → all three qualify
  auto codes = filterSetCodes(data["data"], "", "", "", "TST", "TST2");
  EXPECT_EQ(codes.size(), 3u);
}

TEST(FilterSetCodesTest, UnknownFromSetFallsBackToNoLowerBound) {
  auto data = loadSet("SetList_minimal.json");
  // UNKNOWN not found → effectiveFrom stays "" → no lower bound → all pass
  auto codes = filterSetCodes(data["data"], "", "", "", "UNKNOWN", "");
  EXPECT_EQ(codes.size(), 3u);
}

TEST(FilterSetCodesTest, UnknownToSetFallsBackToNoUpperBound) {
  auto data = loadSet("SetList_minimal.json");
  auto codes = filterSetCodes(data["data"], "", "", "", "", "UNKNOWN");
  EXPECT_EQ(codes.size(), 3u);
}
