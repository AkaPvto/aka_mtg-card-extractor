cmake_minimum_required(VERSION 3.14)
project(mtg_extractor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)

# ---------------------------------------------------------------------------
# Main executable
# ---------------------------------------------------------------------------

set(SOURCES
    src/main.cpp
    src/http_client.cpp
    src/set_lister.cpp
    src/set_exporter.cpp
    src/card_utils.cpp
    src/utils.cpp
)

add_executable(mtg_extractor ${SOURCES})

target_include_directories(mtg_extractor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

target_link_libraries(mtg_extractor CURL::libcurl)

add_custom_command(TARGET mtg_extractor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
    $<TARGET_FILE:mtg_extractor>
    ${CMAKE_CURRENT_SOURCE_DIR}/mtg_extractor
)

include(GNUInstallDirs)
install(TARGETS mtg_extractor DESTINATION ${CMAKE_INSTALL_BINDIR})

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(mtg_tests
    tests/test_utils.cpp
    tests/test_card_utils.cpp
    tests/test_serialization.cpp
    src/card_utils.cpp
    src/utils.cpp
)

target_include_directories(mtg_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

target_compile_definitions(mtg_tests PRIVATE
    "TEST_DATA_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/tests\""
)

target_link_libraries(mtg_tests GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(mtg_tests)
