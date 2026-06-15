#include "set_exporter.hpp"
#include "../third_party/json.hpp"
#include "card_utils.hpp"
#include "http_client.hpp"

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
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

    std::string outPath = buildOutPath(outputDir, setCode);
    std::ofstream outFile(outPath);
    if (!outFile.is_open()) {
      std::cerr << "Failed to open " << outPath << " for writing.\n";
      return;
    }

    auto result = serializeSetToMarkdown(jsonData["data"], setCode);
    outFile << result.content;
    outFile.close();

    if (pruneEmpty && result.exportedCount == 0) {
      std::filesystem::remove(outPath);
      std::lock_guard<std::mutex> lock(cout_mutex);
      std::cout << "Pruned empty set " << setCode << " (0 unique cards)\n";
    } else {
      std::lock_guard<std::mutex> lock(cout_mutex);
      std::cout << "Successfully exported " << result.exportedCount
                << " cards to " << outPath << " (Skipped "
                << result.skippedCount << ")\n";
    }

  } catch (const json::parse_error &e) {
    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cerr << "JSON parse error: " << e.what() << "\n";
  }
}

auto exportSets(const std::vector<std::string> &setCodes,
                const std::string &outputDir, bool pruneEmpty) -> void {
  std::atomic<size_t> currentIndex(0);
  const size_t numThreads = std::min<size_t>(12, setCodes.size());
  std::vector<std::thread> workers;

  std::cout << "Exporting " << setCodes.size() << " sets using " << numThreads
            << " threads...\n";

  for (size_t i = 0; i < numThreads; ++i) {
    workers.emplace_back([&]() {
      while (true) {
        size_t idx = currentIndex.fetch_add(1);
        if (idx >= setCodes.size())
          break;
        exportSet(setCodes[idx], outputDir, pruneEmpty);
      }
    });
  }

  for (auto &worker : workers)
    worker.join();
}

auto exportAllSets(const std::string &outputDir, const std::string &targetType,
                   bool pruneEmpty, const std::string &fromDate,
                   const std::string &toDate, const std::string &fromSet,
                   const std::string &toSet) -> void {
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

    // Warn if fromSet/toSet codes are not found before filtering.
    if (!fromSet.empty() || !toSet.empty()) {
      bool foundFrom = fromSet.empty(), foundTo = toSet.empty();
      for (const auto &setObj : jsonData["data"]) {
        std::string code = setObj.value("code", "");
        if (code == fromSet)
          foundFrom = true;
        if (code == toSet)
          foundTo = true;
      }
      if (!foundFrom)
        std::cerr << "Warning: set code '" << fromSet
                  << "' not found in SetList, ignoring --fromSet\n";
      if (!foundTo)
        std::cerr << "Warning: set code '" << toSet
                  << "' not found in SetList, ignoring --toSet\n";
    }

    auto codes = filterSetCodes(jsonData["data"], targetType, fromDate, toDate,
                                fromSet, toSet);

    std::string listOutPath = outputDir + "/sets_list.md";
    std::ofstream listFile(listOutPath);
    if (listFile.is_open()) {
      listFile << "# MTG Sets List\n\n";
      listFile << "| Code | Type | Name | Release Date |\n";
      listFile << "|------|------|------|--------------|\n";
      for (const auto &setObj : jsonData["data"]) {
        std::string code = setObj.value("code", "");
        if (std::find(codes.begin(), codes.end(), code) != codes.end()) {
          listFile << "| " << code << " | " << setObj.value("type", "") << " | "
                   << setObj.value("name", "") << " | "
                   << setObj.value("releaseDate", "") << " |\n";
        }
      }
      listFile.close();
    }

    if (codes.empty()) {
      std::cout << "No sets found matching criteria.\n";
      return;
    }

    std::atomic<size_t> currentIndex(0);
    const size_t numThreads = 12;
    std::vector<std::thread> workers;

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

    auto latest = findLatestSet(jsonData["data"], targetType);
    if (!latest.code.empty()) {
      std::cout << "Most recent set found: " << latest.code
                << " (Released: " << latest.releaseDate
                << ", Type: " << latest.type << ")\n";
      exportSet(latest.code, outputDir, pruneEmpty);
    } else {
      std::cerr << "No valid sets found matching criteria.\n";
    }

  } catch (const json::parse_error &e) {
    std::cerr << "JSON parse error: " << e.what() << "\n";
  }
}
