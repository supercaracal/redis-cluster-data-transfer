CC    := gcc
SHELL := /bin/bash
SRCS  := net command command_raw cluster
OBJS  := $(addsuffix .o,$(SRCS))

TEST_SRCS := command_raw
TEST_OBJS := $(addsuffix _test.o,$(TEST_SRCS))

CFLAGS += -std=c11 -D_POSIX_C_SOURCE=200809
CFLAGS += -Wall -Wextra -Wpedantic

WORKER   ?= 8
TIMEOUT  ?= 5
PIPELINE ?= 10

define link
	@mkdir -p bin
	$(strip $(LINK.o)) $^ $(LOADLIBES) $(LDLIBS) -o $@
endef

build: bin/exe bin/diff bin/cli bin/setter bin/getter

bin/exe: CFLAGS += -O2
bin/exe: CPPFLAGS += -DMAX_CONCURRENCY=$(WORKER)
bin/exe: CPPFLAGS += -DMAX_TIMEOUT_SEC=$(TIMEOUT)
bin/exe: CPPFLAGS += -DPIPELINING_SIZE=$(PIPELINE)
bin/exe: LDLIBS += -lpthread
bin/exe: main.o copy.o $(OBJS)
	$(call link)

bin/diff: CFLAGS += -O2
bin/diff: CPPFLAGS += -DMAX_CONCURRENCY=$(WORKER)
bin/diff: CPPFLAGS += -DMAX_TIMEOUT_SEC=$(TIMEOUT)
bin/diff: CPPFLAGS += -DPIPELINING_SIZE=$(PIPELINE)
bin/diff: LDLIBS += -lpthread
bin/diff: diff.o $(OBJS)
	$(call link)

bin/cli: CFLAGS += -O2
bin/cli: client.o $(OBJS)
	$(call link)

bin/setter: CFLAGS += -O2
bin/setter: setter.o $(OBJS)
	$(call link)

bin/getter: CFLAGS += -O2
bin/getter: getter.o $(OBJS)
	$(call link)

debug: bin/dexe bin/ddiff bin/dcli

bin/dexe: CFLAGS += -g
bin/dexe: CPPFLAGS += -DDEBUG
bin/dexe: CPPFLAGS += -DMAX_CONCURRENCY=1
bin/dexe: CPPFLAGS += -DMAX_TIMEOUT_SEC=300
bin/dexe: CPPFLAGS += -DPIPELINING_SIZE=2
bin/dexe: LDLIBS += -lpthread
bin/dexe: main.o copy.o $(OBJS)
	$(call link)

bin/ddiff: CFLAGS += -g
bin/ddiff: CPPFLAGS += -DDEBUG
bin/ddiff: CPPFLAGS += -DMAX_CONCURRENCY=1
bin/ddiff: CPPFLAGS += -DMAX_TIMEOUT_SEC=300
bin/ddiff: CPPFLAGS += -DPIPELINING_SIZE=2
bin/ddiff: LDLIBS += -lpthread
bin/ddiff: diff.o $(OBJS)
	$(call link)

bin/dcli: CFLAGS += -g
bin/dcli: CPPFLAGS += -DDEBUG
bin/dcli: client.o $(OBJS)
	$(call link)

test: bin/test

bin/test: CFLAGS += -g
bin/test: CPPFLAGS += -DTEST
bin/test: test.o $(TEST_OBJS) $(OBJS)
	$(call link)

lint:
	@type cpplint
	@cpplint *.h *.c

clean:
	@rm -rf bin *.o

.PHONY: build debug test lint clean
