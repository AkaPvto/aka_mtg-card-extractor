cmake_minimum_required(VERSION 3.10)
project(mtg_extractor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find libcurl
find_package(CURL REQUIRED)

# Include directory for nlohmann/json
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(mtg_extractor main.cpp)

# Link libcurl
target_link_libraries(mtg_extractor CURL::libcurl)

# Create symlink on root directory
add_custom_command(TARGET mtg_extractor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
    $<TARGET_FILE:mtg_extractor>
    ${CMAKE_CURRENT_SOURCE_DIR}/mtg_extractor
)

# Standard install target (make install)
include(GNUInstallDirs)
install(TARGETS mtg_extractor DESTINATION ${CMAKE_INSTALL_BINDIR})
