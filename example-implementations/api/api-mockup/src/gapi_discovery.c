/**
 * @file gapi_discovery.c
 * @brief Definition file for API platform discovery messages
 * @copyright Copyright (C) 2025 Southern California Edison
 */

#include "gapi_discovery.h"

static void api_platform_discovery_req_handler(const char *topic,
					       const int payloadlen,
					       const char *payload)
{
	fprintf(stdout, "Received platform discovery request\n");
	fprintf(stdout, "[msg] topic=%s payload=%.*s\n", topic, payloadlen,
		payload);
}

static void api_manifest_req_handler(const char *topic, const int payloadlen,
				     const char *payload)
{
	fprintf(stdout, "Received app manifest request\n");
	fprintf(stdout, "[msg] topic=%s payload=%.*s\n", topic, payloadlen,
		payload);
}

void api_discovery_init(struct mosquitto *mosq)
{
	api_register_handler("geisa/api/app-manifest-req",
			     api_manifest_req_handler);
	api_register_handler("geisa/api/platform-discovery-req",
			     api_platform_discovery_req_handler);

	api_subscribe(mosq, "geisa/api/platform-discovery-req", 1);
	api_subscribe(mosq, "geisa/api/app-manifest-req/#", 1);
}
