/* player_module.cpp
 * Richard Gibson, Jul 26, 2013
 * Email: richard.g.gibson@gmail.com
 *
 * Player module class that provides an interface for actually playing
 * poker with a dealer and looking up action probabilities.
 *
 * Copyright (C) 2013 by Richard Gibson
 */

/* C / C++ / STL indluces */
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

/* project_acpc_server includes */
extern "C" {
}

/* Pure CFR includes */
#include "player_module.hpp"
#include "utility.hpp"

PlayerModule::PlayerModule( const char *player_file )
  : ag( NULL ),
    verbose( false )
{
  /* Open up the player file for reading */
  FILE *file = fopen( player_file, "r" );
  if( file == NULL ) {
    fprintf( stderr, "Could not open player file [%s]\n", player_file );
    exit( -1 );
  }

  /* Get the parameters from the player file */
  Parameters params;
  if( params.read_params( file ) ) {
    fprintf( stderr, "Failed to read parameters from player file [%s]\n",
	     player_file );
    exit( -1 );
  }

  /* Read the rest of the player file */
  char line[ PATH_LENGTH ];
  char binary_filename[ PATH_LENGTH ];
  while( fgets( line, PATH_LENGTH, file ) ) {

    /* Ignore comments and blank lines */
    if( ( line[ 0 ] == '#' ) || ( line[ 0 ] == '\n' ) ) {
      continue;
    }

    if( !strncmp( line, "PLAYER_END", strlen( "PLAYER_END" ) ) ) {
      /* End of player file */
      break;
      
    } else if( !strncmp( line, "VERBOSE", strlen( "VERBOSE" ) ) ) {
      verbose = true;
      
    } else if( !strncmp( line, "BINARY_FILENAME_PREFIX",
			 strlen( "BINARY_FILENAME_PREFIX" ) ) ) {
      /* Get the binary filename to load later */
      char tmp[ PATH_LENGTH ];
      if( get_next_token( tmp, &line[ strlen( "BINARY_FILENAME_PREFIX" ) ] ) ) {
	fprintf( stderr, "Error reading BINARY_FILENAME_PREFIX from line [%s]\n",
		 line );
	exit( -1 );
      }
      sprintf( binary_filename, "%s.%s", tmp,
	       ( params.do_average ? "avg-strategy" : "regrets" ) );

    } else {
      fprintf( stderr, "Can't parse line [%s] of player file [%s]\n",
	       line, player_file );
      exit( -1 );
    }
  }
  fclose( file );

  /* Initialize abstract game, rng from parameters */
  ag = new AbstractGame( params );
  init_by_array( &rng, params.rng_seeds, NUM_RNG_SEEDS );

  /* Time to load the binary file.  First, get the filesize */
  if( stat( binary_filename, &sb ) == -1 ) {
    fprintf( stderr, "Failed to get filesize of file [%s]\n", binary_filename );
    exit( -1 );
  }

  /* Now MMAP the entire file */
  file = fopen( binary_filename, "r" );
  if( file == NULL ) {
    fprintf( stderr, "Could not open binary file [%s]\n", binary_filename );
    exit( -1 );
  }
  dump_start = mmap( NULL, sb.st_size, PROT_READ, MAP_SHARED, fileno( file ), 0 );
  if( dump_start == MAP_FAILED ) {
    fprintf( stderr, "Error mapping binary file [%s]\n", binary_filename );
    exit( -1 );
  }
  fclose( file );
  void *dump = dump_start;

  /* Next, count the number of entries required per round to store the entries */
  size_t num_entries_per_bucket[ MAX_ROUNDS ];
  size_t total_num_entries[ MAX_ROUNDS ];
  memset( num_entries_per_bucket, 0,
	  MAX_ROUNDS * sizeof( num_entries_per_bucket[ 0 ] ) );
  memset( total_num_entries, 0, MAX_ROUNDS * sizeof( total_num_entries[ 0 ] ) );
  ag->count_entries( num_entries_per_bucket, total_num_entries );
  
  /* Finally, build the entries from the dump */
  for( int r = 0; r < MAX_ROUNDS; ++r ) {
    if( r < ag->game->numRounds ) {
      /* Establish entries for this round and move dump pointer to next set of
       * entries.
       */
      entries[ r ] = new_loaded_entries( num_entries_per_bucket[ r ],
					 total_num_entries[ r ],
					 &dump );
      if( entries[ r ] == NULL ) {
	fprintf( stderr, "Could not load entries for round %d\n", r );
	exit( -1 );
      }
    } else {
      /* Out of range */
      entries[ r ] = NULL;
    }
  }
}

PlayerModule::~PlayerModule( )
{
  /* Unmap the binary file */
  munmap( dump_start, sb.st_size );
  dump_start = NULL;
  for( int r = 0; r < ag->game->numRounds; ++r ) {
    entries[ r ] = NULL;
  }

  /* Destroy the abstract game */
  delete ag;
  ag = NULL;
}

void PlayerModule::get_action_probs( State &state,
				      double action_probs
				      [ MAX_ABSTRACT_ACTIONS ],
				      int bucket )
{
  /* Initialize action probs to the default in case we must abort early
   * for one of several reasons
   */  
  get_default_action_probs( state, action_probs );
  
  if( verbose ) {
    char tmp[ PATH_LENGTH ];
    printState( ag->game, &state, PATH_LENGTH, tmp );
    fprintf( stderr, "\nCurrent real state: %s\n", tmp );
  }

  /* Find the current node from the sequence of actions in state */
  const BettingNode *node = ag->betting_tree_root;
  State old_state;
  initState( ag->game, 0, &old_state );
  if( verbose ) {
    fprintf( stderr, "Translated abstract state: " );
  }
  for( int r = 0; r <= state.round; ++r ) {
    for( int a = 0; a < state.numActions[ r ]; ++a ) {
      const Action real_action = state.action[ r ][ a ];
      Action abstract_actions[ MAX_ABSTRACT_ACTIONS ];
      int num_actions = ag->action_abs->get_actions( ag->game, old_state,
						     abstract_actions );
      if( num_actions != node->get_num_choices( ) ) {
	if( verbose ) {
	  fprintf( stderr, "Number of actions %d does not match number "
		   "of choices %d\n", num_actions, node->get_num_choices( ) );
	}
	return;
      }
      int choice;
      if( ( ag->game->bettingType == noLimitBetting )
	  && ( real_action.type == a_raise ) ) {
	/* Need to translate raise action into a raise that we understand.
	 * What fun...
	 * For now, let's just use soft translation with geometric similarity as
	 * described by [Schnizlein, Bowling, and Szafron; IJCAI 2009],
	 * and let's just ignore any issues arising from repeated translation
	 * of successive decisions.  We'll also use stack ratios rather than
	 * pot fraction ratios just to keep life a little easier.  This is a
	 * fairly naive approach to translation and can likely be improved.
	 */

	/* First, find the smallest abstract raise greater than or equal to the
	 * real raise size (upper), and the largest abstract raise less than or
	 * equal to the real raise size (lower).
	 */
	int32_t lower = 0, upper = ag->game->stack[ node->get_player( ) ] + 1;
	int lower_choice = -1, upper_choice = -1;
	for( int i = 0; i < num_actions; ++i ) {
	  if( abstract_actions[ i ].type == a_raise ) {
	    if( ( abstract_actions[ i ].size <= real_action.size )
		&& ( abstract_actions[ i ].size >= lower ) ) {
	      lower = abstract_actions[ i ].size;
	      lower_choice = i;
	    }
	    if( ( abstract_actions[ i ].size >= real_action.size )
		&& ( abstract_actions[ i ].size <= upper ) ) {
	      upper = abstract_actions[ i ].size;
	      upper_choice = i;
	    }	    
	  }
	}

	/* 4 cases to consider depending on the lower and upper found above */
	if( lower == upper ) {
	  /* We have an exact match! */
	  choice = lower_choice; /* Should be the same as upper_choice */
	} else if( lower_choice == -1 ) {
	  /* No abstract raise less than or equal to real action raise */
	  if( upper_choice == -1 ) {
	    if( verbose ) {
	      fprintf( stderr, "Could not translate at round %d turn %d\n",
		       r, a );
	    }
	    return;
	  }
	  choice = upper_choice;
	} else if( upper_choice == -1 ) {
	  /* No abstract raise greater than or equal to real action raise */
	  if( lower_choice == -1 ) {
	    if( verbose ) {
	      fprintf( stderr, "Could not translate at round %d turn %d\n",
		       r, a );
	    }
	    return;
	  }
	  choice = lower_choice;
	} else {
	  /* Get similarity metric values for lower and upper raises */
	  double lower_sim
	    = ( ( 1.0 * lower / real_action.size ) - ( 1.0 * lower / upper ) )
	    / ( 1 - ( 1.0 * lower / upper ) );

	  double upper_sim
	    = ( ( 1.0 * real_action.size / upper ) - ( 1.0 * lower / upper ) )
	    / ( 1 - ( 1.0 * lower / upper ) );

	  /* Throw a dart and probabilistically choose lower or upper */
	  double dart = genrand_real2( &rng );
	  if( dart < ( lower_sim / ( lower_sim + upper_sim ) ) ) {
	    choice = lower_choice;
	  } else {
	    choice = upper_choice;
	  }
	}
	
      } else {
	/* Limit game or non-raise action. Just match the real action. */
	for( choice = 0; choice < num_actions; ++choice ) {
	  if( abstract_actions[ choice ].type == real_action.type ) {
	    break;
	  }
	}
	if( choice >= num_actions ) {
	  if( verbose ) {
	    fprintf( stderr, "Unable to translate action at round %d, turn %d; "
		     "actions available are:", r, a );
	    for( int i = 0; i < num_actions; ++i ) {
	      char action_str[ PATH_LENGTH ];
	      printAction( ag->game, &abstract_actions[ i ], PATH_LENGTH,
			   action_str );
	      fprintf( stderr, " %s", action_str );
	    }
	    fprintf( stderr, "\n" );
	  }
	  return;
	}
      }

      if( verbose ) {
	char action_str[ PATH_LENGTH ];
	printAction( ag->game, &abstract_actions[ choice ],
		     PATH_LENGTH, action_str );
	fprintf( stderr, " %s", action_str );
      }
      /* Move the current node and old_state along */
      node = node->get_child( );
      for( int i = 0; i < choice; ++i ) {
	node = node->get_sibling( );
	if( node == NULL ) {
	  if( verbose ) {
	    fprintf( stderr, "Ran out of siblings for choice %d\n", choice );
	  }
	  return;
	}
      }
      if( node->get_child( ) == NULL ) {
	if( verbose ) {
	  fprintf( stderr, " Abstract game over\n" );
	}
	return;
      }
      doAction( ag->game, &abstract_actions[ choice ], &old_state );
    }
  }

  /* Bucket the cards */
  if( bucket == -1 ) {
    bucket = ag->card_abs->get_bucket( ag->game, node,
				       state.boardCards, state.holeCards );
  }
  if( verbose ) {
    fprintf( stderr, " Bucket=%d\n", bucket );
  }

  /* Check for problems */
  if( currentPlayer( ag->game, &state ) != node->get_player( ) ) {
    if( verbose ) {
      fprintf( stderr, "Abstract player does not match current player\n" );
    }
    return;
  }
  if( state.round != node->get_round( ) ) {
    if( verbose ) {
      fprintf( stderr, "Abstract round does not match current round\n" );
    }
    return;
  }

  /* Get the positive entries at this information set */
  int num_choices = node->get_num_choices( );
  int64_t soln_idx = node->get_soln_idx( );
  int8_t round = node->get_round( );
  uint64_t pos_entries[ num_choices ];
  uint64_t sum_pos_entries = entries[ round ]->get_pos_values( bucket,
							       soln_idx,
							       num_choices,
							       pos_entries );

  /* Get the abstract game action probabilities */
  if( sum_pos_entries == 0 ) {
    if( verbose ) {
      fprintf( stderr, "ALL POSITIVE ENTRIES ARE ZERO\n" );
    }
    return;
  }
  memset( action_probs, 0, MAX_ABSTRACT_ACTIONS * sizeof( action_probs[ 0 ] ) );
  for( int c = 0; c < num_choices; ++c ) {
    action_probs[ c ] = 1.0 * pos_entries[ c ] / sum_pos_entries;
  }
}

Action PlayerModule::get_action( State &state )
{
  /* Get the abstract game action probabilities */
  double action_probs[ MAX_ABSTRACT_ACTIONS ];
  get_action_probs( state, action_probs );

  /* Get the corresponding actions */
  Action actions[ MAX_ABSTRACT_ACTIONS ];
  int num_choices = ag->action_abs->get_actions( ag->game, state, actions );
  if( verbose ) {
    fprintf( stderr, "probs:" );
    for( int a = 0; a < num_choices; ++a ) {
      if( ( num_choices < 5 ) || ( action_probs[ a ] > 0.001 ) ) {
	char action_str[ PATH_LENGTH ];
	printAction( ag->game, &actions[ a ], PATH_LENGTH, action_str );
	fprintf( stderr, " %lg%%%s", action_probs[ a ] * 100, action_str );
      }
    }
    fprintf( stderr, "\n" );
  }

  /* Choose an action */
  double dart = genrand_real2( &rng );
  int a;
  for( a = 0; a < num_choices - 1; ++a ) {
    if( dart < action_probs[ a ] ) {
      break;
    }
    dart -= action_probs[ a ];
  }
  if( verbose ) {
    char action_str[ PATH_LENGTH ];
    printAction( ag->game, &actions[ a ], PATH_LENGTH, action_str );
    fprintf( stderr, "Action %s chosen\n", action_str );
  }

  /* Make sure action is legal */
  if( !isValidAction( ag->game, &state, 1, &actions[ a ] ) ) {
    if( verbose ) {
      char action_str[ PATH_LENGTH ];
      printAction( ag->game, &actions[ a ], PATH_LENGTH, action_str );
      fprintf( stderr, "Action chosen is not legal, "
	       "now choosing fixed action %s instead\n", action_str );
    }
  }

  return actions[ a ];
}

void PlayerModule::get_default_action_probs( State &state,
					     double action_probs
					     [ MAX_ABSTRACT_ACTIONS ] ) const
{
  /* Default will be always call */
  
  memset( action_probs, 0, MAX_ABSTRACT_ACTIONS * sizeof( action_probs[ 0 ] ) );
  
  /* Get the abstract actions */
  Action actions[ MAX_ABSTRACT_ACTIONS ];
  int num_choices = ag->action_abs->get_actions( ag->game, state, actions );

  /* Find the call action */
  for( int a = 0; a < num_choices; ++a ) {
    if( actions[ a ].type == a_call ) {
      action_probs[ a ] = 1.0;
      return;
    }
  }

  /* Still haven't returned?  This means we couldn't find a call action,
   * so we must be dealing with a very weird action abstraction.
   * Let's just always play the first action then by default.
   */
  action_probs[ 0 ] = 1.0;
}

void print_player_file( const Parameters &params,
			const char *filename_prefix )
{
  /* Build the filename to print to */
  char player_filename[ PATH_LENGTH ];
  snprintf( player_filename, PATH_LENGTH, "%s.player", filename_prefix );

  /* Open the file */
  FILE *file = fopen( player_filename, "w" );
  if( file == NULL ) {
    fprintf( stderr, "Could not open player file [%s], skipping...\n",
	     player_filename );
    return;
  }

  /* Print the goods */
  params.print_params( file );
  fprintf( file, "BINARY_FILENAME_PREFIX %s\n", filename_prefix );
  fprintf( file, "PLAYER_END\n" );
  
  fclose( file );
}
