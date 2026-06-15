#include "zip_utils.hpp"

#include <filesystem>
#include <iostream>
#include <zip.h>

namespace fs = std::filesystem;

auto createZipFromDirectory(const std::string &sourceDir,
                            const std::string &zipPath) -> bool {
  if (!fs::exists(sourceDir) || !fs::is_directory(sourceDir)) {
    std::cerr << "Source directory does not exist: " << sourceDir << "\n";
    return false;
  }

  int err = 0;
  zip_t *archive = zip_open(zipPath.c_str(), ZIP_CREATE | ZIP_TRUNCATE, &err);
  if (!archive) {
    zip_error_t zerr;
    zip_error_init_with_code(&zerr, err);
    std::cerr << "Failed to create zip archive: " << zip_error_strerror(&zerr)
              << "\n";
    zip_error_fini(&zerr);
    return false;
  }

  fs::path basePath = fs::path(sourceDir).parent_path();

  for (const auto &entry : fs::recursive_directory_iterator(sourceDir)) {
    if (!entry.is_regular_file())
      continue;

    std::string absolutePath = entry.path().string();
    std::string relativePath = fs::relative(entry.path(), basePath).string();

    zip_source_t *source =
        zip_source_file(archive, absolutePath.c_str(), 0, 0);
    if (!source) {
      std::cerr << "Failed to create source for: " << absolutePath << "\n";
      zip_discard(archive);
      return false;
    }

    if (zip_file_add(archive, relativePath.c_str(), source,
                     ZIP_FL_OVERWRITE) < 0) {
      std::cerr << "Failed to add to zip: " << relativePath << "\n";
      zip_source_free(source);
      zip_discard(archive);
      return false;
    }
  }

  if (zip_close(archive) != 0) {
    std::cerr << "Failed to finalize zip archive\n";
    return false;
  }

  return true;
}
