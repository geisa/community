# Copyright (C) 2025 Southern California Edison

all: gapi

.PHONY: schemas clean

DESTDIR ?= /usr/bin
CFLAGS := -O2 -Wall -std=c11 -Wextra -pedantic -Werror
LDLIBS := -lmosquitto

gapi: %: %.c gapi_mosquitto.o | schemas

schemas:
	$(MAKE) -C schemas

install: gapi
	install -D -m 755 gapi $(DESTDIR)

lint:
	clang-format --Werror --dry-run *.c *.h
	clang-tidy *.c *.h

clean:
	$(MAKE) -C schemas clean
	rm -f gapi gapi_mosquitto.o
