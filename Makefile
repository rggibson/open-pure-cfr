#
# Makefile for Open Pure CFR
# Richard Gibson, June 26, 2013
#

#OPT = -Wall -O3 -ffast-math -funroll-all-loops -ftree-vectorize -DHAVE_MMAP
OPT = -O0 -Wall -g -fno-inline

PURE_CFR_FILES = pure_cfr.o acpc_server_code/game.o acpc_server_code/rng.o constants.o parameters.o utility.o card_abstraction.o action_abstraction.o betting_node.o entries.o abstract_game.o player_module.o pure_cfr_machine.o

PRINT_PLAYER_STRATEGY_FILES = print_player_strategy.o player_module.o acpc_server_code/game.o acpc_server_code/rng.o constants.o parameters.o utility.o card_abstraction.o action_abstraction.o betting_node.o entries.o abstract_game.o

PURE_CFR_PLAYER_FILES = pure_cfr_player.o player_module.o acpc_server_code/game.o acpc_server_code/rng.o acpc_server_code/net.o constants.o parameters.o utility.o card_abstraction.o action_abstraction.o betting_node.o entries.o abstract_game.o

all: pure_cfr print_player_strategy pure_cfr_player

%.o: %.cpp
	$(CXX) $(OPT) -c $^

pure_cfr: $(PURE_CFR_FILES)
	$(CXX) $(OPT) -lpthread -o $@ $(PURE_CFR_FILES)

print_player_strategy: $(PRINT_PLAYER_STRATEGY_FILES)
	$(CXX) $(OPT) -o $@ $(PRINT_PLAYER_STRATEGY_FILES)

pure_cfr_player: $(PURE_CFR_PLAYER_FILES)
	$(CXX) $(OPT) -o $@ $(PURE_CFR_PLAYER_FILES)

clean: 
	-rm *.o acpc_server_code/*.o
	-rm pure_cfr print_player_strategy pure_cfr_player
