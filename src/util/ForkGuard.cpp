#include <csignal>
#include <cstring>
#include <sys/wait.h>
#include <util/Exception.hpp>
#include <util/ForkGuard.hpp>
using namespace std;

namespace tish {
  namespace utils {
    ForkGuard::ForkGuard( bool block_sig ) : process_id_ {}, subp_ret_ {}, old_set_ { nullptr }
    {
      if ( block_sig ) {
        old_set_ = make_unique<sigset_t>();

        sigemptyset( &new_set_ );
        sigaddset( &new_set_, SIGINT );
        sigaddset( &new_set_, SIGTSTP );
        // block the signal in parent process
        sigprocmask( SIG_BLOCK, &new_set_, old_set_.get() );
      }

      if ( ( process_id_ = fork() ) < 0 )
        throw error::SystemCallError( "fork" );
    }

    ForkGuard::ForkGuard( ForkGuard&& rhs )
      : process_id_ { rhs.process_id_ }
      , subp_ret_ { move( rhs.subp_ret_ ) }
      , old_set_ { move( rhs.old_set_ ) }
    {
      sigemptyset( &new_set_ );
      memcpy( new_set_.__val, rhs.new_set_.__val, sizeof( sigset_t::__val ) );
      sigemptyset( &rhs.new_set_ );
    }

    ForkGuard::~ForkGuard() noexcept
    {
      if ( is_parent() && old_set_ != nullptr ) {
        // wait();
        // resuming the signal processing in parent process
        sigprocmask( SIG_SETMASK, old_set_.get(), nullptr );
      }
    }

    ForkGuard::Pid ForkGuard::pid() const noexcept
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
      if ( is_parent() && !subp_ret_.has_value() ) {
        ExitCode status {};
        if ( waitpid( process_id_, &status, 0 ) < 0 )
          throw error::SystemCallError( "waitpid" );
        subp_ret_ = status;
      }
    }

    optional<ForkGuard::ExitCode> ForkGuard::exit_code() const noexcept
    {
      if ( is_parent() && subp_ret_.has_value() )
        return { static_cast<ExitCode>( WEXITSTATUS( *subp_ret_ ) ) };
      return nullopt;
    }

    void ForkGuard::reset_signals() noexcept
    {
      if ( is_child() ) {
        sigemptyset( &new_set_ );
        sigprocmask( SIG_SETMASK, &new_set_, nullptr );
        signal( SIGINT, SIG_DFL );
      }
    }
  } // namespace utils
} // namespace tish
