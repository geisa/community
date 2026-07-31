/**
 * @file gapi_manifest.c
 * @brief Definition file for API application manifest messages
 * @copyright Copyright (C) 2026 Southern California Edison
 */

#include "gapi_manifest.h"

static GeisaStatus geisa_manifest_success_status = {
	.code = GeisaStatusCode_GEISA_STATUS_SUCCESS,
	.message = "App manifest retrieval successful",
};

static GeisaStatus geisa_manifest_payload_status = {
	.code = GeisaStatusCode_GEISA_STATUS_CODE_REQUEST_MALFORMED_PAYLOAD,
	.message = "Malformed payload in app manifest request",
};

char *deployment_manifest =
	"{"
	"\"geisa-application-manifest\":{"
	"\"manifest\":{"
	"\"api-access\":{"
	"\"actuator\":true,"
	"\"messaging\":true,"
	"\"instantaneous\":false,"
	"\"sensor\":true,"
	"\"waveform\":false"
	"},"

	"\"app-id\":\"com.example.sce.app\","
	"\"author\":\"SCE\","
	"\"name\":\"API-mockup example\","
	"\"description\":\"API-mockup example\","
	"\"app-version\":\"1.2.3-beta\","
	"\"manifest-version\":\"1.0.0\","

	"\"artifacts\":[{"
	"\"image-name\":\"api_mockup_v1.2.3.img\","
	"\"image-type\":\"appoverlay\","
	"\"image-size\":1048576,"
	"\"uncompressed-size\":5242880,"
	"\"signature\":"
	"\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\""
	"}],"

	"\"compatibility\":{"
	"\"GEISA-API\":\"1.0.0\","
	"\"GEISA-LEE\":\"1.0.0\","
	"\"GEISA-VEE\":null,"
	"\"toolchain-id\":\"acme-toolchain-armv7\","
	"\"toolchain-version\":\"2.3.4\""
	"},"

	"\"resources\":{"
	"\"app-cpu\":10,"
	"\"app-ram\":65536,"
	"\"storage-persistent\":10240,"
	"\"storage-nonpersistent\":2048,"
	"\"threads\":2"
	"},"

	"\"communication\":{"
	"\"FAN\":true,"
	"\"HAN\":false,"
	"\"messaging\":{\"daily-messages\":5000},"
	"\"operator\":{"
	"\"daily-volume\": 5000000,"
	"\"inbound\": [\"TCP:10.1.5.22:8080\"],"
	"\"outbound\": [\"TCP:10.1.5.1:443\"]"
	"},"
	"\"internet\": {"
	"\"daily-volume\": 1000000,"
	"\"outbound\": [\"TCP:8.8.8.8:53\"]"
	"},"
	"\"local\": {"
	"\"daily-volume\": 500000,"
	"\"inbound\": [\"TCP:127.0.0.1:6500\"]"
	"}"
	"},"

	"\"external-dependencies\":["
	"\"com.example.database\","
	"\"com.example.security\""
	"],"

	"\"default-launch-strategy\":{"
	"\"auto-restart\":true,"
	"\"max-restarts\":5,"
	"\"restart-period\":60,"
	"\"start-timeout\":10,"
	"\"start-background\":true,"
	"\"start-string\":\"/usr/bin/start-app\","
	"\"stop-string\":\"/usr/bin/stop-app\","
	"\"stop-timeout\":10,"
	"\"notify-timeout\":30,"
	"\"watchdog\":true"
	"},"

	"\"default-configuration\":{"
	"\"mode\":\"production\","
	"\"message\":\"Hello World\""
	"}"
	"},"
	"\"signature\":"
	"\"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\""
	"}"
	"}";

static void api_manifest_req_handler(struct mosquitto *mosq, const char *topic,
				     const int payloadlen,
				     const uint8_t *payload)
{
	GeisaApplicationDeploymentManifest_Req request =
		GeisaApplicationDeploymentManifest_Req_init_default;
	GeisaApplicationDeploymentManifest_Rsp response =
		GeisaApplicationDeploymentManifest_Rsp_init_default;
	size_t encoded_size = 0;
	uint8_t *message = NULL;
	char *rsp_topic = NULL;
	char *app_id = NULL;
	pb_istream_t istream;
	pb_ostream_t ostream;
	bool status = false;

	app_id = basename((char *)topic);

	fprintf(stdout, "[Manifest] Received app manifest request from %s\n",
		app_id);
	fflush(stdout);

	istream = pb_istream_from_buffer(payload, payloadlen);
	status = pb_decode(&istream,
			   GeisaApplicationDeploymentManifest_Req_fields,
			   &request);
	if (!status) {
		fprintf(stderr,
			"[Manifest] Error decoding app manifest request\n");
		response.status = geisa_manifest_payload_status;
		response.has_status = true;
	} else {
		response.status = geisa_manifest_success_status;
		response.has_status = true;
		response.manifest = deployment_manifest;
	}
	pb_release(GeisaApplicationDeploymentManifest_Req_fields, &request);

	if (asprintf(&rsp_topic, "geisa/api/app/manifest/rsp/%s", app_id) ==
	    -1) {
		fprintf(stderr,
			"[Manifest] Error allocating memory for response "
			"topic\n");
		return;
	}

	status = pb_get_encoded_size(
		&encoded_size, GeisaApplicationDeploymentManifest_Rsp_fields,
		&response);
	if (!status) {
		fprintf(stderr, "[Manifest] Error calculating size of "
				"app manifest response\n");
		return;
	}

	message = malloc(encoded_size);
	if (message == NULL) {
		fprintf(stderr,
			"[Manifest] Error allocating memory for response "
			"message\n");
		free(rsp_topic);
		return;
	}

	ostream = pb_ostream_from_buffer(message, encoded_size);
	status = pb_encode(&ostream,
			   GeisaApplicationDeploymentManifest_Rsp_fields,
			   &response);
	if (!status) {
		fprintf(stderr, "[Manifest] Error encoding app manifest "
				"response\n");
		free(rsp_topic);
		free(message);
		return;
	}

	api_publish(mosq, rsp_topic, encoded_size, message, 1);
	free(rsp_topic);
	free(message);
}

void api_manifest_init(struct mosquitto *mosq)
{
	api_register_handler("geisa/api/app/manifest/req",
			     api_manifest_req_handler);

	api_subscribe(mosq, "geisa/api/app/manifest/req/#", 1);
}
