#include <ranges>
#include <algorithm>
#include <cassert>

#include "Parser.hpp"
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
    switch ( const auto tkn_tp = tknizr_.peek().type_;
             tkn_tp ) {
    case TokenType::ENDFILE: // empty statement
      [[fallthrough]];
    case TokenType::NEWLINE: {
      tknizr_.consume( tkn_tp );
      return make_unique<ExprNode>( StmtKind::trivial,
        ExprKind::value, val_decl::EvalSuccess );
    }

    default: {
      return nonempty_statement();
    }
    }
  }

  Parser::StmtNodePtr Parser::nonempty_statement()
  {
    Parser::StmtNodePtr node;
    switch ( const auto tkn_tp = tknizr_.peek().type_;
             tkn_tp ) {
    case TokenType::CMD:
      [[fallthrough]];
    case TokenType::STR: {
      node = expression();
    } break;

    case TokenType::OVR_REDIR:
      [[fallthrough]];
    case TokenType::APND_REDIR:
      [[fallthrough]];
    case TokenType::MERG_OUTPUT:
      [[fallthrough]];
    case TokenType::MERG_APPND: {
      node = redirection( nullptr );
    } break;

    case TokenType::LPAREN: {
      tknizr_.consume( TokenType::LPAREN );
      node = inner_statement();
    } break;

    case TokenType::NOT: {
      node = logical_not();
    } break;

    default: {
      throw error::SyntaxError(
        tknizr_.line_pos(), TokenType::CMD, tknizr_.peek().type_
      );
    }
    }

    return statement_extension( move( node ) );
  }

  Parser::StmtNodePtr Parser::statement_extension( Parser::StmtNodePtr left_stmt )
  {
    switch ( const auto tkn_tp = tknizr_.peek().type_;
             tkn_tp ) {
    case TokenType::AND: { // connector
      auto optr = tknizr_.consume( TokenType::AND );
      return make_unique<StmtNode>( StmtKind::logical_and, move( optr ),
        move( left_stmt ), nonempty_statement() );
    }

    case TokenType::OR: {
      auto optr = tknizr_.consume( TokenType::OR );
      return make_unique<StmtNode>( StmtKind::logical_or, move( optr ),
        move( left_stmt ), nonempty_statement() );
    }

    case TokenType::PIPE: {
      auto optr = tknizr_.consume( TokenType::PIPE );
      return make_unique<StmtNode>( StmtKind::pipeline, move( optr ),
        move( left_stmt ), nonempty_statement() );
    }

    case TokenType::SEMI: {
      auto optr = tknizr_.consume( TokenType::SEMI );
      return make_unique<StmtNode>( StmtKind::sequential, move( optr ),
        move( left_stmt ), nonempty_statement() );
    }

    case TokenType::OVR_REDIR: // redirection
      [[fallthrough]];
    case TokenType::APND_REDIR:
      [[fallthrough]];
    case TokenType::MERG_OUTPUT:
      [[fallthrough]];
    case TokenType::MERG_APPND:
      [[fallthrough]];
    case TokenType::MERG_STREAM:
      [[fallthrough]];
    case TokenType::STDIN_REDIR: {
      return statement_extension( redirection( move( left_stmt ) ) );
    }

    case TokenType::ENDFILE:
      [[fallthrough]];
    case TokenType::NEWLINE: {
      tknizr_.consume( tkn_tp );
      return left_stmt;
    }

    default:
      throw error::SyntaxError(
        tknizr_.line_pos(), TokenType::NEWLINE, tknizr_.peek().type_
      );
    }
  }

  Parser::StmtNodePtr Parser::inner_statement()
  {
    Parser::StmtNodePtr node;
    switch ( tknizr_.peek().type_ ) {
    case TokenType::CMD:
      [[fallthrough]];
    case TokenType::STR: {
      node = expression();
    } break;

    case TokenType::OVR_REDIR:
      [[fallthrough]];
    case TokenType::APND_REDIR:
      [[fallthrough]];
    case TokenType::MERG_OUTPUT:
      [[fallthrough]];
    case TokenType::MERG_APPND:
      [[fallthrough]];
    case TokenType::MERG_STREAM: {
      node = redirection( nullptr );
    } break;

    case TokenType::NOT: {
      node = logical_not();
    } break;

    case TokenType::LPAREN: {
      tknizr_.consume( TokenType::LPAREN );
      node = inner_statement();
    } break;

    default: {
      throw error::SyntaxError(
        tknizr_.line_pos(), TokenType::CMD, tknizr_.peek().type_
      );
    }
    }

    return inner_statement_extension( move( node ) );
  }

  Parser::StmtNodePtr Parser::inner_statement_extension( Parser::StmtNodePtr left_stmt )
  {
    switch ( tknizr_.peek().type_ ) {
    case TokenType::AND: { // connector
      auto optr = tknizr_.consume( TokenType::AND );
      return make_unique<StmtNode>( StmtKind::logical_and, move( optr ),
        move( left_stmt ), inner_statement() );
    }

    case TokenType::OR: {
      auto optr = tknizr_.consume( TokenType::OR );
      return make_unique<StmtNode>( StmtKind::logical_or, move( optr ),
        move( left_stmt ), inner_statement() );
    }

    case TokenType::PIPE: {
      auto optr = tknizr_.consume( TokenType::PIPE );
      return make_unique<StmtNode>( StmtKind::pipeline, move( optr ),
        move( left_stmt ), inner_statement() );
    }

    case TokenType::SEMI: {
      auto optr = tknizr_.consume( TokenType::SEMI );

      StmtNodePtr right_stmt;
      if ( tknizr_.peek().is( TokenType::RPAREN ) )
        tknizr_.consume( TokenType::RPAREN );
      else right_stmt = inner_statement();

      return make_unique<StmtNode>( StmtKind::sequential, move( optr ),
        move( left_stmt ), move( right_stmt ) );
    }

    case TokenType::OVR_REDIR: // redirection
      [[fallthrough]];
    case TokenType::APND_REDIR:
      [[fallthrough]];
    case TokenType::MERG_OUTPUT:
      [[fallthrough]];
    case TokenType::MERG_APPND:
      [[fallthrough]];
    case TokenType::MERG_STREAM:
      [[fallthrough]];
    case TokenType::STDIN_REDIR: {
      return inner_statement_extension( redirection( move( left_stmt ) ) );
    }

    case TokenType::RPAREN: {
      tknizr_.consume( TokenType::RPAREN );
      return left_stmt;
    }

    default: {
      throw error::SyntaxError(
        tknizr_.line_pos(), TokenType::RPAREN, tknizr_.peek().type_
      );
    }
    }
  }

  Parser::StmtNodePtr Parser::redirection( Parser::StmtNodePtr left_stmt )
  {
    StmtKind stmt_kind = StmtKind::trivial;
    type_decl::TokenT optr;
    switch ( tknizr_.peek().type_ ) {
    case TokenType::OVR_REDIR:
      [[fallthrough]];
    case TokenType::APND_REDIR:
      [[fallthrough]];
    case TokenType::MERG_OUTPUT:
      [[fallthrough]];
    case TokenType::MERG_APPND:
      [[fallthrough]];
    case TokenType::MERG_STREAM: {
      return output_redirection( move( left_stmt ) );
    }
    case TokenType::STDIN_REDIR: {
      stmt_kind = StmtKind::stdin_redrct;
      optr = tknizr_.consume( TokenType::STDIN_REDIR );
    } break;
    default:
      throw error::SyntaxError(
        tknizr_.line_pos(), TokenType::OVR_REDIR, tknizr_.peek().type_
      );
    }

    if ( tknizr_.peek().type_ != TokenType::CMD ) {
      throw error::SyntaxError(
        tknizr_.line_pos(), TokenType::CMD, tknizr_.peek().type_
      );
    }

    // The structure of syntax tree node
    // requires that redirected file name argument be stored in sibling nodes.
    StmtNode::SiblingNodes arguments;
    arguments.push_back( expression() );

    return make_unique<StmtNode>( stmt_kind, move( optr ),
      move( left_stmt ), nullptr, move( arguments ) );
  }

  Parser::StmtNodePtr Parser::output_redirection( Parser::StmtNodePtr left_stmt )
  {
    StmtKind stmt_kind = StmtKind::trivial;
    type_decl::TokenT optr;
    switch ( tknizr_.peek().type_ ) {
    case TokenType::OVR_REDIR: { // >
      stmt_kind = StmtKind::ovrwrit_redrct;
      optr = tknizr_.consume( TokenType::OVR_REDIR );
    } break;
    case TokenType::APND_REDIR: { // >>
      stmt_kind = StmtKind::appnd_redrct;
      optr = tknizr_.consume( TokenType::APND_REDIR );
    } break;
    case TokenType::MERG_OUTPUT: { // &>
      stmt_kind = StmtKind::merge_output;
      optr = tknizr_.consume( TokenType::MERG_OUTPUT );
    } break;
    case TokenType::MERG_APPND: { // &>>
      stmt_kind = StmtKind::merge_appnd;
      optr = tknizr_.consume( TokenType::MERG_APPND );
    } break;
    case TokenType::MERG_STREAM: { // >&
      return combined_redirection_extension( move( left_stmt ) );
    }
    default:
      throw error::SyntaxError(
        tknizr_.line_pos(), TokenType::OVR_REDIR, tknizr_.peek().type_
      );
    }

    StmtNode::SiblingNodes arguments;
    arguments.push_back( expression() );

    // The left operator takes precedence, which means `MERG_STREAM` will be the child node.
    if ( tknizr_.peek().is( TokenType::MERG_STREAM ) ) { // output_redirection_extension
      return make_unique<StmtNode>(
        stmt_kind, move( optr ),
        make_unique<StmtNode>( StmtKind::merge_stream,
          tknizr_.consume( TokenType::MERG_STREAM ), move( left_stmt ) ),
        nullptr, move( arguments )
      );
    }
    return make_unique<StmtNode>( stmt_kind,
      move( optr ), move( left_stmt ), nullptr, move( arguments ) );
  }

  Parser::StmtNodePtr Parser::combined_redirection_extension( Parser::StmtNodePtr left_stmt )
  {
    assert( tknizr_.peek().is( TokenType::MERG_STREAM ) );
    auto optr = tknizr_.consume( TokenType::MERG_STREAM );

    switch ( tknizr_.peek().type_ ) {
    case TokenType::OVR_REDIR: { // >
      auto sub_optr = tknizr_.consume( TokenType::OVR_REDIR );
      StmtNode::SiblingNodes arguments;
      arguments.push_back( expression() );

      // The left operator takes precedence, which means `MERG_STREAM` will be the parent node.
      return make_unique<StmtNode>(
        StmtKind::merge_stream, move( optr ),
        make_unique<StmtNode>( StmtKind::ovrwrit_redrct,
          move( sub_optr ), move( left_stmt ), nullptr, move( arguments ) )
      );
    }

    case TokenType::APND_REDIR: { // >>
      auto sub_optr = tknizr_.consume( TokenType::APND_REDIR );
      StmtNode::SiblingNodes arguments;
      arguments.push_back( expression() );

      return make_unique<StmtNode>(
        StmtKind::merge_stream, move( optr ),
        make_unique<StmtNode>( StmtKind::appnd_redrct,
          move( sub_optr ), move( left_stmt ), nullptr, move( arguments ) )
      );
    }

    case TokenType::MERG_OUTPUT: { // &>
      auto sub_optr = tknizr_.consume( TokenType::MERG_OUTPUT );
      StmtNode::SiblingNodes arguments;
      arguments.push_back( expression() );

      return make_unique<StmtNode>(
        StmtKind::merge_stream, move( optr ),
        make_unique<StmtNode>( StmtKind::merge_output,
          move( sub_optr ), move( left_stmt ), nullptr, move( arguments ) )
      );
    }

    case TokenType::MERG_APPND: { // &>>
      auto sub_optr = tknizr_.consume( TokenType::MERG_APPND );
      StmtNode::SiblingNodes arguments;
      arguments.push_back( expression() );

      return make_unique<StmtNode>(
        StmtKind::merge_stream, move( optr ),
        make_unique<StmtNode>( StmtKind::merge_appnd,
          move( sub_optr ), move( left_stmt ), nullptr, move( arguments ) )
      );
    }

    default: {
      return make_unique<StmtNode>( StmtKind::merge_stream,
        move( optr ), move( left_stmt ) );
    }
    }
  }

  Parser::StmtNodePtr Parser::logical_not()
  {
    auto optr = tknizr_.consume( TokenType::NOT );
    StmtNodePtr node;

    if ( tknizr_.peek().is( TokenType::CMD ) || tknizr_.peek().is( TokenType::STR ) ) {
      node = make_unique<StmtNode>( StmtKind::logical_not,
        move( optr ), expression() );
    } else if ( tknizr_.peek().is( TokenType::LPAREN ) ) {
      tknizr_.consume( TokenType::LPAREN );

      node = make_unique<StmtNode>( StmtKind::logical_not,
        move( optr ), inner_statement() );
    } else throw error::SyntaxError(
      tknizr_.line_pos(), TokenType::CMD, tknizr_.peek().type_
    );

    return node;
  }

  Parser::ExprNodePtr Parser::expression()
  {
    assert( tknizr_.peek().is( TokenType::CMD ) || tknizr_.peek().is( TokenType::STR ) );

    ExprNode::SiblingNodes arguments;

    auto expr_kind_map = []( TokenType tkn_tp ) -> ExprKind {
      assert( tkn_tp == TokenType::CMD || tkn_tp == TokenType::STR );
      return tkn_tp == TokenType::CMD ? ExprKind::command : ExprKind::string;
    };
    ExprKind node_kind = expr_kind_map( tknizr_.peek().type_ );
    auto token = tknizr_.peek().is( TokenType::CMD )
      ? tknizr_.consume( TokenType::CMD )
      : tknizr_.consume( TokenType::STR );

    /* Multiple CMD tokens or STR tokens are treated as a single command,
     * so we need to combine them together and store into sibling nodes.

     * This is because, according to the syntax tree node structure,
     * all subsequent tokens of the command string are parameters of the first token. */
    while ( tknizr_.peek().is( TokenType::CMD ) || tknizr_.peek().is( TokenType::STR ) ) {
      assert( tknizr_.peek().value_.empty() == false );

      const TokenType token_type = tknizr_.peek().type_;
      arguments.push_back( make_unique<ExprNode>( StmtKind::trivial,
        expr_kind_map( token_type ), tknizr_.consume( token_type ) ) );
    }

    return make_unique<ExprNode>( StmtKind::trivial,
      arguments.empty() ? node_kind : ExprKind::command, move( token ), move( arguments ) );
  }
}
