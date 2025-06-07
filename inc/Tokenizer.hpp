#ifndef TISH_TOKENIZER
#define TISH_TOKENIZER

#include <istream>
#include <optional>
#include <util/Config.hpp>
#include <utility>

namespace tish {
  class LineBuffer {
    std::istream* input_stream_;

    type::String line_input_;
    std::size_t line_pos_;
    bool received_eof_;

    void swap_members( LineBuffer&& rhs ) noexcept;

  public:
    LineBuffer( const LineBuffer& lhs )            = delete;
    LineBuffer& operator=( const LineBuffer& lhs ) = delete;

    /// @brief Create a line buffer bound to the specified input stream.
    LineBuffer( std::istream& input_stream )
      : input_stream_ { std::addressof( input_stream ) }, line_pos_ {}, received_eof_ { false }
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
    [[nodiscard]] type::StrView context() const noexcept { return line_input_; }

    /// @brief Clear the line buffer.
    void clear() noexcept;

    /// @brief Peek the current character.
    /// @throw error::StreamClosed If `peek` is called again when EOF has been
    /// returned.
    [[nodiscard]] type::Char peek();

    /// @brief Discard the current character from the buffer.
    void consume() noexcept;

    /// @brief Back `num_chars` characters, set to 0 if the line position is
    /// less than `num_chars`.
    void backtrack( std::size_t num_chars ) noexcept;
  };

  class Tokenizer {
  public:
    enum class TokenKind : uint8_t {
      CMD,
      STR,
      AND,
      OR,
      NOT,
      PIPE,
      OVR_REDIR,
      APND_REDIR, // >, >>
      MERG_OUTPUT,
      MERG_APPND,
      MERG_STREAM, // &>, &>>, >&
      STDIN_REDIR, // <
      LPAREN,
      RPAREN,
      NEWLINE,
      SEMI,
      ENDFILE,
      ERROR
    };
    [[nodiscard]] static constexpr type::StrView as_string( TokenKind tk ) noexcept
    {
      switch ( tk ) {
      case TokenKind::STR:         return "string";
      case TokenKind::CMD:         return "command";
      case TokenKind::AND:         return "logical AND";
      case TokenKind::OR:          return "logical OR";
      case TokenKind::NOT:         return "logical NOT";
      case TokenKind::PIPE:        return "pipeline";
      case TokenKind::OVR_REDIR:   [[fallthrough]];
      case TokenKind::APND_REDIR:  return "output redirection";
      case TokenKind::MERG_OUTPUT: [[fallthrough]];
      case TokenKind::MERG_APPND:  [[fallthrough]];
      case TokenKind::MERG_STREAM: return "combined redirection";
      case TokenKind::STDIN_REDIR: return "input redirection";
      case TokenKind::LPAREN:      return "left paren";
      case TokenKind::RPAREN:      return "right paren";
      case TokenKind::NEWLINE:     return "newline";
      case TokenKind::SEMI:        return "semicolon";
      case TokenKind::ENDFILE:     return "end of file";
      case TokenKind::ERROR:       [[fallthrough]];
      default:                     return "error";
      }
    }

    struct Token {
      type::String value_;
      TokenKind type_;

      Token( TokenKind tk, type::String&& val )
        : value_ { std::move( val ) }, type_ { std::move( tk ) }
      {}
      Token() : Token( TokenKind::ERROR, {} ) {}
      [[nodiscard]] bool is( TokenKind tk ) const noexcept { return type_ == tk; }
      [[nodiscard]] bool is_not( TokenKind tk ) const noexcept { return !is( tk ); }
      [[nodiscard]] friend bool operator==( const Token& tkn, TokenKind tk ) noexcept
      {
        return tkn.is( tk );
      }
    };

    Tokenizer( LineBuffer&& line_buf ) : line_buf_ { std::move( line_buf ) }, current_token_ {} {}
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
    [[nodiscard]] type::StrView context() const noexcept { return line_buf_.context(); }

    /// @brief Clear all unprocessed characters.
    void clear()
    {
      line_buf_.clear();
      current_token_.reset();
    }

    Token& peek();

    /// @brief Discard the current token and return it.
    /// @throw error::SyntaxError If `expect` isn't matched with current token.
    type::String consume( TokenKind expect );

    /// @brief Reset the current line buffer with the new one.
    void reset( LineBuffer&& line_buf );

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
