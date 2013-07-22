/* betting_node.cpp
 * Richard Gibson, Jun 28, 2013
 *
 * Constructors and evaluation methods for betting nodes.
 */

/* C / C++ / STL includes */
#include <string.h>

/* Pure CFR includes */
#include "betting_node.hpp"

BettingNode::BettingNode( )
{
  sibling = NULL;
}

BettingNode::~BettingNode( )
{
}

TerminalNode2p::TerminalNode2p( const bool new_showdown,
				const int8_t new_fold_value[ 2 ],
				const int new_money )
  : BettingNode( ),
    showdown( new_showdown ),
    money( new_money )
{
  fold_value[ 0 ] = new_fold_value[ 0 ];
  fold_value[ 1 ] = new_fold_value[ 1 ];
}

TerminalNode2p::~TerminalNode2p( )
{
}

int TerminalNode2p::evaluate( const hand_t *hand, const int position ) const
{
  return ( showdown ? hand->eval.showdown_value_2p[ position ] : fold_value[ position ] ) * money;
}

InfoSetNode2p::InfoSetNode2p( const int64_t new_soln_idx,
			      const int new_num_choices,
			      const int8_t new_player,
			      const int8_t new_round,
			      const BettingNode *new_child )
  : BettingNode( ),
    soln_idx( new_soln_idx ),
    num_choices( new_num_choices ),
    player( new_player ),
    round( new_round ),
    child( new_child )
{
}

InfoSetNode2p::~InfoSetNode2p( )
{
}

TerminalNode3p::TerminalNode3p( const uint32_t new_pot_size,
				const uint32_t new_money_spent[ MAX_PURE_CFR_PLAYERS ],
				const leaf_type_t new_leaf_type )
  : BettingNode( ),
    pot_size( new_pot_size ),
    leaf_type( new_leaf_type )
{
  memcpy( money_spent, new_money_spent, MAX_PURE_CFR_PLAYERS * sizeof( money_spent[ 0 ] ) );
}

TerminalNode3p::~TerminalNode3p( )
{
}

int TerminalNode3p::evaluate( const hand_t *hand, const int position ) const
{
  return ( pot_size / hand->eval.pot_frac_recip[ position ][ leaf_type ] ) - money_spent[ position ];
}

InfoSetNode3p::InfoSetNode3p( const int64_t new_soln_idx,
			      const int new_num_choices,
			      const int8_t new_player,
			      const int8_t new_round,
			      const int8_t new_player_folded[ MAX_PURE_CFR_PLAYERS ],
			      const BettingNode *new_child,
			      const uint32_t new_pot_size,
			      const uint32_t new_money_spent[ MAX_PURE_CFR_PLAYERS ],
			      const leaf_type_t new_leaf_type )
  : TerminalNode3p( new_pot_size, new_money_spent, new_leaf_type ),
    soln_idx( new_soln_idx ),
    num_choices( new_num_choices ),
    player( new_player ),
    round( new_round ),
    child( new_child )
{
  memcpy( player_folded, new_player_folded, MAX_PURE_CFR_PLAYERS * sizeof( player_folded[ 0 ] ) );
}

InfoSetNode3p::~InfoSetNode3p( )
{
}

void get_term_values_3p( const State &state,
			 const Game *game,
			 uint32_t &pot_size,
			 uint32_t money_spent[ MAX_PURE_CFR_PLAYERS ],
			 leaf_type_t &leaf_type )
{
  for( int p = 0; p < game->numPlayers; ++p ) {
    money_spent[ p ] = state.spent[ p ];
    pot_size += money_spent[ p ];
  }

  /* Leaf type, which is 3p-specific */
  if( state.playerFolded[ 1 ] && state.playerFolded[ 2 ] ) {
    leaf_type = LEAF_P0;
  } else if( state.playerFolded[ 0 ] && state.playerFolded[ 2 ] ) {
    leaf_type = LEAF_P1;
  } else if( state.playerFolded[ 0 ] && state.playerFolded [ 1 ] ) {
    leaf_type = LEAF_P2;
  } else if( state.playerFolded[ 2 ] ) {
    leaf_type = LEAF_P0_P1;
  } else if( state.playerFolded[ 1 ] ) {
    leaf_type = LEAF_P0_P2;
  } else if( state.playerFolded[ 0 ] ) {
    leaf_type = LEAF_P1_P2;
  } else {
    leaf_type = LEAF_P0_P1_P2;
  }
}

BettingNode *init_betting_tree_r( State &state,
				  const Game *game,
				  const ActionAbstraction *acttion_abs,
				  size_t num_entries_per_bucket[ MAX_ROUNDS ] )
{
  BettingNode *node;
  
  if( state.finished ) {
    /* Terminal node */
    switch( game->numPlayers ) {
      
    case 2: {
      int8_t showdown = ( state.playerFolded[ 0 ] || state.playerFolded[ 1 ] ? 0 : 1 );
      int8_t fold_value[ 2 ];
      int money = -1;
      for( int p = 0; p < 2; ++p ) {
        if( state.playerFolded[ p ] ) {
	  fold_value[ p ] = -1;
	  money = state.spent[ p ];
	} else if( state.playerFolded[ !p ] ) {
	  fold_value[ p ] = 1;
	  money = state.spent[ !p ];
	} else {
	  fold_value[ p ] = 0;
	  money = state.spent[ p ];
	}
      }
      node = new TerminalNode2p( showdown, fold_value, money );
      break;
    }

    case 3: {
      uint32_t pot_size;
      uint32_t money_spent[ MAX_PURE_CFR_PLAYERS ];
      leaf_type_t leaf_type;
      get_term_values_3p( state, game, pot_size, money_spent, leaf_type );
      node = new TerminalNode3p( pot_size, money_spent, leaf_type );
      break;
    }
    
    default:
      fprintf( stderr, "cannot initialize betting tree for %d-players\n", game->numPlayers );
      assert( 0 );
    }
    
    return node;
  }

  /* Choice node.  First, compute number of different allowable actions */
  Action actions[ MAX_ABSTRACT_ACTIONS ];
  int num_choices = action_abs->get_actions( game, state, actions );

  /* Next, grab the index for this node into the regrets and avg_strategy */
  int64_t soln_idx = num_entries_per_bucket[ state.round ];

  /* Update number of entries */
  num_entries_per_bucket[ state.round ] += num_choices;
  
  /* Recurse to create children */
  BettingNode *first_child = NULL;
  BettingNode *last_child = NULL;
  for( int a = 0; a < num_choices; ++a ) {

    State new_state( state );
    doAction( game, &actions[ a ], &new_state );
    BettingNode *child = init_betting_tree_r( new_state, game, action_abs,
					      num_entries_per_bucket );
    if( last_child != NULL ) {
      last_child->set_sibling( child );
    } else {
      first_child = child;
    }
    last_child = child;
  }
  assert( first_child != NULL );
  assert( last_child != NULL );

  /* Siblings are represented by a linked list,
   * so the last child should have no sibling
   */
  last_child->set_sibling( NULL );

  /* Create the InfoSetNode */
  switch( game->numPlayers ) {
  case 2:
    node = new InfoSetNode2p( soln_idx, num_choices,
			      currentPlayer( game, &state ),
			      state.round, first_child ); 
    break;

  case 3:
    /* We need some additional values not needed in 2p games */
    int8_t player_folded[ MAX_PURE_CFR_PLAYERS ];
    for( int p = 0; p < game->numPlayers; ++p ) {
      player_folded[ p ] = ( state.playerFolded[ p ] ? 1 : 0 );
    }
    uint32_t pot_size;
    uint32_t money_spent[ MAX_PURE_CFR_PLAYERS ];
    leaf_type_t leaf_type;
    get_term_values_3p( state, game, pot_size, money_spent, leaf_type );
    
    node = new InfoSetNode3p( soln_idx, num_choices, currentPlayer( game, &state ),
			      state.round, player_folded, first_child,
			      pot_size, money_spent, leaf_type );
    break;

  default:
    fprintf( stderr, "cannot initialize betting tree for %d-players\n",
	     game->numPlayers );
    assert( 0 );
  }

  return node;
}

void destroy_betting_tree_r( const BettingNode *node )
{
  const BettingNode *child = node->get_child( );
  
  while( child != NULL ) {
    const BettingNode *old_child = child;
    child = child->get_sibling( );
    destroy_betting_tree_r( old_child );
  }
  
  delete node;
}
