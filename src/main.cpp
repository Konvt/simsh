#include <iostream>
#include "Parser.hpp"
#include "Exception.hpp"
#include "Logger.hpp"
using namespace std;

int main()
{
  using std::getline;
  hull::Parser prsr;

  while ( getline( cin, prsr ) ) {
    try {
      auto root = prsr.parse();
      root->evaluate();
    } catch ( const hull::error::ProcessSuicide& e ) {
      return e.value();
    } catch ( const hull::error::TraceBack& e ) {
      hull::utils::logger << e;
    }
  }
}
