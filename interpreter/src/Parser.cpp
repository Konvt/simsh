#include <type_traits>
#include <cassert>

#include "Parser.hpp"
#include "Utils.hpp"
using namespace std;

namespace hull {
  Parser::StmtNodePtr Parser::parse()
  {
    tknizr_.clear();
    return statement();
  }

  Parser::StmtNodePtr Parser::statement()
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

    case TokenType::ENDFILE: // empty statement
      [[fallthrough]];
    case TokenType::NEWLINE: {
      tknizr_.consume( tkn_tp );
      return make_unique<ExprNode>( StmtKind::trivial,
        ExprKind::command, val_decl::EvalSuccess );
    }

    default: {
      throw error::error_factory( error::info::SyntaxErrorInfo(
        tknizr_.line_pos(), TokenType::CMD, tknizr_.peek().type_
      ) );
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
        move( left_stmt ), statement() );
    }

    case TokenType::OR: {
      auto optr = tknizr_.consume( TokenType::OR );
      return make_unique<StmtNode>( StmtKind::logical_or, move( optr ),
        move( left_stmt ), statement() );
    }

    case TokenType::PIPE: {
      auto optr = tknizr_.consume( TokenType::PIPE );
      return make_unique<StmtNode>( StmtKind::pipeline, move( optr ),
        move( left_stmt ), statement() );
    }

    case TokenType::SEMI: {
      auto optr = tknizr_.consume( TokenType::SEMI );
      return make_unique<StmtNode>( StmtKind::sequential, move( optr ),
        move( left_stmt ), statement() );
    }

    case TokenType::OVR_REDIR: // redirection
      [[fallthrough]];
    case TokenType::APND_REDIR:
      [[fallthrough]];
    case TokenType::MERG_OUTPUT:
      [[fallthrough]];
    case TokenType::MERG_APPND:
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
      throw error::error_factory( error::info::SyntaxErrorInfo(
        tknizr_.line_pos(), TokenType::NEWLINE, tknizr_.peek().type_
      ) );
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
    case TokenType::MERG_APPND: {
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
      throw error::error_factory( error::info::SyntaxErrorInfo(
        tknizr_.line_pos(), TokenType::CMD, tknizr_.peek().type_
      ) );
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
    case TokenType::STDIN_REDIR: {
      return inner_statement_extension( redirection( move( left_stmt ) ) );
    }

    case TokenType::RPAREN: {
      tknizr_.consume( TokenType::RPAREN );
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
    StmtKind stmt_kind = StmtKind::trivial;
    type_decl::TokenT token_str;
    switch ( tknizr_.peek().type_ ) {
    case TokenType::OVR_REDIR: {
      stmt_kind = StmtKind::ovrwrit_redrct;
      token_str = tknizr_.consume( TokenType::OVR_REDIR );
    } break;
    case TokenType::APND_REDIR: {
      stmt_kind = StmtKind::appnd_redrct;
      token_str = tknizr_.consume( TokenType::APND_REDIR );
    } break;
    case TokenType::MERG_OUTPUT: {
      stmt_kind = StmtKind::merge_output;
      token_str = tknizr_.consume( TokenType::MERG_OUTPUT );
    } break;
    case TokenType::MERG_APPND: {
      stmt_kind = StmtKind::merge_appnd;
      token_str = tknizr_.consume( TokenType::MERG_APPND );
    } break;
    case TokenType::STDIN_REDIR: {
      stmt_kind = StmtKind::stdin_redrct;
      token_str = tknizr_.consume( TokenType::STDIN_REDIR );
    } break;
    default:
      throw error::error_factory( error::info::SyntaxErrorInfo(
        tknizr_.line_pos(), TokenType::OVR_REDIR, tknizr_.peek().type_
      ) );
    }

    if ( tknizr_.peek().type_ != TokenType::CMD ) {
      throw error::error_factory( error::info::SyntaxErrorInfo(
        tknizr_.line_pos(), TokenType::CMD, tknizr_.peek().type_
      ) );
    }
    return make_unique<StmtNode>( stmt_kind, move( token_str ),
      move( left_stmt ), expression() );
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
    } else throw error::error_factory( error::info::SyntaxErrorInfo(
      tknizr_.line_pos(), TokenType::CMD, tknizr_.peek().type_
    ) );

    return node;
  }

  Parser::ExprNodePtr Parser::expression()
  {
    StmtNode::SiblingNode expr_list;
    ExprKind expr_kind = ExprKind::string;

    auto make_sibling = [this, &expr_kind]() -> StmtNode::ChildNode {
      if ( tknizr_.peek().is( TokenType::CMD ) ) {
        expr_kind = ExprKind::command;
        return make_unique<ExprNode>( StmtKind::trivial,
          ExprKind::command, utils::tilde_expansion( tknizr_.consume( TokenType::CMD ) ) );
      }

      return make_unique<ExprNode>( StmtKind::trivial,
        ExprKind::string, tknizr_.consume( TokenType::STR ) );
    };

    static_assert(is_same_v<
      decltype(move( *make_sibling() ).token()),
      decltype(declval<StmtNode>().token())>);
    auto token = move( *make_sibling() ).token();

    while ( tknizr_.peek().is( TokenType::CMD ) || tknizr_.peek().is( TokenType::STR ) ) {
      expr_kind = ExprKind::command; // 超过一条指令就会被视作命令
      expr_list.push_back( make_sibling() );
    }

    return make_unique<ExprNode>( StmtKind::trivial,
      expr_kind, move( token ), move( expr_list ) );
  }
}
