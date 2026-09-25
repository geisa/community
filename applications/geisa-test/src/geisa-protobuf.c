/*
 * geisa-protobuf.c
 *
 * Adapts the pinned GEISA v0.9.0 nanopb bindings for application requests,
 * lifecycle status, discovery, and manifest messages.
 *
 * Copyright 2026 PragSol Consulting LLC.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pb_decode.h>
#include <pb_encode.h>

#include "app-message.pb.h"
#include "conn-status.pb.h"
#include "discovery.pb.h"
#include "geisa-status.pb.h"
#include "manifest.pb.h"
#include "geisa-protobuf.h"

static int encode_message(const pb_msgdesc_t *fields, const void *message,
                          struct geisa_proto_bytes *out) {
    size_t encoded_size = 0;
    pb_ostream_t stream;

    if (!fields || !message || !out ||
        !pb_get_encoded_size(&encoded_size, fields, message)) {
        return -1;
    }
    out->data = encoded_size ? malloc(encoded_size) : NULL;
    out->len = encoded_size;
    if (encoded_size && !out->data) return -1;
    stream = pb_ostream_from_buffer(out->data, out->len);
    if (!pb_encode(&stream, fields, message)) {
        geisa_proto_bytes_free(out);
        return -1;
    }
    out->len = stream.bytes_written;
    return 0;
}

static int decode_message(const pb_msgdesc_t *fields, const void *payload,
                          size_t payload_len, void *message) {
    pb_istream_t stream;
    if (!fields || (!payload && payload_len) || !message) return -1;
    stream = pb_istream_from_buffer(payload, payload_len);
    return pb_decode(&stream, fields, message) ? 0 : -1;
}

static bool manifest_encode_callback(pb_ostream_t *stream,
                                     const pb_field_t *field,
                                     void *const *arg) {
    const char *manifest = arg && *arg ? (const char *)*arg : "";
    return pb_encode_tag_for_field(stream, field) &&
           pb_encode_string(stream, (const uint8_t *)manifest,
                            strlen(manifest));
}

struct manifest_decode_context {
    char *out;
    size_t capacity;
    size_t length;
};

static bool manifest_decode_callback(pb_istream_t *stream,
                                     const pb_field_t *field, void **arg) {
    struct manifest_decode_context *context = arg ? *arg : NULL;
    size_t length;

    (void)field;
    /* Reserve a byte for NUL; reject manifests that do not fit. */
    if (!context || stream->bytes_left >= context->capacity) return false;
    length = stream->bytes_left;
    if (!pb_read(stream, (uint8_t *)context->out, length)) return false;
    context->length = length;
    context->out[length] = '\0';
    return true;
}

void geisa_proto_bytes_free(struct geisa_proto_bytes *bytes) {
    if (!bytes) return;
    free(bytes->data);
    bytes->data = NULL;
    bytes->len = 0;
}

int geisa_proto_encode_app_request(const char *request_id, unsigned priority,
                                   unsigned message_type, uint64_t timestamp_ms,
                                   uint64_t ttl_seconds,
                                   const char *content_type,
                                   const unsigned char *payload,
                                   size_t payload_len,
                                   struct geisa_proto_bytes *out) {
    GeisaAppMessage_Req message = GeisaAppMessage_Req_init_zero;
    if (!request_id || !request_id[0] || !content_type || !content_type[0] ||
        (!payload && payload_len) || !payload_len ||
        payload_len > sizeof(message.payload.bytes) ||
        priority > GeisaAppMessagePriority_GEISA_APP_MESSAGE_PRIORITY_LATEST ||
        message_type < GeisaAppMessageType_GEISA_APP_MESSAGE_TYPE_CONFIG ||
        message_type > GeisaAppMessageType_GEISA_APP_MESSAGE_TYPE_TELEMETRY ||
        strlen(request_id) >= sizeof(message.request_id) ||
        strlen(content_type) >= sizeof(message.content_type))
        return -1;
    snprintf(message.request_id, sizeof(message.request_id), "%s", request_id);
    message.priority = (GeisaAppMessagePriority)priority;
    message.message_type = (GeisaAppMessageType)message_type;
    message.timestamp_ms = timestamp_ms;
    message.ttl_seconds = ttl_seconds;
    snprintf(message.content_type, sizeof(message.content_type), "%s",
             content_type);
    message.payload.size = payload_len;
    memcpy(message.payload.bytes, payload, payload_len);
    return encode_message(&GeisaAppMessage_Req_msg, &message, out);
}

int geisa_proto_decode_app_request(const unsigned char *payload,
                                   size_t payload_len,
                                   struct geisa_proto_app_request_view *out) {
    GeisaAppMessage_Req message = GeisaAppMessage_Req_init_zero;
    if (!out ||
        decode_message(&GeisaAppMessage_Req_msg, payload, payload_len,
                       &message) != 0 ||
        !message.request_id[0] || !message.content_type[0] ||
        !message.payload.size ||
        message.message_type ==
            GeisaAppMessageType_GEISA_APP_MESSAGE_TYPE_UNSPECIFIED ||
        message.message_type >
            GeisaAppMessageType_GEISA_APP_MESSAGE_TYPE_TELEMETRY ||
        message.priority >
            GeisaAppMessagePriority_GEISA_APP_MESSAGE_PRIORITY_LATEST)
        return -1;
    memset(out, 0, sizeof(*out));
    snprintf(out->request_id, sizeof(out->request_id), "%s",
             message.request_id);
    out->priority = (unsigned)message.priority;
    out->message_type = (unsigned)message.message_type;
    out->timestamp_ms = message.timestamp_ms;
    out->ttl_seconds = message.ttl_seconds;
    snprintf(out->content_type, sizeof(out->content_type), "%s",
             message.content_type);
    memcpy(out->payload_storage, message.payload.bytes, message.payload.size);
    out->payload = out->payload_storage;
    out->payload_len = message.payload.size;
    return 0;
}

int geisa_proto_encode_app_response(const char *request_id, unsigned status,
                                    const char *status_text,
                                    uint64_t timestamp_ms,
                                    struct geisa_proto_bytes *out) {
    GeisaAppMessage_Rsp message = GeisaAppMessage_Rsp_init_zero;
    if (!request_id || !request_id[0] ||
        strlen(request_id) >= sizeof(message.request_id) ||
        status > GeisaAppMessageStatus_GEISA_APP_MESSAGE_STATUS_OTHER ||
        (status_text && strlen(status_text) >= sizeof(message.status_text)))
        return -1;
    snprintf(message.request_id, sizeof(message.request_id), "%s", request_id);
    message.status = (GeisaAppMessageStatus)status;
    if (status_text)
        snprintf(message.status_text, sizeof(message.status_text), "%s",
                 status_text);
    message.timestamp_ms = timestamp_ms;
    return encode_message(&GeisaAppMessage_Rsp_msg, &message, out);
}

int geisa_proto_decode_app_response(const unsigned char *payload,
                                    size_t payload_len,
                                    struct geisa_proto_app_response_view *out) {
    GeisaAppMessage_Rsp message = GeisaAppMessage_Rsp_init_zero;
    if (!out ||
        decode_message(&GeisaAppMessage_Rsp_msg, payload, payload_len,
                       &message) != 0 ||
        !message.request_id[0] ||
        message.status ==
            GeisaAppMessageStatus_GEISA_APP_MESSAGE_STATUS_UNSPECIFIED ||
        message.status > GeisaAppMessageStatus_GEISA_APP_MESSAGE_STATUS_OTHER)
        return -1;
    memset(out, 0, sizeof(*out));
    snprintf(out->request_id, sizeof(out->request_id), "%s",
             message.request_id);
    out->status = (unsigned)message.status;
    snprintf(out->status_text, sizeof(out->status_text), "%s",
             message.status_text);
    out->timestamp_ms = message.timestamp_ms;
    return 0;
}

static int app_status_type_is_assigned(unsigned type) {
    switch (type) {
    case GeisaAppToPlatformStatusType_RUNNING:
    case GeisaAppToPlatformStatusType_CLEARED_PII:
    case GeisaAppToPlatformStatusType_SHUTTING_DOWN:
    case GeisaAppToPlatformStatusType_NEED_TERMINATE_RESTART:
    case GeisaAppToPlatformStatusType_NEED_TERMINATE_NORESTART: return 1;
    default: return 0;
    }
}

int geisa_proto_encode_app_status(unsigned type, int next_status,
                                  int next_status_timeout,
                                  struct geisa_proto_bytes *out) {
    GeisaAppToPlatformStatus message = GeisaAppToPlatformStatus_init_zero;
    if (!app_status_type_is_assigned(type)) {
        return -1;
    }
    message.type = (GeisaAppToPlatformStatusType)type;
    message.next_status = next_status;
    message.next_status_timeout = next_status_timeout;
    return encode_message(&GeisaAppToPlatformStatus_msg, &message, out);
}

int geisa_proto_decode_app_status(const unsigned char *payload,
                                  size_t payload_len,
                                  struct geisa_proto_app_status_view *out) {
    GeisaAppToPlatformStatus message = GeisaAppToPlatformStatus_init_zero;
    if (!out ||
        decode_message(&GeisaAppToPlatformStatus_msg, payload, payload_len,
                       &message) != 0 ||
        !app_status_type_is_assigned((unsigned)message.type))
        return -1;
    out->type = (unsigned)message.type;
    out->next_status = message.next_status;
    out->next_status_timeout = message.next_status_timeout;
    return 0;
}

const char *geisa_proto_app_status_type_name(unsigned type) {
    switch (type) {
    case GeisaAppToPlatformStatusType_RUNNING: return "RUNNING";
    case GeisaAppToPlatformStatusType_CLEARED_PII: return "CLEARED_PII";
    case GeisaAppToPlatformStatusType_SHUTTING_DOWN: return "SHUTTING_DOWN";
    case GeisaAppToPlatformStatusType_NEED_TERMINATE_RESTART:
        return "NEED_TERMINATE_RESTART";
    case GeisaAppToPlatformStatusType_NEED_TERMINATE_NORESTART:
        return "NEED_TERMINATE_NORESTART";
    default: return NULL;
    }
}

int geisa_proto_decode_platform_to_app_status(
    const unsigned char *payload, size_t payload_len,
    struct geisa_proto_platform_to_app_status *out) {
    GeisaPlatformToAppStatus message = GeisaPlatformToAppStatus_init_zero;
    if (!out || decode_message(&GeisaPlatformToAppStatus_msg, payload,
                               payload_len, &message) != 0)
        return -1;
    memset(out, 0, sizeof(*out));
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
    out->conn_msg.yesterday_used = message.conn_msg.yesterday_used;
    out->conn_msg.today_used = message.conn_msg.today_used;
    out->conn_msg.today_limit = message.conn_msg.today_limit;
    out->conn_msg.today_remaining = message.conn_msg.today_remaining;
    out->conn_msg.next_daily_reset_ms = message.conn_msg.next_daily_reset_ms;
    out->conn_oper.policy = message.conn_oper.policy;
    out->conn_oper.yesterday_used_unlimited =
        message.conn_oper.yesterday_used_unlimited;
    out->conn_oper.yesterday_used_metered =
        message.conn_oper.yesterday_used_metered;
    out->conn_oper.today_used_unlimited =
        message.conn_oper.today_used_unlimited;
    out->conn_oper.today_used_metered = message.conn_oper.today_used_metered;
    out->conn_oper.today_limit_metered = message.conn_oper.today_limit_metered;
    out->conn_oper.today_remaining_metered =
        message.conn_oper.today_remaining_metered;
    out->conn_oper.next_daily_reset_ms = message.conn_oper.next_daily_reset_ms;
    out->conn_inet = out->conn_oper;
    out->conn_inet.policy = message.conn_inet.policy;
    out->conn_inet.yesterday_used_unlimited =
        message.conn_inet.yesterday_used_unlimited;
    out->conn_inet.yesterday_used_metered =
        message.conn_inet.yesterday_used_metered;
    out->conn_inet.today_used_unlimited =
        message.conn_inet.today_used_unlimited;
    out->conn_inet.today_used_metered = message.conn_inet.today_used_metered;
    out->conn_inet.today_limit_metered = message.conn_inet.today_limit_metered;
    out->conn_inet.today_remaining_metered =
        message.conn_inet.today_remaining_metered;
    out->conn_inet.next_daily_reset_ms = message.conn_inet.next_daily_reset_ms;
    out->conn_local = out->conn_inet;
    out->conn_local.policy = message.conn_local.policy;
    out->conn_local.yesterday_used_unlimited =
        message.conn_local.yesterday_used_unlimited;
    out->conn_local.yesterday_used_metered =
        message.conn_local.yesterday_used_metered;
    out->conn_local.today_used_unlimited =
        message.conn_local.today_used_unlimited;
    out->conn_local.today_used_metered = message.conn_local.today_used_metered;
    out->conn_local.today_limit_metered =
        message.conn_local.today_limit_metered;
    out->conn_local.today_remaining_metered =
        message.conn_local.today_remaining_metered;
    out->conn_local.next_daily_reset_ms =
        message.conn_local.next_daily_reset_ms;
    return 0;
}

static int fits_uint32(uint64_t value) { return value <= UINT32_MAX; }

static void fill_conn_msg(GeisaConnAppMsg *target,
                          const struct geisa_proto_conn_app_msg *source) {
    target->yesterday_used = (uint32_t)source->yesterday_used;
    target->today_used = (uint32_t)source->today_used;
    target->today_limit = (uint32_t)source->today_limit;
    target->today_remaining = (uint32_t)source->today_remaining;
    target->next_daily_reset_ms = source->next_daily_reset_ms;
}

static void fill_conn_info(GeisaConnAppInfo *target,
                           const struct geisa_proto_conn_app_info *source) {
    target->policy = (GeisaConnPolicy)source->policy;
    target->yesterday_used_unlimited = source->yesterday_used_unlimited;
    target->yesterday_used_metered = source->yesterday_used_metered;
    target->today_used_unlimited = source->today_used_unlimited;
    target->today_used_metered = source->today_used_metered;
    target->today_limit_metered = source->today_limit_metered;
    target->today_remaining_metered = source->today_remaining_metered;
    target->next_daily_reset_ms = source->next_daily_reset_ms;
}

int geisa_proto_encode_platform_to_app_status(
    const struct geisa_proto_platform_to_app_status_values *values,
    struct geisa_proto_bytes *out) {
    GeisaPlatformToAppStatus message = GeisaPlatformToAppStatus_init_zero;

    if (!values || !out || values->cpu_usage < 0.0f ||
        values->cpu_limit < 0.0f || values->cpu_usage > 100.0f ||
        values->cpu_limit > 100.0f ||
        values->conn_oper.policy > GeisaConnPolicy_CONN_POLICY_UNLIMITED ||
        values->conn_inet.policy > GeisaConnPolicy_CONN_POLICY_UNLIMITED ||
        values->conn_local.policy > GeisaConnPolicy_CONN_POLICY_UNLIMITED ||
        !fits_uint32(values->memory_usage) ||
        !fits_uint32(values->memory_limit) ||
        !fits_uint32(values->persist_storage_usage) ||
        !fits_uint32(values->persist_storage_limit) ||
        !fits_uint32(values->nonpersist_storage_usage) ||
        !fits_uint32(values->nonpersist_storage_limit) ||
        !fits_uint32(values->conn_msg.yesterday_used) ||
        !fits_uint32(values->conn_msg.today_used) ||
        !fits_uint32(values->conn_msg.today_limit) ||
        !fits_uint32(values->conn_msg.today_remaining))
        return -1;

    message.cmd_send_status = values->cmd_send_status != 0;
    message.cmd_shut_down = values->cmd_shut_down != 0;
    message.cmd_clear_pii = values->cmd_clear_pii != 0;
    message.cpu_usage = values->cpu_usage;
    message.cpu_limit = values->cpu_limit;
    message.memory_usage = (uint32_t)values->memory_usage;
    message.memory_limit = (uint32_t)values->memory_limit;
    message.persist_storage_usage = (uint32_t)values->persist_storage_usage;
    message.persist_storage_limit = (uint32_t)values->persist_storage_limit;
    message.nonpersist_storage_usage =
        (uint32_t)values->nonpersist_storage_usage;
    message.nonpersist_storage_limit =
        (uint32_t)values->nonpersist_storage_limit;
    message.has_conn_msg = true;
    fill_conn_msg(&message.conn_msg, &values->conn_msg);
    message.has_conn_oper = true;
    fill_conn_info(&message.conn_oper, &values->conn_oper);
    message.has_conn_inet = true;
    fill_conn_info(&message.conn_inet, &values->conn_inet);
    message.has_conn_local = true;
    fill_conn_info(&message.conn_local, &values->conn_local);
    message.timestamp_ms = values->timestamp_ms;
    return encode_message(&GeisaPlatformToAppStatus_msg, &message, out);
}

int geisa_proto_encode_manifest_request(struct geisa_proto_bytes *out) {
    GeisaApplicationDeploymentManifest_Req message =
        GeisaApplicationDeploymentManifest_Req_init_zero;
    return encode_message(&GeisaApplicationDeploymentManifest_Req_msg, &message,
                          out);
}

int geisa_proto_encode_manifest_response(unsigned status_code,
                                         const char *message_text,
                                         const char *details,
                                         const char *manifest,
                                         struct geisa_proto_bytes *out) {
    GeisaApplicationDeploymentManifest_Rsp response =
        GeisaApplicationDeploymentManifest_Rsp_init_zero;
    const char *manifest_value = manifest ? manifest : "";
    if (status_code > GeisaStatusCode_GEISA_STATUS_CODE_DATA_STALE ||
        strlen(message_text ? message_text : "") >=
            sizeof(response.status.message) ||
        strlen(details ? details : "") >= sizeof(response.status.details))
        return -1;
    response.has_status = true;
    response.status.code = (GeisaStatusCode)status_code;
    snprintf(response.status.message, sizeof(response.status.message), "%s",
             message_text ? message_text : "");
    snprintf(response.status.details, sizeof(response.status.details), "%s",
             details ? details : "");
    response.manifest.funcs.encode = manifest_encode_callback;
    response.manifest.arg = (void *)manifest_value;
    return encode_message(&GeisaApplicationDeploymentManifest_Rsp_msg,
                          &response, out);
}

int geisa_proto_decode_manifest_response(
    const unsigned char *payload, size_t payload_len,
    struct geisa_proto_manifest_response_view *out) {
    GeisaApplicationDeploymentManifest_Rsp response =
        GeisaApplicationDeploymentManifest_Rsp_init_zero;
    struct manifest_decode_context context;
    if (!out) return -1;
    memset(out, 0, sizeof(*out));
    context.out = out->manifest;
    context.capacity = sizeof(out->manifest);
    context.length = 0;
    response.manifest.funcs.decode = manifest_decode_callback;
    response.manifest.arg = &context;
    if (decode_message(&GeisaApplicationDeploymentManifest_Rsp_msg, payload,
                       payload_len, &response) != 0 ||
        !response.has_status)
        return -1;
    out->status_code = (unsigned)response.status.code;
    out->manifest_len = context.length;
    return 0;
}

int geisa_proto_encode_discovery_request(struct geisa_proto_bytes *out) {
    GeisaPlatformDiscovery_Req message = GeisaPlatformDiscovery_Req_init_zero;
    return encode_message(&GeisaPlatformDiscovery_Req_msg, &message, out);
}

int geisa_proto_encode_discovery_response(const char *platform_id,
                                          struct geisa_proto_bytes *out) {
    GeisaPlatformDiscovery_Rsp response = GeisaPlatformDiscovery_Rsp_init_zero;
    const char *identity = platform_id ? platform_id : "";
    response.has_status = true;
    response.status.code = GeisaStatusCode_GEISA_STATUS_SUCCESS;
    snprintf(response.status.message, sizeof(response.status.message), "%s",
             "Platform discovery response");
    response.has_geisa = true;
    response.geisa.ver_major = 0;
    response.geisa.ver_minor = 9;
    response.geisa.ver_rev = 0;
    response.geisa.pillar_adm = true;
    response.geisa.pillar_api = true;
    response.geisa.pillar_lee = true;
    if (identity[0] != '\0') {
        response.has_device = true;
        response.device.has_top_module = true;
        snprintf(response.device.top_module.serial_number,
                 sizeof(response.device.top_module.serial_number), "%s",
                 identity);
    }
    return encode_message(&GeisaPlatformDiscovery_Rsp_msg, &response, out);
}

int geisa_proto_discovery_response_is_success(const unsigned char *payload,
                                              size_t payload_len) {
    GeisaPlatformDiscovery_Rsp response = GeisaPlatformDiscovery_Rsp_init_zero;
    return decode_message(&GeisaPlatformDiscovery_Rsp_msg, payload, payload_len,
                          &response) == 0 &&
           response.has_status &&
           response.status.code == GeisaStatusCode_GEISA_STATUS_SUCCESS;
}
