#ifndef __SIMSH_PIPES__
#define __SIMSH_PIPES__

#include <algorithm>
#include <array>
#include <concepts>
#include <cstring>
#include <ranges>
#include <type_traits>
#include <unistd.h>
#include <util/Config.hpp>
#include <vector>

namespace simsh {
  namespace details {
    template<typename T>
    concept ContiguousContainer = requires( const T& value ) {
      requires std::ranges::contiguous_range<T>;
      { value.size() } -> std::convertible_to<std::size_t>;
    };
  } // namespace details

  namespace utils {
    class PipeReader;
    class PipeWriter;

    /// @brief Anonymous pipes.
    class Pipe {
    protected:
      static constexpr types::FileDesc _reader_fd = 0;
      static constexpr types::FileDesc _writer_fd = 1;

      std::array<types::FileDesc, 2> pipefd_;
      bool reader_closed_, writer_closed_;

    public:
      Pipe( const Pipe& )            = delete;
      Pipe& operator=( const Pipe& ) = delete;

      /// @throw error::SystemCallError If an attempt to create a pipe fails.
      Pipe();
      Pipe( Pipe&& rhs ) noexcept;
      Pipe& operator=( Pipe&& rhs ) noexcept;
      virtual ~Pipe() noexcept;
      PipeReader& reader();
      PipeWriter& writer();
    };

    class PipeReader : public Pipe {
      template<typename T>
      static constexpr bool is_valid_type_v = requires {
        !std::is_void_v<std::remove_pointer_t<std::decay_t<T>>>&& std::is_trivially_copyable_v<T>&&
          std::is_default_constructible_v<std::decay_t<T>>;
      };

    public:
      PipeReader( const PipeReader& )            = delete;
      PipeReader& operator=( const PipeReader& ) = delete;

      void close();
      [[nodiscard]] types::FileDesc get() const;

      template<typename T>
        requires is_valid_type_v<T>
      [[nodiscard]] T pop() const
      {
        T value {};
        if ( !reader_closed_ )
          read( pipefd_[_reader_fd], &value, sizeof( value ) );
        return value;
      }

      template<typename T>
        requires is_valid_type_v<T>
      [[nodiscard]] std::vector<T> pop( std::size_t num ) const
      {
        std::vector<T> values( num );
        if ( !reader_closed_ )
          read( pipefd_[_reader_fd], values.data(), num * sizeof( T ) );
        return values;
      }
    };

    class PipeWriter : public Pipe {
    public:
      PipeWriter( const PipeReader& )            = delete;
      PipeWriter& operator=( const PipeReader& ) = delete;

      void close();
      [[nodiscard]] types::FileDesc get() const;

      /// @brief for any type that isn't a pointer
      template<typename T>
        requires std::conjunction_v<std::negation<std::is_pointer<std::decay_t<T>>>,
                                    std::is_trivially_copyable<std::decay_t<T>>>
      const PipeWriter& push( const T& value ) const
      {
        if ( !writer_closed_ )
          write( pipefd_[_writer_fd], std::addressof( value ), sizeof( value ) );
        return *this;
      }

      /// @brief for raw array
      template<typename ArrT, std::size_t N>
        requires std::is_trivially_copyable_v<ArrT>
      const PipeWriter& push( const ArrT ( &value )[N] ) const
      {
        if ( !writer_closed_ )
          write( pipefd_[_writer_fd], &value, sizeof( value ) );
        return *this;
      }

      /// @brief only for pointer types or dynamic arrays
      template<typename T>
        requires( std::is_trivially_copyable_v<std::remove_cvref_t<T>>
                  && !std::is_same_v<std::decay_t<T>, char*>
                  && !std::is_void_v<std::remove_pointer_t<std::decay_t<T>>> )
      const PipeWriter& push( const T* const value, std::size_t length = 1 ) const
      {
        if ( !writer_closed_ )
          write( pipefd_[_writer_fd], value, length * sizeof( T ) );
        return *this;
      }

      /// @brief for the randomly accessible container type
      template<typename R>
        requires( details::ContiguousContainer<R>
                  && std::is_trivially_copyable_v<std::ranges::range_value_t<R>> )
      const PipeWriter& push( const R& value ) const
      {
        if ( !writer_closed_ )
          write( pipefd_[_writer_fd],
                 value.data(),
                 sizeof( std::ranges::range_value_t<R> ) * value.size() );
        return *this;
      }

      /// @brief for the container type that is not randomly accessible
      template<typename R>
        requires( !details::ContiguousContainer<R>
                  && std::is_trivially_copyable_v<std::ranges::range_value_t<R>> )
      const PipeWriter& push( const R& value ) const
      {
        if ( !writer_closed_ )
          std::ranges::for_each( value, [this]( const auto& ele ) -> void { this->push( ele ); } );
        return *this;
      }

      /// @brief for dynamic `c-style` string
      const PipeWriter& push( const char* const value ) const;
    };

    /// @brief Disable blocking behavior when reading from the specified file
    /// descriptor.
    bool disable_blocking( types::FileDesc fd );
  } // namespace utils
} // namespace simsh

#endif // __SIMSH_PIPES__
