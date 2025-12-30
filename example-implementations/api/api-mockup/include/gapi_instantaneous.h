/**
 * @file gapi_instantaneous.h
 * @brief Header file for API instantaneous data sending
 * @copyright Copyright (C) 2025 Southern California Edison
 */

#ifndef GAPI_INSTANTANEOUS_H
#define GAPI_INSTANTANEOUS_H

#include "gapi_mosquitto.h"
#include "schemas/metered_quantities.pb-c.h"
#include <pthread.h>
#include <time.h>

/**
 * @brief Initialize API instantaneous data sending thread
 *
 * @param thread Pointer to the pthread_t variable to store the thread ID
 * @param mosq Pointer to the mosquitto instance
 *
 * @return int 0 on success, non-zero on failure
 */
int api_instantaneous_init(pthread_t *thread, struct mosquitto *mosq);

#define SEC_IN_MS 1000

#endif // GAPI_INSTANTANEOUS_H
