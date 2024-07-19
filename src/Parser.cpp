#include <ranges>
#include <algorithm>
#include <cassert>

#include "Parser.hpp"
#include "Constants.hpp"
#include "Exception.hpp"
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
      return make_unique<ExprNode>( ExprKind::value, constants::EvalSuccess );
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
        tknizr_.context(), TokenType::CMD, tknizr_.peek().type_
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
      tknizr_.consume( TokenType::AND );
      return make_unique<StmtNode>( StmtKind::logical_and,
        move( left_stmt ), nonempty_statement() );
    }

    case TokenType::OR: {
      tknizr_.consume( TokenType::OR );
      return make_unique<StmtNode>( StmtKind::logical_or,
        move( left_stmt ), nonempty_statement() );
    }

    case TokenType::PIPE: {
      tknizr_.consume( TokenType::PIPE );
      return make_unique<StmtNode>( StmtKind::pipeline,
        move( left_stmt ), nonempty_statement() );
    }

    case TokenType::SEMI: {
      tknizr_.consume( TokenType::SEMI );
      return make_unique<StmtNode>( StmtKind::sequential,
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
        tknizr_.context(), TokenType::NEWLINE, tknizr_.peek().type_
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
        tknizr_.context(), TokenType::CMD, tknizr_.peek().type_
      );
    }
    }

    return inner_statement_extension( move( node ) );
  }

  Parser::StmtNodePtr Parser::inner_statement_extension( Parser::StmtNodePtr left_stmt )
  {
    switch ( tknizr_.peek().type_ ) {
    case TokenType::AND: { // connector
      tknizr_.consume( TokenType::AND );
      return make_unique<StmtNode>( StmtKind::logical_and,
        move( left_stmt ), inner_statement() );
    }

    case TokenType::OR: {
      tknizr_.consume( TokenType::OR );
      return make_unique<StmtNode>( StmtKind::logical_or,
        move( left_stmt ), inner_statement() );
    }

    case TokenType::PIPE: {
      tknizr_.consume( TokenType::PIPE );
      return make_unique<StmtNode>( StmtKind::pipeline,
        move( left_stmt ), inner_statement() );
    }

    case TokenType::SEMI: {
      tknizr_.consume( TokenType::SEMI );

      StmtNodePtr right_stmt;
      if ( tknizr_.peek().is( TokenType::RPAREN ) )
        tknizr_.consume( TokenType::RPAREN );
      else right_stmt = inner_statement();

      return make_unique<StmtNode>( StmtKind::sequential,
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
        tknizr_.context(), TokenType::RPAREN, tknizr_.peek().type_
      );
    }
    }
  }

  Parser::StmtNodePtr Parser::redirection( Parser::StmtNodePtr left_stmt )
  {
    StmtKind stmt_kind = StmtKind::trivial;
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
      tknizr_.consume( TokenType::STDIN_REDIR );
    } break;
    default:
      throw error::SyntaxError(
        tknizr_.context(), TokenType::OVR_REDIR, tknizr_.peek().type_
      );
    }

    if ( tknizr_.peek().type_ != TokenType::CMD ) {
      throw error::SyntaxError(
        tknizr_.context(), TokenType::CMD, tknizr_.peek().type_
      );
    }

    // The structure of syntax tree node
    // requires that redirected file name argument be stored in sibling nodes.
    StmtNode::SiblingNodes arguments;
    arguments.push_back( expression() );

    return make_unique<StmtNode>( stmt_kind,
      move( left_stmt ), nullptr, move( arguments ) );
  }

  Parser::StmtNodePtr Parser::output_redirection( Parser::StmtNodePtr left_stmt )
  {
    StmtKind stmt_kind = StmtKind::trivial;
    StmtNode::SiblingNodes arguments;

    auto extract_params = [this]( StmtNode::SiblingNodes& args, types::StrViewT re_str, TokenType expecting ) {
      auto [match_result, matches] = utils::match_string(
        tknizr_.consume( expecting ),
        re_str
      );

      assert( match_result == true );
      if ( expecting == TokenType::MERG_STREAM ) {
        for ( size_t i = 1; i <= 2; ++i ) {
          if ( auto match_str = matches[i].str();
               match_str.empty() )
            args.push_back( make_unique<ExprNode>( ExprKind::value, constants::InvalidValue ) );
          else args.push_back( make_unique<ExprNode>( ExprKind::value, stoi( match_str ) ) );
        }
      } else {
        if ( auto match_str = matches[1].str();
             match_str.empty() )
          args.push_back( make_unique<ExprNode>( ExprKind::value, constants::InvalidValue ) );
        else args.push_back( make_unique<ExprNode>( ExprKind::value, stoi( match_str ) ) );
      }
    };

    switch ( tknizr_.peek().type_ ) {
    case TokenType::OVR_REDIR: { // >
      stmt_kind = StmtKind::ovrwrit_redrct;
      extract_params( arguments, redirection_regex, TokenType::OVR_REDIR );
      assert( arguments.size() == 1 );
    } break;
    case TokenType::APND_REDIR: { // >>
      stmt_kind = StmtKind::appnd_redrct;
      extract_params( arguments, redirection_regex, TokenType::APND_REDIR );
      assert( arguments.size() == 1 );
    } break;
    case TokenType::MERG_OUTPUT: { // &>
      stmt_kind = StmtKind::merge_output;
      tknizr_.consume( TokenType::MERG_OUTPUT );
    } break;
    case TokenType::MERG_APPND: { // &>>
      stmt_kind = StmtKind::merge_appnd;
      tknizr_.consume( TokenType::MERG_APPND );
    } break;
    case TokenType::MERG_STREAM: { // >&
      return combined_redirection_extension( move( left_stmt ) );
    }
    default:
      throw error::SyntaxError(
        tknizr_.context(), TokenType::OVR_REDIR, tknizr_.peek().type_
      );
    }

    arguments.push_back( expression() );

    // The left operator takes precedence, which means `MERG_STREAM` will be the child node.
    if ( tknizr_.peek().is( TokenType::MERG_STREAM ) ) { // output_redirecti
      StmtNode::SiblingNodes subargs;
      extract_params( subargs, combined_redir_regex, TokenType::MERG_STREAM );
      assert( subargs.size() == 2 );

      return make_unique<StmtNode>(
        stmt_kind, 
        make_unique<StmtNode>( StmtKind::merge_stream,
          move( left_stmt ), nullptr, move( subargs ) ),
        nullptr, move( arguments )
      );
    }
    return make_unique<StmtNode>( stmt_kind,
      move( left_stmt ), nullptr, move( arguments ) );
  }

  Parser::StmtNodePtr Parser::combined_redirection_extension( Parser::StmtNodePtr left_stmt )
  {
    assert( tknizr_.peek().is( TokenType::MERG_STREAM ) );

    StmtNode::SiblingNodes arguments;
    auto extract_params = [this]( StmtNode::SiblingNodes& args, types::StrViewT re_str, TokenType expecting ) {
      auto [match_result, matches] = utils::match_string(
        tknizr_.consume( expecting ),
        re_str
      );

      assert( match_result == true );
      if ( expecting == TokenType::MERG_STREAM ) {
        for ( size_t i = 1; i <= 2; ++i ) {
          if ( auto match_str = matches[i].str();
               match_str.empty() )
            args.push_back( make_unique<ExprNode>( ExprKind::value, constants::InvalidValue ) );
          else args.push_back( make_unique<ExprNode>( ExprKind::value, stoi( match_str ) ) );
        }
      } else {
        if ( auto match_str = matches[1].str();
             match_str.empty() )
          args.push_back( make_unique<ExprNode>( ExprKind::value, constants::InvalidValue ) );
        else args.push_back( make_unique<ExprNode>( ExprKind::value, stoi( match_str ) ) );
      }
    };

    extract_params( arguments, combined_redir_regex, TokenType::MERG_STREAM );
    assert( arguments.size() == 2 );

    StmtNodePtr node = nullptr;
    switch ( tknizr_.peek().type_ ) {
    case TokenType::OVR_REDIR: { // >
      StmtNode::SiblingNodes subargs;
      extract_params( subargs, redirection_regex, TokenType::OVR_REDIR );
      assert( subargs.size() == 1 );
      subargs.push_back( expression() );

      // The left operator takes precedence, which means `MERG_STREAM` will be the parent node.
      node = make_unique<StmtNode>( StmtKind::ovrwrit_redrct,
        move( left_stmt ), nullptr, move( subargs ) );
    } break;

    case TokenType::APND_REDIR: { // >>
      StmtNode::SiblingNodes subargs;
      extract_params( subargs, redirection_regex, TokenType::APND_REDIR );
      assert( subargs.size() == 1 );
      subargs.push_back( expression() );

      node = make_unique<StmtNode>( StmtKind::appnd_redrct,
        move( left_stmt ), nullptr, move( subargs ) );
    } break;

    case TokenType::MERG_OUTPUT: { // &>
      tknizr_.consume( TokenType::MERG_OUTPUT );
      StmtNode::SiblingNodes subargs;
      subargs.push_back( expression() );

      node = make_unique<StmtNode>( StmtKind::merge_output,
        move( left_stmt ), nullptr, move( subargs ) );
    } break;

    case TokenType::MERG_APPND: { // &>>
      tknizr_.consume( TokenType::MERG_APPND );
      StmtNode::SiblingNodes subargs;
      subargs.push_back( expression() );

      node = make_unique<StmtNode>( StmtKind::merge_appnd,
        move( left_stmt ), nullptr, move( subargs ) );
    } break;

    default:
      break;
    }
    return make_unique<StmtNode>( StmtKind::merge_stream,
      move( node == nullptr ? left_stmt : node ), nullptr, move( arguments ) );
  }

  Parser::StmtNodePtr Parser::logical_not()
  {
    tknizr_.consume( TokenType::NOT );
    StmtNodePtr node;

    if ( tknizr_.peek().is( TokenType::CMD ) || tknizr_.peek().is( TokenType::STR ) ) {
      node = make_unique<StmtNode>( StmtKind::logical_not, expression() );
    } else if ( tknizr_.peek().is( TokenType::LPAREN ) ) {
      tknizr_.consume( TokenType::LPAREN );

      node = make_unique<StmtNode>( StmtKind::logical_not, inner_statement() );
    } else throw error::SyntaxError(
      tknizr_.context(), TokenType::CMD, tknizr_.peek().type_
    );

    return node;
  }

  Parser::ExprNodePtr Parser::expression()
  {
    if ( !tknizr_.peek().is( TokenType::CMD ) && !tknizr_.peek().is( TokenType::STR ) )
      throw error::SyntaxError(
        tknizr_.context(), TokenType::CMD, tknizr_.peek().type_
      );

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
      arguments.push_back( make_unique<ExprNode>( expr_kind_map( token_type ),
        tknizr_.consume( token_type ) ) );
    }

    return make_unique<ExprNode>( arguments.empty() ? node_kind : ExprKind::command,
      move( token ), move( arguments ) );
  }
}
