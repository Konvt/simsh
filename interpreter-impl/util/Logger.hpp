#ifndef __SIMSH_LOGGER__
# define __SIMSH_LOGGER__

#include <iostream>

#include "Config.hpp"
#include "Exception.hpp"

namespace simsh {
  namespace iout { // IO Utility
    /// @brief Log output used to "consume" all exception types in the program, output to stderr.
    class Logger {
      type_decl::StringT prefix_;

      Logger() noexcept = default;

    public:
      Logger( const Logger& ) = delete;
      Logger& operator=( const Logger& ) = delete;
      ~Logger() noexcept = default;

      static Logger& inst() noexcept;

      type_decl::StrViewT prefix() const noexcept { return prefix_; }

      /// @brief Set a string prefix that comes with each output, which defaults to empty.
      void set_prefix( type_decl::StringT prefix );

      Logger& operator<<( const error::TraceBack& e );
      /// @brief Print the exception via `perror`.
      Logger& print( const error::TraceBack& e );
    };

    /// @brief Log output used to "consume" all exception types in the program, output to stderr.
    extern Logger& logger;

    /// @brief An information output class used to output info to stdout.
    class Prompter {
      Prompter() noexcept = default;

    public:
      Prompter( const Logger& ) = delete;
      Prompter& operator=( const Logger& ) = delete;
      ~Prompter() noexcept = default;

      static Prompter& inst() noexcept;

      /// @brief Output anything that can be inserted to `std::cout`.
      template<typename T>
        requires requires(T info) {
          { std::cout << info };
      } Prompter& operator<<( T&& info ) noexcept {
        std::cout << info << std::flush;
        return *this;
      }
    };

    /// @brief An information output class used to output info to stdout.
    extern Prompter& prmptr;
  }
}

#endif // __SIMSH_LOGGER__
