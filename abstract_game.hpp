#ifndef __PURE_CFR_ABSTRACT_GAME_HPP__
#define __PURE_CFR_ABSTRACT_GAME_HPP__

/* abstract_game.hpp
 * Richard Gibson, Jul 26, 2013
 * Email: richard.g.gibson@gmail.com
 *
 * Wrapper class for game and abstraction classes. 
 */

/* C / C++ / STL indluces */

/* project_acpc_server includes */
extern "C" {
#include "acpc_server_code/game.h"
}

/* Pure CFR includes */
#include "parameters.hpp"
#include "card_abstraction.hpp"
#include "action_abstraction.hpp"
#include "betting_node.hpp"

class AbstractGame {
public:

  AbstractGame( const Parameters &params );
  ~AbstractGame( );

  void count_entries( size_t num_entries_per_bucket[ MAX_ROUNDS ],
		      size_t total_num_entries[ MAX_ROUNDS ] ) const;

  Game *game;

  const CardAbstraction *card_abs;
  const ActionAbstraction *action_abs;
  
  BettingNode *betting_tree_root;

protected:

  void count_entries_r( const BettingNode *node,
			size_t num_entries_per_bucket[ MAX_ROUNDS ],
			size_t total_num_entries[ MAX_ROUNDS ] ) const;
};

#endif
