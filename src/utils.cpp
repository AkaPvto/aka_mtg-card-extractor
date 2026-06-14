#include "utils.hpp"

#include <chrono>
#include <iomanip>
#include <sstream>

auto getTimestampString() -> std::string {
  auto now = std::chrono::system_clock::now();
  std::time_t now_c = std::chrono::system_clock::to_time_t(now);
  std::stringstream ss;
  ss << std::put_time(std::localtime(&now_c), "%Y%m%d_%H%M%S");
  return ss.str();
}
