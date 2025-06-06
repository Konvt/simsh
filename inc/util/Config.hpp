#ifndef __SIMSH_CONFIG__
#define __SIMSH_CONFIG__

#include <cstdint>
#include <string>
#include <string_view>

#if defined( _MSC_VER ) && defined( _MSVC_LANG ) // for msvc
# define __SIMSH_CPP_V__ _MSVC_LANG
#else
# define __SIMSH_CPP_V__ __cplusplus
#endif

#if __SIMSH_CPP_V__ < 202002L
# error "The C++ standard version must be at least C++20."
#endif

#define SIMSH_VERSION "0.1.1"

namespace simsh {
  namespace types {
    using Char    = int;
    using String  = std::string;
    using StrView = std::string_view;
    using Eval    = int64_t;

    using FileDesc = int; // file descriptor
    using Token = String;
  } // namespace types
} // namespace simsh

#endif // __SIMSH_CONFIG__
