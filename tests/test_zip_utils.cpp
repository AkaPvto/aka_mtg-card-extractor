#include "zip_utils.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <zip.h>

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

  auto countZipEntries(const fs::path &path) -> zip_int64_t {
    int err = 0;
    zip_t *archive = zip_open(path.string().c_str(), ZIP_RDONLY, &err);
    if (!archive)
      return -1;
    zip_int64_t count = zip_get_num_entries(archive, 0);
    zip_close(archive);
    return count;
  }

  auto zipContainsFile(const fs::path &path, const std::string &name) -> bool {
    int err = 0;
    zip_t *archive = zip_open(path.string().c_str(), ZIP_RDONLY, &err);
    if (!archive)
      return false;
    bool found = zip_name_locate(archive, name.c_str(), 0) >= 0;
    zip_close(archive);
    return found;
  }
};

TEST_F(ZipUtilsTest, CreatesZipFile) {
  ASSERT_TRUE(createZipFromDirectory(sourceDir.string(), zipPath.string()));
  EXPECT_TRUE(fs::exists(zipPath));
}

TEST_F(ZipUtilsTest, ZipContainsExpectedNumberOfFiles) {
  ASSERT_TRUE(createZipFromDirectory(sourceDir.string(), zipPath.string()));
  // file1.md + file2.md + subdir/nested.md = 3 entries
  EXPECT_EQ(countZipEntries(zipPath), 3);
}

TEST_F(ZipUtilsTest, ZipContainsNestedFile) {
  ASSERT_TRUE(createZipFromDirectory(sourceDir.string(), zipPath.string()));
  std::string dirName = sourceDir.filename().string();
  EXPECT_TRUE(zipContainsFile(zipPath, dirName + "/subdir/nested.md"));
}

TEST_F(ZipUtilsTest, ZipContainsTopLevelFiles) {
  ASSERT_TRUE(createZipFromDirectory(sourceDir.string(), zipPath.string()));
  std::string dirName = sourceDir.filename().string();
  EXPECT_TRUE(zipContainsFile(zipPath, dirName + "/file1.md"));
  EXPECT_TRUE(zipContainsFile(zipPath, dirName + "/file2.md"));
}

TEST_F(ZipUtilsTest, InvalidSourceDirectoryReturnsFalse) {
  EXPECT_FALSE(
      createZipFromDirectory("/nonexistent/path/xyz", zipPath.string()));
  EXPECT_FALSE(fs::exists(zipPath));
}

TEST_F(ZipUtilsTest, EmptyDirectorySucceedsWithoutCreatingFile) {
  // libzip removes the zip file when no entries are added — this is documented
  // behavior. We verify the call succeeds (no error) without asserting the file
  // exists.
  fs::path emptyDir = fs::temp_directory_path() / "mtg_zip_empty";
  fs::path emptyZip = fs::temp_directory_path() / "mtg_zip_empty.zip";
  fs::create_directories(emptyDir);

  EXPECT_TRUE(createZipFromDirectory(emptyDir.string(), emptyZip.string()));

  fs::remove_all(emptyDir);
  fs::remove(emptyZip);
}
