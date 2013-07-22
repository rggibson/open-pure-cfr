#ifndef __PURE_CFR_PURE_CFR_MACHINE_HPP__
#define __PURE_CFR_PURE_CFR_MACHINE_HPP__

/* pure_cfr_machine.hpp
 * Richard Gibson, Jun 28, 2013
 *
 * Stores regrets + average strategy for all players, stores the betting tree
 * and constains Pure CFR iterations routine.
 */

/* C / C++ / STL indluces */

/* project_acpc_server includes */
extern "C" {
#include "acpc_server_code/game.h"
#include "acpc_server_code/rng.h"
}

/* Pure CFR includes */
#include "parameters.hpp"
#include "betting_node.hpp"
#include "entries.hpp"
#include "constants.hpp"
#include "card_abstraction.hpp"
#include "action_abstraction.hpp"
#include "hand.hpp"

class PureCfrMachine {
public:
  
  PureCfrMachine( const Parameters &params );
  ~PureCfrMachine( );

  void do_iteration( rng_state_t &rng );
  
  /* Returns 0 on success, 1 on failure, -1 on warning */
  int write_dump( const char *dump_prefix, const bool do_regrets = true ) const;
  int load_dump( const char *dump_prefix ); 

protected:  
  int generate_hand( hand_t &hand, rng_state_t &rng );
  int walk_pure_cfr( const int position,
		     const BettingNode *cur_node,
		     const hand_t &hand,
		     rng_state_t &rng );

  const game *game;
  const CardAbstraction *card_abs;
  const ActionAbstraction *action_abs;
  
  BettingNode *betting_tree_root;

  const bool do_average;
  Entries *regrets[ MAX_ROUNDS ];
  Entries *avg_strategy[ MAX_ROUNDS ];
};

#endif
