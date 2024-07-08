#include <span>
#include <string>
#include <fstream>

#include <unistd.h>

#include "Pipe.hpp"
#include "Parser.hpp"
#include "Logger.hpp"
#include "Exception.hpp"

#include "Shell.hpp"
using namespace std;

int main( int argc, char **argv )
{
  if ( argc == 0 )
    abort();
  else if ( argc == 1 )
    return simsh::shell::Shell().run();

  if ( "-c"sv == argv[1] || argc > 2 ) {
    if ( argc == 2 && "-c"sv == argv[1] ) {
      simsh::iout::logger << simsh::error::ArgumentError(
        "simsh: -c:", "option requires an argument"
      );
      return EXIT_FAILURE;
    }

    simsh::utils::Pipe pipe;
    dup2( pipe.reader().get(), STDIN_FILENO );
    const auto args = "-c"sv == argv[1]
      ? span( argv + 2, argc - 2 ) : span( argv + 1, argc - 1 );
    for ( auto e : args ) {
      pipe.writer().push( e );
      pipe.writer().push( ' ' );
    }
    pipe.writer().push( '\n' );
    pipe.writer().close();
    return simsh::shell::BaseShell().run();
  } else {
    ifstream ifs { argv[1] };
    if ( !ifs ) {
      simsh::iout::logger.print( simsh::error::SystemCallError(
        format("simsh: {}", argv[1] ) ) );
      return EXIT_FAILURE;
    }
    return simsh::shell::BaseShell( simsh::Parser( ifs ) ).run();
  }
}
