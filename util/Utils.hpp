#ifndef __SIMSH_UTILS__
#define __SIMSH_UTILS__

#include <concepts>
#include <filesystem>
#include <functional>
#include <regex>
#include <span>
#include <type_traits>
#include <utility>

#include "Config.hpp"
#include "EnumLabel.hpp"

namespace simsh {
  namespace utils {
    [[nodiscard]] types::StringT format_char( types::CharT character );

    [[nodiscard]] types::StrViewT token_kind_map( types::TokenType tkn );

    [[nodiscard]] bool create_file( types::StrViewT filename );

    [[nodiscard]] types::StrViewT get_homedir();

    [[nodiscard]] std::vector<types::StringT> get_envpath();

    [[nodiscard]] types::StringT tilde_expansion( const types::StringT& token );

    [[nodiscard]] std::pair<bool, std::smatch> match_string( const types::StringT& str,
                                                             types::StrViewT reg_str );

    /// @brief Finds the path to the given file in the path set.
    template<typename T>
      requires requires( std::decay_t<T> t ) {
        { std::filesystem::path( t ) } -> std::same_as<std::filesystem::path>;
      }
    [[nodiscard]] types::StringT search_filepath( std::span<const T> path_set,
                                                  types::StrViewT filename )
    {
      for ( const auto& env_path : path_set ) {
        try {
          if ( const auto filepath = std::filesystem::path( env_path ) / filename;
               std::filesystem::exists( filepath ) && std::filesystem::is_regular_file( filepath ) )
            return filepath.string();
        } catch ( const std::filesystem::filesystem_error& e ) {
          // just ignore it
        }
      }
      return {};
    }

    /// @brief A wrapper that helps to convert any lambda to a C-style function
    /// interface,
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
      static std::invoke_result_t<F, Args...> invoking( Args... args )
      {
        return std::invoke( *ptr, args... );
      }

      template<typename... Args, size_t = 0>
        requires std::is_invocable_v<F, Args...>
      [[nodiscard]] decltype( &invoking<Args...> ) to_fnptr() noexcept
      {
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
    struct function_traits<R ( * )( Args... )> {
      using result_type = R;
      using arguments   = std::tuple<Args...>;
    };

    // specialized for member function
    template<class Class, typename R, typename... Args>
    struct function_traits<R ( Class::* )( Args... )> {
      using result_type = R;
      using arguments   = std::tuple<Args...>;
    };
    template<class Class, typename R, typename... Args>
    struct function_traits<R ( Class::* )( Args... ) const> {
      using result_type = R;
      using arguments   = std::tuple<Args...>;
    };
    template<class Class, typename R, typename... Args>
    struct function_traits<R ( Class::* )( Args... ) volatile> {
      using result_type = R;
      using arguments   = std::tuple<Args...>;
    };
    template<class Class, typename R, typename... Args>
    struct function_traits<R ( Class::* )( Args... ) const volatile> {
      using result_type = R;
      using arguments   = std::tuple<Args...>;
    };

    template<typename Func> // for functor object
    struct function_traits : public function_traits<decltype( &Func::operator() )> {};
  } // namespace utils
} // namespace simsh

#endif // __SIMSH_UTILS__
