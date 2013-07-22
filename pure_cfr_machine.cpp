/* pure_cfr_machine.cpp
 * Richard Gibson, Jul 1, 2013
 * Email: richard.g.gibson@gmail.com
 *
 * Contains the hand generation and Pure CFR tree walk methods.
 */

/* C / C++ / STL includes */
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

/* C project_acpc_poker includes */
extern "C" {
}

/* Pure CFR includes */
#include "pure_cfr_machine.hpp"

PureCfrMachine::PureCfrMachine( const Parameters &params )
  : do_average( params.do_average )
{
  /* Create Game */
  FILE *file = fopen( params.game_file, "r" );
  if( file == NULL ) {
    fprintf( stderr, "failed to open game file [%s]\n", params.game_file );
    exit( -1 );
  }
  game = readGame( file );
  if( game == NULL ) {
    fprintf( stderr, "failed to read game file [%s]\n", params.game_file );
    exit( -1 );
  }

  /* Check for problems */
  if( do_average && game->numPlayers > 2 ) {
    fprintf( stderr, "Sorry, averaging not implemented for > 2 player games.  "
	     "Use --no-average\n" );
    exit( -1 );
  }

  /* Create action abstraction */
  switch( params.action_abs_type ) {
  case ACTION_ABS_NULL:
    action_abs = new NullActionAbstraction( );
    break;
  case ACTION_ABS_FCPA:
    action_abs = new FcpaActionAbstraction( );
    break;
  default:
    fprintf( stderr, "PureCfrMachine constructor: Unrecognized action abstraction type [%s]\n",
	     action_abs_type_to_str[ ( int ) params.action_abs_type ] );
    exit( -1 );
  }
  
  /* init num_entries_per_bucket to zero */
  size_t num_entries_per_bucket[ MAX_ROUNDS ];
  memset( num_entries_per_bucket, 0,
	  MAX_ROUNDS * sizeof( num_entries_per_bucket[ 0 ] ) );

  /* process betting tree */
  State state;
  initState( game, 0, &state );
  betting_tree_root = init_betting_tree_r( state, game, action_abs,
					   num_entries_per_bucket );

  /* Create card abstraction */
  switch( params.card_abs_type ) {
  case CARD_ABS_NULL:
    card_abs = new NullCardAbstraction( game );
    break;
  case CARD_ABS_BLIND:
    card_abs = new BlindCardAbstraction( );
    break;
  default:
    fprintf( stderr, "PureCfrMachine constructor: Unrecognized card abstraction type [%s]\n",
	     card_abs_type_to_str[ ( int ) params.card_abs_type ] );
    exit( -1 );
  }

  /* count up the total number of entries required per round to store regret,
   * avg_strategy
   */
  size_t total_num_entries[ MAX_ROUNDS ];
  memset( total_num_entries, 0, MAX_ROUNDS * sizeof( total_num_entries[ 0 ] ) );
  card_abs->count_total_num_entries_r( game, betting_tree_root, total_num_entries );
  
  /* initialize regret and avg strategy */
  for( int r = 0; r < MAX_ROUNDS; ++r ) {
    if( r < game->numRounds ) {

      /* Regret */
      switch( REGRET_TYPES[ r ] ) {
      case TYPE_INT:
	regrets[ r ] = new Entries_der<int>( num_entries_per_bucket[ r ],
					     total_num_entries[ r ] );
	break;

      default:
	fprintf( stderr, "unrecognized regret type [%d], "
		 "note that type must be signed\n",
		 REGRET_TYPES[ part ][ p ][ r ] );
	exit( -1 );
      }
	  	  
      if( do_average ) {
	switch( AVG_STRATEGY_TYPES[ r ] ) {
	case TYPE_UINT8_T:
	  avg_strategy[ r ]
	    = new Entries_der<uint8_t>( num_entries_per_bucket[ r ],
					total_num_entries[ r ] );
	  break;

	case TYPE_INT:
	  avg_strategy[ r ]
	    = new Entries_der<int>( num_entries_per_bucket[ r ],
				    total_num_entries[ r ] );
	  break;
	  
	case TYPE_UINT32_T:
	  avg_strategy[ r ]
	    = new Entries_der<uint32_t>( num_entries_per_bucket[ r ],
					 total_num_entries[ r ] );
	  break;
		
	case TYPE_UINT64_T:
	  avg_strategy[ r ]
	    = new Entries_der<uint64_t>( num_entries_per_bucket[ r ],
					 total_num_entries[ r ] );
	  break;
	  
	default:
	  fprintf( stderr, "unrecognized avg strategy type [%d]\n",
		   AVG_STRATEGY_TYPES[ r ] );
	  exit( -1 );
	}
      } else {
	avg_strategy[ r ] = NULL;
      }
	    	  
    } else {
      /* Round out of range */
      regrets[ r ] = NULL;
      avg_strategy[ r ] = NULL;
    }
  }
}

StrategyProfile::~StrategyProfile( )
{
  for( int r = 0; r < MAX_ROUNDS; ++r ) {
    if( regrets[ r ] != NULL ) {
      delete regrets[ r ];
      regrets[ r ] = NULL;
    }
    if( avg_strategy[ r ] != NULL ) {
      delete avg_strategy[ r ];
      avg_strategy[ r ] = NULL;
    }
  }
  
  if( card_abs != NULL ) {
    delete card_abs;
    card_abs = NULL;
  }
  
  if( betting_tree_root != NULL ) {
    destroy_betting_tree_r( betting_tree_root );
    betting_tree_root = NULL;
  }

  if( action_abs != NULL ) {
    delete action_abs;
    action_abs = NULL;
  }

  if( game != NULL ) {
    free( game );
    game = NULL;
  }
}

void PureCfrMachine::do_iteration( rng_state_t &rng )
{
  hand_t hand;
  if( generate_hand( hand, rng ) ) {
    fprintf( stderr, "Unable to generate hand.\n" );
    exit( -1 );
  }
  for( int p = 0; p < game->numPlayers; ++p ) {
    walk_pure_cfr( p, betting_tree_root, hand, rng );
  }
}

int PureCfrMachine::write_dump( const char *dump_prefix,
				const bool do_regrets ) const
{
  /* Let's dump regrets first if required, then average strategy if necessary */
  if( do_regrets ) {
    /* Build the filename */
    char filename[ PATH_LENGTH ];
    snprintf( filename, PATH_LENGTH, "%s.regrets.dump", dump_prefix );

    /* Open the file */
    FILE *file = fopen( filename, "w" );
    if( file == NULL ) {
      fprintf( stderr, "Could not open dump file [%s]\n", filename );
      return 1;
    }

    /* Dump regrets */
    for( int r = 0; r < game->numRounds; ++r ) {
      if( regrets[ r ]->write( file ) ) {
	fprintf( stderr, "Error while dumping round %d to file [%s]\n",
		 r, filename );
	return 1;
      }
    }
    
    fclose( file );
  }

  if( do_average ) {
    /* Dump avg strategy */

    /* Build the filename */
    char filename[ PATH_LENGTH ];
    snprintf( filename, PATH_LENGTH, "%s.avg-strategy.dump", dump_prefix );

    /* Open the file */
    FILE *file = fopen( filename, "w" );
    if( file == NULL ) {
      fprintf( stderr, "Could not open dump file [%s]\n", filename );
      return 1;
    }

    for( int r = 0; r < game->numRounds; ++r ) {
      if( avg_strategy[ r ]->write( file ) ) {
	return 1;
      }
    }

    fclose( file );
  }

  return 0;
}

int PureCfrMachine::load_dump( const char *dump_prefix )
{
  /* Let's load regrets first, then average strategy if necessary */

  /* Build the filename */
  char filename[ PATH_LENGTH ];
  snprintf( filename, PATH_LENGTH, "%s.regrets.dump", dump_prefix );

  /* Open the file */
  FILE *file = fopen( filename, "r" );
  if( file == NULL ) {
    fprintf( stderr, "Could not open dump load file [%s]\n", filename );
    return 1;
  }

  /* Load regrets */
  for( int r = 0; r < game->numRounds; ++r ) {

    if( regrets[ r ]->load( file ) ) {
      fprintf( stderr, "failed to load dump file [%s] for round %d\n",
	       filename, r );
      return 1;
    }
  }
  fclose( file );

  if( do_average ) {
    /* Build the filename */
    snprintf( filename, PATH_LENGTH, "%s.avg-strategy.dump", dump_prefix );

    /* Open the file */
    file = fopen( filename, "r" );
    if( file == NULL ) {
      fprintf( stderr, "WARNING: Could not open dump load file [%s]\n",
	       filename );
      fprintf( stderr, "All average values set to zero.\n" );
      return -1;
    }

    /* Load avg strategy */
    for( int r = 0; r < gamedefs[ 0 ]->ROUNDS; ++r ) {
	  
      if( avg_strategy[ r ]->load( file ) ) {
	fprintf( stderr, "failed to load dump file [%s] for round %d\n",
		 filename, r );
	return 1;
      }
    }

    fclose( file );
  }

  return 0;
}

int PureCfrMachine::generate_hand( hand_t &hand, rng_state_t &rng )
{
  /* First, deal out the cards and copy them over */
  State state;
  dealCards( game, &rng, &state );
  memcpy( hand.boardCards, state.boardCards, MAX_BOARD_CARDS * sizeof( hand.boardCards[ 0 ] ) );
  for( int p = 0; p < MAX_PURE_CFR_PLAYERS; ++p ) {
    memcpy( hand.holeCards[ p ], state.holeCards[ p ],
	    MAX_HOLE_CARDS * sizeof( hand.holeCards[ 0 ] ) );
  }

  /* Bucket the hands for each player, round if possible */
  abs->precompute_buckets( game, state, hand.precomputed_buckets );

  /* Rank the hands */
  int ranks[ MAX_PURE_CFR_PLAYERS ];
  int top_rank = -1;
  int num_ties = 1;;
  for( int p = 0; p < gamedef->NUM_PLAYERS; ++p ) {
    ranks[ p ] = rankHand( game, &state, player );
    if( ranks[ p ] > top_rank ) {
      top_rank = ranks[ p ];
      num_ties = 1;
    } else if( ranks[ p ] == top_rank ) {
      ++num_ties;
    }
  }

  /* Set evaluation values */
  switch( game->numPlayers ) {
  case 2:
    if( ranks[ 0 ] > ranks[ 1 ] ) {
      /* Player 0 wins in showdown */
      hand.eval.showdown_value_2p[ 0 ] = 1;
      hand.eval.showdown_value_2p[ 1 ] = -1;
    } else if( ranks[ 0 ] < ranks[ 1 ] ) {
      /* Player 1 wins in showdown */
      hand.eval.showdown_value_2p[ 0 ] = -1;
      hand.eval.showdown_value_2p[ 1 ] = 1;
    } else {
      /* Players tie in showdown */
      hand.eval.showdown_value_2p[ 0 ] = 0;
      hand.eval.showdown_value_2p[ 1 ] = 0;
    }
    break;

  case 3:
    /* Two players fold */
    hand.eval.pot_frac_recip[ 0 ][ LEAF_P0 ] = 1;
    hand.eval.pot_frac_recip[ 1 ][ LEAF_P0 ] = INT_MAX;
    hand.eval.pot_frac_recip[ 2 ][ LEAF_P0 ] = INT_MAX;
    hand.eval.pot_frac_recip[ 0 ][ LEAF_P1 ] = INT_MAX;
    hand.eval.pot_frac_recip[ 1 ][ LEAF_P1 ] = 1;    
    hand.eval.pot_frac_recip[ 2 ][ LEAF_P1 ] = INT_MAX;
    hand.eval.pot_frac_recip[ 0 ][ LEAF_P2 ] = INT_MAX;
    hand.eval.pot_frac_recip[ 1 ][ LEAF_P2 ] = INT_MAX;    
    hand.eval.pot_frac_recip[ 2 ][ LEAF_P2 ] = 1;
    /* One player folds */
    hand.eval.pot_frac_recip[ 0 ][ LEAF_P1_P2 ] = INT_MAX;  
    hand.eval.pot_frac_recip[ 1 ][ LEAF_P0_P2 ] = INT_MAX;  
    hand.eval.pot_frac_recip[ 2 ][ LEAF_P0_P1 ] = INT_MAX;  
    if( ranks[ 0 ] > ranks[ 1 ] ) {
      /* Player 0 wins in showdown when player 2 folds */
      hand.eval.pot_frac_recip[ 0 ][ LEAF_P0_P1 ] = 1;
      hand.eval.pot_frac_recip[ 1 ][ LEAF_P0_P1 ] = INT_MAX;
    } else if( ranks[ 0 ] < ranks[ 1 ] ) {
      /* Player 1 wins in showdown when player 2 folds */
      hand.eval.pot_frac_recip[ 0 ][ LEAF_P0_P1 ] = INT_MAX;
      hand.eval.pot_frac_recip[ 1 ][ LEAF_P0_P1 ] = 1;
    } else {
      /* Players 0 and 1 tie in showdown when player 2 folds */
      hand.eval.pot_frac_recip[ 0 ][ LEAF_P0_P1 ] = 2;
      hand.eval.pot_frac_recip[ 1 ][ LEAF_P0_P1 ] = 2; 
    }    
    if( ranks[ 0 ] > ranks[ 2 ] ) {
      /* Player 0 wins in showdown when player 1 folds */
      hand.eval.pot_frac_recip[ 0 ][ LEAF_P0_P2 ] = 1;
      hand.eval.pot_frac_recip[ 2 ][ LEAF_P0_P2 ] = INT_MAX;
    } else if( ranks[ 0 ] < ranks[ 2 ] ) {
      /* Player 2 wins in showdown when player 1 folds */
      hand.eval.pot_frac_recip[ 0 ][ LEAF_P0_P2 ] = INT_MAX;
      hand.eval.pot_frac_recip[ 2 ][ LEAF_P0_P2 ] = 1;
    } else {
      /* Players 0 and 2 tie in showdown when player 1 folds */
      hand.eval.pot_frac_recip[ 0 ][ LEAF_P0_P2 ] = 2;
      hand.eval.pot_frac_recip[ 2 ][ LEAF_P0_P2 ] = 2; 
    }    
    if( ranks[ 1 ] > ranks[ 2 ] ) {
      /* Player 1 wins in showdown when player 0 folds */
      hand.eval.pot_frac_recip[ 1 ][ LEAF_P1_P2 ] = 1;
      hand.eval.pot_frac_recip[ 2 ][ LEAF_P1_P2 ] = INT_MAX;
    } else if( ranks[ 1 ] < ranks[ 2 ] ) {
      /* Player 2 wins in showdown when player 0 folds */
      hand.eval.pot_frac_recip[ 1 ][ LEAF_P1_P2 ] = INT_MAX;
      hand.eval.pot_frac_recip[ 2 ][ LEAF_P1_P2 ] = 1;
    } else {
      /* Players 1 and 2 tie in showdown when player 0 folds */
      hand.eval.pot_frac_recip[ 1 ][ LEAF_P1_P2 ] = 2;
      hand.eval.pot_frac_recip[ 2 ][ LEAF_P1_P2 ] = 2; 
    }
    /* No players fold */
    for( int p = 0; p < gamedef->NUM_PLAYERS; ++p ) {
      if( ranks[ p ] == top_rank ) {
	hand.eval.pot_frac_recip[ p ][ LEAF_P0_P1_P2 ] = num_ties;
      } else {
	hand.eval.pot_frac_recip[ p ][ LEAF_P0_P1_P2 ] = INT_MAX;
      }
    }
    break;

  default:
    fprintf( stderr, "can't set terminal pot fraction denominators in "
	     "%d-player game\n", game->numPlayers );
    return 1;
  }

  return 0;  
}

int StrategyProfile::walk_pure_cfr( const int position,
				    const BettingNode *cur_node,
				    const hand_t *hand,
				    rng_state_t *rng )
{
  int retval = 0;

  if( ( cur_node->get_child( ) == NULL ) || cur_node->did_player_fold( position ) ) {
    /* Game over, calculate utility */
    
    retval = cur_node->evaluate( hand, position );
    
    return retval;
  }

  /* Grab some values that will be used often */
  int num_choices = cur_node->get_num_choices( );
  int8_t part = cur_node->get_part( );
  int8_t player = cur_node->get_player( );
  int8_t round = cur_node->get_round( );

  /* Get the positive regrets at this information set */
  uint64_t pos_regrets[ num_choices ];
  uint64_t sum_pos_regrets
    = regrets[ part ][ player ][ round ]->get_pos_values( hand->buckets[ part ][ player ][ round ],
							  cur_node->get_soln_idx( ),
							  num_choices,
							  pos_regrets );
  if( sum_pos_regrets == 0 ) {
    /* No positive regret, so assume a default uniform random current strategy */
    sum_pos_regrets = num_choices;
    for( int c = 0; c < num_choices; ++c ) {
      pos_regrets[ c ] = 1;
    }
  }

  /* Purify the current strategy so that we always take choice */
  uint32_t dart = genrand_int32( rng ) % sum_pos_regrets;
  int choice;
  for( choice = 0; choice < num_choices; ++choice ) {
    if( dart < ( uint32_t ) pos_regrets[ choice ] ) {
      break;
    }
    dart -= pos_regrets[ choice ];
  }
  assert( choice < num_choices );
  assert( pos_regrets[ choice ] > 0 );
  
  const BettingNode *child = cur_node->get_child( );

  if( ( player != position )
      || ( ( expert_dump != NULL ) && ( part == 0 ) ) ) {
    /* Opponent's node, or base strategy node in EXPERT mode. Recurse down the single choice. */

    for( int c = 0; c < choice; ++c ) {
      child = child->get_sibling( );
    }

    retval = walk_pure_cfr( position, child, hand, rng );

    /* Update the average strategy if we are keeping track of one */
    if( avg_strategy[ part ][ player ][ round ] != NULL ) {
      if( avg_strategy[ part ][ player ][ round ]->increment_entry
	  ( hand->buckets[ part ][ player ][ round ], cur_node->get_soln_idx( ), choice ) ) {
	fprintf( stderr, "The average strategy has overflown :(\n" );
	fprintf( stderr, "To fix this, you must set a begger AVG_STRATEGY_TYPE in constants.cpp\n" );
	fprintf( stderr, "If you are loading a checkpointed dump, you must also do the following\n" );
	fprintf( stderr, "in order to use your old dump, after changing AVG_STRATEGY_TYPE:\n" );
	fprintf( stderr, "1. Run convert_dump on your checkpointed dump(s)\n" );
	fprintf( stderr, "2. Delete or rename your old dumps to something else\n" );
	fprintf( stderr, "3. Rename *.dump.converted to *.dump\n" );
	fprintf( stderr, "4. Run pure_cfr again with these new dump files\n" );
	exit( 1 );
      }
    }
    
  } else {
    /* Current player's node. Recurse down all choices to get the value of each */

    int values[ num_choices ];
    
    for( int c = 0; c < num_choices; ++c ) {
      values[ c ] = walk_pure_cfr( position, child, hand, rng );
      child = child->get_sibling( );
    }

    /* We return the value that the sampled pure strategy attains */
    retval = values[ choice ];

    /* Update the regrets at the current node */
    regrets[ part ][ player ][ round ]->update_regret( hand->buckets[ part ][ player ][ round ],
						       cur_node->get_soln_idx( ),
						       num_choices,
						       values,
						       retval );
  }
  
  return retval;
}
