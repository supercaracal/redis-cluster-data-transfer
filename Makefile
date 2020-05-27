CC := gcc
CFLAGS += -Wall
SRCS := main command net cluster
OBJS := $(addsuffix .o,$(SRCS))

build: bin/exe

debug: bin/dbg

bin/exe: CFLAGS += -O2
bin/exe: $(OBJS)
	@mkdir -p bin
	$(strip $(LINK.c)) $(OUTPUT_OPTION) $^

bin/dbg: CFLAGS += -g
bin/dbg: CPPFLAGS += -DDEBUG
bin/dbg: $(OBJS)
	@mkdir -p bin
	$(strip $(LINK.c)) $(OUTPUT_OPTION) $^

clean:
	@rm -rf bin *.o

.PHONY: build debug clean
