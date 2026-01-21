/**
 * @file gapi_discovery.h
 * @brief Header file for API platform discovery messages
 * @copyright Copyright (C) 2025 Southern California Edison
 */

#ifndef GAPI_DISCOVERY_H
#define GAPI_DISCOVERY_H
#define _GNU_SOURCE

#include "gapi_mosquitto.h"
#include "schemas/discovery.pb-c.h"
#include <libgen.h>

/**
 * @brief Initialize API platform discovery messages
 *
 * @param mosq Pointer to the mosquitto instance
 */
void api_discovery_init(struct mosquitto *mosq);

/**
 * @brief Get waveform information for platform discovery
 *
 * @return Pointer to PlatformDiscoveryWaveform structure
 */
PlatformDiscoveryWaveform get_waveform_info();

#endif // GAPI_DISCOVERY_H
