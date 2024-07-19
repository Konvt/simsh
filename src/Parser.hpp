#ifndef __SIMSH_PARSER__
# define __SIMSH_PARSER__

#include <iostream>
#include <memory>

#include "Config.hpp"
#include "TreeNode.hpp"
#include "Tokenizer.hpp"

namespace simsh {
  /// @brief Recursive descent parser.
  class Parser {
    static constexpr types::StrViewT redirection_regex { R"(^(\d*)>{1,2}$)" };
    static constexpr types::StrViewT combined_redir_regex { R"(^(\d*)>&(\d*)$)" };

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
    Parser() : tknizr_ { std::cin } {}
    Parser( LineBuffer line_buf )
      : tknizr_ { std::move( line_buf ) } {}
    Parser( Tokenizer tknizr )
      : tknizr_ { std::move( tknizr ) } {}
    Parser( Parser&& rhs )
      : tknizr_ { std::move( rhs.tknizr_ ) } {}
    ~Parser() = default;
    Parser& operator=( Parser&& rhs ) {
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

    /// @return Syntax root node.
    [[nodiscard]] StmtNodePtr parse();
    [[nodiscard]] bool empty() const { return tknizr_.empty(); }
  };
}

#endif // __SIMSH_PARSER__
