#ifndef __SIMSH_TREENODE__
# define __SIMSH_TREENODE__

#include <list>
#include <memory>
#include <optional>

#include "Config.hpp"
#include "EnumLabel.hpp"

namespace simsh {
  class StmtNode {
  public:
    using ChildNode = std::unique_ptr<StmtNode>;
    using SiblingNodes = std::list<ChildNode>;

  protected:
    StmtKind category_;
    ChildNode l_child_, r_child_;
    /* Each node stores only a string of token.
     * Therefore, any additional arguments must be stored in siblings node.
     * Currently the arguments can only be saved as an unique_ptr pointing to `ExprNode`. */
    SiblingNodes siblings_;

    type_decl::TokenT token_;

  public:
    StmtNode( StmtKind stmt_type, type_decl::TokenT token = { "value" },
              ChildNode left_stmt = nullptr, ChildNode right_stmt = nullptr,
              SiblingNodes siblings = {} )
      : category_ { stmt_type }
      , l_child_ { std::move( left_stmt ) }
      , r_child_ { std::move( right_stmt ) }
      , siblings_ { std::move( siblings ) }
      , token_ { std::move( token ) } {}
    StmtNode( StmtKind stmt_type, SiblingNodes siblings )
      : StmtNode( stmt_type ) {
      siblings_ = std::move( siblings );
    }
    virtual ~StmtNode() = default;

    [[nodiscard]] StmtKind type() const noexcept { return category_; }
    [[nodiscard]] type_decl::TokenT token() && noexcept { return std::move( token_ ); }
    const type_decl::TokenT& token() const & noexcept { return token_; }

    StmtNode* left() const & noexcept { return l_child_.get(); }
    StmtNode* right() const & noexcept { return r_child_.get(); }
    [[nodiscard]] ChildNode left() && noexcept { return std::move( l_child_ ); }
    [[nodiscard]] ChildNode right() && noexcept { return std::move( r_child_ ); }
    const SiblingNodes& siblings() const & noexcept { return siblings_; }
    [[nodiscard]] SiblingNodes siblings() && noexcept { return std::move( siblings_ ); }

    /// @brief 对语句进行求值，若是平凡语句（表达式）则返回表达式求值结果，否则遵循语法规则对两侧子结点递归求值
    /// @throw error::InitError If a specific system call error occurs (i.e. `fork` and `waitpid`).
    /// @throw error::TerminationSignal If this process is a child process.
    [[nodiscard]] virtual type_decl::EvalT evaluate();
  };

  class ExprNode : public StmtNode {
    ExprKind type_;
    std::optional<type_decl::EvalT> result_;

    /// @brief Execute the expression, and return 0 or 1 (a boolean), indicating whether the expression was successful.
    /// @brief The 'successful' means that the return value of child process was `EXIT_SUCCESS`.
    [[nodiscard]] type_decl::EvalT external_exec() const;

    /// @brief Internal instruction execution, not cross-process.
    [[nodiscard]] type_decl::EvalT internal_exec() const;

  public:
    template<typename T>
      requires std::disjunction_v<
        std::is_same<std::decay_t<T>, type_decl::EvalT>,
        std::is_same<std::decay_t<T>, type_decl::TokenT>
      >
    ExprNode( StmtKind stmt_type, ExprKind expr_type, T&& data, SiblingNodes siblings = {} )
      : StmtNode( stmt_type, std::move( siblings ) )
      , type_ { expr_type }, result_ { std::nullopt } {
      using DecayT = std::decay_t<T>;
      if constexpr ( std::is_same_v<DecayT, type_decl::EvalT> )
        result_ = std::forward<T>( data );
      else token_ = std::forward<T>( data );
    }
    virtual ~ExprNode() = default;

    [[nodiscard]] ExprKind kind() const noexcept { return type_; }
    /// @throw error::TerminationSignal If this process is a child process.
    [[nodiscard]] virtual type_decl::EvalT evaluate() override;
  };
}

#endif // __SIMSH_TREENODE__
