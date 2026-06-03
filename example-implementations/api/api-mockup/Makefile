# Copyright (C) 2025 Southern California Edison

include nanopb/extra/nanopb.mk

BUILDIR ?= build
DESTDIR ?= /usr/bin
PROTOS := $(wildcard schemas/*.proto)
PB_C := $(PROTOS:schemas/%.proto=$(BUILDIR)/schemas/%.pb.c)

SRC := $(wildcard src/*.c)

OBJ := $(patsubst src/%.c,$(BUILDIR)/%.o,$(SRC)) $(PB_C:.c=.o) $(NANOPB_CORE:.c=.o)
CFLAGS := -O2 -Wall -std=c11 -Wextra -pedantic -Werror -Iinclude -I$(BUILDIR) -I/usr/include/nanopb -DPB_ENABLE_MALLOC
LDLIBS := -lmosquitto

all: $(BUILDIR)/gapi $(BUILDIR)/test_app
.PHONY: schemas build_dir clean

build_dir:
	@mkdir -p $(BUILDIR)

$(BUILDIR)/schemas/%.pb.o: %.c | build_dir schemas
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDIR)/%.o: src/%.c | build_dir schemas
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDIR)/gapi: $(OBJ)
	$(CC) $(CFLAGS) -o $@ gapi.c $(OBJ) $(LDLIBS)

schemas:
	@mkdir -p $(BUILDIR)/schemas
	protoc --nanopb_out=$(BUILDIR)/schemas schemas/*.proto -I schemas --nanopb_opt='-Inanopb_options'

install: gapi
	install -D -m 755 $(BUILDIR)/gapi $(DESTDIR)

lint:
	clang-format --Werror --dry-run *.c src/*.c include/*.h
	clang-tidy *.c src/*.c include/*.h -- -Iinclude -I$(BUILDIR) -I nanopb

clean:
	rm -rf $(BUILDIR)
	rm -f nanopb/*.o

$(BUILDIR)/test_app: $(OBJ)
	$(CC) $(CFLAGS) -o $@ test/test_app.c $(PB_C:.c=.o) $(NANOPB_CORE:.c=.o) $(LDLIBS)
