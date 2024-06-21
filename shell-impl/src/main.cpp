#include <span>
#include <string>
#include <fstream>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "Pipe.hpp"
#include "Parser.hpp"
#include "Logger.hpp"
#include "Exception.hpp"

#include "Shell.hpp"
using namespace std;

int main(int argc, char **argv)
{
  if ( argc == 0 )
    abort();
  else if ( argc == 1 )
    return simsh::Shell().run();

  if ( "-c"sv == argv[1] || argc > 2 ) {
    if (  argc == 2 && "-c"sv == argv[1] ) {
      simsh::iout::logger << simsh::error::ArgumentError(
        "simsh: -c:", "option requires an argument"
      );
      return EXIT_FAILURE;
    }

    simsh::utils::Pipe pipe;
    if ( auto process_id = fork();
         process_id < 0 ) {
      simsh::iout::logger.print( simsh::error::SystemCallError( "fork" ) );
      return EXIT_FAILURE;
    } else if ( process_id == 0 ) {
      pipe.writer().close();
      dup2( pipe.reader().get(), STDIN_FILENO );
      return simsh::BaseShell().run();
    } else {
      const auto args = "-c"sv == argv[1] ? span( argv + 2, argc - 2 ) : span( argv + 1, argc - 1 );
      for ( auto e : args ) {
        pipe.writer().push( e );
        pipe.writer().push( ' ' );
      }
      pipe.writer().push( '\n' );
      pipe.writer().close();

      int status;
      if ( waitpid( process_id, &status, 0 ) < 0 ) {
        simsh::iout::logger.print( simsh::error::SystemCallError( "waitpid" ) );
        return EXIT_FAILURE;
      }
      return WEXITSTATUS( status );
    }
  } else {
    ifstream ifs { argv[1] };
    if ( !ifs ) {
      simsh::iout::logger << simsh::error::SystemCallError(
        format( "{}: {}", argv[1], "could not open the file" ) );
      return EXIT_FAILURE;
    }
    return simsh::BaseShell( simsh::Parser( ifs ) ).run();
  }
}
