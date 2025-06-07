#ifndef TISH_EXCEPTION
#define TISH_EXCEPTION

#include <Tokenizer.hpp>
#include <exception>
#include <format>
#include <ranges>
#include <util/Config.hpp>
#include <util/Util.hpp>

namespace tish {
  namespace error {
    class TraceBack : public std::exception {
    protected:
      type::String message_;

    public:
      TraceBack( type::String&& message ) : message_ { std::move( message ) } {}
      virtual const char* what() const noexcept { return message_.c_str(); }
    };

    /// @brief This exception indicates errors in some cpp code.
    class RuntimeError : public TraceBack {
    public:
      RuntimeError( type::String&& message ) : TraceBack( std::move( message ) ) {}
    };

    class TokenError : public TraceBack {
      TokenError( std::size_t line_pos, type::StrView context, type::StrView message )
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
                  type::StrView context,
                  type::Char expect,
                  type::Char received )
        : TokenError( line_pos,
                      std::move( context ),
                      std::format( "expect {}, but received {}",
                                   util::format_char( expect ),
                                   util::format_char( received ) ) )
      {}
      TokenError( std::size_t line_pos,
                  type::StrView context,
                  type::StrView expecting,
                  type::Char received )
        : TokenError(
            line_pos,
            std::move( context ),
            std::format( "expect {}, but received {}", expecting, util::format_char( received ) ) )
      {}
    };

    class SyntaxError : public TraceBack {
    public:
      SyntaxError( std::size_t line_pos,
                   type::StrView context,
                   Tokenizer::TokenKind expect,
                   Tokenizer::TokenKind found )
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
                           Tokenizer::as_string( expect ),
                           Tokenizer::as_string( found ) ) );
      }
    };

    class ArgumentError : public TraceBack {
    public:
      ArgumentError( type::StrView where, type::StrView why )
        : TraceBack( std::format( "{}: {}", where, why ) )
      {}
    };

    class SystemCallError : public TraceBack {
    public:
      SystemCallError( type::String&& where ) : TraceBack( std::move( where ) ) {}
    };

    class TerminationSignal : public TraceBack {
      type::Eval exit_val_;

    protected:
      TerminationSignal( type::String&& message, type::Eval exit_val )
        : TraceBack( std::move( message ) ), exit_val_ { exit_val }
      {}

    public:
      TerminationSignal( type::Eval exit_val )
        : TerminationSignal( "current process must be killed", exit_val )
      {}
      type::Eval value() const noexcept { return exit_val_; }
    };

    class StreamClosed : public TerminationSignal {
    public:
      StreamClosed() : TerminationSignal( "target io stream has been closed", EOF ) {}
    };
  } // namespace error
} // namespace tish

#endif // TISH_EXCEPTION
