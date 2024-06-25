#ifndef __SHIMSH_SHELL__
# define __SHIMSH_SHELL__

#include "Config.hpp"
#include "Parser.hpp"

namespace simsh {
  namespace shell {
    /// @brief The simplest implementation of the shell.
    class BaseShell {
    protected:
      Parser prsr_;

    public:
      BaseShell( Parser prsr )
        : prsr_ { std::move( prsr ) } {}
      BaseShell() = default;
      virtual ~BaseShell() = default;

      virtual int run();
    };

    class Shell : public BaseShell {
#define CLEAR_LINE "\x1b[1K\r"
      static constexpr types::StrViewT default_fmt = CLEAR_LINE "\x1b[32;1m{}@{}\x1b[0m:\x1b[34;1m{}\x1b[0m$ ";
      static constexpr types::StrViewT root_fmt = CLEAR_LINE "{}@{}:{}# ";
#undef CLEAR_LINE
      static constexpr types::StrViewT welcome_mes =
        "\n\x1b[36;1m"
        "      _               _     \n"
        "  ___(_)_ __ ___  ___| |__  \n"
        " / __| | '_ ` _ \\/ __| '_ \\ \n"
        " \\__ \\ | | | | | \\__ \\ | | |\n"
        " |___/_|_| |_| |_|___/_| |_|\n"
        "                            \n"
        "\x1b[0m\n"
        "Type \x1b[32mhelp\x1b[0m for more information\n";

      types::StringT prompt_;

      types::StringT host_name_;
      types::StringT home_dir_;
      types::StringT current_dir_;

      types::StringT user_name_;

      void update_prompt();
      void detect_info();

    public:
      Shell( Parser prsr )
        : BaseShell( std::move( prsr ) ) {}
      Shell() : BaseShell() {
        detect_info();
      }
      virtual ~Shell() = default;
      [[nodiscard]] types::StrViewT prompt() const { return prompt_; }

      int run() override;
    };
  }
}

#endif // __SHIMSH_SHELL__
