#include <cassert>
#include "Parser.hpp"
#include "Utils.hpp"
using namespace std;

namespace hull {
  Parser::StmtNodePtr Parser::parse()
  {
    if ( tknizr_.empty() )
      throw error::error_factory( error::info::ArgumentErrorInfo(
        "parser"sv, "empty tokenizer"sv
      ) );
    return statement();
  }

  Parser::StmtNodePtr Parser::statement()
  {
    Parser::StmtNodePtr node;
    switch ( const auto tkn_tp = tknizr_.peek().type_;
             tkn_tp ) {
    case TokenType::CMD: {
      node = expression();
    } break;

    case TokenType::OVR_REDIR:
      [[fallthrough]];
    case TokenType::APND_REDIR: {
      return statement_extension( redirection( nullptr ) );
    }

    case TokenType::NOT: {
      tknizr_.consume( TokenType::NOT );
      return make_unique<StmtNode>( StmtKind::logical_not, statement(), nullptr );
    }

    case TokenType::LPAREN: {
      tknizr_.consume( TokenType::LPAREN );
      node = inner_statement();
      tknizr_.consume( TokenType::RPAREN );
    } break;

    case TokenType::ENDFILE: // empty statement
      [[fallthrough]];
    case TokenType::NEWLINE: {
      tknizr_.consume( tkn_tp );
      return make_unique<ExprNode>( StmtKind::trivial, ExprKind::command, val_decl::EvalSuccess );
    }

    default: {
      throw error::error_factory( error::info::SyntaxErrorInfo(
        tknizr_.line_pos(), TokenType::CMD, tknizr_.peek().type_
      ) );
    } break;
    }

    return statement_extension( move( node ) );
  }

  Parser::StmtNodePtr Parser::statement_extension( Parser::StmtNodePtr left_stmt )
  {
    switch ( const auto tkn_tp = tknizr_.peek().type_;
             tkn_tp ) {
    case TokenType::AND: {
      tknizr_.consume( TokenType::AND );
      return make_unique<StmtNode>( StmtKind::logical_and, move( left_stmt ), statement() );
    }

    case TokenType::OR: {
      tknizr_.consume( TokenType::OR );
      return make_unique<StmtNode>( StmtKind::logical_or, move( left_stmt ), statement() );
    }

    case TokenType::PIPE: {
      tknizr_.consume( TokenType::PIPE );
      return make_unique<StmtNode>( StmtKind::pipeline, move( left_stmt ), statement() );
    }

    case TokenType::OVR_REDIR: // redirection
      [[fallthrough]];
    case TokenType::APND_REDIR:
      [[fallthrough]];
    case TokenType::STDIN_REDIR: {
      return statement_extension( redirection( move( left_stmt ) ) );
    }

    case TokenType::SEMI: {
      tknizr_.consume( TokenType::SEMI );
      return make_unique<StmtNode>(StmtKind::sequential, move( left_stmt ), statement() );
    }

    case TokenType::ENDFILE:
      [[fallthrough]];
    case TokenType::NEWLINE: {
      tknizr_.consume( tkn_tp );
      return left_stmt;
    } break;


    default:
      throw error::error_factory( error::info::SyntaxErrorInfo(
        tknizr_.line_pos(), TokenType::NEWLINE, tknizr_.peek().type_
      ) );
    }
  }

  Parser::StmtNodePtr Parser::inner_statement()
  {
    Parser::StmtNodePtr node;
    switch ( tknizr_.peek().type_ ) {
    case TokenType::CMD: {
      node = expression();
    } break;

    case TokenType::NOT: {
      tknizr_.consume( TokenType::NOT );
      return make_unique<StmtNode>( StmtKind::logical_not, inner_statement(), nullptr );
    } break;

    case TokenType::LPAREN: {
      tknizr_.consume( TokenType::LPAREN );
      node = inner_statement();
      tknizr_.consume( TokenType::RPAREN );
    } break;

    case TokenType::RPAREN: {
      return make_unique<ExprNode>( StmtKind::trivial, ExprKind::command, val_decl::EvalSuccess );
    } break;

    default: {
      throw error::error_factory( error::info::SyntaxErrorInfo(
        tknizr_.line_pos(), TokenType::CMD, tknizr_.peek().type_
      ) );
    } break;
    }

    return inner_statement_extension( move( node ) );
  }

  Parser::StmtNodePtr Parser::inner_statement_extension( Parser::StmtNodePtr left_stmt )
  {
    switch ( tknizr_.peek().type_ ) {
    case TokenType::AND: {
      tknizr_.consume( TokenType::AND );
      return make_unique<StmtNode>( StmtKind::logical_and, move( left_stmt ), inner_statement() );
    }

    case TokenType::OR: {
      tknizr_.consume( TokenType::OR );
      return make_unique<StmtNode>( StmtKind::logical_or, move( left_stmt ), inner_statement() );
    }

    case TokenType::PIPE: {
      tknizr_.consume( TokenType::PIPE );
      return make_unique<StmtNode>( StmtKind::pipeline, move( left_stmt ), inner_statement() );
    }

    case TokenType::OVR_REDIR: // redirection
      [[fallthrough]];
    case TokenType::APND_REDIR:
      [[fallthrough]];
    case TokenType::STDIN_REDIR: {
      return inner_statement_extension( redirection( move( left_stmt ) ) );
    }

    case TokenType::SEMI: {
      tknizr_.consume( TokenType::SEMI );
      return make_unique<StmtNode>(StmtKind::sequential, move( left_stmt ), inner_statement() );
    }

    case TokenType::RPAREN: {
      return left_stmt;
    }

    default:
      throw error::error_factory( error::info::SyntaxErrorInfo(
        tknizr_.line_pos(), TokenType::RPAREN, tknizr_.peek().type_
      ) );
    }
  }

  Parser::StmtNodePtr Parser::redirection( Parser::StmtNodePtr left_stmt )
  {
    if (tknizr_.peek().type_ == TokenType::OVR_REDIR) {
      tknizr_.consume( TokenType::OVR_REDIR );
      return make_unique<StmtNode>(
        StmtKind::ovrwrit_redrct, move( left_stmt ), expression() );
    } else if ( tknizr_.peek().is( TokenType::APND_REDIR ) ) {
      tknizr_.consume( TokenType::APND_REDIR );
      return make_unique<StmtNode>(
        StmtKind::appnd_redrct, move( left_stmt ), expression() );
    } else if ( tknizr_.peek().is( TokenType::STDIN_REDIR ) ) {
      tknizr_.consume( TokenType::STDIN_REDIR );
      return make_unique<StmtNode>(
        StmtKind::stdin_redrct, move( left_stmt ), expression() );
    }

    throw error::error_factory( error::info::SyntaxErrorInfo(
      tknizr_.line_pos(), TokenType::OVR_REDIR, tknizr_.peek().type_
    ) );
  }

  Parser::ExprNodePtr Parser::expression()
  {
    type_decl::TokenT value = move( tknizr_.peek().value_ );
    tknizr_.consume( TokenType::CMD );
    return make_unique<ExprNode>( StmtKind::trivial, ExprKind::command, move( value ) );
  }
}
