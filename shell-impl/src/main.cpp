#include <span>
#include <string>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "Pipe.hpp"
#include "Shell.hpp"
using namespace std;

int main(int argc, char **argv)
{
  if ( argc == 0 )
    abort();
  else if ( argc == 1 )
    return simsh::Shell().run();

  simsh::utils::Pipe pipe;
  if ( auto process_id = fork();
       process_id < 0 ) {
    perror( "fork" );
    return EXIT_FAILURE;
  } else if ( process_id == 0 ) {
    pipe.writer().close();
    dup2( pipe.reader().get(), STDIN_FILENO );
    return simsh::BaseShell().run();
  } else {
    for ( auto e : span( argv + 1, argc - 1 ) ) {
      pipe.writer().push( e );
      pipe.writer().push( ' ' );
    }
    pipe.writer().push( '\n' );
    pipe.writer().close();

    int status;
    waitpid( process_id, &status, 0 );
    return WEXITSTATUS( status );
  }
}
