#include <format>
#include <climits>
#include <cassert>

#include <unistd.h>
#include <pwd.h>
#include <fcntl.h>
#include <sys/stat.h>

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
      default: {
        if ( character <= CHAR_MAX && character >= CHAR_MIN && isprint( character ) )
          return format( "'{}'", static_cast<char>(character) );
        else return format( "\\x{:X}", character );
      }
      }
    }

    types::StrViewT token_kind_map( TokenType tkn )
    {
      switch ( tkn ) {
      case TokenType::STR:         return "string"sv;
      case TokenType::CMD:         return "command"sv;
      case TokenType::AND:         return "logical AND"sv;
      case TokenType::OR:          return "logical OR"sv;
      case TokenType::NOT:         return "logical NOT"sv;
      case TokenType::PIPE:        return "pipeline"sv;
      case TokenType::OVR_REDIR:
        [[fallthrough]];
      case TokenType::APND_REDIR:  return "output redirection"sv;
      case TokenType::MERG_OUTPUT:
        [[fallthrough]];
      case TokenType::MERG_APPND:
        [[fallthrough]];
      case TokenType::MERG_STREAM: return "combined redirection"sv;
      case TokenType::STDIN_REDIR: return "input redirection"sv;
      case TokenType::LPAREN:      return "left paren"sv;
      case TokenType::RPAREN:      return "right paren"sv;
      case TokenType::NEWLINE:     return "newline"sv;
      case TokenType::SEMI:        return "semicolon"sv;
      case TokenType::ENDFILE:     return "end of file"sv;
      case TokenType::ERROR:
        [[fallthrough]];
      default:
        return "error"sv;
      }
    }

    bool create_file( types::StrViewT filename, mode_t mode )
    {
      if ( mode == 0 ) {
        umask( mode = umask( 0 ) );
        mode = ~mode;
      }

      auto fd = creat( filename.data(), mode );
      if ( fd < 0 )
        return false;
      else close( fd );
      return true;
    }

    types::StrViewT get_homedir()
    {
      return getpwuid( getuid() )->pw_dir;
    }

    void tilde_expansion( types::StringT& token )
    {
      if ( regex_search( token, regex( "^~(/.*)?$" ) ) )
        token = format( "{}{}", get_homedir(), types::StrViewT( token.begin() + 1, token.end() ) );
    }

    pair<bool, smatch> match_string( const types::StringT& str, types::StrViewT reg_str )
    {
      regex pattern { reg_str.data() };
      smatch matches;
      bool result = regex_match( str, matches, pattern );
      return { result, move( matches ) };
    }
  }
}
