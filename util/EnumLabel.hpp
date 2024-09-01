#ifndef __SIMSH_ENUM__
#define __SIMSH_ENUM__

#include "Config.hpp"

namespace simsh {
namespace types {
enum class StmtKind : uint8_t {
  trivial,
  sequential,
  logical_and,
  logical_or,
  logical_not,
  pipeline,
  ovrwrit_redrct,
  appnd_redrct, // >, >>
  merge_output,
  merge_appnd,
  merge_stream, // &>, &>>, >&
  stdin_redrct
};

enum class ExprKind : uint8_t { command, string, value };

enum class TokenType : uint8_t {
  CMD,
  STR,
  AND,
  OR,
  NOT,
  PIPE,
  OVR_REDIR,
  APND_REDIR, // >, >>
  MERG_OUTPUT,
  MERG_APPND,
  MERG_STREAM, // &>, &>>, >&
  STDIN_REDIR, // <
  LPAREN,
  RPAREN,
  NEWLINE,
  SEMI,
  ENDFILE,
  ERROR
};
}
}

#endif // __SIMSH_ENUM__
