#include "ssh_connection_scanner.h"

#include <cctype>
#include <format>
#include <fstream>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>

namespace ssh_utils {
inline auto ltrim_view(std::string_view s) -> std::string_view {
  size_t i = 0;
  while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i])))
    ++i;
  return s.substr(i);
}
inline auto rtrim_view(std::string_view s) -> std::string_view {
  size_t i = s.size();
  while (i > 0 && std::isspace(static_cast<unsigned char>(s[i - 1])))
    --i;
  return s.substr(0, i);
}
inline auto trim_view(std::string_view s) -> std::string_view {
  return rtrim_view(ltrim_view(s));
}
inline auto split_key_value_view(std::string_view line)
    -> std::pair<std::string_view, std::string_view> {
  line = trim_view(line);
  if (line.empty()) {
    return {std::string_view{}, std::string_view{}};
  }
  size_t key_end = 0;
  while (key_end < line.size()) {
    char c = line[key_end];
    if (c == '=' || c == ' ' || c == '\t') {
      break;
    }
    ++key_end;
  }
  std::string_view key = line.substr(0, key_end);

  size_t pos = key_end;
  while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) {
    ++pos;
  }
  if (pos < line.size() && line[pos] == '=') {
    ++pos;
    while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) {
      ++pos;
    }
  }

  std::string_view value =
      (pos < line.size()) ? line.substr(pos) : std::string_view{};
  value = trim_view(value);
  return {key, value};
}
inline auto to_lower(std::string_view s) -> std::string {
  std::string result;
  result.reserve(s.size());
  for (char c : s) {
    result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  return result;
}
} // namespace ssh_utils

namespace ssh {
auto SshConnectionScanner::set_path_to_ssh_config(std::string &&path) -> void {
  path_to_ssh_config_ = std::move(path);
}

auto SshConnectionScanner::get_path_to_ssh_config(void) const -> std::string {
  return path_to_ssh_config_;
}

auto SshConnectionScanner::get_ssh_config_content(void) const -> std::string {
  std::ifstream file(path_to_ssh_config_);
  if (!file.is_open()) {
    throw std::runtime_error(
        std::format("Failed to open SSH config file: {}", path_to_ssh_config_));
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

auto SshConnectionScanner::parse_ssh_config_content(std::string &&content) const
    -> std::vector<ssh::SshConnection> {
  std::istringstream stream(content);
  std::string raw;

  std::vector<ssh::SshConnection> out;
  std::vector<std::string> aliases;
  std::optional<std::string> hostname;
  std::optional<std::string> user;

  auto flush_block = [&]() {
    if (aliases.empty()) {
      return;
    }
    for (const auto &a : aliases) {
      ssh::SshConnection c;
      c.host = a;
      c.hostname = hostname;
      c.user = user;
      out.emplace_back(std::move(c));
    }
    aliases.clear();
    hostname.reset();
    user.reset();
  };

  while (std::getline(stream, raw)) {
    if (auto pos = raw.find('#'); pos != std::string::npos) {
      raw.erase(pos);
    }

    std::string_view line = ssh_utils::trim_view(raw);
    if (line.empty()) {
      continue;
    }

    auto [key_sv, val_sv] = ssh_utils::split_key_value_view(line);
    if (key_sv.empty()) {
      continue;
    }

    std::string key = ssh_utils::to_lower(std::string(key_sv));
    std::string value(val_sv);

    if (key == "host") {
      flush_block();
      std::istringstream vs(value);
      std::string tok;
      while (vs >> tok) {
        if (!tok.empty()) {
          aliases.emplace_back(std::move(tok));
        }
      }
      continue;
    }

    if (key == "hostname") {
      if (value.empty()) {
        hostname.reset();
      } else {
        hostname = std::move(value);
      }
      continue;
    }

    if (key == "user") {
      if (value.empty()) {
        user.reset();
      } else {
        user = std::move(value);
      }
      continue;
    }
  }

  flush_block();
  return out;
}

auto SshConnectionScanner::fetch_ssh_connections(void) const
    -> std::vector<ssh::SshConnection> {
  std::string content = get_ssh_config_content();
  std::vector<ssh::SshConnection> connections =
      parse_ssh_config_content(std::move(content));
  auto filtered =
      connections | std::ranges::views::filter([](const ssh::SshConnection &c) {
        return !c.host.empty() && c.host != "*";
      });
  return std::vector<ssh::SshConnection>(filtered.begin(), filtered.end());
}
} // namespace ssh