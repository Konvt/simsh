#ifndef __SIMSH__FORKGUARD__
#define __SIMSH__FORKGUARD__

#include <csignal>
#include <memory>
#include <optional>
#include <sys/types.h>

#include "Config.hpp"
#include "Exception.hpp"

namespace simsh {
  namespace utils {
    class ForkGuard {
    public:
      using ExitCodeT = int;
      using PidT      = pid_t;

    private:
      PidT process_id_;
      std::optional<ExitCodeT> subprocess_exit_code_;

      std::unique_ptr<sigset_t> old_set_;
      sigset_t new_set_;

    public:
      ForkGuard( const ForkGuard& )            = delete;
      ForkGuard& operator=( const ForkGuard& ) = delete;
      // Due to the class already has the process signal set, it cannot be
      // reassigned after construction.
      ForkGuard& operator=( ForkGuard&& )      = delete;

      /// @brief Fork and check for success.
      /// @throw error::SystemCallError When fork failed.
      explicit ForkGuard( bool block_sig = true );
      ForkGuard( ForkGuard&& rhs );

      /// @brief Abandon the management child process, and deconstruct this
      /// object.
      ~ForkGuard() noexcept;
      [[nodiscard]] PidT pid() const noexcept;
      [[nodiscard]] bool is_parent() const noexcept;
      [[nodiscard]] bool is_child() const noexcept;

      /// @brief Check the exit code of subprocess.
      /// @return Return exit code.
      [[nodiscard]] std::optional<ExitCodeT> exit_code() const noexcept;

      /// @brief Wait for the subprocess to exit.
      /// @throw error::SystemCallError If `waitpid` fails.
      void wait();

      /// @brief Only reset the signals in subprocess, it's invalid for parent
      /// process.
      void reset_signals() noexcept;
    };
  } // namespace utils
} // namespace simsh

#endif // __SIMSH__FORKGUARD__
