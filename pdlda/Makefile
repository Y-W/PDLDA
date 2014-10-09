# Makefile for PDLDA train by Yijie Wang (wyijie93@gmail.com)

CONFIG=config.h

CC=g++
MPICC=mpic++
CFLAGS=-O3 -Wall -imacros $(CONFIG)

all: split_dataset pdlda
.PHONY: all clean split_dataset pdlda 

clean:
	rm -f *.o
	rm -f split_dataset pdlda

split_dataset:
	$(CC) -o split_dataset $(CFLAGS) split_dataset.cpp mtrand.cpp

pdlda:
	$(MPICC) -o pdlda $(CFLAGS) ArgRegister.cpp BookLoader.cpp CntrServer.cpp DocState.cpp mtrand.cpp PdldaTrainTest.cpp RandGen.cpp Sampler.cpp TaskAssigner.cpp TVectorPool.cpp
