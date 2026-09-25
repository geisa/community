# Makefile
#
# Build and validate the standalone geisa-test Community application against
# the pinned GEISA v0.9.0 schema set.
#
# Copyright 2026 PragSol Consulting LLC.
# SPDX-License-Identifier: Apache-2.0

.DEFAULT_GOAL := all

APP := geisa-test
APP_VERSION := 0.8.9
GEISA_BASELINE := 0.9.0
GEISA_SCHEMAS_REPO ?= https://github.com/geisa/schemas.git
GEISA_SCHEMAS_REF ?= v0.9.0
GEISA_SCHEMAS_DIR ?=
NANOPB_VERSION ?= 0.4.9.1
BUILD_DIR ?= build
DEPS_DIR ?= .deps
SCHEMAS_DIR := $(if $(strip $(GEISA_SCHEMAS_DIR)),$(GEISA_SCHEMAS_DIR),$(DEPS_DIR)/geisa-schemas)
SCHEMA_SOURCE_DIR := $(SCHEMAS_DIR)
NANOPB_DIR := $(DEPS_DIR)/nanopb
NANOPB_RUNTIME_DIR ?= $(NANOPB_DIR)
VENV_DIR := .venv
PYTHON := $(VENV_DIR)/bin/python
NANOPB_GENERATOR := $(PYTHON) -m nanopb.generator.nanopb_generator
PROTOC ?= protoc
PROTOC_VERSION := $(shell $(PROTOC) --version 2>/dev/null | awk '{print $$2}')
PROTOC_FLAGS ?=
ifeq ($(shell printf "3.12.0\n$(PROTOC_VERSION)\n" | sort -V | head -n1),3.12.0)
  ifneq ($(shell printf "$(PROTOC_VERSION)\n3.15.0\n" | sort -V | head -n1),3.15.0)
    PROTOC_FLAGS += --experimental_allow_proto3_optional
  endif
endif

PROTO_FILES := geisa-status.proto conn-status.proto app-message.proto \
               manifest.proto discovery.proto sensor.proto waveform.proto
NANOPB_BUILD_DIR := $(BUILD_DIR)/nanopb
NANOPB_FILES := $(PROTO_FILES:%.proto=$(NANOPB_BUILD_DIR)/%.pb.c)
NANOPB_HEADERS := $(NANOPB_FILES:.c=.h)
NANOPB_STAMP := $(NANOPB_BUILD_DIR)/.generated
APP_SOURCES := src/geisa-test-message-contract.c src/geisa-protobuf.c \
               src/geisa-test.c
APP_OBJECTS := $(APP_SOURCES:src/%.c=$(BUILD_DIR)/%.o)
MOSQUITTO_CFLAGS := $(shell pkg-config --cflags libmosquitto 2>/dev/null)
MOSQUITTO_LIBS := $(shell pkg-config --libs libmosquitto 2>/dev/null)
CFLAGS ?= -O2 -Wall -Wextra -Werror -std=c11 -D_POSIX_C_SOURCE=200809L
CFLAGS += -pthread -Isrc -I$(NANOPB_BUILD_DIR) -I$(NANOPB_RUNTIME_DIR) \
	$(MOSQUITTO_CFLAGS)
LDLIBS += $(MOSQUITTO_LIBS)
TESTS := config downstream message-contract payload-ownership probe response-accounting \
         lifecycle-manifest reporting summary
TEST_BINS := $(TESTS:%=$(BUILD_DIR)/smoke-%)
DEPFILES := $(APP_OBJECTS:.o=.d) $(TEST_BINS:=.d)

.PHONY: all app setup-dev check-dev-deps regenerate verify-generated \
        dependency-info write-dependency-info test test-api test-lee \
        test-lifecycle test-summary clean help

all: app

app: check-dev-deps $(BUILD_DIR)/geisa-test

$(BUILD_DIR)/geisa-test: $(APP_OBJECTS) $(NANOPB_FILES)
	$(CC) $(CFLAGS) -o $@ $(APP_OBJECTS) $(NANOPB_FILES) \
		$(NANOPB_RUNTIME_DIR)/pb_common.c \
		$(NANOPB_RUNTIME_DIR)/pb_encode.c \
		$(NANOPB_RUNTIME_DIR)/pb_decode.c $(LDLIBS)

$(BUILD_DIR)/%.o: src/%.c $(NANOPB_HEADERS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -MMD -MP -MF $(BUILD_DIR)/$*.d -MT $@ -c $< -o $@

$(NANOPB_STAMP): $(PROTO_FILES:%=$(SCHEMA_SOURCE_DIR)/%) \
		$(SCHEMA_SOURCE_DIR)/nanopb_options/*.options $(PYTHON)
	@mkdir -p $(NANOPB_BUILD_DIR)
	$(NANOPB_GENERATOR) -D $(NANOPB_BUILD_DIR) \
		-I $(SCHEMA_SOURCE_DIR)/nanopb_options \
		--protoc-opt=-I$(SCHEMA_SOURCE_DIR) \
		$(foreach flag,$(PROTOC_FLAGS),--protoc-opt=$(flag)) \
		$(PROTO_FILES:%=$(SCHEMA_SOURCE_DIR)/%)
	$(MAKE) --no-print-directory write-dependency-info
	@touch $@

$(NANOPB_FILES) $(NANOPB_HEADERS): $(NANOPB_STAMP)

check-dev-deps:
	@test -f "$(SCHEMA_SOURCE_DIR)/geisa-status.proto" -a \
	     -f "$(SCHEMA_SOURCE_DIR)/app-message.proto" -a \
	     -f "$(NANOPB_RUNTIME_DIR)/pb_encode.c" -a -x "$(PYTHON)" || { \
		echo 'Run make setup-dev to install the pinned GEISA schemas and nanopb.' >&2; exit 2; }
	@pkg-config --exists libmosquitto || { \
		echo 'Install libmosquitto development files and pkg-config.' >&2; exit 2; }

setup-dev:
	GEISA_SCHEMAS_REPO='$(GEISA_SCHEMAS_REPO)' \
	GEISA_SCHEMAS_REF='$(GEISA_SCHEMAS_REF)' \
	GEISA_SCHEMAS_DIR='$(GEISA_SCHEMAS_DIR)' \
	NANOPB_VERSION='$(NANOPB_VERSION)' sh ./scripts/setup-dev.sh

regenerate: check-dev-deps
	@rm -f $(NANOPB_STAMP)
	@$(MAKE) --no-print-directory $(NANOPB_STAMP)

verify-generated: check-dev-deps $(NANOPB_STAMP)
	@for f in $(NANOPB_FILES) $(NANOPB_HEADERS); do test -s "$$f" || exit 1; done
	@$(MAKE) --no-print-directory write-dependency-info
	@test -s $(BUILD_DIR)/dependency-info.txt
	@printf 'Generated GEISA %s nanopb bindings are present.\n' '$(GEISA_BASELINE)'

# Add each focused test source to TESTS and its grouped test target below.
$(BUILD_DIR)/smoke-%: tools/geisa-test-%-smoke.c $(NANOPB_FILES) $(NANOPB_HEADERS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -MMD -MP -MF $(BUILD_DIR)/smoke-$*.d -MT $@ \
		-o $@ $< src/geisa-test-message-contract.c \
		src/geisa-protobuf.c $(NANOPB_FILES) \
		$(NANOPB_RUNTIME_DIR)/pb_common.c \
		$(NANOPB_RUNTIME_DIR)/pb_encode.c \
		$(NANOPB_RUNTIME_DIR)/pb_decode.c $(LDLIBS)

test-api: check-dev-deps $(BUILD_DIR)/smoke-config $(BUILD_DIR)/smoke-downstream \
		$(BUILD_DIR)/smoke-message-contract $(BUILD_DIR)/smoke-payload-ownership \
		$(BUILD_DIR)/smoke-response-accounting $(BUILD_DIR)/smoke-reporting
	@$(BUILD_DIR)/smoke-config
	@$(BUILD_DIR)/smoke-downstream
	@$(BUILD_DIR)/smoke-message-contract
	@$(BUILD_DIR)/smoke-payload-ownership
	@$(BUILD_DIR)/smoke-response-accounting
	@$(BUILD_DIR)/smoke-reporting

test-lee: check-dev-deps $(BUILD_DIR)/smoke-probe
	@$(BUILD_DIR)/smoke-probe

test-lifecycle: check-dev-deps $(BUILD_DIR)/smoke-lifecycle-manifest
	@$(BUILD_DIR)/smoke-lifecycle-manifest

test-summary: check-dev-deps $(BUILD_DIR)/smoke-summary
	@$(BUILD_DIR)/smoke-summary

test: app test-api test-lee test-lifecycle test-summary

-include $(DEPFILES)

write-dependency-info:
	@test -d "$(SCHEMA_SOURCE_DIR)" || { \
		echo 'GEISA schemas checkout is unavailable; run make setup-dev or set' \
			'GEISA_SCHEMAS_DIR' >&2; exit 2; }
	@test -d "$(NANOPB_DIR)" || { \
		echo 'nanopb checkout is unavailable; run make setup-dev' >&2; exit 2; }
	@schema_commit=$$(git -C "$(SCHEMAS_DIR)" rev-parse HEAD 2>/dev/null || printf local); \
	 schema_ref=$$(git -C "$(SCHEMAS_DIR)" symbolic-ref --short -q HEAD 2>/dev/null || true); \
	 if [ -z "$$schema_ref" ]; then schema_ref=$$(git -C "$(SCHEMAS_DIR)" describe --tags --exact-match HEAD 2>/dev/null || true); fi; \
	 if [ -z "$$schema_ref" ]; then schema_ref=detached-or-local; fi; \
	 nanopb_commit=$$(git -C "$(NANOPB_DIR)" rev-parse HEAD 2>/dev/null || printf local); \
	 mkdir -p "$(BUILD_DIR)"; \
	 { printf 'schemas-source=%s\n' "$(if $(strip $(GEISA_SCHEMAS_DIR)),local checkout,managed checkout)"; \
	   printf 'schemas-repository=%s\n' '$(GEISA_SCHEMAS_REPO)'; \
	   printf 'schemas-path=%s\n' '$(SCHEMAS_DIR)'; \
	   printf 'schemas-commit=%s\n' "$$schema_commit"; \
	   printf 'schemas-ref=%s\n' "$$schema_ref"; \
	   printf 'nanopb-source=managed checkout\n'; \
	   printf 'nanopb-path=%s\n' '$(NANOPB_DIR)'; \
	   printf 'nanopb-commit=%s\n' "$$nanopb_commit"; \
	   printf 'nanopb-version=%s\n' '$(NANOPB_VERSION)'; \
	 } > "$(BUILD_DIR)/dependency-info.txt"

dependency-info: write-dependency-info
	@cat $(BUILD_DIR)/dependency-info.txt

clean:
	rm -rf $(BUILD_DIR)

help:
	@printf '%s\n' 'geisa-test $(APP_VERSION)' \
	  'make setup-dev       Fetch geisa/schemas v0.9.0 and nanopb.' \
	  'make app             Build the application.' \
	  'make test            Build and run all local tests.' \
	  'make test-api        Run app-message, CONFIG, COMMAND, and accounting tests.' \
	  'make test-lee        Run bounded resource-probe tests.' \
	  'make test-lifecycle  Run lifecycle and manifest protocol tests.' \
	  'make test-summary    Validate the stdout JSON result format.' \
	  'make regenerate      Regenerate bindings from selected schemas.' \
	  'make verify-generated Check generated bindings and record dependencies.' \
	  'make dependency-info Show the schema and nanopb revisions in use.' \
	  'make clean           Remove generated local build output.'
