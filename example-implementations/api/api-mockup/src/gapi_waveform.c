/**
 * @file gapi_waveform.c
 * @brief Definition file for API waveform data messages
 * @copyright Copyright (C) 2026 Southern California Edison
 */

#include "gapi_waveform.h"
#include "gapi_discovery.h"

static void
api_platform_waveform_build_response(GeisaWaveform_Rsp *response,
				     char *stream_id,
				     GeisaWaveform_RequestType request_type)
{
	GeisaPlatformDiscovery_Waveform waveform_platform_info;
	(void)request_type;

	waveform_platform_info = get_waveform_info();

	if (waveform_platform_info.streams_count == 0) {
		response->waveform_status =
			GeisaWaveform_Status_WAVEFORM_ERR_NO_RESOURCES;
		return;
	}

	if (stream_id == NULL) {
		response->waveform_status =
			GeisaWaveform_Status_WAVEFORM_ERR_INVALID_STREAM_ID;
		return;
	}

	for (size_t i = 0; i < waveform_platform_info.streams_count; i++) {
		if (strcmp(waveform_platform_info.streams[i].stream_id,
			   stream_id) == 0) {
			response->stream_id = stream_id;
			response->waveform_status =
				GeisaWaveform_Status_WAVEFORM_SUCCESS;
			break;
		}
	}

	if (response->waveform_status !=
	    GeisaWaveform_Status_WAVEFORM_SUCCESS) {
		response->waveform_status =
			GeisaWaveform_Status_WAVEFORM_ERR_UNAVAILABLE;
	}
}

static void api_waveform_req_handler(struct mosquitto *mosq, const char *topic,
				     const int payloadlen,
				     const uint8_t *payload)
{
	GeisaWaveform_Req request = GeisaWaveform_Req_init_default;
	GeisaWaveform_Rsp response = GeisaWaveform_Rsp_init_default;
	size_t encoded_size = 0;
	uint8_t *message = NULL;
	char *rsp_topic = NULL;
	char *app_id = NULL;
	pb_istream_t istream;
	pb_ostream_t ostream;
	bool status = false;

	app_id = basename((char *)topic);

	fprintf(stdout, "[Waveform] Received waveform data request from %s\n",
		app_id);
	fflush(stdout);

	istream = pb_istream_from_buffer(payload, payloadlen);
	status = pb_decode(&istream, GeisaWaveform_Req_fields, &request);
	if (!status) {
		fprintf(stderr,
			"[Waveform] Error decoding waveform app request\n");
		return;
	}

	if (asprintf(&rsp_topic, "geisa/api/waveform/rsp/%s", app_id) == -1) {
		fprintf(stderr, "[Waveform] Error allocating memory for "
				"response topic\n");
		return;
	}

	api_platform_waveform_build_response(&response, request.stream_id,
					     request.request_type);
	pb_release(GeisaWaveform_Req_fields, &request);

	status = pb_get_encoded_size(&encoded_size, GeisaWaveform_Rsp_fields,
				     &response);
	if (!status) {
		fprintf(stderr, "[Waveform] Error calculating size of response "
				"message\n");
		free(rsp_topic);
		return;
	}

	message = malloc(encoded_size);
	if (message == NULL) {
		fprintf(stderr,
			"[Waveform] Error allocating memory for response "
			"message\n");
		free(rsp_topic);
		return;
	}

	ostream = pb_ostream_from_buffer(message, encoded_size);
	status = pb_encode(&ostream, GeisaWaveform_Rsp_fields, &response);
	if (!status) {
		fprintf(stderr, "[Waveform] Error encoding response message\n");
		free(message);
		free(rsp_topic);
		return;
	}

	api_publish(mosq, rsp_topic, encoded_size, message, 1);
	free(message);
}

void api_waveform_init(struct mosquitto *mosq)
{
	api_register_handler("geisa/api/waveform/req",
			     api_waveform_req_handler);

	api_subscribe(mosq, "geisa/api/waveform/req/#", 1);
}
