#ifndef __HULL_UTILS__
# define __HULL_UTILS__

#include <concepts>
#include <type_traits>
#include <utility>
#include <regex>

#include <sys/types.h>

#include "Config.hpp"
#include "EnumLabel.hpp"

namespace hull {
  namespace utils {
    type_decl::StringT format_char( type_decl::CharT character );

    type_decl::StrViewT token_kind_map( TokenType tkn );

    bool create_file( type_decl::StrViewT filename, mode_t mode = 0 );

    std::pair<bool, std::smatch> match_string( const type_decl::StringT& str, type_decl::StrViewT reg_str );
  }
}

#endif // __HULL_UTILS__
