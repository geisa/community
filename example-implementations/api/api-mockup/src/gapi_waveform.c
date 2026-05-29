/**
 * @file gapi_waveform.c
 * @brief Definition file for API waveform data messages
 * @copyright Copyright (C) 2026 Southern California Edison
 */

#include "gapi_waveform.h"
#include "gapi_discovery.h"

enum { MAX_WAVEFORM_SOCKETS = 16 };

typedef struct {
	char *app_id;
	char *stream_id;
	int fd;
} SocketRecord;

static SocketRecord w_sockets[MAX_WAVEFORM_SOCKETS];
static int w_socket_count = 0;

static int add_socket(const char *app_id, const char *stream_id,
		      int file_descriptor)
{
	if (w_socket_count >= MAX_WAVEFORM_SOCKETS) {
		return -1;
	}
	for (int i = 0; i < w_socket_count; i++) {
		if (strcmp(w_sockets[i].app_id, app_id) == 0 &&
		    strcmp(w_sockets[i].stream_id, stream_id) == 0) {
			return -2;
		}
	}
	w_sockets[w_socket_count].app_id = strdup(app_id);
	w_sockets[w_socket_count].stream_id = strdup(stream_id);
	w_sockets[w_socket_count].fd = file_descriptor;
	w_socket_count++;
	return 0;
}

static int find_socket(const char *app_id, const char *stream_id)
{
	for (int i = 0; i < w_socket_count; i++) {
		if (strcmp(w_sockets[i].app_id, app_id) == 0 &&
		    strcmp(w_sockets[i].stream_id, stream_id) == 0) {
			return i;
		}
	}
	return -1;
}

static int remove_socket(const char *app_id, const char *stream_id)
{
	int idx = find_socket(app_id, stream_id);
	if (idx == -1) {
		return -1;
	}
	free(w_sockets[idx].app_id);
	free(w_sockets[idx].stream_id);
	close(w_sockets[idx].fd);

	for (int i = idx; i < w_socket_count - 1; i++) {
		w_sockets[i] = w_sockets[i + 1];
	}
	w_socket_count--;
	return 0;
}

char *get_dirname(const char *path)
{
	char *path_copy = strdup(path);
	char *dir = dirname(path_copy);
	char *result = strdup(dir);

	free(path_copy);

	return result;
}

static int handle_socket_creation(char *app_id, char *stream_id,
				  char *socket_path)
{
	int permissions = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	struct sockaddr_un addr;
	char *dir_path = NULL;
	struct stat statbuf = {0};
	int file_descriptor = 0;

	if (find_socket(app_id, stream_id) != -1) {
		fprintf(stderr,
			"[Waveform] App %s is already subscribed to stream "
			"%s\n",
			app_id, stream_id);
		return 1;
	}

	file_descriptor = socket(AF_UNIX, SOCK_SEQPACKET, 0);
	if (file_descriptor == -1) {
		fprintf(stderr,
			"[Waveform] Error creating socket for app %s and stream"
			"%s\n",
			app_id, stream_id);
		return 2;
	}

	dir_path = get_dirname(socket_path);
	if (stat(dir_path, &statbuf) == -1) {
		if (mkdir(dir_path, permissions) == -1) {
			fprintf(stderr,
				"[Waveform] Error creating directory for socket for app %s and stream %s\n",
				app_id, stream_id);
			close(file_descriptor);
			free(dir_path);
			return 2;
		}
	}
	free(dir_path);

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

	unlink(socket_path);

	if (bind(file_descriptor, (struct sockaddr *)&addr, sizeof(addr)) ==
	    -1) {
		fprintf(stderr,
			"[Waveform] Error binding socket for app %s and stream "
			"%s (error: %s)\n",
			app_id, stream_id, strerror(errno));
		close(file_descriptor);
		return 2;
	}

	if (add_socket(app_id, stream_id, file_descriptor) != 0) {
		fprintf(stderr,
			"[Waveform] Error tracking socket for app %s stream "
			"%s\n",
			app_id, stream_id);
		close(file_descriptor);
		unlink(socket_path);
		return 2;
	}

	fprintf(stdout,
		"[Waveform] %s successfully subscribed on stream "
		"%s\n",
		app_id, stream_id);

	return 0;
}

static int handle_socket_removal(char *app_id, char *stream_id,
				 char *socket_path)
{
	int ret = 0;

	ret = remove_socket(app_id, stream_id);
	if (ret == -1) {
		fprintf(stderr,
			"[Waveform] App %s is not subscribed to stream "
			"%s\n",
			app_id, stream_id);
		return 1;
	}

	if (unlink(socket_path) == -1) {
		fprintf(stderr,
			"[Waveform] Error removing socket for app %s and stream"
			"%s\n",
			app_id, stream_id);
		return 2;
	}

	fprintf(stdout,
		"[Waveform] %s successfully unsubscribed on stream "
		"%s\n",
		app_id, stream_id);

	return 0;
}

static GeisaWaveform_SampleType
get_sample_type(GeisaWaveform_Datatype sample_type)
{
	switch (sample_type) {
	case GeisaWaveform_Datatype_DATA_INT16:
		return GeisaWaveform_SampleType_WAVEFORM_SAMPLE_TYPE_INT16;
	case GeisaWaveform_Datatype_DATA_INT32:
		return GeisaWaveform_SampleType_WAVEFORM_SAMPLE_TYPE_INT32;
	case GeisaWaveform_Datatype_DATA_FLOAT32:
		return GeisaWaveform_SampleType_WAVEFORM_SAMPLE_TYPE_FLOAT32;
	case GeisaWaveform_Datatype_DATA_FLOAT64:
		return GeisaWaveform_SampleType_WAVEFORM_SAMPLE_TYPE_FLOAT64;
	default:
		return GeisaWaveform_SampleType_WAVEFORM_SAMPLE_TYPE_UNSPECIFIED;
	}
}

static void waveform_handle_stream_request(
	GeisaWaveform_Rsp *response, GeisaWaveform_RequestType request_type,
	int stream_number, char *app_id,
	GeisaPlatformDiscovery_Waveform waveform_platform_info)
{
	char *socket_path = NULL;
	int ret = 0;

	if (asprintf(&socket_path, "/run/geisa/waveform/%s/%s.sock", app_id,
		     waveform_platform_info.streams[stream_number].stream_id) ==
	    -1) {
		fprintf(stderr,
			"[Waveform] Error allocating memory for socket path\n");
		response->waveform_status =
			GeisaWaveform_Status_WAVEFORM_ERR_OTHER;
		return;
	}

	switch (request_type) {
	case GeisaWaveform_RequestType_WAVEFORM_SUBSCRIBE:
		response->stream_id =
			waveform_platform_info.streams[stream_number].stream_id;

		ret = handle_socket_creation(app_id, response->stream_id,
					     socket_path);
		if (ret == 1) {
			response->waveform_status =
				GeisaWaveform_Status_WAVEFORM_ERR_ALREADY_SUBSCRIBED;
			break;
		}
		if (ret == 2) {
			response->waveform_status =
				GeisaWaveform_Status_WAVEFORM_ERR_OTHER;
			break;
		}

		response->subscribed = true;
		response->socket_path = socket_path;
		response->sample_type = get_sample_type(
			waveform_platform_info.streams[stream_number].datatype);
		response->voltage_channel_count =
			waveform_platform_info.streams[stream_number]
				.num_voltage_ch;
		response->current_channel_count =
			waveform_platform_info.streams[stream_number]
				.num_current_ch;
		response->total_channel_count =
			waveform_platform_info.streams[stream_number]
				.total_channel_count;
		response->sample_rate_hz =
			waveform_platform_info.streams[stream_number]
				.sample_rate;
		response->samples_per_cycle =
			waveform_platform_info.streams[stream_number]
				.samples_per_cycle;
		response->nominal_frequency_hz =
			waveform_platform_info.streams[stream_number]
				.nominal_frequency_hz;
		response->cycle_aligned =
			waveform_platform_info.streams[stream_number]
				.cycle_aligned;
		response->zero_crossing_aligned =
			waveform_platform_info.streams[stream_number]
				.zero_crossing_aligned;
		response->voltage_scale =
			waveform_platform_info.streams[stream_number]
				.voltage_multiplier;
		response->current_scale =
			waveform_platform_info.streams[stream_number]
				.current_multiplier;
		response->expected_frame_period_ms =
			waveform_platform_info.streams[stream_number]
				.expected_frame_period_ms;
		response->waveform_status =
			GeisaWaveform_Status_WAVEFORM_SUCCESS;
		break;
	case GeisaWaveform_RequestType_WAVEFORM_UNSUBSCRIBE:
		response->stream_id =
			waveform_platform_info.streams[stream_number].stream_id;

		ret = handle_socket_removal(app_id, response->stream_id,
					    socket_path);
		if (ret == 1) {
			response->waveform_status =
				GeisaWaveform_Status_WAVEFORM_ERR_NOT_SUBSCRIBED;
			break;
		}
		if (ret == 2) {
			response->waveform_status =
				GeisaWaveform_Status_WAVEFORM_ERR_OTHER;
			break;
		}
		response->subscribed = false;
		response->waveform_status =
			GeisaWaveform_Status_WAVEFORM_SUCCESS;
		break;
	default:
		response->waveform_status =
			GeisaWaveform_Status_WAVEFORM_ERR_NOT_SUPPORTED;
		break;
	}

	free(socket_path);
}

static GeisaStatus geisa_waveform_success_status = {
	.code = GeisaStatusCode_GEISA_STATUS_SUCCESS,
	.message = "Waveform request successful",
};

static GeisaStatus geisa_waveform_payload_status = {
	.code = GeisaStatusCode_GEISA_STATUS_CODE_REQUEST_MALFORMED_PAYLOAD,
	.message = "Malformed payload in waveform request",
};

static void api_platform_waveform_build_response(
	GeisaWaveform_Rsp *response, char *stream_id,
	GeisaWaveform_RequestType request_type, char *app_id)
{
	GeisaPlatformDiscovery_Waveform waveform_platform_info;

	waveform_platform_info = get_waveform_info();

	if (waveform_platform_info.streams_count == 0) {
		response->waveform_status =
			GeisaWaveform_Status_WAVEFORM_ERR_NO_RESOURCES;
		return;
	}

	if (stream_id == NULL) {
		response->waveform_status =
			GeisaWaveform_Status_WAVEFORM_ERR_INVALID_STREAM_ID;
		return;
	}

	for (int i = 0; i < waveform_platform_info.streams_count; i++) {
		if (strcmp(waveform_platform_info.streams[i].stream_id,
			   stream_id) == 0) {
			waveform_handle_stream_request(response, request_type,
						       i, app_id,
						       waveform_platform_info);
			break;
		}
	}

	if (response->waveform_status ==
	    GeisaWaveform_Status_WAVEFORM_STATUS_UNSPECIFIED) {
		response->waveform_status =
			GeisaWaveform_Status_WAVEFORM_ERR_UNAVAILABLE;
	}
}

static void api_waveform_req_handler(struct mosquitto *mosq, const char *topic,
				     const int payloadlen,
				     const uint8_t *payload)
{
	GeisaWaveform_Req request = GeisaWaveform_Req_init_default;
	GeisaWaveform_Rsp response = GeisaWaveform_Rsp_init_default;
	size_t encoded_size = 0;
	uint8_t *message = NULL;
	char *rsp_topic = NULL;
	char *app_id = NULL;
	pb_istream_t istream;
	pb_ostream_t ostream;
	bool status = false;

	app_id = basename((char *)topic);

	fprintf(stdout, "[Waveform] Received waveform data request from %s\n",
		app_id);
	fflush(stdout);

	istream = pb_istream_from_buffer(payload, payloadlen);
	status = pb_decode(&istream, GeisaWaveform_Req_fields, &request);
	if (!status) {
		fprintf(stderr,
			"[Waveform] Error decoding waveform app request\n");
		response.status = geisa_waveform_payload_status;
		response.has_status = true;
	} else {
		api_platform_waveform_build_response(
			&response, request.stream_id, request.request_type,
			app_id);
		response.status = geisa_waveform_success_status;
		response.has_status = true;
	}

	pb_release(GeisaWaveform_Req_fields, &request);

	if (asprintf(&rsp_topic, "geisa/api/waveform/rsp/%s", app_id) == -1) {
		fprintf(stderr, "[Waveform] Error allocating memory for "
				"response topic\n");
		return;
	}

	status = pb_get_encoded_size(&encoded_size, GeisaWaveform_Rsp_fields,
				     &response);
	if (!status) {
		fprintf(stderr, "[Waveform] Error calculating size of response "
				"message\n");
		free(rsp_topic);
		return;
	}

	message = malloc(encoded_size);
	if (message == NULL) {
		fprintf(stderr,
			"[Waveform] Error allocating memory for response "
			"message\n");
		free(rsp_topic);
		return;
	}

	ostream = pb_ostream_from_buffer(message, encoded_size);
	status = pb_encode(&ostream, GeisaWaveform_Rsp_fields, &response);
	if (!status) {
		fprintf(stderr, "[Waveform] Error encoding response message\n");
		free(message);
		free(rsp_topic);
		return;
	}

	api_publish(mosq, rsp_topic, encoded_size, message, 1);
	free(message);
}

void api_waveform_init(struct mosquitto *mosq)
{
	int permissions = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	struct stat statbuf = {0};

	api_register_handler("geisa/api/waveform/req",
			     api_waveform_req_handler);

	api_subscribe(mosq, "geisa/api/waveform/req/#", 1);

	if (stat("/run/geisa/", &statbuf) == -1) {
		mkdir("/run/geisa", permissions);
	}
	if (stat("/run/geisa/waveform/", &statbuf) == -1) {
		mkdir("/run/geisa/waveform", permissions);
	}
}

void api_waveform_deinit()
{
	int socket_cleanup_count = w_socket_count;
	char *socket_path = NULL;

	fprintf(stdout, "[Waveform] Cleaning up waveform sockets\n");

	for (int i = 0; i < socket_cleanup_count; i++) {
		if (asprintf(&socket_path, "/run/geisa/waveform/%s/%s.sock",
			     w_sockets[i].app_id,
			     w_sockets[i].stream_id) == -1) {
			fprintf(stderr,
				"[Waveform] Error allocating memory for socket path during cleanup\n");
			continue;
		}

		remove_socket(w_sockets[i].app_id, w_sockets[i].stream_id);
		unlink(socket_path);
		free(socket_path);
	}
}
