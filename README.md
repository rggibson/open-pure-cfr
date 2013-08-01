Open Pure CFR
=============

Pure CFR is a time and memory-efficent variant of the [Counterfactual Regret Minimization (CFR) algorithm](http://poker.cs.ualberta.ca/publications/NIPS07-cfr.pdf) algorithm for computing strategies for an extensive-form game with imperfect information, such as poker.  Unlike [Monte Carlo CFR (MCCFR)](http://mlanctot.info/files/papers/nips09mccfr.pdf), Pure CFR samples a *pure* strategy profile (exactly one action assigned probability 1 at every decision state) on each iteration and performs an update assuming the players follow the sampled pure strategy.  In practice, strategies improve faster over time than the original CFR algorithm.  In addition, while previous CFR algorithms require floating-point values (doubles), Pure CFR uses integer values (ints) and thus uses approximately half the memory of other common variants.  Furthermore, Pure CFR retains the theoretical guarantees of MCCFR, and thus converges to an equilibrium profile in two-player zero-sum games.  Credit goes to Oskari Tammelin for the algorithmic invention.

About this Implementation
-------------------------

This implmentation of Pure CFR computes poker strategies that are playable in the [Annual Computer Poker Competition (ACPC)](http://www.computerpokercompetition.org).  In addition to games played at the ACPC, this implmentation can also be run on any game that can be defined under the [project_acpc_server framework](http://www.computerpokercompetition.org/downloads/code/competition_server/project_acpc_server_v1.0.33.tar.bz2).  The code runs on both Linux and Windows under Cygwin.  Mac has not been tested, but may work as well. 

Installing
----------

First, you must have both `make` and `gcc-g++` installed on your machine.  

A [demo of Pure CFR](http://jeskola.net/cfr) and other variants are provided by Oskari Tammelin.
