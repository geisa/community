/*
 * geisa-test-response-accounting-smoke.c
 *
 * Exercises response correlation, duplicate and malformed replies, quota
 * outcomes, and response grouping through the runtime message handler. A late
 * duplicate for an old request must not consume the newer pending response.
 *
 * Copyright 2026 PragSol Consulting LLC.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define main geisa_test_program_main
#include "../src/geisa-test.c"
#undef main

static void prepare(struct app_state *state, const char *expected) {
    memset(state, 0, sizeof(*state));
    snprintf(state->cfg.app_message_response_topic,
             sizeof(state->cfg.app_message_response_topic),
             "geisa/api/message/upstream/rsp/app");
    load_default_options(&state->options);
    snprintf(state->options.expected_status,
             sizeof(state->options.expected_status), "%s", expected);
    snprintf(state->last_request_id, sizeof(state->last_request_id), "req-1");
    state->pending_app_response = 1;
    state->pending_response_kind = PENDING_KIND_REPORT;
}

static void deliver(struct app_state *state, const unsigned char *payload,
                    size_t payload_len) {
    struct mosquitto_message message;
    memset(&message, 0, sizeof(message));
    message.topic = state->cfg.app_message_response_topic;
    message.payload = (void *)payload;
    message.payloadlen = (int)payload_len;
    on_message(NULL, state, &message);
}

static void deliver_platform_status(struct app_state *state,
                                    const unsigned char *payload,
                                    size_t payload_len) {
    struct mosquitto_message message;
    memset(&message, 0, sizeof(message));
    message.topic = state->platform_status_topic;
    message.payload = (void *)payload;
    message.payloadlen = (int)payload_len;
    on_message(NULL, state, &message);
}

int main(void) {
    struct app_state state;
    struct geisa_proto_platform_to_app_status_values platform_status;
    struct geisa_proto_bytes platform_status_payload;

    prepare(&state, STATUS_ACCEPTED);
    deliver(&state, (const unsigned char *)"\x0a\x05req-1\x10\x01", 9U);
    assert(state.accepted_count == 1 && state.published_count == 0);
    deliver(&state, (const unsigned char *)"\x0a\x05req-1\x10\x01", 9U);
    assert(state.duplicate_response_count == 1);

    prepare(&state, STATUS_ACCEPTED);
    deliver(&state, (const unsigned char *)"\x0a\x05req-1\x10\x01", 9U);
    snprintf(state.last_request_id, sizeof(state.last_request_id), "req-2");
    state.pending_app_response = 1;
    state.pending_response_kind = PENDING_KIND_REPORT;
    state.response_seen = 0;
    deliver(&state, (const unsigned char *)"\x0a\x05req-1\x10\x01", 9U);
    assert(state.unmatched_response_count == 1U && !state.response_seen);
    deliver(&state, (const unsigned char *)"\x0a\x05req-2\x10\x01", 9U);
    assert(state.accepted_count == 2U && state.response_seen);

    prepare(&state, STATUS_QUOTA_EXCEEDED);
    deliver(&state, (const unsigned char *)"\x0a\x05req-1\x10\x04", 9U);
    assert(state.rejected_count == 1 && state.status_mismatch_count == 0);

    prepare(&state, STATUS_ACCEPTED);
    state.options.operational_mode = APP_MODE_NORMAL;
    state.discovery_completed = 1;
    snprintf(state.platform_status_topic, sizeof(state.platform_status_topic),
             "geisa/api/status/platform/app");
    deliver(&state, (const unsigned char *)"\x0a\x05req-1\x10\x04", 9U);
    assert(state.rejected_count == 1 && state.quota_rejected &&
           state.platform_message_quota_exhausted && !state.done);
    memset(&platform_status, 0, sizeof(platform_status));
    platform_status.conn_msg.today_used = 1U;
    platform_status.conn_msg.today_limit = 64U;
    platform_status.conn_msg.today_remaining = 63U;
    assert(geisa_proto_encode_platform_to_app_status(
               &platform_status, &platform_status_payload) == 0);
    deliver_platform_status(&state, platform_status_payload.data,
                            platform_status_payload.len);
    geisa_proto_bytes_free(&platform_status_payload);
    assert(state.platform_message_quota_known &&
           !state.platform_message_quota_exhausted && state.next_send_ms > 0U);

    prepare(&state, STATUS_QUOTA_EXCEEDED);
    state.options.operational_mode = APP_MODE_QUOTA_EXCEED;
    deliver(&state, (const unsigned char *)"\x0a\x05req-1\x10\x04", 9U);
    assert(state.done && state.success);

    prepare(&state, STATUS_ACCEPTED);
    state.options.application_mode = APP_RUN_MODE_TEST;
    state.options.operational_mode = APP_MODE_QUOTA_EXCEED;
    state.options.message_count = 1;
    state.options.quota_interval_ms = 1;
    configure_aggregate(&state, APP_MODE_API_ALL);
    state.aggregate_index = 2U;
    state.aggregate_count = 3U;
    deliver(&state, (const unsigned char *)"\x0a\x05req-1\x10\x01", 9U);
    assert(state.accepted_count == 1U && !state.done &&
           state.aggregate_completed == 0U && state.next_send_ms > 0U);
    state.send_attempt = 1U;
    state.options.max_attempts = 1;
    assert(dispatch_next_message(NULL, &state) == MOSQ_ERR_SUCCESS);
    assert(!state.success && state.aggregate_failed == 1 &&
           !state.test_run_pending && state.aggregate_count == 0U);

    prepare(&state, STATUS_ACCEPTED);
    state.options.application_mode = APP_RUN_MODE_TEST;
    state.options.operational_mode = APP_MODE_QUOTA_EXCEED;
    state.options.message_count = 1;
    configure_aggregate(&state, APP_MODE_API_ALL);
    state.aggregate_index = 2U;
    state.aggregate_count = 3U;
    deliver(&state, (const unsigned char *)"\x0a\x05req-1\x10\x04", 9U);
    assert(state.success && state.aggregate_completed == 1U &&
           state.aggregate_count == 0U);

    prepare(&state, STATUS_ACCEPTED);
    state.options.application_mode = APP_RUN_MODE_TEST;
    configure_aggregate(&state, APP_MODE_API_ALL);
    state.aggregate_index = 2U;
    state.aggregate_count = 3U;
    state.options.operational_mode = APP_MODE_QUOTA_EXCEED;
    deliver(&state, (const unsigned char *)"\x0a\x05req-1\x10\x04", 9U);
    assert(state.status_mismatch_count == 0U);
    assert(state.aggregate_completed == 1U && state.success == 1);

    prepare(&state, STATUS_ACCEPTED);
    deliver(&state, (const unsigned char *)"\x0a\x05req-1\x10\x04", 9U);
    assert(state.rejected_count == 1 && state.status_mismatch_count == 1 &&
           !state.success);

    prepare(&state, STATUS_QUOTA_EXCEEDED);
    deliver(&state, (const unsigned char *)"\x0a\x05req-1\x10\x01", 9U);
    assert(state.accepted_count == 1 && state.status_mismatch_count == 1);

    prepare(&state, STATUS_ACCEPTED);
    deliver(&state, (const unsigned char *)"\x0a\x05other\x10\x01", 9U);
    assert(state.unmatched_response_count == 1);

    prepare(&state, STATUS_ACCEPTED);
    deliver(&state, (const unsigned char *)"{}", 2U);
    assert(state.malformed_response_count == 1);

    prepare(&state, STATUS_ACCEPTED);
    state.pending_deadline_ms = 10;
    assert(expire_pending_response(&state, 11) != 0);
    assert(state.timeout_count == 1 && !state.success);
    puts("geisa_test_response_accounting_smoke passed");
    return 0;
}
