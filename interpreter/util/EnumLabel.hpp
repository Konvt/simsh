#ifndef __HULL_ENUM__
# define __HULL_ENUM__

#include "Config.hpp"

namespace hull {
  enum class StmtKind {
    trivial, sequential,
    logical_and, logical_or, logical_not, // && || !
    pipeline, ovrwrit_redrct, appnd_redrct, stdin_redrct, // | > >> <
    merge_output, merge_appnd
  };

  enum class ExprKind {
    command, string
  };

  enum class TokenType {
    CMD, STR,
    AND, OR, NOT, PIPE,
    OVR_REDIR, APND_REDIR, MERG_OUTPUT, MERG_APPND, // >, >>, &>, &>>
    STDIN_REDIR,
    LPAREN, RPAREN, NEWLINE, SEMI,
    ENDFILE, ERROR
  };
}

#endif // __HULL_ENUM__
