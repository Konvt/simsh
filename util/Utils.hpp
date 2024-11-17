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

    /// @brief A wrapper that helps to convert lambda to a C-style function interface, which means
    /// function pointer.
    /// @author The origianl version was written by a bilibili user Ayano_Aishi
    /// @source https://www.bilibili.com/video/BV1Hm421j7qc
    template<typename>
    class FnPtrMaker;
    template<typename Ret, typename... Params>
    class FnPtrMaker<Ret( Params... )> {
      static_assert( std::is_function_v<Ret( Params... )>, "Only available to function type" );

      template<typename Functor>
      using FnPtrType = std::add_pointer_t<Ret( Params... ) noexcept(
        std::is_nothrow_invocable_v<Functor, Params...> )>;

      template<typename Functor, typename = void>
      constexpr static bool is_empty_lambda_v = false;
      template<typename Functor>
      constexpr static bool is_empty_lambda_v<
        Functor,
        std::enable_if_t<std::conjunction_v<
          std::is_class<Functor>,
          std::is_empty<Functor>,
          std::is_same<decltype( +std::declval<Functor>() ), FnPtrType<Functor>>>>> = true;

    public:
      template<std::size_t InstanceTag, typename Functor>
        requires std::is_class_v<std::decay_t<Functor>>
      static FnPtrType<Functor> from( Functor&& fn ) noexcept
      {
        if constexpr ( is_empty_lambda_v<std::decay_t<Functor>> ) {
          return +fn;
        } else {
          static_assert(
            (std::is_lvalue_reference_v<Functor> && std::is_copy_constructible_v<Functor>)
              || (!std::is_lvalue_reference_v<Functor> && std::is_move_constructible_v<Functor>),
            "Functor must be copy or move constructible" );

          static std::decay_t<Functor> fntor = std::forward<Functor>( fn );
          return +[]( Params... args ) noexcept( std::is_nothrow_invocable_v<Functor, Params...> ) {
            return fntor( std::forward<Params>( args )... );
          };
        }
      }
    };

    template<typename FunctionType, std::size_t InstanceTag = 0, typename Functor>
    constexpr auto make_fnptr( Functor&& fn ) noexcept
    {
      return FnPtrMaker<FunctionType>::template from<InstanceTag>( std::forward<Functor>( fn ) );
    }

  } // namespace utils
} // namespace simsh

#endif // __SIMSH_UTILS__
