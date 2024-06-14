#include <iostream>

#include "Logger.hpp"

namespace hull {
  namespace utils {
    Logger& Logger::operator<<( const error::TraceBack& e ) {
      std::cerr << e.what() << std::endl;
      return *this;
    }

    Logger& Logger::inst() {
      static Logger logger;
      return logger;
    }

    Logger& logger = Logger::inst();
  }
}


