#ifndef __HULL_LOGGER__
# define __HULL_LOGGER__

#include "Config.hpp"
#include "Exception.hpp"

namespace hull {
  namespace utils {
    /// @brief 用于“消耗”程序中所有异常类型的无状态日志输出器
    class Logger {
      Logger() noexcept  = default;

    public:
      Logger( const Logger& ) = delete;
      Logger& operator=( const Logger& ) = delete;
      ~Logger() noexcept = default;

      static Logger& inst() noexcept;

      Logger& operator<<( const error::TraceBack& e );
    };

    extern Logger& logger;
  }
}

#endif // __HULL_LOGGER__