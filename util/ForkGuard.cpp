#include <cstring>
#include <sys/wait.h>

#include "ForkGuard.hpp"
#include "Logger.hpp"
using namespace std;

namespace simsh {
namespace utils {
ForkGuard::ForkGuard( bool block_signal )
  : process_id_ {}, subprocess_exit_code_ {}, old_set_ { nullptr }
{
  if ( block_signal ) {
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
  , subprocess_exit_code_ { move( rhs.subprocess_exit_code_ ) }
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
    //  resuming the signal processing in parent process
    sigprocmask( SIG_SETMASK, old_set_.get(), nullptr );
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
