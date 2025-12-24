/**
 * @file gapi_discovery.c
 * @brief Definition file for API platform discovery messages
 * @copyright Copyright (C) 2025 Southern California Edison
 */

#include "gapi_discovery.h"

// NOLINTBEGIN(cppcoreguidelines-interfaces-global-init,cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
static PlatformDiscoveryGEISA geisa_platform_info = {
	.base = PROTOBUF_C_MESSAGE_INIT(&platform_discovery__geisa__descriptor),
	.ver_major = 0,
	.ver_minor = 1,
	.ver_rev = 7,
	.piller_adm = true,
	.piller_api = true,
	.piller_lee = true,
	.piller_vee = false,
};

static const PlatformDiscoveryType top_type_platform_info =
	PLATFORM_DISCOVERY__TYPE__DEVICE_TYPE_ELECTRIC_METER;

static PlatformDiscoveryModule top_module_platform_info = {
	.base = PROTOBUF_C_MESSAGE_INIT(
		&platform_discovery__module__descriptor),
	.type = top_type_platform_info,
	.manufacturer = "SCE",
	.model = "GEISA-1",
	.serial_number = "GEISA0001",
	.hw_revision = "A",
	.fw_revision = "1.0.0",
};

static const PlatformDiscoveryType sub_type_platform_info =
	PLATFORM_DISCOVERY__TYPE__DEVICE_TYPE_METROLOGY_PROCESSOR;

static PlatformDiscoveryModule sub_module1_platform_info = {
	.base = PROTOBUF_C_MESSAGE_INIT(
		&platform_discovery__module__descriptor),
	.type = sub_type_platform_info,
	.manufacturer = "SCE",
	.model = "GEISA-2",
	.serial_number = "GEISA0002",
	.hw_revision = "B",
	.fw_revision = "1.1.0",
};

static PlatformDiscoveryModule *sub_modules_platform_info[] = {
	&sub_module1_platform_info};

static PlatformDiscoveryDevice device_platform_info = {
	.base = PROTOBUF_C_MESSAGE_INIT(
		&platform_discovery__device__descriptor),
	.top_module = &top_module_platform_info,
	.n_sub_module = 1,
	.sub_module = sub_modules_platform_info,
};

static PlatformDiscoveryOperator operator_platform_info =
	PLATFORM_DISCOVERY__OPERATOR__INIT;
static PlatformDiscoveryMetrology metrology_platform_info =
	PLATFORM_DISCOVERY__METROLOGY__INIT;
static PlatformDiscoverySensor sensor_platform_info =
	PLATFORM_DISCOVERY__SENSOR__INIT;
static PlatformDiscoveryNetwork network_platform_info =
	PLATFORM_DISCOVERY__NETWORK__INIT;

static const WaveformDatatype waveform_data_type_platform_info =
	WAVEFORM__DATATYPE__DATA_INT32;

static PlatformDiscoveryWaveformInstance waveform_platform_instance1_info = {
	.base = PROTOBUF_C_MESSAGE_INIT(
		&platform_discovery__waveform__instance__descriptor),
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

static PlatformDiscoveryWaveformInstance *waveform_platform_instances[] = {
	&waveform_platform_instance1_info,
};

static PlatformDiscoveryWaveform waveform_platform_info = {
	PROTOBUF_C_MESSAGE_INIT(&platform_discovery__waveform__descriptor),
	.n_instances = 1,
	.instances = waveform_platform_instances,
};
// NOLINTEND(cppcoreguidelines-interfaces-global-init,cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)

static void
api_platform_discovery_build_response(PlatformDiscoveryRsp *response)
{
	response->geisa = &geisa_platform_info;
	response->device = &device_platform_info;
	response->operator_ = &operator_platform_info;
	response->metrology = &metrology_platform_info;
	response->sensor = &sensor_platform_info;
	response->network = &network_platform_info;
	response->waveform = &waveform_platform_info;
}

static void api_platform_discovery_req_handler(struct mosquitto *mosq,
					       const char *topic,
					       const int payloadlen,
					       const uint8_t *payload)
{
	PlatformDiscoveryReq *request = NULL;
	PlatformDiscoveryRsp response = PLATFORM_DISCOVERY__RSP__INIT;
	uint8_t *message = NULL;
	(void)topic;

	fprintf(stdout, "[Discovery] Received platform discovery request\n");
	request = platform_discovery__req__unpack(NULL, payloadlen, payload);
	if (request == NULL) {
		fprintf(stderr, "[Discovery] Error unpacking platform "
				"discovery request\n");
		return;
	}
	platform_discovery__req__free_unpacked(request, NULL);

	api_platform_discovery_build_response(&response);
	message = malloc(platform_discovery__rsp__get_packed_size(&response));
	if (message == NULL) {
		fprintf(stderr, "[Discovery] Error allocating memory for "
				"platform discovery response\n");
		return;
	}
	platform_discovery__rsp__pack(&response, message);
	api_publish(mosq, "geisa/api/platform-discovery-rsp",
		    platform_discovery__rsp__get_packed_size(&response),
		    message, 1);
	free(message);
}

static void api_manifest_req_handler(struct mosquitto *mosq, const char *topic,
				     const int payloadlen,
				     const uint8_t *payload)
{
	(void)mosq;
	fprintf(stdout, "Received app manifest request\n");
	fprintf(stdout, "[msg] topic=%s payload=%.*s\n", topic, payloadlen,
		payload);
}

void api_discovery_init(struct mosquitto *mosq)
{
	api_register_handler("geisa/api/app-manifest-req",
			     api_manifest_req_handler);
	api_register_handler("geisa/api/platform-discovery-req",
			     api_platform_discovery_req_handler);

	api_subscribe(mosq, "geisa/api/platform-discovery-req", 1);
	api_subscribe(mosq, "geisa/api/app-manifest-req/#", 1);
}
