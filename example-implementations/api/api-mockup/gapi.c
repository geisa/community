/**
 * @file gapi.c
 * @brief GEISA API mockup main
 * @copyright Copyright (C) 2025 Southern California Edison
 */
#include "gapi_mosquitto.h"

volatile bool running = true;
volatile bool isConnected = false;
#define PORT 1883

int main()
{
	const char *broker = "localhost";
	struct mosquitto *mosq = NULL;
	int port = PORT;

	mosq = api_communication_init(broker, port);
	if (!mosq) {
		return EXIT_FAILURE;
	}

	while (running && !isConnected) {
		mosquitto_loop(mosq, -1, 1);
		sleep(1);
	}

	api_communication_deinit(mosq);
	return EXIT_SUCCESS;
}
