/*
 * geisa-test-message-contract.h
 *
 * Declares the app-message topic, payload, envelope, and response helpers used
 * by geisa-test and its focused smoke tests.
 *
 * Copyright 2026 PragSol Consulting LLC.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GEISA_TEST_MESSAGE_CONTRACT_H
#define GEISA_TEST_MESSAGE_CONTRACT_H

#include <stddef.h>
#include <stdint.h>

#define GEISA_APP_MESSAGE_REQUEST_TOPIC_PREFIX "geisa/api/message/upstream/req/"
#define GEISA_APP_MESSAGE_RESPONSE_TOPIC_PREFIX                                \
    "geisa/api/message/upstream/rsp/"
#define GEISA_APP_MESSAGE_DOWNSTREAM_REQUEST_TOPIC_PREFIX                      \
    "geisa/api/message/downstream/req/"
#define GEISA_APP_MESSAGE_DOWNSTREAM_RESPONSE_TOPIC_PREFIX                     \
    "geisa/api/message/downstream/rsp/"
#define GEISA_APP_MESSAGE_STATUS_ACCEPTED "GEISA_APP_MESSAGE_STATUS_ACCEPTED"
#define GEISA_APP_MESSAGE_STATUS_QUOTA_EXCEEDED                                \
    "GEISA_APP_MESSAGE_STATUS_QUOTA_EXCEEDED"

enum geisa_app_message_response_class {
    GEISA_APP_MESSAGE_RESPONSE_MALFORMED = 0,
    GEISA_APP_MESSAGE_RESPONSE_ACCEPTED,
    GEISA_APP_MESSAGE_RESPONSE_REJECTED,
};

struct geisa_app_message_request_view {
    char request_id[192];
    unsigned message_type;
    char content_type[128];
    unsigned char payload_storage[65536];
    const unsigned char *payload;
    size_t payload_len;
};

/* Successful builders allocate output buffers for the caller to free. Request
 * decoding copies payload bytes into the fixed-size view rather than retaining
 * the MQTT buffer. Builders and decoder return 0 on success and -1 on failure;
 * response parsing returns one of the classes below. */
int geisa_app_message_build_topic(char *out, size_t out_size,
                                  const char *prefix, const char *identity);
int geisa_app_message_build_payload(size_t size, const char *pattern,
                                    const char *content_type,
                                    unsigned char **out_data, size_t *out_len);
int geisa_app_message_build_request_id(char *out, size_t out_size,
                                       const char *run_id, uint64_t sequence);
int geisa_app_message_build_request(
    const char *request_id, const char *priority, const char *message_type,
    uint64_t timestamp_ms, uint32_t ttl_seconds, const char *content_type,
    const unsigned char *payload, size_t payload_len,
    unsigned char **out_payload, size_t *out_payload_len);
int geisa_app_message_decode_request(
    const unsigned char *payload, size_t payload_len,
    struct geisa_app_message_request_view *out);
int geisa_app_message_build_response(const char *request_id, const char *status,
                                     const char *status_text,
                                     uint64_t timestamp_ms,
                                     unsigned char **out_payload,
                                     size_t *out_payload_len);
enum geisa_app_message_response_class geisa_app_message_parse_response(
    const unsigned char *payload, size_t payload_len, char *request_id,
    size_t request_id_size, char *status, size_t status_size, char *status_text,
    size_t status_text_size);
int geisa_platform_discovery_response_is_success(const unsigned char *payload,
                                                 size_t payload_len);

#endif
