# Copyright (C) 2025 Southern California Edison

BUILDIR ?= build
DESTDIR ?= /usr/bin
PROTOS := $(wildcard schemas/*.proto)
PB_C := $(PROTOS:schemas/%.proto=$(BUILDIR)/schemas/%.pb-c.c)

SRC := $(wildcard src/*.c)

OBJ := $(patsubst src/%.c,$(BUILDIR)/%.o,$(SRC)) $(PB_C:.c=.o)
CFLAGS := -O2 -Wall -std=c11 -Wextra -pedantic -Werror -Iinclude -I$(BUILDIR) $(shell pkg-config --cflags 'libprotobuf-c >= 1.0.0')
LDLIBS := -lmosquitto $(shell pkg-config --libs 'libprotobuf-c >= 1.0.0')

all: $(BUILDIR)/gapi
.PHONY: schemas build_dir clean

build_dir:
	@mkdir -p $(BUILDIR)

$(BUILDIR)/schemas/%.pb-c.o: %.c | build_dir schemas
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDIR)/%.o: src/%.c | build_dir schemas
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDIR)/gapi: $(OBJ)
	$(CC) $(CFLAGS) -o $@ gapi.c $(OBJ) $(LDLIBS)

schemas:
	protoc --c_out=build schemas/*.proto

install: gapi
	install -D -m 755 $(BUILDIR)/gapi $(DESTDIR)

lint:
	clang-format --Werror --dry-run *.c src/*.c include/*.h
	clang-tidy *.c src/*.c include/*.h -- -Iinclude -I$(BUILDIR)

clean:
	rm -rf $(BUILDIR)
