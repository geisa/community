/*
 * File: src/geisa-protobuf.h
 * Project: geisa-simple
 * Purpose: Declares protobuf helpers and message structures used by the app..
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

#ifndef GEISA_PROTOBUF_H
#define GEISA_PROTOBUF_H

#include <stddef.h>
#include <stdint.h>

enum {
    GEISA_PROTO_STATUS_SUCCESS = 1,
    GEISA_PROTO_MESSAGE_CONFIG = 1,
    GEISA_PROTO_MESSAGE_COMMAND = 2,
    GEISA_PROTO_MESSAGE_EVENT = 5,
    GEISA_PROTO_MESSAGE_STATUS_ACCEPTED = 1,
    GEISA_PROTO_MESSAGE_STATUS_REJECTED = 2,
    GEISA_PROTO_PRIORITY_BEST_EFFORT = 3,
    GEISA_PROTO_APP_STATUS_RUNNING = 0,
    GEISA_PROTO_APP_STATUS_SHUTTING_DOWN = 3
};

#define GEISA_PROTO_MAX_PAYLOAD_SIZE 65536U

/* Owns the heap-allocated encoded protobuf buffer referenced by data. */
struct geisa_proto_bytes {
    unsigned char *data;
    size_t len;
};

/*
 * Holds decoded app-message request fields. payload points to this structure's
 * payload_storage buffer.
 */
struct geisa_proto_app_request_view {
    char request_id[64];
    unsigned priority;
    unsigned message_type;
    char content_type[64];
    /* payload points to this structure's fixed payload_storage buffer. */
    unsigned char payload_storage[GEISA_PROTO_MAX_PAYLOAD_SIZE];
    const unsigned char *payload;
    size_t payload_len;
};

/* Holds decoded request IDs and status text in fixed character arrays. */
struct geisa_proto_app_response_view {
    char request_id[64];
    unsigned status;
    char status_text[96];
};

/* Holds the decoded application lifecycle status type. */
struct geisa_proto_app_status_view {
    unsigned type;
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

struct geisa_proto_conn_app_msg {
    uint64_t yesterday_used;
    uint64_t today_used;
    uint64_t today_limit;
    uint64_t today_remaining;
    uint64_t next_daily_reset_ms;
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

/* Holds decoded platform status and control fields for geisa-simple. */
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

/* Holds Manifest status and copied text in the fixed manifest[] buffer. */
struct geisa_proto_manifest_response_view {
    unsigned status_code;
    char status_text[128];
    char manifest[8192];
    size_t manifest_len;
};

/* Holds Discovery status and the decoded GEISA version fields. */
struct geisa_proto_discovery_response_view {
    unsigned status_code;
    uint32_t geisa_major;
    uint32_t geisa_minor;
    uint32_t geisa_revision;
    int has_geisa_version;
};

void geisa_proto_bytes_free(struct geisa_proto_bytes *bytes);

/* Encode and decode application lifecycle status messages. */
int geisa_proto_encode_app_status(unsigned type, int next_status,
                                  int next_status_timeout,
                                  struct geisa_proto_bytes *out);

int geisa_proto_decode_app_status(const unsigned char *data, size_t len,
                                  struct geisa_proto_app_status_view *out);

/* Encode and decode platform-to-app status and control messages. */
int geisa_proto_encode_platform_to_app_status(
    const struct geisa_proto_platform_to_app_status_values *values,
    struct geisa_proto_bytes *out);

int geisa_proto_decode_platform_to_app_status(
    const unsigned char *data, size_t len,
    struct geisa_proto_platform_to_app_status *out);

/* Encode and decode Deployment Manifest request/response messages. */
int geisa_proto_encode_manifest_request(struct geisa_proto_bytes *out);

int geisa_proto_encode_manifest_response(unsigned status_code,
                                         const char *message,
                                         const char *details,
                                         const char *manifest,
                                         struct geisa_proto_bytes *out);

int geisa_proto_decode_manifest_response(
    const unsigned char *data, size_t len,
    struct geisa_proto_manifest_response_view *out);

/* Encode and decode Platform Discovery request/response messages. */
int geisa_proto_encode_discovery_request(struct geisa_proto_bytes *out);

int geisa_proto_encode_discovery_response(struct geisa_proto_bytes *out);

int geisa_proto_decode_discovery_response(
    const unsigned char *data, size_t len,
    struct geisa_proto_discovery_response_view *out);

/* Encode and decode app-message requests used for CONFIG and EVENT traffic. */
int geisa_proto_encode_app_request(const char *request_id, unsigned priority,
                                   unsigned message_type, uint64_t timestamp_ms,
                                   uint64_t ttl_seconds,
                                   const char *content_type,
                                   const unsigned char *payload,
                                   size_t payload_len,
                                   struct geisa_proto_bytes *out);

int geisa_proto_decode_app_request(const unsigned char *data, size_t len,
                                   struct geisa_proto_app_request_view *out);

/* Encode and decode app-message responses used for request correlation. */
int geisa_proto_encode_app_response(const char *request_id, unsigned status,
                                    const char *status_text,
                                    uint64_t timestamp_ms,
                                    struct geisa_proto_bytes *out);

int geisa_proto_decode_app_response(const unsigned char *data, size_t len,
                                    struct geisa_proto_app_response_view *out);

const char *geisa_proto_app_status_type_name(unsigned type);

#endif
