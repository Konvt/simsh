#ifndef __HULL_PARSER__
# define __HULL_PARSER__

#include <concepts>
#include <type_traits>
#include <iostream>
#include <memory>

#include "Exception.hpp"
#include "TreeNode.hpp"
#include "Tokenizer.hpp"

namespace hull {
  class Parser {
    using StmtNodePtr = std::unique_ptr<StmtNode>;
    using ExprNodePtr = std::unique_ptr<ExprNode>;
    Tokenizer tknizr_;

    [[nodiscard]] StmtNodePtr statement();
    [[nodiscard]] StmtNodePtr statement_extension( StmtNodePtr left_stmt );

    [[nodiscard]] StmtNodePtr inner_statement();
    [[nodiscard]] StmtNodePtr inner_statement_extension( StmtNodePtr left_stmt );

    [[nodiscard]] StmtNodePtr redirection( StmtNodePtr left_stmt );
    [[nodiscard]] StmtNodePtr logical_not();
    [[nodiscard]] ExprNodePtr expression();

  public:
    Parser() : tknizr_ { std::cin } {}
    Parser( LineBuffer line_buf )
      : tknizr_ { std::move( line_buf ) } {}
    ~Parser() = default;

    void reset( LineBuffer line_buf ) { tknizr_.reset( std::move( line_buf ) ); }
    void reset( Tokenizer tknizr ) { tknizr_ = std::move( tknizr ); }

    Tokenizer& tokenizer() noexcept { return tknizr_; }

    [[nodiscard]] StmtNodePtr parse();
    [[nodiscard]] bool empty() const { return tknizr_.empty(); }

    [[nodiscard]] friend StmtNodePtr operator>>( std::istream& is, Parser& prsr ) {
      prsr.reset( is );
      return prsr.parse();
    }
  };
}

#endif // __HULL_PARSER__