#ifndef __SIMSH_HELP__
# define __SIMSH_HELP__

#include "Config.hpp"

namespace simsh {
  namespace utils {
    constexpr inline type_decl::StrViewT help_doc =
      "Single statement:\n\tcommand\n\tcommand;\n"
      "Connect statement:\n\tcommand1 [&& | || | ;] command2\n"
      "Pipeline:\n\tcommand1 | command2\n"
      "Redirection:\n\tcommand [> | >> | &> | &>> | <] filename\n"
      "Logical not:\n\t!command\n"
      "Nested statement:\n\t(command1 && (command2 || comand3))\n"
      "Built-in commands:\n\texit\n\thelp\n\tcd path\n";
  }
}

#endif // __SIMSH_HELP__
