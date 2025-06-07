#ifndef TISH_CONSTANTS
#define TISH_CONSTANTS

#include <limits>
#include <util/Config.hpp>

namespace tish {
  namespace constant {
    constexpr type::Eval invalid_value = std::numeric_limits<type::Eval>::min();
  }
}

#endif
