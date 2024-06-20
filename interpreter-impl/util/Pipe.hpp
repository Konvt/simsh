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
      static constexpr type_decl::FDType reader_fd = 0;
      static constexpr type_decl::FDType writer_fd = 1;

      std::array<type_decl::FDType, 2> pipefd_;
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
      type_decl::FDType get() const;
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
      type_decl::FDType get() const;

      void push( const char* const value ) const {
        if ( !writer_closed_ )
          write( pipefd_[writer_fd], value, sizeof( char ) * strlen( value ) );
      }

      template<typename T>
        requires std::conjunction_v<
          std::negation<std::is_same<std::decay_t<T>, char*>>,
          std::is_trivially_copyable<std::decay_t<T>>
        >
      void push( const T& value ) const {
        if ( !writer_closed_ )
          write( pipefd_[writer_fd], std::addressof( value ), sizeof( value ) );
      }

      template<template <typename> typename T, typename U>
        requires std::is_trivially_copyable_v<U>
      void push( const T<U>& value )const {
        if ( !writer_closed_ )
          std::ranges::for_each( value, [this]( const auto& ele ) -> void {
              this->push( ele );
            }
          );
      }
    };

    bool close_blocking( type_decl::FDType fd );
  }

}

#endif // __SIMSH_PIPES__
