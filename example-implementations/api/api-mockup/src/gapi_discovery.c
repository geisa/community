/**
 * @file gapi_discovery.c
 * @brief Definition file for API platform discovery messages
 * @copyright Copyright (C) 2025 Southern California Edison
 */

#include "gapi_discovery.h"

// NOLINTBEGIN(cppcoreguidelines-interfaces-global-init,cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
static GeisaPlatformDiscoveryGEISA geisa_platform_info = {
	.base = PROTOBUF_C_MESSAGE_INIT(
		&geisa_platform_discovery__geisa__descriptor),
	.ver_major = 0,
	.ver_minor = 1,
	.ver_rev = 7,
	.pillar_adm = true,
	.pillar_api = true,
	.pillar_lee = true,
	.pillar_vee = false,
};

static const GeisaPlatformDiscoveryType top_type_platform_info =
	GEISA_PLATFORM_DISCOVERY__TYPE__DEVICE_TYPE_ELECTRIC_METER;

static GeisaPlatformDiscoveryModule top_module_platform_info = {
	.base = PROTOBUF_C_MESSAGE_INIT(
		&geisa_platform_discovery__module__descriptor),
	.type = top_type_platform_info,
	.manufacturer = "SCE",
	.model = "GEISA-1",
	.serial_number = "GEISA0001",
	.hw_revision = "A",
	.fw_revision = "1.0.0",
};

static const GeisaPlatformDiscoveryType sub_type_platform_info =
	GEISA_PLATFORM_DISCOVERY__TYPE__DEVICE_TYPE_METROLOGY_PROCESSOR;

static GeisaPlatformDiscoveryModule sub_module1_platform_info = {
	.base = PROTOBUF_C_MESSAGE_INIT(
		&geisa_platform_discovery__module__descriptor),
	.type = sub_type_platform_info,
	.manufacturer = "SCE",
	.model = "GEISA-2",
	.serial_number = "GEISA0002",
	.hw_revision = "B",
	.fw_revision = "1.1.0",
};

static GeisaPlatformDiscoveryModule *sub_modules_platform_info[] = {
	&sub_module1_platform_info};

static GeisaPlatformDiscoveryDevice device_platform_info = {
	.base = PROTOBUF_C_MESSAGE_INIT(
		&geisa_platform_discovery__device__descriptor),
	.top_module = &top_module_platform_info,
	.n_sub_module = 1,
	.sub_module = sub_modules_platform_info,
};

static GeisaPlatformDiscoveryOperator operator_platform_info = {
	.base = PROTOBUF_C_MESSAGE_INIT(
		&geisa_platform_discovery__operator__descriptor),
	.operator_name = "SCE",
	.operator_identifier = "SCE001",
};

static GeisaPlatformDiscoveryMetrology metrology_platform_info =
	GEISA_PLATFORM_DISCOVERY__METROLOGY__INIT;
static GeisaPlatformDiscoverySensor sensor_platform_info =
	GEISA_PLATFORM_DISCOVERY__SENSOR__INIT;
static GeisaPlatformDiscoveryNetwork network_platform_info =
	GEISA_PLATFORM_DISCOVERY__NETWORK__INIT;

static const GeisaWaveformDatatype waveform_data_type_platform_info =
	GEISA_WAVEFORM__DATATYPE__DATA_INT32;

static GeisaPlatformDiscoveryWaveformInstance waveform_platform_instance1_info = {
	.base = PROTOBUF_C_MESSAGE_INIT(
		&geisa_platform_discovery__waveform__instance__descriptor),
	.instance_id = 1,
	.data_connection = "/data/waveform/instance1.socket",
	.datatype = waveform_data_type_platform_info,
	.voltage_multiplier = 0.1,
	.current_multiplier = 0.01,
	.num_voltage_ch = 1,
	.num_current_ch = 1,
	.num_other_ch = 0,
	.sample_rate_is_cycle_locked = true,
	.sample_rate = 7680,
	.frame_is_cycle_aligned = true,
	.frame_duration_samples = 200,
	.voltage_filter_lowpass = 0,
	.voltage_filter_highpass = 0,
	.current_filter_lowpass = 0,
	.current_filter_highpass = 0,
};

static GeisaPlatformDiscoveryWaveformInstance *waveform_platform_instances[] = {
	&waveform_platform_instance1_info,
};

static GeisaPlatformDiscoveryWaveform waveform_platform_info = {
	PROTOBUF_C_MESSAGE_INIT(
		&geisa_platform_discovery__waveform__descriptor),
	.n_instances = 1,
	.instances = waveform_platform_instances,
};

char *deployment_manifest =
	"{"
	"\"geisa-application-manifest\":{"
	"\"manifest\":{"
	"\"app-id\":\"com.example.sce.app\","
	"\"author\":\"SCE\","
	"\"name\":\"API-mockup example\","
	"\"description\":\"API-mockup example\","
	"\"app-version\":\"1.2.3-beta\","
	"\"manifest-version\":\"1.0.0\","

	"\"api-access\":{"
	"\"actuator\":true,"
	"\"messaging\":true,"
	"\"instantaneous\":false,"
	"\"sensor\":true,"
	"\"waveform\":false"
	"},"

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
	"\"messaging\":{\"daily-messages\":5000}"
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

// NOLINTEND(cppcoreguidelines-interfaces-global-init,cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)

static void
api_platform_discovery_build_response(GeisaPlatformDiscoveryRsp *response)
{
	response->geisa = &geisa_platform_info;
	response->device = &device_platform_info;
	response->operator_ = &operator_platform_info;
	response->metrology = &metrology_platform_info;
	response->sensor = &sensor_platform_info;
	response->network = &network_platform_info;
	response->waveform = &waveform_platform_info;
}

GeisaPlatformDiscoveryWaveform get_waveform_info()
{
	return waveform_platform_info;
}

static void api_platform_discovery_req_handler(struct mosquitto *mosq,
					       const char *topic,
					       const int payloadlen,
					       const uint8_t *payload)
{
	GeisaPlatformDiscoveryReq *request = NULL;
	GeisaPlatformDiscoveryRsp response =
		GEISA_PLATFORM_DISCOVERY__RSP__INIT;
	uint8_t *message = NULL;
	char *rsp_topic = NULL;
	char *app_id = NULL;

	app_id = basename((char *)topic);

	fprintf(stdout,
		"[Discovery] Received platform discovery request from %s\n",
		app_id);
	fflush(stdout);
	request = geisa_platform_discovery__req__unpack(NULL, payloadlen,
							payload);
	if (request == NULL) {
		fprintf(stderr, "[Discovery] Error unpacking platform "
				"discovery request\n");
		return;
	}
	geisa_platform_discovery__req__free_unpacked(request, NULL);

	api_platform_discovery_build_response(&response);
	message = malloc(
		geisa_platform_discovery__rsp__get_packed_size(&response));
	if (message == NULL) {
		fprintf(stderr, "[Discovery] Error allocating memory for "
				"platform discovery response\n");
		return;
	}

	if (asprintf(&rsp_topic, "geisa/api/platform/discovery/rsp/%s",
		     app_id) == -1) {
		fprintf(stderr,
			"[Discovery] Error allocating memory for response "
			"topic\n");
		free(message);
		return;
	}

	geisa_platform_discovery__rsp__pack(&response, message);
	api_publish(mosq, rsp_topic,
		    geisa_platform_discovery__rsp__get_packed_size(&response),
		    message, 1);
	free(rsp_topic);
	free(message);
}

static void api_manifest_req_handler(struct mosquitto *mosq, const char *topic,
				     const int payloadlen,
				     const uint8_t *payload)
{
	GeisaApplicationDeploymentManifestReq *request = NULL;
	GeisaApplicationDeploymentManifestRsp response =
		GEISA_APPLICATION_DEPLOYMENT_MANIFEST__RSP__INIT;
	uint8_t *message = NULL;
	char *rsp_topic = NULL;
	char *app_id = NULL;

	app_id = basename((char *)topic);

	fprintf(stdout, "[Manifest] Received app manifest request from %s\n",
		app_id);
	fflush(stdout);
	request = geisa_application_deployment_manifest__req__unpack(
		NULL, payloadlen, payload);
	if (request == NULL) {
		fprintf(stderr,
			"[Manifest] Error unpacking app manifest request\n");
		return;
	}
	geisa_application_deployment_manifest__req__free_unpacked(request,
								  NULL);

	if (asprintf(&rsp_topic, "geisa/api/app/manifest/rsp/%s", app_id) ==
	    -1) {
		fprintf(stderr,
			"[Manifest] Error allocating memory for response "
			"topic\n");
		return;
	}

	response.manifest = deployment_manifest;
	message = malloc(
		geisa_application_deployment_manifest__rsp__get_packed_size(
			&response));
	if (message == NULL) {
		fprintf(stderr,
			"[Manifest] Error allocating memory for response "
			"message\n");
		free(rsp_topic);
		return;
	}
	geisa_application_deployment_manifest__rsp__pack(&response, message);
	api_publish(mosq, rsp_topic,
		    geisa_application_deployment_manifest__rsp__get_packed_size(
			    &response),
		    message, 1);
	free(rsp_topic);
	free(message);
}

void api_discovery_init(struct mosquitto *mosq)
{
	api_register_handler("geisa/api/app/manifest/req",
			     api_manifest_req_handler);
	api_register_handler("geisa/api/platform/discovery/req",
			     api_platform_discovery_req_handler);

	api_subscribe(mosq, "geisa/api/platform/discovery/req/#", 1);
	api_subscribe(mosq, "geisa/api/app/manifest/req/#", 1);
}
