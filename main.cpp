#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>
#include <mutex>
#include <atomic>
#include <curl/curl.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <curl/curl.h>
#include "json.hpp"

using json = nlohmann::json;

std::string getTimestampString() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_c), "%Y%m%d_%H%M%S");
    return ss.str();
}

// libcurl write callback
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string fetchURL(const std::string& url) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        // Follow redirects just in case
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        
        // Some systems might have SSL certification issues, ignoring for a simple CLI if needed, 
        // but default is fine usually.
        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }
        curl_easy_cleanup(curl);
    }
    return readBuffer;
}

void listSets(const std::string& targetType) {
    std::cout << "Fetching available expansion sets from MTGJSON...\n";
    std::string jsonString = fetchURL("https://mtgjson.com/api/v5/SetList.json");
    if(jsonString.empty()) {
        std::cerr << "Failed to fetch SetList.json\n";
        return;
    }

    try {
        auto jsonData = json::parse(jsonString);
        if(!jsonData.contains("data")) return;

        std::cout << "Code\t| Release Date | Type         | Name\n";
        std::cout << "--------------------------------------------------------\n";
        for(const auto& setObj : jsonData["data"]) {
            std::string code = setObj.value("code", "N/A");
            std::string name = setObj.value("name", "N/A");
            std::string releaseDate = setObj.value("releaseDate", "N/A");
            std::string type = setObj.value("type", "N/A");

            if (!targetType.empty() && type != targetType) {
                continue;
            }

            // Align type column padding roughly
            std::string typePad = type;
            if (typePad.length() < 12) typePad.append(12 - typePad.length(), ' ');

            std::cout << code << "\t| " << releaseDate << "   | " << typePad << " | " << name << "\n";
        }
    } catch(const json::parse_error& e) {
        std::cerr << "JSON parse error: " << e.what() << "\n";
    }
}

std::mutex cout_mutex;

void exportSet(const std::string& setCode, const std::string& outputDir, bool pruneEmpty) {
    std::string url = "https://mtgjson.com/api/v5/" + setCode + ".json";
    
    // Disable noisy print
    // std::cout << "Fetching data for set: " << setCode << " from " << url << "\n";
    
    std::string jsonString = fetchURL(url);
    if(jsonString.empty()) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cerr << "Failed to fetch data for set " << setCode << ".\n";
        return;
    }

    try {
        auto jsonData = json::parse(jsonString);
        if(!jsonData.contains("data") || !jsonData["data"].contains("cards")) {
            std::cerr << "Invalid JSON format for set data.\n";
            return;
        }

        std::string outPath = outputDir + "/" + setCode + "_cards.md";
        if(outputDir.empty() || outputDir == ".") {
            outPath = setCode + "_cards.md";
        }

        std::ofstream outFile(outPath);
        if(!outFile.is_open()) {
            std::cerr << "Failed to open " << outPath << " for writing.\n";
            return;
        }

        std::string setName = jsonData["data"].value("name", setCode);
        std::string releaseDate = jsonData["data"].value("releaseDate", "Unknown Release Date");
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

        for(const auto& card : jsonData["data"]["cards"]) {
            // Filter: no reprints or alters, unique functional cards
            if(card.value("isReprint", false)) {
                skippedCount++;
                continue;
            }
            if(card.value("isAlternative", false)) {
                skippedCount++;
                continue;
            }
            if(card.value("isPromo", false)) {
                skippedCount++;
                continue;
            }

            std::string name = card.value("name", "Unknown");
            if(seenCards.count(name)) {
                skippedCount++;
                continue;
            }
            seenCards.insert(name);

            // Write entry
            outFile << "## " << name << "\n";
            if(card.contains("manaCost")) {
                outFile << "**Mana Cost**: " << card["manaCost"].get<std::string>() << "\n";
            }
            if(card.contains("type")) {
                outFile << "**Type**: " << card["type"].get<std::string>() << "\n";
            }
            if(card.contains("power") && card.contains("toughness")) {
                outFile << "**Power/Toughness**: " << card["power"].get<std::string>() << "/" << card["toughness"].get<std::string>() << "\n";
            }
            if(card.contains("loyalty")) {
                outFile << "**Loyalty**: " << card["loyalty"].get<std::string>() << "\n";
            }
            if(card.contains("text")) {
                std::string text = card["text"].get<std::string>();
                // Replace newlines in text with markdown blockquotes or just newlines
                outFile << "**Text**:\n";
                // Simple replacement of '\n' with '\n> ' for blockquote style
                size_t pos = 0;
                outFile << "> ";
                while(pos < text.length()) {
                    if(text[pos] == '\n') {
                        outFile << "\n> ";
                    } else {
                        outFile << text[pos];
                    }
                    pos++;
                }
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
            std::cout << "Successfully exported " << exportedCount << " cards to " << outPath 
                      << " (Skipped " << skippedCount << ")\n";
        }

    } catch(const json::parse_error& e) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cerr << "JSON parse error: " << e.what() << "\n";
    }
}

void exportAllSets(const std::string& outputDir, const std::string& targetType, bool pruneEmpty) {
    std::cout << "Fetching SetList.json to determine all available sets...\n";
    std::string jsonString = fetchURL("https://mtgjson.com/api/v5/SetList.json");
    if(jsonString.empty()) {
        std::cerr << "Failed to fetch SetList.json\n";
        return;
    }

    try {
        auto jsonData = json::parse(jsonString);
        if(!jsonData.contains("data")) return;

        std::vector<std::string> codes;
        
        std::string listOutPath = outputDir + "/sets_list.md";
        std::ofstream listFile(listOutPath);
        if (listFile.is_open()) {
            listFile << "# MTG Sets List\n\n";
            listFile << "| Code | Type | Name | Release Date |\n";
            listFile << "|------|------|------|--------------|\n";
        }

        for(const auto& setObj : jsonData["data"]) {
            std::string code = setObj.value("code", "");
            std::string name = setObj.value("name", "");
            std::string rDate = setObj.value("releaseDate", "");
            std::string type = setObj.value("type", "");

            if (!targetType.empty() && type != targetType) {
                continue;
            }

            if(!code.empty()) {
                codes.push_back(code);
                if(listFile.is_open()) {
                    listFile << "| " << code << " | " << type << " | " << name << " | " << rDate << " |\n";
                }
            }
        }
        if(listFile.is_open()) listFile.close();

        if (codes.empty()) {
            std::cout << "No sets found matching criteria.\n";
            return;
        }

        std::atomic<size_t> currentIndex(0);
        const size_t numThreads = 12; // Adjust concurrency level here
        std::vector<std::thread> workers;

        std::cout << "Starting concurrent extraction of " << codes.size() << " sets using " << numThreads << " threads...\n";

        for (size_t i = 0; i < numThreads; ++i) {
            workers.emplace_back([&]() {
                while (true) {
                    size_t idx = currentIndex.fetch_add(1);
                    if (idx >= codes.size()) break;
                    exportSet(codes[idx], outputDir, pruneEmpty);
                }
            });
        }

        for (auto& worker : workers) {
            worker.join();
        }

        std::cout << "Successfully exported " << codes.size() << " sets to " << outputDir << "\n";
    } catch(const json::parse_error& e) {
        std::cerr << "JSON parse error for SetList: " << e.what() << "\n";
    }
}

void exportLastSet(const std::string& outputDir, const std::string& targetType, bool pruneEmpty) {
    std::cout << "Fetching SetList.json to determine the most recent set...\n";
    std::string jsonString = fetchURL("https://mtgjson.com/api/v5/SetList.json");
    if(jsonString.empty()) {
        std::cerr << "Failed to fetch SetList.json\n";
        return;
    }

    try {
        auto jsonData = json::parse(jsonString);
        if(!jsonData.contains("data")) return;

        std::string latestCode = "";
        std::string latestDate = "";
        std::string latestType = "";

        for(const auto& setObj : jsonData["data"]) {
            std::string code = setObj.value("code", "");
            std::string date = setObj.value("releaseDate", "");
            std::string type = setObj.value("type", "");

            if (!targetType.empty() && type != targetType) {
                continue;
            }

            if(!code.empty() && date > latestDate) {
                latestDate = date;
                latestCode = code;
                latestType = type;
            }
        }

        if(!latestCode.empty()) {
            std::cout << "Most recent set found: " << latestCode << " (Released: " << latestDate << ", Type: " << latestType << ")\n";
            exportSet(latestCode, outputDir, pruneEmpty);
        } else {
            std::cerr << "No valid sets found matching criteria.\n";
        }

    } catch(const json::parse_error& e) {
        std::cerr << "JSON parse error: " << e.what() << "\n";
    }
}

void printHelp() {
    std::cout << "MTGJSON Extractor CLI\n"
              << "Actions (Pick one):\n"
              << "  --list                        Shows available MTG expansion sets\n"
              << "  --all                         Exports unique cards from ALL sets to markdown\n"
              << "  --last                        Exports ONLY the most recently released set\n"
              << "  --set <CODE>                  Exports unique cards from ONE set (e.g. ONE, ARN)\n"
              << "Options:\n"
              << "  --setType <TYPE>              Filters parsing to only parse sets matching TYPE (e.g. expansion, commander, core)\n"
              << "  --outDir <DIR>                Specifies base output directory (Default: extraction)\n"
              << "  --prune                       Automatically deletes markdown files if 0 cards were exported\n"
              << "Example:\n"
              << "  mtg_extractor --all --setType expansion --outDir ./my_main_sets\n"
              << "  mtg_extractor --list --setType core\n";
}

int main(int argc, char* argv[]) {
    if(argc < 2) {
        printHelp();
        return 1;
    }

    std::string action = "";
    std::string targetSet = "";
    std::string setType = "";
    std::string outDir = "extraction";
    bool pruneEmpty = false;

    // Basic flag parser
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--list") {
            action = "list";
        } else if (arg == "--all") {
            action = "all";
        } else if (arg == "--last") {
            action = "last";
        } else if (arg == "--prune") {
            pruneEmpty = true;
        } else if (arg == "--set" && i + 1 < argc) {
            action = "set";
            targetSet = argv[++i];
        } else if (arg == "--setType" && i + 1 < argc) {
            setType = argv[++i];
        } else if (arg == "--outDir" && i + 1 < argc) {
            outDir = argv[++i];
        } else {
            std::cerr << "Unknown or incomplete argument: " << arg << "\n";
            printHelp();
            return 1;
        }
    }

    if (action.empty()) {
        std::cerr << "Error: No primary action specified (e.g. --all, --list).\n";
        printHelp();
        return 1;
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);

    if (action == "list") {
        listSets(setType);
    } else if (action == "set") {
        // Create full outpath if executing an export
        std::string finalDir = outDir + "/" + getTimestampString();
        std::filesystem::create_directories(finalDir);
        exportSet(targetSet, finalDir, pruneEmpty);
    } else if (action == "all") {
        std::string finalDir = outDir + "/" + getTimestampString();
        std::filesystem::create_directories(finalDir);
        exportAllSets(finalDir, setType, pruneEmpty);
    } else if (action == "last") {
        std::string finalDir = outDir + "/" + getTimestampString();
        std::filesystem::create_directories(finalDir);
        exportLastSet(finalDir, setType, pruneEmpty);
    }

    curl_global_cleanup();
    return 0;
}
