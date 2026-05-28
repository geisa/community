/**
 * @file gapi_discovery.c
 * @brief Definition file for API platform discovery messages
 * @copyright Copyright (C) 2025 Southern California Edison
 */

#include "gapi_discovery.h"

// NOLINTBEGIN(cppcoreguidelines-interfaces-global-init,cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
static GeisaPlatformDiscovery_GEISA geisa_platform_info = {
	.ver_major = 0,
	.ver_minor = 1,
	.ver_rev = 7,
	.pillar_adm = true,
	.pillar_api = true,
	.pillar_lee = true,
	.pillar_vee = false,
};

static const GeisaPlatformDiscovery_DeviceType top_type_platform_info =
	GeisaPlatformDiscovery_DeviceType_TYPE_ELECTRIC_METER;

static const GeisaPlatformDiscovery_Module top_module_platform_info = {
	.type = top_type_platform_info,
	.manufacturer = "SCE",
	.model = "GEISA-1",
	.serial_number = "GEISA0001",
	.hw_revision = "A",
	.fw_revision = "1.0.0",
};

static const GeisaPlatformDiscovery_DeviceType sub_type_platform_info =
	GeisaPlatformDiscovery_DeviceType_TYPE_METROLOGY_PROCESSOR;

static GeisaPlatformDiscovery_Module sub_module_platform_info = {
	.type = sub_type_platform_info,
	.manufacturer = "SCE",
	.model = "GEISA-2",
	.serial_number = "GEISA0002",
	.hw_revision = "B",
	.fw_revision = "1.1.0",
};

static GeisaPlatformDiscovery_Device device_platform_info = {
	.top_module = top_module_platform_info,
	.has_top_module = true,
	.sub_module_count = 1,
	.sub_module = &sub_module_platform_info,
};

static GeisaPlatformDiscovery_Operator operator_platform_info = {
	.operator_name = "SCE",
	.operator_identifier = "SCE001",
};

static GeisaPlatformDiscovery_Metrology metrology_platform_info = {
	.meter_rating_class = "A",
	.meter_form = "1S",
	.phase_count = 3,
	.neutral_connected = true,
	.nominal_phase_angle_deg = 120,
	.nominal_frequency_hz = 60,
	.nominal_phase_to_phase_voltage_v = 208,
	.nominal_phase_to_neutral_voltage_v = 120,
};

static GeisaSensorDescriptor sensors_platform_info_descriptors[] = {
	{
		.sensor_type = GeisaSensorType_GEISA_SENSOR_TYPE_TEMPERATURE,
		.name = "Ambient Temperature",
		.has_name = true,
		.unit = "Celsius",
		.supports_read = true,
		.supports_publish = false,
	},
	{
		.sensor_type = GeisaSensorType_GEISA_SENSOR_TYPE_HUMIDITY,
		.manufacturer = "SCE",
		.has_manufacturer = true,
		.model = "HUMID-1000",
		.has_model = true,
		.geolocation =
			{
				.latitude = 34.0522,
				.longitude = -118.2437,
			},
		.has_geolocation = true,
		.unit = "Percent",
		.supports_read = true,
		.supports_publish = false,
	},
};

static GeisaPlatformDiscovery_Sensor sensor_platform_info = {
	.sensors_count = 2,
	.sensors = sensors_platform_info_descriptors,
};

static GeisaPlatformDiscovery_Network_Instance network_interfaces_platform_info[] = {
	{
		.interface_id = "eth0",
		.network_class =
			GeisaPlatformDiscovery_NetworkClass_NETWORK_CLASS_INTERNET,
		.owner =
			GeisaPlatformDiscovery_NetworkOwner_NETWORK_OWNER_OPERATOR,
		.technology =
			GeisaPlatformDiscovery_NetworkTechnology_NETWORK_TECHNOLOGY_ETHERNET,
		.supports_ipv4 = true,
		.supports_ipv6 = false,
		.has_name = true,
		.name = "Ethernet 0",
	},
};

static GeisaPlatformDiscovery_Network network_platform_info = {
	.interfaces_count = 1,
	.interfaces = network_interfaces_platform_info,
};

static const GeisaWaveform_Datatype waveform_data_type_platform_info =
	GeisaWaveform_Datatype_DATA_INT32;

static GeisaPlatformDiscovery_Waveform_Instance waveform_platform_instances = {
	.stream_id = "api-mockup-waveform",
	.name = "API Mockup Waveform",
	.description = "Example waveform instance for API mockup application",
	.datatype = waveform_data_type_platform_info,
	.voltage_multiplier = 0.1,
	.current_multiplier = 0.01,
	.num_voltage_ch = 1,
	.num_current_ch = 1,
	.num_other_ch = 0,
	.total_channel_count = 2,
	.cycle_aligned = true,
	.zero_crossing_aligned = true,
	.sample_rate = 7680,
	.samples_per_cycle = 1280,
	.nominal_frequency_hz = 60,
	.expected_frame_period_ms = 1000,
	.voltage_filter_lowpass = 0,
	.voltage_filter_highpass = 0,
	.current_filter_lowpass = 0,
	.current_filter_highpass = 0,
};

static GeisaPlatformDiscovery_Waveform waveform_platform_info = {
	.streams_count = 1,
	.streams = &waveform_platform_instances,
};

static GeisaStatus geisa_discovery_success_status = {
	.code = GeisaStatusCode_GEISA_STATUS_SUCCESS,
	.message = "Platform discovery successful",
};

static GeisaStatus geisa_discovery_payload_status = {
	.code = GeisaStatusCode_GEISA_STATUS_CODE_REQUEST_MALFORMED_PAYLOAD,
	.message = "Malformed payload in platform discovery request",
};

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

// NOLINTEND(cppcoreguidelines-interfaces-global-init,cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)

static void
api_platform_discovery_build_response(GeisaPlatformDiscovery_Rsp *response)
{
	response->status = geisa_discovery_success_status;
	response->has_status = true;
	response->geisa = geisa_platform_info;
	response->has_geisa = true;
	response->device = device_platform_info;
	response->has_device = true;
	response->operator = operator_platform_info;
	response->has_operator = true;
	response->metrology = metrology_platform_info;
	response->has_metrology = true;
	response->sensor = sensor_platform_info;
	response->has_sensor = true;
	response->network = network_platform_info;
	response->has_network = true;
	response->waveform = waveform_platform_info;
	response->has_waveform = true;
}

GeisaPlatformDiscovery_Waveform get_waveform_info()
{
	return waveform_platform_info;
}

static void api_platform_discovery_req_handler(struct mosquitto *mosq,
					       const char *topic,
					       const int payloadlen,
					       const uint8_t *payload)
{
	GeisaPlatformDiscovery_Req request =
		GeisaPlatformDiscovery_Req_init_default;
	GeisaPlatformDiscovery_Rsp response =
		GeisaPlatformDiscovery_Rsp_init_default;
	uint8_t *message = NULL;
	char *rsp_topic = NULL;
	char *app_id = NULL;
	pb_istream_t istream;
	pb_ostream_t ostream;
	bool status = false;
	size_t encoded_size = 0;

	app_id = basename((char *)topic);

	fprintf(stdout,
		"[Discovery] Received platform discovery request from %s\n",
		app_id);
	fflush(stdout);

	istream = pb_istream_from_buffer(payload, payloadlen);
	status = pb_decode(&istream, GeisaPlatformDiscovery_Req_fields,
			   &request);
	if (!status) {
		fprintf(stderr, "[Discovery] Error decoding platform "
				"discovery request\n");
		response.status = geisa_discovery_payload_status;
		response.has_status = true;
	} else {
		api_platform_discovery_build_response(&response);
	}
	pb_release(GeisaPlatformDiscovery_Req_fields, &request);

	status = pb_get_encoded_size(
		&encoded_size, GeisaPlatformDiscovery_Rsp_fields, &response);
	if (!status) {
		fprintf(stderr, "[Discovery] Error calculating size of "
				"platform discovery response\n");
		return;
	}

	message = malloc(encoded_size);
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

	ostream = pb_ostream_from_buffer(message, encoded_size);
	status = pb_encode(&ostream, GeisaPlatformDiscovery_Rsp_fields,
			   &response);
	if (!status) {
		fprintf(stderr, "[Discovery] Error encoding platform "
				"discovery response\n");
		free(rsp_topic);
		free(message);
		return;
	}

	api_publish(mosq, rsp_topic, encoded_size, message, 1);

	free(rsp_topic);
	free(message);
}

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

void api_discovery_init(struct mosquitto *mosq)
{
	api_register_handler("geisa/api/app/manifest/req",
			     api_manifest_req_handler);
	api_register_handler("geisa/api/platform/discovery/req",
			     api_platform_discovery_req_handler);

	api_subscribe(mosq, "geisa/api/platform/discovery/req/#", 1);
	api_subscribe(mosq, "geisa/api/app/manifest/req/#", 1);
}
