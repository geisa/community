/*
 * geisa-test-lifecycle-manifest-smoke.c
 *
 * Exercises lifecycle controls, manifest handling, subscription QoS and
 * SUBACK gating, including startup publication order.
 *
 * Copyright 2026 PragSol Consulting LLC.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define mosquitto_publish geisa_test_mock_publish
#define mosquitto_subscribe geisa_test_mock_subscribe
#define main geisa_test_program_main
#include "../src/geisa-test.c"
#undef main
#undef mosquitto_publish
#undef mosquitto_subscribe

static struct app_state *mock_state;
static unsigned mock_manifest_publish_order;
static unsigned mock_running_publish_order;
static unsigned mock_publish_order;
static int mock_status_qos = -1;
static unsigned mock_status_type;
static unsigned mock_subscribe_count;
static char mock_subscribed_topics[STARTUP_SUBSCRIPTION_COUNT][512];
static int mock_subscribed_qos[STARTUP_SUBSCRIPTION_COUNT];
static const unsigned char manifest_success[] = {0x0a, 0x04, 0x08, 0x01, 0x12,
                                                 0x00, 0x12, 0x02, '{',  '}'};

int geisa_test_mock_publish(struct mosquitto *mosq, int *mid, const char *topic,
                            int payloadlen, const void *payload, int qos,
                            bool retain) {
    struct mosquitto_message message;
    (void)mosq;
    (void)mid;
    (void)payloadlen;
    (void)payload;
    (void)qos;
    (void)retain;
    mock_publish_order++;
    if (mock_state != NULL &&
        strcmp(topic, mock_state->manifest_request_topic) == 0) {
        mock_manifest_publish_order = mock_publish_order;
    }
    if (mock_state != NULL &&
        strcmp(topic, mock_state->app_status_topic) == 0) {
        mock_running_publish_order = mock_publish_order;
        mock_status_qos = qos;
        {
            struct geisa_proto_app_status_view status;
            assert(geisa_proto_decode_app_status(payload, (size_t)payloadlen,
                                                 &status) == 0);
            mock_status_type = status.type;
        }
    }
    if (mock_state != NULL &&
        strcmp(topic, mock_state->manifest_request_topic) == 0) {
        memset(&message, 0, sizeof(message));
        message.topic = (char *)mock_state->manifest_response_topic;
        message.payload = (void *)manifest_success;
        message.payloadlen = (int)sizeof(manifest_success);
        on_message(NULL, mock_state, &message);
    }
    return MOSQ_ERR_SUCCESS;
}

int geisa_test_mock_subscribe(struct mosquitto *mosq, int *mid,
                              const char *topic, int qos) {
    (void)mosq;
    assert(mock_subscribe_count < STARTUP_SUBSCRIPTION_COUNT);
    snprintf(mock_subscribed_topics[mock_subscribe_count],
             sizeof(mock_subscribed_topics[0]), "%s", topic);
    mock_subscribed_qos[mock_subscribe_count] = qos;
    *mid = 100 + (int)mock_subscribe_count;
    mock_subscribe_count++;
    return MOSQ_ERR_SUCCESS;
}

int main(void) {
    static const unsigned char manifest_failure[] = {0x0a, 0x05, 0x08, 0x80,
                                                     0x02, 0x12, 0x00};
    unsigned status_code;
    const unsigned char *manifest;
    size_t manifest_len;
    struct geisa_proto_platform_to_app_status control;
    static const unsigned char send_status[] = {0x08, 0x01};
    static const unsigned char shut_down[] = {0x10, 0x01};
    static const unsigned char false_status[] = {0x08, 0x00};
    static const unsigned char malformed_control[] = {0x0a, 0x01, 'x'};
    int granted_qos = 0;
    int rejected_qos = 0x80;
    struct app_state startup_state;
    struct app_state ack_state;
    struct mosquitto_message control_message;
    struct mosquitto_message app_response_message;
    struct app_options options_before_clear;
    struct geisa_proto_bytes invalid_status = {0};
    struct geisa_proto_app_status_view decoded_status;
    unsigned char *app_response_payload = NULL;
    size_t app_response_payload_len = 0U;
    static const unsigned char clear_pii[] = {0x18, 0x01};

    assert(geisa_proto_encode_app_status(1U, 60, 150, &invalid_status) != 0);
    assert(geisa_proto_decode_app_status((const unsigned char *)"\x08\x01", 2U,
                                         &decoded_status) != 0);

    memset(&ack_state, 0, sizeof(ack_state));
    ack_state.startup_subscription_mids[0] = 10;
    ack_state.startup_subscription_mids[1] = 11;
    ack_state.startup_subscription_mids[2] = 12;
    ack_state.startup_subscription_mids[3] = 13;
    ack_state.startup_subscription_mids[4] = 14;
    ack_state.startup_subscription_mids[5] = 15;
    ack_state.startup_subscription_requested_qos[0] = 0;
    ack_state.startup_subscription_requested_qos[1] = 0;
    ack_state.startup_subscription_requested_qos[2] = 1;
    ack_state.startup_subscription_requested_qos[3] = 1;
    ack_state.startup_subscription_requested_qos[4] = 1;
    ack_state.startup_subscription_requested_qos[5] = 1;
    assert(record_startup_subscription_ack(&ack_state, 99, 1, &granted_qos) ==
           0);
    assert(record_startup_subscription_ack(&ack_state, 10, 1, &granted_qos) ==
           0);
    assert(ack_state.startup_subscription_acks == 1U);
    assert(record_startup_subscription_ack(&ack_state, 10, 1, &granted_qos) ==
           0);
    assert(record_startup_subscription_ack(&ack_state, 11, 1, &rejected_qos) ==
           -1);
    assert(record_startup_subscription_ack(&ack_state, 11, 1, &granted_qos) ==
           0);
    granted_qos = 1;
    assert(record_startup_subscription_ack(&ack_state, 12, 1, &granted_qos) ==
           0);
    assert(record_startup_subscription_ack(&ack_state, 13, 1, &granted_qos) ==
           0);
    assert(record_startup_subscription_ack(&ack_state, 14, 1, &granted_qos) ==
           0);
    assert(record_startup_subscription_ack(&ack_state, 15, 1, &granted_qos) ==
           1);

    memset(&startup_state, 0, sizeof(startup_state));
    load_defaults(&startup_state.cfg);
    load_default_options(&startup_state.options);
    startup_state.options.timeout_seconds = 10;
    startup_state.options.reporting_enabled = 0;
    startup_state.options.stay_running = 1;
    mock_state = &startup_state;
    on_connect(NULL, &startup_state, 0);
    assert(mock_subscribe_count == STARTUP_SUBSCRIPTION_COUNT);
    assert(strcmp(mock_subscribed_topics[0], GLOBAL_PLATFORM_STATUS_TOPIC) ==
           0);
    assert(mock_subscribed_qos[0] == 0 && mock_subscribed_qos[1] == 0);
    /* Startup discovery and RUNNING wait until all required SUBACKs arrive. */
    for (unsigned i = 0; i < STARTUP_SUBSCRIPTION_COUNT; i++) {
        int granted = mock_subscribed_qos[i];
        on_subscribe(NULL, &startup_state, 100 + (int)i, 1, &granted);
    }
    assert(startup_state.manifest_successes == 1U);
    assert(startup_state.manifest_unmatched == 0U);
    assert(!startup_state.manifest_pending);
    assert(startup_state.next_send_ms == 0U);
    assert(startup_state.published_count == 0U);
    assert(mock_manifest_publish_order > 0U);
    assert(mock_running_publish_order < mock_manifest_publish_order);
    assert(mock_status_qos == 0);
    assert(startup_state.next_status_ms > now_ms());
    memset(&control_message, 0, sizeof(control_message));
    control_message.topic = GLOBAL_PLATFORM_STATUS_TOPIC;
    control_message.payload = (void *)send_status;
    control_message.payloadlen = (int)sizeof(send_status);
    on_message(NULL, &startup_state, &control_message);
    assert(startup_state.lifecycle_send_status_commands == 1U);
    assert(mock_status_type == GEISA_PROTO_APP_STATUS_RUNNING &&
           mock_status_qos == 0);
    assert(startup_state.lifecycle_running_reports == 2U);
    assert(startup_state.next_send_ms == 0U);
    assert(startup_state.published_count == 0U);
    control_message.topic = startup_state.platform_status_topic;
    control_message.payload = (void *)clear_pii;
    control_message.payloadlen = (int)sizeof(clear_pii);
    /* No PII is retained; CLEAR_PII must leave this transaction intact. */
    snprintf(startup_state.last_request_id,
             sizeof(startup_state.last_request_id), "pending-request");
    startup_state.pending_response_kind = PENDING_KIND_REPORT;
    startup_state.pending_app_response = 1;
    startup_state.pending_deadline_ms = 123U;
    startup_state.response_seen = 0;
    startup_state.send_attempt = 1U;
    startup_state.published_count = 1U;
    startup_state.accepted_count = 4U;
    startup_state.rejected_count = 2U;
    startup_state.config_apply_count = 3U;
    startup_state.config_reject_count = 1U;
    startup_state.options.application_mode = APP_RUN_MODE_TEST;
    startup_state.options.operational_mode = APP_MODE_PAYLOAD_SIZE;
    snprintf(startup_state.options.test_name,
             sizeof(startup_state.options.test_name), "payload");
    startup_state.aggregate_count = 1U;
    startup_state.aggregate_modes[0] = APP_MODE_PAYLOAD_SIZE;
    startup_state.manifest_pending = 1;
    startup_state.manifest_deadline_ms = 456U;
    memcpy(&options_before_clear, &startup_state.options,
           sizeof(options_before_clear));
    on_message(NULL, &startup_state, &control_message);
    assert(mock_status_type == GEISA_PROTO_APP_STATUS_CLEARED_PII);
    assert(strcmp(geisa_proto_app_status_type_name(mock_status_type),
                  "CLEARED_PII") == 0);
    assert(startup_state.lifecycle_state == GEISA_PROTO_APP_STATUS_CLEARED_PII);
    assert(strcmp(startup_state.last_request_id, "pending-request") == 0);
    assert(startup_state.pending_response_kind == PENDING_KIND_REPORT);
    assert(startup_state.pending_app_response);
    assert(startup_state.pending_deadline_ms == 123U);
    assert(!startup_state.response_seen);
    assert(startup_state.send_attempt == 1U &&
           startup_state.published_count == 1U);
    assert(startup_state.accepted_count == 4U &&
           startup_state.rejected_count == 2U);
    assert(startup_state.config_apply_count == 3U &&
           startup_state.config_reject_count == 1U);
    assert(memcmp(&startup_state.options, &options_before_clear,
                  sizeof(options_before_clear)) == 0);
    assert(startup_state.aggregate_count == 1U &&
           startup_state.aggregate_completed == 0U &&
           startup_state.aggregate_modes[0] == APP_MODE_PAYLOAD_SIZE);
    assert(startup_state.manifest_pending &&
           startup_state.manifest_deadline_ms == 456U);

    assert(geisa_app_message_build_response(
               "pending-request", STATUS_ACCEPTED, "accepted", now_ms(),
               &app_response_payload, &app_response_payload_len) == 0);
    memset(&app_response_message, 0, sizeof(app_response_message));
    app_response_message.topic = startup_state.cfg.app_message_response_topic;
    app_response_message.payload = app_response_payload;
    app_response_message.payloadlen = (int)app_response_payload_len;
    on_message(NULL, &startup_state, &app_response_message);
    free(app_response_payload);
    assert(!startup_state.pending_app_response);
    assert(startup_state.pending_response_kind == PENDING_KIND_NONE);
    assert(startup_state.accepted_count == 5U);
    assert(startup_state.aggregate_completed == 1U && startup_state.success);
    assert(startup_state.manifest_pending &&
           startup_state.manifest_deadline_ms == 456U);
    assert(startup_state.config_apply_count == 3U &&
           startup_state.config_reject_count == 1U);
    assert(memcmp(&startup_state.options, &options_before_clear,
                  sizeof(options_before_clear)) == 0);

    control_message.payload = (void *)shut_down;
    control_message.payloadlen = (int)sizeof(shut_down);
    on_message(NULL, &startup_state, &control_message);
    assert(startup_state.shutdown_requested && startup_state.done);
    assert(mock_status_type == 3U &&
           startup_state.lifecycle_shutdown_reports == 1U);
    mock_state = NULL;

    assert(geisa_proto_decode_platform_to_app_status(
               send_status, sizeof(send_status), &control) == 0);
    assert(control.cmd_send_status && !control.cmd_shut_down);
    assert(geisa_proto_decode_platform_to_app_status(
               shut_down, sizeof(shut_down), &control) == 0);
    assert(!control.cmd_send_status && control.cmd_shut_down);
    assert(geisa_proto_decode_platform_to_app_status(
               false_status, sizeof(false_status), &control) == 0);
    assert(!control.cmd_send_status);
    assert(geisa_proto_decode_platform_to_app_status(
               malformed_control, sizeof(malformed_control), &control) != 0);
    assert(decode_manifest_response(manifest_success, sizeof(manifest_success),
                                    &status_code, &manifest,
                                    &manifest_len) == 0);
    assert(status_code == 1U && manifest_len == 2U &&
           memcmp(manifest, "{}", 2U) == 0);
    assert(decode_manifest_response(manifest_failure, sizeof(manifest_failure),
                                    &status_code, &manifest,
                                    &manifest_len) == 0);
    assert(status_code == 256U);
    assert(decode_manifest_response("\x0a\x01\x08", 3U, &status_code, &manifest,
                                    &manifest_len) != 0);
    puts("geisa_test_lifecycle_manifest_smoke passed");
    return 0;
}
