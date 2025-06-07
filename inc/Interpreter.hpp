#ifndef TISH_INTERPRETER
#define TISH_INTERPRETER

#include <TreeNode.hpp>
#include <cstdlib>
#include <unordered_set>
#include <util/Config.hpp>
#include <util/Constant.hpp>
#include <vector>

namespace tish {
  /// @brief An interpreter for executing syntax tree.
  class Interpreter {
  public:
    struct EvalResult {
      static constexpr type::Eval success = EXIT_SUCCESS;
      static constexpr type::Eval abort   = 127;

      std::vector<type::Eval> side_val_;
      type::Eval value_;

      EvalResult( type::Eval value = constant::invalid_value ) noexcept
        : side_val_ {}, value_ { value }
      {}
      [[nodiscard]] explicit operator bool() const noexcept { return value_ == success; }
    };

  private:
    using StmtNodeT = StmtNode* const;
    using ExprNodeT = ExprNode* const;
    static const std::unordered_set<type::String> _built_in_cmds;

    [[nodiscard]] EvalResult sequential_stmt( StmtNodeT seq_stmt ) const;
    [[nodiscard]] EvalResult logical_and( StmtNodeT and_stmt ) const;
    [[nodiscard]] EvalResult logical_or( StmtNodeT or_stmt ) const;
    [[nodiscard]] EvalResult logical_not( StmtNodeT not_stmt ) const;
    [[nodiscard]] EvalResult pipeline_stmt( StmtNodeT pipeline_stmt ) const;
    [[nodiscard]] EvalResult output_redirection( StmtNodeT oup_redr ) const;
    [[nodiscard]] EvalResult merge_stream( StmtNodeT merg_redr ) const;
    [[nodiscard]] EvalResult input_redirection( StmtNodeT inp_redr ) const;

    [[nodiscard]] EvalResult atom( ExprNodeT expr ) const;

    /// @brief Internal instruction execution, not cross-process.
    [[nodiscard]] EvalResult builtin_exec( ExprNodeT expr ) const;

    /// @brief Execute the expression, and return 0 or 1 (a boolean),
    /// indicating whether the expression was successful.
    /// @brief The 'successful' means that the return value of child process was `EXIT_SUCCESS`.
    [[nodiscard]] EvalResult external_exec( ExprNodeT expr ) const;

  public:
    Interpreter()  = default;
    ~Interpreter() = default;

    /// @brief Evaluates the statement. If it is an atom statement (expression),
    /// @brief returns the expression evaluation result.
    /// @brief Otherwise, the two sides of the child node are evaluated recursively according to the
    /// grammar rules
    /// @throw error::SystemCallError If a specific system call error occurs
    /// (i.e. `fork` or `waitpid`).
    /// @throw error::TerminationSignal If this process is a child process.
    EvalResult evaluate( StmtNodeT stmt_node ) const;
  };
} // namespace tish

#endif // TISH_INTERPRETER
