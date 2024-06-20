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
#include "HelpDocument.hpp"
using namespace std;

namespace simsh {
  type_decl::EvalT StmtNode::evaluate()
  {
    switch ( category_ ) {
    case StmtKind::sequential: {
      assert( l_child_ != nullptr );
      assert( siblings_.empty() == true );

      if ( r_child_ == nullptr )
        return l_child_->evaluate();
      else {
        l_child_->evaluate();
        return r_child_->evaluate();
      }
    }

    case StmtKind::logical_and: {
      assert( l_child_ != nullptr && r_child_ != nullptr );
      assert( siblings_.empty() == true );

      return l_child_->evaluate() && r_child_->evaluate();
    }

    case StmtKind::logical_or: {
      assert( l_child_ != nullptr && r_child_ != nullptr );
      assert( siblings_.empty() == true );

      return l_child_->evaluate() || r_child_->evaluate();
    }

    case StmtKind::logical_not: {
      assert( l_child_ != nullptr && r_child_ == nullptr );
      assert( siblings_.empty() == true );

      return !l_child_->evaluate();
    }

    case StmtKind::pipeline: {
      assert( l_child_ != nullptr && r_child_ != nullptr );
      assert( siblings_.empty() == true );

      utils::Pipe pipe;
      utils::close_blocking( pipe.reader().get() );

      for ( size_t i = 0; i < 2; ++i ) {
        if ( pid_t process_id = fork(); process_id < 0 )
          // At runtime, only critical system call errors cause exceptions to be thrown.
          throw error::InitError(
            "pipeline"sv, "failed to fork"sv 
          );
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
          throw error::InitError(
            "pipeline", "failed to waitpid"sv
          );
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
      assert( r_child_ == nullptr );
      assert( siblings_.empty() == false );
      assert( siblings_.front()->type() == StmtKind::trivial );

      if ( siblings_.size() != 1 ) {
        iout::logger << error::ArgumentError(
          "output redirection"sv, "argument number error"sv
        );
        return !val_decl::EvalSuccess;
      }

      const auto& filename = siblings_.front()->token();
      // Check whether the file descriptor can be obtained.
      if ( access( filename.c_str(), F_OK ) < 0 && !utils::create_file( filename ) ) {
        iout::logger << error::InitError(
          "output redirection"sv, format( "cannot create '{}'", filename )
        );
        return !val_decl::EvalSuccess;
      } else if ( access( filename.c_str(), W_OK ) < 0 ) {
        iout::logger << error::InitError(
          "output redirection"sv, format( "'{}' cannot be written", filename )
        );
        return !val_decl::EvalSuccess;
      }

      type_decl::FDType file_d = STDOUT_FILENO;

      if ( category_ == StmtKind::appnd_redrct || category_ == StmtKind::ovrwrit_redrct ) {
        auto [match_result, matches] = utils::match_string( token_,
          R"(^(\d*)>{1,2}$)"sv );

        assert( match_result == true );

        const auto& fd_str = matches[1].str();
        file_d = fd_str.empty() ? STDOUT_FILENO : stoi( fd_str );
      }

      int status;
      if ( pid_t process_id = fork();
           process_id < 0 )
        throw error::InitError(
          "output redirection"sv, "failed to fork"sv
        );
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

        if ( l_child_ != nullptr )
          l_child_->evaluate();

        close( target_fd );
        throw error::TerminationSignal( EXIT_FAILURE );
      } else if ( waitpid( process_id, &status, 0 ) < 0 )
        throw error::InitError(
          "output redirection"sv, "failed to waitpid"sv
        );

      return WEXITSTATUS( status ) == val_decl::ExecSuccess;
    }

    case StmtKind::merge_stream: {
      assert( l_child_ != nullptr && r_child_ == nullptr );
      assert( siblings_.empty() == true );

      type_decl::FDType l_fd = STDERR_FILENO, r_fd = STDOUT_FILENO;
      auto [match_result, matches] = utils::match_string( token_,
        R"(^(\d*)>&(\d*)$)"sv );

      assert( match_result == true );

      for ( size_t i = 1; i <= 2; ++i ) {
        const auto fd_str = matches[i].str();
        if ( !fd_str.empty() )
          (i == 1 ? l_fd : r_fd) = stoi( fd_str );
      }

      int status;
      if ( pid_t process_id = fork();
           process_id < 0 )
        throw error::InitError(
          "combined redirection"sv, "failed to fork"sv
        );
      else if ( process_id == 0 ) {
        dup2( r_fd, l_fd );

        l_child_->evaluate();
        throw error::TerminationSignal( EXIT_FAILURE );
      } else if ( waitpid( process_id, &status, 0 ) < 0 )
        throw error::InitError(
          "combined redirection"sv, "failed to waitpid"sv
        );

      return WEXITSTATUS( status ) == val_decl::ExecSuccess;
    }

    case StmtKind::stdin_redrct: {
      assert( l_child_ != nullptr && r_child_ == nullptr );
      assert( siblings_.empty() == false );
      assert( siblings_.front()->type() == StmtKind::trivial );

      if ( siblings_.size() != 1 ) {
        iout::logger << error::ArgumentError(
          "input redirection"sv, "argument number error"sv
        );
        return !val_decl::EvalSuccess;
      }

      const auto& filename = siblings_.front()->token();
      if ( access( filename.c_str(), F_OK ) < 0 ) {
        iout::logger << error::InitError(
          "input redirection"sv, format( "'{}' does not exist", filename )
        );
        return !val_decl::EvalSuccess;
      }
      else if ( access( filename.c_str(), R_OK ) < 0 ) {
        iout::logger << error::InitError(
          "input redirection"sv, format( "file '{}' cannot be read", filename )
        );
        return !val_decl::EvalSuccess;
      }

      int status;
      if ( pid_t process_id = fork();
           process_id < 0 )
        throw error::InitError(
          "input redirection"sv, "failed to fork"sv
        );
      else if ( process_id == 0 ) {
        auto target_fd = open( filename.c_str(), O_RDONLY );
        dup2( target_fd, STDIN_FILENO );

        l_child_->evaluate();

        close( target_fd );
        throw error::TerminationSignal( EXIT_FAILURE );
      } else if ( waitpid( process_id, &status, 0 ) < 0 )
        throw error::InitError(
          "input redirection"sv, "failed to waitpid"sv
        );

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
      throw error::InitError(
        "excute"sv, "failed to fork"sv
      );
    } else if ( process_id == 0 ) {
      // child process
      pipe.reader().close();

      vector<char*> exec_argv { const_cast<char*>(token_.data()) };
      exec_argv.reserve( siblings_.size() + 2 );
      ranges::transform( siblings_, back_inserter( exec_argv ),
        []( const ChildNode& sblng ) -> char* {
          return const_cast<char*>(sblng->token().data());
        }
      );
      exec_argv.push_back( nullptr );

      execvp( exec_argv.front(), exec_argv.data() );

      pipe.writer().push( true );
      // Ensure that all scoped objects are destructed normally.
      // In particular the object `utils::Pipe` above.
      throw error::TerminationSignal( EXIT_FAILURE );
    } else {
      int status;
      if ( waitpid( process_id, &status, 0 ) < 0 )
        throw error::InitError(
          "excute"sv, "failed to waitpid"sv
        );

      if ( pipe.reader().pop<bool>() )
        // output error, don't throw it.
        iout::logger << error::InitError(
          token_, "command error or not found"sv
        );

      return WEXITSTATUS( status ) == val_decl::ExecSuccess;
    }
  }

  type_decl::EvalT ExprNode::internal_exec() const
  {
    switch ( token_.front() ) {
    case 'c': { // cd
      assert( siblings_.size() != 0 );
      if ( siblings_.size() > 1 ) {
        iout::logger << error::ArgumentError(
          "cd"sv, "the number of arguments error"sv
        );
        return !val_decl::EvalSuccess;
      }
      return chdir( siblings_.front()->token().c_str() );
    } break;
    case 'e': { // exit
      if ( !siblings_.empty() ) {
        iout::logger << error::ArgumentError(
          "exit"sv, "the number of arguments error"sv
        );
        return !val_decl::EvalSuccess;
      }
      throw error::TerminationSignal( EXIT_SUCCESS );
    }
    case 'h': { // help
      if ( !siblings_.empty() ) {
        iout::logger << error::ArgumentError(
          "help"sv, "the number of arguments error"sv
        );
        return !val_decl::EvalSuccess;
      }
      iout::prmptr << utils::help_doc;
    } break;
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

    token_ = token_ == "$$" ? format( "{}", getpid() ) : token_;
    for ( auto& sblng : siblings_ ) {
      assert( sblng->type() == StmtKind::trivial );

      if ( ExprNode& node = static_cast<ExprNode&>(*sblng.get());
           node.type_ == ExprKind::command ) {
        if ( node.token_ == "$$" )
          node.token_ = format( "{}", getpid() );
        else utils::tilde_expansion( node.token_ );
      }
    }

    if ( !result_.has_value() ) {
      if ( val_decl::internal_command.contains( token_ ) )
        result_ = internal_exec();
      else result_ = external_exec();
    }
    return result_.value();
  }
}
