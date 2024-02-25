CC=gcc

CCFLAGS=-Wall -O3

LIBRARIES=-lz

all: vxp.o
	$(CC) $(CCFLAGS) $^ $(LIBRARIES) -o vxp

vxp.o: vxp.c
	$(CC) $(CCFLAGS) -c $<

