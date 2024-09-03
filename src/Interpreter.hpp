#ifndef __SIMSH_INTERPRETER__
#define __SIMSH_INTERPRETER__

#include <unordered_set>

#include "Config.hpp"
#include "TreeNode.hpp"

namespace simsh {
  /// @brief An interpreter for executing syntax tree.
  class Interpreter {
    using StmtNodeT = StmtNode* const;
    using ExprNodeT = ExprNode* const;
    static const std::unordered_set<types::StringT> built_in_cmds;

    [[nodiscard]] types::EvalT sequential_stmt( StmtNodeT seq_stmt ) const;
    [[nodiscard]] types::EvalT logical_and( StmtNodeT and_stmt ) const;
    [[nodiscard]] types::EvalT logical_or( StmtNodeT or_stmt ) const;
    [[nodiscard]] types::EvalT logical_not( StmtNodeT not_stmt ) const;
    [[nodiscard]] types::EvalT pipeline_stmt( StmtNodeT pipeline_stmt ) const;
    [[nodiscard]] types::EvalT output_redirection( StmtNodeT oup_redr ) const;
    [[nodiscard]] types::EvalT merge_stream( StmtNodeT merg_redr ) const;
    [[nodiscard]] types::EvalT input_redirection( StmtNodeT inp_redr ) const;

    [[nodiscard]] types::EvalT trivial( ExprNodeT expr ) const;

    /// @brief Internal instruction execution, not cross-process.
    [[nodiscard]] types::EvalT builtin_exec( ExprNodeT expr ) const;

    /// @brief Execute the expression, and return 0 or 1 (a boolean),
    /// indicating whether the expression was successful.
    /// @brief The 'successful' means that the return value of child process was `EXIT_SUCCESS`.
    [[nodiscard]] types::EvalT external_exec( ExprNodeT expr ) const;

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
    types::EvalT interpret( StmtNodeT stmt_node ) const;
    types::EvalT operator()( StmtNodeT stmt_node ) const { return interpret( stmt_node ); }
  };
} // namespace simsh

#endif // __SIMSH_INTERPRETER__
