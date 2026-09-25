/*
 * geisa-test-config-smoke.c
 *
 * Exercises CONFIG parsing, candidate validation, defaults, and effective
 * value readback, including which payload settings affect each test group.
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

static int parse_update(const char *json, struct config_request *request) {
    char error[256];
    return parse_config_request_payload(json, request, error, sizeof(error));
}

static void apply_json_update(struct app_options *options, const char *json) {
    struct config_request request;
    assert(parse_update(json, &request) == 0);
    assert(request.operation == CONFIG_OPERATION_SET);
    apply_config_update(options, &request.update);
}

int main(void) {
    struct app_options options;
    struct app_state state;
    struct config_request request;
    char readback[2048];
    size_t profile_len;
    unsigned char *payload = NULL;
    size_t payload_len = 0;
    size_t resolved_size = 0;
    const struct app_message_profile *profiles;
    size_t profile_count = 0;

    load_default_options(&options);
    assert(options.message_count == 0);
    assert(options.payload_size_bytes == 256);
    assert(strcmp(options.payload_pattern, "alpha") == 0);
    assert(strcmp(options.expected_status, STATUS_ACCEPTED) == 0);

    apply_json_update(
        &options,
        "{\"values\":{\"message_count\":7,\"payload_pattern\":\"index\","
        "\"expected_status\":\"GEISA_APP_MESSAGE_STATUS_QUOTA_EXCEEDED\"}}");
    assert(options.message_count == 7);
    assert(strcmp(options.payload_pattern, "index") == 0);
    assert(strcmp(options.expected_status, STATUS_QUOTA_EXCEEDED) == 0);
    build_effective_config_json(&options,
                                CONFIG_UPDATE_MESSAGE_COUNT |
                                    CONFIG_UPDATE_PAYLOAD_PATTERN |
                                    CONFIG_UPDATE_EXPECTED_STATUS,
                                readback, sizeof(readback));
    assert(strstr(readback, "\"message_count\":7") != NULL);
    assert(strstr(readback, "\"payload_pattern\":\"index\"") != NULL);
    assert(strstr(readback, "GEISA_APP_MESSAGE_STATUS_QUOTA_EXCEEDED") != NULL);

    assert(parse_update("{\"values\":{\"message_count\":1000001}}", &request) !=
           0);
    assert(options.message_count == 7);
    assert(parse_update("{\"values\":{\"message_count\":-1}}", &request) != 0);
    assert(parse_update("{\"values\":{\"message_count\":\"bad\"}}", &request) !=
           0);
    apply_json_update(&options, "{\"values\":{\"message_count\":0}}");
    assert(options.message_count == 0);
    apply_json_update(&options, "{\"values\":{\"message_count\":1000000}}");
    assert(options.message_count == 1000000);
    assert(parse_update("{\"values\":{\"payload_pattern\":\"bad\"}}",
                        &request) != 0);
    apply_json_update(&options, "{\"values\":{\"payload_pattern\":\"alpha\"}}");
    apply_json_update(&options, "{\"values\":{\"payload_pattern\":\"zero\"}}");
    apply_json_update(&options, "{\"values\":{\"payload_pattern\":\"index\"}}");
    assert(parse_update("{\"values\":{\"expected_status\":\"\"}}", &request) !=
           0);
    apply_json_update(&options, "{\"values\":{\"expected_status\":\"GEISA_APP_"
                                "MESSAGE_STATUS_UNAVAILABLE\"}}");
    assert(strcmp(options.expected_status,
                  "GEISA_APP_MESSAGE_STATUS_UNAVAILABLE") == 0);
    apply_json_update(&options, "{\"values\":{\"expected_status\":\"GEISA_APP_"
                                "MESSAGE_STATUS_ACCEPTED\"}}");

    options.payload_size_bytes = 0;
    profiles = default_profiles(&profile_count);
    assert(profile_count > 0);
    profile_len = strlen(profiles[0].payload_json);
    assert(build_payload_bytes(&options, &profiles[0], &payload, &payload_len,
                               &resolved_size) == 0);
    assert(payload_len == profile_len && resolved_size == profile_len);
    free(payload);

    options.payload_size_bytes = 256;
    options.operational_mode = APP_MODE_NORMAL;
    assert(build_payload_bytes(&options, &profiles[0], &payload, &payload_len,
                               &resolved_size) == 0);
    assert(payload_len == profile_len &&
           memcmp(payload, profiles[0].payload_json, profile_len) == 0);
    free(payload);
    options.payload_size_bytes = 32;
    options.operational_mode = APP_MODE_PAYLOAD_SIZE;
    assert(build_payload_bytes(&options, &profiles[0], &payload, &payload_len,
                               &resolved_size) == 0);
    assert(payload_len == 32U && resolved_size == 32U && payload[0] == '{' &&
           payload[payload_len - 1U] == '}');
    free(payload);
    options.operational_mode = APP_MODE_BURST;
    assert(build_payload_bytes(&options, &profiles[0], &payload, &payload_len,
                               &resolved_size) == 0);
    assert(payload_len == profile_len &&
           memcmp(payload, profiles[0].payload_json, profile_len) == 0);
    free(payload);

    memset(&state, 0, sizeof(state));
    load_default_options(&state.options);
    state.options.operational_mode = APP_MODE_PAYLOAD_SIZE;
    begin_reporting_cycle(&state);
    assert(state.cycle_remaining == 1U);
    state.options.operational_mode = APP_MODE_BURST;
    state.options.burst_count = 3;
    begin_reporting_cycle(&state);
    assert(state.cycle_remaining == 3U);
    puts("geisa_test_config_smoke passed");
    return 0;
}
