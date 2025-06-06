#include <cassert>
#include <fcntl.h>
#include <unistd.h>
#include <util/Exception.hpp>
#include <util/Pipe.hpp>
using namespace std;

namespace simsh {
  namespace utils {
    Pipe::Pipe() : pipefd_ {}, reader_closed_ { false }, writer_closed_ { false }
    {
      if ( pipe( pipefd_.data() ) < 0 )
        throw error::SystemCallError( "pipe" );
    }

    Pipe::Pipe( Pipe&& rhs ) noexcept
      : pipefd_ {}, reader_closed_ { false }, writer_closed_ { false }
    {
      using std::swap;
      swap( pipefd_, rhs.pipefd_ );
      swap( reader_closed_, rhs.reader_closed_ );
      swap( writer_closed_, rhs.writer_closed_ );
    }

    Pipe& Pipe::operator=( Pipe&& rhs ) noexcept
    {
      using std::swap;
      swap( pipefd_, rhs.pipefd_ );
      swap( reader_closed_, rhs.reader_closed_ );
      swap( writer_closed_, rhs.writer_closed_ );
      return *this;
    }

    Pipe::~Pipe() noexcept
    {
      if ( !reader_closed_ )
        ::close( pipefd_[_reader_fd] );
      if ( !writer_closed_ )
        ::close( pipefd_[_writer_fd] );
    }

    PipeReader& Pipe::reader()
    {
      static_assert( sizeof( PipeReader ) == sizeof( Pipe ), "Invalid reader interface type." );
      return static_cast<PipeReader&>( *this );
    }

    PipeWriter& Pipe::writer()
    {
      static_assert( sizeof( PipeWriter ) == sizeof( Pipe ), "Invalid writer interface type." );
      return static_cast<PipeWriter&>( *this );
    }

    void PipeReader::close()
    {
      if ( !reader_closed_ ) {
        reader_closed_ = true;
        ::close( pipefd_[_reader_fd] );
      }
    }

    types::FileDesc PipeReader::get() const
    {
      return pipefd_[_reader_fd];
    }

    void PipeWriter::close()
    {
      if ( !writer_closed_ ) {
        writer_closed_ = true;
        ::close( pipefd_[writer_closed_] );
      }
    }

    types::FileDesc PipeWriter::get() const
    {
      return pipefd_[_writer_fd];
    }

    const PipeWriter& PipeWriter::push( const char* const value ) const
    {
      if ( !writer_closed_ )
        write( pipefd_[_writer_fd], value, sizeof( char ) * strlen( value ) );
      return *this;
    }

    bool disable_blocking( types::FileDesc fd )
    {
      return fcntl( fd, F_SETFL, fcntl( fd, F_GETFL ) | O_NONBLOCK ) > 0;
    }
  } // namespace utils
} // namespace simsh
