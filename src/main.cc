#include "scc_app.h"
#include "ssh_connection_scanner.h"

#include <argparse/argparse.hpp>
#include <unistd.h>

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char *argv[]) {
  argparse::ArgumentParser program("scc");
  program.add_description("SSH Connection Scanner (TUI)");

  program.add_argument("--config")
      .help("Path to ssh config file")
      .default_value(std::string{});
  program.add_argument("--exec")
      .help("Execute ssh after selection")
      .default_value(false)
      .implicit_value(true);
  program.add_argument("--no-print")
      .help("Do not print the command to stdout")
      .default_value(false)
      .implicit_value(true);

  try {
    program.parse_args(argc, argv);
  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    std::cerr << program;
    return 2;
  }

  ssh::SshConnectionScanner scanner;
  const std::string config_path = program.get<std::string>("--config");
  if (!config_path.empty()) {
    scanner.set_path_to_ssh_config(std::string{config_path});
  }

  std::vector<ssh::SshConnection> connections = scanner.fetch_ssh_connections();
  if (connections.empty()) {
    std::cerr << "No SSH connections found.\n";
    return 1;
  }

  std::vector<std::string> items;
  items.reserve(connections.size());
  for (const auto &c : connections) {
    items.push_back(scc_app::CompactLabel(c));
  }

  scc_app::State st;
  st.connections = std::move(connections);
  st.items = std::move(items);
  st.selected = 0;
  st.quit = false;
  st.do_exec = program.get<bool>("--exec");
  st.no_print = program.get<bool>("--no-print");
  st.pending_cmd.reset();

  scc_app::RunTui(st);

  if (!st.quit) {
    return 0;
  }

  if (st.pending_cmd.has_value()) {
    if (!st.no_print) {
      std::cout << *st.pending_cmd << std::endl;
    }
    if (st.do_exec) {
      std::string dest = scc_app::BuildDest(st.connections[st.selected]);
      std::vector<std::string> args_str = {"ssh", dest};
      std::vector<char *> args;
      args.reserve(args_str.size() + 1);
      for (auto &s : args_str) {
        args.push_back(s.data());
      }
      args.push_back(nullptr);
      const char *ssh_path = "/usr/bin/ssh";
      if (::access(ssh_path, X_OK) == 0) {
        ::execv(ssh_path, args.data());
      } else {
        ::execvp("ssh", args.data());
      }
      std::perror("exec ssh failed");
      return 127;
    }
  }

  return 0;
}
