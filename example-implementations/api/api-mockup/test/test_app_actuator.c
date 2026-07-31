#include "pb.h"
#include "pb_encode.h"
#include "schemas/actuator.pb.h"
#include <mosquitto.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BROKER "localhost"
#define PORT 1883

static bool parse_actuator(const char *value, GeisaTypeActuator *actuator)
{
	if (strcmp(value, "service-switch") == 0) {
		*actuator = GeisaTypeActuator_GEISA_TYPE_ACTUATOR_SERVICE_SWITCH;
		return true;
	}
	if (strcmp(value, "der-switch") == 0) {
		*actuator = GeisaTypeActuator_GEISA_TYPE_ACTUATOR_DER_SWITCH;
		return true;
	}
	if (strcmp(value, "lc-relay-0") == 0) {
		*actuator = GeisaTypeActuator_GEISA_TYPE_ACTUATOR_LC_RELAY_0;
		return true;
	}
	if (strcmp(value, "lc-relay-1") == 0) {
		*actuator = GeisaTypeActuator_GEISA_TYPE_ACTUATOR_LC_RELAY_1;
		return true;
	}
	if (strcmp(value, "lc-relay-2") == 0) {
		*actuator = GeisaTypeActuator_GEISA_TYPE_ACTUATOR_LC_RELAY_2;
		return true;
	}
	if (strcmp(value, "lc-relay-3") == 0) {
		*actuator = GeisaTypeActuator_GEISA_TYPE_ACTUATOR_LC_RELAY_3;
		return true;
	}
	return false;
}

static bool parse_on_off(const char *value, GeisaTypeOnOff *state)
{
	if (strcmp(value, "on") == 0) {
		*state = GeisaTypeOnOff_GEISA_TYPE_ON_OFF_ON;
		return true;
	}
	if (strcmp(value, "off") == 0) {
		*state = GeisaTypeOnOff_GEISA_TYPE_ON_OFF_OFF;
		return true;
	}
	return false;
}

static int publish_encoded(struct mosquitto *mosq, const char *topic,
			   const void *msg, const pb_msgdesc_t *fields)
{
	uint8_t buffer[128];
	pb_ostream_t ostream = pb_ostream_from_buffer(buffer, sizeof(buffer));
	if (!pb_encode(&ostream, fields, msg)) {
		fprintf(stderr, "Encode failed for %s: %s\n", topic,
			PB_GET_ERROR(&ostream));
		return 1;
	}
	if (mosquitto_publish(mosq, NULL, topic, (int)ostream.bytes_written,
			     buffer, 1, false) != MOSQ_ERR_SUCCESS) {
		fprintf(stderr, "Publish failed for %s\n", topic);
		return 1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	struct mosquitto *mosq;
	GeisaActuatorSet_Req set_req = GeisaActuatorSet_Req_init_default;
	GeisaActuatorGet_Req get_req = GeisaActuatorGet_Req_init_default;
	GeisaActuatorStatus status = GeisaActuatorStatus_init_default;
	GeisaTypeActuator actuator;
	GeisaTypeOnOff on_off;
	int rc = 0;

	mosquitto_lib_init();

	if (argc != 3 && argc != 4 && argc != 5) {
		fprintf(stderr,
			"usage: %s --get <actuator> | --set <actuator> <on|off> [position]\n",
			argv[0]);
		mosquitto_lib_cleanup();
		return 1;
	}

	if (strcmp(argv[1], "--get") == 0) {
		if (argc != 3) {
			fprintf(stderr,
				"usage: %s --get <actuator> | --set <actuator> <on|off> [position]\n",
				argv[0]);
			mosquitto_lib_cleanup();
			return 1;
		}
		if (!parse_actuator(argv[2], &actuator)) {
			fprintf(stderr, "invalid actuator: %s\n", argv[2]);
			mosquitto_lib_cleanup();
			return 1;
		}
		get_req.actuator = actuator;
	} else if (strcmp(argv[1], "--set") == 0) {
		if (argc != 4 && argc != 5) {
			fprintf(stderr,
				"usage: %s --set <actuator> <on|off> [position]\n",
				argv[0]);
			mosquitto_lib_cleanup();
			return 1;
		}
		if (!parse_actuator(argv[2], &actuator)) {
			fprintf(stderr, "invalid actuator: %s\n", argv[2]);
			mosquitto_lib_cleanup();
			return 1;
		}
		if (!parse_on_off(argv[3], &on_off)) {
			fprintf(stderr, "invalid state: %s\n", argv[3]);
			mosquitto_lib_cleanup();
			return 1;
		}
		status.actuator = actuator;
		status.on = on_off;
		status.position_present = (argc == 5);
		if (status.position_present) {
			status.position = (int32_t)strtol(argv[4], NULL, 10);
		}
		set_req.new_settings_count = 1;
		set_req.new_settings = &status;
	} else {
		fprintf(stderr,
			"usage: %s --get <actuator> | --set <actuator> <on|off> [position]\n",
			argv[0]);
		mosquitto_lib_cleanup();
		return 1;
	}

	mosq = mosquitto_new(NULL, true, NULL);
	if (!mosq) {
		perror("mosquitto_new");
		mosquitto_lib_cleanup();
		return 1;
	}

	mosquitto_username_pw_set(mosq, "testapp", "testapp");
	if (mosquitto_connect(mosq, BROKER, PORT, 60) != MOSQ_ERR_SUCCESS) {
		fprintf(stderr, "Unable to connect\n");
		mosquitto_destroy(mosq);
		mosquitto_lib_cleanup();
		return 1;
	}

	if (strcmp(argv[1], "--get") == 0) {
		rc = publish_encoded(mosq, "geisa/api/actuator/get/req/testapp",
				     &get_req, GeisaActuatorGet_Req_fields);
	} else {
		rc = publish_encoded(mosq, "geisa/api/actuator/set/req/testapp",
				     &set_req, GeisaActuatorSet_Req_fields);
	}

	if (rc == 0) {
		mosquitto_loop(mosq, 100, 1);
	}

	mosquitto_disconnect(mosq);
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
	return rc;
}
