# Makefile for PDLDA train by Yijie Wang (wyijie93@gmail.com)

CONFIG=train_config.h

CC=g++
MPICC=mpic++
CFLAGS=-O3 -Wall -imacros $(CONFIG)

all: split_dataset pdlda_train
.PHONY: all clean split_dataset pdlda_train 

clean:
	rm -f *.o
	rm -f split_dataset pdlda_train

split_dataset:
	$(CC) -o split_dataset $(CFLAGS) split_dataset.cpp mtrand.cpp

pdlda_train:
	$(MPICC) -o pdlda_train $(CFLAGS) ArgRegister.cpp BookLoader.cpp CntrServer.cpp DocState.cpp mtrand.cpp PdldaTrain.cpp RandGen.cpp Sampler.cpp TaskAssigner.cpp TVectorPool.cpp
