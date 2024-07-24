#ifndef __SIMSH_TREENODE__
# define __SIMSH_TREENODE__

#include <vector>
#include <memory>
#include <optional>
#include <variant>

#include "Config.hpp"
#include "Exception.hpp"
#include "EnumLabel.hpp"

namespace simsh {
  class StmtNode {
  public:
    using ChildNode = std::unique_ptr<StmtNode>;
    using SiblingNodes = std::vector<ChildNode>;

  protected:
    types::StmtKind category_;
    ChildNode l_child_, r_child_;
    /* Any additional arguments are stored in siblings node.
     * Currently the arguments can only be saved as an unique_ptr pointing to `ExprNode`. */
    SiblingNodes siblings_;

  public:
    StmtNode( types::StmtKind stmt_type,
      ChildNode left_stmt = nullptr, ChildNode right_stmt = nullptr,
      SiblingNodes siblings = {} )
      : category_ { stmt_type }
      , l_child_ { std::move( left_stmt ) }
      , r_child_ { std::move( right_stmt ) }
      , siblings_ { std::move( siblings ) } {}
    StmtNode( types::StmtKind stmt_type, SiblingNodes siblings )
      : StmtNode( stmt_type ) {
      siblings_ = std::move( siblings );
    }
    StmtNode( StmtNode&& rhs )
      : category_ { rhs.category_ }
      , l_child_ { std::move( rhs.l_child_ ) }
      , r_child_ { std::move( rhs.r_child_ ) }
      , siblings_ { std::move( rhs.siblings_ ) } {}
    virtual ~StmtNode() = default;

    [[nodiscard]] types::StmtKind type() const noexcept { return category_; }

    StmtNode* left() const & noexcept { return l_child_.get(); }
    StmtNode* right() const & noexcept { return r_child_.get(); }
    [[nodiscard]] ChildNode left() && noexcept { return std::move( l_child_ ); }
    [[nodiscard]] ChildNode right() && noexcept { return std::move( r_child_ ); }
    const SiblingNodes& siblings() const & noexcept { return siblings_; }
    [[nodiscard]] SiblingNodes siblings() && noexcept { return std::move( siblings_ ); }
  };

  class ExprNode : public StmtNode {
    types::ExprKind type_;
    std::variant<types::EvalT, types::TokenT> expr_;

  public:
    /// @throw error::RuntimeError If the type of `data` does not match the value of `expr_type`.
    template<typename T>
      requires std::disjunction_v<
        std::is_arithmetic<std::decay_t<T>>,
          std::is_same<std::decay_t<T>, types::TokenT>
      >
    ExprNode( types::ExprKind expr_type, T&& data, SiblingNodes siblings = {} )
      : StmtNode( types::StmtKind::trivial, std::move( siblings ) )
      , type_ { expr_type }
      , expr_ { std::forward<T>( data ) } {
      if constexpr ( constexpr auto error_mes = "ExprNode: The parameter `data` does not match the type annotation `expr_type`";
                     std::is_arithmetic_v<std::decay_t<T>> ) {
        if ( expr_type != types::ExprKind::value )
          throw error::RuntimeError( error_mes );
      } else {
        if ( expr_type == types::ExprKind::value )
          throw error::RuntimeError( error_mes );
      }
    }
    ExprNode( ExprNode&& rhs )
      : StmtNode( std::move( rhs ) )
      , type_ { rhs.type_ }
      , expr_ { std::move( rhs.expr_ ) } {}
    virtual ~ExprNode() = default;

    [[nodiscard]] types::TokenT token() && { return std::move( std::get<types::TokenT>( expr_ ) ); }
    const types::TokenT& token() const & { return std::get<types::TokenT>( expr_ ); }

    /// @brief Expose the token object so that it can be changed externally.
    types::TokenT& replace() { return std::get<types::TokenT>( expr_ ); }

    [[nodiscard]] types::EvalT value() const { return std::get<types::EvalT>( expr_ ); }

    [[nodiscard]] types::ExprKind kind() const noexcept { return type_; }
  };
}

#endif // __SIMSH_TREENODE__
