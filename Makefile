CC   := gcc
SRCS := command command_raw net cluster copy
OBJS := $(addsuffix .o,$(SRCS))

CFLAGS += -std=c11 -D_POSIX_C_SOURCE=200809
CFLAGS += -Wall -Wextra -Wpedantic

WORKER   ?= 8
TIMEOUT  ?= 15
PIPELINE ?= 20

define link
	@mkdir -p bin
	$(strip $(LINK.o)) $^ $(LOADLIBES) $(LDLIBS) -o $@
endef

build: bin/exe bin/cli bin/setter bin/getter

bin/exe: CFLAGS += -O2
bin/exe: CPPFLAGS += -DMAX_CONCURRENCY=$(WORKER)
bin/exe: CPPFLAGS += -DMAX_TIMEOUT_SEC=$(TIMEOUT)
bin/exe: CPPFLAGS += -DPIPELINING_SIZE=$(PIPELINE)
bin/exe: LDLIBS += -lpthread
bin/exe: main.o $(OBJS)
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

debug: bin/dexe bin/dcli

bin/dexe: CFLAGS += -g
bin/dexe: CPPFLAGS += -DDEBUG
bin/dexe: CPPFLAGS += -DMAX_CONCURRENCY=1
bin/dexe: CPPFLAGS += -DMAX_TIMEOUT_SEC=300
bin/dexe: CPPFLAGS += -DPIPELINING_SIZE=2
bin/dexe: LDLIBS += -lpthread
bin/dexe: main.o $(OBJS)
	$(call link)

bin/dcli: CFLAGS += -g
bin/dcli: CPPFLAGS += -DDEBUG
bin/dcli: client.o $(OBJS)
	$(call link)

clean:
	@rm -rf bin *.o

.PHONY: build debug clean
