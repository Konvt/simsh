#include <ranges>
#include <algorithm>
#include <format>
#include <cstdlib>
#include <cassert>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "TreeNode.hpp"
#include "Exception.hpp"
#include "Logger.hpp"
#include "Pipe.hpp"
#include "Utils.hpp"
using namespace std;

namespace hull {
  type_decl::EvalT StmtNode::evaluate()
  {
    switch ( category_ ) {
    case StmtKind::sequential: {
      assert( l_child_ != nullptr && r_child_ != nullptr );
      l_child_->evaluate();
      return r_child_->evaluate();
    }

    case StmtKind::logical_and: {
      assert( l_child_ != nullptr && r_child_ != nullptr );
      return l_child_->evaluate() && r_child_->evaluate();
    }

    case StmtKind::logical_or: {
      assert( l_child_ != nullptr && r_child_ != nullptr );
      return l_child_->evaluate() || r_child_->evaluate();
    }

    case StmtKind::logical_not: {
      assert( l_child_ != nullptr && r_child_ == nullptr );
      return l_child_->evaluate() != val_decl::EvalSuccess;
    }

    case StmtKind::pipeline: {
      assert( l_child_ != nullptr && r_child_ != nullptr );

      utils::Pipe pipe;
      utils::close_blocking( pipe.reader().get() );

      for ( size_t i = 0; i < 2; ++i ) {
        if ( pid_t process_id = fork(); process_id < 0 )
          throw error::error_factory( error::info::InitErrorInfo(
            "pipeline"sv, "failed to fork"sv, {} // 在运行时，只有关键系统调用错误才会导致异常抛出
          ) );
        else if ( process_id == 0 ) {
          // child process
          if ( i == 0 ) {
            pipe.reader().close();
            dup2( pipe.writer().get(), STDOUT_FILENO );
            l_child_->evaluate();
          } else {
            pipe.writer().close();
            dup2( pipe.reader().get(), STDIN_FILENO );
            r_child_->evaluate();
          }
          throw error::ProcessSuicide( EXIT_FAILURE );
        } else if ( waitpid( process_id, nullptr, 0 ) < 0 )
          throw error::error_factory( error::info::InitErrorInfo(
            "pipeline", "failed to waitpid"sv, {}
          ) );
      }

      return val_decl::EvalSuccess;
    }

    case StmtKind::appnd_redrct:
      [[fallthrough]];
    case StmtKind::ovrwrit_redrct: {
      assert( r_child_ != nullptr );
      assert( r_child_->category_ == StmtKind::trivial );

      auto& right_expr = static_cast<ExprNode&>(*r_child_.get());
      if ( const size_t expr_size = right_expr.expression().size();
           expr_size > 1 || expr_size == 0 ) {
        utils::logger << error::error_factory( error::info::ArgumentErrorInfo(
          "output redirection"sv, "argument number error"sv
        ) );
        return !val_decl::EvalSuccess;
      }

      const auto& filename = right_expr.expression().front();
      if ( access( filename.c_str(), F_OK ) < 0 && !utils::create_file( filename ) ) { // 判断能否获取文件描述符
        utils::logger << error::error_factory( error::info::InitErrorInfo(
          "output redirection"sv, "open file failed"sv, format( "cannot create '{}'", filename )
        ) );
        return !val_decl::EvalSuccess;
      } else if ( access( filename.c_str(), W_OK ) < 0 ) {
        utils::logger << error::error_factory( error::info::InitErrorInfo(
          "output redirection"sv, "open file failed"sv, format( "'{}' cannot be written", filename )
        ) );
        return !val_decl::EvalSuccess;
      }

      auto [match_result, matches] = utils::match_string( optr_, "^([0-9]*)>{1,2}$"sv );
      assert( match_result == true );

      const auto& fd_str = matches[1].str();
      const type_decl::FDType file_d = fd_str.empty() ? STDOUT_FILENO : stoi( fd_str );

      int status;
      if ( pid_t process_id = fork();
           process_id < 0 )
        throw error::error_factory( error::info::InitErrorInfo(
          "output redirection"sv, "failed to fork"sv, {}
        ) );
      else if ( process_id == 0 ) {
        auto fd = open( filename.c_str(),
          O_WRONLY | (category_ == StmtKind::appnd_redrct ? O_APPEND : O_TRUNC) );
        dup2( fd, file_d );

        if ( l_child_ != nullptr ) // 允许语句 `./prog < txt.txt > output.txt` 存在
          l_child_->evaluate();

        close( fd );
        throw error::ProcessSuicide( EXIT_FAILURE );
      } else if ( waitpid( process_id, &status, 0 ) < 0 )
        throw error::error_factory( error::info::InitErrorInfo(
          "output redirection"sv, "failed to waitpid"sv, {}
        ) );

      return WEXITSTATUS( status ) == val_decl::ExecSuccess;
    }

    case StmtKind::stdin_redrct: {
      assert( l_child_ != nullptr && r_child_ != nullptr );
      assert( r_child_->category_ == StmtKind::trivial );

      auto& right_expr = static_cast<ExprNode&>(*r_child_.get());
      if ( const size_t expr_size = right_expr.expression().size();
           expr_size > 1 || expr_size == 0 ) {
        utils::logger << error::error_factory( error::info::ArgumentErrorInfo(
          "input redirection"sv, "argument number error"sv
        ) );
        return !val_decl::EvalSuccess;
      }

      const auto& filename = right_expr.expression().front();
      if ( access( filename.c_str(), F_OK ) < 0 ) {
        utils::logger << error::error_factory( error::info::InitErrorInfo(
          "input redirection"sv, "open file failed"sv, format( "'{}' does not exist", filename )
        ) );
        return !val_decl::EvalSuccess;
      }
      else if ( access( filename.c_str(), R_OK ) < 0 ) {
        utils::logger << error::error_factory( error::info::InitErrorInfo(
          "input redirection"sv, "open file failed"sv, format( "file '{}' cannot be read", filename )
        ) );
        return !val_decl::EvalSuccess;
      }

      int status;
      if ( pid_t process_id = fork();
           process_id < 0 )
        throw error::error_factory( error::info::InitErrorInfo(
          "input redirection"sv, "failed to fork"sv, {}
        ) );
      else if ( process_id == 0 ) {
        auto fd = open( filename.c_str(), O_RDONLY );
        dup2( fd, STDIN_FILENO );

        l_child_->evaluate();

        close( fd );
        throw error::ProcessSuicide( EXIT_FAILURE );
      } else if ( waitpid( process_id, &status, 0 ) < 0 )
        throw error::error_factory( error::info::InitErrorInfo(
          "input redirection"sv, "failed to waitpid"sv, {}
        ) );

      return WEXITSTATUS( status ) == val_decl::ExecSuccess;
    }

    case StmtKind::trivial:
      [[fallthrough]];
    default:
      assert( false );
      break;
    }

    return !val_decl::EvalSuccess;
  }

  type_decl::EvalT ExprNode::external_exec( const type_decl::TokenT& expression )
  {
    utils::Pipe pipe;
    utils::close_blocking( pipe.reader().get() );

    if ( const pid_t process_id = fork(); process_id < 0 ) {
      throw error::error_factory( error::info::InitErrorInfo(
        "excute"sv, "failed to fork"sv, {}
      ) );
    } else if ( process_id == 0 ) {
      // child process
      pipe.reader().close();

      vector<char*> exec_argv;
      exec_argv.reserve( expression.size() + 1 );
      ranges::transform( expression, back_inserter( exec_argv ),
        []( const type_decl::StringT& str )-> char* {
          return const_cast<char*>(str.data());
      } );
      exec_argv.push_back( nullptr );

      execvp( exec_argv.front(), exec_argv.data() );

      pipe.writer().push( true );
      throw error::ProcessSuicide( EXIT_FAILURE ); // 保证各作用域对象析构正常进行
    } else {
      int status;
      if ( waitpid( process_id, &status, 0 ) < 0 )
        throw error::error_factory( error::info::InitErrorInfo(
          "excute"sv, "failed to waitpid"sv, {}
        ) );

      if ( pipe.reader().pop<bool>() )
        // output error, don't throw it.
        utils::logger << error::error_factory( error::info::InitErrorInfo(
          expression.front(), "command error or not found"sv, {}
        ) );

      return WEXITSTATUS( status ) == val_decl::ExecSuccess;
    }
  }

  type_decl::EvalT ExprNode::internal_exec( const type_decl::TokenT& expression )
  {
    switch ( expression.front().front() ) {
    case 'c': { // cd
      if ( expression.size() > 2 || expression.size() < 1 ) {
        utils::logger << error::error_factory( error::info::ArgumentErrorInfo(
          "cd"sv, "the number of arguments error"sv
        ) );
        return !val_decl::EvalSuccess;
      }
      return chdir( expression[1].c_str() );
    } break;
    case 'e': { // exit
      if ( expression.size() > 1 ) {
        utils::logger << error::error_factory( error::info::ArgumentErrorInfo(
          "exit"sv, "the number of arguments error"sv
        ) );
        return !val_decl::EvalSuccess;
      }
      throw error::ProcessSuicide( EXIT_SUCCESS );
    }
    default:
      assert( false );
      break;
    }

    return val_decl::EvalSuccess;
  }

  type_decl::EvalT ExprNode::evaluate()
  {
    if ( !result_.has_value() ) {
      if ( val_decl::internal_command.contains( expr_.front() ) )
        result_ = internal_exec( expr_ );
      else result_ = external_exec( expr_ );
    }
    return result_.value();
  }
}
