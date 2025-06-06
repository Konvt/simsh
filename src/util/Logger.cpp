#include <format>
#include <util/Logger.hpp>
using namespace std;

namespace tish {
  namespace iout {
    const Logger& Logger::print( const error::TraceBack& e ) const
    {
      if ( !prefix_.empty() )
        cerr << prefix_;
      perror( e.what() );
      return *this;
    }

    const Logger& operator<<( const Logger& logr, const error::TraceBack& e )
    {
      if ( logr.prefix_.empty() )
        cerr << e.what() << endl;
      else
        cerr << format( "{}{}\n", logr.prefix_, e.what() );
      return logr;
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
} // namespace tish
