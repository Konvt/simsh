#ifndef TISH_CONSTANTS
#define TISH_CONSTANTS

#include <cstdlib>
#include <limits>
#include <util/Config.hpp>

namespace tish {
  namespace constants {
    inline constexpr types::Eval ExecSuccess     = EXIT_SUCCESS;
    inline constexpr types::Eval ExecFailureExit = 114;
    inline constexpr types::Eval EvalSuccess     = static_cast<types::Eval>( true );
    inline constexpr types::Eval InvalidValue    = std::numeric_limits<types::Eval>::min();
  }
} // namespace tish

#endif // TISH_CONSTANTS
