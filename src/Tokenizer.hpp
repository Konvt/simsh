#ifndef __HULL_SCANNER__
# define __HULL_SCANNER__

#include <istream>
#include <utility>
#include <optional>

#include "Config.hpp"
#include "EnumLabel.hpp"

namespace hull {
  namespace utils {
    class LineBuffer {
      size_t line_pos_;
      type_decl::StringT line_input_;

    public:
      LineBuffer( type_decl::StringT line_input )
        : line_pos_ {}, line_input_ { std::move( line_input.append( "\n" ) ) } {}
      ~LineBuffer() = default;

      [[nodiscard]] size_t line_pos() const noexcept { return line_pos_; }
      [[nodiscard]] bool empty() const noexcept { return line_input_.empty(); }

      /// @brief End of line.
      bool eol() const noexcept { return line_pos_ >= line_input_.size(); }

      /// @brief Peek the current character.
      [[nodiscard]] type_decl::CharT peek() const { return line_input_[line_pos_]; }

      /// @brief Discard the current character from the buffer.
      void discard();

      void reset( type_decl::StringT line_input ) {
        line_input_ = std::move( line_input.append( "\n" ) );
        line_pos_ = 0;
      }
    };

    template<typename T>
    concept LineBufType =
      std::disjunction_v<
        std::is_same<std::decay_t<T>, utils::LineBuffer>,
        std::is_convertible<std::decay_t<T>, type_decl::StringT>
      >;
  }

  class Tokenizer {
  public:
    struct Token {
      TokenType type_;
      type_decl::TokenT value_;

      Token( TokenType tp, type_decl::TokenT val )
        : type_ { std::move( tp ) }, value_ { std::move( val ) } {}
      Token() : Token( TokenType::ERROR, {} ) {}
      [[nodiscard]] bool is( TokenType tp ) const noexcept { return type_ == tp; }
      [[nodiscard]] bool is_not( TokenType tp ) const noexcept { return !is( tp ); }
    };

    template<utils::LineBufType T>
    Tokenizer( T&& line_buf )
      : line_buf_ { std::move( line_buf ) }
      , current_token_ { std::nullopt } {}
    ~Tokenizer() = default;

    [[nodiscard]] size_t line_pos() const noexcept { return line_buf_.line_pos(); }
    [[nodiscard]] bool empty() const noexcept { return line_buf_.eol(); }

    /// @throw error::ArgumentError If the tokenizer is empty.
    Token& peek();

    /// @throw error::SyntaxError If tkn isn't matched with current token.
    void consume( TokenType expect );

    template<utils::LineBufType T>
    void reset( T&& line_input ) {
      utils::LineBuffer new_line_buf { std::forward<T>( line_input ) };
      if ( new_line_buf.empty() )
        return;
      line_buf_ = std::move( new_line_buf );
      current_token_.reset();
    }

  private:
    utils::LineBuffer line_buf_;
    std::optional<Token> current_token_;

    /// @throw hull::TokenError, hull::TraceBack
    [[nodiscard]] Token next();
  };
}

#endif // __HULL_SCANNER__
