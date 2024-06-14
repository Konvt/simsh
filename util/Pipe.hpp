#ifndef __HULL_PIPES__
# define __HULL_PIPES__

#include <type_traits>
#include <concepts>
#include <array>

#include "Config.hpp"

namespace hull {
  namespace utils {
    class PipeReader;
    class PipeWriter;

    /// @brief Anonymous pipes.
    class Pipe {
    public:
      using FDType = int;

    protected:
      static constexpr FDType reader_fd = 0;
      static constexpr FDType writer_fd = 1;

      std::array<FDType, 2> pipefd_;
      bool reader_closed_, writer_closed_;

    public:
      Pipe( const Pipe& ) = delete;
      Pipe& operator=( const Pipe& ) = delete;

      Pipe();
      Pipe( Pipe&& rhs ) noexcept;
      Pipe& operator=( Pipe&& rhs ) noexcept;
      virtual ~Pipe() noexcept;
      PipeReader& reader();
      PipeWriter& writer();
    };

    class PipeReader : public Pipe {
    public:
      PipeReader( const PipeReader& ) = delete;
      PipeReader& operator=( const PipeReader& ) = delete;

      void close();
      Pipe::FDType get() const;
      template<typename T>
        requires std::is_trivially_copyable_v<T>
      T pop() const {
        T value {};
        if ( !reader_closed_ )
          read( pipefd_[reader_fd], &value, sizeof( value ) );
        return value;
      }
    };

    class PipeWriter : public Pipe {
    public:
      PipeWriter( const PipeReader& ) = delete;
      PipeWriter& operator=( const PipeReader& ) = delete;

      void close();
      Pipe::FDType get() const;
      template<typename T>
        requires std::is_trivially_copyable_v<std::decay_t<T>>
      void push( const T& value ) const {
        if ( !writer_closed_ )
          write( pipefd_[writer_fd], &value, sizeof( value ) );
      }
    };

    bool close_blocking( Pipe::FDType fd );
  }

}

#endif // __HULL_PIPES__
