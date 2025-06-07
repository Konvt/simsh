#ifndef TISH_CONFIG
#define TISH_CONFIG

#include <cstdint>
#include <string>
#include <string_view>

#if defined( _MSC_VER ) && defined( _MSVC_LANG ) // for msvc
# define TISH_CC_STANDARD _MSVC_LANG
#else
# define TISH_CC_STANDARD __cplusplus
#endif

#if TISH_CC_STANDARD < 202002L
# error "The C++ standard version must be at least C++20."
#endif

#define TISH_VERSION "0.2.0"

namespace tish {
  namespace type {
    using Char     = int;
    using String   = std::string;
    using StrView  = std::string_view;
    using Eval     = int64_t;
    using FileDesc = int; // file descriptor
  } // namespace type
} // namespace tish

#endif // TISH_CONFIG
