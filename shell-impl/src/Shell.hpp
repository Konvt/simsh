#ifndef __SHIMSH_SHELL__
# define __SHIMSH_SHELL__

#include "Config.hpp"

namespace simsh {
  class Shell {
  public:
    static constexpr type_decl::StrViewT default_fmt = "\x1b[32;1m{}@{}\x1b[0m:\x1b[34;1m{}\x1b[0m$ ";
    static constexpr type_decl::StrViewT root_fmt = "{}@{}:{}# ";

  private:
    type_decl::StringT prompt_;

    type_decl::StringT host_name_;
    type_decl::StringT home_dir_;
    type_decl::StringT current_dir_;

    type_decl::StringT user_name_;

    void update_prompt();
    void detect_info();

  public:
    Shell() {
      detect_info();
    }
    ~Shell() = default;

    int run();
  };
}


#endif // __SHIMSH_SHELL__