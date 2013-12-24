Open Pure CFR
=============

Pure CFR is a time and memory-efficent variant of the [Counterfactual Regret Minimization (CFR) algorithm](http://poker.cs.ualberta.ca/publications/NIPS07-cfr.pdf) algorithm for computing strategies for an extensive-form game with imperfect information, such as poker.  Unlike [Monte Carlo CFR (MCCFR)](http://mlanctot.info/files/papers/nips09mccfr.pdf), Pure CFR samples a *pure* strategy profile (exactly one action assigned probability 1 at every state) on each iteration and performs an update assuming the players and chance follow the sampled pure profile.  In practice, strategies improve faster over time than the original CFR algorithm.  In addition, while previous CFR algorithms require floating-point values (doubles), Pure CFR uses integer values (ints) and thus uses approximately half the memory of other common variants.  Furthermore, Pure CFR retains the theoretical guarantees of MCCFR, and thus converges (probabilistically) to an equilibrium profile in two-player zero-sum games.  More details regarding Pure CFR can be found in [my PhD dissertation](http://richardggibson.appspot.com/static/work/thesis-phd/thesis-phd-paper.pdf).  Credit goes to Oskari Tammelin for inventing the algorithm.

About this Implementation
-------------------------

This implmentation of Pure CFR computes poker strategies that are playable in the [Annual Computer Poker Competition (ACPC)](http://www.computerpokercompetition.org).  In addition to games played at the ACPC, this implementation can also be run on any game that can be defined under the [project_acpc_server framework](http://www.computerpokercompetition.org/downloads/code/competition_server/project_acpc_server_v1.0.33.tar.bz2).  The code runs on both Linux and Windows under [Cygwin](http://www.cygwin.com/).  The code has not been tested on a Mac, but feel free to give Mac a shot if you are feeling adventurous. 

Installing
----------

First, you must have both `make` and `gcc-g++` installed on your machine.  Then, in your open-pure-cfr directory, simply run `make` and wait for the code to finish compiling.  Once complete, you should have three new programs in your open-pure-cfr directory: `pure_cfr`, `print_player_strategy`, and `pure_cfr_player`.

`pure_cfr`
----------

This is the main program that generates strategies for games specified under the [project_acpc_server framework](http://www.computerpokercompetition.org/downloads/code/competition_server/project_acpc_server_v1.0.33.tar.bz2).  Sets of three files are generated during the run, a `.regrets`, `.avg-strategy`, and `.player` file.  Run `./pure_cfr` with no additional arguments to display the usage information.  We will now describe in detail each of the required and optional arguments before providing some examples of how to run `pure_cfr` on a variety of games.

###Command-line Arguments

`pure_cfr` requires two arguments.  The first argument must be a file that defines the game to be played.  The games provided by the [project_acpc_server code](http://www.computerpokercompetition.org/downloads/code/competition_server/project_acpc_server_v1.0.33.tar.bz2) can be found in the `games/` subdirectory, along with a definition for [Kuhn Poker](http://en.wikipedia.org/wiki/Kuhn_poker).  The second argument is a prefix that specifies where and what name the output files will be and have respectively.  

After these two arguments are specified, a number of different options can be selected:
  * `--config=<file>` - Overwrites the two required arguments and the default options through values specified in `file`.  See `parameters.cpp::read_params( )` for details on how to format this file.
  * `--rng=<seed1:seed2:seed3:seed4|TIME>` - Specifies the seeds to be used to initialize the random number generator, where `seed1`, `seed2`, `seed3`, and `seed4` are integer values.  The random number generator is used to sample a pure strategy profile on each iteration from chance and the players.  Alternatively, passing the option `--rng=TIME` initializes the random number generator according to the current time.
  * `--card-abs=<NULL|BLIND>` - Specifies a card abstraction to be used.  Only two card abstractions are currently implemented.  `--card-abs=NULL` specifies no card abstraction (not even suit isomorphisms), while `--card-abs=BLIND` specifies that all hands fall into the same bucket.  NULL is only feasible in toy games, like Kuhn Poker, that use very few cards, while BLIND essentially means that the players never look at the public or their private cards.
  * `--action-abs=<NULL|FCPA>` - Specifies an action abstraction to be used.  This option should only be used for nolimit games.  `--action-abs=NULL` specifies that all actions remain legal in the abstract game, while `--action-abs=FCPA` specifies that only fold, call, pot-sized raises, and all-ins are legal in the abstract game.  NULL is only feasible in small nolimit games with low stack sizes.  
  * `--load-dump=<dump_prefix>` - Loads the regrets and (if `--no-average` is not selected) average strategy from a previous run from the files prefixed by `dump_prefix`.  This prefix should be the full name of the files to be loaded, but without the `.regrets` or `.avg-strategy` suffix.
  * `--threads=<num_threads>` - Specifies the number of threads to use.  Additional threads provide a near-linear speed-up in the algorithm, so use as many as you can afford.
  * `--status=<dd:hh:mm:ss>` - Prints status updates to `stderr` every `dd` days, `hh` hours, `mm` minutes, and `ss` seconds.
  * `--checkpoint=<start_time[,mult_time[,add_time]]>` - Specifies how frequently the program should dump the regrets and average strategy to disk, where `start_time`, `mult_time`, and `add_time` are specified using the `dd:hh:mm:ss` format.  First, the program will dump after `start_time` has passed from the time the program started.  Later dump times depend on whether `mult_time` and `add_time` are provided.  If `mult_time` is provided, the next dump will come after `start_time` * `mult_time`, then again after `start_time` * `mult_time` * `mult_time`, and so on until the program terminates.  If, in addition, `add_time` is provided, then the next dump will come after `start_time` * `mult_time` + `add_time`, then again after (`start_time` * `mult_time` + `add_time`) * `mult_time` + `add_time`, and so on.  If `mult_time` is not specified, then the next dumps will occur at 2 * `start_time`, then again after 3 * `start_time`, and so on.
  * `--max-walltime=<dd:hh:mm:ss>` - Specifies when it is time to perform a final dump of regrets and average strategy to disk.  After the final dump, the program is terminated.
  * `--no-average` - Specifies that no average strategy is to be computed.  Currently, average strategy computation in games with more than two players is not supported, and so for such games, this option is mandatory.

###Examples

Let's start with a very simple example that requires very little computing resources to run:

    ./pure_cfr games/kuhn.game test.kuhn --status=1 --max-walltime=30

This runs `pure_cfr` on unabstracted Kuhn Poker for 30 seconds, with status updates printed every second.  In all likelihood, the resulting average strategy should be an approximate Nash equilibrium profile.  Once complete, you should have three files that look something like:
* `test.kuhn.iter-???.secs-60.regrets` - This is a binary file that contains the accumulated regret for both players at every information set in the game.
* `test.kuhn.iter-???.secs-60.avg-strategy` - This is another binary file that specifies the (sampled) average strategy for both players.
* `test.kuhn.iter-???.secs-60.player` - This is a human-readable wrapper file that contains information about the command-line arguments specified and the prefix of the binary files.  On repeated runs, you can instead pass in `--config=test.kuhn.iter-???.secs-60.player` rather than specifying the options on the command line.  In addition, the `DO_AVERAGE` line in this file specifies whether the average strategy (`DO_AVERAGE TRUE`) or the current strategy (`DO_AVERAGE FALSE`) is to be played and printed by `pure_cfr_player` and `print_player_strategy` respectively.

Our second example converges to an equilibrium for abstract heads-up limit Texas hold'em where neither player looks at the cards dealt:

    ./pure_cfr games/holdem.limit.2p.reverse_blinds.game test.holdem.2pl --rng=TIME --card-abs=BLIND 
    --threads=4 --checkpoint=1:0:0 --max-walltime=1:0:0:0

Here, we specify the random number generator to be initialized by the current time, use 4 processors, dump regrets and average strategy to disk every hour, and terminate after 1 day.

Finally, we also provide an example of running `pure_cfr` on abstract three-player nolimit Texas hold'em:

    ./pure_cfr games/holdem.nolimit.3p.game test.holdem.3pn --card-abs=BLIND --action-abs=FCPA 
    --threads=4 --max-walltime=3:0:0:0 --no-average
    
In addition to the players not being able to see any cards, the players may also only take the actions fold, call, pot-sized raise, and all-in.  The options specify 4 processors to be used and to terminate after 3 days of computation.  In addition, no average strategy is computed (recall that this is currently required for games with more than two players). Thus, upon completion, only the `.regrets` and `.player` files are written to disk.

`print_player_strategy`
-----------------------

This program is a simple tool for displaying the outputted strategy in a human-readable format.  Running `./print_player_strategy` with no arguments displays the usage.  

One argument is required and a second argument is optional.  For the required argument, `print_player_strategy` takes the filename of a `.player` file generated from `pure_cfr`.  The optional argument `--max-round=<round>` can be used to only print the strategy up to and including round `round`.  

For example, we can print our Kuhn Poker strategy generated from the example above as follows:

    ./print_player_strategy test.kuhn.iter-???.secs-60.player
    
In addition, you can also display the *current* strategy specified by the regrets.  To do so, in the `.player` file, change the line `DO_AVERAGE TRUE` to `DO_AVERAGE FALSE` and run `print_player_strategy` again.  This strategy may not be an approximate equilibrium.

As a second example, to print out the pre-flop strategy of our heads-up limit hold'em profile generated above after one hour, we would run

    ./print_player_strategy test.holdem.2pl.iter-???.secs-3600.player --max-round=1
    
`pure_cfr_player`
-----------------

This program allows a strategy profile generated by `pure_cfr` to be played through the [ACPC protocol](http://www.computerpokercompetition.org/downloads/documents/protocols/protocol.pdf).  It is based on `example_player` from the [project_acpc_server framework](http://www.computerpokercompetition.org/downloads/code/competition_server/project_acpc_server_v1.0.33.tar.bz2) that communicates back and forth with a dealer program.  Similar to the examples provided by the project_acpc_server package, a bash script or something similar is required to run an agent.  For example, to run a heads-up limit hold'em profile generated after 1 hour, write a bash script that looks something like this:

    #!/bin/bash
    cd path/to/open-pure-cfr/
    ./pure_cfr_player test.holdem.2pl.iter-???.secs-3600.player $1 $2
    
(Note that ACPC submissions in the past require the `cd path/to/open-pure-cfr/` line to be removed).  Save this to a file called, say, `test.holdem.2pl.iter-???.secs-3600.sh` and make it executable via `chmod +x test.holdem.2pl.iter-???.secs-3600.sh`.  This bash script can then be used to play through the ACPC protocol.  For more information on running a match through the ACPC framework, please see the documentation included with the [project_acpc_server framework](http://www.computerpokercompetition.org/downloads/code/competition_server/project_acpc_server_v1.0.33.tar.bz2).

Implementation Quirks
---------------------

###Parallelization

When multiple threads are specified for `pure_cfr` through the `--threads` option, these threads act independently on the regrets and average strategy in shared memory.  Each thread runs independent iterations through the entire tree visited by the sampled pure strategy profile and no safety precautions are taken to avoid the threads from conflicting with one another.  This means that if two threads happen to update the regret at the same location at the same time, one of the updates will be overwritten.  Because the chances of this occurring in a large game tree are slim, and because billions of iterations are typically required before competent play is reached, a few iterations of lost updates are not a big concern.

###Data Types

As mentioned in the opening of this README, Pure CFR stores regrets and the average strategy using integer values rather than floating-point values.  In this implementation, each regret entry is stored as an `int` and each average strategy entry is stored as an `int32_t`.  One exception to this is that each average strategy entry in the preflop round is stored as an `int64_t`.  The reason 64-bit ints are used in the preflop instead of 32-bit ints is because the preflop entries are updated (incremented) most frequently of all the average strategy entries and will be the first to overflow.  I found cases where overflow occurred with 32-bit ints in the preflop long before the strategy had finished improving, and so 64-bit ints are now used to prevent early overflow.  Since the preflop round is also the smallest, the increase in memory usage in very minor.

Acknowledgements
----------------

First of all, thanks to Oskari Tammelin for sharing his ideas for Pure CFR.  I must also thank Neil Burch and [Michael Johanson](http://cs.ualberta.ca/~johanson) of the [Computer Poker Research Group](http://poker.cs.ualberta.ca) as many of the optimization tricks in this implementation were learned from them.

More Information
----------------

A [demo of Pure CFR](http://jeskola.net/cfr) and other variants are provided by Oskari Tammelin.

Contact
-------

Feel free to email me at [richard.g.gibson@gmail.com](mailto:richard.g.gibson@gmail.com) with any questions about this project.
