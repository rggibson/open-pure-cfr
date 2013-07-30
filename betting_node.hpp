#ifndef __PURE_CFR_BETTING_NODE_HPP__
#define __PURE_CFR_BETTING_NODE_HPP__

/* betting_node.hpp
 * Richard Gibson, Jun 28, 2013
 * Email: richard.g.gibson@gmail.com
 *
 * Classes that represent nodes in the betting tree (game tree without cards).
 * These nodes are used as an alternative to passing an acpc_server state object
 * during the tree walk.  These nodes store information about how to evaluate
 * the game at terminal nodes so that this information only needs to be
 * computed once.
 *
 * Copyright (C) 2013 by Richard Gibson
 */

/* C / C++ / STL indluces */
#include <inttypes.h>
#include <assert.h>

/* C project_acpc_server indluces */
extern "C" {
#include "acpc_server_code/game.h"
}

/* Pure CFR includes */
#include "hand.hpp"
#include "constants.hpp"
#include "action_abstraction.hpp"


class BettingNode {
public:

  BettingNode( );
  virtual ~BettingNode( );

  virtual int evaluate( const hand_t &hand, const int position ) const
  { assert( 0 ); }

  virtual int64_t get_soln_idx( ) const { assert( 0 ); }
  virtual int get_num_choices( ) const { assert( 0 ); }
  virtual int8_t get_player( ) const { assert( 0 ); }
  virtual int8_t get_round( ) const { assert( 0 ); }
  virtual int8_t did_player_fold( const int position ) const { assert( 0 ); }

  virtual const BettingNode *get_child( ) const = 0;
  virtual BettingNode *get_sibling( ) const { return sibling; }
  virtual void set_sibling( BettingNode *new_sibling ) { sibling = new_sibling; }

protected:
  BettingNode *sibling; /* NULL if this is the last sibling */  
};

class TerminalNode2p : public BettingNode {
public:

  TerminalNode2p( const bool new_showdown, const int8_t new_fold_value[ 2 ], const int new_money );
  virtual ~TerminalNode2p( );

  virtual int evaluate( const hand_t &hand, const int position ) const;

  virtual BettingNode *get_child( ) const { return NULL; }

protected:
  const int8_t showdown; /* 0 = end by folding, 1 = end in showdown */
  int8_t fold_value[ 2 ]; /* (-1,1) if end by folding and (lose,win), 0 for showdown */
  const int money; /* amount of money changing hands at leaf */
};

class InfoSetNode2p : public BettingNode {
public:

  InfoSetNode2p( const int64_t new_soln_idx,
		 const int new_num_choices,
		 const int8_t new_player,
		 const int8_t new_round,
		 const BettingNode *new_child );
  virtual ~InfoSetNode2p( );

  virtual int64_t get_soln_idx( ) const { return soln_idx; }
  virtual int get_num_choices( ) const { return num_choices; }
  virtual int8_t get_player( ) const { return player; }
  virtual int8_t get_round( ) const { return round; }
  virtual int8_t did_player_fold( const int position ) const { return 0; }

  virtual const BettingNode *get_child( ) const { return child; }

protected:
  const int64_t soln_idx;
  const int num_choices;
  const int8_t player;
  const int8_t round;
  const BettingNode* child;
};

class TerminalNode3p : public BettingNode {
public:

  TerminalNode3p( const uint32_t new_pot_size,
		  const uint32_t new_money_spent[ MAX_PURE_CFR_PLAYERS ],
		  const leaf_type_t new_leaf_type );
  virtual ~TerminalNode3p( );

  virtual int evaluate( const hand_t &hand, const int position ) const;

  virtual const BettingNode *get_child( ) const { return NULL; }

protected:
  const uint32_t pot_size;
  uint32_t money_spent[ MAX_PURE_CFR_PLAYERS ];
  const leaf_type_t leaf_type;
};

/* InfoSetNode3p derives from TerminalNode3p since we want to terminate tree walks prematurely
 * before reaching a terminal node after the current player folds.
 */
class InfoSetNode3p : public TerminalNode3p {
public:

  InfoSetNode3p( const int64_t new_soln_idx,
		 const int new_num_choices,
		 const int8_t new_player,
		 const int8_t new_round,
		 const int8_t new_player_folded[ MAX_PURE_CFR_PLAYERS ],
		 const BettingNode *new_child,
		 const uint32_t new_pot_size,
		 const uint32_t new_money_spent[ MAX_PURE_CFR_PLAYERS ],
		 const leaf_type_t new_leaf_type );
  virtual ~InfoSetNode3p( );

  virtual int64_t get_soln_idx( ) const { return soln_idx; }
  virtual int get_num_choices( ) const { return num_choices; }
  virtual int8_t get_player( ) const { return player; }
  virtual int8_t get_round( ) const { return round; }
  virtual int8_t did_player_fold( const int position ) const
  { return player_folded[ position ]; }

  virtual const BettingNode *get_child( ) const { return child; }

protected:
  const int64_t soln_idx;
  const int num_choices;
  const int8_t player;
  const int8_t round;
  int8_t player_folded[ MAX_PURE_CFR_PLAYERS ];
  const BettingNode *child;
};

BettingNode *init_betting_tree_r( State &state,
				  const Game *game,
				  const ActionAbstraction *action_abs,
				  size_t num_entries_per_bucket[ MAX_ROUNDS ] );

void destroy_betting_tree_r( const BettingNode *node );

#endif
