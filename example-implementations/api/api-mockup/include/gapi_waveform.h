/**
 * @file gapi_waveform.h
 * @brief Header file for API waveform data messages
 * @copyright Copyright (C) 2026 Southern California Edison
 */

#ifndef GAPI_WAVEFORM_H
#define GAPI_WAVEFORM_H
#define _GNU_SOURCE

#include "gapi_mosquitto.h"
#include "pb.h"
#include "pb_decode.h"
#include "pb_encode.h"
#include "schemas/waveform.pb.h"
#include <libgen.h>

/**
 * @brief Initialize API waveform data messages
 *
 * @param mosq Pointer to the mosquitto instance
 */
void api_waveform_init(struct mosquitto *mosq);

#endif // GAPI_WAVEFORM_H
