#ifndef __PURE_CFR_CONSTANTS_HPP__
#define __PURE_CFR_CONSTANTS_HPP__

/* constants.hpp
 * Richard Gibson, Jun 28, 2013
 * Email: richard.g.gibson@gmail.com
 *
 * Defines a number of contant values for array sizes and stuff.
 *
 * Copyright (C) 2013 by Richard Gibson
 */

/* C / C++ / STL includes */

/* C project-acpc-server includes */
extern "C" {
#include "acpc_server_code/game.h"
}

/* pure cfr includes */

/* Maximum number of players this program can handle right now */
const int MAX_PURE_CFR_PLAYERS = 3;

/* Maximum number of abstract actions a player can choose from */
const int MAX_ABSTRACT_ACTIONS = 4;

/* Length of strings used for filenames */
const int PATH_LENGTH = 1024;

/* Number of iterations to run per thread before checking for pause or quit */
const int ITERATION_BLOCK_SIZE = 1000;

/* Enum of card abstraction types */
typedef enum {
  CARD_ABS_NULL = 0,
  CARD_ABS_BLIND = 1,
  NUM_CARD_ABS_TYPES = 2
} card_abs_type_t;
extern const char card_abs_type_to_str[ NUM_CARD_ABS_TYPES ][ PATH_LENGTH ];

/* Enum of action abstraction types */
typedef enum {
  ACTION_ABS_NULL = 0,
  ACTION_ABS_FCPA = 1,
  NUM_ACTION_ABS_TYPES = 2
} action_abs_type_t;
extern const char action_abs_type_to_str[ NUM_ACTION_ABS_TYPES ][ PATH_LENGTH ];

/* Enum of all possible combinations of players that have not folded at a leaf */
typedef enum {
  LEAF_P0 = 0,
  LEAF_P1 = 1,
  LEAF_P0_P1 = 2,
  LEAF_P2 = 3,
  LEAF_P0_P2 = 4,
  LEAF_P1_P2 = 5,
  LEAF_P0_P1_P2 = 6,
  LEAF_NUM_TYPES = 7
} leaf_type_t;

/* Possible regret and average strategy storage types */
typedef enum {
  TYPE_UINT8_T = 0,
  TYPE_INT = 1,
  TYPE_UINT32_T = 2,
  TYPE_UINT64_T = 3,
  TYPE_NUM_TYPES = 4
} pure_cfr_entry_type_t;

extern const pure_cfr_entry_type_t
REGRET_TYPES[ MAX_ROUNDS ];

extern const pure_cfr_entry_type_t
AVG_STRATEGY_TYPES[ MAX_ROUNDS ];

#endif
