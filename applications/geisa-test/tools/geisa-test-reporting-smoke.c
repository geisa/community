/*
 * geisa-test-reporting-smoke.c
 *
 * Exercises downstream reporting configuration, effective readback, cycle
 * scheduling, and the selected API message profiles.
 *
 * Copyright 2026 PragSol Consulting LLC.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define mosquitto_publish geisa_test_reporting_mock_publish
#define main geisa_test_program_main
#include "../src/geisa-test.c"
#undef main
#undef mosquitto_publish

#define CAPTURED_REQUESTS 8U

static struct app_state *mock_state;
static unsigned captured_count;
static unsigned captured_types[CAPTURED_REQUESTS];
static char captured_payloads[CAPTURED_REQUESTS][256];
static unsigned config_request_number;

int geisa_test_reporting_mock_publish(struct mosquitto *mosq, int *mid,
                                      const char *topic, int payloadlen,
                                      const void *payload, int qos,
                                      bool retain) {
    struct geisa_app_message_request_view request;
    (void)mosq;
    (void)mid;
    (void)qos;
    (void)retain;

    if (mock_state != NULL &&
        strcmp(topic, mock_state->cfg.app_message_request_topic) == 0) {
        assert(captured_count < CAPTURED_REQUESTS);
        assert(payloadlen > 0);
        assert(geisa_app_message_decode_request(payload, (size_t)payloadlen,
                                                &request) == 0);
        captured_types[captured_count] = request.message_type;
        assert(request.payload_len < sizeof(captured_payloads[0]));
        memcpy(captured_payloads[captured_count], request.payload,
               request.payload_len);
        captured_payloads[captured_count][request.payload_len] = '\0';
        captured_count++;
    }
    return MOSQ_ERR_SUCCESS;
}

static void send_config(const char *json) {
    struct geisa_app_message_request_view request;
    unsigned char *wire = NULL;
    size_t wire_len = 0U;
    char request_id[32];

    snprintf(request_id, sizeof(request_id), "config-%u",
             ++config_request_number);
    assert(geisa_app_message_build_request(
               request_id, "GEISA_APP_MESSAGE_PRIORITY_IMMEDIATE", "CONFIG", 1U,
               30U, "application/json", (const unsigned char *)json,
               strlen(json), &wire, &wire_len) == 0);
    assert(geisa_app_message_decode_request(wire, wire_len, &request) == 0);
    handle_downstream_config(NULL, mock_state, &request);
    free(wire);
}

static void accept_pending_report(void) {
    struct mosquitto_message message;
    unsigned char *wire = NULL;
    size_t wire_len = 0U;

    assert(mock_state->pending_app_response);
    assert(geisa_app_message_build_response(mock_state->last_request_id,
                                            STATUS_ACCEPTED, "accepted", 2U,
                                            &wire, &wire_len) == 0);
    memset(&message, 0, sizeof(message));
    message.topic = mock_state->cfg.app_message_response_topic;
    message.payload = wire;
    message.payloadlen = (int)wire_len;
    on_message(NULL, mock_state, &message);
    free(wire);
}

int main(void) {
    struct app_state state;
    struct config_request read_request;
    char error[256];
    char effective[2048];
    uint64_t before_response;
    uint64_t after_response;

    memset(&state, 0, sizeof(state));
    load_defaults(&state.cfg);
    load_default_options(&state.options);
    assert(validate_options(&state.options) == 0);
    state.discovery_completed = 1;
    state.options.stay_running = 1;
    state.options.reporting_enabled = 0;
    mock_state = &state;

    assert(dispatch_next_message(NULL, &state) == MOSQ_ERR_SUCCESS);
    assert(captured_count == 0U && state.published_count == 0U);
    assert(!state.done && state.next_send_ms == 0U);

    send_config("{\"values\":{\"reporting_interval_seconds\":60}}");
    assert(state.options.reporting_interval_seconds == 60);
    send_config("{\"values\":{\"reporting_interval_seconds\":86400}}");
    assert(state.options.reporting_interval_seconds == 86400);
    send_config("{\"values\":{\"reporting_interval_seconds\":59}}");
    assert(state.config_reject_count == 1U);
    assert(state.options.reporting_interval_seconds == 86400);
    send_config("{\"values\":{\"reporting_interval_seconds\":86401}}");
    assert(state.config_reject_count == 2U);
    assert(state.options.reporting_interval_seconds == 86400);

    send_config("{\"values\":{\"reporting_enabled\":true}}");
    assert(state.options.reporting_enabled);
    /* Live settings select the profiles and pace each acknowledged cycle. */
    assert(state.next_send_ms != 0U && state.cycle_remaining == 4U);
    state.next_send_ms = 0U;
    assert(dispatch_next_message(NULL, &state) == MOSQ_ERR_SUCCESS);
    assert(captured_count == 1U);
    assert(captured_types[0] == GEISA_PROTO_MESSAGE_EVENT);

    send_config("{\"values\":{\"reporting_interval_seconds\":120,\"reporting_"
                "message_types\":[\"ALARM\",\"TELEMETRY\"]}}");
    assert(state.options.reporting_interval_seconds == 120);
    assert(state.reporting_profile_count == 2U);

    assert(parse_config_request_payload(
               "{\"operation\":\"get_effective_configuration\",\"keys\":["
               "\"reporting_enabled\",\"reporting_interval_seconds\","
               "\"reporting_message_types\"]}",
               &read_request, error, sizeof(error)) == 0);
    assert(read_request.operation == CONFIG_OPERATION_GET_EFFECTIVE);
    build_effective_config_json(&state.options,
                                read_request.requested_keys_mask, effective,
                                sizeof(effective));
    assert(strstr(effective, "\"reporting_enabled\":true") != NULL);
    assert(strstr(effective, "\"reporting_interval_seconds\":120") != NULL);
    assert(strstr(effective,
                  "\"reporting_message_types\":[\"ALARM\",\"TELEMETRY\"]") !=
           NULL);

    before_response = now_ms();
    accept_pending_report();
    after_response = now_ms();
    assert(state.next_send_ms >= before_response + (uint64_t)120U * 1000ULL);
    assert(state.next_send_ms <= after_response + (uint64_t)120U * 1000ULL);
    assert(state.cycle_remaining == 2U);

    schedule_next_reporting_cycle_at(&state, 5000U);
    assert(state.next_send_ms == 125000U);
    assert(state.cycle_remaining == 2U);
    state.next_send_ms = 0U;
    assert(dispatch_next_message(NULL, &state) == MOSQ_ERR_SUCCESS);
    assert(captured_count == 2U);
    assert(captured_types[1] == GEISA_PROTO_MESSAGE_ALARM);
    assert(strstr(captured_payloads[1], "Test Alarm") != NULL);
    accept_pending_report();
    state.next_send_ms = 0U;
    assert(dispatch_next_message(NULL, &state) == MOSQ_ERR_SUCCESS);
    assert(captured_count == 3U);
    assert(captured_types[2] == GEISA_PROTO_MESSAGE_TELEMETRY);
    assert(strstr(captured_payloads[2], "sample-watts") != NULL);
    assert(state.cycle_remaining == 0U && state.pending_app_response);

    mock_state = NULL;
    puts("geisa_test_reporting_smoke passed");
    return 0;
}
