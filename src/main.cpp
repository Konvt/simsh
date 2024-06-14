#include "Parser.hpp"
#include "Exception.hpp"
#include "Logger.hpp"
using namespace std;

int main()
{
  hull::Parser prsr;

  try {
    prsr.parse()->evaluate();
  } catch ( const hull::error::ProcessSuicide& e ) {
    return e.value();
  } catch ( const hull::error::TraceBack& e ) {
    hull::utils::logger << e;
  }
}
