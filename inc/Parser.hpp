#ifndef TISH_PARSER
#define TISH_PARSER

#include <Tokenizer.hpp>
#include <TreeNode.hpp>
#include <memory>
#include <util/Config.hpp>

namespace tish {
  /// @brief Recursive descent parser.
  class Parser {
    static constexpr types::StrView _pattern_redirection { R"(^(\d*)>{1,2}$)" };
    static constexpr types::StrView _pattern_combined_redir { R"(^(\d*)>&(\d*)$)" };

    using StmtNodePtr = std::unique_ptr<StmtNode>;
    using ExprNodePtr = std::unique_ptr<ExprNode>;
    Tokenizer tknizr_;

    [[nodiscard]] StmtNodePtr statement();
    [[nodiscard]] StmtNodePtr nonempty_statement();
    [[nodiscard]] StmtNodePtr statement_extension( StmtNodePtr left_stmt );

    [[nodiscard]] StmtNodePtr inner_statement();
    [[nodiscard]] StmtNodePtr inner_statement_extension( StmtNodePtr left_stmt );

    [[nodiscard]] StmtNodePtr redirection( StmtNodePtr left_stmt );
    [[nodiscard]] StmtNodePtr output_redirection( StmtNodePtr left_stmt );
    [[nodiscard]] StmtNodePtr combined_redirection_extension( StmtNodePtr left_stmt );

    [[nodiscard]] StmtNodePtr logical_not();
    [[nodiscard]] ExprNodePtr expression();

  public:
    Parser();
    Parser( LineBuffer line_buf ) : tknizr_ { std::move( line_buf ) } {}
    Parser( Tokenizer tknizr ) : tknizr_ { std::move( tknizr ) } {}
    Parser( Parser&& rhs ) : tknizr_ { std::move( rhs.tknizr_ ) } {}
    ~Parser() = default;
    Parser& operator=( Parser&& rhs )
    {
      using std::swap;
      swap( tknizr_, rhs.tknizr_ );
      return *this;
    }

    /// @brief Reset the current line buffer with a new one.
    void reset( LineBuffer line_buf ) { tknizr_.reset( std::move( line_buf ) ); }

    /// @brief Reset the current tokenizer with a new one.
    void reset( Tokenizer tknizr ) { tknizr_ = std::move( tknizr ); }

    Tokenizer& tokenizer() noexcept { return tknizr_; }
    const Tokenizer& tokenizer() const noexcept { return tknizr_; }

    /// @return The root node of a syntax tree.
    [[nodiscard]] StmtNodePtr parse();
    [[nodiscard]] bool empty() const { return tknizr_.empty(); }
  };
} // namespace tish

#endif // TISH_PARSER
