#ifndef __PURE_CFR_PURE_CFR_PLAYER_HPP__
#define __PURE_CFR_PURE_CFR_PLAYER_HPP__

/* pure_cfr_player.hpp
 * Richard Gibson, Jul 26, 2013
 * Email: richard.g.gibson@gmail.com
 *
 * Player class that provides an interface for actually playing
 * poker with a dealer and looking up action probabilities.
 */

/* C / C++ / STL indluces */
#include <sys/stat.h>

/* project_acpc_server includes */
extern "C" {
#include "acpc_server_code/game.h"
#include "acpc_server_code/rng.h"
}

/* Pure CFR includes */
#include "parameters.hpp"
#include "abstract_game.hpp"
#include "entries.hpp"

class PureCfrPlayer {
public:

  PureCfrPlayer( const char *player_file );
  ~PureCfrPlayer( );

  virtual const AbstractGame *get_abstract_game( ) const { return ag; }

  virtual void get_action_probs( State &state,
				 double action_probs
				 [ MAX_ABSTRACT_ACTIONS ],
				 int bucket = -1 );
  virtual Action get_action( State &state );

protected:

  virtual void get_default_action_probs( State &state,
					 double action_probs
					 [ MAX_ABSTRACT_ACTIONS ] ) const;
  
  const AbstractGame *ag;
  rng_state_t rng;
  bool verbose;
  Entries *entries[ MAX_ROUNDS ];
  struct stat sb;
  void *dump_start;
};

void print_player_file( const Parameters &params,
			const char *filename_prefix );

#endif
