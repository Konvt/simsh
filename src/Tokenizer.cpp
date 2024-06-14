#include <cassert>
#include <regex>

#include "Tokenizer.hpp"
#include "Exception.hpp"
using namespace std;

namespace hull {
  void utils::LineBuffer::discard()
  {
    assert( line_pos_ <= line_input_.size() );
    ++line_pos_;
  }

  Tokenizer::Token& Tokenizer::peek()
  {
    if ( current_token_.has_value() )
      return current_token_.value();
    else if ( empty() )
      throw error::error_factory( error::info::ArgumentErrorInfo(
        "tokenizer"sv, "empty tokenizer"sv
      ) );
    return (current_token_ = next()).value();
  }

  void Tokenizer::consume( TokenType expect )
  {
    if ( current_token_->is( expect ) )
      current_token_.reset();
    else throw error::error_factory( error::info::SyntaxErrorInfo(
      line_buf_.line_pos(), expect, current_token_->type_
    ) );
  }

  Tokenizer::Token Tokenizer::next()
  {
    Token new_token;
    const auto back_insert = []( auto& container ) {
      container.push_back( {} );
      return prev( container.end() );
    };
    auto inserter = back_insert( new_token.value_ );

    enum class StateType {
      START, DONE, CMD_SKIPSPACE,
      INCMD, INSTR, INAND,
      INPIPE_LIKE, INRARR, // means "right arrow"
    };

    for ( StateType state = StateType::START; state != StateType::DONE; ) {
      const auto character = line_buf_.peek();
      bool save_char = true, discard_char = true;

      switch ( state ) {
      case StateType::START: {
        if ( character != '\n' && isspace( character ) )
          save_char = false;
        else {
          state = StateType::DONE;
          switch ( character ) {
          case EOF: {
            save_char = false;
            new_token.type_ = TokenType::ENDFILE;
          } break;
          case '\0':
            [[fallthrough]];
          case '\n': {
            new_token.type_ = TokenType::NEWLINE;
          } break;
          case ';': {
            new_token.type_ = TokenType::SEMI;
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
            new_token.type_ = TokenType::STDIN_REDIR;
          } break;
          case '!': {
            new_token.type_ = TokenType::NOT;
          } break;
          case '>': {
            state = StateType::INRARR;
          } break;
          case '(': {
            new_token.type_ = TokenType::LPAREN;
          } break;
          case ')': {
            new_token.type_ = TokenType::RPAREN;
          } break;
          default: {
            state = StateType::INCMD;
          } break;
          }
        }
      } break;

      case StateType::INCMD: {
        if ( character != '\n' && isspace( character ) ) {
          save_char = false;
          inserter = back_insert( new_token.value_ );
          state = StateType::CMD_SKIPSPACE;
        } else if ( character == '"' ) {
          save_char = false;
          state = StateType::INSTR;
        } else if ( !regex_match( type_decl::StringT( 1, character ), regex( "[A-Za-z0-9_\\.,+\\-*/@:~]" ) ) ) {
          // 遇到了不应该出现在 command 中的字符，结束状态
          new_token.type_ = TokenType::CMD;
          save_char = false;
          discard_char = false;
          state = StateType::DONE;
        }
      } break;

      case StateType::CMD_SKIPSPACE: {
        save_char = false;
        if ( character == '\n' || !isspace( character ) ) {
          discard_char = false;
          state = StateType::INCMD;
        }
      } break;

      case StateType::INSTR: {
        if ( character == '"' ) {
          save_char = false;
          state = StateType::INCMD;
        } else if ( ("\n\0"sv).find( character ) != type_decl::StrViewT::npos || character == EOF )
          throw error::error_factory( error::info::TokenErrorInfo(
            line_buf_.line_pos(), '"', character
          ) );
      } break;

      case StateType::INAND: { // &&
        if ( character != '&' ) {
          new_token.type_ = TokenType::ERROR;
          throw error::error_factory( error::info::TokenErrorInfo(
            line_buf_.line_pos(), '&', character
          ) );
        } else {
          state = StateType::DONE;
          new_token.type_ = TokenType::AND;
        }
      } break;

      case StateType::INPIPE_LIKE: {
        if ( character != '|' ) {
          save_char = false;
          discard_char = false;
          new_token.type_ = TokenType::PIPE;
        } else new_token.type_ = TokenType::OR;
        state = StateType::DONE;
      } break;

      case StateType::INRARR: {
        if ( character != '>' ) {
          save_char = false;
          discard_char = false;
          new_token.type_ = TokenType::OVR_REDIR;
        } else new_token.type_ = TokenType::APND_REDIR;
        state = StateType::DONE;
      } break;

      case StateType::DONE:
        [[fallthrough]];
      default: {
        state = StateType::DONE;
        new_token.type_ = TokenType::ERROR;
        throw error::error_factory( error::info::ArgumentErrorInfo(
          "tokenizer"sv, "the state machine status is incorrect"sv
        ) );
      } break;
      }

      if ( save_char )
        inserter->push_back( character );
      if ( discard_char )
        line_buf_.discard();
    }

    if ( inserter->empty() )
      new_token.value_.erase( inserter );

    return new_token;
  }
}
