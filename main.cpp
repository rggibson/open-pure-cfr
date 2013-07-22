/* main.cpp
 * Richard Gibson, Jun 27, 2013
 * Email: richard.g.gibson@gmail.com
 *
 * Entry point into Pure CFR that spawns worker threads to run pure cfr iterations.
 */

/* C / C++ includes */
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

/* C project-acpc-server includes */
extern "C" {
#include "acpc_server_code/game.h"
#include "acpc_server_code/rng.h"
}

/* Pure CFR includes */
#include "parameters.hpp"
#include "betting_node.hpp"
#include "abstraction.hpp"

int main( const int argc, const char *argv[] )
{
  /* Parse command line */
  Parameters params;
  if( params.parse( argc, argv ) ) {
    /* Problem parsing params; exit */
    return 1;
  } else {
    params.print_params( stderr );
  }

  /* Initialize regrets and things before starting Pure CFR iterations */
  PureCfrMachine pcm( params );

  /* TEST CODE BELOW */
  
  // FILE *file = fopen( params.game_file, "r" );
  // Game *game = readGame( file );
  // fclose( file );

  // Abstraction abs( params.card_abs_type, params.action_abs_type, game );

  // size_t num_entries_per_bucket[ MAX_ROUNDS ];
  // size_t total_num_entries[ MAX_ROUNDS ];
  // memset( num_entries_per_bucket, 0, MAX_ROUNDS * sizeof( size_t ) );
  // memset( total_num_entries, 0, MAX_ROUNDS * sizeof( size_t ) );

  // State state;
  // initState( game, 0, &state );

  // BettingNode *root = init_betting_tree_r( state,
  // 					   game,
  // 					   abs,
  // 					   num_entries_per_bucket,
  // 					   total_num_entries );

  // fprintf( stderr, "NUM_ENTRIES" );
  // for( int r = 0; r < MAX_ROUNDS; ++r ) {
  //   fprintf( stderr, " %jd", num_entries_per_bucket[ r ] );
  // }
  // fprintf( stderr, "\n" );

  // const BettingNode *cur = root;
  // fprintf( stderr, "soln_idx %jd\n", cur->get_soln_idx() );
  // fprintf( stderr, "num_choices %d\n", cur->get_num_choices() );
  // fprintf( stderr, "player %d\n", cur->get_player() );
  // fprintf( stderr, "round %d\n\n", cur->get_round() );

  // cur = cur->get_child();
  // cur = cur->get_sibling();
  // fprintf( stderr, "soln_idx %jd\n", cur->get_soln_idx() );
  // fprintf( stderr, "num_choices %d\n", cur->get_num_choices() );
  // fprintf( stderr, "player %d\n", cur->get_player() );
  // fprintf( stderr, "round %d\n\n", cur->get_round() );

  // cur = cur->get_child();
  // fprintf( stderr, "soln_idx %jd\n", cur->get_soln_idx() );
  // fprintf( stderr, "num_choices %d\n", cur->get_num_choices() );
  // fprintf( stderr, "player %d\n", cur->get_player() );
  // fprintf( stderr, "round %d\n\n", cur->get_round() );

  // cur = cur->get_child();
  // fprintf( stderr, "soln_idx %jd\n", cur->get_soln_idx() );
  // fprintf( stderr, "num_choices %d\n", cur->get_num_choices() );
  // fprintf( stderr, "player %d\n", cur->get_player() );
  // fprintf( stderr, "round %d\n\n", cur->get_round() );

  // cur = cur->get_child();
  // cur = cur->get_sibling();
  // fprintf( stderr, "soln_idx %jd\n", cur->get_soln_idx() );
  // fprintf( stderr, "num_choices %d\n", cur->get_num_choices() );
  // fprintf( stderr, "player %d\n", cur->get_player() );
  // fprintf( stderr, "round %d\n\n", cur->get_round() );

  /* Done! */
  return 0;
}
