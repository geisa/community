/**
 * @file gapi.c
 * @brief GEISA API mockup main
 * @copyright Copyright (C) 2025 Southern California Edison
 */
#include "gapi_discovery.h"
#include "gapi_instantaneous.h"
#include "gapi_mosquitto.h"

volatile bool running = true;
volatile bool isConnected = false;
#define PORT 1883

int main()
{
	pthread_t instantaneous_thread = 0;
	const char *broker = "localhost";
	struct mosquitto *mosq = NULL;
	int ret = EXIT_SUCCESS;
	int port = PORT;

	mosq = api_communication_init(broker, port);
	if (!mosq) {
		return EXIT_FAILURE;
	}

	while (running && !isConnected) {
		mosquitto_loop(mosq, -1, 1);
		sleep(1);
	}

	api_discovery_init(mosq);
	ret = api_instantaneous_init(&instantaneous_thread, mosq);
	if (ret != EXIT_SUCCESS) {
		goto deinit;
	}

	while (running) {
		mosquitto_loop(mosq, -1, 1);
		sleep(1);
	}

	pthread_join(instantaneous_thread, NULL);

deinit:
	api_communication_deinit(mosq);
	return ret;
}
