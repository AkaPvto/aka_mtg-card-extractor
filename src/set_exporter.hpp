#pragma once
#include <string>

void exportSet(const std::string& setCode, const std::string& outputDir, bool pruneEmpty);
void exportAllSets(const std::string& outputDir, const std::string& targetType, bool pruneEmpty);
void exportLastSet(const std::string& outputDir, const std::string& targetType, bool pruneEmpty);
