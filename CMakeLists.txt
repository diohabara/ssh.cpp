# Language
cmake_minimum_required(VERSION 3.22)
project(scc
  LANGUAGES CXX
  VERSION 0.1.0
)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ftxui
include(FetchContent)
FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
  GIT_TAG v6.1.9
)
FetchContent_MakeAvailable(ftxui)

# argparse
FetchContent_Declare(
  argparse
  GIT_REPOSITORY https://github.com/p-ranav/argparse.git
)
FetchContent_MakeAvailable(argparse)

# Build library for ssh_connection_scanner so both binary and tests can link it
add_library(ssh_connection_scanner STATIC
  src/ssh_connection_scanner.cc
)
target_include_directories(ssh_connection_scanner PUBLIC
  ${CMAKE_SOURCE_DIR}/include
)
target_compile_features(ssh_connection_scanner PUBLIC cxx_std_20)

# executable (link against the library)
add_executable(scc
  src/main.cc
  src/scc_app.cc
)
target_include_directories(scc PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(scc PRIVATE
  ssh_connection_scanner
  # ftxui
  ftxui::component
  # argparse
  argparse
)

# test
enable_testing()
# Delegate test target creation to the tests/ subdirectory
add_subdirectory(tests)

# Compiler features
target_compile_features(scc PRIVATE cxx_std_20)

# Install target
install(TARGETS scc DESTINATION bin)
