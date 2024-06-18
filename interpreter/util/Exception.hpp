#ifndef __SIMSH_EXCEPTION__
# define __SIMSH_EXCEPTION__

#include <format>
#include <exception>
#include <utility>
#include <optional>
#include <cstdio>

#include "EnumLabel.hpp"
#include "Utils.hpp"

namespace simsh {
  namespace error {
    class TraceBack : public std::exception {
    protected:
      type_decl::StringT message_;

    public:
      TraceBack( type_decl::StringT message )
        : message_ { std::move( message ) } {}
      virtual const char* what() const noexcept { return message_.c_str(); }
    };

    class EvaluateError : public TraceBack {
      std::optional<type_decl::EvalT> result_;

    public:
      EvaluateError( type_decl::StrViewT where, type_decl::StrViewT why,
                     std::optional<type_decl::EvalT> result = std::nullopt )
        : TraceBack( "" ), result_ { std::move( result ) } {
        message_ = result.has_value()
          ? std::format( "{}: {}, return status {}", where, why, *result )
          : std::format( "{}: {}", where, why );
      }
      std::optional<type_decl::EvalT> value() const {
        return result_;
      }
    };

    class TokenError : public TraceBack {
    public:
      TokenError( size_t line_pos, type_decl::CharT expect, type_decl::CharT received )
        : TraceBack( std::format( "in position {}: expect {}, but received {}",
          line_pos, utils::format_char( expect ), utils::format_char( received ) ) ) {}
      TokenError( size_t line_pos, type_decl::StrViewT expecting, type_decl::CharT received )
        : TraceBack( std::format( "in position {}: expect {}, but received {}",
          line_pos, expecting, utils::format_char( received ) ) ) {}
    };

    class SyntaxError : public TraceBack {
    public:
      SyntaxError( size_t line_pos, TokenType expect, TokenType found )
        : TraceBack( std::format( "in position {}: expect {}, but found {}",
          line_pos, utils::token_kind_map( expect ), utils::token_kind_map( found ) ) ) {}
    };

    class ArgumentError : public TraceBack {
    public:
      ArgumentError( type_decl::StrViewT where, type_decl::StrViewT why )
        : TraceBack( std::format( "{}: {}", where, why ) ) {}
    };

    class InitError : public TraceBack {
    public:
      InitError( type_decl::StrViewT where, type_decl::StrViewT why )
        : TraceBack( std::format( "{}: {}", where, why ) ) {}
    };

    class TerminationSignal : public TraceBack {
      type_decl::EvalT exit_val_;

    protected:
      TerminationSignal( type_decl::StringT message, type_decl::EvalT exit_val )
        : TraceBack( std::move( message ) ), exit_val_ { exit_val } {}

    public:
      TerminationSignal( type_decl::EvalT exit_val )
        : TerminationSignal( "current process must be killed", exit_val ) {}
      type_decl::EvalT value() const noexcept { return exit_val_; }
    };

    class StreamClosed : public TerminationSignal {
    public:
      StreamClosed()
        : TerminationSignal( "target io stream has been closed", EOF ) {}
    };
  }
}

#endif // __SIMSH_EXCEPTION__
