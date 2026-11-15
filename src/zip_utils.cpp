#include "zip_utils.hpp"

#include <cerrno>
#include <filesystem>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

namespace fs = std::filesystem;

auto createZipFromDirectory(const std::string &sourceDir,
                            const std::string &zipPath) -> bool {
  if (!fs::exists(sourceDir) || !fs::is_directory(sourceDir)) {
    std::cerr << "Source directory does not exist: " << sourceDir << "\n";
    return false;
  }

  const fs::path absoluteSource = fs::absolute(sourceDir).lexically_normal();
  const fs::path absoluteZip = fs::absolute(zipPath).lexically_normal();
  const fs::path basePath = absoluteSource.parent_path();
  const std::string sourceName = "./" + absoluteSource.filename().string();

  bool hasFiles = false;
  for (const auto &entry : fs::recursive_directory_iterator(absoluteSource)) {
    if (entry.is_regular_file()) {
      hasFiles = true;
      break;
    }
  }

  std::error_code removeError;
  fs::remove(absoluteZip, removeError);
  if (removeError) {
    std::cerr << "Failed to replace zip archive: " << absoluteZip << "\n";
    return false;
  }
  if (!hasFiles)
    return true;

  const pid_t child = fork();
  if (child < 0) {
    std::cerr << "Failed to start zip process\n";
    return false;
  }

  if (child == 0) {
    if (chdir(basePath.c_str()) != 0) {
      std::cerr << "Failed to access source directory parent\n";
      _exit(126);
    }

    execlp("zip", "zip", "-r", "-q", "-D", absoluteZip.c_str(),
           sourceName.c_str(), static_cast<char *>(nullptr));
    std::cerr << "Failed to execute zip\n";
    _exit(127);
  }

  int status = 0;
  while (waitpid(child, &status, 0) < 0) {
    if (errno == EINTR)
      continue;
    std::cerr << "Failed waiting for zip process\n";
    return false;
  }

  return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}
