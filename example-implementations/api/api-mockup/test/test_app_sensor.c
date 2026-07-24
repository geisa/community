#include "pb.h"
#include "pb_encode.h"
#include "pb_decode.h"
#include "schemas/sensor.pb.h"
#include <mosquitto.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BROKER "localhost"
#define PORT 1883

static int publish_encoded(struct mosquitto *mosq, const char *topic,
			   const void *msg, const pb_msgdesc_t *fields)
{
	uint8_t *buffer = NULL;
	size_t encoded_size = 0;
	pb_ostream_t ostream;
	bool status;
	int rc = 0;

	status = pb_get_encoded_size(&encoded_size, fields, msg);
	if (!status) {
		fprintf(stderr, "Encode size calc failed for %s\n", topic);
		return 1;
	}

	buffer = malloc(encoded_size);
	if (!buffer) {
		fprintf(stderr, "Allocation failed for encode buffer\n");
		return 1;
	}

	ostream = pb_ostream_from_buffer(buffer, encoded_size);
	if (!pb_encode(&ostream, fields, msg)) {
		fprintf(stderr, "Encode failed for %s: %s\n", topic,
			PB_GET_ERROR(&ostream));
		rc = 1;
		goto cleanup;
	}

	if (mosquitto_publish(mosq, NULL, topic, (int)ostream.bytes_written,
			      buffer, 1, false) != MOSQ_ERR_SUCCESS) {
		fprintf(stderr, "Publish failed for %s\n", topic);
		rc = 1;
		goto cleanup;
	}

cleanup:
	free(buffer);
	return rc;
}

int main(int argc, char **argv)
{
	struct mosquitto *mosq;
	GeisaSensorReadings_Req req = GeisaSensorReadings_Req_init_default;
	int rc = 0;

	if (argc > 1) {
		if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
			fprintf(stderr,
				"usage: %s [sensor_id ...]\n",
				argv[0]);
			return 0;
		}

		int count = argc - 1;
		req.sensor_id_count = (pb_size_t)count;
		req.sensor_id = malloc((size_t)count * sizeof(char *));
		if (!req.sensor_id) {
			fprintf(stderr,
				"Allocation failed for sensor_id pointers\n");
			return 1;
		}
		for (int i = 0; i < count; i++) {
			req.sensor_id[i] = malloc(strlen(argv[i + 1]) + 1);
			if (!req.sensor_id[i]) {
				fprintf(stderr,
					"Allocation failed for sensor_id string\n");
				rc = 1;
				goto cleanup_req;
			}
			strcpy(req.sensor_id[i], argv[i + 1]);
		}
	} else {
		req.sensor_id_count = 0;
		req.sensor_id = NULL;
	}

	mosquitto_lib_init();

	mosq = mosquitto_new(NULL, true, NULL);
	if (!mosq) {
		perror("mosquitto_new");
		rc = 1;
		goto lib_cleanup_mosq;
	}

	mosquitto_username_pw_set(mosq, "testapp", "testapp");
	if (mosquitto_connect(mosq, BROKER, PORT, 60) != MOSQ_ERR_SUCCESS) {
		fprintf(stderr, "Unable to connect\n");
		rc = 1;
		goto destroy_mosq;
	}

	rc = publish_encoded(mosq, "geisa/api/sensor-req/testapp", &req,
			     GeisaSensorReadings_Req_fields);

	if (rc == 0) {
		mosquitto_loop(mosq, 100, 1);
	}

	mosquitto_disconnect(mosq);
destroy_mosq:
	mosquitto_destroy(mosq);
lib_cleanup_mosq:
	mosquitto_lib_cleanup();
cleanup_req:
	pb_release(GeisaSensorReadings_Req_fields, &req);
	return rc;
}
