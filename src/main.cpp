#include <CLI.hpp>
#include <Parser.hpp>
#include <fstream>
#include <span>
#include <unistd.h>
#include <util/Exception.hpp>
#include <util/Logger.hpp>
#include <util/Pipe.hpp>
#include <util/Utils.hpp>
using namespace std;

int main( int argc, char** argv )
{
  if ( argc == 0 )
    abort();
  else if ( argc == 1 )
    return tish::cli::CLI().run();

  if ( "-c"sv == argv[1] || argc > 2 ) {
    if ( argc == 2 && "-c"sv == argv[1] ) {
      tish::iout::logger << tish::error::ArgumentError( "tish: -c:",
                                                        "option requires an argument" );
      return EXIT_FAILURE;
    }

    tish::utils::Pipe pipe;
    tish::utils::rebind_fd( pipe.reader().get(), STDIN_FILENO );
    ranges::for_each(
      "-c"sv == argv[1] ? span( argv + 2, argc - 2 ) : span( argv + 1, argc - 1 ),
      [&writer = pipe.writer()]( const auto e ) noexcept { writer.push( e ).push( ' ' ); } );
    pipe.writer().push( '\n' );
    pipe.writer().close();
    return tish::cli::BaseCLI().run();
  } else if ( "-v"sv == argv[1] || "--version"sv == argv[1] ) {
    tish::iout::prmptr << format( "tish, version {}\n", TISH_VERSION );
    return EXIT_SUCCESS;
  } else {
    ifstream ifs { argv[1] };
    if ( !ifs ) {
      tish::iout::logger.print( tish::error::SystemCallError( format( "tish: {}", argv[1] ) ) );
      return EXIT_FAILURE;
    }
    return tish::cli::BaseCLI( tish::Parser( ifs ) ).run();
  }
}
