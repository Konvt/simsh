#include <fstream>
#include <span>
#include <string>

#include <unistd.h>

#include "Exception.hpp"
#include "Logger.hpp"
#include "Parser.hpp"
#include "Pipe.hpp"

#include "CLI.hpp"
using namespace std;

int main( int argc, char** argv )
{
  if ( argc == 0 )
    abort();
  else if ( argc == 1 )
    return simsh::cli::CLI().run();

  if ( "-c"sv == argv[1] || argc > 2 ) {
    if ( argc == 2 && "-c"sv == argv[1] ) {
      simsh::iout::logger << simsh::error::ArgumentError(
        "simsh: -c:", "option requires an argument" );
      return EXIT_FAILURE;
    }

    simsh::utils::Pipe pipe;
    dup2( pipe.reader().get(), STDIN_FILENO );
    const auto args = "-c"sv == argv[1] ? span( argv + 2, argc - 2 )
                                        : span( argv + 1, argc - 1 );
    for ( const auto e : args ) {
      pipe.writer().push( e );
      pipe.writer().push( ' ' );
    }
    pipe.writer().push( '\n' );
    pipe.writer().close();
    return simsh::cli::BaseCLI().run();
  } else if ( "-v"sv == argv[1] || "--version"sv == argv[1] ) {
    simsh::iout::prmptr << format( "simsh, version {}\n", SIMSH_VERSION );
    return EXIT_SUCCESS;
  } else {
    ifstream ifs { argv[1] };
    if ( !ifs ) {
      simsh::iout::logger.print(
        simsh::error::SystemCallError( format( "simsh: {}", argv[1] ) ) );
      return EXIT_FAILURE;
    }
    return simsh::cli::BaseCLI( simsh::Parser( ifs ) ).run();
  }
}
