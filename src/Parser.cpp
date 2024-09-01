#include <algorithm>
#include <cassert>
#include <ranges>

#include "Constants.hpp"
#include "Exception.hpp"
#include "Parser.hpp"
#include "TreeNode.hpp"
#include "Utils.hpp"
using namespace std;

namespace simsh {
  Parser::StmtNodePtr Parser::parse()
  {
    tknizr_.clear();
    return statement();
  }

  Parser::StmtNodePtr Parser::statement()
  {
    switch ( const auto tkn_tp = tknizr_.peek().type_; tkn_tp ) {
    case types::TokenType::ENDFILE: // empty statement
      [[fallthrough]];
    case types::TokenType::NEWLINE: {
      tknizr_.consume( tkn_tp );
      return make_unique<ExprNode>( types::ExprKind::value, constants::EvalSuccess );
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
    case types::TokenType::CMD: [[fallthrough]];
    case types::TokenType::STR: {
      node = expression();
    } break;

    case types::TokenType::OVR_REDIR:   [[fallthrough]];
    case types::TokenType::APND_REDIR:  [[fallthrough]];
    case types::TokenType::MERG_OUTPUT: [[fallthrough]];
    case types::TokenType::MERG_APPND:  {
      node = redirection( nullptr );
    } break;

    case types::TokenType::LPAREN: {
      tknizr_.consume( types::TokenType::LPAREN );
      node = inner_statement();
    } break;

    case types::TokenType::NOT: {
      node = logical_not();
    } break;

    default: {
      throw error::SyntaxError(
        tknizr_.line_pos(), tknizr_.context(), types::TokenType::CMD, tknizr_.peek().type_ );
    }
    }

    return statement_extension( move( node ) );
  }

  Parser::StmtNodePtr Parser::statement_extension( Parser::StmtNodePtr left_stmt )
  {
    switch ( const auto tkn_tp = tknizr_.peek().type_; tkn_tp ) {
    case types::TokenType::AND: { // connector
      tknizr_.consume( types::TokenType::AND );
      return make_unique<StmtNode>(
        types::StmtKind::logical_and, move( left_stmt ), nonempty_statement() );
    }

    case types::TokenType::OR: {
      tknizr_.consume( types::TokenType::OR );
      return make_unique<StmtNode>(
        types::StmtKind::logical_or, move( left_stmt ), nonempty_statement() );
    }

    case types::TokenType::PIPE: {
      tknizr_.consume( types::TokenType::PIPE );
      return make_unique<StmtNode>(
        types::StmtKind::pipeline, move( left_stmt ), nonempty_statement() );
    }

    case types::TokenType::SEMI: {
      tknizr_.consume( types::TokenType::SEMI );
      return make_unique<StmtNode>(
        types::StmtKind::sequential, move( left_stmt ), nonempty_statement() );
    }

    case types::TokenType::OVR_REDIR: // redirection
      [[fallthrough]];
    case types::TokenType::APND_REDIR:  [[fallthrough]];
    case types::TokenType::MERG_OUTPUT: [[fallthrough]];
    case types::TokenType::MERG_APPND:  [[fallthrough]];
    case types::TokenType::MERG_STREAM: [[fallthrough]];
    case types::TokenType::STDIN_REDIR: {
      return statement_extension( redirection( move( left_stmt ) ) );
    }

    case types::TokenType::ENDFILE: [[fallthrough]];
    case types::TokenType::NEWLINE: {
      tknizr_.consume( tkn_tp );
      return left_stmt;
    }

    default:
      throw error::SyntaxError(
        tknizr_.line_pos(), tknizr_.context(), types::TokenType::NEWLINE, tknizr_.peek().type_ );
    }
  }

  Parser::StmtNodePtr Parser::inner_statement()
  {
    Parser::StmtNodePtr node;
    switch ( tknizr_.peek().type_ ) {
    case types::TokenType::CMD: [[fallthrough]];
    case types::TokenType::STR: {
      node = expression();
    } break;

    case types::TokenType::OVR_REDIR:   [[fallthrough]];
    case types::TokenType::APND_REDIR:  [[fallthrough]];
    case types::TokenType::MERG_OUTPUT: [[fallthrough]];
    case types::TokenType::MERG_APPND:  [[fallthrough]];
    case types::TokenType::MERG_STREAM: {
      node = redirection( nullptr );
    } break;

    case types::TokenType::NOT: {
      node = logical_not();
    } break;

    case types::TokenType::LPAREN: {
      tknizr_.consume( types::TokenType::LPAREN );
      node = inner_statement();
    } break;

    default: {
      throw error::SyntaxError(
        tknizr_.line_pos(), tknizr_.context(), types::TokenType::CMD, tknizr_.peek().type_ );
    }
    }

    return inner_statement_extension( move( node ) );
  }

  Parser::StmtNodePtr Parser::inner_statement_extension( Parser::StmtNodePtr left_stmt )
  {
    switch ( tknizr_.peek().type_ ) {
    case types::TokenType::AND: { // connector
      tknizr_.consume( types::TokenType::AND );
      return make_unique<StmtNode>(
        types::StmtKind::logical_and, move( left_stmt ), inner_statement() );
    }

    case types::TokenType::OR: {
      tknizr_.consume( types::TokenType::OR );
      return make_unique<StmtNode>(
        types::StmtKind::logical_or, move( left_stmt ), inner_statement() );
    }

    case types::TokenType::PIPE: {
      tknizr_.consume( types::TokenType::PIPE );
      return make_unique<StmtNode>(
        types::StmtKind::pipeline, move( left_stmt ), inner_statement() );
    }

    case types::TokenType::SEMI: {
      tknizr_.consume( types::TokenType::SEMI );

      StmtNodePtr right_stmt;
      if ( tknizr_.peek().is( types::TokenType::RPAREN ) )
        tknizr_.consume( types::TokenType::RPAREN );
      else right_stmt = inner_statement();

      return make_unique<StmtNode>(
        types::StmtKind::sequential, move( left_stmt ), move( right_stmt ) );
    }

    case types::TokenType::OVR_REDIR: // redirection
      [[fallthrough]];
    case types::TokenType::APND_REDIR:  [[fallthrough]];
    case types::TokenType::MERG_OUTPUT: [[fallthrough]];
    case types::TokenType::MERG_APPND:  [[fallthrough]];
    case types::TokenType::MERG_STREAM: [[fallthrough]];
    case types::TokenType::STDIN_REDIR: {
      return inner_statement_extension( redirection( move( left_stmt ) ) );
    }

    case types::TokenType::RPAREN: {
      tknizr_.consume( types::TokenType::RPAREN );
      return left_stmt;
    }

    default: {
      throw error::SyntaxError(
        tknizr_.line_pos(), tknizr_.context(), types::TokenType::RPAREN, tknizr_.peek().type_ );
    }
    }
  }

  Parser::StmtNodePtr Parser::redirection( Parser::StmtNodePtr left_stmt )
  {
    types::StmtKind stmt_kind = types::StmtKind::trivial;
    switch ( tknizr_.peek().type_ ) {
    case types::TokenType::OVR_REDIR:   [[fallthrough]];
    case types::TokenType::APND_REDIR:  [[fallthrough]];
    case types::TokenType::MERG_OUTPUT: [[fallthrough]];
    case types::TokenType::MERG_APPND:  [[fallthrough]];
    case types::TokenType::MERG_STREAM: {
      return output_redirection( move( left_stmt ) );
    }
    case types::TokenType::STDIN_REDIR: {
      stmt_kind = types::StmtKind::stdin_redrct;
      tknizr_.consume( types::TokenType::STDIN_REDIR );
    } break;
    default:
      throw error::SyntaxError(
        tknizr_.line_pos(), tknizr_.context(), types::TokenType::OVR_REDIR, tknizr_.peek().type_ );
    }

    if ( tknizr_.peek().type_ != types::TokenType::CMD ) {
      throw error::SyntaxError(
        tknizr_.line_pos(), tknizr_.context(), types::TokenType::CMD, tknizr_.peek().type_ );
    }

    // The structure of syntax tree node
    // requires that redirected file name argument be stored in sibling nodes.
    StmtNode::SiblingNodes arguments;
    arguments.emplace_back( expression() );

    return make_unique<StmtNode>( stmt_kind, move( left_stmt ), nullptr, move( arguments ) );
  }

  Parser::StmtNodePtr Parser::output_redirection( Parser::StmtNodePtr left_stmt )
  {
    types::StmtKind stmt_kind = types::StmtKind::trivial;
    StmtNode::SiblingNodes arguments;

    auto extract_params = [this]( StmtNode::SiblingNodes& args,
                                  types::StrViewT re_str,
                                  types::TokenType expecting ) {
      auto [match_result, matches] = utils::match_string( tknizr_.consume( expecting ), re_str );

      assert( match_result == true );
      if ( expecting == types::TokenType::MERG_STREAM ) {
        for ( size_t i = 1; i <= 2; ++i ) {
          if ( auto match_str = matches[i].str(); match_str.empty() )
            args.emplace_back(
              make_unique<ExprNode>( types::ExprKind::value, constants::InvalidValue ) );
          else
            args.emplace_back( make_unique<ExprNode>( types::ExprKind::value, stoi( match_str ) ) );
        }
      } else {
        if ( auto match_str = matches[1].str(); match_str.empty() )
          args.emplace_back(
            make_unique<ExprNode>( types::ExprKind::value, constants::InvalidValue ) );
        else
          args.emplace_back( make_unique<ExprNode>( types::ExprKind::value, stoi( match_str ) ) );
      }
    };

    switch ( tknizr_.peek().type_ ) {
    case types::TokenType::OVR_REDIR: { // >
      stmt_kind = types::StmtKind::ovrwrit_redrct;
      extract_params( arguments, redirection_regex, types::TokenType::OVR_REDIR );
      assert( arguments.size() == 1 );
    } break;
    case types::TokenType::APND_REDIR: { // >>
      stmt_kind = types::StmtKind::appnd_redrct;
      extract_params( arguments, redirection_regex, types::TokenType::APND_REDIR );
      assert( arguments.size() == 1 );
    } break;
    case types::TokenType::MERG_OUTPUT: { // &>
      stmt_kind = types::StmtKind::merge_output;
      tknizr_.consume( types::TokenType::MERG_OUTPUT );
    } break;
    case types::TokenType::MERG_APPND: { // &>>
      stmt_kind = types::StmtKind::merge_appnd;
      tknizr_.consume( types::TokenType::MERG_APPND );
    } break;
    case types::TokenType::MERG_STREAM: { // >&
      return combined_redirection_extension( move( left_stmt ) );
    }
    default:
      throw error::SyntaxError(
        tknizr_.line_pos(), tknizr_.context(), types::TokenType::OVR_REDIR, tknizr_.peek().type_ );
    }

    arguments.emplace_back( expression() );

    // The left operator takes precedence, which means `MERG_STREAM` will be the
    // child node.
    if ( tknizr_.peek().is( types::TokenType::MERG_STREAM ) ) { // output_redirecti
      StmtNode::SiblingNodes subargs;
      extract_params( subargs, combined_redir_regex, types::TokenType::MERG_STREAM );
      assert( subargs.size() == 2 );

      return make_unique<StmtNode>(
        stmt_kind,
        make_unique<StmtNode>(
          types::StmtKind::merge_stream, move( left_stmt ), nullptr, move( subargs ) ),
        nullptr,
        move( arguments ) );
    }
    return make_unique<StmtNode>( stmt_kind, move( left_stmt ), nullptr, move( arguments ) );
  }

  Parser::StmtNodePtr Parser::combined_redirection_extension( Parser::StmtNodePtr left_stmt )
  {
    assert( tknizr_.peek().is( types::TokenType::MERG_STREAM ) );

    StmtNode::SiblingNodes arguments;
    auto extract_params = [this]( StmtNode::SiblingNodes& args,
                                  types::StrViewT re_str,
                                  types::TokenType expecting ) {
      auto [match_result, matches] = utils::match_string( tknizr_.consume( expecting ), re_str );

      assert( match_result == true );
      if ( expecting == types::TokenType::MERG_STREAM ) {
        for ( size_t i = 1; i <= 2; ++i ) {
          if ( auto match_str = matches[i].str(); match_str.empty() )
            args.emplace_back(
              make_unique<ExprNode>( types::ExprKind::value, constants::InvalidValue ) );
          else
            args.emplace_back( make_unique<ExprNode>( types::ExprKind::value, stoi( match_str ) ) );
        }
      } else {
        if ( auto match_str = matches[1].str(); match_str.empty() )
          args.emplace_back(
            make_unique<ExprNode>( types::ExprKind::value, constants::InvalidValue ) );
        else
          args.emplace_back( make_unique<ExprNode>( types::ExprKind::value, stoi( match_str ) ) );
      }
    };

    extract_params( arguments, combined_redir_regex, types::TokenType::MERG_STREAM );
    assert( arguments.size() == 2 );

    StmtNodePtr node = nullptr;
    switch ( tknizr_.peek().type_ ) {
    case types::TokenType::OVR_REDIR: { // >
      StmtNode::SiblingNodes subargs;
      extract_params( subargs, redirection_regex, types::TokenType::OVR_REDIR );
      assert( subargs.size() == 1 );
      subargs.emplace_back( expression() );

      // The left operator takes precedence, which means `MERG_STREAM` will be
      // the parent node.
      node = make_unique<StmtNode>(
        types::StmtKind::ovrwrit_redrct, move( left_stmt ), nullptr, move( subargs ) );
    } break;

    case types::TokenType::APND_REDIR: { // >>
      StmtNode::SiblingNodes subargs;
      extract_params( subargs, redirection_regex, types::TokenType::APND_REDIR );
      assert( subargs.size() == 1 );
      subargs.emplace_back( expression() );

      node = make_unique<StmtNode>(
        types::StmtKind::appnd_redrct, move( left_stmt ), nullptr, move( subargs ) );
    } break;

    case types::TokenType::MERG_OUTPUT: { // &>
      tknizr_.consume( types::TokenType::MERG_OUTPUT );
      StmtNode::SiblingNodes subargs;
      subargs.emplace_back( expression() );

      node = make_unique<StmtNode>(
        types::StmtKind::merge_output, move( left_stmt ), nullptr, move( subargs ) );
    } break;

    case types::TokenType::MERG_APPND: { // &>>
      tknizr_.consume( types::TokenType::MERG_APPND );
      StmtNode::SiblingNodes subargs;
      subargs.emplace_back( expression() );

      node = make_unique<StmtNode>(
        types::StmtKind::merge_appnd, move( left_stmt ), nullptr, move( subargs ) );
    } break;

    default: break;
    }
    return make_unique<StmtNode>( types::StmtKind::merge_stream,
                                  move( node == nullptr ? left_stmt : node ),
                                  nullptr,
                                  move( arguments ) );
  }

  Parser::StmtNodePtr Parser::logical_not()
  {
    tknizr_.consume( types::TokenType::NOT );

    if ( tknizr_.peek().is( types::TokenType::CMD ) || tknizr_.peek().is( types::TokenType::STR ) )
    {
      return make_unique<StmtNode>( types::StmtKind::logical_not, expression() );
    } else if ( tknizr_.peek().is( types::TokenType::LPAREN ) ) {
      tknizr_.consume( types::TokenType::LPAREN );
      return make_unique<StmtNode>( types::StmtKind::logical_not, inner_statement() );
    } else if ( tknizr_.peek().is( types::TokenType::NOT ) ) {
      return make_unique<StmtNode>( types::StmtKind::logical_not, logical_not() );
    }

    throw error::SyntaxError(
      tknizr_.line_pos(), tknizr_.context(), types::TokenType::CMD, tknizr_.peek().type_ );
  }

  Parser::ExprNodePtr Parser::expression()
  {
    if ( !tknizr_.peek().is( types::TokenType::CMD )
         && !tknizr_.peek().is( types::TokenType::STR ) )
      throw error::SyntaxError(
        tknizr_.line_pos(), tknizr_.context(), types::TokenType::CMD, tknizr_.peek().type_ );

    StmtNode::SiblingNodes arguments;

    const auto token_type = tknizr_.peek().type_;
    const auto token_str  = tknizr_.consume(
      token_type == types::TokenType::CMD ? types::TokenType::CMD : types::TokenType::STR );

    /* Multiple CMD tokens or STR tokens are treated as a single command,
     * so we need to combine them together and store into sibling nodes.

     * This is because, according to the syntax tree node structure,
     * all subsequent tokens of the command string are parameters of the first
     token. */
    while ( tknizr_.peek().is( types::TokenType::CMD )
            || tknizr_.peek().is( types::TokenType::STR ) )
    {
      assert( tknizr_.peek().value_.empty() == false );

      const auto tkn_tp = tknizr_.peek().type_;
      arguments.emplace_back( make_unique<ExprNode>(
        tkn_tp == types::TokenType::CMD ? types::ExprKind::command : types::ExprKind::string,
        tknizr_.consume( tknizr_.peek().type_ ) ) );
    }

    return make_unique<ExprNode>( arguments.empty() && token_type == types::TokenType::STR
                                    ? types::ExprKind::string
                                    : types::ExprKind::command,
                                  move( token_str ),
                                  move( arguments ) );
  }
} // namespace simsh
