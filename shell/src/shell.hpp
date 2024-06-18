#ifndef __SHIMSH_SHELL__
# define __SHIMSH_SHELL__

#include "interpreter/util/Config.hpp"

namespace simsh {
  class shell {
    type_decl::StringT prompt_;

  public:
    shell() = default;

  };
}


#endif // __SHIMSH_SHELL__
