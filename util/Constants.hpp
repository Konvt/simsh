#ifndef __SIMSH_CONSTANTS__
#define __SIMSH_CONSTANTS__

#include <cstdint>
#include <cstdlib>
#include <limits>

#include "Config.hpp"

namespace simsh {
  namespace constants {
    inline constexpr types::EvalT ExecSuccess     = EXIT_SUCCESS;
    inline constexpr types::EvalT ExecFailureExit = 114;
    inline constexpr types::EvalT EvalSuccess     = static_cast<types::EvalT>( true );
    inline constexpr types::EvalT InvalidValue    = std::numeric_limits<types::EvalT>::min();
  }
} // namespace simsh

#endif // __SIMSH_CONSTANTS__
