CC := gcc
CFLAGS += -Wall
MY_TARGET_FILES := main.o command.o net.o cluster.o

build: bin/exe

bin/exe: CFLAGS += -O2
bin/exe: $(MY_TARGET_FILES)
	@mkdir -p bin
	$(strip $(LINK.c)) $(OUTPUT_OPTION) $^

bin/dbg: CFLAGS += -g
bin/dbg: CPPFLAGS += -DDEBUG
bin/dbg: OUTPUT_OPTION = -o $@
bin/dbg: $(MY_TARGET_FILES)
	@mkdir -p bin
	$(strip $(LINK.c)) $(OUTPUT_OPTION) $^

clean:
	@rm -rf bin *.o

.PHONY: clean
