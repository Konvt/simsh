#include <format>
#include "Logger.hpp"
using namespace std;

namespace simsh {
  namespace iout {
    Logger& Logger::operator<<( const error::TraceBack& e )
    {
      if ( prefix_.empty() )
        cerr << e.what() << endl;
      else cerr << format( "{}{}{}\n", prefix_, e.what(), suffix_ );
      return *this;
    }

    void Logger::set_prefix( type_decl::StringT prefix )
    {
      prefix_ = move( prefix );
    }

    void Logger::set_suffix( type_decl::StringT suffix )
    {
      suffix_ = move( suffix );
    }

    Logger& Logger::inst() noexcept
    {
      static Logger logger;
      return logger;
    }

    Logger& logger = Logger::inst();

    Prompter& Prompter::inst() noexcept
    {
      static Prompter prompter;
      return prompter;
    }

    Prompter& prmptr = Prompter::inst();
  }
}
