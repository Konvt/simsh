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
    [[nodiscard]] types::StringT format_char( types::CharT character );

    [[nodiscard]] types::StrViewT token_kind_map( TokenType tkn );

    [[nodiscard]] bool create_file( types::StrViewT filename, mode_t mode = 0 );

    types::StrViewT get_homedir();

    void tilde_expansion( types::StringT& token );

    [[nodiscard]] std::pair<bool, std::smatch> match_string( const types::StringT& str, types::StrViewT reg_str );

    /// @brief A wrapper that helps to convert any lambda to a C-style function interface,
    /// @brief which means function pointer.
    /// @tparam F The type of the lambda.
    /// @tparam InstTag A tag to distinguish different lambda instances.
    /// @author Ayano_Aishi
    /// @source https://www.bilibili.com/video/BV1Hm421j7qc
    template<typename F, size_t InstTag = 0>
    struct functor_wrapper {
      static inline const F* ptr = nullptr;

      template<typename... Args>
        requires std::is_invocable_v<F, Args...>
      static std::invoke_result_t<F, Args...> invoking( Args... args ) {
        return std::invoke( *ptr, args... );
      }

      template<typename... Args, size_t = 0>
        requires std::is_invocable_v<F, Args...>
      [[nodiscard]] decltype(&invoking<Args...>) to_fnptr() noexcept {
        return &invoking<Args...>;
      }
    };

    template<size_t InstTag = 0, typename F>
      requires std::is_lvalue_reference_v<std::remove_cv_t<F>>
    [[nodiscard]] functor_wrapper<std::decay_t<F>, InstTag> make_fntor_wrapper( F&& f )
    {
      functor_wrapper<std::decay_t<F>, InstTag>::ptr = std::addressof( f );
      return functor_wrapper<std::decay_t<F>, InstTag>();
    }

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
