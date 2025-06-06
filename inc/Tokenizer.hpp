#ifndef TISH_TOKENIZER
#define TISH_TOKENIZER

#include <istream>
#include <optional>
#include <util/Config.hpp>
#include <util/Enums.hpp>
#include <utility>

namespace tish {
  class LineBuffer {
    std::istream* input_stream_;

    bool received_eof_;
    std::size_t line_pos_;
    types::String line_input_;

    void swap_members( LineBuffer&& rhs ) noexcept;

  public:
    LineBuffer( const LineBuffer& lhs )            = delete;
    LineBuffer& operator=( const LineBuffer& lhs ) = delete;

    /// @brief Create a line buffer bound to the specified input stream.
    LineBuffer( std::istream& input_stream )
      : input_stream_ { std::addressof( input_stream ) }, received_eof_ { false }, line_pos_ {}
    {}
    LineBuffer( LineBuffer&& rhs ) noexcept : LineBuffer( *rhs.input_stream_ )
    {
      swap_members( std::move( rhs ) );
    }
    ~LineBuffer() = default;

    LineBuffer& operator=( LineBuffer&& rhs ) noexcept;

    [[nodiscard]] bool eof() const { return received_eof_; }
    [[nodiscard]] std::size_t line_pos() const noexcept { return line_pos_; }

    /// @brief Returns the current scanned string.
    [[nodiscard]] types::String context() const noexcept { return line_input_; }

    /// @brief Clear the line buffer.
    void clear() noexcept;

    /// @brief Peek the current character.
    /// @throw error::StreamClosed If `peek` is called again when EOF has been
    /// returned.
    [[nodiscard]] types::Char peek();

    /// @brief Discard the current character from the buffer.
    void consume() noexcept;

    /// @brief Back `num_chars` characters, set to 0 if the line position is
    /// less than `num_chars`.
    void backtrack( std::size_t num_chars ) noexcept;
  };

  class Tokenizer {
  public:
    struct Token {
      types::TokenType type_;
      types::Token value_;

      Token( types::TokenType tp, types::Token val )
        : type_ { std::move( tp ) }, value_ { std::move( val ) }
      {}
      Token() : Token( types::TokenType::ERROR, {} ) {}
      [[nodiscard]] bool is( types::TokenType tp ) const noexcept { return type_ == tp; }
      [[nodiscard]] bool is_not( types::TokenType tp ) const noexcept { return !is( tp ); }
    };

    Tokenizer( LineBuffer line_buf ) : line_buf_ { std::move( line_buf ) }, current_token_ {} {}
    ~Tokenizer() = default;
    Tokenizer( Tokenizer&& rhs ) : Tokenizer( std::move( rhs.line_buf_ ) )
    {
      using std::swap;
      swap( current_token_, rhs.current_token_ );
    }

    Tokenizer& operator=( Tokenizer&& rhs );

    [[nodiscard]] bool empty() const noexcept { return line_buf_.eof(); }
    [[nodiscard]] std::size_t line_pos() const noexcept { return line_buf_.line_pos(); }

    /// @brief Returns the current scanned string.
    [[nodiscard]] types::String context() const noexcept { return line_buf_.context(); }

    /// @brief Clear all unprocessed characters.
    void clear()
    {
      line_buf_.clear();
      current_token_.reset();
    }

    Token& peek();

    /// @brief Discard the current token and return it.
    /// @throw error::SyntaxError If `expect` isn't matched with current token.
    types::Token consume( types::TokenType expect );

    /// @brief Reset the current line buffer with the new one.
    void reset( LineBuffer line_buf );

    [[nodiscard]] LineBuffer&& line_buf() && noexcept { return std::move( line_buf_ ); }
    const LineBuffer& line_buf() const& noexcept { return line_buf_; }

    [[nodiscard]] std::optional<Token>&& token() && noexcept { return std::move( current_token_ ); }
    const std::optional<Token>& token() const& noexcept { return current_token_; }

  private:
    LineBuffer line_buf_;
    std::optional<Token> current_token_;

    /// @brief A dfa, which returns a single token.
    /// @throw error::ArgumentError If the state machine is stepped into a wrong
    /// state.
    /// @throw error::TokenError If the state machine received an unexpected
    /// character.
    [[nodiscard]] Token next();
  };
} // namespace tish

#endif // TISH_TOKENIZER
