#ifndef __HULL_SCANNER__
# define __HULL_SCANNER__

#include <istream>
#include <utility>
#include <list>

#include "Config.hpp"
#include "EnumLabel.hpp"

namespace hull {
  class LineBuffer {
    std::istream* input_stream_;

    bool received_eof_;
    size_t line_pos_;
    type_decl::StringT line_input_;

    void swap_members( LineBuffer&& rhs ) noexcept;

  public:
    LineBuffer( const LineBuffer& lhs ) = delete;
    LineBuffer& operator=( const LineBuffer& lhs ) = delete;

    LineBuffer( std::istream& input_stream )
      : input_stream_ { std::addressof( input_stream ) }
      , received_eof_ { false }, line_pos_ {} {}
    LineBuffer( LineBuffer&& rhs ) noexcept : LineBuffer( *rhs.input_stream_ ) {
      swap_members( std::move( rhs ) );
    }
    ~LineBuffer() = default;

    LineBuffer& operator=( LineBuffer&& rhs ) noexcept;

    [[nodiscard]] bool eof() const { return input_stream_->eof(); }
    [[nodiscard]] size_t line_pos() const noexcept { return line_pos_; }

    /// @brief Clear the line buffer.
    void clear() noexcept;

    /// @brief Peek the current character.
    /// @throw error::StreamClosed If `peek` is called again when EOF has been returned.
    [[nodiscard]] type_decl::CharT peek();

    /// @brief Discard the current character from the buffer.
    void consume() noexcept;
  };

  class Tokenizer {
  public:
    struct Token {
      TokenType type_;
      type_decl::TokensT value_;

      Token( TokenType tp, type_decl::TokensT val )
        : type_ { std::move( tp ) }, value_ { std::move( val ) } {}
      Token() : Token( TokenType::ERROR, {} ) {}
      [[nodiscard]] bool is( TokenType tp ) const noexcept { return type_ == tp; }
      [[nodiscard]] bool is_not( TokenType tp ) const noexcept { return !is( tp ); }
    };

    Tokenizer( LineBuffer line_buf, type_decl::StringT prompt = "> " )
      : line_buf_ { std::move( line_buf ) }
      , prompt_ { std::move( prompt ) }, token_list_ {} {}
    ~Tokenizer() = default;
    Tokenizer( Tokenizer&& rhs )
      : Tokenizer( std::move( rhs.line_buf_ ), std::move( rhs.prompt_ ) ) {
      using std::swap;
      swap( token_list_, rhs.token_list_ );
    }

    Tokenizer& operator=( Tokenizer&& rhs );

    [[nodiscard]] size_t line_pos() const noexcept { return line_buf_.line_pos(); }
    [[nodiscard]] bool empty() const noexcept { return line_buf_.eof(); }

    /// @brief Clear all unprocessed characters.
    void clear() { line_buf_.clear(); token_list_.clear(); }

    /// @throw error::ArgumentError If the tokenizer is empty.
    Token& peek();

    /// @brief 消耗当前 token，并将 token 串返回
    /// @throw error::SyntaxError If `expect` isn't matched with current token.
    type_decl::TokensT consume( TokenType expect );

    void reset( LineBuffer line_buf );
    void reset( type_decl::StringT prompt );
    void reset( LineBuffer line_buf, type_decl::StringT prompt );

    LineBuffer& line_buf() && noexcept { return line_buf_; }
    const LineBuffer& line_buf() const & noexcept { return line_buf_; }

    type_decl::StringT& prompt() && noexcept { return prompt_; }
    const type_decl::StringT& prompt() const & noexcept { return prompt_; }

    std::list<Token>& token_list() && noexcept { return token_list_; }
    const std::list<Token>& token_list() const & noexcept { return token_list_; }

  private:
    LineBuffer line_buf_;
    type_decl::StringT prompt_; // for getting further input when the token stream is blocked by line breaks
    std::list<Token> token_list_;

    void next();

    /// @throw error::ArgumentError If the state machine is stepped into a wrong state.
    /// @throw error::TokenError If the state machine received an unexpected character.
    [[nodiscard]] std::pair<TokenType, type_decl::StringT> dfa();
  };
}

#endif // __HULL_SCANNER__