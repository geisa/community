/**
 * @file gapi_actuator.c
 * @brief Definition file for API actuator messages
 * @copyright Copyright (C) 2026 Southern California Edison
 */

#include "gapi_actuator.h"

static GeisaStatus geisa_actuator_success_status = {
	.code = GeisaStatusCode_GEISA_STATUS_SUCCESS,
	.message = "Actuator request successful",
};

static GeisaStatus geisa_actuator_payload_status = {
	.code = GeisaStatusCode_GEISA_STATUS_CODE_REQUEST_MALFORMED_PAYLOAD,
	.message = "Malformed payload in actuator request",
};

static GeisaStatus geisa_actuator_invalid_status = {
	.code = GeisaStatusCode_GEISA_STATUS_CODE_REQUEST_INVALID_ARGUMENT,
	.message = "Invalid actuator target",
};

static GeisaActuatorStatus actuator_statuses[_GeisaTypeActuator_ARRAYSIZE] = {
	[GeisaTypeActuator_GEISA_TYPE_ACTUATOR_UNSPECIFIED] =
		{
			.actuator =
				GeisaTypeActuator_GEISA_TYPE_ACTUATOR_UNSPECIFIED,
			.on = _GeisaTypeOnOff_MIN,
		},
	[GeisaTypeActuator_GEISA_TYPE_ACTUATOR_SERVICE_SWITCH] =
		{
			.actuator =
				GeisaTypeActuator_GEISA_TYPE_ACTUATOR_SERVICE_SWITCH,
			.on = _GeisaTypeOnOff_MIN,
		},
	[GeisaTypeActuator_GEISA_TYPE_ACTUATOR_DER_SWITCH] =
		{
			.actuator =
				GeisaTypeActuator_GEISA_TYPE_ACTUATOR_DER_SWITCH,
			.on = _GeisaTypeOnOff_MIN,
		},
	[GeisaTypeActuator_GEISA_TYPE_ACTUATOR_LC_RELAY_0] =
		{
			.actuator =
				GeisaTypeActuator_GEISA_TYPE_ACTUATOR_LC_RELAY_0,
			.on = _GeisaTypeOnOff_MIN,
		},
	[GeisaTypeActuator_GEISA_TYPE_ACTUATOR_LC_RELAY_1] =
		{
			.actuator =
				GeisaTypeActuator_GEISA_TYPE_ACTUATOR_LC_RELAY_1,
			.on = _GeisaTypeOnOff_MIN,
		},
	[GeisaTypeActuator_GEISA_TYPE_ACTUATOR_LC_RELAY_2] =
		{
			.actuator =
				GeisaTypeActuator_GEISA_TYPE_ACTUATOR_LC_RELAY_2,
			.on = _GeisaTypeOnOff_MIN,
		},
	[GeisaTypeActuator_GEISA_TYPE_ACTUATOR_LC_RELAY_3] =
		{
			.actuator =
				GeisaTypeActuator_GEISA_TYPE_ACTUATOR_LC_RELAY_3,
			.on = _GeisaTypeOnOff_MIN,
		},
};

static bool actuator_is_valid(GeisaTypeActuator actuator)
{
	switch (actuator) {
	case GeisaTypeActuator_GEISA_TYPE_ACTUATOR_UNSPECIFIED:
	case GeisaTypeActuator_GEISA_TYPE_ACTUATOR_SERVICE_SWITCH:
	case GeisaTypeActuator_GEISA_TYPE_ACTUATOR_DER_SWITCH:
	case GeisaTypeActuator_GEISA_TYPE_ACTUATOR_LC_RELAY_0:
	case GeisaTypeActuator_GEISA_TYPE_ACTUATOR_LC_RELAY_1:
	case GeisaTypeActuator_GEISA_TYPE_ACTUATOR_LC_RELAY_2:
	case GeisaTypeActuator_GEISA_TYPE_ACTUATOR_LC_RELAY_3:
		return true;
	default:
		return false;
	}
}

static void actuator_status_apply(const GeisaActuatorStatus *status)
{
	if (status == NULL || !actuator_is_valid(status->actuator)) {
		return;
	}

	actuator_statuses[status->actuator] = *status;
}

static int actuator_status_find(GeisaTypeActuator actuator)
{
	if (!actuator_is_valid(actuator)) {
		return -1;
	}

	return (int)actuator;
}

static void api_actuator_set_req_handler(struct mosquitto *mosq,
					 const char *topic,
					 const int payloadlen,
					 const uint8_t *payload)
{
	GeisaActuatorSet_Req request = GeisaActuatorSet_Req_init_default;
	GeisaActuatorSet_Rsp response = GeisaActuatorSet_Rsp_init_default;
	uint8_t *message = NULL;
	char *rsp_topic = NULL;
	char *app_id = NULL;
	pb_istream_t istream;
	pb_ostream_t ostream;
	bool status = false;
	size_t encoded_size = 0;

	app_id = basename((char *)topic);

	fprintf(stdout, "[Actuator] Received actuator set request from %s\n",
		app_id);
	fflush(stdout);

	istream = pb_istream_from_buffer(payload, payloadlen);
	status = pb_decode(&istream, GeisaActuatorSet_Req_fields, &request);
	if (!status) {
		fprintf(stderr,
			"[Actuator] Error decoding actuator set request\n");
		response.status = geisa_actuator_payload_status;
		response.has_status = true;
	} else {
		for (pb_size_t i = 0; i < request.new_settings_count; i++) {
			actuator_status_apply(&request.new_settings[i]);
		}
		response.status = geisa_actuator_success_status;
		response.has_status = true;
	}
	pb_release(GeisaActuatorSet_Req_fields, &request);

	status = pb_get_encoded_size(&encoded_size, GeisaActuatorSet_Rsp_fields,
				     &response);
	if (!status) {
		fprintf(stderr,
			"[Actuator] Error calculating size of actuator set response\n");
		return;
	}

	message = malloc(encoded_size);
	if (message == NULL) {
		fprintf(stderr,
			"[Actuator] Error allocating memory for actuator set response\n");
		return;
	}

	if (asprintf(&rsp_topic, "geisa/api/actuator/set/rsp/%s", app_id) ==
	    -1) {
		fprintf(stderr,
			"[Actuator] Error allocating memory for response topic\n");
		free(message);
		return;
	}

	ostream = pb_ostream_from_buffer(message, encoded_size);
	status = pb_encode(&ostream, GeisaActuatorSet_Rsp_fields, &response);
	if (!status) {
		fprintf(stderr,
			"[Actuator] Error encoding platform actuator set response\n");
		free(rsp_topic);
		free(message);
		return;
	}

	api_publish(mosq, rsp_topic, encoded_size, message, 1);

	free(rsp_topic);
	free(message);
}

static void api_actuator_get_req_handler(struct mosquitto *mosq,
					 const char *topic,
					 const int payloadlen,
					 const uint8_t *payload)
{
	GeisaActuatorGet_Req request = GeisaActuatorGet_Req_init_default;
	GeisaActuatorGet_Rsp response = GeisaActuatorGet_Rsp_init_default;
	uint8_t *message = NULL;
	char *rsp_topic = NULL;
	char *app_id = NULL;
	pb_istream_t istream;
	pb_ostream_t ostream;
	bool status = false;
	size_t encoded_size = 0;
	int actuator_idx = -1;

	app_id = basename((char *)topic);

	fprintf(stdout, "[Actuator] Received actuator get request from %s\n",
		app_id);
	fflush(stdout);

	istream = pb_istream_from_buffer(payload, payloadlen);
	status = pb_decode(&istream, GeisaActuatorGet_Req_fields, &request);
	if (!status) {
		fprintf(stderr,
			"[Actuator] Error decoding actuator get request\n");
		response.status = geisa_actuator_payload_status;
		response.has_status = true;
	} else {
		actuator_idx = actuator_status_find(request.actuator);
		if (actuator_idx < 0) {
			response.status = geisa_actuator_invalid_status;
			response.has_status = true;
		} else {
			response.status = geisa_actuator_success_status;
			response.has_status = true;
			response.actuator_status =
				actuator_statuses[actuator_idx];
			response.has_actuator_status = true;
		}
	}
	pb_release(GeisaActuatorGet_Req_fields, &request);

	status = pb_get_encoded_size(&encoded_size, GeisaActuatorGet_Rsp_fields,
				     &response);
	if (!status) {
		fprintf(stderr,
			"[Actuator] Error calculating size of actuator get response\n");
		return;
	}

	message = malloc(encoded_size);
	if (message == NULL) {
		fprintf(stderr,
			"[Actuator] Error allocating memory for actuator get response\n");
		return;
	}

	if (asprintf(&rsp_topic, "geisa/api/actuator/get/rsp/%s", app_id) ==
	    -1) {
		fprintf(stderr,
			"[Actuator] Error allocating memory for response topic\n");
		free(message);
		return;
	}

	ostream = pb_ostream_from_buffer(message, encoded_size);
	status = pb_encode(&ostream, GeisaActuatorGet_Rsp_fields, &response);
	if (!status) {
		fprintf(stderr,
			"[Actuator] Error encoding platform actuator get response\n");
		free(rsp_topic);
		free(message);
		return;
	}

	api_publish(mosq, rsp_topic, encoded_size, message, 1);

	free(rsp_topic);
	free(message);
}

void api_actuator_init(struct mosquitto *mosq)
{
	api_register_handler("geisa/api/actuator/get/req",
			     api_actuator_get_req_handler);
	api_register_handler("geisa/api/actuator/set/req",
			     api_actuator_set_req_handler);

	api_subscribe(mosq, "geisa/api/actuator/get/req/#", 1);
	api_subscribe(mosq, "geisa/api/actuator/set/req/#", 1);
}
