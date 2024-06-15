#ifndef __HULL_ENUM__
# define __HULL_ENUM__

#include "Config.hpp"

namespace hull {
  enum class StmtKind {
    trivial, sequential,
    logical_and, logical_or, logical_not, // && || !
    pipeline, ovrwrit_redrct, appnd_redrct, stdin_redrct, merge_redrct, // | > >> <
  };

  enum class ExprKind {
    command
  };

  enum class TokenType {
    CMD,
    AND, OR, NOT, PIPE,
    OVR_REDIR, APND_REDIR, // >, >>
    STDIN_REDIR, MERG_OUTPUT, // <, >&
    LPAREN, RPAREN, NEWLINE, SEMI,
    ENDFILE, ERROR
  };
}

#endif // __HULL_ENUM__
