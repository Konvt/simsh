#include <cassert>
#include <regex>

#include "Tokenizer.hpp"
#include "Exception.hpp"
using namespace std;

namespace hull {
  void utils::LineBuffer::discard() noexcept
  {
    assert( line_pos_ <= line_input_.size() );
    ++line_pos_;
  }

  Tokenizer::Token& Tokenizer::peek()
  {
    if ( !token_list_.empty() )
      return token_list_.front();
    else if ( empty() ) {
      throw error::error_factory( error::info::ArgumentErrorInfo(
        "tokenizer"sv, "empty tokenizer"sv
      ) );
    } else next();
    return token_list_.front();
  }

  type_decl::TokenT Tokenizer::consume( TokenType expect )
  {
    assert( token_list_.empty() == false );
    if ( token_list_.front().is( expect ) ) {
      type_decl::TokenT discard_tokens = move( token_list_.front().value_ );
      token_list_.pop_front();
      return discard_tokens;
    }
    throw error::error_factory( error::info::SyntaxErrorInfo(
      line_buf_.line_pos(), expect, token_list_.front().type_
    ) );
  }

  void Tokenizer::next()
  {
    assert( empty() == false );
    assert( token_list_.empty() == true );

    Tokenizer::Token new_token;
    {
      auto [tkn_tp, tkn_str] = dfa();
      new_token.type_ = tkn_tp; new_token.value_.push_back( move( tkn_str ) );
    }
    if ( new_token.is( TokenType::CMD ) ) {
      while ( true ) {
        assert( empty() == false );
        auto [new_type, new_str] = dfa();
        if ( new_type == TokenType::CMD )
          new_token.value_.push_back( move( new_str ) );
        else {
          token_list_.push_back( move( new_token ) );
          token_list_.push_back( Tokenizer::Token( new_type, type_decl::TokenT( 1, move( new_str ) ) ) );
          break;
        }
      }
    } else token_list_.push_back( move( new_token ) );
  }

  pair<TokenType, type_decl::StringT> Tokenizer::dfa()
  {
    TokenType token_type = TokenType::ERROR;
    type_decl::StringT token_str;

    enum class StateType {
      START, DONE,
      INCMD, INNUM_LIKE, INSTR, INAND,
      INPIPE_LIKE, INRARR, // means "right arrow"
    };

    for ( StateType state = StateType::START; state != StateType::DONE; ) {
      const auto character = line_buf_.peek();
      bool save_char = true, discard_char = true;

      switch ( state ) {
      case StateType::START: {
        if ( character != '\n' && isspace( character ) )
          save_char = false;
        else if ( isdigit( character ) )
          state = StateType::INNUM_LIKE;
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
        if ( !regex_match( type_decl::StringT( 1, character ), regex( "[A-Za-z0-9_\\.,+\\-*/@:~]" ) ) ) {
          // 遇到了不应该出现在 command 中的字符，结束状态
          token_type = TokenType::CMD;
          save_char = false;
          discard_char = false;
          state = StateType::DONE;
        }
      } break;

      case StateType::INNUM_LIKE: {
        if ( character == '>' )
          state = StateType::INRARR;
        else if ( !isdigit( character ) ) {
          save_char = false;
          discard_char = false;
          state = StateType::INCMD;
        }
      } break;

      case StateType::INSTR: {
        if ( character == '"' ) {
          save_char = false;
          token_type = TokenType::CMD;
          state = StateType::DONE;
        } else if ( ("\n\0"sv).find( character ) != type_decl::StrViewT::npos || character == EOF )
          throw error::error_factory( error::info::TokenErrorInfo(
            line_buf_.line_pos(), '"', character
          ) );
      } break;

      case StateType::INAND: { // &&
        if ( character != '&' ) {
          token_type = TokenType::ERROR;
          throw error::error_factory( error::info::TokenErrorInfo(
            line_buf_.line_pos(), '&', character
          ) );
        } else {
          state = StateType::DONE;
          token_type = TokenType::AND;
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
        line_buf_.discard();
    }

    return make_pair( token_type, move( token_str ) );
  }
}
