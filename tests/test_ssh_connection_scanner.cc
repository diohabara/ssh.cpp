#include "ssh_connection_scanner.h"
#include <algorithm>
#include <cstring>
#include <format>
#include <gtest/gtest.h>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

// RAII helper
class ScopedEnvVar {
public:
  ScopedEnvVar(const char *name, const char *value) : name_(name) {
    auto *prev = std::getenv(name);
    if (prev) {
      old_.emplace(prev);
    }
    setenv(name, value, 1);
  }

  ~ScopedEnvVar() {
    if (old_) {
      setenv(name_.c_str(), old_->c_str(), 1);
    } else {
      unsetenv(name_.c_str());
    }
  }

  ScopedEnvVar(const ScopedEnvVar &) = delete;
  ScopedEnvVar &operator=(const ScopedEnvVar &) = delete;
  ScopedEnvVar(ScopedEnvVar &&) = delete;
  ScopedEnvVar &operator=(ScopedEnvVar &&) = delete;

private:
  std::string name_;
  std::optional<std::string> old_;
};

} // namespace

namespace ssh {

TEST(SshConnectionScannerTest, TestGetPathToSshConfig_Custom) {
  // Arrange
  SshConnectionScanner scanner{"/tmp/ssh_config"};
  // Act
  auto achieved = scanner.get_path_to_ssh_config();
  // Assert
  auto expected = std::string("/tmp/ssh_config");
  EXPECT_EQ(achieved, expected);
}

TEST(SshConnectionScannerTest, TestGetPathToSshConfig_DefaultUsesHome) {
  // Arrange
  ScopedEnvVar env("HOME", "/home/vscode");
  SshConnectionScanner scanner{};
  // Act
  auto achieved = scanner.get_path_to_ssh_config();
  // Assert
  auto expected = std::string("/home/vscode/.ssh/config");
  EXPECT_EQ(achieved, expected);
}

TEST(SshConnectionScannerTest, TestGetPathToSshConfig_NoHome) {
  // Arrange
  ScopedEnvVar env("HOME", "/");
  SshConnectionScanner scanner{};
  // Act
  auto achieved = scanner.get_path_to_ssh_config();
  // Assert
  auto expected = std::string("/.ssh/config");
  EXPECT_EQ(achieved, expected);
}

TEST(SshConnectionScannerTest, TestSshConfigContent_FileNotExists) {
  // Arrange
  auto path = "/tmp/this_file_does_not_exist";
  SshConnectionScanner scanner{path};
  // Act & Assert
  auto act = [&] { (void)scanner.get_ssh_config_content(); };
  // Assert
  ASSERT_THROW(act(), std::runtime_error);
}

TEST(SshConnectionScannerTest, TestSshConfigContent_FileExists) {
  // Arrange
  auto path = "/etc/ssh/ssh_config"; // This file should exist on most systems
  SshConnectionScanner scanner{path};
  // Act
  auto content = scanner.get_ssh_config_content();
  // Assert
  ASSERT_FALSE(content.empty());
}

TEST(SshConnectionScannerTest, TestParseSshConfig_CustomFile) {
  // Arrange
  SshConnectionScanner scanner{};
  std::string content = R"(
# This is a comment
Host example.com
    User user1
Host test.com
    User user2
Host *.example.org
    User user3
Host 192.168.1.1
    User user4
HOst HogeHoge
    Hostname hoge.example.com
    User user5
)";
  // Act
  auto parsed_connections =
      scanner.parse_ssh_config_content(std::move(content));
  // Assert
  std::vector<SshConnection> expected = {
      SshConnection{"example.com", std::nullopt, "user1"},
      SshConnection{"test.com", std::nullopt, "user2"},
      SshConnection{"*.example.org", std::nullopt, "user3"},
      SshConnection{"192.168.1.1", std::nullopt, "user4"},
      SshConnection{"HogeHoge", "hoge.example.com", "user5"},
  };
  EXPECT_EQ(parsed_connections, expected);
}

} // namespace ssh
