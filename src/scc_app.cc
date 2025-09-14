#include "scc_app.h"

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <string>
#include <utility>
#include <vector>

namespace scc_app {

std::string CompactLabel(const ssh::SshConnection &c) {
  if (c.user.has_value() && c.hostname.has_value()) {
    return *c.user + "@" + *c.hostname + " (" + c.host + ")";
  } else if (c.user.has_value() && !c.hostname.has_value()) {
    return *c.user + "@" + c.host;
  } else if (!c.user.has_value() && c.hostname.has_value()) {
    return *c.hostname + " (" + c.host + ")";
  } else {
    return c.host;
  }
}

std::string BuildDest(const ssh::SshConnection &c) {
  if (c.user.has_value() && c.hostname.has_value()) {
    return *c.user + "@" + *c.hostname;
  } else if (c.user.has_value() && !c.hostname.has_value()) {
    return *c.user + "@" + c.host;
  } else if (!c.user.has_value() && c.hostname.has_value()) {
    return *c.hostname;
  } else {
    return c.host;
  }
}

std::string BuildCmd(const ssh::SshConnection &c) {
  return "ssh " + BuildDest(c);
}

State Update(State s, const Msg &m) {
  if (std::holds_alternative<MsgQuit>(m)) {
    s.quit = true;
    return s;
  }
  if (std::holds_alternative<MsgSetSelected>(m)) {
    int i = std::get<MsgSetSelected>(m).index;
    if (i < 0) {
      i = 0;
    } else if (i >= static_cast<int>(s.connections.size())) {
      i = static_cast<int>(s.connections.size()) - 1;
    }
    s.selected = i;
    return s;
  }
  if (std::holds_alternative<MsgSelect>(m)) {
    if (!s.connections.empty()) {
      s.pending_cmd = BuildCmd(s.connections[s.selected]);
    }
    s.quit = true;
    return s;
  }
  return s;
}

void RunTui(State &s) {
  auto screen = ftxui::ScreenInteractive::Fullscreen();

  ftxui::MenuOption menu_opt;
  menu_opt.on_enter = [&] {
    s = Update(std::move(s), MsgSelect{});
    screen.ExitLoopClosure()();
  };
  menu_opt.on_change = [&] {
    s = Update(std::move(s), MsgSetSelected{s.selected});
  };

  auto menu = ftxui::Menu(s.items, &s.selected, menu_opt);

  auto details = ftxui::Renderer([&] {
    const auto &c = s.connections[s.selected];
    const std::string host = c.host.empty() ? "(none)" : c.host;
    const std::string hostname =
        c.hostname.has_value() ? *c.hostname : "(none)";
    const std::string user = c.user.has_value() ? *c.user : "(none)";
    const std::string cmd = BuildCmd(c);
    return ftxui::vbox(ftxui::Elements{
               ftxui::text("Details") | ftxui::bold,
               ftxui::separator(),
               ftxui::hbox(ftxui::Elements{ftxui::text("Host:     "),
                                           ftxui::text(host)}),
               ftxui::hbox(ftxui::Elements{ftxui::text("Hostname: "),
                                           ftxui::text(hostname)}),
               ftxui::hbox(ftxui::Elements{ftxui::text("User:     "),
                                           ftxui::text(user)}),
               ftxui::separator(),
               ftxui::text("SSH command:"),
               ftxui::text(cmd) | ftxui::color(ftxui::Color::Green),
           }) |
           ftxui::border | ftxui::flex;
  });

  auto btn_select = ftxui::Button("Select", [&] {
    s = Update(std::move(s), MsgSelect{});
    screen.ExitLoopClosure()();
  });
  auto btn_quit = ftxui::Button("Quit", [&] {
    s = Update(std::move(s), MsgQuit{});
    screen.ExitLoopClosure()();
  });

  auto buttons =
      ftxui::Container::Horizontal(ftxui::Components{btn_select, btn_quit});

  auto layout = ftxui::Container::Horizontal(ftxui::Components{
      menu,
      ftxui::Container::Vertical(ftxui::Components{
          details,
          buttons,
      }),
  });

  const std::string legend =
      "↑/↓: Move  Enter: Select  q: Quit  Esc: Quit  Tab: Switch Focus";

  auto body = ftxui::Renderer(layout, [&] {
    auto main_row = ftxui::hbox(ftxui::Elements{
                        ftxui::vbox(ftxui::Elements{
                            ftxui::text("Connections") | ftxui::bold,
                            ftxui::separator(),
                            menu->Render() | ftxui::frame |
                                ftxui::size(ftxui::WIDTH, ftxui::LESS_THAN, 50),
                        }) | ftxui::border,
                        ftxui::separator(),
                        ftxui::vbox(ftxui::Elements{
                            details->Render(),
                            ftxui::separator(),
                            ftxui::hbox(ftxui::Elements{
                                btn_select->Render() | ftxui::center,
                                ftxui::text("  "),
                                btn_quit->Render() | ftxui::center,
                            }) | ftxui::center,
                        }) | ftxui::flex,
                    }) |
                    ftxui::flex;

    return ftxui::vbox(ftxui::Elements{
               main_row,
               ftxui::separator(),
               ftxui::text(legend) | ftxui::dim | ftxui::center,
           }) |
           ftxui::flex;
  });

  auto app = ftxui::CatchEvent(body, [&](ftxui::Event e) {
    if (e == ftxui::Event::Character('q') ||
        e == ftxui::Event::Character('Q') || e == ftxui::Event::Escape) {
      s = Update(std::move(s), MsgQuit{});
      screen.ExitLoopClosure()();
      return true;
    }
    return false;
  });

  screen.Loop(app);
}

} // namespace scc_app
