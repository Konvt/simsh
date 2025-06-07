#ifndef TISH_HELP
#define TISH_HELP

#include <util/Config.hpp>

namespace tish {
  namespace util {
    inline type::StrView help_doc()
    {
      return { "Single statement:\n\tcommand\n\tcommand;\n"
               "Connect statement:\n\tcommand1 [&& | || | ;] command2\n"
               "Pipeline:\n\tcommand1 | command2\n"
               "Redirection:\n\tcommand [> | >> | &> | &>> | <] filename "
               "[>&]\n\tcommand >&\n"
               "Logical not:\n\t!command\n"
               "Nested statement:\n\t(command1 && (command2 || command3))\n"
               "Comment:\n\tcommand # Here is a comment.\n"
               "Built-in commands:\n\texit\n\thelp\n\tcd path\n\ttype "
               "command-name\n\texec command-name\n" };
    }
  }
} // namespace tish

#endif // TISH_HELP
