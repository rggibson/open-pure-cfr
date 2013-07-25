#
# Makefile for Open Pure CFR
# Richard Gibson, June 26, 2013
#

#OPT = -Wall -O3 -ffast-math -funroll-all-loops -ftree-vectorize -DHAVE_MMAP
OPT = -O0 -Wall -g -fno-inline

PURE_CFR_FILES = main.o acpc_server_code/game.o acpc_server_code/rng.o constants.o parameters.o utility.o card_abstraction.o action_abstraction.o betting_node.o entries.o pure_cfr_machine.o

all: pure_cfr

pure_cfr: $(PURE_CFR_FILES)
	$(CXX) $(OPT) -lpthread -o $@ $(PURE_CFR_FILES)

clean: 
	-rm *.o acpc_server_code/*.o
	-rm pure_cfr
