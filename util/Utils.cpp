#include <algorithm>
#include <cassert>
#include <climits>
#include <cstdlib>
#include <format>
#include <fstream>
#include <ranges>

#include <pwd.h>
#include <unistd.h>

#include "Utils.hpp"
using namespace std;

namespace simsh {
  namespace utils {
    types::StringT format_char( types::CharT character )
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
        if ( character <= CHAR_MAX && character >= CHAR_MIN && isprint( character ) )
          return format( "'{}'", static_cast<char>( character ) );
        else return format( "\\x{:X}", character );
      }
      }
    }

    types::StrViewT token_kind_map( types::TokenType tkn )
    {
      switch ( tkn ) {
      case types::TokenType::STR:         return "string"sv;
      case types::TokenType::CMD:         return "command"sv;
      case types::TokenType::AND:         return "logical AND"sv;
      case types::TokenType::OR:          return "logical OR"sv;
      case types::TokenType::NOT:         return "logical NOT"sv;
      case types::TokenType::PIPE:        return "pipeline"sv;
      case types::TokenType::OVR_REDIR:   [[fallthrough]];
      case types::TokenType::APND_REDIR:  return "output redirection"sv;
      case types::TokenType::MERG_OUTPUT: [[fallthrough]];
      case types::TokenType::MERG_APPND:  [[fallthrough]];
      case types::TokenType::MERG_STREAM: return "combined redirection"sv;
      case types::TokenType::STDIN_REDIR: return "input redirection"sv;
      case types::TokenType::LPAREN:      return "left paren"sv;
      case types::TokenType::RPAREN:      return "right paren"sv;
      case types::TokenType::NEWLINE:     return "newline"sv;
      case types::TokenType::SEMI:        return "semicolon"sv;
      case types::TokenType::ENDFILE:     return "end of file"sv;
      case types::TokenType::ERROR:       [[fallthrough]];
      default:                            return "error"sv;
      }
    }

    bool create_file( types::StrViewT filename )
    {
      return ofstream( filename.data() ).is_open();
    }

    types::StrViewT get_homedir()
    {
      return getpwuid( getuid() )->pw_dir;
    }

    vector<types::StringT> get_envpath()
    {
      types::StrViewT envpath = getenv( "PATH" );

      vector<types::StringT> ret;
      ranges::transform( envpath | views::split( ':' ), back_inserter( ret ), []( auto&& path ) {
        return types::StringT( ranges::begin( path ), ranges::end( path ) );
      } );
      return ret;
    }

    types::StringT tilde_expansion( const types::StringT& token )
    {
      if ( regex_search( token, regex( "^~(/.*)?$" ) ) )
        return format( "{}{}", get_homedir(), types::StrViewT( token.begin() + 1, token.end() ) );
      return {};
    }

    pair<bool, smatch> match_string( const types::StringT& str, types::StrViewT reg_str )
    {
      regex pattern { reg_str.data() };
      smatch matches;
      bool result = regex_match( str, matches, pattern );
      return { result, move( matches ) };
    }
  } // namespace utils
} // namespace simsh
