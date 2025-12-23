# Copyright (C) 2025 Southern California Edison

BUILDIR ?= build
DESTDIR ?= /usr/bin
SRC := $(wildcard src/*.c)
OBJ = $(patsubst src/%.c,$(BUILDIR)/%.o,$(SRC))
CFLAGS := -O2 -Wall -std=c11 -Wextra -pedantic -Werror -Iinclude
LDLIBS := -lmosquitto

all: $(BUILDIR)/gapi
.PHONY: schemas build_dir clean

build_dir:
	@mkdir -p $(BUILDIR)

$(BUILDIR)/%.o: src/%.c | build_dir
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDIR)/gapi: $(OBJ) | schemas
	$(CC) $(CFLAGS) -o $@ gapi.c $(OBJ) $(LDLIBS)

schemas:
	$(MAKE) -C schemas

install: gapi
	install -D -m 755 $(BUILDIR)/gapi $(DESTDIR)

lint:
	clang-format --Werror --dry-run *.c src/*.c include/*.h
	clang-tidy *.c src/*.c include/*.h -- -Iinclude

clean:
	$(MAKE) -C schemas clean
	rm -rf $(BUILDIR)
