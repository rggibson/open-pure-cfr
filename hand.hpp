#ifndef __PURE_CFR_HAND_HPP__
#define __PURE_CFR_HAND_HPP__

/* 
 * hand.hpp
 * Richard Gibson, Jun 28, 2013
 *
 * Structure to represent a hand with possible precomputed buckets and showdown info.
 */

/* C / C++ / STL includes */

/* C project_acpc_server includes */
extern "C" {
}

/* Pure CFR includes */
#include "constants.hpp"

typedef struct {
  /* The actual cards for this hand */
  uint8_t boardCards[ MAX_BOARD_CARDS ];
  uint8_t holeCards[ MAX_PURE_CFR_PLAYERS ][ MAX_HOLE_CARDS ];
  /* When bucketing is only dependent on the round,
   * we just compute the buckets once and store
   */
  int precomputed_buckets[ MAX_PURE_CFR_PLAYERS ][ MAX_ROUNDS ];
  union {
    /* Potsize divided by pot_frac_recip[ p ][ type ] = utilily for player p
     * (>2p only)
     */
    int pot_frac_recip[ MAX_PURE_CFR_PLAYERS ][ LEAF_NUM_TYPES ];
    /* (-1,0,1) if player (loses,ties,wins) in showdown (2p only)*/
    int8_t showdown_value_2p[ 2 ]; 
  } eval;
} hand_t;

#endif
