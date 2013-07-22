/* utility.cpp
 * Richard Gibson, Jun 28, 2013
 * Email: richard.g.gibson@gmail.com
 *
 * Home of implementations of useful functions used in multiple places in the project.
 */

/* C / C++ / STL includes */
#include <stdio.h>
#include <string.h>

/* Pure CFR includes */
#include "utility.hpp"

int strtoint64_units( const char *ptr, int64_t &retval )
{
  int64_t number = 0;
  long long int parseNumber = 0;
  char unit = 'x';
  int scanned = sscanf(ptr, "%lld%c", &parseNumber, &unit);
  number = parseNumber;
  if (scanned == 0) {
    return 1;
  } else if (scanned == 1) {
    /* Only scanned a number.  Return it. */
    retval = number;
    return 0;
  } else if (scanned == 2) {
    /* Scanned a number and a unit. */
    switch(unit)
      {
      case 'k':
	number *= 1000;
	break;
      case 'm':
	number *= 1000000;
	break;
      case 'b':
	number *= 1000000000;
	break;
      default:
	return 2;
      }
    retval = number;
    return 0;
  } else {
    return 2;
  }
}

void int64tostr_units( int64_t val, char *ptr, int n )
{
  char unit = ' ';

  if( ( val >= 1000000000 ) && ( val % 1000000000 == 0 ) ) {
    val /= 1000000000;
    unit = 'b';
  } else if( ( val >= 1000000 ) && ( val % 1000000 == 0 ) ) {
    val /= 1000000;
    unit = 'm';
  } else if( ( val >= 1000 ) && ( val % 1000 == 0 ) ) {
    val /= 1000;
    unit = 'k';
  }

  if( unit == ' ' ) {
    snprintf( ptr, n, "%jd", ( intmax_t ) val );
  } else {
    snprintf( ptr, n, "%jd%c", ( intmax_t ) val, unit);
  }
}

int time_string_to_seconds( const char *str )
{
  int units[ 4 ];
  int numParsed = sscanf( str, "%d:%d:%d:%d", 
			  &units[ 0 ],
			  &units[ 1 ],
			  &units[ 2 ],
			  &units[ 3 ] );
  int total = 0;
  if( numParsed == 4 ) {
    total = units[ 0 ] * 86400 
      + units[ 1 ] * 3600 
      + units[ 2 ] * 60
      + units[ 3 ];
  } else if( numParsed == 3 ) {
    total = units[ 0 ] * 3600 
      + units[ 1 ] * 60
      + units[ 2 ];
  } else if( numParsed == 2 ) {
    total = units[ 0 ] * 60
      + units[ 1 ];
  } else if( numParsed == 1 ) {
    total = units[ 0 ];
  } 
  return total;
}

void time_seconds_to_string( int time, char *str, int strlen )
{
  int days = 0;
  int hours = 0;
  int minutes = 0;
  int seconds = 0;
  if( time > 86400 ) {
    days = time / 86400;
    time -= days * 86400;
  }
  if( time > 3600 ) {
    hours = time / 3600;
    time -= hours * 3600;
  } 
  if( time > 60 ) {
    minutes = time / 60;
    time -= minutes * 60;
  }
  seconds = time;
  
  if( days ) {
    snprintf(str, strlen, "%d:%02d:%02d:%02d days", days, hours, minutes, seconds);
  } else if( hours ) {
    snprintf(str, strlen, "%d:%02d:%02d hours", hours, minutes, seconds);
  } else if( minutes ) {
    snprintf(str, strlen, "%d:%02d minutes", minutes, seconds);
  } else if( seconds ) {
    snprintf(str, strlen, "%d seconds", seconds);
  } else {
    snprintf(str, strlen, "0 seconds");
  }
}
