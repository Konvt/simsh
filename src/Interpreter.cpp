#include <HelpDocument.hpp>
#include <Interpreter.hpp>
#include <cassert>
#include <cstdlib>
#include <fcntl.h>
#include <filesystem>
#include <unistd.h>
#include <util/Constants.hpp>
#include <util/Enums.hpp>
#include <util/Exception.hpp>
#include <util/ForkGuard.hpp>
#include <util/Logger.hpp>
#include <util/Pipe.hpp>
#include <util/Utils.hpp>
using namespace std;

namespace simsh {
  const std::unordered_set<types::String> Interpreter::built_in_cmds = { "cd",
                                                                          "exit",
                                                                          "help",
                                                                          "type",
                                                                          "exec" };

  types::Eval Interpreter::sequential_stmt( StmtNodeT seq_stmt ) const
  {
    assert( seq_stmt != nullptr );
    assert( seq_stmt->left() != nullptr );
    assert( seq_stmt->siblings().empty() == true );

    if ( seq_stmt->right() == nullptr )
      return interpret( seq_stmt->left() );
    else {
      interpret( seq_stmt->left() );
      return interpret( seq_stmt->right() );
    }
  }

  types::Eval Interpreter::logical_and( StmtNodeT and_stmt ) const
  {
    assert( and_stmt != nullptr );
    assert( and_stmt->left() != nullptr && and_stmt->right() != nullptr );
    assert( and_stmt->siblings().empty() == true );

    return interpret( and_stmt->left() ) && interpret( and_stmt->right() );
  }

  types::Eval Interpreter::logical_or( StmtNodeT or_stmt ) const
  {
    assert( or_stmt != nullptr );
    assert( or_stmt->left() != nullptr && or_stmt->right() != nullptr );
    assert( or_stmt->siblings().empty() == true );

    return interpret( or_stmt->left() ) || interpret( or_stmt->right() );
  }

  types::Eval Interpreter::logical_not( StmtNodeT not_stmt ) const
  {
    assert( not_stmt != nullptr );

    assert( not_stmt->left() != nullptr && not_stmt->right() == nullptr );
    assert( not_stmt->siblings().empty() == true );

    return !interpret( not_stmt->left() );
  }

  types::Eval Interpreter::pipeline_stmt( StmtNodeT pipeline_stmt ) const
  {
    assert( pipeline_stmt != nullptr );
    assert( pipeline_stmt->left() != nullptr && pipeline_stmt->right() != nullptr );
    assert( pipeline_stmt->siblings().empty() == true );

    utils::Pipe pipe;
    utils::close_blocking( pipe.reader().get() );

    for ( size_t i = 0; i < 2; ++i ) {
      utils::ForkGuard pguard;
      if ( pguard.is_child() ) {
        // child process
        if ( i == 0 ) {
          pipe.reader().close();
          dup2( pipe.writer().get(), STDOUT_FILENO );
          interpret( pipeline_stmt->left() );
        } else {
          pipe.writer().close();
          dup2( pipe.reader().get(), STDIN_FILENO );
          interpret( pipeline_stmt->right() );
        }
        throw error::TerminationSignal( EXIT_FAILURE );
      }
      pguard.wait();
      pipe.writer().close();
    }

    return constants::EvalSuccess;
  }

  types::Eval Interpreter::output_redirection( StmtNodeT oup_redr ) const
  {
    assert( oup_redr != nullptr );

    assert( oup_redr->right() == nullptr );
    assert( oup_redr->siblings().empty() == false );
    assert( oup_redr->siblings().front()->type() == types::StmtKind::trivial );

    assert( static_cast<const ExprNode*>(
              oup_redr
                ->siblings()[oup_redr->type() == types::StmtKind::appnd_redrct
                                 || oup_redr->type() == types::StmtKind::ovrwrit_redrct
                               ? 1
                               : 0]
                .get() )
              ->kind()
            != types::ExprKind::value );

    const auto& filename =
      static_cast<const ExprNode*>(
        oup_redr
          ->siblings()[oup_redr->type() == types::StmtKind::appnd_redrct
                           || oup_redr->type() == types::StmtKind::ovrwrit_redrct
                         ? 1
                         : 0]
          .get() )
        ->token();

    // Check whether the file descriptor can be obtained.
    if ( !filesystem::exists( filename ) && !utils::create_file( filename ) ) {
      iout::logger.print( error::SystemCallError( filename ) );
      return !constants::EvalSuccess;
    } else if ( const auto perms = filesystem::status( filename ).permissions();
                ( perms & filesystem::perms::owner_write )
                == filesystem::perms::none ) { // not writable
      iout::logger.print( error::SystemCallError( filename ) );
      return !constants::EvalSuccess;
    }

    /* For `types::StmtKind::appnd_redrct` and `types::StmtKind::ovrwrit_redrct`
     * node, the first element of `merg_redr->siblings()` is `ExprNode` of type
     * `types::ExprKind::value`, which specifies the destination file
     * descriptor.
     */
    types::FileDesc file_d = STDOUT_FILENO;
    if ( oup_redr->type() == types::StmtKind::appnd_redrct
         || oup_redr->type() == types::StmtKind::ovrwrit_redrct ) {
      ExprNodeT arg_node = static_cast<ExprNode*>( oup_redr->siblings().front().get() );

      assert( arg_node->kind() == types::ExprKind::value );

      file_d = arg_node->value() == constants::InvalidValue ? STDOUT_FILENO : arg_node->value();
    }

    utils::ForkGuard pguard;
    if ( pguard.is_child() ) {
      auto target_fd = open( filename.c_str(),
                             O_WRONLY
                               | ( oup_redr->type() == types::StmtKind::appnd_redrct
                                       || oup_redr->type() == types::StmtKind::merge_appnd
                                     ? O_APPEND
                                     : O_TRUNC ) );

      dup2( target_fd, file_d );
      if ( oup_redr->type() == types::StmtKind::merge_output
           || oup_redr->type() == types::StmtKind::merge_appnd ) {
        if ( file_d != STDOUT_FILENO )
          dup2( target_fd, STDOUT_FILENO );
        if ( file_d != STDERR_FILENO )
          dup2( target_fd, STDERR_FILENO );
      }

      if ( oup_redr->left() != nullptr ) {
        interpret( oup_redr->left() );
        close( target_fd );
        throw error::TerminationSignal( EXIT_FAILURE );
      } else {
        close( target_fd );
        throw error::TerminationSignal( EXIT_SUCCESS );
      }
    }
    pguard.wait();

    assert( pguard.exit_code().has_value() );

    if ( oup_redr->left() != nullptr && oup_redr->left()->type() == types::StmtKind::trivial )
      return pguard.exit_code() == constants::ExecSuccess;
    else
      return pguard.exit_code().value();
  }

  types::Eval Interpreter::merge_stream( StmtNodeT merg_redr ) const
  {
    assert( merg_redr != nullptr );

    assert( merg_redr->left() != nullptr && merg_redr->right() == nullptr );
    assert( merg_redr->siblings().size() == 2 );
    assert( merg_redr->siblings()[0]->type() == types::StmtKind::trivial
            && merg_redr->siblings()[1]->type() == types::StmtKind::trivial );

    /* For `types::StmtKind::merge_stream` node,
     * the first and second elements of `merg_redr->siblings()` is `ExprNode` of
     * type `types::ExprKind::value`, which specifies the destination file descriptor. */
    ExprNodeT arg_node1 = static_cast<ExprNode*>( merg_redr->siblings()[0].get() );
    ExprNodeT arg_node2 = static_cast<ExprNode*>( merg_redr->siblings()[1].get() );

    assert( arg_node1->kind() == types::ExprKind::value );
    assert( arg_node2->kind() == types::ExprKind::value );

    const auto l_fd =
      arg_node1->value() == constants::InvalidValue ? STDERR_FILENO : arg_node1->value();
    const auto r_fd =
      arg_node2->value() == constants::InvalidValue ? STDOUT_FILENO : arg_node2->value();

    utils::ForkGuard pguard;
    if ( pguard.is_child() ) {
      dup2( r_fd, l_fd );

      interpret( merg_redr->left() );
      throw error::TerminationSignal( EXIT_FAILURE );
    }
    pguard.wait();

    assert( pguard.exit_code().has_value() );

    if ( merg_redr->left()->type() == types::StmtKind::trivial )
      return pguard.exit_code() == constants::ExecSuccess;
    else
      return pguard.exit_code().value();
  }

  types::Eval Interpreter::input_redirection( StmtNodeT inp_redr ) const
  {
    assert( inp_redr != nullptr );
    assert( inp_redr->left() != nullptr && inp_redr->right() == nullptr );
    assert( inp_redr->siblings().empty() == false );
    assert( inp_redr->siblings().front()->type() == types::StmtKind::trivial );

    if ( inp_redr->siblings().size() != 1 ) {
      iout::logger << error::ArgumentError( "input redirection"sv, "argument number error"sv );
      return !constants::EvalSuccess;
    }

    const auto& filename =
      static_cast<const ExprNode*>( inp_redr->siblings().front().get() )->token();
    if ( !filesystem::exists( filename ) ) {
      iout::logger.print( error::SystemCallError( filename ) );
      return !constants::EvalSuccess;
    } else if ( const auto perms = filesystem::status( filename ).permissions();
                ( perms & filesystem::perms::owner_read )
                == filesystem::perms::none ) { // not readable
      iout::logger.print( error::SystemCallError( filename ) );
      return !constants::EvalSuccess;
    }

    utils::ForkGuard pguard;
    if ( pguard.is_child() ) {
      auto target_fd = open( filename.c_str(), O_RDONLY );
      dup2( target_fd, STDIN_FILENO );

      interpret( inp_redr->left() );

      close( target_fd );
      throw error::TerminationSignal( EXIT_FAILURE );
    }
    pguard.wait();

    return pguard.exit_code() == constants::ExecSuccess;
  }

  types::Eval Interpreter::trivial( ExprNodeT expr ) const
  {
    assert( expr != nullptr );

    assert( expr->left() == nullptr && expr->right() == nullptr );
    assert( expr->type() == types::StmtKind::trivial );

    const auto cmd_expand = []( ExprNodeT node ) -> void {
      if ( node->kind() == types::ExprKind::value )
        return;
      else if ( node->token() == "$$"sv )
        node->replace_with( format( "{}", getpid() ) );
      else if ( node->token() == "$SIMSH_VERSION"sv )
        node->replace_with( SIMSH_VERSION );
      else if ( node->kind() == types::ExprKind::command ) {
        if ( auto new_token = utils::tilde_expansion( node->token() ); !new_token.empty() )
          node->replace_with( move( new_token ) );
      }
    };

    cmd_expand( expr );
    for ( auto& sblng : expr->siblings() ) {
      assert( sblng->type() == types::StmtKind::trivial );

      const auto node = static_cast<ExprNode*>( sblng.get() );
      assert( node->kind() != types::ExprKind::value );
      cmd_expand( node );
    }

    if ( expr->kind() == types::ExprKind::value )
      return expr->value();
    else if ( built_in_cmds.contains( expr->token() ) )
      return builtin_exec( expr );
    else
      return external_exec( expr );
  }

  types::Eval Interpreter::builtin_exec( ExprNodeT expr ) const
  {
    assert( expr != nullptr );
    assert( expr->kind() != types::ExprKind::value );

    switch ( expr->token().front() ) {
    case 'c': { // cd
      if ( expr->siblings().size() > 1 ) {
        iout::logger << error::ArgumentError( "cd"sv, "the number of arguments error"sv );
        return !constants::EvalSuccess;
      }

      assert( static_cast<const ExprNode*>( expr->siblings().front().get() )->kind()
              != types::ExprKind::value );

      types::StrView target_dir =
        expr->siblings().size() == 0
          ? utils::get_homedir()
          : static_cast<const ExprNode*>( expr->siblings().front().get() )->token();

      try {
        filesystem::current_path( target_dir );
      } catch ( const filesystem::filesystem_error& e ) {
        iout::logger.print( error::SystemCallError( format( "cd: {}", target_dir.data() ) ) );
        return !constants::EvalSuccess;
      }
      return constants::EvalSuccess;
    } break;

    case 'e': { // exit or exec
      if ( expr->token() == "exit" ) {
        if ( !expr->siblings().empty() ) {
          iout::logger << error::ArgumentError( "exit"sv, "the number of arguments error"sv );
          return !constants::EvalSuccess;
        }
        throw error::TerminationSignal( EXIT_SUCCESS );
      } else if ( !expr->siblings().empty() ) {
        /* Using `exec` with empty arguments does nothing in bash.
         * so there is not `else` branch to handle that case */
        vector<char*> exec_argv;
        exec_argv.reserve( expr->siblings().size() + 2 );
        ranges::transform( expr->siblings(),
                           back_inserter( exec_argv ),
                           []( const auto& sblng ) -> char* {
                             assert( sblng->type() == types::StmtKind::trivial );
                             ExprNodeT arg_node = static_cast<ExprNode*>( sblng.get() );
                             assert( arg_node->kind() != types::ExprKind::value );

                             return const_cast<char*>( arg_node->token().data() );
                           } );
        exec_argv.push_back( nullptr );

        execvp( exec_argv.front(), exec_argv.data() );

        const auto error_info = static_cast<ExprNode*>( expr->siblings().front().get() );
        iout::logger << error::ArgumentError(
          "exec",
          format( "{}: command not found", error_info->token() ) );
        return !constants::ExecSuccess;
      }
    } break;

    case 'h': { // help
      if ( !expr->siblings().empty() ) {
        iout::logger << error::ArgumentError( "help"sv, "the number of arguments error"sv );
        return !constants::EvalSuccess;
      }
      iout::prmptr << utils::help_doc();
    } break;

    case 't': { // type
      if ( expr->siblings().empty() )
        return !constants::EvalSuccess;

      for ( const auto& sblng : expr->siblings() ) {
        assert( sblng->type() == types::StmtKind::trivial );

        ExprNodeT arg_node = static_cast<ExprNode*>( sblng.get() );
        assert( arg_node->kind() != types::ExprKind::value );

        if ( built_in_cmds.contains( arg_node->token() ) )
          iout::prmptr << format( "{} is a builtin\n", arg_node->token() );
        else if ( const auto filepath =
                    utils::search_filepath( utils::get_envpath(), arg_node->token() );
                  filepath.empty() )
          iout::logger << error::ArgumentError(
            "type"sv,
            format( "could not find '{}'", arg_node->token() ) );
        else
          iout::prmptr << format( "{} is {}\n", arg_node->token(), filepath );
      }
    } break;
    default: assert( false ); break;
    }

    return constants::EvalSuccess;
  }

  types::Eval Interpreter::external_exec( ExprNodeT expr ) const
  {
    assert( expr != nullptr );

    utils::Pipe pipe;
    utils::close_blocking( pipe.reader().get() );

    utils::ForkGuard pguard;
    if ( pguard.is_child() ) {
      // child process
      vector<char*> exec_argv { const_cast<char*>( expr->token().data() ) };
      exec_argv.reserve( expr->siblings().size() + 2 );
      ranges::transform( expr->siblings(),
                         back_inserter( exec_argv ),
                         []( const auto& sblng ) -> char* {
                           assert( sblng->type() == types::StmtKind::trivial );
                           ExprNodeT arg_node = static_cast<ExprNode*>( sblng.get() );
                           assert( arg_node->kind() != types::ExprKind::value );

                           return const_cast<char*>( arg_node->token().data() );
                         } );
      exec_argv.push_back( nullptr );

      pguard.reset_signals();
      execvp( exec_argv.front(), exec_argv.data() );

      pipe.writer().push( true );
      // Ensure that all scoped objects are destructed normally.
      // In particular the object `utils::Pipe` above.
      throw error::TerminationSignal( constants::ExecFailureExit );
    } else {
      pguard.wait();
      if ( pipe.reader().pop<bool>() )
        iout::logger << error::ArgumentError( expr->token(), "command not found" );

      return pguard.exit_code() == constants::ExecSuccess;
    }
  }

  types::Eval Interpreter::interpret( StmtNodeT stmt_node ) const
  {
    if ( stmt_node == nullptr )
      throw error::ArgumentError( "interpreter", "syntax tree node is null" );

    switch ( stmt_node->type() ) {
    case types::StmtKind::sequential: {
      return sequential_stmt( stmt_node );
    }
    case types::StmtKind::logical_and: {
      return logical_and( stmt_node );
    }
    case types::StmtKind::logical_or: {
      return logical_or( stmt_node );
    }
    case types::StmtKind::logical_not: {
      return logical_not( stmt_node );
    }
    case types::StmtKind::pipeline: {
      return pipeline_stmt( stmt_node );
    }
    case types::StmtKind::appnd_redrct:   [[fallthrough]];
    case types::StmtKind::ovrwrit_redrct: [[fallthrough]];
    case types::StmtKind::merge_output:   [[fallthrough]];
    case types::StmtKind::merge_appnd:    {
      return output_redirection( stmt_node );
    }
    case types::StmtKind::merge_stream: {
      return merge_stream( stmt_node );
    }
    case types::StmtKind::stdin_redrct: {
      return input_redirection( stmt_node );
    }
    case types::StmtKind::trivial: {
      return trivial( static_cast<ExprNode*>( stmt_node ) );
    }
    default: assert( false ); break;
    }

    return !constants::EvalSuccess;
  }
} // namespace simsh
