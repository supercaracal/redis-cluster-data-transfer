CC := gcc
CFLAGS += -Wall
SRCS := command net cluster
OBJS := $(addsuffix .o,$(SRCS))

build: bin/exe

debug: bin/dbg

client: bin/cli

bin/exe: CFLAGS += -O2
bin/exe: main.o $(OBJS)
	@mkdir -p bin
	$(strip $(LINK.c)) $(OUTPUT_OPTION) $^

bin/dbg: CFLAGS += -g
bin/dbg: CPPFLAGS += -DDEBUG
bin/dbg: main.o $(OBJS)
	@mkdir -p bin
	$(strip $(LINK.c)) $(OUTPUT_OPTION) $^

bin/cli: CFLAGS += -g
bin/cli: CPPFLAGS += -DDEBUG
bin/cli: client.o $(OBJS)
	@mkdir -p bin
	$(strip $(LINK.c)) $(OUTPUT_OPTION) $^

clean:
	@rm -rf bin *.o

.PHONY: build debug client clean
