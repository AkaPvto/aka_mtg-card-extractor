#pragma once

#include <string>
#include <vector>

auto exportSet(const std::string &setCode, const std::string &outputDir,
               bool pruneEmpty) -> void;

auto exportSets(const std::vector<std::string> &setCodes,
                const std::string &outputDir, bool pruneEmpty) -> void;

auto exportAllSets(const std::string &outputDir, const std::string &targetType,
                   bool pruneEmpty, const std::string &fromDate = "",
                   const std::string &toDate = "",
                   const std::string &fromSet = "",
                   const std::string &toSet = "") -> void;

auto exportLastSet(const std::string &outputDir, const std::string &targetType,
                   bool pruneEmpty) -> void;
