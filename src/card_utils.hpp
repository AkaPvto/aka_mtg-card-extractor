#pragma once

#include "../third_party/json.hpp"

#include <string>

using json = nlohmann::json;

struct SetMarkdown {
  std::string content;
  int exportedCount;
  int skippedCount;
};

struct SetInfo {
  std::string code;
  std::string releaseDate;
  std::string type;
};

// Returns true if the card should be excluded from the export (reprint,
// alternate art, or promo).
auto shouldSkipCard(const json &card) -> bool;

// Converts newlines in card rules text into markdown blockquote line breaks.
auto textToBlockquote(const std::string &text) -> std::string;

// Builds the output .md file path for a given set code and output directory.
auto buildOutPath(const std::string &outputDir, const std::string &setCode)
    -> std::string;

// Serializes a single card JSON object into its markdown section.
auto serializeCardToMarkdown(const json &card) -> std::string;

// Serializes a full set's data node into markdown, filtering and deduplicating
// cards. Returns the rendered content along with exported and skipped counts.
auto serializeSetToMarkdown(const json &setData, const std::string &setCode)
    -> SetMarkdown;

// Finds the most recently released set in a SetList data array, optionally
// filtered by type. Returns an empty SetInfo if nothing matches.
auto findLatestSet(const json &setsArray, const std::string &targetType)
    -> SetInfo;
