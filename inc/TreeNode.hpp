#ifndef TISH_TREENODE
#define TISH_TREENODE

#include <memory>
#include <util/Config.hpp>
#include <util/Exception.hpp>
#include <variant>
#include <vector>

namespace tish {
  class StmtNode {
  public:
    enum class StmtKind : uint8_t {
      atom,
      sequential,
      logical_and,
      logical_or,
      logical_not,
      pipeline,
      ovrwrit_redrct,
      appnd_redrct, // >, >>
      merge_output,
      merge_appnd,
      merge_stream, // &>, &>>, >&
      stdin_redrct
    };
    using ChildNode    = std::unique_ptr<StmtNode>;
    using SiblingNodes = std::vector<ChildNode>;

  protected:
    StmtKind category_;
    ChildNode l_child_, r_child_;
    /* Any additional arguments are stored in siblings node.
     * Currently the arguments can only be saved as an unique_ptr pointing to
     * `ExprNode`. */
    SiblingNodes siblings_;

  public:
    StmtNode( StmtKind stmt_type,
              ChildNode left_stmt   = nullptr,
              ChildNode right_stmt  = nullptr,
              SiblingNodes siblings = {} )
      : category_ { stmt_type }
      , l_child_ { std::move( left_stmt ) }
      , r_child_ { std::move( right_stmt ) }
      , siblings_ { std::move( siblings ) }
    {}
    StmtNode( StmtKind stmt_type, SiblingNodes siblings ) : StmtNode( stmt_type )
    {
      siblings_ = std::move( siblings );
    }
    StmtNode( StmtNode&& rhs )
      : category_ { rhs.category_ }
      , l_child_ { std::move( rhs.l_child_ ) }
      , r_child_ { std::move( rhs.r_child_ ) }
      , siblings_ { std::move( rhs.siblings_ ) }
    {}
    virtual ~StmtNode() = default;

    [[nodiscard]] StmtKind type() const noexcept { return category_; }

    StmtNode* left() const& noexcept { return l_child_.get(); }
    StmtNode* right() const& noexcept { return r_child_.get(); }
    [[nodiscard]] ChildNode&& left() && noexcept { return std::move( l_child_ ); }
    [[nodiscard]] ChildNode&& right() && noexcept { return std::move( r_child_ ); }
    const SiblingNodes& siblings() const& noexcept { return siblings_; }
    [[nodiscard]] SiblingNodes&& siblings() && noexcept { return std::move( siblings_ ); }
  };

  class ExprNode : public StmtNode {
  public:
    enum class ExprKind : uint8_t { command, string, value };

  private:
    ExprKind type_;
    std::variant<type::Eval, type::String> expr_;

  public:
    template<typename T>
      requires std::disjunction_v<std::is_arithmetic<std::decay_t<T>>,
                                  std::is_same<std::decay_t<T>, type::String>>
    ExprNode( ExprKind expr_type, T&& data, SiblingNodes siblings = {} )
      : StmtNode( StmtKind::atom, std::move( siblings ) )
      , type_ { expr_type }
      , expr_ { std::forward<T>( data ) }
    {
      if constexpr ( constexpr auto error_mes =
                       "ExprNode: The parameter `data` does not match the type "
                       "annotation `expr_type`";
                     std::is_arithmetic_v<std::decay_t<T>> ) {
        if ( expr_type != ExprKind::value ) [[unlikely]]
          throw error::RuntimeError( error_mes );
      } else {
        if ( expr_type == ExprKind::value ) [[unlikely]]
          throw error::RuntimeError( error_mes );
      }
    }
    ExprNode( ExprNode&& rhs )
      : StmtNode( std::move( rhs ) ), type_ { rhs.type_ }, expr_ { std::move( rhs.expr_ ) }
    {}
    virtual ~ExprNode() = default;

    [[nodiscard]] type::String&& token() && { return std::move( std::get<type::String>( expr_ ) ); }
    const type::String& token() const& { return std::get<type::String>( expr_ ); }

    /// @brief Replace the current token with the new token.
    void replace_with( type::String token )
    {
      std::get<type::String>( expr_ ) = std::move( token );
    }

    [[nodiscard]] type::Eval value() const { return std::get<type::Eval>( expr_ ); }

    [[nodiscard]] ExprKind kind() const noexcept { return type_; }
  };
} // namespace tish

#endif // TISH_TREENODE
