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
      static constexpr type_decl::StrViewT default_fmt = "\x1b[32;1m{}@{}\x1b[0m:\x1b[34;1m{}\x1b[0m$ ";
      static constexpr type_decl::StrViewT root_fmt = "{}@{}:{}# ";
      static constexpr type_decl::StrViewT welcome_mes =
        "\x1b[36;1m"
        "      _               _     \n"
        "  ___(_)_ __ ___  ___| |__  \n"
        " / __| | '_ ` _ \\/ __| '_ \\ \n"
        " \\__ \\ | | | | | \\__ \\ | | |\n"
        " |___/_|_| |_| |_|___/_| |_|\n"
        "                            \n"
        "\x1b[0m"
        "Type \x1b[32mhelp\x1b[0m for more information\n";

      type_decl::StringT prompt_;

      type_decl::StringT host_name_;
      type_decl::StringT home_dir_;
      type_decl::StringT current_dir_;

      type_decl::StringT user_name_;

      void update_prompt();
      void detect_info();

    public:
      Shell( Parser prsr )
        : BaseShell( std::move( prsr ) ) {}
      Shell() : BaseShell() {
        detect_info();
      }
      virtual ~Shell() = default;
      type_decl::StrViewT prompt() const { return prompt_; }

      int run() override;
    };
  }
}

#endif // __SHIMSH_SHELL__
