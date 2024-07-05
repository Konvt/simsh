#ifndef __SIMSH_PIPES__
# define __SIMSH_PIPES__

#include <type_traits>
#include <concepts>
#include <array>
#include <ranges>
#include <algorithm>
#include <cstring>

#include "Config.hpp"

namespace simsh {
  namespace utils {
    class PipeReader;
    class PipeWriter;

    /// @brief Anonymous pipes.
    class Pipe {
    protected:
      static constexpr types::FDType reader_fd = 0;
      static constexpr types::FDType writer_fd = 1;

      std::array<types::FDType, 2> pipefd_;
      bool reader_closed_, writer_closed_;

    public:
      Pipe( const Pipe& ) = delete;
      Pipe& operator=( const Pipe& ) = delete;

      /// @throw error::ArgumentError If an attempt to create a pipe fails.
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
      [[nodiscard]] types::FDType get() const;
      template<typename T>
        requires std::conjunction_v<
          std::negation<std::is_void<std::remove_pointer_t<std::decay_t<T>>>>, // forbidden any void pointers
          std::is_trivially_copyable<T>
        >
      [[nodiscard]] T pop() const {
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
      [[nodiscard]] types::FDType get() const;

      /// @brief for any type that isn't a pointer
      template<typename T>
        requires std::conjunction_v<
          std::negation<std::is_pointer<std::decay_t<T>>>,
          std::is_trivially_copyable<std::decay_t<T>>
        >
      void push( const T& value ) const {
        if ( !writer_closed_ )
          write( pipefd_[writer_fd], std::addressof( value ), sizeof( value ) );
      }

      /// @brief for raw array
      template<typename ArrT, size_t N>
        requires std::is_trivially_copyable_v<ArrT>
      void push( const ArrT (&value)[N] ) const {
        if ( !writer_closed_ )
          write( pipefd_[writer_fd], &value, sizeof( value ) );
      }

      /// @brief only for pointer types and dynamic arrays
      template<typename T>
        requires std::conjunction_v<
          std::is_trivially_copyable<std::remove_reference_t<T>>,
          std::negation<std::is_same<std::decay_t<T>, char*>>,
          std::negation<std::is_void<std::remove_pointer_t<std::decay_t<T>>>> // forbidden any void pointers
        >
      void push( const T* const value, size_t length = 1 ) const {
        if ( !writer_closed_ )
          write( pipefd_[writer_fd], value, length * sizeof( T ) );
      }

      /// @brief for the randomly accessible container type that its element type is trivially copyable
      template<template <typename, typename...> typename T, typename U, typename... Args>
        requires requires(const T<U, Args...>& value) {
          { value.data() } -> std::same_as<const U*>;
          { value.size() } -> std::same_as<size_t>;
          std::is_trivially_copyable_v<U>;
      }
      void push( const T<U, Args...>& value ) const {
        if ( !writer_closed_ )
          write( pipefd_[writer_fd], value.data(), sizeof( U ) * value.size() );
      }

      /// @brief for any container type that its underlying element type is trivially copyable
      template<template <typename, typename...> typename T, typename U, typename... Args>
      void push( const T<U, Args...>& value ) const {
        if ( !writer_closed_ )
          std::ranges::for_each( value,
            [this]( const auto& ele ) -> void {
              this->push( ele );
            }
          );
      }

      /// @brief for dynamic `c-style` string
      void push( const char* const value ) const {
        if ( !writer_closed_ )
          write( pipefd_[writer_fd], value, sizeof( char ) * strlen( value ) );
      }
    };

    /// @brief Disable blocking behavior when reading from the specified file descriptor.
    bool close_blocking( types::FDType fd );
  }

}

#endif // __SIMSH_PIPES__
