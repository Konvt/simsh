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
  } // namespace utils

  namespace details {
    template<typename QualifierInfo, typename Ret, typename... Args>
    class FnAdapter {
      template<typename Qualifier, typename T>
        requires( !std::is_reference_v<Qualifier> )
      static constexpr std::
        conditional_t<std::is_const_v<std::remove_reference_t<Qualifier>>, const T&&, T&&>
        callee_fwd( T&& param ) noexcept
      {
        return std::forward<T>( param );
      }
      template<typename Qualifier, typename T>
        requires std::is_lvalue_reference_v<Qualifier>
      static constexpr std::conditional_t<(std::is_const_v<std::remove_reference_t<Qualifier>>
                                           || std::is_rvalue_reference_v<T&&>),
                                          const std::remove_reference_t<T>&,
                                          std::remove_reference_t<T>&>
        callee_fwd( T&& param ) noexcept
      {
        return param;
      }
      template<typename Qualifier, typename T>
        requires std::is_rvalue_reference_v<Qualifier>
      static constexpr std::conditional_t<std::is_const_v<std::remove_reference_t<Qualifier>>,
                                          const std::remove_reference_t<T>&&,
                                          std::remove_reference_t<T>>&&
        callee_fwd( T&& param ) noexcept
      {
        return std::move( param );
      }

      template<typename Fn>
      using Callee_t = decltype( callee_fwd<QualifierInfo>( std::declval<std::decay_t<Fn>>() ) );

    public:
      template<auto = [] {}, typename Fn>
        requires( std::is_invocable_r_v<Ret, Callee_t<Fn>, Args...>
                  && std::is_constructible_v<std::decay_t<Fn>, Fn> )
      [[nodiscard]] static auto from( Fn&& fn )
        noexcept( std::is_nothrow_constructible_v<std::decay_t<Fn>, Fn> )
      {
        static std::decay_t<Fn> fntor = std::forward<Fn>( fn );
        return +[]( Args... args ) noexcept(
                  std::is_nothrow_invocable_r_v<Ret, Callee_t<Fn>, Args...> ) -> Ret {
          return std::invoke( callee_fwd<QualifierInfo>( fntor ), std::forward<Args>( args )... );
        };
      }
    };
  } // namespace details

  namespace utils {
    /// @brief A wrapper that helps to convert lambda to a C-style function interface, which means
    /// function pointer.
    template<typename... Signature>
    struct CFnCast;
    template<typename Ret, typename... Args>
    struct CFnCast<Ret( Args... )> : public details::FnAdapter<int, Ret, Args...> {};
    template<typename Ret, typename... Args>
    struct CFnCast<Ret( Args... )&> : public details::FnAdapter<int&, Ret, Args...> {};
    template<typename Ret, typename... Args>
    struct CFnCast<Ret( Args... ) &&> : public details::FnAdapter<int&&, Ret, Args...> {};
    template<typename Ret, typename... Args>
    struct CFnCast<Ret( Args... ) const> : public details::FnAdapter<const int, Ret, Args...> {};
    template<typename Ret, typename... Args>
    struct CFnCast<Ret( Args... ) const&> : public details::FnAdapter<const int&, Ret, Args...> {};
    template<typename Ret, typename... Args>
    struct CFnCast<Ret( Args... ) const&&>
      : public details::FnAdapter<const int&&, Ret, Args...> {};

    template<typename Signature, typename Fn>
    [[nodiscard]] auto make_fnptr( Fn&& fn )
      noexcept( std::is_nothrow_constructible_v<std::decay_t<Fn>, Fn> )
    {
      return CFnCast<Signature>::from( std::forward<Fn>( fn ) );
    }
  } // namespace utils
} // namespace simsh

#endif // __SIMSH_UTILS__
