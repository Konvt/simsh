#include <format>

#include "Exception.hpp"
#include "Utils.hpp"
using namespace std;

namespace simsh {
  namespace error {
    EvaluateError error_factory( info::EvaluateErrorInfo context )
    {
      auto& [where, why, eval_result] = context;
      if ( eval_result.has_value() )
        return EvaluateError( format( "{}: {}, return status {}",
          where, why, *eval_result ), eval_result );
      else return EvaluateError( format( "{}: {}", where, why ), move( eval_result ) );
    }

    TokenError error_factory( info::TokenErrorInfo context )
    {
      auto& [line_pos, expect, received] = context;
      return TokenError( format( "in position {}: expect {}, but received {}",
        line_pos, utils::format_char( expect ), utils::format_char( received ) ) );
    }

    SyntaxError error_factory( info::SyntaxErrorInfo context )
    {
      auto& [line_pos, expect, found] = context;
      return SyntaxError( format( "in position {}: expect {}, but found {}",
        line_pos, utils::token_kind_map( expect ), utils::token_kind_map( found ) ) );
    }

    ArgumentError error_factory( info::ArgumentErrorInfo context )
    {
      auto& [where, why] = context;
      return ArgumentError( format( "{}: {}", where, why ) );
    }

    InitError error_factory( info::InitErrorInfo context )
    {
      auto& [where, why, causes] = context;
      if ( causes.empty() )
        return InitError( format( "{}: {}", where, why ) );
      else return InitError( format( "{}: {}, attributed to {}", where, why, causes ) );
    }
  }
}
