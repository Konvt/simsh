#include <format>
#include <util/Logger.hpp>
using namespace std;

namespace simsh {
  namespace iout {
    Logger& Logger::operator<<( const error::TraceBack& e )
    {
      if ( prefix_.empty() )
        cerr << e.what() << endl;
      else
        cerr << format( "{}{}\n", prefix_, e.what() );
      return *this;
    }

    Logger& Logger::print( const error::TraceBack& e )
    {
      if ( !prefix_.empty() )
        cerr << prefix_;
      perror( e.what() );
      return *this;
    }

    void Logger::set_prefix( types::String prefix )
    {
      prefix_ = move( prefix );
    }

    Logger& Logger::inst() noexcept
    {
      static Logger logger_;
      return logger_;
    }
  } // namespace iout
} // namespace simsh
