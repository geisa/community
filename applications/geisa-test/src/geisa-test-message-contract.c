/*
 * geisa-test-message-contract.c
 *
 * Builds GEISA app-message topics, envelopes, and payload fixtures while
 * keeping transport encoding separate from test payload semantics.
 *
 * Copyright 2026 PragSol Consulting LLC.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "geisa-protobuf.h"
#include "geisa-test-message-contract.h"

#define MAX_PAYLOAD_BYTES 65536U

static int priority_code(const char *priority, unsigned *out) {
    if (strcmp(priority, "GEISA_APP_MESSAGE_PRIORITY_URGENT") == 0)
        *out = 1U;
    else if (strcmp(priority, "GEISA_APP_MESSAGE_PRIORITY_IMMEDIATE") == 0)
        *out = 2U;
    else if (strcmp(priority, "GEISA_APP_MESSAGE_PRIORITY_BEST_EFFORT") == 0)
        *out = 3U;
    else if (strcmp(priority, "GEISA_APP_MESSAGE_PRIORITY_LATEST") == 0)
        *out = 4U;
    else
        return -1;
    return 0;
}

static int message_type_code(const char *message_type, unsigned *out) {
    static const char *const names[] = {
        "",      "CONFIG", "COMMAND",  "COMMAND_RESULT", "STATUS",
        "EVENT", "ALARM",  "APP_DATA", "TELEMETRY"};
    unsigned i;
    for (i = 1U; i < sizeof(names) / sizeof(names[0]); i++) {
        if (strcmp(message_type, names[i]) == 0) {
            *out = i;
            return 0;
        }
    }
    return -1;
}

static const char *status_name(unsigned code) {
    static const char *const names[] = {
        "GEISA_APP_MESSAGE_STATUS_UNSPECIFIED",
        "GEISA_APP_MESSAGE_STATUS_ACCEPTED",
        "GEISA_APP_MESSAGE_STATUS_REJECTED",
        "GEISA_APP_MESSAGE_STATUS_PERMISSION_DENIED",
        "GEISA_APP_MESSAGE_STATUS_QUOTA_EXCEEDED",
        "GEISA_APP_MESSAGE_STATUS_UNAVAILABLE",
        "GEISA_APP_MESSAGE_STATUS_EXPIRED",
        "GEISA_APP_MESSAGE_STATUS_OTHER"};
    return code < sizeof(names) / sizeof(names[0]) ? names[code] : NULL;
}

static int status_code(const char *status, unsigned *out) {
    unsigned i;
    for (i = 1U; i <= 7U; i++) {
        if (strcmp(status, status_name(i)) == 0) {
            *out = i;
            return 0;
        }
    }
    return -1;
}

int geisa_app_message_build_topic(char *out, size_t out_size,
                                  const char *prefix, const char *identity) {
    int written;
    if (!out || !out_size || !prefix || !identity || !identity[0]) return -1;
    written = snprintf(out, out_size, "%s%s", prefix, identity);
    return written >= 0 && (size_t)written < out_size ? 0 : -1;
}

int geisa_app_message_build_payload(size_t size, const char *pattern,
                                    const char *content_type,
                                    unsigned char **out_data, size_t *out_len) {
    unsigned char *data;
    size_t i;
    const char *selected = !pattern || !pattern[0] ? "alpha" : pattern;
    if (!out_data || !out_len || !size || size > MAX_PAYLOAD_BYTES ||
        (strcmp(selected, "alpha") && strcmp(selected, "zero") &&
         strcmp(selected, "index")))
        return -1;
    data = malloc(size + 1U);
    if (!data) return -1;
    if (content_type && strstr(content_type, "json") && size >= 9U) {
        const char *prefix = "{\"v\":\"";
        const char *suffix = "\"}";
        size_t prefix_len = strlen(prefix);
        size_t suffix_len = strlen(suffix);
        memcpy(data, prefix, prefix_len);
        for (i = prefix_len; i < size - suffix_len; i++) {
            size_t offset = i - prefix_len;
            data[i] = strcmp(selected, "zero") == 0 ? '0'
                      : strcmp(selected, "index") == 0
                          ? (unsigned char)('0' + offset % 10U)
                          : (unsigned char)('A' + offset % 26U);
        }
        memcpy(data + size - suffix_len, suffix, suffix_len);
    } else {
        for (i = 0; i < size; i++)
            data[i] = strcmp(selected, "zero") == 0 ? '0'
                      : strcmp(selected, "index") == 0
                          ? (unsigned char)('0' + i % 10U)
                          : (unsigned char)('A' + i % 26U);
    }
    data[size] = '\0';
    *out_data = data;
    *out_len = size;
    return 0;
}

int geisa_app_message_build_request_id(char *out, size_t out_size,
                                       const char *run_id, uint64_t sequence) {
    int written;
    if (!out || !out_size || !run_id || !run_id[0]) return -1;
    written = snprintf(out, out_size, "%s-%llu", run_id,
                       (unsigned long long)sequence);
    return written >= 0 && (size_t)written < out_size ? 0 : -1;
}

int geisa_app_message_build_request(
    const char *request_id, const char *priority, const char *message_type,
    uint64_t timestamp_ms, uint32_t ttl_seconds, const char *content_type,
    const unsigned char *payload, size_t payload_len,
    unsigned char **out_payload, size_t *out_payload_len) {
    /* Keep application JSON opaque: the GEISA envelope owns transport fields;
     * CONFIG and COMMAND handlers own payload meaning. */
    struct geisa_proto_bytes encoded = {0};
    unsigned priority_value;
    unsigned type_value;
    if (!request_id || !priority || !message_type || !content_type ||
        !payload || !out_payload || !out_payload_len || !request_id[0] ||
        !content_type[0] || !payload_len || payload_len > MAX_PAYLOAD_BYTES ||
        priority_code(priority, &priority_value) ||
        message_type_code(message_type, &type_value) ||
        geisa_proto_encode_app_request(request_id, priority_value, type_value,
                                       timestamp_ms, ttl_seconds, content_type,
                                       payload, payload_len, &encoded) != 0) {
        geisa_proto_bytes_free(&encoded);
        return -1;
    }
    *out_payload = encoded.data;
    *out_payload_len = encoded.len;
    return 0;
}

int geisa_app_message_decode_request(
    const unsigned char *payload, size_t payload_len,
    struct geisa_app_message_request_view *out) {
    struct geisa_proto_app_request_view decoded;
    if (!out ||
        geisa_proto_decode_app_request(payload, payload_len, &decoded) != 0 ||
        decoded.payload_len > sizeof(out->payload_storage))
        return -1;
    memset(out, 0, sizeof(*out));
    snprintf(out->request_id, sizeof(out->request_id), "%s",
             decoded.request_id);
    out->message_type = decoded.message_type;
    snprintf(out->content_type, sizeof(out->content_type), "%s",
             decoded.content_type);
    memcpy(out->payload_storage, decoded.payload, decoded.payload_len);
    out->payload = out->payload_storage;
    out->payload_len = decoded.payload_len;
    return 0;
}

int geisa_app_message_build_response(const char *request_id, const char *status,
                                     const char *status_text,
                                     uint64_t timestamp_ms,
                                     unsigned char **out_payload,
                                     size_t *out_payload_len) {
    struct geisa_proto_bytes encoded = {0};
    unsigned code;
    if (!request_id || !status || !out_payload || !out_payload_len ||
        !request_id[0] || status_code(status, &code) ||
        geisa_proto_encode_app_response(request_id, code, status_text,
                                        timestamp_ms, &encoded) != 0) {
        geisa_proto_bytes_free(&encoded);
        return -1;
    }
    *out_payload = encoded.data;
    *out_payload_len = encoded.len;
    return 0;
}

enum geisa_app_message_response_class geisa_app_message_parse_response(
    const unsigned char *payload, size_t payload_len, char *request_id,
    size_t request_id_size, char *status, size_t status_size, char *status_text,
    size_t status_text_size) {
    struct geisa_proto_app_response_view decoded;
    const char *name;
    if (!request_id || !status || !request_id_size || !status_size ||
        (status_text && !status_text_size) ||
        geisa_proto_decode_app_response(payload, payload_len, &decoded) != 0) {
        return GEISA_APP_MESSAGE_RESPONSE_MALFORMED;
    }
    name = status_name(decoded.status);
    if (!name || strlen(decoded.request_id) >= request_id_size ||
        strlen(name) >= status_size ||
        (status_text && strlen(decoded.status_text) >= status_text_size)) {
        return GEISA_APP_MESSAGE_RESPONSE_MALFORMED;
    }
    snprintf(request_id, request_id_size, "%s", decoded.request_id);
    snprintf(status, status_size, "%s", name);
    if (status_text) {
        snprintf(status_text, status_text_size, "%s", decoded.status_text);
    }
    return decoded.status == 1U ? GEISA_APP_MESSAGE_RESPONSE_ACCEPTED
                                : GEISA_APP_MESSAGE_RESPONSE_REJECTED;
}

int geisa_platform_discovery_response_is_success(const unsigned char *payload,
                                                 size_t payload_len) {
    return geisa_proto_discovery_response_is_success(payload, payload_len);
}
