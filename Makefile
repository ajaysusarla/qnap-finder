# Platform
CC=gcc
DEBUG=-ggdb
CFLAGS=-Wall -Wno-pointer-sign $(DEBUG)
LDFLAGS=
NULL=

UNAME := $(shell $(CC) -dumpmachine 2>&1 | grep -E -o "linux|darwin")

ifeq ($(UNAME), darwin)
        OSSUPPORT = darwin
        OSSUPPORT_CFLAGS += -DDARWIN
endif

CFLAGS += $(OSSUPPORT_CFLAGS)
LDFLAGS += -lpthread -lm

all: qnap-finder

qnap-finder.o: qnap-finder.c qnap-finder.h
	$(CC) $(CFLAGS) -c -o qnap-finder.o qnap-finder.c

list.o: list.c list.h
	$(CC) $(CFLAGS) -c -o list.o list.c

qnap-finder: qnap-finder.o list.o
	$(CC) $(CFLAGS) -o qnap-finder qnap-finder.o list.o $(LDFLAGS)

clean:
	rm -f *.o qnap-finder
