#include <Parser.hpp>
#include <Tokenizer.hpp>
#include <TreeNode.hpp>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <util/Constant.hpp>
#include <util/Exception.hpp>
#include <util/Util.hpp>
using namespace std;

namespace tish {
  Parser::Parser() : tknizr_ { cin } {}

  Parser::StmtNodePtr Parser::parse()
  {
    tknizr_.clear();
    return statement();
  }

  Parser::StmtNodePtr Parser::statement()
  {
    switch ( const auto tkn_tp = tknizr_.peek().type_; tkn_tp ) {
    case Tokenizer::TokenKind::ENDFILE: // empty statement
      [[fallthrough]];
    case Tokenizer::TokenKind::NEWLINE: {
      tknizr_.consume( tkn_tp );
      return make_unique<ExprNode>( ExprNode::ExprKind::value, EXIT_SUCCESS );
    }

    default: {
      return nonempty_statement();
    }
    }
  }

  Parser::StmtNodePtr Parser::nonempty_statement()
  {
    Parser::StmtNodePtr node;
    switch ( const auto tkn_tp = tknizr_.peek().type_; tkn_tp ) {
    case Tokenizer::TokenKind::CMD: [[fallthrough]];
    case Tokenizer::TokenKind::STR: {
      node = expression();
    } break;

    case Tokenizer::TokenKind::OVR_REDIR:   [[fallthrough]];
    case Tokenizer::TokenKind::APND_REDIR:  [[fallthrough]];
    case Tokenizer::TokenKind::MERG_OUTPUT: [[fallthrough]];
    case Tokenizer::TokenKind::MERG_APPND:  {
      node = redirection( nullptr );
    } break;

    case Tokenizer::TokenKind::LPAREN: {
      tknizr_.consume( Tokenizer::TokenKind::LPAREN );
      node = inner_statement();
    } break;

    case Tokenizer::TokenKind::NOT: {
      node = logical_not();
    } break;

    default: {
      throw error::SyntaxError( tknizr_.line_pos(),
                                tknizr_.context(),
                                Tokenizer::TokenKind::CMD,
                                tknizr_.peek().type_ );
    }
    }

    return statement_extension( move( node ) );
  }

  Parser::StmtNodePtr Parser::statement_extension( Parser::StmtNodePtr left_stmt )
  {
    switch ( const auto tkn_tp = tknizr_.peek().type_; tkn_tp ) {
    case Tokenizer::TokenKind::AND: { // connector
      tknizr_.consume( Tokenizer::TokenKind::AND );
      return make_unique<StmtNode>( StmtNode::StmtKind::logical_and,
                                    move( left_stmt ),
                                    nonempty_statement() );
    }

    case Tokenizer::TokenKind::OR: {
      tknizr_.consume( Tokenizer::TokenKind::OR );
      return make_unique<StmtNode>( StmtNode::StmtKind::logical_or,
                                    move( left_stmt ),
                                    nonempty_statement() );
    }

    case Tokenizer::TokenKind::PIPE: {
      tknizr_.consume( Tokenizer::TokenKind::PIPE );
      return make_unique<StmtNode>( StmtNode::StmtKind::pipeline,
                                    move( left_stmt ),
                                    nonempty_statement() );
    }

    case Tokenizer::TokenKind::SEMI: {
      tknizr_.consume( Tokenizer::TokenKind::SEMI );
      return make_unique<StmtNode>( StmtNode::StmtKind::sequential,
                                    move( left_stmt ),
                                    nonempty_statement() );
    }

    case Tokenizer::TokenKind::OVR_REDIR: // redirection
      [[fallthrough]];
    case Tokenizer::TokenKind::APND_REDIR:  [[fallthrough]];
    case Tokenizer::TokenKind::MERG_OUTPUT: [[fallthrough]];
    case Tokenizer::TokenKind::MERG_APPND:  [[fallthrough]];
    case Tokenizer::TokenKind::MERG_STREAM: [[fallthrough]];
    case Tokenizer::TokenKind::STDIN_REDIR: {
      return statement_extension( redirection( move( left_stmt ) ) );
    }

    case Tokenizer::TokenKind::ENDFILE: [[fallthrough]];
    case Tokenizer::TokenKind::NEWLINE: {
      tknizr_.consume( tkn_tp );
      return left_stmt;
    }

    default:
      throw error::SyntaxError( tknizr_.line_pos(),
                                tknizr_.context(),
                                Tokenizer::TokenKind::NEWLINE,
                                tknizr_.peek().type_ );
    }
  }

  Parser::StmtNodePtr Parser::inner_statement()
  {
    Parser::StmtNodePtr node;
    switch ( tknizr_.peek().type_ ) {
    case Tokenizer::TokenKind::CMD: [[fallthrough]];
    case Tokenizer::TokenKind::STR: {
      node = expression();
    } break;

    case Tokenizer::TokenKind::OVR_REDIR:   [[fallthrough]];
    case Tokenizer::TokenKind::APND_REDIR:  [[fallthrough]];
    case Tokenizer::TokenKind::MERG_OUTPUT: [[fallthrough]];
    case Tokenizer::TokenKind::MERG_APPND:  [[fallthrough]];
    case Tokenizer::TokenKind::MERG_STREAM: {
      node = redirection( nullptr );
    } break;

    case Tokenizer::TokenKind::NOT: {
      node = logical_not();
    } break;

    case Tokenizer::TokenKind::LPAREN: {
      tknizr_.consume( Tokenizer::TokenKind::LPAREN );
      node = inner_statement();
    } break;

    default: {
      throw error::SyntaxError( tknizr_.line_pos(),
                                tknizr_.context(),
                                Tokenizer::TokenKind::CMD,
                                tknizr_.peek().type_ );
    }
    }

    return inner_statement_extension( move( node ) );
  }

  Parser::StmtNodePtr Parser::inner_statement_extension( Parser::StmtNodePtr left_stmt )
  {
    switch ( tknizr_.peek().type_ ) {
    case Tokenizer::TokenKind::AND: { // connector
      tknizr_.consume( Tokenizer::TokenKind::AND );
      return make_unique<StmtNode>( StmtNode::StmtKind::logical_and,
                                    move( left_stmt ),
                                    inner_statement() );
    }

    case Tokenizer::TokenKind::OR: {
      tknizr_.consume( Tokenizer::TokenKind::OR );
      return make_unique<StmtNode>( StmtNode::StmtKind::logical_or,
                                    move( left_stmt ),
                                    inner_statement() );
    }

    case Tokenizer::TokenKind::PIPE: {
      tknizr_.consume( Tokenizer::TokenKind::PIPE );
      return make_unique<StmtNode>( StmtNode::StmtKind::pipeline,
                                    move( left_stmt ),
                                    inner_statement() );
    }

    case Tokenizer::TokenKind::SEMI: {
      tknizr_.consume( Tokenizer::TokenKind::SEMI );

      StmtNodePtr right_stmt;
      if ( tknizr_.peek().is( Tokenizer::TokenKind::RPAREN ) )
        tknizr_.consume( Tokenizer::TokenKind::RPAREN );
      else
        right_stmt = inner_statement();

      return make_unique<StmtNode>( StmtNode::StmtKind::sequential,
                                    move( left_stmt ),
                                    move( right_stmt ) );
    }

    case Tokenizer::TokenKind::OVR_REDIR: // redirection
      [[fallthrough]];
    case Tokenizer::TokenKind::APND_REDIR:  [[fallthrough]];
    case Tokenizer::TokenKind::MERG_OUTPUT: [[fallthrough]];
    case Tokenizer::TokenKind::MERG_APPND:  [[fallthrough]];
    case Tokenizer::TokenKind::MERG_STREAM: [[fallthrough]];
    case Tokenizer::TokenKind::STDIN_REDIR: {
      return inner_statement_extension( redirection( move( left_stmt ) ) );
    }

    case Tokenizer::TokenKind::RPAREN: {
      tknizr_.consume( Tokenizer::TokenKind::RPAREN );
      return left_stmt;
    }

    default: {
      throw error::SyntaxError( tknizr_.line_pos(),
                                tknizr_.context(),
                                Tokenizer::TokenKind::RPAREN,
                                tknizr_.peek().type_ );
    }
    }
  }

  Parser::StmtNodePtr Parser::redirection( Parser::StmtNodePtr left_stmt )
  {
    StmtNode::StmtKind stmt_kind = StmtNode::StmtKind::atom;
    switch ( tknizr_.peek().type_ ) {
    case Tokenizer::TokenKind::OVR_REDIR:   [[fallthrough]];
    case Tokenizer::TokenKind::APND_REDIR:  [[fallthrough]];
    case Tokenizer::TokenKind::MERG_OUTPUT: [[fallthrough]];
    case Tokenizer::TokenKind::MERG_APPND:  [[fallthrough]];
    case Tokenizer::TokenKind::MERG_STREAM: {
      return output_redirection( move( left_stmt ) );
    }
    case Tokenizer::TokenKind::STDIN_REDIR: {
      stmt_kind = StmtNode::StmtKind::stdin_redrct;
      tknizr_.consume( Tokenizer::TokenKind::STDIN_REDIR );
    } break;
    default:
      throw error::SyntaxError( tknizr_.line_pos(),
                                tknizr_.context(),
                                Tokenizer::TokenKind::OVR_REDIR,
                                tknizr_.peek().type_ );
    }

    if ( tknizr_.peek().type_ != Tokenizer::TokenKind::CMD ) {
      throw error::SyntaxError( tknizr_.line_pos(),
                                tknizr_.context(),
                                Tokenizer::TokenKind::CMD,
                                tknizr_.peek().type_ );
    }

    // The structure of syntax tree node
    // requires that redirected file name argument be stored in sibling nodes.
    StmtNode::SiblingNodes arguments;
    arguments.emplace_back( expression() );

    return make_unique<StmtNode>( stmt_kind, move( left_stmt ), nullptr, move( arguments ) );
  }

  Parser::StmtNodePtr Parser::output_redirection( Parser::StmtNodePtr left_stmt )
  {
    StmtNode::StmtKind stmt_kind = StmtNode::StmtKind::atom;
    StmtNode::SiblingNodes arguments;

    auto extract_params =
      [this]( StmtNode::SiblingNodes& args, type::StrView re_str, Tokenizer::TokenKind expecting ) {
        auto [match_result, matches] = util::match_string( tknizr_.consume( expecting ), re_str );

        assert( match_result == true );
        if ( expecting == Tokenizer::TokenKind::MERG_STREAM ) {
          for ( size_t i = 1; i <= 2; ++i ) {
            if ( auto match_str = matches[i].str(); match_str.empty() )
              args.emplace_back(
                make_unique<ExprNode>( ExprNode::ExprKind::value, constant::invalid_value ) );
            else
              args.emplace_back(
                make_unique<ExprNode>( ExprNode::ExprKind::value, stoi( match_str ) ) );
          }
        } else {
          if ( auto match_str = matches[1].str(); match_str.empty() )
            args.emplace_back(
              make_unique<ExprNode>( ExprNode::ExprKind::value, constant::invalid_value ) );
          else
            args.emplace_back(
              make_unique<ExprNode>( ExprNode::ExprKind::value, stoi( match_str ) ) );
        }
      };

    switch ( tknizr_.peek().type_ ) {
    case Tokenizer::TokenKind::OVR_REDIR: { // >
      stmt_kind = StmtNode::StmtKind::ovrwrit_redrct;
      extract_params( arguments, _pattern_redirection, Tokenizer::TokenKind::OVR_REDIR );
      assert( arguments.size() == 1 );
    } break;
    case Tokenizer::TokenKind::APND_REDIR: { // >>
      stmt_kind = StmtNode::StmtKind::appnd_redrct;
      extract_params( arguments, _pattern_redirection, Tokenizer::TokenKind::APND_REDIR );
      assert( arguments.size() == 1 );
    } break;
    case Tokenizer::TokenKind::MERG_OUTPUT: { // &>
      stmt_kind = StmtNode::StmtKind::merge_output;
      tknizr_.consume( Tokenizer::TokenKind::MERG_OUTPUT );
    } break;
    case Tokenizer::TokenKind::MERG_APPND: { // &>>
      stmt_kind = StmtNode::StmtKind::merge_appnd;
      tknizr_.consume( Tokenizer::TokenKind::MERG_APPND );
    } break;
    case Tokenizer::TokenKind::MERG_STREAM: { // >&
      return combined_redirection_extension( move( left_stmt ) );
    }
    default:
      throw error::SyntaxError( tknizr_.line_pos(),
                                tknizr_.context(),
                                Tokenizer::TokenKind::OVR_REDIR,
                                tknizr_.peek().type_ );
    }

    arguments.emplace_back( expression() );

    // The left operator takes precedence, which means `MERG_STREAM` will be the
    // child node.
    if ( tknizr_.peek().is( Tokenizer::TokenKind::MERG_STREAM ) ) { // output_redirecti
      StmtNode::SiblingNodes subargs;
      extract_params( subargs, _pattern_combined_redir, Tokenizer::TokenKind::MERG_STREAM );
      assert( subargs.size() == 2 );

      return make_unique<StmtNode>( stmt_kind,
                                    make_unique<StmtNode>( StmtNode::StmtKind::merge_stream,
                                                           move( left_stmt ),
                                                           nullptr,
                                                           move( subargs ) ),
                                    nullptr,
                                    move( arguments ) );
    }
    return make_unique<StmtNode>( stmt_kind, move( left_stmt ), nullptr, move( arguments ) );
  }

  Parser::StmtNodePtr Parser::combined_redirection_extension( Parser::StmtNodePtr left_stmt )
  {
    assert( tknizr_.peek().is( Tokenizer::TokenKind::MERG_STREAM ) );

    StmtNode::SiblingNodes arguments;
    auto extract_params =
      [this]( StmtNode::SiblingNodes& args, type::StrView re_str, Tokenizer::TokenKind expecting ) {
        auto [match_result, matches] = util::match_string( tknizr_.consume( expecting ), re_str );

        assert( match_result == true );
        if ( expecting == Tokenizer::TokenKind::MERG_STREAM ) {
          for ( size_t i = 1; i <= 2; ++i ) {
            if ( auto match_str = matches[i].str(); match_str.empty() )
              args.emplace_back(
                make_unique<ExprNode>( ExprNode::ExprKind::value, constant::invalid_value ) );
            else
              args.emplace_back(
                make_unique<ExprNode>( ExprNode::ExprKind::value, stoi( match_str ) ) );
          }
        } else {
          if ( auto match_str = matches[1].str(); match_str.empty() )
            args.emplace_back(
              make_unique<ExprNode>( ExprNode::ExprKind::value, constant::invalid_value ) );
          else
            args.emplace_back(
              make_unique<ExprNode>( ExprNode::ExprKind::value, stoi( match_str ) ) );
        }
      };

    extract_params( arguments, _pattern_combined_redir, Tokenizer::TokenKind::MERG_STREAM );
    assert( arguments.size() == 2 );

    StmtNodePtr node = nullptr;
    switch ( tknizr_.peek().type_ ) {
    case Tokenizer::TokenKind::OVR_REDIR: { // >
      StmtNode::SiblingNodes subargs;
      extract_params( subargs, _pattern_redirection, Tokenizer::TokenKind::OVR_REDIR );
      assert( subargs.size() == 1 );
      subargs.emplace_back( expression() );

      // The left operator takes precedence, which means `MERG_STREAM` will be
      // the parent node.
      node = make_unique<StmtNode>( StmtNode::StmtKind::ovrwrit_redrct,
                                    move( left_stmt ),
                                    nullptr,
                                    move( subargs ) );
    } break;

    case Tokenizer::TokenKind::APND_REDIR: { // >>
      StmtNode::SiblingNodes subargs;
      extract_params( subargs, _pattern_redirection, Tokenizer::TokenKind::APND_REDIR );
      assert( subargs.size() == 1 );
      subargs.emplace_back( expression() );

      node = make_unique<StmtNode>( StmtNode::StmtKind::appnd_redrct,
                                    move( left_stmt ),
                                    nullptr,
                                    move( subargs ) );
    } break;

    case Tokenizer::TokenKind::MERG_OUTPUT: { // &>
      tknizr_.consume( Tokenizer::TokenKind::MERG_OUTPUT );
      StmtNode::SiblingNodes subargs;
      subargs.emplace_back( expression() );

      node = make_unique<StmtNode>( StmtNode::StmtKind::merge_output,
                                    move( left_stmt ),
                                    nullptr,
                                    move( subargs ) );
    } break;

    case Tokenizer::TokenKind::MERG_APPND: { // &>>
      tknizr_.consume( Tokenizer::TokenKind::MERG_APPND );
      StmtNode::SiblingNodes subargs;
      subargs.emplace_back( expression() );

      node = make_unique<StmtNode>( StmtNode::StmtKind::merge_appnd,
                                    move( left_stmt ),
                                    nullptr,
                                    move( subargs ) );
    } break;

    default: break;
    }
    return make_unique<StmtNode>( StmtNode::StmtKind::merge_stream,
                                  move( node == nullptr ? left_stmt : node ),
                                  nullptr,
                                  move( arguments ) );
  }

  Parser::StmtNodePtr Parser::logical_not()
  {
    tknizr_.consume( Tokenizer::TokenKind::NOT );

    if ( tknizr_.peek().is( Tokenizer::TokenKind::CMD )
         || tknizr_.peek().is( Tokenizer::TokenKind::STR ) ) {
      return make_unique<StmtNode>( StmtNode::StmtKind::logical_not, expression() );
    } else if ( tknizr_.peek().is( Tokenizer::TokenKind::LPAREN ) ) {
      tknizr_.consume( Tokenizer::TokenKind::LPAREN );
      return make_unique<StmtNode>( StmtNode::StmtKind::logical_not, inner_statement() );
    } else if ( tknizr_.peek().is( Tokenizer::TokenKind::NOT ) ) {
      return make_unique<StmtNode>( StmtNode::StmtKind::logical_not, logical_not() );
    }

    throw error::SyntaxError( tknizr_.line_pos(),
                              tknizr_.context(),
                              Tokenizer::TokenKind::CMD,
                              tknizr_.peek().type_ );
  }

  Parser::ExprNodePtr Parser::expression()
  {
    if ( !tknizr_.peek().is( Tokenizer::TokenKind::CMD )
         && !tknizr_.peek().is( Tokenizer::TokenKind::STR ) )
      throw error::SyntaxError( tknizr_.line_pos(),
                                tknizr_.context(),
                                Tokenizer::TokenKind::CMD,
                                tknizr_.peek().type_ );

    StmtNode::SiblingNodes arguments;

    const auto token_type = tknizr_.peek().type_;
    const auto token_str =
      tknizr_.consume( token_type == Tokenizer::TokenKind::CMD ? Tokenizer::TokenKind::CMD
                                                               : Tokenizer::TokenKind::STR );

    /* Multiple CMD tokens or STR tokens are treated as a single command,
     * so we need to combine them together and store into sibling nodes.

     * This is because, according to the syntax tree node structure,
     * all subsequent tokens of the command string are parameters of the first
     token. */
    while ( tknizr_.peek().is( Tokenizer::TokenKind::CMD )
            || tknizr_.peek().is( Tokenizer::TokenKind::STR ) ) {
      assert( tknizr_.peek().value_.empty() == false );

      const auto tkn_tp = tknizr_.peek().type_;
      arguments.emplace_back( make_unique<ExprNode>( tkn_tp == Tokenizer::TokenKind::CMD
                                                       ? ExprNode::ExprKind::command
                                                       : ExprNode::ExprKind::string,
                                                     tknizr_.consume( tknizr_.peek().type_ ) ) );
    }

    return make_unique<ExprNode>( arguments.empty() && token_type == Tokenizer::TokenKind::STR
                                    ? ExprNode::ExprKind::string
                                    : ExprNode::ExprKind::command,
                                  move( token_str ),
                                  move( arguments ) );
  }
} // namespace tish
