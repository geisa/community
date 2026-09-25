/*
 * geisa-test-payload-ownership-smoke.c
 *
 * Covers request-encoding failure after payload generation. The caller still
 * owns that buffer and must free it exactly once.
 *
 * Copyright 2026 PragSol Consulting LLC.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int fail_request_build(const char *request_id, const char *priority,
                       const char *message_type, uint64_t timestamp_ms,
                       uint32_t ttl_seconds, const char *content_type,
                       const unsigned char *payload, size_t payload_len,
                       unsigned char **out_payload, size_t *out_payload_len);

#define geisa_app_message_build_request fail_request_build
#define main geisa_test_program_main
#include "../src/geisa-test.c"
#undef main
#undef geisa_app_message_build_request

int fail_request_build(const char *request_id, const char *priority,
                       const char *message_type, uint64_t timestamp_ms,
                       uint32_t ttl_seconds, const char *content_type,
                       const unsigned char *payload, size_t payload_len,
                       unsigned char **out_payload, size_t *out_payload_len) {
    (void)request_id;
    (void)priority;
    (void)message_type;
    (void)timestamp_ms;
    (void)ttl_seconds;
    (void)content_type;
    assert(payload != NULL && payload_len > 0U);
    assert(out_payload != NULL && out_payload_len != NULL);
    return -1;
}

int main(void) {
    struct app_state state;
    size_t profile_count = 0U;
    const struct app_message_profile *profiles =
        default_profiles(&profile_count);

    assert(profile_count > 0U);
    memset(&state, 0, sizeof(state));
    load_default_options(&state.options);
    assert(publish_one_message(NULL, &state, &profiles[0]) == MOSQ_ERR_NOMEM);
    assert(state.send_attempt == 1U);
    puts("geisa_test_payload_ownership_smoke passed");
    return 0;
}
