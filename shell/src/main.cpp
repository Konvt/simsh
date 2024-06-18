#include <span>
#include "interpreter/src/Parser.hpp"
#include "interpreter/util/Exception.hpp"
#include "interpreter/util/Logger.hpp"
using namespace std;

int main(int argc, char **argv)
{
  simsh::Parser prsr;

  while ( !prsr.empty() ) {
    simsh::iout::prmptr << ">>> ";
    try {
      prsr.parse()->evaluate();
    } catch ( const simsh::error::TerminationSignal& e ) {
      return e.value();
    } catch ( const simsh::error::TraceBack& e ) {
      simsh::iout::logger << e;
    }
  }
}
