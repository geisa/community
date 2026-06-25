#include "pb.h"
#include "pb_decode.h"
#include "pb_encode.h"
#include "schemas/actuator.pb.h"
#include "schemas/discovery.pb.h"
#include "schemas/manifest.pb.h"
#include "schemas/metered_quantities.pb.h"
#include "schemas/waveform.pb.h"
#include <mosquitto.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BROKER "localhost"
#define PORT 1883

void on_connect(struct mosquitto *mosq, void *obj, int rc)
{
	(void)obj;
	if (rc == 0) {
		printf("Connected to broker\n");
		mosquitto_subscribe(mosq, NULL, "geisa/api/instantaneous/data",
				    0);
		mosquitto_subscribe(mosq, NULL,
				    "geisa/api/platform/discovery/rsp/testapp",
				    0);
		mosquitto_subscribe(mosq, NULL,
				    "geisa/api/app/manifest/rsp/testapp", 0);
		mosquitto_subscribe(mosq, NULL,
				    "geisa/api/waveform/rsp/testapp", 0);
		mosquitto_subscribe(mosq, NULL,
				    "geisa/api/actuator/get/rsp/testapp", 0);
		mosquitto_subscribe(mosq, NULL,
				    "geisa/api/actuator/set/rsp/testapp", 0);
	} else {
		printf("Connection failed: %d\n", rc);
	}
}

void handle_instantaneous(const void *payload, size_t len)
{
	bool status;
	pb_istream_t istream;
	GeisaInstantaneousQuantities msg =
		GeisaInstantaneousQuantities_init_default;

	istream = pb_istream_from_buffer(payload, len);

	status = pb_decode(&istream, GeisaInstantaneousQuantities_fields, &msg);
	if (!status) {
		printf("Failed to decode manifest request: %s\n",
		       PB_GET_ERROR(&istream));
		return;
	}
	printf("Decoded instantaneous quantities successfully\n");

	GeisaTypeInstantaneousQuantities_PerPhase a = msg.phase_A;
	GeisaTypeInstantaneousQuantities_PerPhase b = msg.phase_B;
	GeisaTypeInstantaneousQuantities_PerPhase c = msg.phase_C;
	GeisaTypeInstantaneousQuantities_PerPhase n = msg.phase_N;
	printf("Timestamp: %ld\n", msg.timestamp);
	printf("Phase A: microamps:%f microvolts:%f\n", a.microAmps,
	       a.microVolts);
	printf("Phase B: microamps: %f microvolts:%f\n", b.microAmps,
	       b.microVolts);
	printf("Phase C: microamps: %f microvolts:%f\n", c.microAmps,
	       c.microVolts);
	printf("Phase N: microamps: %f microvolts:%f\n", n.microAmps,
	       n.microVolts);

	pb_release(GeisaInstantaneousQuantities_fields, &msg);
}

void handle_discovery(const void *payload, size_t len)
{
	bool status;
	pb_istream_t istream;
	GeisaPlatformDiscovery_Rsp msg =
		GeisaPlatformDiscovery_Rsp_init_default;

	istream = pb_istream_from_buffer(payload, len);
	status = pb_decode(&istream, GeisaPlatformDiscovery_Rsp_fields, &msg);
	if (!status) {
		printf("Failed to decode GeisaPlatformDiscoveryRsp: %s\n",
		       PB_GET_ERROR(&istream));
		return;
	}
	GeisaPlatformDiscovery_Module *sub_module = msg.device.sub_module;
	if (!msg.has_geisa) {
		printf("GEISA field is missing in the response\n");
	}
	printf("Status: %s\n", msg.status.message);
	printf("GEISA: ver_major=%d ver_minor=%d\n", msg.geisa.ver_major,
	       msg.geisa.ver_minor);
	printf("Device: top_model=%s sub_model=%s\n",
	       msg.device.top_module.model, sub_module[0].model);
	printf("waveform: data_connection=%s, sample_rate=%d\n",
	       msg.waveform.streams[0].description,
	       msg.waveform.streams[0].sample_rate);
	printf("Metrology: meter_rating_class:%s, neutral_connected:%d\n",
	       msg.metrology.meter_rating_class,
	       msg.metrology.neutral_connected);
	printf("Sensors: sensor1_name:%s sensor1_type:%d\n",
	       msg.sensor.sensors[0].name, msg.sensor.sensors[0].sensor_type);
	printf("Sensors: sensor2_model:%s sensor2_latitude:%f\n",
	       msg.sensor.sensors[1].model,
	       msg.sensor.sensors[1].geolocation.latitude);
	printf("Network: interface1_id:%s\n",
	       msg.network.interfaces[0].interface_id);

	pb_release(GeisaPlatformDiscovery_Rsp_fields, &msg);
}

void handle_manifest(const void *payload, size_t len)
{
	bool status;
	pb_istream_t istream;
	GeisaApplicationDeploymentManifest_Rsp msg =
		GeisaApplicationDeploymentManifest_Rsp_init_default;

	istream = pb_istream_from_buffer(payload, len);
	status = pb_decode(&istream,
			   GeisaApplicationDeploymentManifest_Rsp_fields, &msg);
	if (!status) {
		printf("Failed to decode ApplicationManifestRsps\n");
		return;
	}
	printf("Status: %s\n", msg.status.message);
	printf("manifest:%s\n", msg.manifest);

	pb_release(GeisaApplicationDeploymentManifest_Rsp_fields, &msg);
}

void handle_waveform(const void *payload, size_t len)
{
	bool status;
	pb_istream_t istream;
	GeisaWaveform_Rsp msg = GeisaWaveform_Rsp_init_default;

	istream = pb_istream_from_buffer(payload, len);
	status = pb_decode(&istream, GeisaWaveform_Rsp_fields, &msg);
	if (!status) {
		printf("Failed to decode WaveformRsp\n");
		return;
	}
	printf("waveform status: %d, subscribed:%d, expected_frame_period_ms: %d\n", msg.waveform_status,
	       msg.subscribed, msg.expected_frame_period_ms);

	pb_release(GeisaWaveform_Rsp_fields, &msg);
}

void handle_actuator_get_response(const void *payload, size_t len)
{
	bool status;
	pb_istream_t istream;
	GeisaActuatorGet_Rsp msg = GeisaActuatorGet_Rsp_init_default;

	istream = pb_istream_from_buffer(payload, len);
	status = pb_decode(&istream, GeisaActuatorGet_Rsp_fields, &msg);
	if (!status) {
		printf("Failed to decode ActuatorGetRsp\n");
		return;
	}
	printf("actuator get response status: %d, message: %s\n",
	       msg.status.code,
	       msg.status.message);
	if (msg.has_actuator_status) {
		printf("actuator get response actuator: %d, on: %d, position_present: %d",
		       msg.actuator_status.actuator,
		       msg.actuator_status.on,
		       msg.actuator_status.position_present);
		if (msg.actuator_status.position_present) {
			printf(", position: %d", msg.actuator_status.position);
		}
		printf("\n");
	}

	pb_release(GeisaActuatorGet_Rsp_fields, &msg);
}

void handle_actuator_set_response(const void *payload, size_t len)
{
	bool status;
	pb_istream_t istream;
	GeisaActuatorSet_Rsp msg = GeisaActuatorSet_Rsp_init_default;

	istream = pb_istream_from_buffer(payload, len);
	status = pb_decode(&istream, GeisaActuatorSet_Rsp_fields, &msg);
	if (!status) {
		printf("Failed to decode ActuatorSetRsp\n");
		return;
	}
	printf("actuator set response status: %d, message: %s\n",
	       msg.status.code,
	       msg.status.message);

	pb_release(GeisaActuatorSet_Rsp_fields, &msg);
}

void on_message(struct mosquitto *mosq, void *obj,
		const struct mosquitto_message *msg)
{
	(void)mosq;
	(void)obj;
	printf("Received message on topic: %s\n", msg->topic);
	if (strcmp(msg->topic, "geisa/api/instantaneous/data") == 0) {
		handle_instantaneous(msg->payload, msg->payloadlen);
	} else if (strcmp(msg->topic,
			  "geisa/api/platform/discovery/rsp/testapp") == 0) {
		handle_discovery(msg->payload, msg->payloadlen);
	} else if (strcmp(msg->topic, "geisa/api/app/manifest/rsp/testapp") ==
		   0) {
		handle_manifest(msg->payload, msg->payloadlen);
	} else if (strcmp(msg->topic, "geisa/api/waveform/rsp/testapp") == 0) {
		handle_waveform(msg->payload, msg->payloadlen);
	} else if (strcmp(msg->topic, "geisa/api/actuator/get/rsp/testapp") == 0) {
		handle_actuator_get_response(msg->payload, msg->payloadlen);
	} else if (strcmp(msg->topic, "geisa/api/actuator/set/rsp/testapp") == 0) {
		handle_actuator_set_response(msg->payload, msg->payloadlen);
	} else {
		printf("Unknown topic\n");
	}
}

int main()
{
	struct mosquitto *mosq;
	mosquitto_lib_init();
	mosq = mosquitto_new(NULL, true, NULL);
	if (!mosq) {
		perror("mosquitto_new");
		return 1;
	}
	mosquitto_connect_callback_set(mosq, on_connect);
	mosquitto_message_callback_set(mosq, on_message);
	mosquitto_username_pw_set(mosq, "testapp", "testapp");
	if (mosquitto_connect(mosq, BROKER, PORT, 60) != MOSQ_ERR_SUCCESS) {
		fprintf(stderr, "Unable to connect\n");
		return 1;
	}
	mosquitto_loop_forever(mosq, -1, 1);
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
	return 0;
}
