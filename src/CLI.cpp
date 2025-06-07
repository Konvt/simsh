#include <CLI.hpp>
#include <Parser.hpp>
#include <array>
#include <filesystem>
#include <format>
#include <pwd.h>
#include <unistd.h>
#include <util/Config.hpp>
#include <util/Exception.hpp>
#include <util/Logger.hpp>
#include <util/Util.hpp>
using namespace std;

namespace tish {
  namespace cli {
    std::atomic<bool> BaseCLI::_already_exist = false;

    int BaseCLI::run()
    {
      signal(
        SIGINT,
        +[]( int signum ) -> void {
          [[maybe_unused]] auto _ = signum;
          iout::prmptr << "\n" << std::flush;
        } );
      signal(
        SIGTSTP,
        +[]( int signum ) -> void {
          [[maybe_unused]] auto _ = signum;
          iout::prmptr << "\n" << std::flush;
        } );
      tish::iout::logger.set_prefix( "tish: " );

      while ( !prsr_.empty() ) {
        try {
          interp_.evaluate( prsr_.parse().get() );
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
      type::String error_info;
      if ( last_result_.has_value() && !last_result_.value() ) {
        error_info = last_result_->side_val_.size() < 2
                     ? format( FG_RED "[" BOLD_TXT "{}" RESETSTYLE FG_RED "]" RESETSTYLE,
                               last_result_->value_ )
                     : format( FG_RED "[" BOLD_TXT "{}|{}" RESETSTYLE FG_RED "]" RESETSTYLE,
                               last_result_->side_val_.front(),
                               last_result_->side_val_.back() );
        last_result_.reset();
      }
      if ( user_name_ == "root" )
        prompt_ = format( _root_fmt, user_name_, host_name_, current_dir_, error_info );
      else
        prompt_ = format( _default_fmt, user_name_, host_name_, current_dir_, error_info );
    }

    void CLI::detect_info()
    {
      bool info_updated = last_result_.has_value() && !last_result_.value();

      const passwd* const pw = getpwuid( getuid() );
      if ( user_name_ != pw->pw_name ) {
        user_name_   = pw->pw_name;
        info_updated = true;
      }
      if ( home_dir_ != pw->pw_dir ) {
        home_dir_    = pw->pw_dir;
        info_updated = true;
      }
      if ( auto cwd = filesystem::current_path().string(); current_dir_ != cwd ) {
        swap( current_dir_, cwd );
        info_updated = true;
      }
      array<char, HOST_NAME_MAX + 1> hostname;
      gethostname( hostname.data(), hostname.size() * sizeof( char ) );
      if ( host_name_ != hostname.data() ) {
        host_name_   = hostname.data();
        info_updated = true;
      }

      if ( info_updated )
        update_prompt();
    }

    int CLI::run()
    {
      auto wrapper =
        util::make_fnptr<std::remove_pointer_t<sighandler_t>>( [this]( int signum ) noexcept {
          [[maybe_unused]] auto _ = signum;
          iout::prmptr << prompt() << std::flush;
        } );
      signal( SIGINT, wrapper );
      signal( SIGTSTP, wrapper );

      tish::iout::logger.set_prefix( "tish: " );
      tish::iout::prmptr << _welcome_mes;

      while ( !prsr_.empty() ) {
        detect_info();
        iout::prmptr << prompt() << std::flush;

        try {
          last_result_ = interp_.evaluate( prsr_.parse().get() );
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
  } // namespace cli
} // namespace tish
