#include "card_utils.hpp"

#include <gtest/gtest.h>

#include <fstream>
#include <sstream>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static auto readFile(const std::string &path) -> std::string {
  std::ifstream f(path);
  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

static auto mockDir() -> std::string {
  return std::string(TEST_DATA_DIR) + "/mock_data/";
}

static auto resultsDir() -> std::string {
  return std::string(TEST_DATA_DIR) + "/results/";
}

static auto loadCard(const std::string &filename) -> json {
  return json::parse(readFile(mockDir() + filename));
}

static auto loadSet(const std::string &filename) -> json {
  return json::parse(readFile(mockDir() + filename));
}

static auto expected(const std::string &filename) -> std::string {
  return readFile(resultsDir() + filename);
}

// ---------------------------------------------------------------------------
// serializeCardToMarkdown
// ---------------------------------------------------------------------------

TEST(SerializeCardTest, CreatureCard) {
  EXPECT_EQ(serializeCardToMarkdown(loadCard("card_creature.json")),
            expected("card_creature_expected.md"));
}

TEST(SerializeCardTest, PlaneswalkerCard) {
  EXPECT_EQ(serializeCardToMarkdown(loadCard("card_planeswalker.json")),
            expected("card_planeswalker_expected.md"));
}

TEST(SerializeCardTest, MinimalCard) {
  EXPECT_EQ(serializeCardToMarkdown(loadCard("card_minimal.json")),
            expected("card_minimal_expected.md"));
}

TEST(SerializeCardTest, MultilineTextCard) {
  EXPECT_EQ(serializeCardToMarkdown(loadCard("card_multiline_text.json")),
            expected("card_multiline_text_expected.md"));
}

// ---------------------------------------------------------------------------
// serializeSetToMarkdown
// ---------------------------------------------------------------------------

TEST(SerializeSetTest, FiltersReprintsPromosAndDuplicates) {
  auto data = loadSet("set_mixed_cards.json");
  auto result = serializeSetToMarkdown(data["data"], "MIX");

  EXPECT_EQ(result.exportedCount, 2); // Normal Card + Another Card
  EXPECT_EQ(result.skippedCount, 3);  // reprint + promo + duplicate name
}

TEST(SerializeSetTest, EmptySetMatchesExpected) {
  auto data = loadSet("set_empty.json");
  auto result = serializeSetToMarkdown(data["data"], "EMP");

  EXPECT_EQ(result.exportedCount, 0);
  EXPECT_EQ(result.skippedCount, 0);
  EXPECT_EQ(result.content, expected("set_empty_expected.md"));
}

TEST(SerializeSetTest, FullSetMatchesExpected) {
  auto data = loadSet("TST_set.json");
  auto result = serializeSetToMarkdown(data["data"], "TST");

  EXPECT_EQ(result.exportedCount, 3);
  EXPECT_EQ(result.skippedCount, 2);
  EXPECT_EQ(result.content, expected("TST_set_expected.md"));
}

// ---------------------------------------------------------------------------
// findLatestSet
// ---------------------------------------------------------------------------

TEST(FindLatestSetTest, ReturnsLatestByDate) {
  auto data = loadSet("SetList_minimal.json");
  auto latest = findLatestSet(data["data"], "");

  EXPECT_EQ(latest.code, "TST2");
  EXPECT_EQ(latest.releaseDate, "2025-06-01");
}

TEST(FindLatestSetTest, FiltersByType) {
  auto data = loadSet("SetList_minimal.json");
  auto latest = findLatestSet(data["data"], "commander");

  EXPECT_EQ(latest.code, "TCMD");
  EXPECT_EQ(latest.type, "commander");
}

TEST(FindLatestSetTest, ReturnsEmptyForNonMatchingType) {
  auto data = loadSet("SetList_minimal.json");
  EXPECT_TRUE(findLatestSet(data["data"], "core").code.empty());
}

TEST(FindLatestSetTest, ReturnsEmptyForEmptyArray) {
  EXPECT_TRUE(findLatestSet(json::array(), "").code.empty());
}
