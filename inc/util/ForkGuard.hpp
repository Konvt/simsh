#ifndef TISH_FORKGUARD
#define TISH_FORKGUARD

#include <memory>
#include <optional>
#include <sys/types.h>

namespace tish {
  namespace util {
    class ForkGuard {
    public:
      using ExitCode = int;
      using Pid      = pid_t;

    private:
      Pid process_id_;
      std::optional<ExitCode> subp_ret_;

      std::unique_ptr<sigset_t> old_set_;
      sigset_t new_set_;

    public:
      ForkGuard( const ForkGuard& )            = delete;
      ForkGuard& operator=( const ForkGuard& ) = delete;
      // Due to the class already has the process signal set, it cannot be
      // reassigned after construction.
      ForkGuard& operator=( ForkGuard&& )      = delete;

      /// @brief Fork and check for success.
      explicit ForkGuard( bool block_sig = true ) noexcept( false );
      ForkGuard( ForkGuard&& rhs ) noexcept;

      /// @brief Abandon the management child process, and deconstruct this
      /// object.
      ~ForkGuard() noexcept;
      [[nodiscard]] Pid pid() const noexcept;
      [[nodiscard]] bool is_parent() const noexcept;
      [[nodiscard]] bool is_child() const noexcept;

      /// @brief Check the exit code of subprocess.
      /// @return Return exit code.
      [[nodiscard]] std::optional<ExitCode> exit_code() const noexcept;

      /// @brief Wait for the subprocess to exit.
      void wait() noexcept( false );

      /// @brief Only reset the signals in subprocess, it's invalid for parent
      /// process.
      void reset_signals() noexcept;
    };
  } // namespace util
} // namespace tish

#endif // TISH_FORKGUARD
