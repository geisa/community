/*
 * File: tools/geisa-simple-readiness-smoke.c
 * Project: geisa-simple
 * Purpose: Verifies startup subscription readiness, initial GEISA publication
 *          ordering, and read-only CONFIG scheduling behavior.
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

#include <assert.h>
#include <conn-status.pb.h>
#include <mosquitto.h>
#include <pb_encode.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static const char *published_topics[3];
static int published_qos[3];
static unsigned published_count;
static int capture_startup_publications = 1;

enum {
    READINESS_STATUS_QOS = 0,
    READINESS_REQUEST_QOS = 1,
    READINESS_SUBACK_QOS_COUNT = 1,
    EXPECTED_STARTUP_PUBLICATION_COUNT = 3U,
    UNCHANGED_NEXT_MESSAGE_MS = 123456789U
};

/* Include private startup helpers while capturing their MQTT messages */
static int capture_mosquitto_publish(struct mosquitto *mosq, int *mid,
                                     const char *topic, int payloadlen,
                                     const void *payload, int qos,
                                     bool retain) {
    (void)mosq;
    (void)mid;
    (void)payloadlen;
    (void)payload;
    (void)retain;
    if (capture_startup_publications) {
        assert(qos == (published_count == 0U ? READINESS_STATUS_QOS
                                             : READINESS_REQUEST_QOS));
    } else {
        assert(qos == READINESS_REQUEST_QOS);
    }
    assert(published_count < EXPECTED_STARTUP_PUBLICATION_COUNT);
    published_topics[published_count++] = topic;
    published_qos[published_count - 1U] = qos;
    return MOSQ_ERR_SUCCESS;
}

#define mosquitto_publish capture_mosquitto_publish
#define main geisa_simple_program_main
#include "../src/geisa-app.c"
#undef main
#undef mosquitto_publish

int main(void) {
    struct app_state state;
    struct geisa_proto_bytes wire = {0};
    unsigned char global_payload[GeisaPlatformStatus_size];
    GeisaPlatformStatus global_status = GeisaPlatformStatus_init_zero;
    pb_ostream_t global_stream;
    struct mosquitto_message global_message = {0};
    int granted_qos = READINESS_REQUEST_QOS;
    int denied_qos = MQTT_SUBACK_FAILURE;
    static const char read_config[] =
        "{\"operation\":\"get_effective_configuration\"}";

    memset(&state, 0, sizeof(state));
    geisa_simple_config_defaults(&state.config);
    state.startup_subscription_mids[0] = 10;
    state.startup_subscription_mids[1] = 11;
    state.startup_subscription_mids[2] = 12;
    state.startup_subscription_mids[3] = 13;
    state.startup_subscription_mids[4] = 14;
    state.startup_subscription_mids[5] = 15;
    state.mosq = (struct mosquitto *)1;

    snprintf(state.mqtt.app_status_topic, sizeof(state.mqtt.app_status_topic),
             "%s", "app-status");
    snprintf(state.mqtt.discovery_request_topic,
             sizeof(state.mqtt.discovery_request_topic), "%s",
             "discovery-request");
    snprintf(state.mqtt.manifest_request_topic,
             sizeof(state.mqtt.manifest_request_topic), "%s",
             "manifest-request");

    /* Unmatched and duplicate SUBACKs remain pending. */
    assert(record_startup_subscription_ack(
               &state, 99, READINESS_SUBACK_QOS_COUNT, &granted_qos) ==
           STARTUP_SUBSCRIPTION_PENDING);
    assert(record_startup_subscription_ack(
               &state, 10, READINESS_SUBACK_QOS_COUNT, &granted_qos) ==
           STARTUP_SUBSCRIPTION_PENDING);
    assert(record_startup_subscription_ack(
               &state, 10, READINESS_SUBACK_QOS_COUNT, &granted_qos) ==
           STARTUP_SUBSCRIPTION_PENDING);
    /* A rejected SUBACK reports rejection; later successes reach READY. */
    assert(record_startup_subscription_ack(
               &state, 11, READINESS_SUBACK_QOS_COUNT, &denied_qos) ==
           STARTUP_SUBSCRIPTION_REJECTED);
    assert(record_startup_subscription_ack(
               &state, 11, READINESS_SUBACK_QOS_COUNT, &granted_qos) ==
           STARTUP_SUBSCRIPTION_PENDING);
    assert(record_startup_subscription_ack(
               &state, 12, READINESS_SUBACK_QOS_COUNT, &granted_qos) ==
           STARTUP_SUBSCRIPTION_PENDING);
    assert(record_startup_subscription_ack(
               &state, 13, READINESS_SUBACK_QOS_COUNT, &granted_qos) ==
           STARTUP_SUBSCRIPTION_PENDING);
    assert(record_startup_subscription_ack(
               &state, 14, READINESS_SUBACK_QOS_COUNT, &granted_qos) ==
           STARTUP_SUBSCRIPTION_PENDING);
    assert(record_startup_subscription_ack(
               &state, 15, READINESS_SUBACK_QOS_COUNT, &granted_qos) ==
           STARTUP_SUBSCRIPTIONS_READY);
    assert(record_startup_subscription_ack(
               &state, 13, READINESS_SUBACK_QOS_COUNT, &granted_qos) ==
           STARTUP_SUBSCRIPTION_PENDING);
    assert(state.startup_subscription_acks == 6U);
    assert(publish_startup_transactions(&state) == 0);
    assert(published_count == EXPECTED_STARTUP_PUBLICATION_COUNT);
    assert(strcmp(published_topics[0], "app-status") == 0);
    assert(strcmp(published_topics[1], "discovery-request") == 0);
    assert(strcmp(published_topics[2], "manifest-request") == 0);
    assert(published_qos[0] == READINESS_STATUS_QOS);
    assert(published_qos[1] == READINESS_REQUEST_QOS);
    assert(published_qos[2] == READINESS_REQUEST_QOS);

    global_status.timestamp_ms = 42U;
    global_status.mode = GeisaPlatformMode_PLATFORM_MODE_NORMAL;
    global_status.conn_msg = GeisaConnState_CONN_ENABLED_UP;
    global_status.sys_high_cpu = true;
    global_stream =
        pb_ostream_from_buffer(global_payload, sizeof(global_payload));
    assert(pb_encode(&global_stream, &GeisaPlatformStatus_msg, &global_status));
    global_message.topic = (char *)GLOBAL_PLATFORM_STATUS_TOPIC;
    global_message.payload = global_payload;
    global_message.payloadlen = (int)global_stream.bytes_written;
    on_message(NULL, &state, &global_message);
    capture_startup_publications = 0;

    snprintf(state.mqtt.downstream_response_topic,
             sizeof(state.mqtt.downstream_response_topic), "%s",
             "config-response");

    /* A read-only CONFIG request should not change EVENT timing. */
    state.next_message_ms = UNCHANGED_NEXT_MESSAGE_MS;
    published_count = 0U;
    assert(geisa_proto_encode_app_request(
               "config-read", GEISA_PROTO_PRIORITY_BEST_EFFORT,
               GEISA_PROTO_MESSAGE_CONFIG, 1U, 60U, "application/json",
               (const unsigned char *)read_config, strlen(read_config),
               &wire) == 0);
    handle_config_request(&state, wire.data, wire.len);
    assert(state.next_message_ms == UNCHANGED_NEXT_MESSAGE_MS);
    geisa_proto_bytes_free(&wire);
    puts("geisa-simple readiness smoke PASS");

    return EXIT_SUCCESS;
}
