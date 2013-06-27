#
# Makefile for Open Pure CFR
# Richard Gibson, June 26, 2013
#

#OPT = -Wall -O3 -ffast-math -funroll-all-loops -ftree-vectorize -DHAVE_MMAP
OPT = -O0 -Wall -g -fno-inline

PURE_CFR_FILES = main.o acpc_server_code/game.o acpc_server_code/rng.o 

all: pure_cfr

pure_cfr: $(PURE_CFR_FILES)
	$(CXX) $(OPT) -o $@ $(PURE_CFR_FILES)

clean: 
	-rm *.o acpc_server_code/*.o
	-rm pure_cfr
