/*
 * geisa-test-message-contract-smoke.c
 *
 * Validates app-message topics, payload patterns, request envelopes, and
 * response parsing independently of the MQTT runtime.
 *
 * Copyright 2026 PragSol Consulting LLC.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/geisa-test-message-contract.h"

int main(void) {
    unsigned char *payload = NULL;
    size_t payload_len = 0;
    char topic[256];
    char request_id[128];
    unsigned char *request = NULL;
    size_t request_len = 0U;
    char parsed_id[128];
    char status[128];
    char status_text[128];
    unsigned char *response = NULL;
    size_t response_len = 0U;

    assert(geisa_app_message_build_topic(topic, sizeof(topic),
                                         GEISA_APP_MESSAGE_REQUEST_TOPIC_PREFIX,
                                         "com.example.app") == 0);
    assert(strcmp(topic, "geisa/api/message/upstream/req/com.example.app") ==
           0);
    assert(geisa_app_message_build_payload(32, "index", "application/json",
                                           &payload, &payload_len) == 0);
    assert(payload_len == 32 && payload[0] == '{' && payload[31] == '}');
    assert(geisa_app_message_build_request_id(request_id, sizeof(request_id),
                                              "run-abc", 7) == 0);
    assert(strcmp(request_id, "run-abc-7") == 0);
    assert(geisa_app_message_build_request(
               request_id, "GEISA_APP_MESSAGE_PRIORITY_BEST_EFFORT", "APP_DATA",
               1, 300, "application/json", payload, payload_len, &request,
               &request_len) == 0);
    assert(request_len > 0U);
    assert(geisa_app_message_parse_response(
               (const unsigned char *)"\x0a\x09run-abc-7\x10\x01", 13U,
               parsed_id, sizeof(parsed_id), status, sizeof(status),
               status_text,
               sizeof(status_text)) == GEISA_APP_MESSAGE_RESPONSE_ACCEPTED);
    assert(strcmp(parsed_id, request_id) == 0);
    assert(geisa_app_message_parse_response(
               (const unsigned char *)"{}", 2, parsed_id, sizeof(parsed_id),
               status, sizeof(status), status_text,
               sizeof(status_text)) == GEISA_APP_MESSAGE_RESPONSE_MALFORMED);
    assert(geisa_app_message_build_response(
               "run-abc-7", GEISA_APP_MESSAGE_STATUS_QUOTA_EXCEEDED,
               "daily quota reached", 10U, &response, &response_len) == 0);
    assert(geisa_app_message_parse_response(
               response, response_len, parsed_id, sizeof(parsed_id), status,
               sizeof(status), status_text,
               sizeof(status_text)) == GEISA_APP_MESSAGE_RESPONSE_REJECTED);
    assert(strcmp(status_text, "daily quota reached") == 0);
    free(response);
    free(payload);
    free(request);
    puts("geisa_test_message_contract_smoke passed");
    return 0;
}
