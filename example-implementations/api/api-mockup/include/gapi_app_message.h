/**
 * @file gapi_app_message.h
 * @brief Header file for API application messages
 * @copyright Copyright (C) 2026 Southern California Edison
 */

#ifndef GAPI_APP_MESSAGE_H
#define GAPI_APP_MESSAGE_H
#define _GNU_SOURCE

#include "gapi_mosquitto.h"
#include "pb.h"
#include "pb_decode.h"
#include "pb_encode.h"
#include "schemas/app-message.pb.h"
#include <libgen.h>

/**
 * @brief Initialize API application messages
 *
 * @param mosq Pointer to the mosquitto instance
 */
void api_app_message_init(struct mosquitto *mosq);

#endif // GAPI_APP_MESSAGE_H
