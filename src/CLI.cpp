#include <array>
#include <climits>
#include <filesystem>
#include <format>

#include "Exception.hpp"
#include "Logger.hpp"
#include "Parser.hpp"

#include <pwd.h>
#include <unistd.h>

#include "CLI.hpp"
#include "Utils.hpp"
using namespace std;

namespace simsh {
namespace cli {
std::atomic<bool> BaseCLI::already_exist_ = false;

int BaseCLI::run()
{
  signal(
    SIGINT, +[]( int signum ) -> void {
      [[maybe_unused]] auto _ = signum;
      iout::prmptr << "\n" << std::flush;
    } );
  signal(
    SIGTSTP, +[]( int signum ) -> void {
      [[maybe_unused]] auto _ = signum;
      iout::prmptr << "\n" << std::flush;
    } );
  simsh::iout::logger.set_prefix( "simsh: " );

  while ( !prsr_.empty() ) {
    try {
      interp_( prsr_.parse().get() );
    } catch ( const error::SystemCallError& e ) {
      iout::logger.print( e );
    } catch ( const error::TerminationSignal& e ) {
      return e.value();
    } catch ( const error::TraceBack& e ) {
      iout::logger << e;
    }
  }
  return EXIT_SUCCESS;
}

void CLI::update_prompt()
{
  if ( current_dir_.size() >= home_dir_.size()
       && current_dir_.compare( 0, home_dir_.size(), home_dir_ ) == 0 )
    current_dir_.replace( 0, home_dir_.size(), "~" );
  if ( user_name_ == "root" )
    prompt_ = format( root_fmt, user_name_, host_name_, current_dir_ );
  else
    prompt_ = format( default_fmt, user_name_, host_name_, current_dir_ );
}

void CLI::detect_info()
{
  bool info_updated = false;

  const passwd* const pw = getpwuid( getuid() );
  if ( user_name_ != pw->pw_name ) {
    user_name_ = pw->pw_name;
    info_updated = true;
  }
  if ( home_dir_ != pw->pw_dir ) {
    home_dir_ = pw->pw_dir;
    info_updated = true;
  }
  if ( auto cwd = filesystem::current_path().string(); current_dir_ != cwd ) {
    swap( current_dir_, cwd );
    info_updated = true;
  }
  array<char, HOST_NAME_MAX + 1> hostname;
  gethostname( hostname.data(), hostname.size() * sizeof( char ) );
  if ( host_name_ != hostname.data() ) {
    host_name_ = hostname.data();
    info_updated = true;
  }

  if ( info_updated )
    update_prompt();
}

int CLI::run()
{
  auto sigint_handler = [this]( int signum ) -> void {
    [[maybe_unused]] auto _ = signum;
    iout::prmptr << prompt() << std::flush;
  };

  using LambdaT = decltype( sigint_handler );
  static_assert(
    is_same_v<utils::function_traits<LambdaT>::result_type,
              utils::function_traits<sighandler_t>::result_type>
      && is_same_v<utils::function_traits<LambdaT>::arguments,
                   utils::function_traits<sighandler_t>::arguments>,
    "'sigint_handler' and 'sighandler_t' must have the same signature" );

  auto wrapper = utils::make_fntor_wrapper( sigint_handler );
  signal( SIGINT, wrapper.to_fnptr<int>() );
  signal( SIGTSTP, wrapper.to_fnptr<int>() );

  simsh::iout::logger.set_prefix( "simsh: " );
  simsh::iout::prmptr << welcome_mes;

  while ( !prsr_.empty() ) {
    detect_info();
    iout::prmptr << prompt_ << std::flush;

    try {
      interp_( prsr_.parse().get() );
    } catch ( const error::SystemCallError& e ) {
      iout::logger.print( e );
    } catch ( const error::TerminationSignal& e ) {
      return e.value();
    } catch ( const error::TraceBack& e ) {
      iout::logger << e;
    }
  }
  iout::prmptr << "\nexit" << std::flush;
  return EXIT_SUCCESS;
}
}
}
