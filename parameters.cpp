/* parameters.cpp
 * Richard Gibson, Jun 28, 2013
 * Email: richard.g.gibson@gmail.com
 *
 * Parse command line arguments and store options selected.
 */

/* C / C++ / STL includes */
#include <string.h>
#include <limits.h>
#include <time.h>

/* Pure CFR includes */
#include "parameters.hpp"
#include "utility.hpp"

Parameters::Parameters( )
{
  /* Set optional parameters to defaults */
  load_dump = false;
  card_abs_type = CARD_ABS_NULL;
  action_abs_type = ACTION_ABS_NULL;
  rng_seeds[ 0 ] = 6;
  rng_seeds[ 1 ] = 12;
  rng_seeds[ 2 ] = 1983;
  rng_seeds[ 3 ] = 28;
  num_threads = 1;
  status_freq_seconds = 60;
  dump.seconds_start = INT_MAX;
  dump.seconds_mult = 1;
  dump.seconds_add = 0;
  max_walltime_seconds = INT_MAX;
  do_average = true;
}

Parameters::~Parameters( )
{
}

void Parameters::print_usage( const char *prog_name ) const
{
  char status_freq_seconds_str[ PATH_LENGTH ];
  time_seconds_to_string( status_freq_seconds, status_freq_seconds_str,
			  PATH_LENGTH );
  fprintf( stderr, "Usage: %s <game> <output_prefix> [options]\n", prog_name );
  fprintf( stderr, "Options:\n" );
  fprintf( stderr, "  --rng=<seed1:seed2:seed3:seed4|TIME>  "
	   "(default: %d:%d:%d:%d)\n",
	   rng_seeds[ 0 ], rng_seeds[ 1 ], rng_seeds[ 2 ], rng_seeds[ 3 ] );
  fprintf( stderr, "  --card-abs={" );
  for( int i = 0; i < NUM_CARD_ABS_TYPES; ++i ) {
    if( i > 0 ) {
      fprintf( stderr, "|" );
    }
    fprintf( stderr, "%s", card_abs_type_to_str[ i ] );
  }
  fprintf( stderr, "}  (default: %s)\n", card_abs_type_to_str[ card_abs_type ] );
  fprintf( stderr, "  --action-abs={" );
  for( int i = 0; i < NUM_ACTION_ABS_TYPES; ++i ) {
    if( i > 0 ) {
      fprintf( stderr, "|" );
    }
    fprintf( stderr, "%s", action_abs_type_to_str[ i ] );
  }
  fprintf( stderr, "}  (default: %s)\n",
	   action_abs_type_to_str[ card_abs_type ] );
  fprintf( stderr, "  --load-dump=<dump_prefix>\n" );
  fprintf( stderr, "  --threads=<num_threads>  (default: %d)\n", num_threads );
  fprintf( stderr, "  --status=<dd:hh:mm:ss>  (default: %s)\n",
	   status_freq_seconds_str );
  fprintf( stderr, "  --checkpoint=<start_time[,mult_time[,add_time]]>\n" );
  fprintf( stderr, "  --max-walltime=<dd:hh:mm:ss>\n" );
  fprintf( stderr, "  --no-average\n" );
}

int Parameters::parse( const int argc, const char *argv[] )
{
  int index = 1;

  if( argc < 3 ) {
    print_usage( argv[ 0 ] );
    return 1;
  }

  /* game */
  strncpy( game_file, argv[ index ], PATH_LENGTH );
  ++index;
  
  /* output prefix */
  strncpy( output_prefix, argv[ index ], PATH_LENGTH );
  ++index;

  /* optional parameters */
  bool rng_set = false;
  for( ; index < argc; ++index ) {
    
    if( !strncmp( argv[ index ], "--rng=", strlen( "--rng=" ) ) ) {
      if( strcmp( &argv[ index ][ strlen( "--rng=" ) ], "TIME" ) == 0 ) {
	for( int j = 0; j < 4; ++j ) {
	  rng_seeds[ j ] = time( NULL ) + 4*j + 1;
	}
      } else if( sscanf( &argv[ index ][ strlen( "--rng=" ) ], "%d:%d:%d:%d",
			 &rng_seeds[ 0 ], &rng_seeds[ 1 ],
			 &rng_seeds[ 2 ], &rng_seeds[ 3 ] ) < 4 ) {
	fprintf( stderr, "Could not read in 4 integer seeds from [%s]\n",
		 argv[ index ] );
	return 1;
      }
      rng_set = true;      
      
    } else if( !strncmp( argv[ index ], "--card-abs=",
			 strlen( "--card-abs=" ) ) ) {
      const char *abs_str = &argv[ index ][ strlen( "--card-abs=" ) ];
      int i;
      for( i = 0; i < NUM_CARD_ABS_TYPES; ++i ) {
	if( !strcmp( abs_str, card_abs_type_to_str[ i ] ) ) {
	  card_abs_type = ( card_abs_type_t ) i;
	  break;
	}
      }
      if( i >= NUM_CARD_ABS_TYPES ) {
	fprintf( stderr, "Could not parse card abstraction type [%s]\n",
		 abs_str );
	return 1;
      }

    } else if( !strncmp( argv[ index ], "--action-abs=",
			 strlen( "--action-abs=" ) ) ) {
      const char *abs_str = &argv[ index ][ strlen( "--action-abs=" ) ];
      int i;
      for( i = 0; i < NUM_ACTION_ABS_TYPES; ++i ) {
	if( !strcmp( abs_str, action_abs_type_to_str[ i ] ) ) {
	  action_abs_type = ( action_abs_type_t ) i;
	  break;
	}
      }
      if( i >= NUM_ACTION_ABS_TYPES ) {
	fprintf( stderr, "Could not parse action abstraction type [%s]\n",
		 abs_str );
	return 1;
      }

    } else if( !strncmp( argv[ index ], "--load-dump=",
			 strlen( "--load-dump=" ) ) ) {
      strncpy( load_dump_prefix, &argv[ index ][ strlen( "--load-dump=" ) ], PATH_LENGTH );
      load_dump = true;
      if( !rng_set ) {
	/* We should use different random seeds than we used previously
	   for a restarted run */
	for( int j = 0; j < 4; ++j ) {
	  rng_seeds[ j ] = time( NULL ) + 4*j + 1;
	}
	rng_set = true;
      }
      
    } else if( !strncmp( argv[ index ], "--threads=", strlen( "--threads=" ) ) ) {
      if( sscanf( &argv[ index ][ strlen( "--threads=" ) ], "%d",
    		  &num_threads ) < 1 ) {
    	fprintf( stderr, "could not read number of threads from [%s]\n", argv[ index ] );
    	return 1;
      }

    } else if( !strncmp( argv[ index ], "--status=", strlen( "--status=" ) ) ) {
      status_freq_seconds = time_string_to_seconds( &argv[ index ][ strlen( "--status=" ) ] );
      if( status_freq_seconds <= 0 ) {
    	fprintf( stderr, "could not read status frequency from [%s]\n", argv[ index ] );
    	return 1;
      }

    } else if( !strncmp( argv[ index ], "--checkpoint=", strlen( "--checkpoint=" ) ) ) {
      char temp[ 3 ][ 100 ];
      int num_args = sscanf( &argv[ index ][ strlen( "--checkpoint=" ) ],
			     "%100[^,],%100[^,],%100[^,]", temp[ 0 ], temp[ 1 ], temp[ 2 ] );
      if( num_args < 1 ) {
	fprintf( stderr, "could not read checkpoint time from [%s]\n", argv[ index ] );
	return 1;
      }
      dump.seconds_start = time_string_to_seconds( temp[ 0 ] );
      if( num_args > 1 ) {
	dump.seconds_mult = time_string_to_seconds( temp[ 1 ] );
	if( num_args > 2 ) {
	  dump.seconds_add = time_string_to_seconds( temp[ 2 ] );
	} else {
	  dump.seconds_add = 0;
	}
      } else {
	dump.seconds_mult = 1;
	dump.seconds_add = dump.seconds_start;
      }
      
    } else if( !strncmp( argv[ index ], "--max-walltime=", strlen( "--max-walltime=" ) ) ) {
      max_walltime_seconds = time_string_to_seconds( &argv[ index ][ strlen( "--max-walltime=" ) ] );
      if( max_walltime_seconds <= 0 ) {
    	fprintf( stderr, "could not read max walltime from [%s]\n", argv[ index ] );
    	return 1;
      }
    } else if( !strncmp( argv[ index ], "--no-average", strlen( "--no-average" ) ) ) {
      do_average = false;

    } else {
      fprintf( stderr, "unknown option [%s]\n", argv[ index ] );
      return 1;
    }
  }
  
  /* all done */
  return 0;
}

void Parameters::print_params( FILE *file ) const
{
  fprintf( file, "GAME_FILE %s\n", game_file );
  fprintf( file, "OUTPUT_PREFIX %s\n", output_prefix );
  fprintf( file, "RNG_SEEDS %zu %zu %zu %zu\n", rng_seeds[ 0 ], rng_seeds[ 1 ],
	   rng_seeds[ 2 ], rng_seeds[ 3 ] );
  fprintf( file, "CARD_ABSTRACTION %s\n", card_abs_type_to_str[ card_abs_type ] );
  fprintf( file, "ACTION_ABSTRACTION %s\n",
	   action_abs_type_to_str[ action_abs_type ] );
  if( load_dump ) {
    fprintf( file, "LOAD_DUMP_PREFIX %s\n", load_dump_prefix );
  }
  fprintf( file, "NUM_THREADS %d\n", num_threads );
  fprintf( file, "STATUS_FREQ_SECONDS %d\n", status_freq_seconds );
  fprintf( file, "DUMP_TIMER %d %d %d\n", dump.seconds_start, dump.seconds_mult, dump.seconds_add );
  fprintf( file, "MAX_WALLTIME_SECONDS %d\n", max_walltime_seconds );
  if( do_average ) {
    fprintf( file, "DO_AVERAGE TRUE\n" );
  } else {
    fprintf( file, "DO_AVERAGE FALSE\n" );
  }
}
