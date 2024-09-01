#ifndef __SHIMSH_SHELL__
#define __SHIMSH_SHELL__

#include <atomic>
#include <csignal>
#include <stdexcept>

#include "Config.hpp"
#include "Interpreter.hpp"
#include "Parser.hpp"

namespace simsh {
  namespace cli {
    /// @brief The simplest implementation of the shell.
    class BaseCLI {
      static std::atomic<bool> already_exist_; // A process can have only one shell
                                               // instance in a same scope.

      struct sigaction old_action_;

    protected:
      Parser prsr_;
      Interpreter interp_;

    public:
      /// @throw error::RuntimeError If more than one object instances are
      /// created in the same scope.
      BaseCLI( Parser prsr ) : prsr_ { std::move( prsr ) }, interp_ {}
      {
        [[unlikely]] if ( already_exist_ )
          throw error::RuntimeError( "BaseCLI: CLI already exists" );
        else already_exist_ = true;

        sigaction( SIGINT, nullptr, &old_action_ ); // save the old signal action
      }
      BaseCLI() : BaseCLI( Parser() ) {}
      virtual ~BaseCLI()
      {
        sigaction( SIGINT, &old_action_, nullptr );
        already_exist_ = false;
      };

      virtual int run();
    };

    /// @brief A shell with prompt.
    class CLI : public BaseCLI {
#define CLEAR_LINE "\x1b[1K\r"
      static constexpr types::StrViewT default_fmt =
        CLEAR_LINE "\x1b[32;1m{}@{}\x1b[0m:\x1b[34;1m{}\x1b[0m$ ";
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

      /// @brief Check whether the information in the prompt has changed, and
      /// update the prompt if so.
      void detect_info();

    public:
      CLI( Parser prsr ) : BaseCLI( std::move( prsr ) ) { detect_info(); }
      CLI() : CLI( Parser() ) {}
      virtual ~CLI() = default;
      [[nodiscard]] types::StrViewT prompt() const { return prompt_; }

      virtual int run();
    };
  } // namespace cli
} // namespace simsh

#endif // __SHIMSH_SHELL__
