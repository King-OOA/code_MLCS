OBJS = suc_tabs.o graph.o
LIBS = -ladt -lbits -lpatset -lcommon -lmem  -lm

#OBJS := $(patsubst ./src/%.c, %.o, $(wildcard ./src/*.c))
CC = gcc

INCLUDE_1 = ../libs/include
INCLUDE_2 = ./include

vpath %.h $(INCLUDE_1) $(INCLUDE_2)
vpath %.c ./src

CFLAGS = -Wall -c -std=c99 -O3 -I$(INCLUDE_1) -I$(INCLUDE_2)

mlcs: main.o $(OBJS) $(LIBS)
	$(CC) $^ -o $@
	rm *.o

$(OBJS):  mlcs.h suc_tabs.h graph.h

main.o: main.c 
	$(CC) $(CFLAGS) $< -o $@

$(OBJS): %.o: %.c %.h
	$(CC) $(CFLAGS) $< -o $@

.PHONY: ra
ra : ro
	-rm a.test.out
ro:
	-rm $(OBJS)
