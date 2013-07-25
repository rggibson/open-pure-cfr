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
#include <sys/time.h>
#include <assert.h>
#include <unistd.h>

/* C project-acpc-server includes */
extern "C" {
#include "acpc_server_code/rng.h"
}

/* Pure CFR includes */
#include "parameters.hpp"
#include "pure_cfr_machine.hpp"
#include "utility.hpp"

typedef struct {
  int64_t iterations;
  int seconds;
} pure_cfr_counter_t;

typedef struct {
  int thread_num;
  Parameters *params;
  PureCfrMachine *pcm;
  int64_t iterations;
  int *do_pause;
  int am_paused;
  int *do_quit;
} worker_thread_args_t;

pthread_attr_t thread_attributes;

void print_player_file( const Parameters &params, const char *filename_prefix )
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
  
  fclose( file );
}

void init_pure_cfr_counter( pure_cfr_counter_t &counter )
{
  counter.iterations = 0;
  counter.seconds = 0;
}

/* Return 0 on success, 1 on failure */
int set_pure_cfr_counter( const char *load_dump_prefix,
			  pure_cfr_counter_t &counter )
{
  char temp[ 100 ];

  const char *ptr = load_dump_prefix;
  while( ptr[ 0 ] != '\0' ) {
    if( sscanf( ptr, "iter-%100[^.]", temp ) == 1 ) {
      if( strtoint64_units( temp, counter.iterations ) ) {
	fprintf( stderr, "could not translate iterations count from %s\n",
		 load_dump_prefix );
	return 1;
      }
    }
    sscanf( ptr, "secs-%d", &counter.seconds );

    while( ( ptr[ 0 ] != '.' )
	   && ( ptr[ 0 ] != '\0' ) ) {
      ptr += 1;
    }
    if( ptr[ 0 ] != '\0' ) {
      ptr += 1;
    }
  }

  if( ( counter.iterations <= 0 )
      || ( counter.seconds <= 0 ) ) {
    fprintf( stderr, "could not find iterations or seconds count in "
	     "prefix [%s]\n", load_dump_prefix );
    return 1;
  }

  return 0;
}

void *thread_iterations( void *thread_args )
{
  worker_thread_args_t *args = ( worker_thread_args_t * ) thread_args;

  /* Initialize RNG using this crazy array because why not,
   * and we ensure that the seeds are different for each thread
   */
  rng_state_t rng;
  uint32_t seeds[ 4 ];
  for( int i = 0; i < 4; ++i ) {
    seeds[ i ] = args->params->rng_seeds[ i ] + 1234 + 4 * args->thread_num + i;
  }
  init_by_array( &rng, seeds, 4 );

  while( true ) {

    /* Have we been told to pause? */
    if( *args->do_pause && !( *args->do_quit ) ) {
      /* Yes, so let's pause and wait until we are no longer told to pause */
      args->am_paused = 1;
      while( *args->do_pause && !( *args->do_quit ) ) {
	sleep( 1 );
      }
      args->am_paused = 0;
    }

    /* Time to quit? */
    if( *args->do_quit ) {
      /* Yes, so quit */
      args->am_paused = 1;
      break;
    }

    /* Run a block of iterations */
    for( int i = 0; i < ITERATION_BLOCK_SIZE; ++i ) {
      args->pcm->do_iteration( rng );
    }
    args->iterations += ITERATION_BLOCK_SIZE;
  }
  
  pthread_exit( NULL );
}

void run_iterations( Parameters &params, PureCfrMachine &pcm )
{
  int do_pause = 0;
  int do_quit = 0;

  /* Record the time we started */
  struct timeval absolute_start_time;
  gettimeofday( &absolute_start_time, NULL );

  pure_cfr_counter_t initial_counts;
  init_pure_cfr_counter( initial_counts );  

  if( params.load_dump ) {
    /* Load dump */
    if( set_pure_cfr_counter( params.load_dump_prefix, initial_counts ) ) {
      /* Failed to parse counter info from dump; exit */
      return;
    }
    fprintf( stderr, "Loading dump [%s]... ", params.load_dump_prefix );
    if( pcm.load_dump( params.load_dump_prefix ) > 0 ) {
      /* Failed to load dump; exit */
      return;
    }
    fprintf( stderr, "done!\n\n" );
  }

  /* Set up threads */
  worker_thread_args_t thread_args[ params.num_threads ];
  pthread_t threads[ params.num_threads ];
  for( int i = 0; i < params.num_threads; ++i ) {
    thread_args[ i ].thread_num = i;
    thread_args[ i ].params = &params;
    thread_args[ i ].pcm = &pcm;
    thread_args[ i ].iterations = 0;
    thread_args[ i ].do_pause = &do_pause;
    thread_args[ i ].am_paused = 0;
    thread_args[ i ].do_quit = &do_quit;
  }

  /* Launch threads */
  for( int i = 0; i < params.num_threads; ++i ) {
    int status = pthread_create( &threads[ i ],
				 &thread_attributes,
				 thread_iterations,
				 &thread_args[ i ] );
    if( status ) {
      fprintf( stderr, "Couldn't launch worker thread %d, status = %d\n",
	       i, status );
      exit( -1 );
    }
  }
  
  /* Get the current time */
  struct timeval start_time;
  gettimeofday( &start_time, NULL );
  
  /* Counter to keep track of the last time we printed a status update */
  pure_cfr_counter_t last_status_counter;
  last_status_counter.seconds = 0;
  last_status_counter.iterations = initial_counts.iterations;

  /* Keep track of the last time we dumped a checkpoint */
  int next_dump_seconds = params.dump_timer.seconds_start;;
  while( initial_counts.seconds >= next_dump_seconds ) {
    next_dump_seconds = next_dump_seconds * params.dump_timer.seconds_mult
      + params.dump_timer.seconds_add;
  }

  /* Variable to keep track of how much time is spent dumping files to disk */
  int dumping_secs = 0;

  while( !do_quit ) {
    
    /* Sleep a second so that we don't busy-wait */
    sleep( 1 );

    /* Get the current time */
    struct timeval cur_time;
    gettimeofday( &cur_time, NULL );

    /* Is it time to quit? */
    do_quit = ( cur_time.tv_sec - absolute_start_time.tv_sec
		>= params.max_walltime_seconds );
    
    /* Get the number of iterations completed */
    int64_t iterations_complete = initial_counts.iterations;
    for( int t = 0; t < params.num_threads; ++t ) {
      iterations_complete += thread_args[ t ].iterations;
    }

    /* Get the total amount of time we've been doing work */
    int work_seconds = initial_counts.seconds + cur_time.tv_sec
      - start_time.tv_sec - dumping_secs;

    /* Is it time to print status? */
    if( cur_time.tv_sec - last_status_counter.seconds
	>= params.status_freq_seconds ) {
      
      /* Yes, print status */
      double overall_speed = ( 1.0 * iterations_complete ) / work_seconds;
      if( last_status_counter.seconds > 0 ) {
	double recent_speed = ( 1.0 * ( iterations_complete
					- last_status_counter.iterations ) )
	  / ( cur_time.tv_sec - last_status_counter.seconds );
	fprintf( stderr, "%jd iterations complete; %lg i/s overall, "
		 "%lg i/s recent\n",
		 ( intmax_t ) iterations_complete, overall_speed, recent_speed );
      } else {
	fprintf( stderr, "%jd iterations complete; %lg i/s overal\n",
		 ( intmax_t ) iterations_complete, overall_speed );
      }
      char temp[ 100 ];
      time_seconds_to_string( next_dump_seconds - work_seconds, temp, 100 );
      fprintf( stderr, "%s until next checkpoint\n", temp );
      time_seconds_to_string( params.max_walltime_seconds -
			      ( cur_time.tv_sec - absolute_start_time.tv_sec ),
			      temp, 100 );
      fprintf( stderr, "%s until quit\n", temp );

      /* Update status counter */
      last_status_counter.seconds = cur_time.tv_sec;
      last_status_counter.iterations = iterations_complete;
      fprintf( stderr, "\n" );
    }

    /* Is it time to checkpoint? */
    if( ( work_seconds >= next_dump_seconds ) || do_quit ) {
      /* Yes, dump a checkpoint */

      /* First, pause the threads */
      do_pause = 1;
      fprintf( stderr, "Pause initiated to begin dump\n" );
      fprintf( stderr, "Number of threads paused:" );
      int num_paused;
      do {
	sleep( 1 );
	num_paused = 0;
	for( int t = 0; t < params.num_threads; ++t ) {
	  num_paused += thread_args[ t ].am_paused;
	}
	fprintf( stderr, " %d", num_paused );
      } while( num_paused < params.num_threads );
      fprintf( stderr, "\n" );

      /* Record time dump started */
      struct timeval dump_start_time;
      gettimeofday( &dump_start_time, NULL );

      /* Build the filename */
      iterations_complete = initial_counts.iterations;
      for( int t = 0; t < params.num_threads; ++t ) {
	iterations_complete += thread_args[ t ].iterations;
      }
      char filename[ PATH_LENGTH ];
      char iterations_str[ PATH_LENGTH ];
      int64tostr_units( iterations_complete, iterations_str, PATH_LENGTH );
      work_seconds = initial_counts.seconds + cur_time.tv_sec
	- start_time.tv_sec - dumping_secs;
      snprintf( filename, PATH_LENGTH, "%s.iter-%s.secs-%d", params.output_prefix,
		iterations_str, work_seconds );
      print_player_file( params, filename );

      fprintf( stderr, "Checkpointing files with prefix [%s]... ", filename );
      pcm.write_dump( filename );
      fprintf( stderr, "done!\n" );

      /* Unpause the threads */
      do_pause = 0;
      fprintf( stderr, "Pause released\n\n" );

      /* How much time was spent dumping? */
      struct timeval dump_end_time;
      gettimeofday( &dump_end_time, NULL );
      dumping_secs += dump_end_time.tv_sec - dump_start_time.tv_sec;

      /* Update the next dump */
      while( work_seconds >= next_dump_seconds ) {
	next_dump_seconds = next_dump_seconds * params.dump_timer.seconds_mult
	  + params.dump_timer.seconds_add;
      }
    }
  }

  /* Wait for threads to return */
  for( int i = 0; i < params.num_threads; ++i ) {
    fprintf( stderr, "Waiting for thread %d to finish\n", i );
    int status = pthread_join( threads[ i ], NULL );
    if( status ) {
      fprintf( stderr, "Couldn't join to thread %d, status = %d\n", i, status );
    }
  }

  fprintf( stderr, "\nAll Dun :)\n" );
}

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

  /* Increase thread stack size */
  pthread_attr_init( &thread_attributes );
  pthread_attr_setstacksize( &thread_attributes, 8192 * 1024 );
  
  /* Turn control over to the main loop */
  run_iterations( params, pcm );

  
  /* TEST CODE BELOW */
  // rng_state_t rng;
  // init_by_array( &rng, params.rng_seeds, 4 );

  // pcm.load_dump( params.output_prefix );
  // pcm.do_iteration( rng );
  
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
