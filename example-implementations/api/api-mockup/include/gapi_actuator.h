/**
 * @file gapi_actuator.h
 * @brief Header file for API actuator messages
 * @copyright Copyright (C) 2026 Southern California Edison
 */

#ifndef GAPI_ACTUATOR_H
#define GAPI_ACTUATOR_H
#define _GNU_SOURCE

#include "gapi_mosquitto.h"
#include "pb.h"
#include "pb_decode.h"
#include "pb_encode.h"
#include "schemas/actuator.pb.h"

/**
 * @brief Initialize API actuator messages
 *
 * @param mosq Pointer to the mosquitto instance
 */
void api_actuator_init(struct mosquitto *mosq);

#endif // GAPI_ACTUATOR_H
