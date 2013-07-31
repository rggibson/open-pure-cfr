#ifndef __PURE_CFR_ACTION_ABSTRACTION_HPP__
#define __PURE_CFR_ACTION_ABSTRACTION_HPP__

/* action_abstraction.hpp
 * Richard Gibson, Jun 28, 2013
 *
 * Home of the action_abstraction abstract class and all implementing classes
 *
 * Copyright (C) 2013 by Richard Gibson
 */

/* C / C++ / STL indluces */

/* project_acpc_server includes */
extern "C" {
#include "acpc_server_code/game.h"
}

/* Pure CFR includes */
#include "constants.hpp"

/* Base Class */
class ActionAbstraction {
public:

  ActionAbstraction( );
  virtual ~ActionAbstraction( );

  virtual int get_actions( const Game *game,
			   const State &state,
			   Action actions[ MAX_ABSTRACT_ACTIONS ] ) const = 0;

protected:
};

/* The null action abstraction makes every action in the real game allowable in
 * the abstract game.  Standard for limit games, but not feasible for large
 * nolimit games.
 */
class NullActionAbstraction : public ActionAbstraction {
public:

  NullActionAbstraction( );
  virtual ~NullActionAbstraction( );

  virtual int get_actions( const Game *game,
			   const State &state,
			   Action actions[ MAX_ABSTRACT_ACTIONS ] ) const;

protected:
};

/* The FCPA action abstraction only allows the fold, call, pot (if legal), and
 * allin actions.  Not applicable in limit games, but a good starting point for
 * nolimit games.
 */
class FcpaActionAbstraction : public ActionAbstraction {
public:

  FcpaActionAbstraction( );
  virtual ~FcpaActionAbstraction( );

  virtual int get_actions( const Game *game,
			   const State &state,
			   Action actions[ MAX_ABSTRACT_ACTIONS ] ) const;

protected:
};

#endif
