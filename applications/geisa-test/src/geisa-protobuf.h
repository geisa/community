/*
 * geisa-protobuf.h
 *
 * Declares the bounded adapter between geisa-test and its pinned GEISA
 * v0.9.0 nanopb bindings.
 *
 * Copyright 2026 PragSol Consulting LLC.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GEISA_PROTOBUF_H
#define GEISA_PROTOBUF_H

#include <stddef.h>
#include <stdint.h>

/* Encoders allocate output bytes; callers release them with
 * geisa_proto_bytes_free(). Decoders copy variable-length fields into fixed
 * views, so returned views do not borrow the input buffer. */
#define GEISA_PROTO_PRIORITY_BEST_EFFORT 3U
#define GEISA_PROTO_MESSAGE_CONFIG 1U
#define GEISA_PROTO_MESSAGE_COMMAND 2U
#define GEISA_PROTO_MESSAGE_EVENT 5U
#define GEISA_PROTO_MESSAGE_ALARM 6U
#define GEISA_PROTO_MESSAGE_APP_DATA 7U
#define GEISA_PROTO_MESSAGE_TELEMETRY 8U
#define GEISA_PROTO_STATUS_SUCCESS 1U
#define GEISA_PROTO_MESSAGE_STATUS_ACCEPTED 1U
#define GEISA_PROTO_MESSAGE_STATUS_REJECTED 2U
#define GEISA_PROTO_APP_STATUS_RUNNING 0U
#define GEISA_PROTO_APP_STATUS_CLEARED_PII 2U
#define GEISA_PROTO_APP_STATUS_SHUTTING_DOWN 3U

struct geisa_proto_bytes {
    unsigned char *data;
    size_t len;
};

struct geisa_proto_app_request_view {
    char request_id[192];
    unsigned priority;
    unsigned message_type;
    uint64_t timestamp_ms;
    uint64_t ttl_seconds;
    char content_type[128];
    unsigned char payload_storage[65536];
    const unsigned char *payload;
    size_t payload_len;
};

struct geisa_proto_app_response_view {
    char request_id[192];
    unsigned status;
    char status_text[256];
    uint64_t timestamp_ms;
};

struct geisa_proto_conn_app_msg {
    uint64_t yesterday_used;
    uint64_t today_used;
    uint64_t today_limit;
    uint64_t today_remaining;
    uint64_t next_daily_reset_ms;
};

struct geisa_proto_conn_app_info {
    unsigned policy;
    uint64_t yesterday_used_unlimited;
    uint64_t yesterday_used_metered;
    uint64_t today_used_unlimited;
    uint64_t today_used_metered;
    uint64_t today_limit_metered;
    uint64_t today_remaining_metered;
    uint64_t next_daily_reset_ms;
};

struct geisa_proto_platform_to_app_status {
    int cmd_send_status;
    int cmd_shut_down;
    int cmd_clear_pii;
    float cpu_usage;
    float cpu_limit;
    uint64_t memory_usage;
    uint64_t memory_limit;
    uint64_t persist_storage_usage;
    uint64_t persist_storage_limit;
    uint64_t nonpersist_storage_usage;
    uint64_t nonpersist_storage_limit;
    struct geisa_proto_conn_app_msg conn_msg;
    struct geisa_proto_conn_app_info conn_oper;
    struct geisa_proto_conn_app_info conn_inet;
    struct geisa_proto_conn_app_info conn_local;
};

struct geisa_proto_platform_to_app_status_values {
    int cmd_send_status;
    int cmd_shut_down;
    int cmd_clear_pii;
    uint64_t timestamp_ms;
    float cpu_usage;
    float cpu_limit;
    uint64_t memory_usage;
    uint64_t memory_limit;
    uint64_t persist_storage_usage;
    uint64_t persist_storage_limit;
    uint64_t nonpersist_storage_usage;
    uint64_t nonpersist_storage_limit;
    struct geisa_proto_conn_app_msg conn_msg;
    struct geisa_proto_conn_app_info conn_oper;
    struct geisa_proto_conn_app_info conn_inet;
    struct geisa_proto_conn_app_info conn_local;
};

struct geisa_proto_app_status_view {
    unsigned type;
    int next_status;
    int next_status_timeout;
};

struct geisa_proto_manifest_response_view {
    unsigned status_code;
    char manifest[65536];
    size_t manifest_len;
};

/* Encoders/decoders return 0 on success and -1 for invalid input, allocation,
 * or protobuf errors. Output structs must be valid pointers. */
int geisa_proto_encode_app_request(const char *request_id, unsigned priority,
                                   unsigned message_type, uint64_t timestamp_ms,
                                   uint64_t ttl_seconds,
                                   const char *content_type,
                                   const unsigned char *payload,
                                   size_t payload_len,
                                   struct geisa_proto_bytes *out);
int geisa_proto_decode_app_request(const unsigned char *payload,
                                   size_t payload_len,
                                   struct geisa_proto_app_request_view *out);
int geisa_proto_encode_app_response(const char *request_id, unsigned status,
                                    const char *status_text,
                                    uint64_t timestamp_ms,
                                    struct geisa_proto_bytes *out);
int geisa_proto_decode_app_response(const unsigned char *payload,
                                    size_t payload_len,
                                    struct geisa_proto_app_response_view *out);

int geisa_proto_encode_app_status(unsigned type, int next_status,
                                  int next_status_timeout,
                                  struct geisa_proto_bytes *out);
int geisa_proto_decode_app_status(const unsigned char *payload,
                                  size_t payload_len,
                                  struct geisa_proto_app_status_view *out);
const char *geisa_proto_app_status_type_name(unsigned type);
int geisa_proto_decode_platform_to_app_status(
    const unsigned char *payload, size_t payload_len,
    struct geisa_proto_platform_to_app_status *out);
int geisa_proto_encode_platform_to_app_status(
    const struct geisa_proto_platform_to_app_status_values *values,
    struct geisa_proto_bytes *out);

int geisa_proto_encode_manifest_request(struct geisa_proto_bytes *out);
int geisa_proto_encode_manifest_response(unsigned status_code,
                                         const char *message,
                                         const char *details,
                                         const char *manifest,
                                         struct geisa_proto_bytes *out);
int geisa_proto_decode_manifest_response(
    const unsigned char *payload, size_t payload_len,
    struct geisa_proto_manifest_response_view *out);

int geisa_proto_encode_discovery_request(struct geisa_proto_bytes *out);
int geisa_proto_encode_discovery_response(const char *platform_id,
                                          struct geisa_proto_bytes *out);
int geisa_proto_discovery_response_is_success(const unsigned char *payload,
                                              size_t payload_len);

void geisa_proto_bytes_free(struct geisa_proto_bytes *bytes);

#endif
