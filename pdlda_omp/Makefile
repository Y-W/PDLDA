# Makefile for PDLDA (OpenMP version) by Yijie Wang (wyijie93@gmail.com)

CC=g++
CFLAGS=-fopenmp -O3 -Wall

all: pdlda_omp
.PHONY: all clean pdlda_omp 

clean:
	rm -f *.o
	rm -f pdlda_omp

pdlda_omp: *.*
	$(CC) -o pdlda_omp $(CFLAGS) *.cpp
