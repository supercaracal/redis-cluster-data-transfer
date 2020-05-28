CC := gcc
CFLAGS += -Wall
SRCS := command net cluster
OBJS := $(addsuffix .o,$(SRCS))

define link
	@mkdir -p bin
	$(strip $(LINK.c)) $(OUTPUT_OPTION) $^
endef

build: bin/exe bin/cli

bin/exe: CFLAGS += -O2
bin/exe: main.o $(OBJS)
	$(call link)

bin/cli: CFLAGS += -O2
bin/cli: client.o $(OBJS)
	$(call link)

debug: bin/exed bin/clid

bin/exed: CFLAGS += -g
bin/exed: CPPFLAGS += -DDEBUG
bin/exed: main.o $(OBJS)
	$(call link)

bin/clid: CFLAGS += -g
bin/clid: CPPFLAGS += -DDEBUG
bin/clid: client.o $(OBJS)
	$(call link)

clean:
	@rm -rf bin *.o

.PHONY: build debug clean
