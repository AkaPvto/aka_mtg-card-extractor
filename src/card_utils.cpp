#include "card_utils.hpp"

#include <algorithm>
#include <set>
#include <string>
#include <vector>

auto shouldSkipCard(const json &card) -> bool {
  return card.value("isReprint", false) || card.value("isAlternative", false) ||
         card.value("isPromo", false);
}

auto textToBlockquote(const std::string &text) -> std::string {
  std::string result = "> ";
  for (char c : text)
    result += (c == '\n' ? "\n> " : std::string(1, c));
  return result;
}

auto buildOutPath(const std::string &outputDir, const std::string &setCode)
    -> std::string {
  if (outputDir.empty() || outputDir == ".")
    return setCode + "_cards.md";
  return outputDir + "/" + setCode + "_cards.md";
}

auto serializeCardToMarkdown(const json &card) -> std::string {
  std::string out = "## " + card.value("name", "Unknown") + "\n";
  if (card.contains("manaCost"))
    out += "**Mana Cost**: " + card["manaCost"].get<std::string>() + "\n";
  if (card.contains("type"))
    out += "**Type**: " + card["type"].get<std::string>() + "\n";
  if (card.contains("power") && card.contains("toughness"))
    out += "**Power/Toughness**: " + card["power"].get<std::string>() + "/" +
           card["toughness"].get<std::string>() + "\n";
  if (card.contains("loyalty"))
    out += "**Loyalty**: " + card["loyalty"].get<std::string>() + "\n";
  if (card.contains("text"))
    out += "**Text**:\n" + textToBlockquote(card["text"].get<std::string>()) +
           "\n";
  out += "\n";
  return out;
}

auto serializeSetToMarkdown(const json &setData, const std::string &setCode)
    -> SetMarkdown {
  std::string setName = setData.value("name", setCode);
  std::string releaseDate =
      setData.value("releaseDate", "Unknown Release Date");
  std::string setType = setData.value("type", "Unknown Type");
  int totalSetSize = setData.value("totalSetSize", 0);

  std::string content;
  content += "# " + setName + " (" + setCode + ")\n\n";
  content += "**Release Date**: " + releaseDate + "\n";
  content += "**Set Type**: " + setType + "\n";
  content += "**Size**: " + std::to_string(totalSetSize) + " cards\n\n";
  content += "---\n\n";

  std::set<std::string> seenCards;
  int exportedCount = 0;
  int skippedCount = 0;

  for (const auto &card : setData.value("cards", json::array())) {
    if (shouldSkipCard(card)) {
      skippedCount++;
      continue;
    }
    std::string name = card.value("name", "Unknown");
    if (seenCards.count(name)) {
      skippedCount++;
      continue;
    }
    seenCards.insert(name);
    content += serializeCardToMarkdown(card);
    exportedCount++;
  }

  return {content, exportedCount, skippedCount};
}

auto findLatestSet(const json &setsArray, const std::string &targetType)
    -> SetInfo {
  SetInfo latest;
  for (const auto &setObj : setsArray) {
    std::string code = setObj.value("code", "");
    std::string date = setObj.value("releaseDate", "");
    std::string type = setObj.value("type", "");

    if (!targetType.empty() && type != targetType)
      continue;
    if (!code.empty() && date > latest.releaseDate) {
      latest = {code, date, type};
    }
  }
  return latest;
}

auto filterSetCodes(const json &setsArray, const std::string &targetType,
                    const std::string &fromDate, const std::string &toDate,
                    const std::string &fromSet, const std::string &toSet)
    -> std::vector<std::string> {
  std::string effectiveFrom = fromDate;
  std::string effectiveTo = toDate;

  if (!fromSet.empty() || !toSet.empty()) {
    for (const auto &setObj : setsArray) {
      std::string code = setObj.value("code", "");
      std::string date = setObj.value("releaseDate", "");
      if (!fromSet.empty() && code == fromSet)
        effectiveFrom = date;
      if (!toSet.empty() && code == toSet)
        effectiveTo = date;
    }
  }

  std::vector<std::string> codes;
  for (const auto &setObj : setsArray) {
    std::string code = setObj.value("code", "");
    std::string type = setObj.value("type", "");
    std::string date = setObj.value("releaseDate", "");

    if (!targetType.empty() && type != targetType)
      continue;
    if (!effectiveFrom.empty() && date < effectiveFrom)
      continue;
    if (!effectiveTo.empty() && date > effectiveTo)
      continue;
    if (!code.empty())
      codes.push_back(code);
  }
  return codes;
}

auto sortSetList(json setsArray, const std::string &orderBy) -> json {
  if (orderBy.empty())
    return setsArray;
  std::stable_sort(setsArray.begin(), setsArray.end(),
                   [&](const json &a, const json &b) {
                     return a.value(orderBy, "") < b.value(orderBy, "");
                   });
  return setsArray;
}
