#ifndef __SIMSH_INTERPRETER__
#define __SIMSH_INTERPRETER__

#include <TreeNode.hpp>
#include <unordered_set>
#include <util/Config.hpp>

namespace simsh {
  /// @brief An interpreter for executing syntax tree.
  class Interpreter {
    using StmtNodeT = StmtNode* const;
    using ExprNodeT = ExprNode* const;
    static const std::unordered_set<types::String> built_in_cmds;

    [[nodiscard]] types::Eval sequential_stmt( StmtNodeT seq_stmt ) const;
    [[nodiscard]] types::Eval logical_and( StmtNodeT and_stmt ) const;
    [[nodiscard]] types::Eval logical_or( StmtNodeT or_stmt ) const;
    [[nodiscard]] types::Eval logical_not( StmtNodeT not_stmt ) const;
    [[nodiscard]] types::Eval pipeline_stmt( StmtNodeT pipeline_stmt ) const;
    [[nodiscard]] types::Eval output_redirection( StmtNodeT oup_redr ) const;
    [[nodiscard]] types::Eval merge_stream( StmtNodeT merg_redr ) const;
    [[nodiscard]] types::Eval input_redirection( StmtNodeT inp_redr ) const;

    [[nodiscard]] types::Eval trivial( ExprNodeT expr ) const;

    /// @brief Internal instruction execution, not cross-process.
    [[nodiscard]] types::Eval builtin_exec( ExprNodeT expr ) const;

    /// @brief Execute the expression, and return 0 or 1 (a boolean),
    /// indicating whether the expression was successful.
    /// @brief The 'successful' means that the return value of child process was `EXIT_SUCCESS`.
    [[nodiscard]] types::Eval external_exec( ExprNodeT expr ) const;

  public:
    Interpreter()  = default;
    ~Interpreter() = default;

    /// @brief Evaluates the statement. If it is an trivial statement (expression),
    /// @brief returns the expression evaluation result.
    /// @brief Otherwise, the two sides of the child node are evaluated recursively according to the
    /// grammar rules
    /// @throw error::SystemCallError If a specific system call error occurs
    /// (i.e. `fork` or `waitpid`).
    /// @throw error::TerminationSignal If this process is a child process.
    types::Eval interpret( StmtNodeT stmt_node ) const;
    types::Eval operator()( StmtNodeT stmt_node ) const { return interpret( stmt_node ); }
  };
} // namespace simsh

#endif // __SIMSH_INTERPRETER__
