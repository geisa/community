/*
 * File: tools/geisa-simple-config-smoke.c
 * Project: geisa-simple
 * Purpose: Exercise geisa-simple's JSON CONFIG parser
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

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    struct geisa_simple_config config;
    struct geisa_simple_config before;
    char formatted[256];
    char error[128];
#define APPLY(value)                                                           \
    geisa_simple_config_apply_json((value), strlen(value), &config, error,     \
                                   sizeof(error))

    /* Defaults and a valid update */
    geisa_simple_config_defaults(&config);
    assert(config.reporting_enabled == 1);
    assert(config.reporting_interval_seconds ==
           GEISA_SIMPLE_DEFAULT_REPORTING_INTERVAL_SECONDS);

    /* Invalid updates should leave the previous configuration unchanged. */
    before = config;
    assert(APPLY("{\"operation\":\"set_configuration\","
                 "\"values\":{\"reporting_enabled\":false,"
                 "\"reporting_interval_seconds\":120}}") ==
           GEISA_SIMPLE_CONFIG_APPLIED);
    assert(config.reporting_enabled == 0);
    assert(config.reporting_interval_seconds == 120);
    assert(geisa_simple_config_format(&config, formatted, sizeof(formatted)) ==
           0);
    assert(strstr(formatted, "reporting_interval_seconds") != NULL);

    before = config;
    assert(APPLY("{\"operation\":\"set_configuration\","
                 "\"values\":{\"unknown\":true}}") ==
           GEISA_SIMPLE_CONFIG_UNKNOWN_KEY);
    assert(memcmp(&config, &before, sizeof(config)) == 0);
    assert(APPLY("{\"operation\":\"set_configuration\","
                 "\"values\":{\"reporting_enabled\":1}}") ==
           GEISA_SIMPLE_CONFIG_INVALID_TYPE);
    assert(memcmp(&config, &before, sizeof(config)) == 0);
    assert(APPLY("{\"operation\":\"set_configuration\","
                 "\"values\":{\"reporting_interval_seconds\":10}}") ==
           GEISA_SIMPLE_CONFIG_OUT_OF_RANGE);
    assert(memcmp(&config, &before, sizeof(config)) == 0);

    /* A read returns the effective configuration without applying changes. */
    assert(APPLY("{\"operation\":\"get_effective_configuration\"}") ==
           GEISA_SIMPLE_CONFIG_READ);
    assert(APPLY("{\"operation\":\"get_effective_configuration\"} trailing") ==
           GEISA_SIMPLE_CONFIG_INVALID_JSON);
    assert(APPLY("{\"operation\":\"set_configuration\","
                 "\"values\":{") == GEISA_SIMPLE_CONFIG_INVALID_JSON);
    assert(before.reporting_interval_seconds == 120);
    puts("geisa-simple config smoke PASS");
#undef APPLY
    return EXIT_SUCCESS;
}
