#include <sys/wait.h>

#include "ForkGuard.hpp"
#include "Logger.hpp"
using namespace std;

namespace simsh {
  namespace utils {
    ForkGuard::ForkGuard()
      : process_id_ {}
      , subprocess_exit_code_ {}
    {
      sigemptyset( &new_set_ );
      sigaddset( &new_set_, SIGINT );
      // block the signal in parent process
      if ( sigprocmask( SIG_BLOCK, &new_set_, &old_set_ ) < 0 )
        throw error::SystemCallError( "sigprocmask" );

      process_id_ = fork();

      if ( process_id_ < 0 )
        throw error::SystemCallError( "fork" );
    }

    ForkGuard::~ForkGuard() noexcept(false)
    {
      if ( is_parent() ) {
        wait();

        // resuming the signal processing in parent process
        if ( sigprocmask( SIG_SETMASK, &old_set_, nullptr ) < 0 ) {
          iout::logger << error::SystemCallError( "sigprocmask" );
          throw error::TerminationSignal( EXIT_FAILURE );
        }
      }
    }

    pid_t ForkGuard::pid() const noexcept
    {
      return process_id_;
    }

    bool ForkGuard::is_parent() const noexcept
    {
      return process_id_ != 0;
    }

    bool ForkGuard::is_child() const noexcept
    {
      return process_id_ == 0;
    }

    void ForkGuard::wait()
    {
      if ( is_parent() && !subprocess_exit_code_.has_value() ) {
        int status {};
        if ( waitpid( process_id_, &status, 0 ) < 0 )
          throw error::SystemCallError( "waitpid" );
        subprocess_exit_code_ = status;
      }
    }

    int ForkGuard::exit_code() const noexcept
    {
      if ( is_parent() && subprocess_exit_code_.has_value() )
        return WEXITSTATUS( *subprocess_exit_code_ );
      return INT32_MIN;
    }

    void ForkGuard::reset_signals() noexcept
    {
      if ( is_child() ) {
        sigemptyset( &new_set_ );
        sigprocmask( SIG_SETMASK, &new_set_, nullptr );
        signal( SIGINT, SIG_DFL );
      }
    }
  }
}
