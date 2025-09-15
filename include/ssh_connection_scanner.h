#pragma once

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace ssh_utils {
inline std::string get_default_path_to_ssh_config() {
  if (const char *home = std::getenv("HOME")) {
    return (std::filesystem::path(home) / ".ssh" / "config").string();
  }
  throw std::runtime_error("HOME environment variable is not set.");
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
  }

  friend std::ostream &operator<<(std::ostream &os, const SshConnection &conn) {
    os << "Host: " << (conn.host.empty() ? "(none)" : conn.host)
       << ", Hostname: " << conn.hostname.value_or("(none)")
       << ", User: " << conn.user.value_or("(none)");
    return os;
  }
};

class SshConnectionScanner {
private:
  std::vector<std::string> ssh_connections_;
  std::string path_to_ssh_config_;

public:
  explicit SshConnectionScanner(
      std::string &&p = ssh_utils::get_default_path_to_ssh_config())
      : path_to_ssh_config_(std::move(p)) {}
  ~SshConnectionScanner() = default;
  SshConnectionScanner(const SshConnectionScanner &) = delete;
  SshConnectionScanner &operator=(const SshConnectionScanner &) = delete;
  SshConnectionScanner(SshConnectionScanner &&) noexcept = default;
  SshConnectionScanner &operator=(SshConnectionScanner &&) noexcept = default;

  void set_path_to_ssh_config(std::string &&p);
  [[nodiscard]] std::string get_path_to_ssh_config() const;
  [[nodiscard]] std::string get_ssh_config_content() const;
  [[nodiscard]] std::vector<SshConnection>
  parse_ssh_config_content(std::string &&content) const;
  [[nodiscard]] std::vector<SshConnection> fetch_ssh_connections() const;
};

class SshConnectionNotFoundException : public std::runtime_error {
public:
  explicit SshConnectionNotFoundException(
      std::string msg = "No SSH connections found in the configuration file.")
      : std::runtime_error(std::move(msg)) {}
};

} // namespace ssh