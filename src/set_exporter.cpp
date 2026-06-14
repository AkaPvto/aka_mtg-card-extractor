#include "set_exporter.hpp"
#include "../third_party/json.hpp"
#include "http_client.hpp"

#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

using json = nlohmann::json;

// Protects stdout/stderr from interleaving when exportSet runs concurrently.
static std::mutex cout_mutex;

auto exportSet(const std::string &setCode, const std::string &outputDir,
               bool pruneEmpty) -> void {
  std::string url = "https://mtgjson.com/api/v5/" + setCode + ".json";
  std::string jsonString = fetchURL(url);
  if (jsonString.empty()) {
    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cerr << "Failed to fetch data for set " << setCode << ".\n";
    return;
  }

  try {
    auto jsonData = json::parse(jsonString);
    if (!jsonData.contains("data") || !jsonData["data"].contains("cards")) {
      std::cerr << "Invalid JSON format for set data.\n";
      return;
    }

    std::string outPath = outputDir + "/" + setCode + "_cards.md";
    if (outputDir.empty() || outputDir == ".") {
      outPath = setCode + "_cards.md";
    }

    std::ofstream outFile(outPath);
    if (!outFile.is_open()) {
      std::cerr << "Failed to open " << outPath << " for writing.\n";
      return;
    }

    std::string setName = jsonData["data"].value("name", setCode);
    std::string releaseDate =
        jsonData["data"].value("releaseDate", "Unknown Release Date");
    std::string setType = jsonData["data"].value("type", "Unknown Type");
    int totalSetSize = jsonData["data"].value("totalSetSize", 0);

    outFile << "# " << setName << " (" << setCode << ")\n\n";
    outFile << "**Release Date**: " << releaseDate << "\n";
    outFile << "**Set Type**: " << setType << "\n";
    outFile << "**Size**: " << totalSetSize << " cards\n\n";
    outFile << "---\n\n";

    std::set<std::string> seenCards;
    int exportedCount = 0;
    int skippedCount = 0;

    for (const auto &card : jsonData["data"]["cards"]) {
      if (card.value("isReprint", false) ||
          card.value("isAlternative", false) || card.value("isPromo", false)) {
        skippedCount++;
        continue;
      }

      std::string name = card.value("name", "Unknown");
      if (seenCards.count(name)) {
        skippedCount++;
        continue;
      }
      seenCards.insert(name);

      outFile << "## " << name << "\n";
      if (card.contains("manaCost"))
        outFile << "**Mana Cost**: " << card["manaCost"].get<std::string>()
                << "\n";
      if (card.contains("type"))
        outFile << "**Type**: " << card["type"].get<std::string>() << "\n";
      if (card.contains("power") && card.contains("toughness"))
        outFile << "**Power/Toughness**: " << card["power"].get<std::string>()
                << "/" << card["toughness"].get<std::string>() << "\n";
      if (card.contains("loyalty"))
        outFile << "**Loyalty**: " << card["loyalty"].get<std::string>()
                << "\n";
      if (card.contains("text")) {
        std::string text = card["text"].get<std::string>();
        // Render as a markdown blockquote: prefix each line with "> ".
        outFile << "**Text**:\n> ";
        for (char c : text)
          outFile << (c == '\n' ? "\n> " : std::string(1, c));
        outFile << "\n";
      }
      outFile << "\n";
      exportedCount++;
    }

    outFile.close();

    if (pruneEmpty && exportedCount == 0) {
      std::filesystem::remove(outPath);
      std::lock_guard<std::mutex> lock(cout_mutex);
      std::cout << "Pruned empty set " << setCode << " (0 unique cards)\n";
    } else {
      std::lock_guard<std::mutex> lock(cout_mutex);
      std::cout << "Successfully exported " << exportedCount << " cards to "
                << outPath << " (Skipped " << skippedCount << ")\n";
    }

  } catch (const json::parse_error &e) {
    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cerr << "JSON parse error: " << e.what() << "\n";
  }
}

auto exportAllSets(const std::string &outputDir, const std::string &targetType,
                   bool pruneEmpty) -> void {
  std::cout << "Fetching SetList.json to determine all available sets...\n";
  std::string jsonString = fetchURL("https://mtgjson.com/api/v5/SetList.json");
  if (jsonString.empty()) {
    std::cerr << "Failed to fetch SetList.json\n";
    return;
  }

  try {
    auto jsonData = json::parse(jsonString);
    if (!jsonData.contains("data"))
      return;

    std::vector<std::string> codes;

    std::string listOutPath = outputDir + "/sets_list.md";
    std::ofstream listFile(listOutPath);
    if (listFile.is_open()) {
      listFile << "# MTG Sets List\n\n";
      listFile << "| Code | Type | Name | Release Date |\n";
      listFile << "|------|------|------|--------------|\n";
    }

    for (const auto &setObj : jsonData["data"]) {
      std::string code = setObj.value("code", "");
      std::string name = setObj.value("name", "");
      std::string rDate = setObj.value("releaseDate", "");
      std::string type = setObj.value("type", "");

      if (!targetType.empty() && type != targetType)
        continue;

      if (!code.empty()) {
        codes.push_back(code);
        if (listFile.is_open())
          listFile << "| " << code << " | " << type << " | " << name << " | "
                   << rDate << " |\n";
      }
    }
    if (listFile.is_open())
      listFile.close();

    if (codes.empty()) {
      std::cout << "No sets found matching criteria.\n";
      return;
    }

    std::atomic<size_t> currentIndex(0);
    const size_t numThreads = 12; // Tune this to control API request concurrency.
    std::vector<std::thread> workers;
    // Each worker atomically claims the next set index, so all threads stay
    // busy without duplicating work.

    std::cout << "Starting concurrent extraction of " << codes.size()
              << " sets using " << numThreads << " threads...\n";

    for (size_t i = 0; i < numThreads; ++i) {
      workers.emplace_back([&]() {
        while (true) {
          size_t idx = currentIndex.fetch_add(1);
          if (idx >= codes.size())
            break;
          exportSet(codes[idx], outputDir, pruneEmpty);
        }
      });
    }

    for (auto &worker : workers)
      worker.join();

    std::cout << "Successfully exported " << codes.size() << " sets to "
              << outputDir << "\n";
  } catch (const json::parse_error &e) {
    std::cerr << "JSON parse error for SetList: " << e.what() << "\n";
  }
}

auto exportLastSet(const std::string &outputDir, const std::string &targetType,
                   bool pruneEmpty) -> void {
  std::cout << "Fetching SetList.json to determine the most recent set...\n";
  std::string jsonString = fetchURL("https://mtgjson.com/api/v5/SetList.json");
  if (jsonString.empty()) {
    std::cerr << "Failed to fetch SetList.json\n";
    return;
  }

  try {
    auto jsonData = json::parse(jsonString);
    if (!jsonData.contains("data"))
      return;

    std::string latestCode;
    std::string latestDate;
    std::string latestType;

    for (const auto &setObj : jsonData["data"]) {
      std::string code = setObj.value("code", "");
      std::string date = setObj.value("releaseDate", "");
      std::string type = setObj.value("type", "");

      if (!targetType.empty() && type != targetType)
        continue;

      if (!code.empty() && date > latestDate) {
        latestDate = date;
        latestCode = code;
        latestType = type;
      }
    }

    if (!latestCode.empty()) {
      std::cout << "Most recent set found: " << latestCode
                << " (Released: " << latestDate << ", Type: " << latestType
                << ")\n";
      exportSet(latestCode, outputDir, pruneEmpty);
    } else {
      std::cerr << "No valid sets found matching criteria.\n";
    }

  } catch (const json::parse_error &e) {
    std::cerr << "JSON parse error: " << e.what() << "\n";
  }
}
