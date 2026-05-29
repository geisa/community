/**
 * @file gapi_manifest.h
 * @brief Header file for API application manifest messages
 * @copyright Copyright (C) 2026 Southern California Edison
 */

#ifndef GAPI_MANIFEST_H
#define GAPI_MANIFEST_H
#define _GNU_SOURCE

#include "gapi_mosquitto.h"
#include "pb.h"
#include "pb_decode.h"
#include "pb_encode.h"
#include "schemas/manifest.pb.h"
#include <libgen.h>

/**
 * @brief Initialize API application manifest messages
 *
 * @param mosq Pointer to the mosquitto instance
 */
void api_manifest_init(struct mosquitto *mosq);

#endif // GAPI_MANIFEST_H
