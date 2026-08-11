/*
 * File: src/geisa-protobuf.c
 * Project: geisa-simple
 * Purpose: Encodes and decodes the GEISA protobuf messages used by geisa-simple
 *
 * Copyright 2026 PragSol Consulting LLC.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "geisa-protobuf.h"

#include <pb_decode.h>
#include <pb_encode.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "app-message.pb.h"
#include "conn-status.pb.h"
#include "discovery.pb.h"
#include "geisa-status.pb.h"
#include "manifest.pb.h"

enum {
    GEISA_DISCOVERY_VERSION_MAJOR = 0,
    GEISA_DISCOVERY_VERSION_MINOR = 9,
    GEISA_DISCOVERY_VERSION_REVISION = 0
};

/*
 * Allocates a buffer, encodes the nanopb message into it, and returns its
 * address through data. The caller releases that buffer with
 * geisa_proto_bytes_free.
 */
static int encode_message(const pb_msgdesc_t *fields, const void *message,
                          size_t max_size, struct geisa_proto_bytes *out) {
    pb_ostream_t stream;
    unsigned char *data = malloc(max_size ? max_size : 1U);
    if (!data) return -1;
    stream = pb_ostream_from_buffer(data, max_size);
    if (!pb_encode(&stream, fields, message)) {
        free(data);
        return -1;
    }
    out->data = data;
    out->len = stream.bytes_written;
    return 0;
}

void geisa_proto_bytes_free(struct geisa_proto_bytes *bytes) {
    /* Release the heap buffer and reset its pointer and length fields. */
    if (!bytes) return;
    free(bytes->data);
    bytes->data = NULL;
    bytes->len = 0;
}

/*
 * Encodes and decodes lifecycle status messages for geisa-simple's RUNNING and
 * SHUTTING_DOWN publications. Encoded buffers belong to the caller.
 */
int geisa_proto_encode_app_status(unsigned type, int next_status,
                                  int next_status_timeout,
                                  struct geisa_proto_bytes *out) {
    GeisaAppToPlatformStatus message = GeisaAppToPlatformStatus_init_zero;
    message.type = (GeisaAppToPlatformStatusType)type;
    message.next_status = next_status;
    message.next_status_timeout = next_status_timeout;
    return encode_message(&GeisaAppToPlatformStatus_msg, &message,
                          GeisaAppToPlatformStatus_size, out);
}

int geisa_proto_decode_app_status(const unsigned char *data, size_t len,
                                  struct geisa_proto_app_status_view *out) {
    GeisaAppToPlatformStatus message = GeisaAppToPlatformStatus_init_zero;
    pb_istream_t stream = pb_istream_from_buffer(data, len);
    if (!pb_decode(&stream, &GeisaAppToPlatformStatus_msg, &message)) return -1;
    out->type = (unsigned)message.type;
    return 0;
}

static void copy_conn_info(struct geisa_proto_conn_app_info *out,
                           const GeisaConnAppInfo *message) {
    /* Copy nanopb's nested connection fields into the output structure. */
    out->policy = (unsigned)message->policy;
    out->yesterday_used_unlimited = message->yesterday_used_unlimited;
    out->yesterday_used_metered = message->yesterday_used_metered;
    out->today_used_unlimited = message->today_used_unlimited;
    out->today_used_metered = message->today_used_metered;
    out->today_limit_metered = message->today_limit_metered;
    out->today_remaining_metered = message->today_remaining_metered;
    out->next_daily_reset_ms = message->next_daily_reset_ms;
}

struct manifest_decode_context {
    char *buffer;
    size_t capacity;
    size_t length;
};

/*
 * Encodes the caller's manifest string through nanopb's callback interface
 */
static bool encode_manifest(pb_ostream_t *stream, const pb_field_t *field,
                            void *const *arg) {
    const char *manifest = *arg;
    if (!pb_encode_tag_for_field(stream, field)) return false;
    return pb_encode_string(stream, (const pb_byte_t *)manifest,
                            strlen(manifest));
}

/*
 * Copies the manifest from the nanopb callback into the manifest buffer in
 * the caller-provided buffer
 */
static bool decode_manifest(pb_istream_t *stream, const pb_field_t *field,
                            void **arg) {
    struct manifest_decode_context *context = *arg;
    size_t remaining = stream->bytes_left;
    if (remaining >= context->capacity - context->length) return false;
    if (!pb_read(stream, (pb_byte_t *)context->buffer + context->length,
                 remaining)) {
        return false;
    }
    context->length += remaining;
    context->buffer[context->length] = '\0';
    (void)field;
    return true;
}

/*
 * Encodes and decodes platform-to-app status messages used for immediate
 * status and shutdown requests, and the platform status log. Decoded fields
 * are copied into the caller-provided struct instance
 */
int geisa_proto_encode_platform_to_app_status(
    const struct geisa_proto_platform_to_app_status_values *values,
    struct geisa_proto_bytes *out) {
    GeisaPlatformToAppStatus message = GeisaPlatformToAppStatus_init_zero;
    message.cmd_send_status = values->cmd_send_status;
    message.cmd_shut_down = values->cmd_shut_down;
    message.cmd_clear_pii = values->cmd_clear_pii;
    message.cpu_usage = values->cpu_usage;
    message.cpu_limit = values->cpu_limit;
    message.memory_usage = values->memory_usage;
    message.memory_limit = values->memory_limit;
    message.persist_storage_usage = values->persist_storage_usage;
    message.persist_storage_limit = values->persist_storage_limit;
    message.nonpersist_storage_usage = values->nonpersist_storage_usage;
    message.nonpersist_storage_limit = values->nonpersist_storage_limit;
    message.has_conn_msg = true;
    message.conn_msg.yesterday_used = values->conn_msg.yesterday_used;
    message.conn_msg.today_used = values->conn_msg.today_used;
    message.conn_msg.today_limit = values->conn_msg.today_limit;
    message.conn_msg.today_remaining = values->conn_msg.today_remaining;
    message.conn_msg.next_daily_reset_ms = values->conn_msg.next_daily_reset_ms;
    message.has_conn_oper = true;
    message.conn_oper.policy = (GeisaConnPolicy)values->conn_oper.policy;
    message.conn_oper.yesterday_used_unlimited =
        values->conn_oper.yesterday_used_unlimited;
    message.conn_oper.yesterday_used_metered =
        values->conn_oper.yesterday_used_metered;
    message.conn_oper.today_used_unlimited =
        values->conn_oper.today_used_unlimited;
    message.conn_oper.today_used_metered = values->conn_oper.today_used_metered;
    message.conn_oper.today_limit_metered =
        values->conn_oper.today_limit_metered;
    message.conn_oper.today_remaining_metered =
        values->conn_oper.today_remaining_metered;
    message.has_conn_inet = true;
    message.conn_inet = message.conn_oper;
    message.has_conn_local = true;
    message.conn_local = message.conn_oper;
    message.timestamp_ms = values->timestamp_ms;
    return encode_message(&GeisaPlatformToAppStatus_msg, &message,
                          GeisaPlatformToAppStatus_size, out);
}

int geisa_proto_decode_platform_to_app_status(
    const unsigned char *data, size_t len,
    struct geisa_proto_platform_to_app_status *out) {
    GeisaPlatformToAppStatus message = GeisaPlatformToAppStatus_init_zero;
    pb_istream_t stream = pb_istream_from_buffer(data, len);
    memset(out, 0, sizeof(*out));
    if (!pb_decode(&stream, &GeisaPlatformToAppStatus_msg, &message)) return -1;
    out->cmd_send_status = message.cmd_send_status;
    out->cmd_shut_down = message.cmd_shut_down;
    out->cmd_clear_pii = message.cmd_clear_pii;
    out->cpu_usage = message.cpu_usage;
    out->cpu_limit = message.cpu_limit;
    out->memory_usage = message.memory_usage;
    out->memory_limit = message.memory_limit;
    out->persist_storage_usage = message.persist_storage_usage;
    out->persist_storage_limit = message.persist_storage_limit;
    out->nonpersist_storage_usage = message.nonpersist_storage_usage;
    out->nonpersist_storage_limit = message.nonpersist_storage_limit;
    if (message.has_conn_msg) {
        out->conn_msg.yesterday_used = message.conn_msg.yesterday_used;
        out->conn_msg.today_used = message.conn_msg.today_used;
        out->conn_msg.today_limit = message.conn_msg.today_limit;
        out->conn_msg.today_remaining = message.conn_msg.today_remaining;
        out->conn_msg.next_daily_reset_ms =
            message.conn_msg.next_daily_reset_ms;
    }
    if (message.has_conn_oper) {
        copy_conn_info(&out->conn_oper, &message.conn_oper);
    }
    if (message.has_conn_inet) {
        copy_conn_info(&out->conn_inet, &message.conn_inet);
    }
    if (message.has_conn_local) {
        copy_conn_info(&out->conn_local, &message.conn_local);
    }
    return 0;
}

int geisa_proto_decode_global_platform_status(
    const unsigned char *data, size_t len,
    struct geisa_proto_global_platform_status *out) {
    GeisaPlatformStatus message = GeisaPlatformStatus_init_zero;
    pb_istream_t stream = pb_istream_from_buffer(data, len);

    if (!out || !pb_decode(&stream, &GeisaPlatformStatus_msg, &message))
        return -1;
    out->timestamp_ms = message.timestamp_ms;
    out->mode = (unsigned)message.mode;
    out->conn_msg = (unsigned)message.conn_msg;
    out->sys_over_temp = message.sys_over_temp;
    out->sys_high_cpu = message.sys_high_cpu;
    out->sys_low_mem = message.sys_low_mem;
    out->sys_power_degraded = message.sys_power_degraded;
    out->sys_power_loss = message.sys_power_loss;
    out->sys_reboot_soon = message.sys_reboot_soon;
    out->sys_shutdown_soon = message.sys_shutdown_soon;
    return 0;
}

/*
 * Encodes and decodes deployment manifest exchanges used during geisa-simple
 * startup. Decoded manifest text is copied into the manifest[] buffer in the
 * caller-provided response structure.
 */
int geisa_proto_encode_manifest_request(struct geisa_proto_bytes *out) {
    GeisaApplicationDeploymentManifest_Req message =
        GeisaApplicationDeploymentManifest_Req_init_zero;
    return encode_message(&GeisaApplicationDeploymentManifest_Req_msg, &message,
                          GeisaApplicationDeploymentManifest_Req_size, out);
}

int geisa_proto_decode_manifest_response(
    const unsigned char *data, size_t len,
    struct geisa_proto_manifest_response_view *out) {
    GeisaApplicationDeploymentManifest_Rsp message =
        GeisaApplicationDeploymentManifest_Rsp_init_zero;
    pb_istream_t stream = pb_istream_from_buffer(data, len);
    struct manifest_decode_context context;
    memset(out, 0, sizeof(*out));
    context.buffer = out->manifest;
    context.capacity = sizeof(out->manifest);
    context.length = 0;
    message.manifest.funcs.decode = decode_manifest;
    message.manifest.arg = &context;
    if (!pb_decode(&stream, &GeisaApplicationDeploymentManifest_Rsp_msg,
                   &message)) {
        return -1;
    }
    out->status_code = (unsigned)message.status.code;
    snprintf(out->status_text, sizeof(out->status_text), "%s",
             message.status.message);
    out->manifest_len = context.length;
    return 0;
}

int geisa_proto_encode_manifest_response(unsigned status_code,
                                         const char *message_text,
                                         const char *details,
                                         const char *manifest,
                                         struct geisa_proto_bytes *out) {
    GeisaApplicationDeploymentManifest_Rsp response =
        GeisaApplicationDeploymentManifest_Rsp_init_zero;
    const char *manifest_value = manifest ? manifest : "";
    response.has_status = true;
    response.status.code = (GeisaStatusCode)status_code;
    snprintf(response.status.message, sizeof(response.status.message), "%s",
             message_text ? message_text : "");
    snprintf(response.status.details, sizeof(response.status.details), "%s",
             details ? details : "");
    response.manifest.funcs.encode = encode_manifest;
    response.manifest.arg = (void *)manifest_value;
    return encode_message(&GeisaApplicationDeploymentManifest_Rsp_msg,
                          &response, GEISA_PROTO_MAX_PAYLOAD_SIZE, out);
}

/*
 * Encodes and decodes Platform Discovery exchanges used during geisa-simple
 * startup. The decoder copies needed GEISA version numbers into the
 * caller-provided output structure.
 */
int geisa_proto_encode_discovery_request(struct geisa_proto_bytes *out) {
    GeisaPlatformDiscovery_Req message = GeisaPlatformDiscovery_Req_init_zero;
    return encode_message(&GeisaPlatformDiscovery_Req_msg, &message,
                          GeisaPlatformDiscovery_Req_size, out);
}

int geisa_proto_encode_discovery_response(struct geisa_proto_bytes *out) {
    GeisaPlatformDiscovery_Rsp response = GeisaPlatformDiscovery_Rsp_init_zero;
    response.has_status = true;
    response.status.code = GeisaStatusCode_GEISA_STATUS_SUCCESS;
    response.has_geisa = true;
    response.geisa.ver_major = GEISA_DISCOVERY_VERSION_MAJOR;
    response.geisa.ver_minor = GEISA_DISCOVERY_VERSION_MINOR;
    response.geisa.ver_rev = GEISA_DISCOVERY_VERSION_REVISION;
    response.geisa.pillar_api = true;
    response.geisa.pillar_lee = true;
    return encode_message(&GeisaPlatformDiscovery_Rsp_msg, &response,
                          GeisaPlatformDiscovery_Rsp_size, out);
}

int geisa_proto_decode_discovery_response(
    const unsigned char *data, size_t len,
    struct geisa_proto_discovery_response_view *out) {
    GeisaPlatformDiscovery_Rsp message = GeisaPlatformDiscovery_Rsp_init_zero;
    pb_istream_t stream = pb_istream_from_buffer(data, len);
    memset(out, 0, sizeof(*out));
    if (!pb_decode(&stream, &GeisaPlatformDiscovery_Rsp_msg, &message)) {
        return -1;
    }
    out->status_code = (unsigned)message.status.code;
    if (message.has_geisa) {
        out->has_geisa_version = 1;
        out->geisa_major = message.geisa.ver_major;
        out->geisa_minor = message.geisa.ver_minor;
        out->geisa_revision = message.geisa.ver_rev;
    }
    return 0;
}

/*
 * Encodes and decodes application-message requests used for geisa-simple
 * CONFIG and EVENT traffic. Decoded bytes are copied into payload_storage in
 * the caller-provided request structure, and payload points to that buffer.
 */
int geisa_proto_encode_app_request(const char *request_id, unsigned priority,
                                   unsigned message_type, uint64_t timestamp_ms,
                                   uint64_t ttl_seconds,
                                   const char *content_type,
                                   const unsigned char *payload,
                                   size_t payload_len,
                                   struct geisa_proto_bytes *out) {
    GeisaAppMessage_Req message = GeisaAppMessage_Req_init_zero;
    if (!payload || payload_len == 0 ||
        payload_len > sizeof(message.payload.bytes)) {
        return -1;
    }
    snprintf(message.request_id, sizeof(message.request_id), "%s", request_id);
    message.priority = (GeisaAppMessagePriority)priority;
    message.message_type = (GeisaAppMessageType)message_type;
    message.timestamp_ms = timestamp_ms;
    message.ttl_seconds = ttl_seconds;
    snprintf(message.content_type, sizeof(message.content_type), "%s",
             content_type);
    message.payload.size = payload_len;
    memcpy(message.payload.bytes, payload, payload_len);
    return encode_message(&GeisaAppMessage_Req_msg, &message,
                          GeisaAppMessage_Req_size, out);
}

int geisa_proto_decode_app_request(const unsigned char *data, size_t len,
                                   struct geisa_proto_app_request_view *out) {
    GeisaAppMessage_Req message = GeisaAppMessage_Req_init_zero;
    pb_istream_t stream = pb_istream_from_buffer(data, len);
    if (!out || !pb_decode(&stream, &GeisaAppMessage_Req_msg, &message) ||
        !message.request_id[0] || !message.content_type[0] ||
        message.payload.size == 0 ||
        message.message_type ==
            GeisaAppMessageType_GEISA_APP_MESSAGE_TYPE_UNSPECIFIED ||
        message.message_type >
            GeisaAppMessageType_GEISA_APP_MESSAGE_TYPE_TELEMETRY ||
        message.priority >
            GeisaAppMessagePriority_GEISA_APP_MESSAGE_PRIORITY_LATEST) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    snprintf(out->request_id, sizeof(out->request_id), "%s",
             message.request_id);
    out->priority = (unsigned)message.priority;
    out->message_type = (unsigned)message.message_type;
    snprintf(out->content_type, sizeof(out->content_type), "%s",
             message.content_type);
    memcpy(out->payload_storage, message.payload.bytes, message.payload.size);
    out->payload = out->payload_storage;
    out->payload_len = message.payload.size;
    return 0;
}

/*
 * Encodes and decodes application-message responses used to correlate CONFIG
 * and EVENT results. Decoded request IDs and status text are copied into
 * character arrays in the caller-provided response structure.
 */
int geisa_proto_encode_app_response(const char *request_id, unsigned status,
                                    const char *status_text,
                                    uint64_t timestamp_ms,
                                    struct geisa_proto_bytes *out) {
    GeisaAppMessage_Rsp message = GeisaAppMessage_Rsp_init_zero;
    snprintf(message.request_id, sizeof(message.request_id), "%s", request_id);
    message.status = (GeisaAppMessageStatus)status;
    snprintf(message.status_text, sizeof(message.status_text), "%s",
             status_text);
    message.timestamp_ms = timestamp_ms;
    return encode_message(&GeisaAppMessage_Rsp_msg, &message,
                          GeisaAppMessage_Rsp_size, out);
}

int geisa_proto_decode_app_response(const unsigned char *data, size_t len,
                                    struct geisa_proto_app_response_view *out) {
    GeisaAppMessage_Rsp message = GeisaAppMessage_Rsp_init_zero;
    pb_istream_t stream = pb_istream_from_buffer(data, len);
    memset(out, 0, sizeof(*out));
    if (!pb_decode(&stream, &GeisaAppMessage_Rsp_msg, &message)) return -1;
    snprintf(out->request_id, sizeof(out->request_id), "%s",
             message.request_id);
    out->status = (unsigned)message.status;
    snprintf(out->status_text, sizeof(out->status_text), "%s",
             message.status_text);
    return 0;
}

const char *geisa_proto_app_status_type_name(unsigned type) {
    /* Map wire enum values to the names used in lifecycle logs. */
    switch (type) {
    case GEISA_PROTO_APP_STATUS_RUNNING: return "RUNNING";
    case GEISA_PROTO_APP_STATUS_SHUTTING_DOWN: return "SHUTTING_DOWN";
    default: return "UNKNOWN";
    }
}
