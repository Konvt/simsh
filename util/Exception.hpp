#ifndef __SIMSH_EXCEPTION__
# define __SIMSH_EXCEPTION__

#include <exception>
#include <format>
#include <optional>

#include "Config.hpp"
#include "Utils.hpp"

namespace simsh {
  namespace error {
    class TraceBack : public std::exception {
    protected:
      types::StringT message_;

    public:
      TraceBack( types::StringT message )
        : message_ { std::move( message ) } {}
      virtual const char* what() const noexcept { return message_.c_str(); }
    };

    class TokenError : public TraceBack {
      TokenError( types::StrViewT context, types::StrViewT message )
        : TraceBack( "\n    " ) {
        if ( context.back() == '\n' )
          context.remove_suffix( 1 );
        message_.append( std::format( "{}\n    ", context ) )
                .append( types::StringT( context.size() - 1, '~' ) )
                .append( std::format( "^\n  {}", message ) );
      }

    public:
      TokenError( size_t line_pos, types::CharT expect, types::CharT received )
        : TraceBack( std::format( "at position {}: expect {}, but received {}",
          line_pos, utils::format_char( expect ), utils::format_char( received ) ) ) {}
      TokenError( size_t line_pos, types::StrViewT expecting, types::CharT received )
        : TraceBack( std::format( "at position {}: expect {}, but received {}",
          line_pos, expecting, utils::format_char( received ) ) ) {}

      TokenError( types::StrViewT context, types::CharT expect, types::CharT received )
        : TokenError( std::move( context ), std::format( "except {}, but found {}", utils::format_char( expect ), utils::format_char( received ) ) ) {}
      TokenError( types::StrViewT context, types::StrViewT expecting, types::CharT received )
        : TokenError( std::move( context ), std::format( "except {}, but found {}", expecting, utils::format_char( received ) ) ) {}
    };

    class SyntaxError : public TraceBack {
    public:
      SyntaxError( size_t line_pos, TokenType expect, TokenType found )
        : TraceBack( std::format( "at position {}: expect {}, but found {}",
          line_pos, utils::token_kind_map( expect ), utils::token_kind_map( found ) ) ) {}
      SyntaxError( types::StrViewT context, TokenType expect, TokenType found )
        : TraceBack( "\n    " ) {
        if ( context.back() == '\n' )
          context.remove_suffix( 1 );
        message_.append( format( "{}\n    ", context ) )
                .append( types::StringT( context.size() - 1, '~' ) )
                .append( format("^\n  except {}, but found {}", utils::token_kind_map( expect ), utils::token_kind_map( found ) ) );
      }
    };

    class ArgumentError : public TraceBack {
    public:
      ArgumentError( types::StrViewT where, types::StrViewT why )
        : TraceBack( std::format( "{}: {}", where, why ) ) {}
    };

    class SystemCallError : public TraceBack {
    public:
      SystemCallError( types::StringT where )
        : TraceBack( std::move( where ) ) {}
    };

    class TerminationSignal : public TraceBack {
      types::EvalT exit_val_;

    protected:
      TerminationSignal( types::StringT message, types::EvalT exit_val )
        : TraceBack( std::move( message ) ), exit_val_ { exit_val } {}

    public:
      TerminationSignal( types::EvalT exit_val )
        : TerminationSignal( "current process must be killed", exit_val ) {}
      types::EvalT value() const noexcept { return exit_val_; }
    };

    class StreamClosed : public TerminationSignal {
    public:
      StreamClosed()
        : TerminationSignal( "target io stream has been closed", EOF ) {}
    };
  }
}

#endif // __SIMSH_EXCEPTION__
