#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <limits>
#include <pwd.h>
#include <ranges>
#include <unistd.h>
#include <util/Config.hpp>
#include <util/Utils.hpp>
using namespace std;

namespace tish {
  namespace utils {
    types::String format_char( types::Char character )
    {
      switch ( character ) {
      case '\0': return "'\\0'";
      case '\a': return "'\\a'";
      case '\b': return "'\\b'";
      case '\t': return "'\\t'";
      case '\n': return "'\\n'";
      case '\v': return "'\\v'";
      case '\f': return "'\\f'";
      case '\r': return "'\\r'";
      default:   {
        if ( character <= numeric_limits<char>::max() && character >= numeric_limits<char>::min()
             && isprint( character ) )
          return format( "'{}'", static_cast<char>( character ) );
        else
          return format( "\\x{:X}", character );
      }
      }
    }

    bool create_file( types::StrView filename )
    {
      return ofstream( filename.data() ).is_open();
    }

    types::StrView get_homedir()
    {
      return getpwuid( getuid() )->pw_dir;
    }

    vector<types::String> get_envpath()
    {
      types::StrView envpath = getenv( "PATH" );

      vector<types::String> ret;
      ranges::transform( envpath | views::split( ':' ), back_inserter( ret ), []( auto&& path ) {
        return types::String( ranges::begin( path ), ranges::end( path ) );
      } );
      return ret;
    }

    types::String tilde_expansion( const types::String& token )
    {
      if ( regex_search( token, regex( "^~(/.*)?$" ) ) )
        return format( "{}{}", get_homedir(), types::StrView( token.begin() + 1, token.end() ) );
      return {};
    }

    pair<bool, smatch> match_string( const types::String& str, types::StrView reg_str )
    {
      regex pattern { reg_str.data() };
      smatch matches;
      bool result = regex_match( str, matches, pattern );
      return { result, move( matches ) };
    }

    types::String search_filepath( std::span<const types::String> path_set,
                                   types::StrView filename )
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

    bool rebind_fd( types::FileDesc old_fd, types::FileDesc new_fd ) noexcept
    {
      return dup2( old_fd, new_fd ) == -1;
    }
  } // namespace utils
} // namespace tish
