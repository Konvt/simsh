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
      assert( l_child_ != nullptr );
      assert( tokens_.size() == 1 );

      if ( r_child_ == nullptr )
        return l_child_->evaluate();
      else {
        l_child_->evaluate();
        return r_child_->evaluate();
      }
    }

    case StmtKind::logical_and: {
      assert( l_child_ != nullptr && r_child_ != nullptr );
      assert( tokens_.size() == 1 );

      return l_child_->evaluate() && r_child_->evaluate();
    }

    case StmtKind::logical_or: {
      assert( l_child_ != nullptr && r_child_ != nullptr );
      assert( tokens_.size() == 1 );

      return l_child_->evaluate() || r_child_->evaluate();
    }

    case StmtKind::logical_not: {
      assert( l_child_ != nullptr && r_child_ == nullptr );
      assert( tokens_.size() == 1 );

      return !l_child_->evaluate();
    }

    case StmtKind::pipeline: {
      assert( l_child_ != nullptr && r_child_ != nullptr );
      assert( tokens_.size() == 1 );

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
          throw error::TerminationSignal( EXIT_FAILURE );
        } else if ( waitpid( process_id, nullptr, 0 ) < 0 )
          throw error::error_factory( error::info::InitErrorInfo(
            "pipeline", "failed to waitpid"sv, {}
          ) );
      }

      return val_decl::EvalSuccess;
    }

    case StmtKind::appnd_redrct:
      [[fallthrough]];
    case StmtKind::ovrwrit_redrct:
      [[fallthrough]];
    case StmtKind::merge_output:
      [[fallthrough]];
    case StmtKind::merge_appnd: {
      assert( r_child_ != nullptr );
      assert( r_child_->category_ == StmtKind::trivial );
      assert( tokens_.size() == 1 );

      if ( r_child_->tokens().size() != 1 ) {
        iout::logger << error::error_factory( error::info::ArgumentErrorInfo(
          "output redirection"sv, "argument number error"sv
        ) );
        return !val_decl::EvalSuccess;
      }

      const auto& filename = r_child_->tokens().front();
      if ( access( filename.c_str(), F_OK ) < 0 && !utils::create_file( filename ) ) { // 判断能否获取文件描述符
        iout::logger << error::error_factory( error::info::InitErrorInfo(
          "output redirection"sv, "open file failed"sv, format( "cannot create '{}'", filename )
        ) );
        return !val_decl::EvalSuccess;
      } else if ( access( filename.c_str(), W_OK ) < 0 ) {
        iout::logger << error::error_factory( error::info::InitErrorInfo(
          "output redirection"sv, "open file failed"sv, format( "'{}' cannot be written", filename )
        ) );
        return !val_decl::EvalSuccess;
      }

      type_decl::FDType file_d;
      auto [match_result, matches] = utils::match_string( tokens_.front(),
        (category_ == StmtKind::appnd_redrct || category_ == StmtKind::ovrwrit_redrct ?
          R"(^(\d*)>{1,2}$)"sv : R"(^(\d*)&>{1,2}$)"sv) );

      assert( match_result == true );

      const auto& fd_str = matches[1].str();
      file_d = fd_str.empty() ? STDOUT_FILENO : stoi( fd_str );

      int status;
      if ( pid_t process_id = fork();
           process_id < 0 )
        throw error::error_factory( error::info::InitErrorInfo(
          "output redirection"sv, "failed to fork"sv, {}
        ) );
      else if ( process_id == 0 ) {
        auto target_fd = open( filename.c_str(),
          O_WRONLY | (category_ == StmtKind::appnd_redrct ||category_ == StmtKind::merge_appnd ? O_APPEND : O_TRUNC) );

        dup2( target_fd, file_d );
        if ( category_ == StmtKind::merge_output || category_ == StmtKind::merge_appnd ) {
          if ( file_d != STDOUT_FILENO )
            dup2( target_fd, STDOUT_FILENO );
          if ( file_d != STDERR_FILENO )
            dup2( target_fd, STDERR_FILENO );
        }

        if ( l_child_ != nullptr ) // 允许语句 `./prog < txt.txt > output.txt` 存在
          l_child_->evaluate();

        close( target_fd );
        throw error::TerminationSignal( EXIT_FAILURE );
      } else if ( waitpid( process_id, &status, 0 ) < 0 )
        throw error::error_factory( error::info::InitErrorInfo(
          "output redirection"sv, "failed to waitpid"sv, {}
        ) );

      return WEXITSTATUS( status ) == val_decl::ExecSuccess;
    }

    case StmtKind::stdin_redrct: {
      assert( l_child_ != nullptr && r_child_ != nullptr );
      assert( r_child_->category_ == StmtKind::trivial );
      assert( tokens_.size() == 1 );

      if ( r_child_->tokens().size() != 1 ) {
        iout::logger << error::error_factory( error::info::ArgumentErrorInfo(
          "input redirection"sv, "argument number error"sv
        ) );
        return !val_decl::EvalSuccess;
      }

      const auto& filename = r_child_->tokens().front();
      if ( access( filename.c_str(), F_OK ) < 0 ) {
        iout::logger << error::error_factory( error::info::InitErrorInfo(
          "input redirection"sv, "open file failed"sv, format( "'{}' does not exist", filename )
        ) );
        return !val_decl::EvalSuccess;
      }
      else if ( access( filename.c_str(), R_OK ) < 0 ) {
        iout::logger << error::error_factory( error::info::InitErrorInfo(
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
        auto target_fd = open( filename.c_str(), O_RDONLY );
        dup2( target_fd, STDIN_FILENO );

        l_child_->evaluate();

        close( target_fd );
        throw error::TerminationSignal( EXIT_FAILURE );
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

  type_decl::EvalT ExprNode::external_exec() const
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
      exec_argv.reserve( tokens_.size() + 1 );
      ranges::transform( tokens_, back_inserter( exec_argv ),
        []( const type_decl::StringT& tkn_str ) -> char* {
          return const_cast<char*>(tkn_str.data());
        }
      );
      exec_argv.push_back( nullptr );

      execvp( exec_argv.front(), exec_argv.data() );

      pipe.writer().push( true );
      throw error::TerminationSignal( EXIT_FAILURE ); // 保证各作用域对象析构正常进行
    } else {
      int status;
      if ( waitpid( process_id, &status, 0 ) < 0 )
        throw error::error_factory( error::info::InitErrorInfo(
          "excute"sv, "failed to waitpid"sv, {}
        ) );

      if ( pipe.reader().pop<bool>() )
        // output error, don't throw it.
        iout::logger << error::error_factory( error::info::InitErrorInfo(
          tokens_.front(), "command error or not found"sv, {}
        ) );

      return WEXITSTATUS( status ) == val_decl::ExecSuccess;
    }
  }

  type_decl::EvalT ExprNode::internal_exec() const
  {
    switch ( tokens_.front().front() ) {
    case 'c': { // cd
      if ( tokens_.size() != 2 ) {
        iout::logger << error::error_factory( error::info::ArgumentErrorInfo(
          "cd"sv, "the number of arguments error"sv
        ) );
        return !val_decl::EvalSuccess;
      }
      return chdir( tokens_[1].c_str() );
    } break;
    case 'e': { // exit
      if ( tokens_.size() != 1 ) {
        iout::logger << error::error_factory( error::info::ArgumentErrorInfo(
          "exit"sv, "the number of arguments error"sv
        ) );
        return !val_decl::EvalSuccess;
      }
      throw error::TerminationSignal( EXIT_SUCCESS );
    }
    default:
      assert( false );
      break;
    }

    return val_decl::EvalSuccess;
  }

  type_decl::EvalT ExprNode::evaluate()
  {
    assert( l_child_ == nullptr && r_child_ == nullptr );
    assert( category_ == StmtKind::trivial );

    if ( !result_.has_value() ) {
      if ( val_decl::internal_command.contains( tokens_.front() ) )
        result_ = internal_exec();
      else result_ = external_exec();
    }
    return result_.value();
  }
}
