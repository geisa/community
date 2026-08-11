/*
 * File: src/geisa-simple-config.c
 * Project: geisa-simple
 * Purpose: Parses and applies JSON CONFIG options for geisa-simple
 *
 * Copyright 2026 PragSol Consulting LLC.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "geisa-simple-config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void skip_ws(const char **cursor, const char *end) {
    while (*cursor < end && (**cursor == ' ' || **cursor == '\t' ||
                             **cursor == '\r' || **cursor == '\n')) {
        (*cursor)++;
    }
}

/*
 * Consumes a quoted JSON string, copies its contents into out, and advances
 * *cursor. Escaped characters are not supported.
 */
static int consume_string(const char **cursor, const char *end, char *out,
                          size_t out_size) {
    const char *start;
    size_t length;

    skip_ws(cursor, end);
    if (*cursor >= end || **cursor != '"') return -1;
    (*cursor)++;
    start = *cursor;
    while (*cursor < end && **cursor != '"') {
        if (**cursor == '\\') return -1;
        (*cursor)++;
    }
    if (*cursor >= end) return -1;
    length = (size_t)(*cursor - start);
    if (length + 1U > out_size) return -1;
    memcpy(out, start, length);
    out[length] = '\0';
    (*cursor)++;
    return 0;
}

/* Consumes the expected JSON literal and advances *cursor. */
static int consume_literal(const char **cursor, const char *end,
                           const char *literal) {
    size_t length = strlen(literal);
    skip_ws(cursor, end);
    if ((size_t)(end - *cursor) < length ||
        memcmp(*cursor, literal, length) != 0)
        return -1;
    *cursor += length;
    return 0;
}

/*
 * Parses a non-negative decimal integer, stores it in *out, and advances
 * *cursor. Values above the parser's supported maximum are rejected here.
 */
static int consume_integer(const char **cursor, const char *end, int *out) {
    const char *start;
    char *parse_end = NULL;
    long value;

    skip_ws(cursor, end);
    start = *cursor;
    value = strtol(start, &parse_end, 10);
    if (parse_end == start || parse_end > end || value < 0 ||
        value > GEISA_SIMPLE_MAX_REPORTING_INTERVAL_SECONDS) {
        return -1;
    }
    *out = (int)value;
    *cursor = parse_end;
    return 0;
}

static void set_error(char *error, size_t error_size, const char *message) {
    if (error && error_size > 0) snprintf(error, error_size, "%s", message);
}

/* Initializes *config with geisa-simple's default CONFIG values. */
void geisa_simple_config_defaults(struct geisa_simple_config *config) {
    if (!config) return;
    config->reporting_enabled = 1;
    config->reporting_interval_seconds =
        GEISA_SIMPLE_DEFAULT_REPORTING_INTERVAL_SECONDS;
}

/*
 * Parses a CONFIG request, validates its operation, keys, types, and ranges,
 * and applies valid SET changes transactionally. GET requests return
 * GEISA_SIMPLE_CONFIG_READ; validation failures leave *config unchanged and
 * return a named error.
 * validation failures leave config unchanged and return a named error.
 */
int geisa_simple_config_apply_json(const char *json, size_t length,
                                   struct geisa_simple_config *config,
                                   char *error, size_t error_size) {
    const char *cursor;
    const char *end;
    char operation[64];
    struct geisa_simple_config candidate;
    int is_get = 0;
    int saw_value = 0;

    if (!json || !config || length == 0) {
        set_error(error, error_size, "CONFIG payload must be a JSON object");
        return GEISA_SIMPLE_CONFIG_INVALID_JSON;
    }
    cursor = json;
    end = json + length;
    skip_ws(&cursor, end);
    if (cursor >= end || *cursor++ != '{') {
        set_error(error, error_size, "CONFIG payload must be a JSON object");
        return GEISA_SIMPLE_CONFIG_INVALID_JSON;
    }
    skip_ws(&cursor, end);
    if (consume_string(&cursor, end, operation, sizeof(operation)) != 0 ||
        strcmp(operation, "operation") != 0) {
        set_error(error, error_size, "CONFIG operation is required");
        return GEISA_SIMPLE_CONFIG_INVALID_JSON;
    }
    skip_ws(&cursor, end);
    if (cursor >= end || *cursor++ != ':') {
        set_error(error, error_size, "CONFIG operation is malformed");
        return GEISA_SIMPLE_CONFIG_INVALID_JSON;
    }
    /* Parses the operation that selects a CONFIG read or set request. */
    if (consume_string(&cursor, end, operation, sizeof(operation)) != 0) {
        set_error(error, error_size, "CONFIG operation must be a string");
        return GEISA_SIMPLE_CONFIG_INVALID_TYPE;
    }
    if (strcmp(operation, "get_effective_configuration") == 0) {
        is_get = 1;
    } else if (strcmp(operation, "set_configuration") != 0) {
        set_error(error, error_size, "unsupported CONFIG operation");
        return GEISA_SIMPLE_CONFIG_UNKNOWN_KEY;
    }
    skip_ws(&cursor, end);
    if (is_get) {
        /* Handles a read without changing the active configuration. */
        if (cursor >= end || *cursor++ != '}') {
            set_error(error, error_size, "GET CONFIG has unexpected fields");
            return GEISA_SIMPLE_CONFIG_INVALID_JSON;
        }
        skip_ws(&cursor, end);
        if (cursor != end) {
            set_error(error, error_size, "CONFIG payload has trailing content");
            return GEISA_SIMPLE_CONFIG_INVALID_JSON;
        }
        return GEISA_SIMPLE_CONFIG_READ;
    }
    /* Parses the values object for a SET request. */
    if (cursor >= end || *cursor++ != ',') {
        set_error(error, error_size, "set_configuration requires values");
        return GEISA_SIMPLE_CONFIG_INVALID_JSON;
    }
    skip_ws(&cursor, end);
    {
        char key[64];
        if (consume_string(&cursor, end, key, sizeof(key)) != 0 ||
            strcmp(key, "values") != 0) {
            set_error(error, error_size, "set_configuration requires values");
            return GEISA_SIMPLE_CONFIG_INVALID_JSON;
        }
    }
    skip_ws(&cursor, end);
    if (cursor >= end || *cursor++ != ':') {
        set_error(error, error_size, "CONFIG values are malformed");
        return GEISA_SIMPLE_CONFIG_INVALID_JSON;
    }
    skip_ws(&cursor, end);
    if (cursor >= end || *cursor++ != '{') {
        set_error(error, error_size, "CONFIG values must be an object");
        return GEISA_SIMPLE_CONFIG_INVALID_TYPE;
    }
    /* Copies the active configuration so validation cannot partially modify it. */
    candidate = *config;
    for (;;) {
        char key[64];
        skip_ws(&cursor, end);
        if (cursor >= end) {
            set_error(error, error_size, "CONFIG values are unterminated");
            return GEISA_SIMPLE_CONFIG_INVALID_JSON;
        }
        if (*cursor == '}') {
            cursor++;
            break;
        }
        if (consume_string(&cursor, end, key, sizeof(key)) != 0) {
            set_error(error, error_size, "CONFIG key is malformed");
            return GEISA_SIMPLE_CONFIG_INVALID_JSON;
        }
        skip_ws(&cursor, end);
        if (cursor >= end || *cursor++ != ':') {
            set_error(error, error_size, "CONFIG value is malformed");
            return GEISA_SIMPLE_CONFIG_INVALID_JSON;
        }
        if (strcmp(key, "reporting_enabled") == 0) {
            skip_ws(&cursor, end);
            if (consume_literal(&cursor, end, "true") == 0) {
                candidate.reporting_enabled = 1;
            } else if (consume_literal(&cursor, end, "false") == 0) {
                candidate.reporting_enabled = 0;
            } else {
                set_error(error, error_size,
                          "reporting_enabled must be boolean");
                return GEISA_SIMPLE_CONFIG_INVALID_TYPE;
            }
        } else if (strcmp(key, "reporting_interval_seconds") == 0) {
            int interval;
            if (consume_integer(&cursor, end, &interval) != 0) {
                set_error(error, error_size,
                          "reporting_interval_seconds must be an integer");
                return GEISA_SIMPLE_CONFIG_INVALID_TYPE;
            }
            if (interval < GEISA_SIMPLE_MIN_REPORTING_INTERVAL_SECONDS ||
                interval > GEISA_SIMPLE_MAX_REPORTING_INTERVAL_SECONDS) {
                set_error(error, error_size,
                          "reporting_interval_seconds is out of range");
                return GEISA_SIMPLE_CONFIG_OUT_OF_RANGE;
            }
            candidate.reporting_interval_seconds = interval;
        } else {
            set_error(error, error_size, "unsupported CONFIG key");
            return GEISA_SIMPLE_CONFIG_UNKNOWN_KEY;
        }
        saw_value = 1;
        skip_ws(&cursor, end);
        if (cursor < end && *cursor == ',') {
            cursor++;
            continue;
        }
        if (cursor < end && *cursor == '}') {
            cursor++;
            break;
        }
        set_error(error, error_size, "CONFIG values are malformed");
        return GEISA_SIMPLE_CONFIG_INVALID_JSON;
    }
    if (!saw_value) {
        set_error(error, error_size, "CONFIG values must not be empty");
        return GEISA_SIMPLE_CONFIG_INVALID_JSON;
    }
    skip_ws(&cursor, end);
    if (cursor >= end || *cursor++ != '}') {
        set_error(error, error_size, "CONFIG payload has trailing content");
        return GEISA_SIMPLE_CONFIG_INVALID_JSON;
    }
    skip_ws(&cursor, end);
    if (cursor != end) {
        set_error(error, error_size, "CONFIG payload has trailing content");
        return GEISA_SIMPLE_CONFIG_INVALID_JSON;
    }
    /* Commits the validated candidate only after the full request succeeds. */
    *config = candidate;
    return GEISA_SIMPLE_CONFIG_APPLIED;
}

/* Formats the effective CONFIG values as JSON in out. */
int geisa_simple_config_format(const struct geisa_simple_config *config,
                               char *out, size_t out_size) {
    int written;
    if (!config || !out || out_size == 0) return -1;
    written = snprintf(out, out_size,
                       "{\"reporting_enabled\":%s,"
                       "\"reporting_interval_seconds\":%d}",
                       config->reporting_enabled ? "true" : "false",
                       config->reporting_interval_seconds);
    return written >= 0 && (size_t)written < out_size ? 0 : -1;
}
