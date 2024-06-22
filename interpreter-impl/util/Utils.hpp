#ifndef __SIMSH_UTILS__
# define __SIMSH_UTILS__

#include <concepts>
#include <type_traits>
#include <utility>
#include <regex>
#include <functional>

#include <sys/types.h>
#include <signal.h>

#include "Config.hpp"
#include "EnumLabel.hpp"

namespace simsh {
  namespace utils {
    type_decl::StringT format_char( type_decl::CharT character );

    type_decl::StrViewT token_kind_map( TokenType tkn );

    bool create_file( type_decl::StrViewT filename, mode_t mode = 0 );

    void tilde_expansion( type_decl::StringT& token );

    std::pair<bool, std::smatch> match_string( const type_decl::StringT& str, type_decl::StrViewT reg_str );

    // declare function_traits struct
    template<typename Func>
    struct function_traits;

    // specialized for function pointer
    template<typename R, typename... Args>
    struct function_traits<R( * )(Args...)> {
      using result_type = R;
      using arguments = std::tuple<Args...>;
    };

    // specialized for member function
    template<typename Class, typename R, typename... Args>
    struct function_traits<R( Class::* )(Args...)> {
      using result_type = R;
      using arguments = std::tuple<Args...>;
    };
    template<typename Class, typename R, typename... Args>
    struct function_traits<R( Class::* )(Args...) const> {
      using result_type = R;
      using arguments = std::tuple<Args...>;
    };
    template<typename Class, typename R, typename... Args>
    struct function_traits<R( Class::* )(Args...) volatile> {
      using result_type = R;
      using arguments = std::tuple<Args...>;
    };
    template<typename Class, typename R, typename... Args>
    struct function_traits<R( Class::* )(Args...) const volatile> {
      using result_type = R;
      using arguments = std::tuple<Args...>;
    };

    template<typename Func> // for functor object
    struct function_traits : public function_traits<decltype(&Func::operator())> {};
  }
}

#endif // __SIMSH_UTILS__
