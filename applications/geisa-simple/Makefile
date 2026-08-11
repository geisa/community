# File: Makefile
# Project: geisa-simple
# Purpose: Build the application and its generated GEISA protobuf bindings.
# Usage: make setup-dev, make, make test, or make manifest.
#
# Copyright 2026 PragSol Consulting LLC.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0

APP := geisa-simple
APP_VERSION := 0.9.0
GEISA_SCHEMAS_REF ?= v0.9.0
GEISA_SCHEMAS_DIR ?=
NANOPB_VERSION ?= 0.4.9.1
TOOLCHAIN_ID ?=
TOOLCHAIN_VERSION ?=
BUILD_DIR ?= build
DEPS_DIR ?= .deps
SCHEMAS_DIR := $(if $(strip $(GEISA_SCHEMAS_DIR)),\
                 $(GEISA_SCHEMAS_DIR),$(DEPS_DIR)/geisa-schemas)
NANOPB_DIR := $(DEPS_DIR)/nanopb
VENV_DIR := .venv
PYTHON := $(VENV_DIR)/bin/python
NANOPB_GENERATOR := $(PYTHON) -m nanopb.generator.nanopb_generator
NANOPB_BUILD_DIR := $(BUILD_DIR)/nanopb
NANOPB_RUNTIME_DIR ?= $(NANOPB_DIR)

PROTO_FILES := geisa-status.proto conn-status.proto app-message.proto \
               manifest.proto discovery.proto
TRANSITIVE_PROTO_FILES := sensor.proto waveform.proto
GENERATED_PROTO_FILES := $(PROTO_FILES) $(TRANSITIVE_PROTO_FILES)
NANOPB_FILES := $(GENERATED_PROTO_FILES:%.proto=$(NANOPB_BUILD_DIR)/%.pb.c)
NANOPB_HEADERS := $(NANOPB_FILES:.c=.h)
NANOPB_STAMP := $(NANOPB_BUILD_DIR)/.generated
COMMON_C := src/geisa-protobuf.c src/geisa-simple-config.c
APP_OBJECTS := $(COMMON_C:src/%.c=$(BUILD_DIR)/%.o) $(BUILD_DIR)/geisa-app.o
MOSQUITTO_CFLAGS := $(shell pkg-config --cflags libmosquitto 2>/dev/null)
MOSQUITTO_LIBS := $(shell pkg-config --libs libmosquitto 2>/dev/null)
CFLAGS ?= -O2 -Wall -Wextra -Werror -std=c11
CFLAGS += -Isrc -I$(NANOPB_BUILD_DIR) -I$(NANOPB_RUNTIME_DIR) \
          $(MOSQUITTO_CFLAGS)
LDLIBS += $(MOSQUITTO_LIBS)

.PHONY: all app test setup-dev regenerate verify-generated clean \
        check-dev-deps \
        dependency-info write-dependency-info manifest help

all: app

app: check-dev-deps $(BUILD_DIR)/geisa-app

$(BUILD_DIR)/geisa-app: $(APP_OBJECTS) $(NANOPB_FILES)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(APP_OBJECTS) $(NANOPB_FILES) \
		$(NANOPB_RUNTIME_DIR)/pb_common.c \
		$(NANOPB_RUNTIME_DIR)/pb_encode.c \
		$(NANOPB_RUNTIME_DIR)/pb_decode.c $(LDLIBS)

$(BUILD_DIR)/%.o: src/%.c $(NANOPB_HEADERS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/geisa-app.o: src/geisa-app.c $(NANOPB_HEADERS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

write-dependency-info:
	@test -d "$(SCHEMAS_DIR)" || { \
		echo 'GEISA schemas checkout is unavailable; run make setup-dev or set' \
			'GEISA_SCHEMAS_DIR' >&2; exit 2; }
	@test -d "$(NANOPB_DIR)" || { \
		echo 'nanopb checkout is unavailable; run make setup-dev' >&2; exit 2; }
	@test -x "$(PYTHON)" || { \
		echo 'nanopb Python environment is unavailable; run make setup-dev' \
			>&2; exit 2; }
	@schema_commit=$$(git -C "$(SCHEMAS_DIR)" rev-parse HEAD \
		2>/dev/null) || { \
		echo 'GEISA schemas directory is not a Git checkout' >&2; exit 2; }; \
	 schema_ref=$$(git -C "$(SCHEMAS_DIR)" symbolic-ref --short -q HEAD \
		2>/dev/null || true); \
	 if [ -z "$$schema_ref" ]; then schema_ref=$$(git -C "$(SCHEMAS_DIR)" \
		describe --tags --exact-match HEAD 2>/dev/null || true); fi; \
	 if [ -z "$$schema_ref" ]; then schema_ref=detached; fi; \
	 if [ -n "$(strip $(GEISA_SCHEMAS_DIR))" ]; then \
		schema_source='local checkout'; \
	 else schema_source='managed checkout'; fi; \
	 mkdir -p "$(BUILD_DIR)"; \
	 { printf 'schemas-source=%s\n' "$$schema_source"; \
	   printf 'schemas-path=%s\n' "$(SCHEMAS_DIR)"; \
	   printf 'schemas-commit=%s\n' "$$schema_commit"; \
	   printf 'schemas-ref=%s\n' "$$schema_ref"; \
	   printf 'nanopb-source=managed checkout\n'; \
	   printf 'nanopb-path=%s\n' "$(NANOPB_DIR)"; \
	   printf 'nanopb-commit='; git -C "$(NANOPB_DIR)" rev-parse HEAD; \
	   printf 'nanopb-version=%s\n' "$(NANOPB_VERSION)"; \
	 } > "$(BUILD_DIR)/dependency-info.txt"

$(NANOPB_STAMP): $(GENERATED_PROTO_FILES:%=$(SCHEMAS_DIR)/%) \
		$(SCHEMAS_DIR)/nanopb_options/*.options $(PYTHON)
	@mkdir -p $(NANOPB_BUILD_DIR)
	$(NANOPB_GENERATOR) -D $(NANOPB_BUILD_DIR) \
		-I $(SCHEMAS_DIR)/nanopb_options \
		--protoc-opt=-I$(SCHEMAS_DIR) \
		$(GENERATED_PROTO_FILES:%=$(SCHEMAS_DIR)/%)
	$(MAKE) --no-print-directory write-dependency-info
	@touch $@

$(NANOPB_FILES) $(NANOPB_HEADERS): $(NANOPB_STAMP)

check-dev-deps:
	@if [ ! -f "$(SCHEMAS_DIR)/geisa-status.proto" ] || \
		[ ! -f "$(NANOPB_RUNTIME_DIR)/pb_encode.c" ] || \
		[ ! -x "$(PYTHON)" ]; then \
		echo 'Development dependencies are not installed.' >&2; \
		echo 'Run: make setup-dev' >&2; \
		exit 2; \
	fi

setup-dev:
	sh ./scripts/setup-dev.sh

regenerate: check-dev-deps
	$(MAKE) $(NANOPB_STAMP) \
		GEISA_SCHEMAS_REF='$(GEISA_SCHEMAS_REF)' \
		GEISA_SCHEMAS_DIR='$(GEISA_SCHEMAS_DIR)'

verify-generated: check-dev-deps $(NANOPB_STAMP)
	@test -s $(NANOPB_BUILD_DIR)/app-message.pb.c
	@test -s $(NANOPB_BUILD_DIR)/discovery.pb.c
	@test -s $(BUILD_DIR)/dependency-info.txt

test: check-dev-deps app
	@set -e; for test in config protocol readiness; do \
		$(CC) $(CFLAGS) -DGEISA_SIMPLE_TEST -o $(BUILD_DIR)/smoke-$$test \
			tools/geisa-simple-$$test-smoke.c src/geisa-simple-config.c \
			src/geisa-protobuf.c $(NANOPB_FILES) $(NANOPB_RUNTIME_DIR)/pb_common.c \
			$(NANOPB_RUNTIME_DIR)/pb_encode.c \
			$(NANOPB_RUNTIME_DIR)/pb_decode.c $(LDLIBS); \
		$(BUILD_DIR)/smoke-$$test; \
	done

dependency-info: write-dependency-info
	@cat $(BUILD_DIR)/dependency-info.txt

manifest: $(BUILD_DIR)/manifest.json

$(BUILD_DIR)/manifest.json: geisa-simple-manifest.json
	@test -n "$(strip $(TOOLCHAIN_ID))" || { \
		echo 'TOOLCHAIN_ID is required' >&2; exit 2; }
	@test -n "$(strip $(TOOLCHAIN_VERSION))" || { \
		echo 'TOOLCHAIN_VERSION is required' >&2; exit 2; }
	@mkdir -p $(BUILD_DIR)
	sed -e 's|__TOOLCHAIN_ID__|$(TOOLCHAIN_ID)|g' \
		-e 's|__TOOLCHAIN_VERSION__|$(TOOLCHAIN_VERSION)|g' \
		geisa-simple-manifest.json > $@

clean:
	rm -rf $(BUILD_DIR)

help:
	@printf '%s\n' \
		'geisa-simple $(APP_VERSION)' \
		'' \
		'make setup-dev  Download the pinned schemas and nanopb dependencies.' \
		'make             Build the application.' \
		'make test        Build and run app-local smoke tests.' \
		'make regenerate  Regenerate protobuf bindings from the selected schemas.' \
		'make verify-generated  Check generated bindings and dependency info.' \
		'make dependency-info  Show the active schema checkout and nanopb version.' \
		'make manifest TOOLCHAIN_ID=... TOOLCHAIN_VERSION=...  Fill fields.' \
		'' \
		'GEISA_SCHEMAS_REF=v0.9.0 is the reproducible default.' \
		'Use GEISA_SCHEMAS_REF=main, a tag, commit, or local checkout.'
