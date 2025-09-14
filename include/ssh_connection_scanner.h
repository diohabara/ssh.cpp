#pragma once

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace ssh_utils {
inline std::string get_default_path_to_ssh_config() {
  auto home = std::getenv("HOME");
  if (home) {
    return (std::filesystem::path(home) / ".ssh" / "config").string();
  } else {
    throw std::runtime_error("HOME environment variable is not set.");
  }
}
} // namespace ssh_utils

namespace ssh {
struct SshConnection {
  std::string host;
  std::optional<std::string> hostname;
  std::optional<std::string> user;

  SshConnection() = default;
  SshConnection(std::string h, std::optional<std::string> hn = std::nullopt,
                std::optional<std::string> u = std::nullopt)
      : host(std::move(h)), hostname(std::move(hn)), user(std::move(u)) {}
  friend bool operator==(const SshConnection &a, const SshConnection &b) {
    return a.host == b.host && a.hostname == b.hostname && a.user == b.user;
  };

  friend std::ostream &operator<<(std::ostream &os, const SshConnection &conn) {
    os << "Host: " << (conn.host.empty() ? "(none)" : conn.host);
    os << ", Hostname: " << conn.hostname.value_or("(none)");
    os << ", User: " << conn.user.value_or("(none)");
    return os;
  }
};

class SshConnectionScanner {
private:
  std::vector<std::string> ssh_connections_;
  std::string path_to_ssh_config_;

public:
  explicit SshConnectionScanner(
      std::string p = ssh_utils::get_default_path_to_ssh_config())
      : path_to_ssh_config_(std::move(p)) {}
  ~SshConnectionScanner() noexcept = default;
  SshConnectionScanner(const SshConnectionScanner &other) = delete;
  SshConnectionScanner &operator=(const SshConnectionScanner &other) = delete;
  SshConnectionScanner(SshConnectionScanner &&other) = default;
  SshConnectionScanner &operator=(SshConnectionScanner &&other) = delete;

  auto set_path_to_ssh_config(std::string &&path) -> void;
  auto get_path_to_ssh_config(void) const -> std::string;
  auto get_ssh_config_content(void) const -> std::string;
  auto parse_ssh_config_content(std::string &&content) const
      -> std::vector<SshConnection>;
  auto fetch_ssh_connections(void) const -> std::vector<SshConnection>;
};

class SshConnectionNotFoundException : public std::exception {
public:
  const char *what() const noexcept override {
    return "No SSH connections found in the configuration file.";
  }
};

} // namespace ssh