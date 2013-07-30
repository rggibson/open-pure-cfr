/* pure_cfr_player.cpp
 * Richard Gibson, Jul 29, 2013
 * Email: richard.g.gibson@gmail.com
 *
 * Player program for actually playing poker with an ACPC dealer.
 * Based on example_player.c from ACPC code.
 *
 * Copyright (C) 2013 by Richard Gibson
 */

/* C / C++ includes */
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/socket.h>

/* C project-acpc-server includes */
extern "C" {
#include "acpc_server_code/net.h"
}

/* Pure CFR includes */
#include "player_module.hpp"

int main( int argc, char *argv[] )
{
  /* Print usage */
  if( argc != 4 ) {
    fprintf( stderr, "Usage: pure_cfr_player <player_file> <server> <port>\n" );
    return 1;
  }

  /* Initialize player module and get the abstract game */
  PlayerModule player_module( argv[ 1 ] );
  const AbstractGame *ag = player_module.get_abstract_game( );

  /* Connect to the dealer */
  uint16_t port;
  if( sscanf( argv[ 3 ], "%"SCNu16, &port ) < 1 ) {
    fprintf( stderr, "ERROR: invalid port %s\n", argv[ 3 ] );
    return 1;
  }
  int sock = connectTo( argv[ 2 ], port );
  if( sock < 0 ) {
    return 1;
  }
  FILE *toServer = fdopen( sock, "w" );
  FILE *fromServer = fdopen( sock, "r" );
  if( toServer == NULL || fromServer == NULL ) {
    fprintf( stderr, "ERROR: could not get socket streams\n" );
    return 1;
  }

  /* Send version string to dealer */
  if( fprintf( toServer, "VERSION:%"PRIu32".%"PRIu32".%"PRIu32"\n",
	       VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION ) != 14 ) {
    fprintf( stderr, "ERROR: could not get send version to server\n" );
    return 1;
  }
  fflush( toServer );

  /* Play the game! */
  char line[ MAX_LINE_LEN ];
  while( fgets( line, MAX_LINE_LEN, fromServer ) ) {

    /* Ignore comments */
    if( ( line[ 0 ] == '#' ) || ( line[ 0 ] == ';' ) ) {
      continue;
    }

    /* Read the incoming match state */
    MatchState match_state;
    int len = readMatchState( line, ag->game, &match_state );
    if( len < 0 ) {
      fprintf( stderr, "ERROR: Could not read match state [%s]\n", line );
      return 1;
    }

    /* Ignore game over message */
    if( stateFinished( &match_state.state ) ) {
      continue;
    }

    /* Ignore states that we are not acting in */
    if( currentPlayer( ag->game, &match_state.state )
	!= match_state.viewingPlayer ) {
      continue;
    }

    /* Start building the response to the server by adding a colon
     * (guaranteed to fit because we read a new-line in fgets)
     */
    line[ len ] = ':';
    ++len;

    /* Get an action to play */
    Action action = player_module.get_action( match_state.state );

    /* Send the action to the server */
    assert( isValidAction( ag->game, &match_state.state, 0, &action ) );
    int act_str_len = printAction( ag->game, &action, MAX_LINE_LEN - len - 2,
				   &line[ len ] );
    if( act_str_len < 0 ) {
      fprintf( stderr, "ERROR: Response too long after printing action\n" );
      return 1;
    }
    len += act_str_len;
    line[ len ] = '\r';
    ++len;
    line[ len ] = '\n';
    ++len;
    if( fwrite( line, 1, len, toServer ) != ( size_t ) len ) {
      fprintf( stderr, "ERROR: Problem sending response to server\n" );
      return 1;
    }
    fflush( toServer );
  }

  return 0;
}
