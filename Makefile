CC := gcc
CFLAGS += -Wall
MY_SRC := main command net cluster
MY_OBJ := $(addsuffix .o,$(MY_SRC))

build: bin/exe

debug: bin/dbg

bin/exe: CFLAGS += -O2
bin/exe: $(MY_OBJ)
	@mkdir -p bin
	$(strip $(LINK.c)) $(OUTPUT_OPTION) $^

bin/dbg: CFLAGS += -g
bin/dbg: CPPFLAGS += -DDEBUG
bin/dbg: OUTPUT_OPTION = -o $@
bin/dbg: $(MY_OBJ)
	@mkdir -p bin
	$(strip $(LINK.c)) $(OUTPUT_OPTION) $^

clean:
	@rm -rf bin *.o

.PHONY: clean
