#ifndef __SIMSH_CONFIG__
# define __SIMSH_CONFIG__

#include <string>
#include <string_view>
#include <vector>
#include <unordered_set>
#include <cstdint>
#include <cstdlib>

#if defined(_MSC_VER) && defined(_MSVC_LANG) // for msvc
# define __SIMSH_CPP_V__ _MSVC_LANG
#else
# define __SIMSH_CPP_V__ __cplusplus
#endif

#if __SIMSH_CPP_V__ < 202002L
# error "The C++ standard version must be at least C++20."
#endif

namespace simsh {
  namespace type_decl {
    using CharT = int;
    using StringT = std::string;
    using StrViewT = std::string_view;
    using EvalT = int64_t;

    using FDType = int; // file descriptor
    using TokenT = StringT;
  }

  namespace val_decl {
    inline constexpr type_decl::EvalT ExecSuccess = EXIT_SUCCESS;
    inline constexpr type_decl::EvalT EvalSuccess = static_cast<type_decl::EvalT>(true);
    inline const std::unordered_set<type_decl::StringT> internal_command {
      "cd", "exit", "help"
    };
  }
}

#endif // __SIMSH_CONFIG__