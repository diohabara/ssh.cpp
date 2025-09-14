#pragma once
#include "ssh_connection_scanner.h"

#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace scc_app {

std::string CompactLabel(const ssh::SshConnection &c);
std::string BuildDest(const ssh::SshConnection &c);
std::string BuildCmd(const ssh::SshConnection &c);

struct State {
  std::vector<ssh::SshConnection> connections;
  std::vector<std::string> items;
  int selected = 0;
  bool quit = false;
  bool do_exec = false;
  bool no_print = false;
  std::optional<std::string> pending_cmd;
};

struct MsgSelect {};
struct MsgQuit {};
struct MsgSetSelected {
  int index;
};
using Msg = std::variant<MsgSelect, MsgQuit, MsgSetSelected>;

State Update(State s, const Msg &m);
void RunTui(State &s);

} // namespace scc_app
