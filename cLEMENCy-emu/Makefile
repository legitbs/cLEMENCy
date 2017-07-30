SVCNAME = clemency-emu

EXTRAFLAGS = -O2 -funroll-loops -m64 -Wno-unused-parameter

#DEFINES=-DNOTIME

CC = clang-3.9
AS = clang-3.9
CFLAGS = -g -Wall -Wshadow -Wextra -std=gnu99 $(EXTRAFLAGS)

# This makes it so that each c file is compiled in the directory
CFILES = $(wildcard *.c)
OBJECTS = $(filter-out debug.o debug-cmds.o disassembler.o recordstate.o,$(patsubst %.c,%.o,$(CFILES)))
DEBUGOBJECTS = $(patsubst %.c,%-debug.o,$(CFILES))
DEBUGOBJECTSTEAM = $(filter-out recordstate-debug-team.o,$(patsubst %.c,%-debug-team.o,$(CFILES)))
LIBS = -lreadline -lseccomp

$(shell touch disassembler.c)

$(SVCNAME): $(SVCNAME)-debug $(SVCNAME)-debug-team $(OBJECTS) $(AS_OBJECTS) disassembler_opcodes.h
	$(CC) $(CFLAGS) $(DEFINES) -DNODEBUG $(OBJECTS) -o $(SVCNAME) $(LIBS)
	make strip

$(SVCNAME)-debug: $(DEBUGOBJECTS) $(AS_OBJECTS) disassembler_opcodes.h
	$(CC) $(CFLAGS) $(DEFINES) $(DEBUGOBJECTS) -o $(SVCNAME)-debug $(LIBS)
	make strip-debug

$(SVCNAME)-debug-team: $(DEBUGOBJECTSTEAM) $(AS_OBJECTS) disassembler_opcodes.h
	$(CC) $(CFLAGS) $(DEFINES) $(DEBUGOBJECTSTEAM) -o $(SVCNAME)-debug-team $(LIBS)
	make strip-debug-team

disassembler-debug.o: disassembler.c
	python create-disas-data.py
	$(CC) $(CFLAGS) $(DEFINES) -c $< -o $@

disassembler.o: disassembler.c
	python create-disas-data.py
	$(CC) $(CFLAGS) $(DEFINES) -DNODEBUG -c $< -o $@

%-debug.o:  %.c Makefile
	$(CC) $(CFLAGS) $(DEFINES) -c $< -o $@

%-debug-team.o:  %.c Makefile
	$(CC) $(CFLAGS) $(DEFINES) -DTEAMBUILD -c $< -o $@

%.o:  %.c Makefile
	$(CC) $(CFLAGS) $(DEFINES) -DNODEBUG -c $< -o $@

clean:
	rm -rf $(SVCNAME) $(SVCNAME)-debug *.o

strip-debug-team:
	strip -s $(SVCNAME)-debug-team
	strip -g $(SVCNAME)-debug-team
	strip -S $(SVCNAME)-debug-team
	strip -d $(SVCNAME)-debug-team
	strip -x $(SVCNAME)-debug-team
	strip -X $(SVCNAME)-debug-team

strip-debug:
	strip -s $(SVCNAME)-debug
	strip -g $(SVCNAME)-debug
	strip -S $(SVCNAME)-debug
	strip -d $(SVCNAME)-debug
	strip -x $(SVCNAME)-debug
	strip -X $(SVCNAME)-debug

strip:
	strip -s $(SVCNAME)
	strip -g $(SVCNAME)
	strip -S $(SVCNAME)
	strip -d $(SVCNAME)
	strip -x $(SVCNAME)
	strip -X $(SVCNAME)

.PHONY: clean
