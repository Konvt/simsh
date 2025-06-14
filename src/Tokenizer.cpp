#include <Tokenizer.hpp>
#include <cassert>
#include <util/Exception.hpp>
using namespace std;

namespace tish {
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

  type::Char LineBuffer::peek()
  {
    assert( input_stream_ != nullptr );
    if ( line_pos_ >= line_input_.size() ) {
      clear();

      getline( *input_stream_, line_input_ );
      if ( input_stream_->eof() ) {
        if ( received_eof_ )
          throw error::StreamClosed();
        else
          received_eof_ = true;

        line_input_.push_back( EOF );
      } else {
        received_eof_ = false;
        // `getline` always discards the line break
        // but line break is a syntactic token, thus we need push a new one.
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

  void LineBuffer::backtrack( size_t num_chars ) noexcept
  {
    line_pos_ = line_pos_ <= num_chars ? 0 : line_pos_ - num_chars;
  }

  void Tokenizer::reset( LineBuffer&& line_buf ) noexcept
  {
    if ( !line_buf.eof() ) {
      line_buf_ = std::move( line_buf );
      current_token_.reset();
    }
  }

  Tokenizer& Tokenizer::operator=( Tokenizer&& rhs ) noexcept
  {
    using std::swap;
    swap( line_buf_, rhs.line_buf_ );
    swap( current_token_, rhs.current_token_ );
    return *this;
  }

  Tokenizer::Token& Tokenizer::peek()
  {
    if ( !current_token_.has_value() )
      current_token_ = next();

    return *current_token_;
  }

  type::String Tokenizer::consume( TokenKind expect )
  {
    assert( current_token_.has_value() );
    if ( current_token_->is( expect ) ) {
      type::String discard_tokens = move( current_token_->value_ );
      current_token_.reset();
      return discard_tokens;
    }
    throw error::SyntaxError( line_buf_.line_pos(),
                              line_buf_.context(),
                              expect,
                              current_token_->type_ );
  }

  Tokenizer::Token Tokenizer::next()
  {
    Token new_token;
    auto& [token_str, token_type] = new_token;

    enum class StateType : uint8_t {
      START,
      DONE,
      INCOMMENT,
      INCMD,
      INDIGIT,
      INSTR, // "string"
      INAND, // &&, &>
      INMEG_OUTPUT,
      INMEG_STREAM, // &>, >&
      INPIPE_LIKE,  // ||, |
      INRARR,       // >, >>, >&
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
            save_char  = false;
            token_type = TokenKind::ENDFILE;
          } break;
          case '\0': [[fallthrough]];
          case '\n': {
            token_type = TokenKind::NEWLINE;
          } break;
          case ';': {
            token_type = TokenKind::SEMI;
          } break;
          case '"': {
            save_char = false;
            state     = StateType::INSTR;
          } break;
          case '#': {
            save_char = false;
            state     = StateType::INCOMMENT;
          } break;
          case '&': {
            state = StateType::INAND;
          } break;
          case '|': {
            state = StateType::INPIPE_LIKE;
          } break;
          case '<': {
            token_type = TokenKind::STDIN_REDIR;
          } break;
          case '!': {
            token_type = TokenKind::NOT;
          } break;
          case '>': {
            state = StateType::INRARR;
          } break;
          case '(': {
            token_type = TokenKind::LPAREN;
          } break;
          case ')': {
            token_type = TokenKind::RPAREN;
          } break;
          default: {
            if ( ( "':^%"sv ).find( character ) != type::StrView::npos )
              throw error::TokenError( line_buf_.line_pos(),
                                       line_buf_.context(),
                                       "any valid command character"sv,
                                       character );
            else
              state = StateType::INCMD;
          } break;
          }
        }
      } break;

      case StateType::INCOMMENT: {
        save_char = false;
        if ( character == '\n' ) {
          token_type = TokenKind::NEWLINE;
          state      = StateType::DONE;
        } else if ( character == EOF ) {
          token_type = TokenKind::ENDFILE;
          state      = StateType::DONE;
        }
      } break;

      case StateType::INCMD: {
        if ( isspace( character )
             || ( "&|!<>\"';:()^%#"sv ).find( character ) != type::StrView::npos ) {
          if ( token_str.empty() ) {
            throw error::TokenError( line_buf_.line_pos(),
                                     line_buf_.context(),
                                     "any valid command character"sv,
                                     character );
          }
          token_type = TokenKind::CMD;
          save_char  = ( discard_char = false );
          state      = StateType::DONE;
        }
      } break;

      case StateType::INDIGIT: {
        if ( character == '>' ) // get (\d+)>, expecting '&' or '>'
          state = StateType::INRARR;
        else if ( !isdigit( character ) ) { // get (\d+), then expecting CMD
                                            // characters
          save_char = ( discard_char = false );
          state     = StateType::INCMD;
        }
      } break;

      case StateType::INSTR: {
        if ( character == '"' ) {
          save_char  = false;
          token_type = TokenKind::STR;
          state      = StateType::DONE;
        } else if ( ( "\n"sv ).find( character ) != type::StrView::npos || character == EOF )
          throw error::TokenError( line_buf_.line_pos(), line_buf_.context(), '"', character );
      } break;

      case StateType::INAND: {
        if ( character == '&' ) { // get &&, done
          state      = StateType::DONE;
          token_type = TokenKind::AND;
        } else if ( character == '>' )
          state = StateType::INMEG_OUTPUT; // get &>, still expecting '>' or
                                           // nothing
        else {
          token_type = TokenKind::ERROR;
          throw error::TokenError( line_buf_.line_pos(), line_buf_.context(), '&', character );
        }
      } break;

      case StateType::INMEG_OUTPUT: {
        if ( character == '>' )
          token_type = TokenKind::MERG_APPND; // get &>>
        else {
          save_char  = ( discard_char = false );
          token_type = TokenKind::MERG_OUTPUT; // get &>
        }
        state = StateType::DONE;
      } break;

      case StateType::INMEG_STREAM: { // get (\d*)>&, expecting (\d*) or
                                      // nothing
        if ( !isdigit( character ) ) {
          save_char  = ( discard_char = false );
          token_type = TokenKind::MERG_STREAM;
          state      = StateType::DONE;
        }
      } break;

      case StateType::INPIPE_LIKE: {
        if ( character != '|' ) {
          save_char  = ( discard_char = false );
          token_type = TokenKind::PIPE;
        } else
          token_type = TokenKind::OR;
        state = StateType::DONE;
      } break;

      case StateType::INRARR: {
        state = StateType::DONE;
        if ( character == '&' ) // get (\d*)>&, expecting (\d*) or nothing
          state = StateType::INMEG_STREAM;
        else if ( character == '>' ) // get >>, done
          token_type = TokenKind::APND_REDIR;
        else { // get >, done
          save_char  = ( discard_char = false );
          token_type = TokenKind::OVR_REDIR;
        }
      } break;

      case StateType::DONE: [[fallthrough]];
      default:              {
        state      = StateType::DONE;
        token_type = TokenKind::ERROR;
        throw error::ArgumentError( "tokenizer"sv, "the state machine status is incorrect"sv );
      } break;
      }

      if ( save_char )
        token_str.push_back( character );
      if ( discard_char )
        line_buf_.consume();
    }

    return new_token;
  }
} // namespace tish
