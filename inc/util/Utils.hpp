#ifndef __SIMSH_UTILS__
#define __SIMSH_UTILS__

#include <functional>
#include <regex>
#include <span>
#include <type_traits>
#include <util/Config.hpp>
#include <util/Enums.hpp>
#include <utility>

namespace simsh {
  namespace utils {
    [[nodiscard]] types::String format_char( types::Char character );

    [[nodiscard]] constexpr types::StrView token_kind_map( types::TokenType tkn ) noexcept
    {
      switch ( tkn ) {
      case types::TokenType::STR:         return "string";
      case types::TokenType::CMD:         return "command";
      case types::TokenType::AND:         return "logical AND";
      case types::TokenType::OR:          return "logical OR";
      case types::TokenType::NOT:         return "logical NOT";
      case types::TokenType::PIPE:        return "pipeline";
      case types::TokenType::OVR_REDIR:   [[fallthrough]];
      case types::TokenType::APND_REDIR:  return "output redirection";
      case types::TokenType::MERG_OUTPUT: [[fallthrough]];
      case types::TokenType::MERG_APPND:  [[fallthrough]];
      case types::TokenType::MERG_STREAM: return "combined redirection";
      case types::TokenType::STDIN_REDIR: return "input redirection";
      case types::TokenType::LPAREN:      return "left paren";
      case types::TokenType::RPAREN:      return "right paren";
      case types::TokenType::NEWLINE:     return "newline";
      case types::TokenType::SEMI:        return "semicolon";
      case types::TokenType::ENDFILE:     return "end of file";
      case types::TokenType::ERROR:       [[fallthrough]];
      default:                            return "error";
      }
    }

    [[nodiscard]] bool create_file( types::StrView filename );

    [[nodiscard]] types::StrView get_homedir();

    [[nodiscard]] std::vector<types::String> get_envpath();

    [[nodiscard]] types::String tilde_expansion( const types::String& token );

    [[nodiscard]] std::pair<bool, std::smatch> match_string( const types::String& str,
                                                             types::StrView reg_str );

    /// @brief Finds the path to the given file in the path set.
    [[nodiscard]] types::String search_filepath( std::span<const types::StrView> path_set,
                                                 types::StrView filename );
    [[nodiscard]] types::String search_filepath( std::span<const types::String> path_set,
                                                 types::StrView filename );
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
