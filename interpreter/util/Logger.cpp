#include <iostream>
#include "Logger.hpp"
using namespace std;

namespace hull {
  namespace utils {
    Logger& Logger::operator<<( const error::TraceBack& e ) {
      cerr << e.what() << endl;
      return *this;
    }

    Logger& Logger::inst() noexcept {
      static Logger logger;
      return logger;
    }

    Logger& logger = Logger::inst();
  }
}


