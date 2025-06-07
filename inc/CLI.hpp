#ifndef TISH_CLI
#define TISH_CLI

#include <Interpreter.hpp>
#include <Parser.hpp>
#include <atomic>
#include <csignal>
#include <optional>
#include <util/Config.hpp>
#include <util/Exception.hpp>
#include <util/Term.hpp>

namespace tish {
  namespace cli {
    /// @brief The simplest implementation of the shell.
    class BaseCLI {
      // A process can have only one shell instance in a same scope.
      static std::atomic<bool> _existed;

      struct sigaction _old_action;

    protected:
      Parser prsr_;
      Interpreter interp_;

    public:
      BaseCLI( Parser&& prsr ) : prsr_ { std::move( prsr ) }, interp_ {}
      {
        if ( _existed ) [[unlikely]]
          throw error::RuntimeError( "BaseCLI: CLI already exists" );
        else
          _existed = true;

        sigaction( SIGINT, nullptr, &_old_action ); // save the old signal action
      }
      BaseCLI() : BaseCLI( Parser() ) {}
      virtual ~BaseCLI() noexcept
      {
        sigaction( SIGINT, &_old_action, nullptr );
        _existed = false;
      };

      virtual int run();
    };

    /// @brief A shell with prompt.
    class CLI : public BaseCLI {
      static constexpr type::StrView _default_fmt = LINEWIPE LINESTART FG_GREEN BOLD_TXT
        "{}@{}" RESETSTYLE ":" FG_BLUE BOLD_TXT "{}" RESETSTYLE " {}$ ";
      static constexpr type::StrView _root_fmt = LINEWIPE LINEWIPE "{}@{}:{} {}# ";
      static constexpr type::StrView _binary_err_fmt =
        FG_RED "[" BOLD_TXT "{}|{}" RESETSTYLE FG_RED "]" RESETSTYLE;
      static constexpr type::StrView _unary_err_fmt =
        FG_RED "[" BOLD_TXT "{}" RESETSTYLE FG_RED "]" RESETSTYLE;
      static constexpr type::StrView _welcome_mes =
        FG_CYAN BOLD_TXT "  _   _     _\n"
                         " | |_(_)___| |__\n"
                         " | __| / __| '_ \\\n"
                         " | |_| \\__ \\ | | |\n"
                         "  \\__|_|___/_| |_|\n" RESETSTYLE "\n"
                         "Type " FG_GREEN "help" RESETSTYLE " for more information\n";

      type::String prompt_;

      type::String host_name_;
      type::String home_dir_;
      type::String current_dir_;

      type::String user_name_;
      std::optional<Interpreter::EvalResult> last_result_;

      void update_prompt();

      /// @brief Check whether the information in the prompt has changed, and
      /// update the prompt if so.
      void detect_info();

    public:
      CLI( Parser&& prsr ) : BaseCLI( std::move( prsr ) ) { detect_info(); }
      CLI() : CLI( Parser() ) {}
      virtual ~CLI() = default;
      [[nodiscard]] type::StrView prompt() const noexcept { return prompt_; }

      virtual int run();
    };
  } // namespace cli
} // namespace tish

#endif // TISH_CLI
