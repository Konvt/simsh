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

#define TISH_MAJOR_V 0L
#define TISH_MINOR_V 2L
#define TISH_PATCH_V 1L

#ifdef NDEBUG
# define TISH_BUILD_MODE "release"
#else
# define TISH_BUILD_MODE "debug"
#endif

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
