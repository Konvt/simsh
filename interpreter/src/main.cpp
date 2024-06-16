#include "Parser.hpp"
#include "Exception.hpp"
#include "Logger.hpp"
using namespace std;

int main()
{
  hull::Parser prsr;

  while ( !prsr.empty() ) {
    hull::iout::prmptr << ">>> ";
    try {
      prsr.parse()->evaluate();
    } catch ( const hull::error::TerminationSignal& e ) {
      return e.value();
    } catch ( const hull::error::TraceBack& e ) {
      hull::iout::logger << e;
    }
  }
}
