#ifndef __HULL_EXCEPTION__
# define __HULL_EXCEPTION__

#include <exception>
#include <utility>
#include <optional>
#include <cstdio>

#include "EnumLabel.hpp"

namespace hull {
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
      EvaluateError( type_decl::StringT message,
                     std::optional<type_decl::EvalT> result = std::nullopt )
        : TraceBack( std::move( message ) ), result_ { std::move( result ) } {}
      std::optional<type_decl::EvalT> value() const {
        return result_;
      }
    };

    class TokenError : public TraceBack {
    public:
      TokenError( type_decl::StringT message )
        : TraceBack( std::move( message ) ) {}
    };

    class SyntaxError : public TraceBack {
    public:
      SyntaxError( type_decl::StringT message )
        : TraceBack( std::move( message ) ) {}
    };

    class ArgumentError : public TraceBack {
    public:
      ArgumentError( type_decl::StringT message )
        : TraceBack( std::move( message ) ) {}
    };

    class InitError : public TraceBack {
    public:
      InitError( type_decl::StringT message )
        : TraceBack( std::move( message ) ) {}
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

    namespace info {
      using EvaluateErrorInfo = // where, why, eval_result
        std::tuple<type_decl::StrViewT, type_decl::StrViewT, std::optional<type_decl::EvalT>>;

      using TokenErrorInfo = // line_pos, expect, received
        std::tuple<size_t, type_decl::CharT, type_decl::CharT>;

      using SyntaxErrorInfo = // line_pos, expect, found
        std::tuple<size_t, TokenType, TokenType>;

      using ArgumentErrorInfo = // where, why
        std::tuple<type_decl::StrViewT, type_decl::StrViewT>;

      using InitErrorInfo = // where, why, causes or contexts
        std::tuple<type_decl::StrViewT, type_decl::StrViewT, type_decl::StringT>;
    }

    EvaluateError error_factory( info::EvaluateErrorInfo context );

    TokenError error_factory( info::TokenErrorInfo context );

    SyntaxError error_factory( info::SyntaxErrorInfo context );

    ArgumentError error_factory( info::ArgumentErrorInfo context );

    InitError error_factory( info::InitErrorInfo context );
  }
}

#endif // __HULL_EXCEPTION__
