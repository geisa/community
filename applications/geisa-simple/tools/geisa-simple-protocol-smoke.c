/*
 * File: tools/geisa-simple-protocol-smoke.c
 * Project: geisa-simple
 * Purpose: Verifies protobuf encoding and decoding for geisa-simple messages
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

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void check_bytes(const struct geisa_proto_bytes *bytes) {
    assert(bytes != NULL);
    assert(bytes->data != NULL);
    assert(bytes->len > 0U);
}

static void check_conn_info(const struct geisa_proto_conn_app_info *info) {
    assert(info->policy == 0U);
    assert(info->today_limit_metered == 0U);
}

int main(void) {
    struct geisa_proto_bytes wire = {0};
    struct geisa_proto_app_request_view request;
    struct geisa_proto_app_response_view response;
    struct geisa_proto_app_status_view app_status;
    struct geisa_proto_platform_to_app_status_values values = {0};
    struct geisa_proto_platform_to_app_status platform_status;
    struct geisa_proto_manifest_response_view manifest;
    struct geisa_proto_discovery_response_view discovery;
    const unsigned char event[] = "{\"kind\":\"hello\"}";
    const unsigned char malformed[] = {0xff};

    /* Application-message EVENT request and response correlation */
    assert(geisa_proto_encode_app_request(
               "simple-1", GEISA_PROTO_PRIORITY_BEST_EFFORT,
               GEISA_PROTO_MESSAGE_EVENT, 10U, 60U, "application/json", event,
               sizeof(event) - 1U, &wire) == 0);
    check_bytes(&wire);
    assert(geisa_proto_decode_app_request(wire.data, wire.len, &request) == 0);
    assert(strcmp(request.request_id, "simple-1") == 0);
    assert(request.priority == GEISA_PROTO_PRIORITY_BEST_EFFORT);
    assert(request.message_type == GEISA_PROTO_MESSAGE_EVENT);
    assert(request.payload_len == sizeof(event) - 1U);
    assert(request.payload == request.payload_storage);
    assert(memcmp(request.payload, event, request.payload_len) == 0);
    geisa_proto_bytes_free(&wire);
    assert(geisa_proto_decode_app_request(malformed, sizeof(malformed),
                                          &request) != 0);

    assert(geisa_proto_encode_app_response("simple-1",
                                           GEISA_PROTO_MESSAGE_STATUS_ACCEPTED,
                                           "accepted", 11U, &wire) == 0);
    assert(geisa_proto_decode_app_response(wire.data, wire.len, &response) ==
           0);
    assert(strcmp(response.request_id, "simple-1") == 0);
    assert(response.status == GEISA_PROTO_MESSAGE_STATUS_ACCEPTED);
    assert(strcmp(response.request_id, "other-id") != 0);
    geisa_proto_bytes_free(&wire);

    /* Application lifecycle status messages */
    assert(geisa_proto_encode_app_status(GEISA_PROTO_APP_STATUS_RUNNING, 0, 0,
                                         &wire) == 0);
    assert(geisa_proto_decode_app_status(wire.data, wire.len, &app_status) ==
           0);
    assert(app_status.type == GEISA_PROTO_APP_STATUS_RUNNING);
    geisa_proto_bytes_free(&wire);
    assert(geisa_proto_encode_app_status(GEISA_PROTO_APP_STATUS_SHUTTING_DOWN,
                                         0, 0, &wire) == 0);
    assert(geisa_proto_decode_app_status(wire.data, wire.len, &app_status) ==
           0);
    assert(app_status.type == GEISA_PROTO_APP_STATUS_SHUTTING_DOWN);
    geisa_proto_bytes_free(&wire);

    /* Platform-to-app status fields, including messaging connection usage */
    values.cmd_send_status = 1;
    values.cpu_usage = 12.5f;
    values.cpu_limit = 25.0f;
    values.memory_usage = 4096U;
    values.memory_limit = 32768U;
    values.persist_storage_usage = 512U;
    values.persist_storage_limit = 8192U;
    values.nonpersist_storage_usage = 256U;
    values.nonpersist_storage_limit = 8192U;
    values.conn_msg.today_used = 1U;
    values.conn_msg.today_limit = 4U;
    values.conn_msg.today_remaining = 3U;
    assert(geisa_proto_encode_platform_to_app_status(&values, &wire) == 0);
    assert(geisa_proto_decode_platform_to_app_status(wire.data, wire.len,
                                                     &platform_status) == 0);
    assert(platform_status.cmd_send_status == 1);
    assert(platform_status.cpu_limit == 25.0f);
    assert(platform_status.memory_usage == 4096U);
    assert(platform_status.persist_storage_limit == 8192U);
    assert(platform_status.conn_msg.today_used == 1U);
    check_conn_info(&platform_status.conn_oper);
    check_conn_info(&platform_status.conn_inet);
    check_conn_info(&platform_status.conn_local);
    geisa_proto_bytes_free(&wire);

    /* Deployment Manifest and Platform Discovery */
    assert(geisa_proto_encode_manifest_request(&wire) == 0);
    geisa_proto_bytes_free(&wire);
    assert(geisa_proto_encode_manifest_response(
               GEISA_PROTO_STATUS_SUCCESS, "success", "",
               "{\"manifest-version\":\"0.9.0\"}", &wire) == 0);
    assert(geisa_proto_decode_manifest_response(wire.data, wire.len,
                                                &manifest) == 0);
    assert(manifest.status_code == GEISA_PROTO_STATUS_SUCCESS);
    assert(strstr(manifest.manifest, "0.9.0") != NULL);
    geisa_proto_bytes_free(&wire);

    assert(geisa_proto_encode_discovery_request(&wire) == 0);
    assert(wire.data != NULL);
    geisa_proto_bytes_free(&wire);
    assert(geisa_proto_encode_discovery_response(&wire) == 0);
    assert(geisa_proto_decode_discovery_response(wire.data, wire.len,
                                                 &discovery) == 0);
    assert(discovery.status_code == GEISA_PROTO_STATUS_SUCCESS);
    assert(discovery.has_geisa_version);
    assert(discovery.geisa_major == 0U && discovery.geisa_minor == 9U);
    geisa_proto_bytes_free(&wire);

    puts("geisa-simple protocol smoke PASS");
    return EXIT_SUCCESS;
}
