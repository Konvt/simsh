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
    using CharT    = int;
    using StringT  = std::string;
    using StrViewT = std::string_view;
    using EvalT    = int64_t;

    using FDType = int; // file descriptor
    using TokenT = StringT;
  } // namespace types
} // namespace simsh

#endif // __SIMSH_CONFIG__
