/**
 * @file gapi_sensor.h
 * @brief Header file for API sensors data messages
 * @copyright Copyright (C) 2026 Southern California Edison
 */
#ifndef GAPI_SENSOR_H
#define GAPI_SENSOR_H

#include "gapi_mosquitto.h"

/**
 * @brief Initialize API sensor handlers
 *
 * @param mosq Pointer to mosquitto instance
 */
void api_sensor_init(struct mosquitto *mosq);

#endif // GAPI_SENSOR_H
