CC := gcc
CFLAGS += -Wall
SRCS := command net cluster
OBJS := $(addsuffix .o,$(SRCS))

define link
	@mkdir -p bin
	$(strip $(LINK.o)) $^ $(LOADLIBES) $(LDLIBS) -o $@
endef

build: bin/exe bin/cli

bin/exe: CFLAGS += -O2
bin/exe: LDLIBS += -lpthread
bin/exe: main.o $(OBJS)
	$(call link)

bin/cli: CFLAGS += -O2
bin/cli: client.o $(OBJS)
	$(call link)

debug: bin/dexe bin/dcli

bin/dexe: CFLAGS += -g
bin/dexe: CPPFLAGS += -DDEBUG
bin/dexe: CPPFLAGS += -DMAX_CONCURRENCY=1
bin/dexe: CPPFLAGS += -DMAX_TIMEOUT_SEC=300
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
