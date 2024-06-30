#ifndef __SIMSH_CONSTANTS__
# define __SIMSH_CONSTANTS__

#include <unordered_set>
#include <limits>
#include <cstdint>
#include <cstdlib>

#include "Config.hpp"

namespace simsh {
  namespace constants {
    inline constexpr types::EvalT ExecSuccess = EXIT_SUCCESS;
    inline constexpr types::EvalT EvalSuccess = static_cast<types::EvalT>(true);
    inline constexpr types::EvalT InvalidValue = std::numeric_limits<types::EvalT>::min();
    inline const std::unordered_set<types::StringT> built_in_cmds {
      "cd", "exit", "help"
    };
  }
}

#endif // __SIMSH_CONSTANTS__
