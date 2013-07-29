#ifndef __PURE_CFR_CARD_ABSTRACTION_HPP__
#define __PURE_CFR_CARD_ABSTRACTION_HPP__

/* card_abstraction.hpp
 * Richard Gibson, Jun 28, 2013
 *
 * Home of the card_abstraction abstract class and all implementing classes
 */

/* C / C++ / STL indluces */

/* project_acpc_server includes */
extern "C" {
#include "acpc_server_code/game.h"
}

/* Pure CFR includes */
#include "constants.hpp"
#include "betting_node.hpp"

/* Base class */
class CardAbstraction {
public:

  CardAbstraction( );
  virtual ~CardAbstraction( );

  virtual int num_buckets( const Game *game, const BettingNode *node ) const = 0;
  virtual int get_bucket( const Game *game,
			  const BettingNode *node,
			  const uint8_t board_cards[ MAX_BOARD_CARDS ],
			  const uint8_t hole_cards[ MAX_PURE_CFR_PLAYERS ]
			  [ MAX_HOLE_CARDS ] ) const = 0;
  virtual bool can_precompute_buckets( ) const { return false; }
  virtual void precompute_buckets( const Game *game,
				   hand_t &hand ) const;

protected:
};

/* The null card abstraction treats every set of cards as its own bucket.
 * This is a naive abstraction that does not take suit isomorphism into account.
 * Further, more buckets are used than necessary as card removal and order
 * are ignored.
 */
class NullCardAbstraction : public CardAbstraction {
public:

  NullCardAbstraction( const Game *game );
  virtual ~NullCardAbstraction( );

  virtual int num_buckets( const Game *game, const BettingNode *node ) const;
  virtual int get_bucket( const Game *game,
			  const BettingNode *node,
			  const uint8_t board_cards[ MAX_BOARD_CARDS ],
			  const uint8_t hole_cards[ MAX_PURE_CFR_PLAYERS ]
			  [ MAX_HOLE_CARDS ] ) const;
  virtual bool can_precompute_buckets( ) const { return true; }
  virtual void precompute_buckets( const Game *game,
				   hand_t &hand ) const;

protected:
  virtual int get_bucket_internal( const Game *game,
				   const uint8_t board_cards[ MAX_BOARD_CARDS ],
				   const uint8_t hole_cards[ MAX_PURE_CFR_PLAYERS ]
				   [ MAX_HOLE_CARDS ],
				   const int player,
				   const int round ) const;
  
  const int deck_size;
  int m_num_buckets[ MAX_ROUNDS ];
};

/* The blind card abstraction treats every set of cards as the same.
 * This is a simple abstraction that enforces the agent to not look at its cards.
 */
class BlindCardAbstraction : public CardAbstraction {
public:

  BlindCardAbstraction( );
  virtual ~BlindCardAbstraction( );

  virtual int num_buckets( const Game *game, const BettingNode *node ) const;
  virtual int get_bucket( const Game *game,
			  const BettingNode *node,
			  const uint8_t board_cards[ MAX_BOARD_CARDS ],
			  const uint8_t hole_cards[ MAX_PURE_CFR_PLAYERS ]
			  [ MAX_HOLE_CARDS ] ) const;
  virtual bool can_precompute_buckets( ) const { return true; }
  virtual void precompute_buckets( const Game *game,
				   hand_t &hand ) const;
};

#endif
