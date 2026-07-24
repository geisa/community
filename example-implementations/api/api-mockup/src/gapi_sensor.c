/**
 * @file gapi_sensor.c
 * @brief Definition file for API sensor messages
 * @copyright Copyright (C) 2026 Southern California Edison
 */

#define _GNU_SOURCE
#include "gapi_sensor.h"
#include "gapi_discovery.h"
#include "pb.h"
#include "pb_decode.h"
#include "pb_encode.h"
#include "schemas/sensor.pb.h"
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum { SEC_IN_MS = 1000 };
enum { MAX_SENSORS_READINGS = 16 };

typedef struct {
	const char *sensor_id;
	double value;
} sensor_t;

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
static sensor_t sensors[] = {
	{"ambient-temp-1", 25.0},
	{"ambient-humidity-1", 45.0},
	{"contact-closure-1", 0.0},
};
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)

static const size_t sensor_count = sizeof(sensors) / sizeof(sensors[0]);

static GeisaStatus geisa_sensor_success_status = {
	.code = GeisaStatusCode_GEISA_STATUS_SUCCESS,
	.message = "Sensor request successful",
};

static GeisaStatus geisa_sensor_malformed_payload_status = {
	.code = GeisaStatusCode_GEISA_STATUS_CODE_REQUEST_MALFORMED_PAYLOAD,
	.message = "Malformed payload in sensor request",
};

static GeisaStatus geisa_sensor_not_found_status = {
	.code = GeisaStatusCode_GEISA_STATUS_CODE_RESOURCE_NOT_FOUND,
	.message = "Requested sensor not found",
};

static GeisaStatus geisa_sensor_not_supported_status = {
	.code = GeisaStatusCode_GEISA_STATUS_CODE_EXEC_NOT_SUPPORTED,
	.message = "Requested read is not supported by the sensor",
};

static int sensor_find(const char *sensor_id)
{
	if (sensor_id == NULL) {
		return -1;
	}

	for (size_t i = 0; i < sensor_count; i++) {
		if (strcmp(sensor_id, sensors[i].sensor_id) == 0) {
			return (int)i;
		}
	}
	return -1;
}

static void get_all_sensors_info(GeisaSensorReadings_Rsp *response,
				 GeisaPlatformDiscovery_Sensor sensors_desc,
				 GeisaSensorDescriptor sensor_desc,
				 size_t readings_idx, uint64_t now_ms,
				 GeisaSensorReading *readings,
				 GeisaSensorValue *values)
{
	for (size_t i = 0;
	     i < sensor_count && readings_idx < MAX_SENSORS_READINGS; i++) {
		sensor_desc = sensors_desc.sensors[i];
		if (sensor_desc.supports_read) {
			const sensor_t *sensor = &sensors[i];
			GeisaSensorReading *read = &readings[readings_idx];
			GeisaSensorValue *value = &values[readings_idx];

			memset(read, 0, sizeof(*read));
			strncpy(read->sensor_id, sensor->sensor_id,
				sizeof(read->sensor_id) - 1);
			read->timestamp_ms = now_ms;

			memset(value, 0, sizeof(*value));
			value->which_value = GeisaSensorValue_double_value_tag;
			value->value.double_value = sensor->value;
			read->values_count = 1;
			read->values = value;
			read->has_unit = true;
			strncpy(read->unit, sensor_desc.unit,
				sizeof(read->unit));

			readings_idx++;
		}
	}
	response->status = geisa_sensor_success_status;
	response->has_status = true;
	response->readings_count = (pb_size_t)readings_idx;
	response->readings = readings;
}

static void get_requested_sensors_info(
	GeisaSensorReadings_Rsp *response,
	GeisaPlatformDiscovery_Sensor sensors_desc,
	GeisaSensorDescriptor sensor_desc, size_t readings_idx, uint64_t now_ms,
	GeisaSensorReading *readings, GeisaSensorValue *values,
	const GeisaSensorReadings_Req *request)
{
	for (pb_size_t i = 0; i < request->sensor_id_count &&
			      readings_idx < MAX_SENSORS_READINGS;
	     i++) {

		const char *req_id = request->sensor_id[i];
		int idx = sensor_find(req_id);
		// NOLINTNEXTLINE: Out of bound check is done above
		const sensor_t *sensor = &sensors[idx];
		sensor_desc = sensors_desc.sensors[idx];

		GeisaSensorReading *read = &readings[readings_idx];
		GeisaSensorValue *value = &values[readings_idx];

		memset(read, 0, sizeof(*read));
		strncpy(read->sensor_id, sensor->sensor_id,
			sizeof(read->sensor_id) - 1);
		read->timestamp_ms = now_ms;

		memset(value, 0, sizeof(*value));
		value->which_value = GeisaSensorValue_double_value_tag;
		value->value.double_value = sensor->value;
		read->values_count = 1;
		read->values = value;
		read->has_unit = true;
		strncpy(read->unit, sensor_desc.unit, sizeof(read->unit));

		readings_idx++;
	}

	response->status = geisa_sensor_success_status;
	response->has_status = true;
	response->readings_count = (pb_size_t)readings_idx;
	response->readings = readings;
}

static bool geisa_sensor_build_response(const GeisaSensorReadings_Req *request,
					GeisaSensorReadings_Rsp *response)
{
	static GeisaSensorReading readings[MAX_SENSORS_READINGS];
	static GeisaSensorValue values[MAX_SENSORS_READINGS];
	GeisaPlatformDiscovery_Sensor sensors_desc =
		GeisaPlatformDiscovery_Sensor_init_default;
	GeisaSensorDescriptor sensor_desc = GeisaSensorDescriptor_init_default;

	size_t readings_idx = 0;
	uint64_t now_ms = (uint64_t)time(NULL) * SEC_IN_MS;
	sensors_desc = get_sensors_info();

	if (request->sensor_id_count == 0) {
		get_all_sensors_info(response, sensors_desc, sensor_desc,
				     readings_idx, now_ms, readings, values);
		return true;
	}

	for (pb_size_t i = 0; i < request->sensor_id_count; i++) {
		const char *req_id = request->sensor_id[i];
		int idx = sensor_find(req_id);
		if (idx < 0) {
			response->status = geisa_sensor_not_found_status;
			response->has_status = true;
			response->readings_count = 0;
			response->readings = NULL;
			return false;
		}
		if (!sensors_desc.sensors[idx].supports_read) {
			response->status = geisa_sensor_not_supported_status;
			response->has_status = true;
			response->readings_count = 0;
			response->readings = NULL;
			return false;
		}
	}

	get_requested_sensors_info(response, sensors_desc, sensor_desc,
				   readings_idx, now_ms, readings, values,
				   request);
	return true;
}

static void api_sensor_req_handler(struct mosquitto *mosq, const char *topic,
				   const int payloadlen, const uint8_t *payload)
{
	GeisaSensorReadings_Req request = GeisaSensorReadings_Req_init_default;
	GeisaSensorReadings_Rsp response = GeisaSensorReadings_Rsp_init_default;
	size_t encoded_size = 0;
	uint8_t *message = NULL;
	char *rsp_topic = NULL;
	char *app_id = NULL;
	pb_istream_t istream;
	pb_ostream_t ostream;
	bool status = false;

	app_id = basename((char *)topic);

	fprintf(stdout, "[Sensor] Received sensor read request from %s\n",
		app_id);
	fflush(stdout);

	istream = pb_istream_from_buffer(payload, payloadlen);
	status = pb_decode(&istream, GeisaSensorReadings_Req_fields, &request);
	if (!status) {
		fprintf(stderr, "[Sensor] Error decoding sensor request\n");
		response.status = geisa_sensor_malformed_payload_status;
		response.has_status = true;
	} else {
		geisa_sensor_build_response(&request, &response);
	}
	pb_release(GeisaSensorReadings_Req_fields, &request);

	if (asprintf(&rsp_topic, "geisa/api/sensor-rsp/%s", app_id) == -1) {
		fprintf(stderr,
			"[Sensor] Error allocating memory for response topic\n");
		return;
	}

	status = pb_get_encoded_size(&encoded_size,
				     GeisaSensorReadings_Rsp_fields, &response);
	if (!status) {
		fprintf(stderr,
			"[Sensor] Error calculating size of response\n");
		goto rsp_topic_cleanup;
	}

	message = malloc(encoded_size);
	if (message == NULL) {
		fprintf(stderr,
			"[Sensor] Error allocating memory for response message\n");
		goto rsp_topic_cleanup;
	}

	ostream = pb_ostream_from_buffer(message, encoded_size);
	status = pb_encode(&ostream, GeisaSensorReadings_Rsp_fields, &response);
	if (!status) {
		fprintf(stderr, "[Sensor] Error encoding response message\n");
		goto message_cleanup;
	}

	api_publish(mosq, rsp_topic, encoded_size, message, 1);

message_cleanup:
	free(message);
rsp_topic_cleanup:
	free(rsp_topic);
}

void api_sensor_init(struct mosquitto *mosq)
{
	api_register_handler("geisa/api/sensor-req", api_sensor_req_handler);

	api_subscribe(mosq, "geisa/api/sensor-req/#", 0);
}
