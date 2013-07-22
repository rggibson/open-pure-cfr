/* card_abstraction.cpp
 * Richard Gibson, Jun 28, 2013
 *
 * Home of the card_abstraction abstract class and all implementing classes
 */

/* C / C++ / STL indluces */

/* project_acpc_server includes */
extern "C" {
}

/* Pure CFR includes */
#include "card_abstraction.hpp"

CardAbstraction::CardAbstraction( )
{
}

CardAbstraction::~CardAbstraction( )
{
}

/* By default, assume cannot precompute buckets */
void CardAbstraction::precompute_buckets( const Game *game, hand_t &hand ) const
{
  for( int p = 0; p < MAX_PURE_CFR_PLAYERS; ++p ) {
    for( int r = 0; r < MAX_ROUNDS; ++r ) {
      hand.precomputed_buckets[ p ][ r ] = -1;
    }
  }
}

void CardAbstraction::count_total_num_entries( const Game *game,
					       const BettingNode *node,
					       size_t total_num_entries[ MAX_ROUNDS ] ) const
{
  const BettingNode *child = node->get_child( );

  if( child == NULL ) {
    /* Terminal node */
    return;
  }

  /* Update entries counts */
  const int buckets = num_buckets( game, node );
  total_num_entries[ node->round ] += buckets * node->num_choices;

  /* Recurse */
  for( int c = 0; c < node->num_choices; ++c ) {
    count_total_num_entries( game, child, total_num_entries );
    child = child->get_sibling( );
  }
}

NullCardAbstraction::NullCardAbstraction( const Game *game )
  : deck_size( game->numSuits * game->numRanks )
{
  /* Precompute number of buckets per round */
  m_num_buckets[ 0 ] = 1;
  for( int i = 0; i < game->numHoleCards; ++i ) {
    m_num_buckets[ 0 ] *= deck_size;
  }
  for( int r = 0; r < MAX_ROUNDS; ++r ) {
    if( r < game->numRounds ) {
      if( r > 0 ) {
	m_num_buckets[ r ] = m_num_buckets[ r - 1 ];
      }
      for( int i = 0; i < game->numBoardCards[ r ]; ++i ) {
	m_num_buckets[ r ] *= deck_size;
      }
    } else {
      m_num_buckets[ r ] = 0;
    }
  }
}

NullCardAbstraction::~NullCardAbstraction( )
{
}

int NullCardAbstraction::num_buckets( const Game *game,
				      const BettingNode *node ) const
{
  return m_num_buckets[ node->get_round() ];
}

int NullCardAbstraction::get_bucket( const Game *game,
				     const BettingNode *node,
				     const hand_t &hand ) const
{
  return get_bucket_internal( game, hand, node->get_player(), state.round );
}

void NullCardAbstraction::precompute_buckets( const Game *game,
					      hand_t &hand ) const
{
  for( int p = 0; p < game->numPlayers; ++p ) {
    for( int r = 0; r < game->numRounds; ++r ) {
      hand.precomputed_buckets[ p ][ r ] = get_bucket_internal( game, hand, p, r );
    }
  }
}

int NullCardAbstraction::get_bucket_internal( const Game *game,
					      const hand_t &hand,
					      const int player,
					      const int round ) const
{
  /* Calculate the unique bucket number for this hand */
  int bucket = 0;
  for( int i = 0; i < game->numHoleCards; ++i ) {
    if( i > 0 ) {
      bucket *= deck_size;
    }
    bucket += hand.holeCards[ player ][ i ];
  }
  for( int r = 0; r <= round; ++r ) {
    for( int i = bcStart( game, r ); i < sumBoardCards( game, r ); ++i ) {
      bucket *= deck_size;
      bucket += hand.boardCards[ i ];
    }
  }

  return bucket;
}

BlindCardAbstraction::BlindCardAbstraction( )
{
}

BlindCardAbstraction::~BlindCardAbstraction( )
{
}

int BlindCardAbstraction::num_buckets( const Game *game,
				       const BettingNode *node ) const
{
  return 1;
}

int BlindCardAbstraction::get_bucket( const Game *game,
				      const BettingNode *node,
				      const hand_t &hand ) const
{
  return 0;
}

void BlindCardAbstraction::precompute_buckets( const Game *game, hand_t &hand ) const
{
  for( int p = 0; p < game->numPlayers; ++p ) {
    for( int r = 0; r < game->numRounds; ++r ) {
      hand.precomputed_buckets[ p ][ r ] = 0;
    }
  }
}
