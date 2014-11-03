# Makefile for PDLDA train by Yijie Wang (wyijie93@gmail.com)

CONFIG=config.h

CC=g++
MPICC=mpic++
CFLAGS=-O3 -Wall -imacros $(CONFIG)

all: pdlda_simp
.PHONY: all clean pdlda_simp 

clean:
	rm -f *.o
	rm -f pdlda_simp

pdlda_simp:
	$(MPICC) -o pdlda_simp $(CFLAGS) ArgRegister.cpp BookLoader.cpp CntrServer_simp.cpp DocState.cpp mtrand.cpp PdldaTrainTest_simp.cpp RandGen.cpp Sampler.cpp TaskAssigner.cpp TVectorPool.cpp
