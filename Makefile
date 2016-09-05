#OBJS = 
LIBS = -ladt -lbits -lpatset -lcommon -lmem  -lm

#OBJS := $(patsubst ./src/%.c, %.o, $(wildcard ./src/*.c))
CC = gcc

INCLUDE_1 = ../libs/include
INCLUDE_2 = ./include

vpath %.h $(INCLUDE_1) $(INCLUDE_2)
vpath %.c ./src

CFLAGS = -Wall -c -std=c99 -g -I$(INCLUDE_1) -I$(INCLUDE_2)

mlcs: main.o $(OBJS) $(LIBS)
	$(CC) $^ -o $@
	rm *.o

$(OBJS):  mem.h common.h

main.o: main.c common.h
	$(CC) $(CFLAGS) $< -o $@


# $(OBJS): %.o: %.c %.h
# 	$(CC) $(CFLAGS) $< -o $@

.PHONY: ra
ra : ro
	-rm a.test.out
ro:
	-rm $(OBJS)
