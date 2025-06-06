#ifndef __SIMSH_LOGGER__
#define __SIMSH_LOGGER__

#include <iostream>

#include <util/Config.hpp>
#include <util/Exception.hpp>

namespace simsh {
  namespace iout { // IO Utility
    /// @brief Log output used to "consume" all exception types in the program,
    /// output to stderr.
    class Logger {
      types::String prefix_;

      Logger() noexcept = default;

    public:
      Logger( const Logger& )            = delete;
      Logger& operator=( const Logger& ) = delete;
      ~Logger() noexcept                 = default;

      static Logger& inst() noexcept;

      [[nodiscard]] types::StrView prefix() const& noexcept { return prefix_; }
      [[nodiscard]] types::String&& prefix() && noexcept { return std::move( prefix_ ); }

      /// @brief Set a string prefix that comes with each output, which defaults
      /// to empty.
      void set_prefix( types::String prefix );

      /// @brief Print the exception via `perror`.
      const Logger& print( const error::TraceBack& e ) const;

      friend const Logger& operator<<( const Logger& logr, const error::TraceBack& e );
    };

    /// @brief Print the exception information via `std::cerr`.
    const Logger& operator<<( const Logger& logr, const error::TraceBack& e );

    /// @brief Log output used to "consume" all exception types in the program,
    /// output to stderr.
    inline Logger& logger = Logger::inst();

    /// @brief An information output class used to output info to stdout.
    inline auto& prmptr = std::cout;
  } // namespace iout
} // namespace simsh

#endif // __SIMSH_LOGGER__
