#ifndef TISH_UTILS
#define TISH_UTILS

#include <functional>
#include <regex>
#include <span>
#include <type_traits>
#include <util/Config.hpp>
#include <utility>

namespace tish {
  namespace util {
    [[nodiscard]] type::String format_char( type::Char character );

    bool create_file( type::StrView filename );

    [[nodiscard]] type::StrView get_homedir();

    [[nodiscard]] std::vector<type::String> get_envpath();

    [[nodiscard]] type::String tilde_expansion( const type::String& token );

    [[nodiscard]] std::pair<bool, std::smatch> match_string( const type::String& str,
                                                             type::StrView reg_str );

    /// @brief Finds the path to the given file in the path set.
    [[nodiscard]] type::String search_filepath( std::span<const type::String> path_set,
                                                type::StrView filename );

    bool rebind_fd( type::FileDesc old_fd, type::FileDesc new_fd ) noexcept;
  } // namespace util

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

  namespace util {
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
  } // namespace util
} // namespace tish

#endif // TISH_UTILS
