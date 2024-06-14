#ifndef __HULL_PARSER__
# define __HULL_PARSER__

#include <concepts>
#include <type_traits>
#include <istream>
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
    [[nodiscard]] ExprNodePtr expression();

  public:
    Parser() : tknizr_ { "" } {}
    template<typename T>
      requires std::disjunction_v<
        std::bool_constant<utils::LineBufType<T>>,
        std::is_same<std::decay_t<T>, Tokenizer>
      >
    Parser( T&& line_input )
      : tknizr_ { std::forward<T>( line_input ) } {}
    ~Parser() = default;

    template<utils::LineBufType T>
    void reset( T&& line_input ) {
      tknizr_.reset( std::forward<T>( line_input ) );
    }

    [[nodiscard]] StmtNodePtr parse();

    friend std::istream& getline( std::istream& is, Parser& parsr ) {
      using std::getline;
      if ( is.eof() )
        throw error::ProcessSuicide( EXIT_SUCCESS );
      type_decl::StringT line_input; getline( is, line_input );
      parsr.reset( utils::LineBuffer( std::move( line_input ) ) );
      return is;
    }
  };
}

#endif // __HULL_PARSER__