/**
 * @file gapi_instantaneous.c
 * @brief Definition file for API instantaneous data sending
 * @copyright Copyright (C) 2025 Southern California Edison
 */

#include "gapi_instantaneous.h"

static void *gapi_instantaneous_thread(void *arg)
{
	struct mosquitto *mosq = (struct mosquitto *)arg;
	uint8_t *message = (uint8_t *)"instdata";

	while (running) {
		api_publish(mosq, "geisa/api/instantaneous-data",
			    sizeof(message), message, 0);
		sleep(1);
	}

	return NULL;
}

int api_instantaneous_init(pthread_t *thread, struct mosquitto *mosq)
{
	int ret = 0;
	ret = pthread_create(thread, NULL, gapi_instantaneous_thread, mosq);
	if (ret != 0) {
		fprintf(stderr, "[Instantaneous] Failed to create "
				"instantaneous thread\n");
		return ret;
	}
	return 0;
}
