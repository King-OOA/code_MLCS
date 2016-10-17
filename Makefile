OBJS = print.o suc_tabs.o graph.o 
LIBS = -ladt -lbits -lpatset -lcommon -lmem  -lm

#OBJS := $(patsubst ./src/%.c, %.o, $(wildcard ./src/*.c))
CC = gcc

LIBPATH = /home/pz/code/libs/a/

INCLUDE_1 = ../libs/include
INCLUDE_2 = ./include

LIBPATH_A = /home/pz/code/libs/a/


vpath %.h $(INCLUDE_1) $(INCLUDE_2)
vpath %.c ./src

CFLAGS = -Wall -c -std=c99 -O3  -I$(INCLUDE_1) -I$(INCLUDE_2)


# 库的出现位置要在.o文件之后
mlcs_new: main.o $(OBJS)
	$(CC) $^ -static -L$(LIBPATH) $(LIBS) -o $@
	rm *.o
	scp -P 12306 $@ guansw@10.171.1.3:/home/guansw/PengZhan

$(OBJS): mlcs.h suc_tabs.h graph.h

main.o: main.c print.h
	$(CC) $(CFLAGS) $< -o $@

$(OBJS): %.o: %.c %.h
	$(CC) $(CFLAGS) $< -o $@

.PHONY: ra
ra : ro
	-rm mlcs
ro:
	-rm $(OBJS)
