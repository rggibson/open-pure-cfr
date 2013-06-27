/* main.cpp
 * Richard Gibson, Nov 19, 02012
 *
 * Entry point into Pure CFR that spawns worker threads to run pure cfr iterations.
 * Follows general organization of CCFR.
 */

/* C / C++ includes */
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

/* C project-acpc-server includes */
extern "C" {
#include "acpc_server_code/game.h"
}

int main( const int argc, const char *argv[] )
{
  printf("Hello World!\n");

  FILE *file = fopen( "games/holdem.limit.3p.game", "r" );
  Game *game = readGame( file );
  printGame( stdout, game );
  free( game );
  
  /* Done! */
  return 0;
}
