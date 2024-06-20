#ifndef __SIMSH_PARSER__
# define __SIMSH_PARSER__

#include <concepts>
#include <type_traits>
#include <iostream>
#include <memory>

#include "Exception.hpp"
#include "TreeNode.hpp"
#include "Tokenizer.hpp"

namespace simsh {
  /// @brief Recursive descent parser.
  class Parser {
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
    ~Parser() = default;

    /// @brief Reset the current line buffer with a new one.
    void reset( LineBuffer line_buf ) { tknizr_.reset( std::move( line_buf ) ); }

    /// @brief Reset the current tokenizer with a new one.
    void reset( Tokenizer tknizr ) { tknizr_ = std::move( tknizr ); }

    Tokenizer& tokenizer() noexcept { return tknizr_; }
    const Tokenizer& tokenizer() const noexcept { return tknizr_; }

    /// @return Syntax root node.
    [[nodiscard]] StmtNodePtr parse();
    [[nodiscard]] bool empty() const { return tknizr_.empty(); }

    /// @brief Reset and get input from a new input stream.
    [[nodiscard]] friend StmtNodePtr operator>>( std::istream& is, Parser& prsr ) {
      prsr.reset( is );
      return prsr.parse();
    }
  };
}

#endif // __SIMSH_PARSER__
