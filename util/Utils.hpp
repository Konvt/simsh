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

    /// @brief A wrapper that helps to convert lambda to a C-style function interface,
    /// @brief which means function pointer.
    /// @tparam F The type of the lambda.
    /// @tparam Tag A tag to distinguish different lambda instances.
    /// @author The origianl version was written by bilibili user Ayano_Aishi
    /// @source https://www.bilibili.com/video/BV1Hm421j7qc
    template<typename, typename, std::size_t Tag>
    class LambdaWrapper;
    template<typename Fn, template<typename...> class List, typename... Args, std::size_t Tag>
    class LambdaWrapper<Fn, List<Args...>, Tag> {
      static_assert( std::is_class_v<Fn>, "Only available to lambda" );
      static_assert( !std::is_empty_v<Fn>, "Only available to lambda with capture" );
      static_assert( std::is_same_v<List<Args...>, std::tuple<Args...>>,
                     "Only accepts std::tuple types" );

      static const Fn* fntor;

      static std::invoke_result_t<Fn, Args...> invoking( Args... args )
      {
        return ( *fntor )( std::forward<Args>( args )... );
      }

      template<typename ParamList, std::size_t InstTag, typename FnTp>
        requires std::conjunction_v<std::is_class<std::remove_reference_t<FnTp>>,
                                    std::negation<std::is_empty<std::remove_reference_t<FnTp>>>>
      friend decltype( &LambdaWrapper<std::remove_reference_t<FnTp>, ParamList, InstTag>::invoking )
        make_fnptr( FnTp&& fn ) noexcept;
    };
    template<typename Fn, template<typename...> class List, typename... Args, std::size_t Tag>
    const Fn* LambdaWrapper<Fn, List<Args...>, Tag>::fntor = nullptr;

    template<typename ParamList = std::tuple<>, std::size_t InstTag = 0, typename FnTp>
      requires std::conjunction_v<std::is_class<std::remove_reference_t<FnTp>>,
                                  std::negation<std::is_empty<std::remove_reference_t<FnTp>>>>
    [[nodiscard]] decltype( &LambdaWrapper<std::remove_reference_t<FnTp>, ParamList, InstTag>::
                              invoking )
      make_fnptr( FnTp&& fn ) noexcept
    {
      using LambdaType  = std::remove_reference_t<FnTp>;
      using WrapperType = LambdaWrapper<LambdaType, ParamList, InstTag>;

      static LambdaType fntor = std::forward<FnTp>( fn );
      if ( WrapperType::fntor == nullptr )
        WrapperType::fntor = std::addressof( fntor );
      return &WrapperType::invoking;
    }
  } // namespace utils
} // namespace simsh

#endif // __SIMSH_UTILS__
