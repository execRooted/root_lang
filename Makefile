# root_lang build configuration.
#
# Targets:
#   make            build the compiler into bin/rootc
#   make install    install to $(PREFIX) (default /usr/local)
#   make test       build and run the example test-suite
#   make clean      remove build artefacts

CC      ?= cc
CFLAGS  ?= -std=c11 -O2 -Wall -Wextra -Wno-unused-parameter
PREFIX  ?= /usr/local

BIN     := bin/rootc
SRC     := $(wildcard src/*.c)
OBJ     := $(SRC:src/%.c=build/%.o)

.PHONY: all clean install uninstall test

all: $(BIN)

build:
	@mkdir -p build

bin:
	@mkdir -p bin

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN): $(OBJ) | bin
	$(CC) $(CFLAGS) $(OBJ) -o $(BIN)
	@echo "built $(BIN)"

install: all
	install -d $(PREFIX)/bin
	install -m755 $(BIN) $(PREFIX)/bin/rootc
	install -d $(PREFIX)/lib/root_lang/runtime
	install -m644 runtime/rootrt.c runtime/rootrt.h $(PREFIX)/lib/root_lang/runtime/
	install -d $(PREFIX)/lib/root_lang/stdlib
	install -m644 stdlib/root.rtl $(PREFIX)/lib/root_lang/stdlib/
	@echo "installed to $(PREFIX)"
	@echo "set ROOTLANG_HOME=$(PREFIX)/lib/root_lang when compiling"

uninstall:
	rm -f $(PREFIX)/bin/rootc
	rm -rf $(PREFIX)/lib/root_lang

test: all
	./run_tests.sh

clean:
	rm -rf build bin
	@echo "cleaned"
