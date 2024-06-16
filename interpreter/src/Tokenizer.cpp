#include <regex>
#include <string>
#include <cassert>

#include "Tokenizer.hpp"
#include "Exception.hpp"
using namespace std;

namespace hull {
  void LineBuffer::swap_members( LineBuffer&& rhs ) noexcept
  {
    using std::swap;
    swap( received_eof_, rhs.received_eof_ );
    swap( line_pos_, rhs.line_pos_ );
    swap( line_input_, rhs.line_input_ );
  }

  LineBuffer& LineBuffer::operator=( LineBuffer&& rhs ) noexcept
  {
    using std::swap;
    swap( input_stream_, rhs.input_stream_ );
    swap_members( move( rhs ) );
    return *this;
  }

  void LineBuffer::clear() noexcept
  {
    line_input_.clear();
    line_pos_ = 0;
  }

  type_decl::CharT LineBuffer::peek()
  {
    assert( input_stream_ != nullptr );
    if ( line_pos_ >= line_input_.size() ) {
      clear();

      getline( *input_stream_, line_input_ );
      if ( input_stream_->eof() ) {
        if ( received_eof_ )
          throw error::StreamClosed();
        else received_eof_ = true;

        line_input_.push_back( EOF );
      }
      else {
        received_eof_ = false;
        line_input_.push_back( '\n' );
      }
    }
    return line_input_[line_pos_];
  }

  void LineBuffer::consume() noexcept
  {
    assert( line_pos_ <= line_input_.size() );
    ++line_pos_;
  }

  void Tokenizer::reset( LineBuffer line_buf )
  {
    if ( !line_buf.eof() ) {
      line_buf_ = std::move( line_buf );
      token_list_.reset();
    }
  }

  void Tokenizer::reset( type_decl::StringT prompt )
  {
    prompt_ = std::move( prompt );
  }

  void Tokenizer::reset( LineBuffer line_buf, type_decl::StringT prompt )
  {
    reset( std::move( line_buf ) );
    reset( std::move( prompt ) );
  }

  Tokenizer& Tokenizer::operator=( Tokenizer&& rhs )
  {
    using std::swap;
    swap( line_buf_, rhs.line_buf_ );
    swap( prompt_, rhs.prompt_ );
    swap( token_list_, rhs.token_list_ );
    return *this;
  }

  Tokenizer::Token& Tokenizer::peek()
  {
    if ( !token_list_.has_value() )
      token_list_ = next();

    return *token_list_;
  }

  type_decl::TokenT Tokenizer::consume( TokenType expect )
  {
    assert( token_list_.has_value() );
    if ( token_list_->is( expect ) ) {
      type_decl::TokenT discard_tokens = move( token_list_->value_ );
      token_list_.reset();
      return discard_tokens;
    }
    throw error::error_factory( error::info::SyntaxErrorInfo(
      line_buf_.line_pos(), expect, token_list_->type_
    ) );
  }

  Tokenizer::Token Tokenizer::next()
  {
    Token new_token;
    auto& [token_type, token_str] = new_token;

    enum class StateType {
      START, DONE,
      INCMD, INDIGIT, INSTR, INAND, INMEG_OUTPUT,
      INPIPE_LIKE, INRARR,
    };

    for ( StateType state = StateType::START; state != StateType::DONE; ) {
      const auto character = line_buf_.peek();
      bool save_char = true, discard_char = true;

      switch ( state ) {
      case StateType::START: {
        if ( character != '\n' && isspace( character ) )
          save_char = false;
        else if ( isdigit( character ) )
          state = StateType::INDIGIT;
        else {
          state = StateType::DONE;
          switch ( character ) {
          case EOF: {
            save_char = false;
            token_type = TokenType::ENDFILE;
          } break;
          case '\0':
            [[fallthrough]];
          case '\n': {
            token_type = TokenType::NEWLINE;
          } break;
          case ';': {
            token_type = TokenType::SEMI;
          } break;
          case '"': {
            state = StateType::INSTR;
            save_char = false;
          } break;
          case '&': {
            state = StateType::INAND;
          } break;
          case '|': {
            state = StateType::INPIPE_LIKE;
          } break;
          case '<': {
            token_type = TokenType::STDIN_REDIR;
          } break;
          case '!': {
            token_type = TokenType::NOT;
          } break;
          case '>': {
            state = StateType::INRARR;
          } break;
          case '(': {
            token_type = TokenType::LPAREN;
          } break;
          case ')': {
            token_type = TokenType::RPAREN;
          } break;
          default: {
            state = StateType::INCMD;
          } break;
          }
        }
      } break;

      case StateType::INCMD: {
        if ( isspace(character) || regex_match( type_decl::StringT( 1, character ), regex( R"([&|!<>"':\()^%$#])" ) ) ) {
          // 遇到了不应该出现在 command 中的字符，结束状态
          token_type = TokenType::CMD;
          save_char = false;
          discard_char = false;
          state = StateType::DONE;
        }
      } break;

      case StateType::INDIGIT: {
        if ( character == '>' )
          state = StateType::INRARR;
        else if ( character == '&' )
          state = StateType::INAND;
        else if ( !isdigit( character ) ) {
          save_char = false;
          discard_char = false;
          state = StateType::INCMD;
        }
      } break;

      case StateType::INSTR: {
        if ( character == '"' ) {
          save_char = false;
          token_type = TokenType::STR;
          state = StateType::DONE;
        } else if ( ("\n\0"sv).find( character ) != type_decl::StrViewT::npos || character == EOF )
          throw error::error_factory( error::info::TokenErrorInfo(
            line_buf_.line_pos(), '"', character
          ) );
      } break;

      case StateType::INAND: { // &&
        if ( character == '&' ) {
          state = StateType::DONE;
          token_type = TokenType::AND;
        } else if ( character == '>' ) { // &>
          state = StateType::INMEG_OUTPUT;
        } else {
          token_type = TokenType::ERROR;
          throw error::error_factory( error::info::TokenErrorInfo(
            line_buf_.line_pos(), '&', character
          ) );
        }
      } break;

      case StateType::INMEG_OUTPUT: {
        state = StateType::DONE;
        if ( character == '>' )
          token_type = TokenType::MERG_APPND;
        else {
          save_char = false;
          discard_char = false;
          token_type = TokenType::MERG_OUTPUT;
        }
      } break;

      case StateType::INPIPE_LIKE: {
        if ( character != '|' ) {
          save_char = false;
          discard_char = false;
          token_type = TokenType::PIPE;
        } else token_type = TokenType::OR;
        state = StateType::DONE;
      } break;

      case StateType::INRARR: {
        if ( character != '>' ) {
          save_char = false;
          discard_char = false;
          token_type = TokenType::OVR_REDIR;
        } else token_type = TokenType::APND_REDIR;
        state = StateType::DONE;
      } break;

      case StateType::DONE:
        [[fallthrough]];
      default: {
        state = StateType::DONE;
        token_type = TokenType::ERROR;
        throw error::error_factory( error::info::ArgumentErrorInfo(
          "tokenizer"sv, "the state machine status is incorrect"sv
        ) );
      } break;
      }

      if ( save_char )
        token_str.push_back( character );
      if ( discard_char )
        line_buf_.consume();
    }

    return new_token;
  }
}
