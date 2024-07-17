#include <ranges>
#include <algorithm>
#include <filesystem>
#include <format>
#include <cstdlib>
#include <cassert>

#include <unistd.h>
#include <fcntl.h>

#include "TreeNode.hpp"
#include "Constants.hpp"
#include "Exception.hpp"
#include "Logger.hpp"
#include "Pipe.hpp"
#include "ForkGuard.hpp"
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
        utils::ForkGuard pguard;
        if ( pguard.is_child() ) {
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
        }
        pguard.wait();
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
      if ( !filesystem::exists( filename ) && !utils::create_file( filename ) ) {
        iout::logger.print( error::SystemCallError( filename ) );
        return !constants::EvalSuccess;
      } else if ( const auto perms = filesystem::status( filename ).permissions();
                  (perms & filesystem::perms::owner_write) == filesystem::perms::none ) { // not writable
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

      utils::ForkGuard pguard;
      if ( pguard.is_child() ) {
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
      }
      pguard.wait();

      if ( l_child_ != nullptr && l_child_->type() == StmtKind::trivial )
        return pguard.exit_code() == constants::ExecSuccess;
      else return pguard.exit_code();
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

      utils::ForkGuard pguard;
      if ( pguard.is_child() ) {
        dup2( r_fd, l_fd );

        l_child_->evaluate();
        throw error::TerminationSignal( EXIT_FAILURE );
      }
      pguard.wait();

      if ( l_child_->type() == StmtKind::trivial )
        return pguard.exit_code() == constants::ExecSuccess;
      else return pguard.exit_code();
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
      if ( !filesystem::exists( filename ) ) {
        iout::logger.print( error::SystemCallError( filename ) );
        return !constants::EvalSuccess;
      } else if ( const auto perms = filesystem::status( filename ).permissions();
                  (perms & filesystem::perms::owner_read) == filesystem::perms::none ) { // not readable
        iout::logger.print( error::SystemCallError( filename ) );
        return !constants::EvalSuccess;
      }

      utils::ForkGuard pguard;
      if ( pguard.is_child() ) {
        auto target_fd = open( filename.c_str(), O_RDONLY );
        dup2( target_fd, STDIN_FILENO );

        l_child_->evaluate();

        close( target_fd );
        throw error::TerminationSignal( EXIT_FAILURE );
      }
      pguard.wait();

      return pguard.exit_code() == constants::ExecSuccess;
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

    utils::ForkGuard pguard;
    if ( pguard.is_child() ) {
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

      pguard.reset_signals();
      execvp( exec_argv.front(), exec_argv.data() );

      pipe.writer().push( true );
      // Ensure that all scoped objects are destructed normally.
      // In particular the object `utils::Pipe` above.
      throw error::TerminationSignal( EXIT_FAILURE );
    } else {
      pguard.wait();

      if ( pipe.reader().pop<bool>() )
        // output error, don't throw it.
        iout::logger << error::ArgumentError(
          token_, "command error or not found"sv
        );

      return pguard.exit_code() == constants::ExecSuccess;
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

      try {
        filesystem::current_path( target_dir );
      } catch ( const filesystem::filesystem_error& e ) {
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
      iout::prmptr << utils::help_doc();
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
