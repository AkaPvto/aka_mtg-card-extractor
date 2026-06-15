#pragma once

#include <string>
#include <vector>

auto getTimestampString() -> std::string;
auto parseCommaSeparated(const std::string &input) -> std::vector<std::string>;
