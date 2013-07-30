/* abstract_game.cpp
 * Richard Gibson, Jul 26, 2013
 * Email: richard.g.gibson@gmail.com
 *
 * Wrapper class constructor/deconstructor for game and abstraction classes. 
 *
 * Copyright (C) 2013 by Richard Gibson
 */

/* C / C++ / STL indluces */
#include <string.h>

/* project_acpc_server includes */
extern "C" {
}

/* Pure CFR includes */
#include "abstract_game.hpp"
#include "constants.hpp"

AbstractGame::AbstractGame( const Parameters &params )
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

  /* Create action abstraction */
  switch( params.action_abs_type ) {
  case ACTION_ABS_NULL:
    action_abs = new NullActionAbstraction( );
    break;
  case ACTION_ABS_FCPA:
    action_abs = new FcpaActionAbstraction( );
    break;
  default:
    fprintf( stderr, "PureCfrMachine constructor: "
	     "Unrecognized action abstraction type [%s]\n",
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
    fprintf( stderr, "AbstractGame constructor: "
	     "Unrecognized card abstraction type [%s]\n",
	     card_abs_type_to_str[ ( int ) params.card_abs_type ] );
    exit( -1 );
  }  
}

AbstractGame::~AbstractGame( )
{
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

void AbstractGame::count_entries_r( const BettingNode *node,
				    size_t num_entries_per_bucket[ MAX_ROUNDS ],
				    size_t total_num_entries[ MAX_ROUNDS ] ) const
{
  const BettingNode *child = node->get_child( );

  if( child == NULL ) {
    /* Terminal node */
    return;
  }

  const int8_t round = node->get_round( );
  const int num_choices = node->get_num_choices( );

  /* Update entries counts */
  num_entries_per_bucket[ round ] += num_choices;
  const int buckets = card_abs->num_buckets( game, node );
  total_num_entries[ round ] += buckets * num_choices;

  /* Recurse */
  for( int c = 0; c < num_choices; ++c ) {
    count_entries_r( child, num_entries_per_bucket, total_num_entries );
    child = child->get_sibling( );
  }
}

void AbstractGame::count_entries( size_t num_entries_per_bucket[ MAX_ROUNDS ],
				  size_t total_num_entries[ MAX_ROUNDS ] ) const
{
  count_entries_r( betting_tree_root, num_entries_per_bucket, total_num_entries );
}
