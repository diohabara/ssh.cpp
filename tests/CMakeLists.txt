# Download and make GoogleTest available
include(FetchContent)
FetchContent_Declare(
  googletest
  DOWNLOAD_EXTRACT_TIMESTAMP ON
  URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
)
# For MSVC, avoid overriding parent settings (no harm on Linux)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Build test executable named "tests" so CTest can find it
add_executable(tests test_ssh_connection_scanner.cc)

# Ensure test can find project headers
target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Link with gtest_main so main() is provided and with your library
target_link_libraries(tests PRIVATE gtest_main ssh_connection_scanner)

# Register tests with CTest
include(GoogleTest)
gtest_discover_tests(tests)
