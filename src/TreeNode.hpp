#ifndef __HULL_TREENODE__
# define __HULL_TREENODE__

#include <vector>
#include <memory>
#include <optional>
#include <span>
#include <concepts>

#include "Config.hpp"
#include "EnumLabel.hpp"

namespace hull {
  class StmtNode {
    StmtKind category_;
    using Node = std::unique_ptr<StmtNode>;
    Node l_child_, r_child_;

  public:
    StmtNode( StmtKind stmt_type )
      : category_ { stmt_type }
      , l_child_ { nullptr }, r_child_ { nullptr } {}
    StmtNode( StmtKind stmt_type, Node left_stmt, Node right_stmt )
      : category_ { stmt_type }
      , l_child_ { std::move( left_stmt ) }
      , r_child_ { std::move( right_stmt ) } {}
    virtual ~StmtNode() = default;

    [[nodiscard]] StmtKind type() const noexcept { return category_; }
    StmtNode* left() const noexcept { return l_child_.get(); }
    StmtNode* right() const noexcept { return r_child_.get(); }

    /// @brief 对语句进行求值，若是平凡语句（表达式）则返回表达式求值结果，否则遵循语法规则对两侧子结点递归求值
    [[nodiscard]] virtual type_decl::EvalT evaluate();
  };

  class ExprNode : public StmtNode {
    ExprKind category_;
    type_decl::TokenT expr_;
    std::optional<type_decl::EvalT> result_;

    /// @brief 执行表达式，并将执行结果作为求值结果返回
    /// @param expression 一条命令表达式
    /// @return 子进程返回值
    [[nodiscard]] static type_decl::EvalT external_exec( const type_decl::TokenT& expression );

    /// @brief 内部指令执行，不会跨进程
    /// @param expression 一条命令表达式
    /// @return 执行结果
    [[nodiscard]] static type_decl::EvalT internal_exec( const type_decl::TokenT& expression );

  public:
    template<typename T>
      requires std::disjunction_v<
        std::is_same<std::decay_t<T>, type_decl::EvalT>,
        std::is_same<std::decay_t<T>, type_decl::TokenT>
      >
    ExprNode( StmtKind stmt_type, ExprKind expr_type, T&& data )
      : StmtNode( stmt_type ), category_ { expr_type }
      , expr_ {}, result_ { std::nullopt } {
      using DecayT = std::decay_t<T>;
      if constexpr ( std::is_same_v<DecayT, type_decl::EvalT> )
        result_ = std::forward<T>( data );
      else expr_ = std::forward<T>( data );
    }
    virtual ~ExprNode() = default;

    /// @brief 对表达式进行求值，如果结点类型为 `ExprKind::command`，则执行命令并返回命令执行结果，
    /// @brief 若结点类型为 `ExprKind::string`，则返回字符串是否非空结果
    [[nodiscard]] virtual type_decl::EvalT evaluate() override;

    /// @brief 返回一个只读的表达式视图
    [[nodiscard]] std::span<const type_decl::StringT> expression() const { return expr_; }
  };
}

#endif // __HULL_TREENODE__
