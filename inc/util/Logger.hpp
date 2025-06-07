#ifndef TISH_LOGGER
#define TISH_LOGGER

#include <iostream>

#include <util/Config.hpp>
#include <util/Exception.hpp>

namespace tish {
  namespace iout { // IO Utility
    /// @brief Log output used to "consume" all exception types in the program,
    /// output to stderr.
    class Logger {
      type::String prefix_;

      Logger() noexcept = default;

    public:
      Logger( const Logger& )            = delete;
      Logger& operator=( const Logger& ) = delete;
      ~Logger() noexcept                 = default;

      static Logger& inst() noexcept;

      [[nodiscard]] type::StrView prefix() const& noexcept { return prefix_; }
      [[nodiscard]] type::String&& prefix() && noexcept { return std::move( prefix_ ); }

      /// @brief Set a string prefix that comes with each output, which defaults
      /// to empty.
      void set_prefix( type::String prefix );

      /// @brief Print the exception via `perror`.
      const Logger& print( const error::TraceBack& e ) const;

      friend const Logger& operator<<( const Logger& logr, const error::TraceBack& e );
      friend const Logger& operator<<( const Logger& logr, type::StrView info );
    };

    /// @brief Print the exception information via `std::cerr`.
    const Logger& operator<<( const Logger& logr, const error::TraceBack& e );
    const Logger& operator<<( const Logger& logr, type::StrView info );

    /// @brief Log output used to "consume" all exception type in the program,
    /// output to stderr.
    extern Logger& logger;

    /// @brief An information output class used to output info to stdout.
    extern std::ostream& prmptr;
  } // namespace iout
} // namespace tish

#endif // TISH_LOGGER
