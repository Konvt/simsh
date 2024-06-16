#include <iostream>
#include "Logger.hpp"
using namespace std;

namespace hull {
  namespace utils {
    Logger& Logger::operator<<( const error::TraceBack& e ) {
      if ( prefix_.empty() )
        cerr << e.what() << endl;
      else cerr << (prefix_ + e.what()) << endl;
      return *this;
    }

    void Logger::set_prefix( type_decl::StringT prefix )
    {
      prefix_ = move( prefix );
    }

    Logger& Logger::inst() noexcept {
      static Logger logger;
      return logger;
    }

    Logger& logger = Logger::inst();
  }
}


