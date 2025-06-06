#ifndef TISH_EXCEPTION
#define TISH_EXCEPTION

#include <exception>
#include <format>
#include <ranges>
#include <util/Config.hpp>
#include <util/Utils.hpp>

namespace tish {
  namespace error {
    class TraceBack : public std::exception {
    protected:
      types::String message_;

    public:
      TraceBack( types::String message ) : message_ { std::move( message ) } {}
      virtual const char* what() const noexcept { return message_.c_str(); }
    };

    /// @brief This exception indicates errors in some cpp code.
    class RuntimeError : public TraceBack {
    public:
      RuntimeError( types::String message ) : TraceBack( std::move( message ) ) {}
    };

    class TokenError : public TraceBack {
      TokenError( std::size_t line_pos, types::StrView context, types::StrView message )
        : TraceBack( "\n    " )
      {
        // Trim trailing whitespace from the string_view
        context.remove_suffix(
          std::distance( context.rbegin(),
                         std::ranges::find_if_not( context | std::views::reverse,
                                                   []( char c ) { return std::isspace( c ); } ) ) );
        line_pos = line_pos >= context.size() ? context.size() - 1 : line_pos;

        message_.append( std::format( "{}\n    ", context ) )
          .append( line_pos, '~' )
          .append( std::format( "^\n  {}", message ) );
      }

    public:
      TokenError( std::size_t line_pos,
                  types::StrView context,
                  types::Char expect,
                  types::Char received )
        : TokenError( line_pos,
                      std::move( context ),
                      std::format( "expect {}, but received {}",
                                   utils::format_char( expect ),
                                   utils::format_char( received ) ) )
      {}
      TokenError( std::size_t line_pos,
                  types::StrView context,
                  types::StrView expecting,
                  types::Char received )
        : TokenError(
            line_pos,
            std::move( context ),
            std::format( "expect {}, but received {}", expecting, utils::format_char( received ) ) )
      {}
    };

    class SyntaxError : public TraceBack {
    public:
      SyntaxError( std::size_t line_pos,
                   types::StrView context,
                   types::TokenType expect,
                   types::TokenType found )
        : TraceBack( "\n    " )
      {
        // Trim trailing whitespace from the string_view
        context.remove_suffix(
          std::distance( context.rbegin(),
                         std::ranges::find_if_not( context | std::views::reverse,
                                                   []( char c ) { return std::isspace( c ); } ) ) );
        line_pos = line_pos >= context.size() ? context.size() - 1 : line_pos;

        message_.append( format( "{}\n    ", context ) )
          .append( line_pos, '~' )
          .append( format( "^\n  except {}, but found {}",
                           utils::token_kind_map( expect ),
                           utils::token_kind_map( found ) ) );
      }
    };

    class ArgumentError : public TraceBack {
    public:
      ArgumentError( types::StrView where, types::StrView why )
        : TraceBack( std::format( "{}: {}", where, why ) )
      {}
    };

    class SystemCallError : public TraceBack {
    public:
      SystemCallError( types::String where ) : TraceBack( std::move( where ) ) {}
    };

    class TerminationSignal : public TraceBack {
      types::Eval exit_val_;

    protected:
      TerminationSignal( types::String message, types::Eval exit_val )
        : TraceBack( std::move( message ) ), exit_val_ { exit_val }
      {}

    public:
      TerminationSignal( types::Eval exit_val )
        : TerminationSignal( "current process must be killed", exit_val )
      {}
      types::Eval value() const noexcept { return exit_val_; }
    };

    class StreamClosed : public TerminationSignal {
    public:
      StreamClosed() : TerminationSignal( "target io stream has been closed", EOF ) {}
    };
  } // namespace error
} // namespace tish

#endif // TISH_EXCEPTION
