# Tests

## Running the tests

From the project root using a preset (`debug-clang` or `devel-clang`):

```bash
cmake --preset debug-clang          # configure (first time / after CMakeLists changes)
cmake --build --preset debug-clang  # build
ctest --preset debug-clang          # run all tests
```

To run a specific test suite by name:

```bash
ctest --preset debug-clang -R SerializeSetTest
```

To run the test binary directly (more verbose output):

```bash
./build/debug/mtg_tests
./build/debug/mtg_tests --gtest_filter="SerializeCardTest.*"
```

---

## Test structure

| File | What it covers |
|---|---|
| `test_utils.cpp` | `getTimestampString` — format and length; `parseCommaSeparated` — single, multiple, and edge-case inputs |
| `test_card_utils.cpp` | `shouldSkipCard`, `textToBlockquote`, `buildOutPath` |
| `test_serialization.cpp` | `serializeCardToMarkdown`, `serializeSetToMarkdown`, `findLatestSet` |
| `test_set_filter.cpp` | `filterSetCodes` — type filter, `--fromDate`/`--toDate` (inclusive bounds, narrow ranges, combined with type), `--fromSet`/`--toSet` (resolves to release date, both bounds, unknown set fallback) |
| `test_zip_utils.cpp` | `createZipFromDirectory` — file creation, correct entry count, nested paths, top-level files, invalid source dir, empty directory |

### Supporting directories

- **`mock_data/`** — JSON fixtures that simulate real MTGJSON API responses. Each file is a minimal but realistic payload used as input to the serialization functions.
- **`results/`** — Expected markdown output files. Tests that verify the full serialization pipeline read the corresponding mock JSON, run it through `serializeSetToMarkdown`, and compare the result byte-for-byte against these files.

---

## Adding a new test

### Unit test for a pure function

Add a `TEST(SuiteName, TestName)` block to the relevant file. No extra wiring needed — GTest discovers tests automatically.

```cpp
TEST(MyNewSuite, DoesWhatItShould) {
  EXPECT_EQ(myFunction("input"), "expected output");
}
```

### Serialization test with mock data

1. **Add a mock JSON file** under `mock_data/` that represents the API response you want to test. Follow the MTGJSON v5 envelope format: `{ "data": { ... } }` for single sets or `{ "data": [ ... ] }` for set lists.

2. **Add an expected output file** under `results/` with the exact markdown you expect `serializeSetToMarkdown` to produce for that input.

3. **Write the test** in `test_serialization.cpp` using the `TEST_DATA_DIR` macro, which resolves to the absolute path of this `tests/` directory at compile time:

```cpp
TEST(SerializeSetTest, MyNewSet) {
  std::string rawJson = readFile(std::string(TEST_DATA_DIR) + "/mock_data/MY_set.json");
  auto jsonData = json::parse(rawJson);
  auto result = serializeSetToMarkdown(jsonData["data"], "MY");

  std::string expected = readFile(std::string(TEST_DATA_DIR) + "/results/MY_set_expected.md");
  EXPECT_EQ(result.content, expected);
}
```

> **Tip:** If you are unsure what the expected output should look like, run the binary against the mock JSON locally first and inspect the generated `.md` file.

### Testing a new extracted helper

If you add a new pure function to `card_utils.cpp`, declare it in `card_utils.hpp` and add tests for it in `test_card_utils.cpp` or `test_serialization.cpp` depending on whether it is a low-level utility or part of the serialization pipeline.

If you add a new filter or selection criterion to `filterSetCodes`, add cases to `test_set_filter.cpp` using the existing `SetList_minimal.json` mock (or extend it with the new set types/dates you need).

Functions that make HTTP calls (`fetchURL`, `exportSet`, etc.) cannot be unit tested without a mocking layer and are intentionally left out of scope.
