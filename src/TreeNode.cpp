#include <ranges>
#include <algorithm>
#include <format>
#include <cstdlib>
#include <cassert>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include "TreeNode.hpp"
#include "Constants.hpp"
#include "Exception.hpp"
#include "Logger.hpp"
#include "Pipe.hpp"
#include "Utils.hpp"
#include "HelpDocument.hpp"
using namespace std;

namespace simsh {
  types::EvalT StmtNode::evaluate()
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
          throw error::SystemCallError( "fork" );
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
          throw error::SystemCallError( "waitpid" );
      }

      return constants::EvalSuccess;
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

      const auto& filename =
        category_ == StmtKind::appnd_redrct || category_ == StmtKind::ovrwrit_redrct
        ? static_cast<ExprNode*>(siblings_[1].get())->token()
        : static_cast<ExprNode*>(siblings_.front().get())->token();
      // Check whether the file descriptor can be obtained.
      if ( access( filename.c_str(), F_OK ) < 0 && !utils::create_file( filename ) ) {
        iout::logger.print( error::SystemCallError( filename ) );
        return !constants::EvalSuccess;
      } else if ( access( filename.c_str(), W_OK ) < 0 ) {
        iout::logger.print( error::SystemCallError( filename ) );
        return !constants::EvalSuccess;
      }

      /* For `StmtKind::appnd_redrct` and `StmtKind::ovrwrit_redrct` node,
       * the first element of `siblings_` is `ExprNode` of type `ExprKind::value`,
       * which specifies the destination file descriptor. */
      types::FDType file_d = STDOUT_FILENO;
      if ( category_ == StmtKind::appnd_redrct || category_ == StmtKind::ovrwrit_redrct ) {
        assert( static_cast<ExprNode*>(siblings_.front().get())->kind() == ExprKind::value );
        file_d = siblings_.front()->evaluate() == constants::InvalidValue ? STDOUT_FILENO : siblings_.front()->evaluate();
      }

      int status;
      if ( pid_t process_id = fork();
           process_id < 0 )
        throw error::SystemCallError( "fork" );
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

        if ( l_child_ != nullptr ) {
          l_child_->evaluate();
          close( target_fd );
          throw error::TerminationSignal( EXIT_FAILURE );
        } else {
          close( target_fd );
          throw error::TerminationSignal( EXIT_SUCCESS );
        }
      } else if ( waitpid( process_id, &status, 0 ) < 0 )
        throw error::SystemCallError( "waitpid" );

      if ( l_child_ != nullptr && l_child_->type() == StmtKind::trivial )
        return WEXITSTATUS( status ) == constants::ExecSuccess;
      else return WEXITSTATUS( status );
    }

    case StmtKind::merge_stream: {
      assert( l_child_ != nullptr && r_child_ == nullptr );
      assert( siblings_.size() == 2 );
      assert( siblings_[0]->type() == StmtKind::trivial && siblings_[1]->type() == StmtKind::trivial );

      /* For `StmtKind::merge_stream` node,
       * the first and second elements of `siblings_` is `ExprNode` of type `ExprKind::value`,
       * which specifies the destination file descriptor. */
      assert( static_cast<ExprNode*>(siblings_[0].get())->kind() == ExprKind::value );
      assert( static_cast<ExprNode*>(siblings_[1].get())->kind() == ExprKind::value );
      const auto l_fd = siblings_[0]->evaluate() == constants::InvalidValue ? STDERR_FILENO : siblings_[0]->evaluate();
      const auto r_fd = siblings_[1]->evaluate() == constants::InvalidValue ? STDOUT_FILENO : siblings_[1]->evaluate();

      int status;
      if ( pid_t process_id = fork();
           process_id < 0 )
        throw error::SystemCallError( "fork" );
      else if ( process_id == 0 ) {
        dup2( r_fd, l_fd );

        l_child_->evaluate();
        throw error::TerminationSignal( EXIT_FAILURE );
      } else if ( waitpid( process_id, &status, 0 ) < 0 )
        throw error::SystemCallError( "waitpid" );

      if ( l_child_->type() == StmtKind::trivial )
        return WEXITSTATUS( status ) == constants::ExecSuccess;
      else return WEXITSTATUS( status );
    }

    case StmtKind::stdin_redrct: {
      assert( l_child_ != nullptr && r_child_ == nullptr );
      assert( siblings_.empty() == false );
      assert( siblings_.front()->type() == StmtKind::trivial );

      if ( siblings_.size() != 1 ) {
        iout::logger << error::ArgumentError(
          "input redirection"sv, "argument number error"sv
        );
        return !constants::EvalSuccess;
      }

      const auto& filename = static_cast<ExprNode*>(siblings_.front().get())->token();
      if ( access( filename.c_str(), F_OK ) < 0 ) {
        iout::logger.print( error::SystemCallError( filename ) );
        return !constants::EvalSuccess;
      }
      else if ( access( filename.c_str(), R_OK ) < 0 ) {
        iout::logger.print( error::SystemCallError( filename ) );
        return !constants::EvalSuccess;
      }

      int status;
      if ( pid_t process_id = fork();
           process_id < 0 )
        throw error::SystemCallError( "fork" );
      else if ( process_id == 0 ) {
        auto target_fd = open( filename.c_str(), O_RDONLY );
        dup2( target_fd, STDIN_FILENO );

        l_child_->evaluate();

        close( target_fd );
        throw error::TerminationSignal( EXIT_FAILURE );
      } else if ( waitpid( process_id, &status, 0 ) < 0 )
        throw error::SystemCallError( "waitpid" );

      return WEXITSTATUS( status ) == constants::ExecSuccess;
    }

    case StmtKind::trivial:
      [[fallthrough]];
    default:
      assert( false );
      break;
    }

    return !constants::EvalSuccess;
  }

  types::EvalT ExprNode::external_exec() const
  {
    utils::Pipe pipe;
    utils::close_blocking( pipe.reader().get() );

    sigset_t new_set, old_set;
    sigemptyset( &new_set );
    sigaddset( &new_set, SIGINT );
    // block the signal in parent process
    if ( sigprocmask( SIG_BLOCK, &new_set, &old_set ) < 0 )
      throw error::SystemCallError( "sigprocmask" );

    if ( const pid_t process_id = fork(); process_id < 0 )
      throw error::SystemCallError( "fork" );
    else if ( process_id == 0 ) {
      // child process
      pipe.reader().close();

      vector<char*> exec_argv { const_cast<char*>(token_.data()) };
      exec_argv.reserve( siblings_.size() + 2 );
      ranges::transform( siblings_, back_inserter( exec_argv ),
        []( const ChildNode& sblng ) -> char* {
          assert( sblng->type() == StmtKind::trivial );
          return const_cast<char*>(static_cast<ExprNode*>(sblng.get())->token().data());
        }
      );
      exec_argv.push_back( nullptr );

      // set the signal in child process to default
      sigemptyset( &new_set );
      sigprocmask( SIG_SETMASK, &new_set, nullptr );
      signal( SIGINT, SIG_DFL );

      execvp( exec_argv.front(), exec_argv.data() );

      pipe.writer().push( true );
      // Ensure that all scoped objects are destructed normally.
      // In particular the object `utils::Pipe` above.
      throw error::TerminationSignal( EXIT_FAILURE );
    } else {
      int status;
      if ( waitpid( process_id, &status, 0 ) < 0 )
        throw error::SystemCallError( "waitpid" );

      if ( pipe.reader().pop<bool>() )
        // output error, don't throw it.
        iout::logger << error::ArgumentError(
          token_, "command error or not found"sv
        );

      // resuming the signal processing in parent process 
      if ( sigprocmask( SIG_SETMASK, &old_set, nullptr ) < 0 ) {
        iout::logger << error::SystemCallError( "sigprocmask" );
        throw error::TerminationSignal( EXIT_FAILURE );
      }

      return WEXITSTATUS( status ) == constants::ExecSuccess;
    }
  }

  types::EvalT ExprNode::internal_exec() const
  {
    switch ( token_.front() ) {
    case 'c': { // cd
      auto exec_result = constants::EvalSuccess;

      if ( siblings_.size() > 1 ) {
        iout::logger << error::ArgumentError(
          "cd"sv, "the number of arguments error"sv
        );
        exec_result = !constants::EvalSuccess;
      }
      types::StrViewT target_dir =
        siblings_.size() == 0
        ? utils::get_homedir()
        : static_cast<ExprNode*>(siblings_.front().get())->token();

      if ( chdir( target_dir.data() ) < 0 ) {
        iout::logger.print( error::SystemCallError( format( "cd: {}", target_dir.data() ) ) );
        exec_result = !constants::EvalSuccess;
      }
      return exec_result;
    } break;
    case 'e': { // exit
      if ( !siblings_.empty() ) {
        iout::logger << error::ArgumentError(
          "exit"sv, "the number of arguments error"sv
        );
        return !constants::EvalSuccess;
      }
      throw error::TerminationSignal( EXIT_SUCCESS );
    }
    case 'h': { // help
      if ( !siblings_.empty() ) {
        iout::logger << error::ArgumentError(
          "help"sv, "the number of arguments error"sv
        );
        return !constants::EvalSuccess;
      }
      iout::prmptr << utils::help_doc;
    } break;
    default:
      assert( false );
      break;
    }

    return constants::EvalSuccess;
  }

  types::EvalT ExprNode::evaluate()
  {
    assert( l_child_ == nullptr && r_child_ == nullptr );
    assert( category_ == StmtKind::trivial );

    token_ = token_ == "$$" ? format( "{}", getpid() ) : token_;
    if ( type_ == ExprKind::command )
      utils::tilde_expansion( token_ );
    for ( auto& sblng : siblings_ ) {
      assert( sblng->type() == StmtKind::trivial );

      if ( ExprNode& node = static_cast<ExprNode&>(*sblng.get());
           node.token_ == "$$" )
        node.token_ = format( "{}", getpid() );
      else if ( node.type_ == ExprKind::command )
        utils::tilde_expansion( node.token_ );
    }

    if ( !result_.has_value() ) {
      if ( constants::built_in_cmds.contains( token_ ) )
        result_ = internal_exec();
      else result_ = external_exec();
    }
    return result_.value();
  }
}
