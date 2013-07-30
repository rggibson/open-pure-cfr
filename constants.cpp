/* constants.cpp
 * Richard Gibson, Jun 28, 2013
 * Email: richard.g.gibson@gmail.com
 *
 * Initializes constant arrays to their values.
 *
 * Copyright (C) 2013 by Richard Gibson
 */

/* C / C++ / STL includes */

/* Pure CFR includes */
#include "constants.hpp"

const char card_abs_type_to_str[ NUM_CARD_ABS_TYPES ][ PATH_LENGTH ]
= { "NULL", "BLIND" };

const char action_abs_type_to_str[ NUM_ACTION_ABS_TYPES ][ PATH_LENGTH ]
= { "NULL", "FCPA" };

/* Store regrets as ints because they can have either sign and typically don't get "too" positive */
const pure_cfr_entry_type_t
REGRET_TYPES[ MAX_ROUNDS ] = { TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT };

/* Store avg strategy as unsigned ints since they are nonnegative.
 * Also, store preflop avg strategy in 64-bit ints since preflop info sets hit often.
 * Using 64-bit ints prevents overflow from happening too early and is cheap for the preflop.
 */
const pure_cfr_entry_type_t
AVG_STRATEGY_TYPES[ MAX_ROUNDS ] = { TYPE_UINT64_T, TYPE_UINT32_T, TYPE_UINT32_T, TYPE_UINT32_T };
