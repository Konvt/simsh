#ifndef __HULL_TREENODE__
# define __HULL_TREENODE__

#include <vector>
#include <memory>
#include <optional>
#include <variant>
#include <concepts>

#include "Config.hpp"
#include "EnumLabel.hpp"

namespace hull {
  class StmtNode {
    StmtKind category_;

    using Node = std::unique_ptr<StmtNode>;
    Node l_child_, r_child_;

  protected:
    type_decl::TokensT tokens_;

  public:
    StmtNode( StmtKind stmt_type )
      : category_ { stmt_type }
      , l_child_ { nullptr }, r_child_ { nullptr }
      , tokens_ { 1, type_decl::StringT( "command" ) }{}
    StmtNode( StmtKind stmt_type, type_decl::TokensT tokens, Node left_stmt, Node right_stmt )
      : category_ { stmt_type } , l_child_ { std::move( left_stmt ) }
      , r_child_ { std::move( right_stmt ) } , tokens_ { move( tokens ) } {}
    virtual ~StmtNode() = default;

    [[nodiscard]] StmtKind type() const noexcept { return category_; }
    const type_decl::TokensT& token() const noexcept { return tokens_; }

    StmtNode* left() const noexcept { return l_child_.get(); }
    StmtNode* right() const noexcept { return r_child_.get(); }

    /// @brief 对语句进行求值，若是平凡语句（表达式）则返回表达式求值结果，否则遵循语法规则对两侧子结点递归求值
    /// @throw error::InitError If a specific system call error occurs (i.e. `fork` and `waitpid`).
    /// @throw error::TerminationSignal If this process is a child process.
    [[nodiscard]] virtual type_decl::EvalT evaluate();
  };

  class ExprNode : public StmtNode {
    ExprKind category_;
    std::optional<type_decl::EvalT> result_;

    /// @brief 执行表达式，并将“执行结果是否成功”的布尔值作为求值结果返回
    /// @param expression 一条命令表达式
    /// @return 子进程返回值
    [[nodiscard]] static type_decl::EvalT external_exec( const type_decl::TokensT& expression );

    /// @brief 内部指令执行，不会跨进程
    /// @param expression 一条命令表达式
    /// @return 执行结果
    [[nodiscard]] static type_decl::EvalT internal_exec( const type_decl::TokensT& expression );

  public:
    template<typename T>
      requires std::disjunction_v<
        std::is_same<std::decay_t<T>, type_decl::EvalT>,
        std::is_same<std::decay_t<T>, type_decl::TokensT>
      >
    ExprNode( StmtKind stmt_type, ExprKind expr_type, T&& data )
      : StmtNode( stmt_type ) , category_ { expr_type } , result_ { std::nullopt } {
      using DecayT = std::decay_t<T>;
      if constexpr ( std::is_same_v<DecayT, type_decl::EvalT> )
        result_ = std::forward<T>( data );
      else tokens_ = std::forward<T>( data );
    }
    virtual ~ExprNode() = default;

    [[nodiscard]] ExprKind kind() const noexcept { return category_; }
    /// @throw error::TerminationSignal If this process is a child process.
    [[nodiscard]] virtual type_decl::EvalT evaluate() override;
  };
}

#endif // __HULL_TREENODE__
