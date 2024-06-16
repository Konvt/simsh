#include <format>
#include <climits>
#include <cassert>

#include <unistd.h>
#include <pwd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "Utils.hpp"
using namespace std;

namespace hull {
  namespace utils {
    type_decl::StringT format_char( type_decl::CharT character )
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

    type_decl::StrViewT token_kind_map( TokenType tkn )
    {
      switch ( tkn ) {
      case TokenType::CMD:         return "string"sv;
      case TokenType::AND:
        [[fallthrough]];
      case TokenType::OR:
        [[fallthrough]];
      case TokenType::NOT:
        [[fallthrough]];
      case TokenType::PIPE:        return "connector"sv;
      case TokenType::OVR_REDIR:
        [[fallthrough]];
      case TokenType::APND_REDIR:
        [[fallthrough]];
      case TokenType::STDIN_REDIR: return "redirection"sv;
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

    bool create_file( type_decl::StrViewT filename, mode_t mode )
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

    type_decl::StringT tilde_expansion( const type_decl::StringT& token )
    {
      if ( regex_search( token, regex( "^~(/.*)?$" ) ) ) {
        const passwd * const pw = getpwuid( getuid() ); // should nerver be freed or deleted here
        if ( pw != nullptr )
          return format( "{}{}", pw->pw_dir, type_decl::StrViewT( token.begin() + 1, token.end() ) );
      }

      return token;
    }

    pair<bool, smatch> match_string( const type_decl::StringT& str, type_decl::StrViewT reg_str )
    {
      regex pattern { reg_str.data() };
      smatch matches;
      bool result = regex_match( str, matches, pattern );
      return { result, move( matches ) };
    }
  }
}
