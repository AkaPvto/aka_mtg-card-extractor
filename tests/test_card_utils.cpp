#include "card_utils.hpp"

#include <gtest/gtest.h>

// ---------------------------------------------------------------------------
// shouldSkipCard
// ---------------------------------------------------------------------------

TEST(ShouldSkipCardTest, SkipsReprint) {
  json card = {{"isReprint", true}};
  EXPECT_TRUE(shouldSkipCard(card));
}

TEST(ShouldSkipCardTest, SkipsAlternative) {
  json card = {{"isAlternative", true}};
  EXPECT_TRUE(shouldSkipCard(card));
}

TEST(ShouldSkipCardTest, SkipsPromo) {
  json card = {{"isPromo", true}};
  EXPECT_TRUE(shouldSkipCard(card));
}

TEST(ShouldSkipCardTest, DoesNotSkipNormalCard) {
  json card = {{"name", "Lightning Bolt"}, {"isReprint", false}};
  EXPECT_FALSE(shouldSkipCard(card));
}

TEST(ShouldSkipCardTest, DoesNotSkipCardWithNoFlags) {
  json card = {{"name", "Lightning Bolt"}};
  EXPECT_FALSE(shouldSkipCard(card));
}

// ---------------------------------------------------------------------------
// textToBlockquote
// ---------------------------------------------------------------------------

TEST(TextToBlockquoteTest, SingleLine) {
  EXPECT_EQ(textToBlockquote("Flying"), "> Flying");
}

TEST(TextToBlockquoteTest, MultiLine) {
  EXPECT_EQ(textToBlockquote("Flying\nVigilance"), "> Flying\n> Vigilance");
}

TEST(TextToBlockquoteTest, EmptyString) {
  EXPECT_EQ(textToBlockquote(""), "> ");
}

TEST(TextToBlockquoteTest, TrailingNewline) {
  EXPECT_EQ(textToBlockquote("Flying\n"), "> Flying\n> ");
}

// ---------------------------------------------------------------------------
// buildOutPath
// ---------------------------------------------------------------------------

TEST(BuildOutPathTest, NormalDir) {
  EXPECT_EQ(buildOutPath("extraction/20260101", "ARN"),
            "extraction/20260101/ARN_cards.md");
}

TEST(BuildOutPathTest, EmptyDir) {
  EXPECT_EQ(buildOutPath("", "ARN"), "ARN_cards.md");
}

TEST(BuildOutPathTest, DotDir) {
  EXPECT_EQ(buildOutPath(".", "ARN"), "ARN_cards.md");
}
