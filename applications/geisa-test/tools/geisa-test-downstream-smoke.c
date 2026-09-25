/*
 * geisa-test-downstream-smoke.c
 *
 * Exercises downstream CONFIG and COMMAND decoding, validation, and GEISA
 * disposition responses using constructed app-message fixtures.
 *
 * Copyright 2026 PragSol Consulting LLC.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define main geisa_test_program_main
#include "../src/geisa-test.c"
#undef main

static void decode_request(const char *type, const char *json,
                           struct geisa_app_message_request_view *request) {
    unsigned char *wire = NULL;
    size_t wire_len = 0;
    assert(geisa_app_message_build_request(
               "fixture-1", "GEISA_APP_MESSAGE_PRIORITY_IMMEDIATE", type, 1, 30,
               "application/json", (const unsigned char *)json, strlen(json),
               &wire, &wire_len) == 0);
    assert(geisa_app_message_decode_request(wire, wire_len, request) == 0);
    free(wire);
}

int main(void) {
    struct app_state state;
    struct geisa_app_message_request_view request;
    struct app_options before;
    struct config_request effective_request;
    char config_error[256];
    unsigned char *response = NULL;
    size_t response_len = 0;
    char response_id[192];
    char response_status[96];
    char response_detail[256];

    memset(&state, 0, sizeof(state));
    load_default_options(&state.options);
    decode_request("CONFIG",
                   "{\"operation\":\"set_configuration\",\"values\":{\"mode\":"
                   "\"test\",\"test\":\"payload\"}}",
                   &request);
    handle_downstream_config(NULL, &state, &request);
    assert(state.config_apply_count == 1U);
    assert(state.options.application_mode == APP_RUN_MODE_TEST);
    assert(strcmp(state.options.test_name, "payload") == 0);
    assert(state.options.operational_mode == APP_MODE_PAYLOAD_SIZE);

    decode_request("CONFIG",
                   "{\"operation\":\"get_effective_configuration\",\"keys\":["
                   "\"mode\",\"test\"]}",
                   &request);
    assert(parse_config_request_payload((const char *)request.payload,
                                        &effective_request, config_error,
                                        sizeof(config_error)) == 0);
    assert(effective_request.operation == CONFIG_OPERATION_GET_EFFECTIVE);
    handle_downstream_config(NULL, &state, &request);

    decode_request("CONFIG",
                   "{\"operation\":\"set_configuration\",\"values\":{\"mode\":"
                   "\"test\",\"test\":\"api_all\"}}",
                   &request);
    handle_downstream_config(NULL, &state, &request);
    assert(state.config_apply_count == 2U);
    assert(strcmp(state.options.test_name, "api_all") == 0);
    assert(state.options.operational_mode == APP_MODE_API_ALL);
    before = state.options;
    decode_request("CONFIG", "{\"values\":{\"test\":\"not-a-test\"}}",
                   &request);
    handle_downstream_config(NULL, &state, &request);
    assert(state.config_reject_count == 1U);
    assert(state.options.operational_mode == before.operational_mode);
    decode_request("CONFIG", "{\"values\":{\"message_count\":\"bad\"}}",
                   &request);
    handle_downstream_config(NULL, &state, &request);
    assert(state.config_reject_count == 2U);
    decode_request("CONFIG", "{\"values\":{\"unknown_key\":1}}", &request);
    handle_downstream_config(NULL, &state, &request);
    assert(state.config_reject_count == 3U);
    decode_request("COMMAND", "{\"command\":\"run_once\"}", &request);
    state.discovery_completed = 1;
    handle_downstream_command(NULL, &state, &request);
    assert(state.command_accepted_count == 1U &&
           state.command_executed_count == 1U);
    assert(state.options.operational_mode == APP_MODE_API_ALL);
    decode_request("COMMAND", "{\"command\":\"unknown\"}", &request);
    handle_downstream_command(NULL, &state, &request);
    assert(state.command_rejected_count == 1U);
    assert(state.options.operational_mode == APP_MODE_API_ALL);
    assert(geisa_app_message_build_response(
               "fixture-1", "GEISA_APP_MESSAGE_STATUS_ACCEPTED",
               "config_applied", 2, &response, &response_len) == 0);
    assert(geisa_app_message_parse_response(
               response, response_len, response_id, sizeof(response_id),
               response_status, sizeof(response_status), response_detail,
               sizeof(response_detail)) == GEISA_APP_MESSAGE_RESPONSE_ACCEPTED);
    assert(strcmp(response_id, "fixture-1") == 0);
    assert(strcmp(response_detail, "config_applied") == 0);
    free(response);
    puts("geisa_test_downstream_smoke passed");
    return 0;
}
