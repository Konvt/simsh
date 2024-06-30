#ifndef __SIMSH_TREENODE__
# define __SIMSH_TREENODE__

#include <vector>
#include <memory>
#include <optional>

#include "Config.hpp"
#include "EnumLabel.hpp"

namespace simsh {
  class StmtNode {
  public:
    using ChildNode = std::unique_ptr<StmtNode>;
    using SiblingNodes = std::vector<ChildNode>;

  protected:
    StmtKind category_;
    ChildNode l_child_, r_child_;
    /* Each node stores only a string of token.
     * Therefore, any additional arguments must be stored in siblings node.
     * Currently the arguments can only be saved as an unique_ptr pointing to `ExprNode`. */
    SiblingNodes siblings_;

  public:
    StmtNode( StmtKind stmt_type,
              ChildNode left_stmt = nullptr, ChildNode right_stmt = nullptr,
              SiblingNodes siblings = {} )
      : category_ { stmt_type }
      , l_child_ { std::move( left_stmt ) }
      , r_child_ { std::move( right_stmt ) }
      , siblings_ { std::move( siblings ) } {}
    StmtNode( StmtKind stmt_type, SiblingNodes siblings )
      : StmtNode( stmt_type ) {
      siblings_ = std::move( siblings );
    }
    virtual ~StmtNode() = default;

    [[nodiscard]] StmtKind type() const noexcept { return category_; }

    StmtNode* left() const & noexcept { return l_child_.get(); }
    StmtNode* right() const & noexcept { return r_child_.get(); }
    [[nodiscard]] ChildNode left() && noexcept { return std::move( l_child_ ); }
    [[nodiscard]] ChildNode right() && noexcept { return std::move( r_child_ ); }
    const SiblingNodes& siblings() const & noexcept { return siblings_; }
    [[nodiscard]] SiblingNodes siblings() && noexcept { return std::move( siblings_ ); }

    /// @brief Evaluates the statement. If it is an trivial statement (expression),
    /// @brief returns the expression evaluation result.
    /// @brief Otherwise, the two sides of the child node are evaluated recursively according to the grammar rules
    /// @throw error::SystemCallError If a specific system call error occurs (i.e. `fork` and `waitpid`).
    /// @throw error::TerminationSignal If this process is a child process.
    virtual types::EvalT evaluate();
  };

  class ExprNode : public StmtNode {
    ExprKind type_;
    std::optional<types::EvalT> result_;

    types::TokenT token_;

    /// @brief Execute the expression, and return 0 or 1 (a boolean), indicating whether the expression was successful.
    /// @brief The 'successful' means that the return value of child process was `EXIT_SUCCESS`.
    [[nodiscard]] types::EvalT external_exec() const;

    /// @brief Internal instruction execution, not cross-process.
    [[nodiscard]] types::EvalT internal_exec() const;

  public:
    template<typename T>
      requires std::disjunction_v<
        std::is_arithmetic<std::decay_t<T>>,
        std::is_same<std::decay_t<T>, types::TokenT>
      >
    ExprNode( ExprKind expr_type, T&& data, SiblingNodes siblings = {} )
      : StmtNode( StmtKind::trivial, std::move( siblings ) )
      , type_ { expr_type }, result_ { std::nullopt }, token_ {} {
      using DecayT = std::decay_t<T>;
      if constexpr ( std::is_arithmetic_v<DecayT> )
        result_ = std::forward<T>( data );
      else token_ = std::forward<T>( data );
    }
    virtual ~ExprNode() = default;

    [[nodiscard]] types::TokenT token() && noexcept { return std::move( token_ ); }
    const types::TokenT& token() const & noexcept { return token_; }

    [[nodiscard]] ExprKind kind() const noexcept { return type_; }
    /// @throw error::TerminationSignal If this process is a child process.
    [[nodiscard]] virtual types::EvalT evaluate() override;
  };
}

#endif // __SIMSH_TREENODE__
