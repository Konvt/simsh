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
      if ( !logr.prefix_.empty() )
        cerr << logr.prefix_;
      cerr << e.what() << endl;
      return logr;
    }
    const Logger& operator<<( const Logger& logr, type::StrView info )
    {
      if ( !logr.prefix_.empty() )
        cerr << logr.prefix_;
      cerr << info << endl;
      return logr;
    }

    void Logger::set_prefix( type::String prefix )
    {
      prefix_ = move( prefix );
    }

    Logger& Logger::inst() noexcept
    {
      static Logger logger_;
      return logger_;
    }

    Logger& logger       = Logger::inst();
    std::ostream& prmptr = std::cout;
  } // namespace iout
} // namespace tish
