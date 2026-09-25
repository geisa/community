/*
 * geisa-test-summary-smoke.c
 *
 * Checks the machine-readable result record, including JSON string escaping
 * for values supplied by runtime configuration and responses.
 *
 * Copyright 2026 PragSol Consulting LLC.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>

#define main geisa_test_program_main
#include "../src/geisa-test.c"
#undef main

int main(void) {
    struct app_state state;
    memset(&state, 0, sizeof(state));
    load_default_options(&state.options);
    snprintf(state.options.request_id_prefix,
             sizeof(state.options.request_id_prefix), "run\"\\\n\t\002");
    state.options.message_count = 2;
    state.options.payload_size_bytes = 256;
    state.options.payload_pattern[0] = 'i';
    state.options.payload_pattern[1] = 'n';
    state.options.payload_pattern[2] = 'd';
    state.options.payload_pattern[3] = 'e';
    state.options.payload_pattern[4] = 'x';
    state.options.payload_pattern[5] = '\0';
    state.send_attempt = 2;
    state.published_count = 2;
    state.accepted_count = 1;
    state.rejected_count = 1;
    snprintf(state.response_status_counts[0].status,
             sizeof(state.response_status_counts[0].status), "%s",
             STATUS_ACCEPTED);
    state.response_status_counts[0].count = 1;
    snprintf(state.response_status_counts[1].status,
             sizeof(state.response_status_counts[1].status), "%s",
             STATUS_QUOTA_EXCEEDED);
    state.response_status_counts[1].count = 1;
    state.response_status_count = 2;
    state.success = 1;
    return emit_machine_summary(&state, 1, "message_count") == 0 ? 0 : 1;
}
