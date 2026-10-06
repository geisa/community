/**
 * @file gapi_app_message.c
 * @brief Definition file for API application messages
 * @copyright Copyright (C) 2026 Southern California Edison
 */

#include "gapi_app_message.h"

static void api_app_message_req_handler(struct mosquitto *mosq,
					const char *topic, const int payloadlen,
					const uint8_t *payload)
{
	GeisaAppMessage_Req request = GeisaAppMessage_Req_init_default;
	GeisaAppMessage_Rsp response = GeisaAppMessage_Rsp_init_default;
	size_t encoded_size = 0;
	uint8_t *message = NULL;
	char *rsp_topic = NULL;
	char *app_id = NULL;
	pb_istream_t istream;
	pb_ostream_t ostream;

	app_id = basename((char *)topic);

	fprintf(stdout, "[AppMessage] Received app message request from %s\n",
		app_id);
	fflush(stdout);

	istream = pb_istream_from_buffer(payload, payloadlen);
	if (pb_decode(&istream, GeisaAppMessage_Req_fields, &request)) {
		response.request_id = request.request_id;
		response.status =
			GeisaAppMessageStatus_GEISA_APP_MESSAGE_STATUS_ACCEPTED;
		response.status_text = "App message accepted";
	} else {
		fprintf(stderr,
			"[AppMessage] Error decoding app message request\n");
		response.status =
			GeisaAppMessageStatus_GEISA_APP_MESSAGE_STATUS_REJECTED;
		response.status_text = "Malformed app message request";
	}

	if (asprintf(&rsp_topic, "geisa/api/message/upstream/rsp/%s", app_id) ==
	    -1) {
		fprintf(stderr, "[AppMessage] Error allocating memory for "
				"response topic\n");
		rsp_topic = NULL;
		goto cleanup;
	}

	if (!pb_get_encoded_size(&encoded_size, GeisaAppMessage_Rsp_fields,
				 &response)) {
		fprintf(stderr, "[AppMessage] Error calculating size of app "
				"message response\n");
		goto cleanup;
	}

	message = malloc(encoded_size);
	if (message == NULL) {
		fprintf(stderr, "[AppMessage] Error allocating memory for "
				"response message\n");
		goto cleanup;
	}

	ostream = pb_ostream_from_buffer(message, encoded_size);
	if (!pb_encode(&ostream, GeisaAppMessage_Rsp_fields, &response)) {
		fprintf(stderr,
			"[AppMessage] Error encoding app message response\n");
		goto cleanup;
	}

	api_publish(mosq, rsp_topic, encoded_size, message, 1);

cleanup:
	free(rsp_topic);
	free(message);
	pb_release(GeisaAppMessage_Req_fields, &request);
}

void api_app_message_init(struct mosquitto *mosq)
{
	api_register_handler("geisa/api/message/upstream/req",
			     api_app_message_req_handler);

	api_subscribe(mosq, "geisa/api/message/upstream/req/#", 1);
}
