CC := gcc
CFLAGS += -Wall
CFLAGS += -g
CPPFLAGS += -DDEBUG

all: bin/exe

main.o: main.c

command.o: command.c

net.o: net.c

cluster.o: cluster.c

bin/exe: OUTPUT_OPTION = -o $@
bin/exe: main.o command.o net.o cluster.o
	@mkdir -p bin
	$(strip $(LINK.c)) $(OUTPUT_OPTION) $^
