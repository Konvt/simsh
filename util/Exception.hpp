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

    /// @brief This exception indicates errors in some cpp code.
    class RuntimeError : public TraceBack {
    public:
      RuntimeError( types::StringT message )
        : TraceBack( std::move( message ) ) {}
    };

    class TokenError : public TraceBack {
      TokenError( size_t line_pos, types::StrViewT context, types::StrViewT message )
        : TraceBack( "\n    " ) {
        line_pos -= line_pos == context.size();
        context.remove_suffix( 1 );
        message_.append( std::format( "{}\n    ", context ) )
                .append( types::StringT( line_pos, '~' ) )
                .append( std::format( "^\n  {}", message ) );
      }

    public:
      TokenError( size_t line_pos, types::StrViewT context, types::CharT expect, types::CharT received )
        : TokenError( line_pos, std::move( context ), std::format( "expect {}, but received {}",
            utils::format_char( expect ), utils::format_char( received ) ) ) {}
      TokenError( size_t line_pos, types::StrViewT context, types::StrViewT expecting, types::CharT received )
        : TokenError( line_pos, std::move( context ), std::format( "expect {}, but received {}",
            expecting, utils::format_char( received ) ) ) {}
    };

    class SyntaxError : public TraceBack {
    public:
      SyntaxError( size_t line_pos, types::StrViewT context, types::TokenType expect, types::TokenType found )
        : TraceBack( "\n    " ) {
        line_pos -= line_pos == context.size();
        context.remove_suffix( 1 );
        message_.append( format( "{}\n    ", context ) )
                .append( types::StringT( line_pos, '~' ) )
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
