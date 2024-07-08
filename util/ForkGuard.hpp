#ifndef __SIMSH__FORKGUARD__
# define __SIMSH__FORKGUARD__

#include <optional>
#include <csignal>
#include <sys/types.h>

#include "Config.hpp"
#include "Exception.hpp"

namespace simsh {
  namespace utils {
    class ForkGuard {
      pid_t process_id_;
      std::optional<int> subprocess_exit_code_;

      sigset_t new_set_, old_set_;

    public:
      /// @brief Fork and check for success.
      /// @throw error::SystemCallError Fork failed or block the signals failed.
      ForkGuard();

      /// @brief Waits and recovers the signals in parent process ONLY.
      /// @throw error::TerminationSignal Thrown only in parent process, if the recovery signal failed.
      ~ForkGuard() noexcept(false);
      [[nodiscard]] pid_t pid() const noexcept;
      [[nodiscard]] bool is_parent() const noexcept;
      [[nodiscard]] bool is_child() const noexcept;
      [[nodiscard]] int exit_code() const noexcept;

      /// @brief Wait for the subprocess to exit.
      /// @throw error::SystemCallError If `waitpid` fails.
      void wait();

      /// @brief Only reset the signals in subprocess, it's invalid for parent process.
      void reset_signals() noexcept;
    };
  }
}

#endif // __SIMSH__FORKGUARD__
