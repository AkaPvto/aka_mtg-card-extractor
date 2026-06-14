#include "set_lister.hpp"
#include "http_client.hpp"
#include "../third_party/json.hpp"
#include <iostream>

using json = nlohmann::json;

void listSets(const std::string& targetType) {
    std::cout << "Fetching available expansion sets from MTGJSON...\n";
    std::string jsonString = fetchURL("https://mtgjson.com/api/v5/SetList.json");
    if (jsonString.empty()) {
        std::cerr << "Failed to fetch SetList.json\n";
        return;
    }

    try {
        auto jsonData = json::parse(jsonString);
        if (!jsonData.contains("data")) return;

        std::cout << "Code\t| Release Date | Type         | Name\n";
        std::cout << "--------------------------------------------------------\n";
        for (const auto& setObj : jsonData["data"]) {
            std::string code = setObj.value("code", "N/A");
            std::string name = setObj.value("name", "N/A");
            std::string releaseDate = setObj.value("releaseDate", "N/A");
            std::string type = setObj.value("type", "N/A");

            if (!targetType.empty() && type != targetType) continue;

            std::string typePad = type;
            if (typePad.length() < 12) typePad.append(12 - typePad.length(), ' ');

            std::cout << code << "\t| " << releaseDate << "   | " << typePad << " | " << name << "\n";
        }
    } catch (const json::parse_error& e) {
        std::cerr << "JSON parse error: " << e.what() << "\n";
    }
}
