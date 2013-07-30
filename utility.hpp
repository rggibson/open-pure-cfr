#ifndef __PURE_CFR_UTILITY_H__
#define __PURE_CFR_UTILITY_H__

/* utility.hpp
 * Richard Gibson, Jun 28, 2013
 * Email: richard.g.gibson@gmail.com
 *
 * Home of useful functions used in multiple places in the project.
 *
 * Copyright (C) 2013 by Richard Gibson
 */

/* C / C++ / STL includes */
#include <inttypes.h>

/* Pure CFR includes */
#include "constants.hpp"

int strtoint64_units(const char *ptr, int64_t &retval);
void int64tostr_units( int64_t val, char *ptr, int n );
int time_string_to_seconds( const char *str );
void time_seconds_to_string( int seconds, char *str, int strlen );
/* Returns 0 on success, 1 on failure */
int get_next_token( char out[ PATH_LENGTH ], const char *str );

#endif
