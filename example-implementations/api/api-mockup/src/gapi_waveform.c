/**
 * @file gapi_waveform.c
 * @brief Definition file for API waveform data messages
 * @copyright Copyright (C) 2026 Southern California Edison
 */

#include "gapi_waveform.h"

static void api_waveform_req_handler(struct mosquitto *mosq, const char *topic,
				     const int payloadlen,
				     const uint8_t *payload)
{
	WaveformReq *request = NULL;
	WaveformRsp response = WAVEFORM__RSP__INIT;

	uint8_t *message = NULL;
	char *rsp_topic = NULL;
	char *app_id = NULL;

	app_id = basename((char *)topic);

	fprintf(stdout, "[Waveform] Received waveform data request from %s\n",
		app_id);
	request = waveform__req__unpack(NULL, payloadlen, payload);
	if (request == NULL) {
		fprintf(stderr,
			"[Waveform] Error unpacking app manifest request\n");
		return;
	}

	waveform__req__free_unpacked(request, NULL);

	if (asprintf(&rsp_topic, "geisa/api/waveform-rsp/%s", app_id) == -1) {
		fprintf(stderr, "[Waveform] Error allocating memory for "
				"response topic\n");
		return;
	}

	response.status = WAVEFORM__STATUS__WAVEFORM_SUCCESS;
	message = malloc(waveform__rsp__get_packed_size(&response));
	if (message == NULL) {
		fprintf(stderr,
			"[Waveform] Error allocating memory for response "
			"message\n");
		free(rsp_topic);
		return;
	}
	waveform__rsp__pack(&response, message);
	api_publish(mosq, rsp_topic, waveform__rsp__get_packed_size(&response),
		    message, 1);
	free(message);
}

void api_waveform_init(struct mosquitto *mosq)
{
	api_register_handler("geisa/api/waveform-req",
			     api_waveform_req_handler);

	api_subscribe(mosq, "geisa/api/waveform-req/#", 1);
}
