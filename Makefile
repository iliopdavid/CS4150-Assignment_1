PROGS = sandbox.so

CC = g++ # clang++

CLEANFILES = $(PROGS) *.o *.d

SRCDIR ?= ./

NO_MAN=
CFLAGS = -O0 # maybe we want some additional like -ggdb
CFLAGS += -Werror -Wall -Wunused-function -Wno-deprecated -std=c++20
CFLAGS += -Wextra
CFLAGS += -shared -fPIC

C_SRCS = sandbox.cpp
OBJS = $(C_SRCS:.c=.o)

.PHONY: all
all: $(PROGS)

$(PROGS): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	-@rm -rf $(CLEANFILES)
