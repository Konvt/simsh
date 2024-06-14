#ifndef __HULL_UTILS__
# define __HULL_UTILS__

#include <concepts>
#include <type_traits>
#include <sys/types.h>

#include "Config.hpp"
#include "EnumLabel.hpp"
#include "Pipe.hpp"

namespace hull {
  namespace utils {
    type_decl::StringT format_char( type_decl::CharT character );

    type_decl::StrViewT token_kind_map( TokenType tkn );

    bool create_file( const type_decl::StrViewT filename, mode_t mode = 0 );

    template<typename E>
      requires std::is_enum_v<std::decay_t<E>>
    constexpr size_t enum_index( E&& val ) noexcept {
      return static_cast<size_t>( val );
    }
  }
}

#endif // __HULL_UTILS__
