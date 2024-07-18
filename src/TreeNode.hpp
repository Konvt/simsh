#ifndef __SIMSH_TREENODE__
# define __SIMSH_TREENODE__

#include <vector>
#include <memory>
#include <optional>
#include <variant>

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
  };

  class ExprNode : public StmtNode {
    ExprKind type_;
    std::variant<types::EvalT, types::TokenT> expr_;

  public:
    template<typename T>
      requires std::disjunction_v<
        std::is_arithmetic<std::decay_t<T>>,
        std::is_same<std::decay_t<T>, types::TokenT>
      >
    ExprNode( ExprKind expr_type, T&& data, SiblingNodes siblings = {} )
      : StmtNode( StmtKind::trivial, std::move( siblings ) )
      , type_ { expr_type }, expr_ {} {
      expr_ = std::forward<T>( data );
    }
    virtual ~ExprNode() = default;

    [[nodiscard]] types::TokenT token() && { return std::move( std::get<types::TokenT>( expr_ ) ); }
    const types::TokenT& token() const & { return std::get<types::TokenT>( expr_ ); }

    /// @brief Expose the token object so that it can be changed externally.
    types::TokenT& replace() { return std::get<types::TokenT>( expr_ ); }

    [[nodiscard]] types::EvalT value() const { return std::get<types::EvalT>( expr_ ); }

    [[nodiscard]] ExprKind kind() const noexcept { return type_; }
  };
}

#endif // __SIMSH_TREENODE__
