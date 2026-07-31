# Copyright (C) 2025 Southern California Edison

include nanopb/extra/nanopb.mk

BUILDIR ?= build
DESTDIR ?= /usr/bin
VENV ?= .venv
PYTHON ?= python3
PROTOC_CMD ?= protoc

VENV_BIN := $(CURDIR)/$(VENV)/bin
NANOPB_PLUGIN := $(CURDIR)/nanopb/generator/protoc-gen-nanopb
PROTOC_FLAGS := \
	--experimental_allow_proto3_optional \
	--plugin=protoc-gen-nanopb=$(NANOPB_PLUGIN)

MOSQUITTO_CFLAGS := $(shell pkg-config --cflags libmosquitto)
MOSQUITTO_LIBS := $(shell pkg-config --libs libmosquitto)

PROTOS := $(wildcard schemas/*.proto)
PB_C := $(PROTOS:schemas/%.proto=$(BUILDIR)/schemas/%.pb.c)
SCHEMA_STAMP := $(BUILDIR)/schemas/.generated
SRC := $(wildcard src/*.c)

OBJ := $(patsubst src/%.c,$(BUILDIR)/%.o,$(SRC)) \
	$(PB_C:.c=.o) \
	$(NANOPB_CORE:.c=.o)

CFLAGS ?= -O2 -Wall -std=c11 -Wextra -pedantic -Werror
CFLAGS += -Iinclude -I$(BUILDIR) -Inanopb $(MOSQUITTO_CFLAGS) \
	-DPB_ENABLE_MALLOC
LDLIBS += $(MOSQUITTO_LIBS)

export PATH := $(VENV_BIN):$(PATH)

.PHONY: all build_dir check-dev clean install lint schemas setup-dev

all: check-dev $(BUILDIR)/gapi $(BUILDIR)/test_app \
	$(BUILDIR)/test_app_actuator $(BUILDIR)/test_app_sensor

setup-dev:
	$(PYTHON) -m venv $(VENV)
	$(VENV_BIN)/python -m pip install --upgrade pip
	$(VENV_BIN)/python -m pip install -r nanopb/requirements.txt

check-dev:
	@test -x "$(VENV_BIN)/python" || { \
		echo "Development environment missing. Run 'make setup-dev' first."; \
		exit 1; \
	}

build_dir:
	@mkdir -p $(BUILDIR)

$(PB_C): $(SCHEMA_STAMP)

$(BUILDIR)/schemas/%.pb.o: $(BUILDIR)/schemas/%.pb.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDIR)/%.o: src/%.c | build_dir $(SCHEMA_STAMP)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDIR)/gapi: $(OBJ)
	$(CC) $(CFLAGS) -o $@ gapi.c $(OBJ) $(LDLIBS)

$(BUILDIR)/test_app: $(OBJ)
	$(CC) $(CFLAGS) -o $@ test/test_app.c $(PB_C:.c=.o) \
		$(NANOPB_CORE:.c=.o) $(LDLIBS)

$(BUILDIR)/test_app_actuator: $(OBJ)
	$(CC) $(CFLAGS) -o $@ test/test_app_actuator.c $(PB_C:.c=.o) \
		$(NANOPB_CORE:.c=.o) $(LDLIBS)

$(BUILDIR)/test_app_sensor: $(OBJ)
	$(CC) $(CFLAGS) -o $@ test/test_app_sensor.c $(PB_C:.c=.o) \
		$(NANOPB_CORE:.c=.o) $(LDLIBS)

schemas: $(SCHEMA_STAMP)

$(SCHEMA_STAMP): $(PROTOS) $(wildcard nanopb_options/*.options) | check-dev
	@mkdir -p $(BUILDIR)/schemas
	$(PROTOC_CMD) $(PROTOC_FLAGS) \
		--nanopb_out=$(BUILDIR)/schemas \
		-I schemas \
		--nanopb_opt='-Inanopb_options' \
		$(PROTOS)
	@touch $@

install: $(BUILDIR)/gapi
	install -D -m 755 $(BUILDIR)/gapi $(DESTDIR)/gapi

lint: schemas
	clang-format --Werror --dry-run *.c src/*.c include/*.h
	clang-tidy *.c src/*.c include/*.h -- \
		-Iinclude -I$(BUILDIR) -Inanopb $(MOSQUITTO_CFLAGS)

clean:
	rm -rf $(BUILDIR)
	rm -f nanopb/*.o
