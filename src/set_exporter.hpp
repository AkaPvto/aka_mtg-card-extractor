#pragma once

#include <string>

auto exportSet(const std::string &setCode, const std::string &outputDir,
               bool pruneEmpty) -> void;
auto exportAllSets(const std::string &outputDir, const std::string &targetType,
                   bool pruneEmpty) -> void;
auto exportLastSet(const std::string &outputDir, const std::string &targetType,
                   bool pruneEmpty) -> void;
