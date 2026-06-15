#pragma once

#include <string>

// Recursively compresses sourceDir into a zip archive at zipPath.
// Returns true on success.
auto createZipFromDirectory(const std::string &sourceDir,
                            const std::string &zipPath) -> bool;
