#ifndef __PURE_CFR_PARAMETERS_HPP__
#define __PURE_CFR_PARAMETERS_HPP__

/* parameters.hpp
 * Richard Gibson, Jun 28, 2013
 * Email: richard.g.gibson@gmail.com
 *
 * The Parameters class parses command line arguments and stores the parameter
 * options selected.
 *
 * Copyright (C) 2013 by Richard Gibson
 */

/* C / C++ / STL includes */
#include <stdio.h>
#include <inttypes.h>

/* C project-acpc-server includes */
extern "C" {
#include "acpc_server_code/game.h"
}

/* Pure CFR includes */
#include "constants.hpp"

const int NUM_RNG_SEEDS = 4;

typedef struct {
  int seconds_start;
  int seconds_mult;
  int seconds_add;
} output_timer_t;

class Parameters {
public:
  Parameters();
  virtual ~Parameters();

  virtual void print_usage( const char *prog_name ) const;
  virtual int parse( const int argc, const char *argv[] );
  virtual void print_params( FILE *file ) const;
  virtual int read_params( FILE *file );

  /* Required parameters */
  char game_file[ PATH_LENGTH ];
  char output_prefix[ PATH_LENGTH ];

  /* Optional parameters */
  uint32_t rng_seeds[ NUM_RNG_SEEDS ];
  card_abs_type_t card_abs_type;
  action_abs_type_t action_abs_type;
  bool load_dump;
  char load_dump_prefix[ PATH_LENGTH ];
  int num_threads;
  int status_freq_seconds;
  output_timer_t dump_timer;
  int max_walltime_seconds;
  bool do_average;
};

#endif
