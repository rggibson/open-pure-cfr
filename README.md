Open Pure CFR
=============

Pure CFR is a time and memory-efficent variant of the [Counterfactual Regret Minimization (CFR) algorithm](http://poker.cs.ualberta.ca/publications/NIPS07-cfr.pdf) algorithm for computing strategies for an extensive-form game with imperfect information, such as poker.  Unlike [Monte Carlo CFR (MCCFR)](http://mlanctot.info/files/papers/nips09mccfr.pdf), Pure CFR samples a *pure* strategy profile (exactly one action assigned probability 1 at every state) on each iteration and performs an update assuming the players and chance follow the sampled pure profile.  In practice, strategies improve faster over time than the original CFR algorithm.  In addition, while previous CFR algorithms require floating-point values (doubles), Pure CFR uses integer values (ints) and thus uses approximately half the memory of other common variants.  Furthermore, Pure CFR retains the theoretical guarantees of MCCFR, and thus converges (probabilistically) to an equilibrium profile in two-player zero-sum games.  Credit goes to Oskari Tammelin for the algorithmic invention.

About this Implementation
-------------------------

This implmentation of Pure CFR computes poker strategies that are playable in the [Annual Computer Poker Competition (ACPC)](http://www.computerpokercompetition.org).  In addition to games played at the ACPC, this implmentation can also be run on any game that can be defined under the [project_acpc_server framework](http://www.computerpokercompetition.org/downloads/code/competition_server/project_acpc_server_v1.0.33.tar.bz2).  The code runs on both Linux and Windows under [Cygwin](http://www.cygwin.com/).  The code has not been tested on a Mac, but feel free to give Mac a shot if you are feeling adventurous. 

Installing
----------

First, you must have both `make` and `gcc-g++` installed on your machine.  Then, in your open-pure-cfr directory, simply run `make` and wait for the code to finish compiling.  Once complete, you should have three new programs in your open-pure-cfr directory: `pure_cfr`, `print_player_strategy`, and `pure_cfr_player`.

`pure_cfr`
----------

This is the main program that generates strategies for games specified under the [project_acpc_server framework](http://www.computerpokercompetition.org/downloads/code/competition_server/project_acpc_server_v1.0.33.tar.bz2).  Run `./pure_cfr` with no additional arguments to display the usage information.

`pure_cfr` requires two arguments.  The first argument must be a file that defines the game to be played.  The games provided by the [project_acpc_server code](http://www.computerpokercompetition.org/downloads/code/competition_server/project_acpc_server_v1.0.33.tar.bz2) can be found in the `games/` subdirectory, along with a definition for [Kuhn Poker](http://en.wikipedia.org/wiki/Kuhn_poker).  The second argument is a prefix that specifies where and what name the output files will be and have respectively.  

After these two arguments are specified, a number of different options can be selected:
  * `--config=<file>` - Overwrites the two required arguments and the default options through values specified in `file`.  See `parameters.cpp::read_params( )` for details on how to format this file.
  * `--rng=<seed1:seed2:seed3:seed4|TIME>` - Specifies the seeds to be used to initialize the random number generator, where `seed1`, `seed2`, `seed3`, and `seed4` are integer values.  The random number generator is used to sample a pure strategy profile on each iteration from chance and the players.  Alternatively, passing the option `--rng=TIME` initializes the random number generator according to the current time.
  * `--card-abs=<NULL|BLIND>` - Specifies a card abstraction to be used.  Only two card abstractions are currently implemented.  `--card-abs=NULL` specifies no card abstraction (not even suit isomorphisms), while `--card-abs=BLIND` specifies that all hands fall into the same bucket.  NULL is only feasible in toy games, like Kuhn Poker, that use very few cards, while BLIND essentially means that the players never look at the public or their private cards.
  * `--action-abs=<NULL|FCPA>` - Specifies an action abstraction to be used.  This option should only be used for nolimit games.  `--action-abs=NULL` specifies that all actions remain legal in the abstract game, while `--action-abs=FCPA` specifies that only fold, call, pot-sized raises, and all-ins are legal in the abstract game.  NULL is only feasible in small nolimit games with low stack sizes.  
  * `--load-dump=<dump_prefix>` - Loads the regrets and (if `--no-average` is not selected) average strategy from a previous run from the files prefixed by `dump_prefix`.  This prefix should be the same prefix specified as the second required argument from the previous run (unless the filename(s) have since changed).
  * `--threads=<num_threads>` - Specifies the number of threads to use.  Additional threads provide a near-linear speed-up in the algorithm, so use as many as you can afford.

Implementation Quirks
---------------------

###Parallelization

When multiple threads are specified for `pure_cfr` through the `--threads` option, these threads act independently on the regrets and average strategy in shared memory.  Each threas runs independent iterations through the entire tree visited by the sampled pure strategy profile and no safety precautions are taken to avoid the threads from conflicting with one another.  This means that if two threads happen to update the regret at the same location at the same time, one of the updates will be overwritten.  Because the chances of this occurring in a large game tree are slim, and because billions of iterations are typically required before competent play is reached, a few iterations of work lost is not a big concern.

More Information
----------------

A [demo of Pure CFR](http://jeskola.net/cfr) and other variants are provided by Oskari Tammelin.
