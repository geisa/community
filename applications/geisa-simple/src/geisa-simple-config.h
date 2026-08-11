/*
 * File: src/geisa-simple-config.h
 * Project: geisa-simple
 * Purpose: Declares geisa-simple's JSON CONFIG options
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

#ifndef GEISA_SIMPLE_CONFIG_H
#define GEISA_SIMPLE_CONFIG_H

#include <stddef.h>

#define GEISA_SIMPLE_DEFAULT_REPORTING_INTERVAL_SECONDS 60
#define GEISA_SIMPLE_MIN_REPORTING_INTERVAL_SECONDS 60
#define GEISA_SIMPLE_MAX_REPORTING_INTERVAL_SECONDS 604800

/* Holds geisa-simple's reporting enablement and interval settings */
struct geisa_simple_config {
    int reporting_enabled;
    int reporting_interval_seconds;
};

/* Results distinguish applied / SET, read-only GET, and rejected CONFIG requests */
enum geisa_simple_config_result {
    GEISA_SIMPLE_CONFIG_APPLIED = 0,
    GEISA_SIMPLE_CONFIG_READ = 1,
    GEISA_SIMPLE_CONFIG_INVALID_JSON = -1,
    GEISA_SIMPLE_CONFIG_UNKNOWN_KEY = -2,
    GEISA_SIMPLE_CONFIG_INVALID_TYPE = -3,
    GEISA_SIMPLE_CONFIG_OUT_OF_RANGE = -4
};

/* Initializes defaults, parses CONFIG requests, and formats effective JSON */
void geisa_simple_config_defaults(struct geisa_simple_config *config);
int geisa_simple_config_apply_json(const char *json, size_t length,
                                   struct geisa_simple_config *config,
                                   char *error, size_t error_size);
int geisa_simple_config_format(const struct geisa_simple_config *config,
                               char *out, size_t out_size);

#endif
