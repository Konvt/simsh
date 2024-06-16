#ifndef __HULL_LOGGER__
# define __HULL_LOGGER__

#include <iostream>

#include "Config.hpp"
#include "Exception.hpp"

namespace hull {
  namespace iout { // IO Utility
    /// @brief 用于“消耗”程序中所有异常类型的日志输出器
    class Logger {
      type_decl::StringT prefix_;

      Logger() noexcept = default;

    public:
      Logger( const Logger& ) = delete;
      Logger& operator=( const Logger& ) = delete;
      ~Logger() noexcept = default;

      static Logger& inst() noexcept;

      void set_prefix( type_decl::StringT prefix );

      Logger& operator<<( const error::TraceBack& e );
    };

    extern Logger& logger;

    /// @brief 用于向终端输出提示的信息输出器
    class Prompter {
      Prompter() noexcept = default;

    public:
      Prompter( const Logger& ) = delete;
      Prompter& operator=( const Logger& ) = delete;
      ~Prompter() noexcept = default;

      static Prompter& inst() noexcept;

      /// @brief 将 info 输出到 STDOUT 中
      template<typename T>
        requires requires(T info) {
          { std::cout << info };
      } Prompter& operator<<( T&& info ) noexcept {
        std::cout << info;
        return *this;
      }
    };

    extern Prompter& prmptr;
  }
}

#endif // __HULL_LOGGER__