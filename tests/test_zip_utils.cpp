#include "zip_utils.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

class ZipUtilsTest : public ::testing::Test {
protected:
  fs::path sourceDir;
  fs::path zipPath;

  void SetUp() override {
    sourceDir = fs::temp_directory_path() / "mtg_zip_test_src";
    zipPath = fs::temp_directory_path() / "mtg_zip_test.zip";
    fs::create_directories(sourceDir / "subdir");
    std::ofstream(sourceDir / "file1.md") << "# Card A\n";
    std::ofstream(sourceDir / "file2.md") << "# Card B\n";
    std::ofstream(sourceDir / "subdir" / "nested.md") << "# Nested\n";
  }

  void TearDown() override {
    fs::remove_all(sourceDir);
    fs::remove(zipPath);
  }

  auto shellQuote(const std::string &value) -> std::string {
    std::string result = "'";
    for (char ch : value) {
      if (ch == '\'')
        result += "'\\''";
      else
        result += ch;
    }
    return result + "'";
  }

  auto listZipEntries(const fs::path &path) -> std::vector<std::string> {
    const fs::path listingPath = path.string() + ".entries";
    const std::string command = "unzip -Z1 " + shellQuote(path.string()) +
                                " > " + shellQuote(listingPath.string());
    if (std::system(command.c_str()) != 0)
      return {};

    std::vector<std::string> entries;
    std::ifstream listing(listingPath);
    for (std::string entry; std::getline(listing, entry);)
      entries.push_back(entry);
    fs::remove(listingPath);
    return entries;
  }
};

TEST_F(ZipUtilsTest, CreatesZipFile) {
  ASSERT_TRUE(createZipFromDirectory(sourceDir.string(), zipPath.string()));
  EXPECT_TRUE(fs::exists(zipPath));
}

TEST_F(ZipUtilsTest, ZipContainsExpectedNumberOfFiles) {
  ASSERT_TRUE(createZipFromDirectory(sourceDir.string(), zipPath.string()));
  // file1.md + file2.md + subdir/nested.md = 3 entries
  EXPECT_EQ(listZipEntries(zipPath).size(), 3);
}

TEST_F(ZipUtilsTest, ZipContainsNestedFile) {
  ASSERT_TRUE(createZipFromDirectory(sourceDir.string(), zipPath.string()));
  std::string dirName = sourceDir.filename().string();
  const auto entries = listZipEntries(zipPath);
  EXPECT_NE(
      std::find(entries.begin(), entries.end(), dirName + "/subdir/nested.md"),
      entries.end());
}

TEST_F(ZipUtilsTest, ZipContainsTopLevelFiles) {
  ASSERT_TRUE(createZipFromDirectory(sourceDir.string(), zipPath.string()));
  std::string dirName = sourceDir.filename().string();
  const auto entries = listZipEntries(zipPath);
  EXPECT_NE(std::find(entries.begin(), entries.end(), dirName + "/file1.md"),
            entries.end());
  EXPECT_NE(std::find(entries.begin(), entries.end(), dirName + "/file2.md"),
            entries.end());
}

TEST_F(ZipUtilsTest, InvalidSourceDirectoryReturnsFalse) {
  EXPECT_FALSE(
      createZipFromDirectory("/nonexistent/path/xyz", zipPath.string()));
  EXPECT_FALSE(fs::exists(zipPath));
}

TEST_F(ZipUtilsTest, EmptyDirectorySucceedsWithoutCreatingFile) {
  fs::path emptyDir = fs::temp_directory_path() / "mtg_zip_empty";
  fs::path emptyZip = fs::temp_directory_path() / "mtg_zip_empty.zip";
  fs::create_directories(emptyDir);

  EXPECT_TRUE(createZipFromDirectory(emptyDir.string(), emptyZip.string()));

  fs::remove_all(emptyDir);
  fs::remove(emptyZip);
}
