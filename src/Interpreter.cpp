#include <util/Config.hpp>
#include <HelpDocument.hpp>
#include <Interpreter.hpp>
#include <cassert>
#include <cstdlib>
#include <fcntl.h>
#include <filesystem>
#include <unistd.h>
#include <util/Constant.hpp>
#include <util/Exception.hpp>
#include <util/ForkGuard.hpp>
#include <util/Logger.hpp>
#include <util/Pipe.hpp>
#include <util/Util.hpp>
using namespace std;

namespace tish {
  const std::unordered_set<type::String> Interpreter::_built_in_cmds = { "cd",
                                                                         "exit",
                                                                         "help",
                                                                         "type",
                                                                         "exec" };

  Interpreter::EvalResult Interpreter::sequential_stmt( StmtNodeT seq_stmt ) const
  {
    assert( seq_stmt != nullptr );
    assert( seq_stmt->left() != nullptr );
    assert( seq_stmt->siblings().empty() == true );

    EvalResult ret;
    ret.side_val_.push_back( evaluate( seq_stmt->left() ).value_ );
    if ( seq_stmt->right() != nullptr )
      ret.side_val_.push_back( evaluate( seq_stmt->right() ).value_ );
    ret.value_ = ret.side_val_.back();
    return ret;
  }

  Interpreter::EvalResult Interpreter::logical_and( StmtNodeT and_stmt ) const
  {
    assert( and_stmt != nullptr );
    assert( and_stmt->left() != nullptr && and_stmt->right() != nullptr );
    assert( and_stmt->siblings().empty() == true );

    auto ret = EvalResult( evaluate( and_stmt->left() ).value_ );
    ret.side_val_.push_back( ret.value_ );
    if ( ret ) {
      ret.side_val_.push_back( evaluate( and_stmt->right() ).value_ );
      ret.value_ = ret.value_ && ret.side_val_.back();
    }
    return ret;
  }

  Interpreter::EvalResult Interpreter::logical_or( StmtNodeT or_stmt ) const
  {
    assert( or_stmt != nullptr );
    assert( or_stmt->left() != nullptr && or_stmt->right() != nullptr );
    assert( or_stmt->siblings().empty() == true );

    auto ret = EvalResult( evaluate( or_stmt->left() ).value_ );
    ret.side_val_.push_back( ret.value_ );
    if ( !ret ) {
      ret.side_val_.push_back( evaluate( or_stmt->right() ).value_ );
      ret.value_ = ret.value_ || ret.side_val_.back();
    }
    return ret;
  }

  Interpreter::EvalResult Interpreter::logical_not( StmtNodeT not_stmt ) const
  {
    assert( not_stmt != nullptr );

    assert( not_stmt->left() != nullptr && not_stmt->right() == nullptr );
    assert( not_stmt->siblings().empty() == true );

    EvalResult ret;
    ret.side_val_.push_back( evaluate( not_stmt->left() ).value_ );
    ret.value_ = !ret.side_val_.back();
    return ret;
  }

  Interpreter::EvalResult Interpreter::pipeline_stmt( StmtNodeT pipeline_stmt ) const
  {
    assert( pipeline_stmt != nullptr );
    assert( pipeline_stmt->left() != nullptr && pipeline_stmt->right() != nullptr );
    assert( pipeline_stmt->siblings().empty() == true );

    util::Pipe pipe;
    util::disable_blocking( pipe.reader().get() );

    EvalResult ret;
    for ( size_t i = 0; i < 2; ++i ) {
      util::ForkGuard pguard;
      if ( pguard.is_child() ) {
        // child process
        if ( i == 0 ) {
          pipe.reader().close();
          util::rebind_fd( pipe.writer().get(), STDOUT_FILENO );
          evaluate( pipeline_stmt->left() );
        } else {
          pipe.writer().close();
          util::rebind_fd( pipe.reader().get(), STDIN_FILENO );
          evaluate( pipeline_stmt->right() );
        }
        throw error::TerminationSignal( EXIT_FAILURE );
      }
      pguard.wait();
      ret.side_val_.push_back( pguard.exit_code().value() );
    }

    ret.value_ =
      ret.side_val_.front() == EvalResult::success ? ret.side_val_.back() : ret.side_val_.front();
    return ret;
  }

  Interpreter::EvalResult Interpreter::output_redirection( StmtNodeT oup_redr ) const
  {
    assert( oup_redr != nullptr );

    assert( oup_redr->right() == nullptr );
    assert( oup_redr->siblings().empty() == false );
    assert( oup_redr->siblings().front()->type() == StmtNode::StmtKind::atom );

    assert( static_cast<const ExprNode*>(
              oup_redr
                ->siblings()[oup_redr->type() == StmtNode::StmtKind::appnd_redrct
                                 || oup_redr->type() == StmtNode::StmtKind::ovrwrit_redrct
                               ? 1
                               : 0]
                .get() )
              ->kind()
            != ExprNode::ExprKind::value );

    const auto& filename =
      static_cast<const ExprNode*>(
        oup_redr
          ->siblings()[oup_redr->type() == StmtNode::StmtKind::appnd_redrct
                           || oup_redr->type() == StmtNode::StmtKind::ovrwrit_redrct
                         ? 1
                         : 0]
          .get() )
        ->token();

    // Check whether the file descriptor can be obtained.
    if ( !filesystem::exists( filename ) && !util::create_file( filename ) ) {
      iout::logger.print( error::SystemCallError( type::String( filename ) ) );
      return { EvalResult::abort };
    } else if ( const auto perms = filesystem::status( filename ).permissions();
                ( perms & filesystem::perms::owner_write )
                == filesystem::perms::none ) { // not writable
      iout::logger.print( error::SystemCallError( type::String( filename ) ) );
      return { EvalResult::abort };
    }

    /* For `StmtNode::StmtKind::appnd_redrct` and `StmtNode::StmtKind::ovrwrit_redrct`
     * node, the first element of `merg_redr->siblings()` is `ExprNode` of type
     * `ExprNode::ExprKind::value`, which specifies the destination file
     * descriptor. */
    type::FileDesc file_d = STDOUT_FILENO;
    if ( oup_redr->type() == StmtNode::StmtKind::appnd_redrct
         || oup_redr->type() == StmtNode::StmtKind::ovrwrit_redrct ) {
      ExprNodeT arg_node = static_cast<ExprNode*>( oup_redr->siblings().front().get() );

      assert( arg_node->kind() == ExprNode::ExprKind::value );

      file_d = arg_node->value() == constant::invalid_value ? STDOUT_FILENO : arg_node->value();
    }

    util::ForkGuard pguard;
    if ( pguard.is_child() ) {
      auto target_fd = open( filename.c_str(),
                             O_WRONLY
                               | ( oup_redr->type() == StmtNode::StmtKind::appnd_redrct
                                       || oup_redr->type() == StmtNode::StmtKind::merge_appnd
                                     ? O_APPEND
                                     : O_TRUNC ) );

      util::rebind_fd( target_fd, file_d );
      if ( oup_redr->type() == StmtNode::StmtKind::merge_output
           || oup_redr->type() == StmtNode::StmtKind::merge_appnd ) {
        if ( file_d != STDOUT_FILENO )
          util::rebind_fd( target_fd, STDOUT_FILENO );
        if ( file_d != STDERR_FILENO )
          util::rebind_fd( target_fd, STDERR_FILENO );
      }

      if ( oup_redr->left() != nullptr ) {
        evaluate( oup_redr->left() );
        close( target_fd );
        throw error::TerminationSignal( EXIT_FAILURE );
      } else {
        close( target_fd );
        throw error::TerminationSignal( EXIT_SUCCESS );
      }
    }
    pguard.wait();

    assert( pguard.exit_code().has_value() );

    auto ret = EvalResult( pguard.exit_code().value() );
    if ( oup_redr->left() == nullptr || oup_redr->left()->type() != StmtNode::StmtKind::atom )
      ret.side_val_.push_back( ret.value_ ); // Exists a subexpression
    return ret;
  }

  Interpreter::EvalResult Interpreter::merge_stream( StmtNodeT merg_redr ) const
  {
    assert( merg_redr != nullptr );

    assert( merg_redr->left() != nullptr && merg_redr->right() == nullptr );
    assert( merg_redr->siblings().size() == 2 );
    assert( merg_redr->siblings()[0]->type() == StmtNode::StmtKind::atom
            && merg_redr->siblings()[1]->type() == StmtNode::StmtKind::atom );

    /* For `StmtNode::StmtKind::merge_stream` node,
     * the first and second elements of `merg_redr->siblings()` is `ExprNode` of
     * type `ExprNode::ExprKind::value`, which specifies the destination file descriptor. */
    ExprNodeT arg_node1 = static_cast<ExprNode*>( merg_redr->siblings()[0].get() );
    ExprNodeT arg_node2 = static_cast<ExprNode*>( merg_redr->siblings()[1].get() );

    assert( arg_node1->kind() == ExprNode::ExprKind::value );
    assert( arg_node2->kind() == ExprNode::ExprKind::value );

    const auto l_fd =
      arg_node1->value() == constant::invalid_value ? STDERR_FILENO : arg_node1->value();
    const auto r_fd =
      arg_node2->value() == constant::invalid_value ? STDOUT_FILENO : arg_node2->value();

    util::ForkGuard pguard;
    if ( pguard.is_child() ) {
      util::rebind_fd( r_fd, l_fd );

      evaluate( merg_redr->left() );
      throw error::TerminationSignal( EXIT_FAILURE );
    }
    pguard.wait();

    assert( pguard.exit_code().has_value() );

    auto ret = EvalResult( pguard.exit_code().value() );
    if ( merg_redr->left()->type() != StmtNode::StmtKind::atom )
      ret.side_val_.push_back( ret.value_ );
    return ret;
  }

  Interpreter::EvalResult Interpreter::input_redirection( StmtNodeT inp_redr ) const
  {
    assert( inp_redr != nullptr );
    assert( inp_redr->left() != nullptr && inp_redr->right() == nullptr );
    assert( inp_redr->siblings().empty() == false );
    assert( inp_redr->siblings().front()->type() == StmtNode::StmtKind::atom );

    if ( inp_redr->siblings().size() != 1 ) {
      iout::logger << error::ArgumentError( "input redirection"sv, "argument number error"sv );
      return { EvalResult::abort };
    }

    const auto& filename =
      static_cast<const ExprNode*>( inp_redr->siblings().front().get() )->token();
    if ( !filesystem::exists( filename ) ) {
      iout::logger.print( error::SystemCallError( type::String( filename ) ) );
      return { EvalResult::abort };
    } else if ( const auto perms = filesystem::status( filename ).permissions();
                ( perms & filesystem::perms::owner_read )
                == filesystem::perms::none ) { // not readable
      iout::logger.print( error::SystemCallError( type::String( filename ) ) );
      return { EvalResult::abort };
    }

    util::ForkGuard pguard;
    if ( pguard.is_child() ) {
      auto target_fd = open( filename.c_str(), O_RDONLY );
      util::rebind_fd( target_fd, STDIN_FILENO );

      evaluate( inp_redr->left() );

      close( target_fd );
      throw error::TerminationSignal( EXIT_FAILURE );
    }
    pguard.wait();
    return { pguard.exit_code().value() };
  }

  Interpreter::EvalResult Interpreter::atom( ExprNodeT expr ) const
  {
    assert( expr != nullptr );

    assert( expr->left() == nullptr && expr->right() == nullptr );
    assert( expr->type() == StmtNode::StmtKind::atom );

    const auto cmd_expand = []( ExprNodeT node ) -> void {
      if ( node->kind() == ExprNode::ExprKind::value )
        return;
      else if ( node->token() == "$$"sv )
        node->replace_with( format( "{}", getpid() ) );
      else if ( node->token() == "$TISH_VERSION"sv )
        node->replace_with( TISH_VERSION );
      else if ( node->kind() == ExprNode::ExprKind::command ) {
        if ( auto new_token = util::tilde_expansion( node->token() ); !new_token.empty() )
          node->replace_with( move( new_token ) );
      }
    };

    cmd_expand( expr );
    for ( auto& sblng : expr->siblings() ) {
      assert( sblng->type() == StmtNode::StmtKind::atom );

      const auto node = static_cast<ExprNode*>( sblng.get() );
      assert( node->kind() != ExprNode::ExprKind::value );
      cmd_expand( node );
    }

    if ( expr->kind() == ExprNode::ExprKind::value )
      return { expr->value() };
    else if ( _built_in_cmds.contains( expr->token() ) )
      return builtin_exec( expr );
    else
      return external_exec( expr );
  }

  Interpreter::EvalResult Interpreter::builtin_exec( ExprNodeT expr ) const
  {
    assert( expr != nullptr );
    assert( expr->kind() != ExprNode::ExprKind::value );

    EvalResult ret;
    switch ( expr->token().front() ) {
    case 'c': { // cd
      if ( expr->siblings().size() > 1 ) {
        iout::logger << error::ArgumentError( "cd"sv, "the number of arguments error"sv );
        return { EvalResult::abort };
      }

      assert( static_cast<const ExprNode*>( expr->siblings().front().get() )->kind()
              != ExprNode::ExprKind::value );

      type::StrView target_dir =
        expr->siblings().size() == 0
          ? util::get_homedir()
          : static_cast<const ExprNode*>( expr->siblings().front().get() )->token();

      try {
        filesystem::current_path( target_dir );
      } catch ( const filesystem::filesystem_error& e ) {
        iout::logger.print( error::SystemCallError( format( "cd: {}", target_dir.data() ) ) );
        return { EvalResult::abort };
      }
      return { EvalResult::success };
    } break;

    case 'e': { // exit or exec
      if ( expr->token() == "exit" ) {
        if ( !expr->siblings().empty() ) {
          iout::logger << error::ArgumentError( "exit"sv, "the number of arguments error"sv );
          return { EvalResult::abort };
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
                             assert( sblng->type() == StmtNode::StmtKind::atom );
                             ExprNodeT arg_node = static_cast<ExprNode*>( sblng.get() );
                             assert( arg_node->kind() != ExprNode::ExprKind::value );

                             return const_cast<char*>( arg_node->token().data() );
                           } );
        exec_argv.push_back( nullptr );

        execvp( exec_argv.front(), exec_argv.data() );

        const auto error_info = static_cast<ExprNode*>( expr->siblings().front().get() );
        iout::logger << error::ArgumentError(
          "exec",
          format( "{}: command not found", error_info->token() ) );
        return { EvalResult::abort };
      }
    } break;

    case 'h': { // help
      if ( !expr->siblings().empty() ) {
        iout::logger << error::ArgumentError( "help"sv, "the number of arguments error"sv );
        return { EvalResult::abort };
      }
      iout::prmptr << util::help_doc();
    } break;

    case 't': { // type
      if ( expr->siblings().empty() )
        return { EvalResult::abort };

      for ( const auto& sblng : expr->siblings() ) {
        assert( sblng->type() == StmtNode::StmtKind::atom );

        ExprNodeT arg_node = static_cast<ExprNode*>( sblng.get() );
        assert( arg_node->kind() != ExprNode::ExprKind::value );

        if ( _built_in_cmds.contains( arg_node->token() ) )
          iout::prmptr << format( "{} is a builtin\n", arg_node->token() );
        else if ( const auto filepath =
                    util::search_filepath( util::get_envpath(), arg_node->token() );
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

    return { EvalResult::success };
  }

  Interpreter::EvalResult Interpreter::external_exec( ExprNodeT expr ) const
  {
    assert( expr != nullptr );

    util::Pipe pipe;
    util::disable_blocking( pipe.reader().get() );

    util::ForkGuard pguard;
    if ( pguard.is_child() ) {
      // child process
      vector<char*> exec_argv { const_cast<char*>( expr->token().data() ) };
      exec_argv.reserve( expr->siblings().size() + 2 );
      ranges::transform( expr->siblings(),
                         back_inserter( exec_argv ),
                         []( const auto& sblng ) -> char* {
                           assert( sblng->type() == StmtNode::StmtKind::atom );
                           ExprNodeT arg_node = static_cast<ExprNode*>( sblng.get() );
                           assert( arg_node->kind() != ExprNode::ExprKind::value );

                           return const_cast<char*>( arg_node->token().data() );
                         } );
      exec_argv.push_back( nullptr );

      pguard.reset_signals();
      execvp( exec_argv.front(), exec_argv.data() );

      pipe.writer().push( true );
      // Ensure that all scoped objects are destructed normally.
      // In particular the object `util::Pipe` above.
      throw error::TerminationSignal( EvalResult::abort );
    } else {
      pguard.wait();
      if ( pipe.reader().pop<bool>() )
        iout::logger << error::ArgumentError( expr->token(), "command not found" );

      return { pguard.exit_code().value() };
    }
  }

  Interpreter::EvalResult Interpreter::evaluate( StmtNodeT stmt_node ) const
  {
    if ( stmt_node == nullptr )
      throw error::ArgumentError( "interpreter", "syntax tree node is null" );

    switch ( stmt_node->type() ) {
    case StmtNode::StmtKind::sequential: {
      return sequential_stmt( stmt_node );
    }
    case StmtNode::StmtKind::logical_and: {
      return logical_and( stmt_node );
    }
    case StmtNode::StmtKind::logical_or: {
      return logical_or( stmt_node );
    }
    case StmtNode::StmtKind::logical_not: {
      return logical_not( stmt_node );
    }
    case StmtNode::StmtKind::pipeline: {
      return pipeline_stmt( stmt_node );
    }
    case StmtNode::StmtKind::appnd_redrct:   [[fallthrough]];
    case StmtNode::StmtKind::ovrwrit_redrct: [[fallthrough]];
    case StmtNode::StmtKind::merge_output:   [[fallthrough]];
    case StmtNode::StmtKind::merge_appnd:    {
      return output_redirection( stmt_node );
    }
    case StmtNode::StmtKind::merge_stream: {
      return merge_stream( stmt_node );
    }
    case StmtNode::StmtKind::stdin_redrct: {
      return input_redirection( stmt_node );
    }
    case StmtNode::StmtKind::atom: {
      return atom( static_cast<ExprNode*>( stmt_node ) );
    }
    default: assert( false ); break;
    }

    return {};
  }
} // namespace tish
