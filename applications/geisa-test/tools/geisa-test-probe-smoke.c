/*
 * geisa-test-probe-smoke.c
 *
 * Exercises bounded CPU, memory, and storage probes, worker-start failure,
 * cancellation, and removal of probe-owned files and directories.
 *
 * Copyright 2026 PragSol Consulting LLC.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int geisa_test_mock_pthread_create(pthread_t *thread,
                                   const pthread_attr_t *attributes,
                                   void *(*start_routine)(void *),
                                   void *argument);

#define pthread_create geisa_test_mock_pthread_create
#define main geisa_test_program_main
#include "../src/geisa-test.c"
#undef main
#undef pthread_create

static int fail_next_thread_create;

int geisa_test_mock_pthread_create(pthread_t *thread,
                                   const pthread_attr_t *attributes,
                                   void *(*start_routine)(void *),
                                   void *argument) {
    if (fail_next_thread_create) {
        fail_next_thread_create = 0;
        return EAGAIN;
    }
    return pthread_create(thread, attributes, start_routine, argument);
}

static void wait_probe(struct app_state *state) {
    while (state->probe.running) {
        struct timespec pause = {0, 1000000L};
        nanosleep(&pause, NULL);
    }
    pthread_join(state->probe.thread, NULL);
}

int main(void) {
    struct app_state state;
    char root_template[256];
    char persistent[512];
    char transient[512];
    char probe_dir[640];
    struct config_request request;
    char error[256];

    assert(snprintf(root_template, sizeof(root_template),
                    "/tmp/geisa-test-probes-%ld",
                    (long)getpid()) < (int)sizeof(root_template));
    assert(mkdir(root_template, 0700) == 0);
    assert(snprintf(persistent, sizeof(persistent), "%s/persistent",
                    root_template) < (int)sizeof(persistent));
    assert(snprintf(transient, sizeof(transient), "%s/transient",
                    root_template) < (int)sizeof(transient));
    assert(mkdir(persistent, 0700) == 0);
    assert(mkdir(transient, 0700) == 0);

    memset(&state, 0, sizeof(state));
    load_default_options(&state.options);
    assert(test_mode_from_string("cpu") == APP_MODE_LEE_EXCEED_CPU);
    assert(test_mode_from_string("memory") == APP_MODE_LEE_EXCEED_MEM);
    assert(test_mode_from_string("lee_all") == APP_MODE_LEE_ALL);
    assert(test_mode_from_string("api_all") == APP_MODE_API_ALL);
    assert(test_mode_from_string("all") == APP_MODE_ALL);
    assert(test_mode_from_string("unknown") < 0);
    assert(parse_config_request_payload("{\"values\":{\"test_duration_ms\":0}}",
                                        &request, error, sizeof(error)) != 0);
    assert(parse_config_request_payload(
               "{\"values\":{\"test_target_bytes\":67108865}}", &request, error,
               sizeof(error)) != 0);

    state.discovery_completed = 1;
    assert(
        parse_config_request_payload("{\"values\":{\"mode\":\"test\",\"test\":"
                                     "\"cpu\",\"test_duration_ms\":1}}",
                                     &request, error, sizeof(error)) == 0);
    assert(apply_config_update_to_state(&state, &request.update, error,
                                        sizeof(error)) == 0);
    start_pending_test(&state);
    assert(state.probe.running == 1);
    wait_probe(&state);
    assert(state.probe.invocations == 1U);
    assert(state.test_run_pending == 0);

    configure_aggregate(&state, APP_MODE_LEE_ALL);
    assert(state.aggregate_count == 4U);
    assert(state.aggregate_modes[0] == APP_MODE_LEE_EXCEED_CPU);
    assert(state.aggregate_modes[1] == APP_MODE_LEE_EXCEED_MEM);
    assert(state.aggregate_modes[2] == APP_MODE_LEE_EXCEED_PERSISTENT_STORAGE);
    assert(state.aggregate_modes[3] == APP_MODE_LEE_EXCEED_TRANSIENT_STORAGE);
    configure_aggregate(&state, APP_MODE_API_ALL);
    assert(state.aggregate_count == 3U);
    assert(state.aggregate_modes[0] == APP_MODE_PAYLOAD_SIZE);
    assert(state.aggregate_modes[1] == APP_MODE_BURST);
    assert(state.aggregate_modes[2] == APP_MODE_QUOTA_EXCEED);
    configure_aggregate(&state, APP_MODE_ALL);
    assert(state.aggregate_count == 7U &&
           state.aggregate_modes[0] == APP_MODE_PAYLOAD_SIZE &&
           state.aggregate_modes[6] == APP_MODE_QUOTA_EXCEED);
    configure_aggregate(&state, APP_MODE_LEE_ALL);
    state.options.application_mode = APP_RUN_MODE_TEST;
    for (unsigned i = 1U; i <= 4U; i++) {
        finalize_probe_completion(&state);
        assert(state.aggregate_completed == i);
        assert(state.aggregate_failed == 0);
        assert(state.test_run_pending == (i < 4U));
    }
    assert(state.success == 1);
    configure_aggregate(&state, APP_MODE_LEE_ALL);
    complete_current_test(&state, 0);
    complete_current_test(&state, 1);
    complete_current_test(&state, 1);
    complete_current_test(&state, 1);
    assert(state.aggregate_failed == 1 && state.success == 0);

    state.options.operational_mode = APP_MODE_LEE_EXCEED_CPU;
    fail_next_thread_create = 1;
    assert(start_selected_probe(&state) == EAGAIN);
    assert(!state.probe.running && !state.probe.done &&
           strcmp(state.probe.result, "failed") == 0 &&
           strstr(state.probe.error, "pthread_create:") == state.probe.error);

    state.options.duration_ms = 30;
    assert(start_selected_probe(&state) == 0);
    wait_probe(&state);
    assert(strcmp(state.probe.result, "completed") == 0);
    assert(state.probe.cpu_time_ms > 0U);

    state.options.duration_ms = 500;
    assert(start_selected_probe(&state) == 0);
    {
        struct timespec pause = {0, 5000000L};
        nanosleep(&pause, NULL);
    }
    state.probe.cancel = 1;
    wait_probe(&state);
    assert(strcmp(state.probe.result, "interrupted") == 0);

    state.options.operational_mode = APP_MODE_LEE_EXCEED_MEM;
    state.options.target_bytes = 4096;
    state.options.chunk_bytes = 1024;
    state.options.duration_ms = 1;
    assert(start_selected_probe(&state) == 0);
    wait_probe(&state);
    assert(strcmp(state.probe.result, "completed") == 0);
    assert(state.probe.completed_bytes == 4096U &&
           state.probe.touched_bytes == 4096U);
    assert(state.probe.cleaned_up);

    state.options.operational_mode = APP_MODE_LEE_EXCEED_PERSISTENT_STORAGE;
    assert(start_selected_probe(&state) != 0);
    snprintf(state.options.persistent_storage_dir,
             sizeof(state.options.persistent_storage_dir), "%s", persistent);
    assert(start_selected_probe(&state) == 0);
    wait_probe(&state);
    assert(strcmp(state.probe.result, "completed") == 0);
    assert(state.probe.cleaned_up);
    assert(snprintf(probe_dir, sizeof(probe_dir), "%s/geisa-test-probes",
                    persistent) < (int)sizeof(probe_dir));
    errno = 0;
    assert(access(probe_dir, F_OK) != 0 && errno == ENOENT);
    assert(access(persistent, F_OK) == 0);

    state.options.operational_mode = APP_MODE_LEE_EXCEED_TRANSIENT_STORAGE;
    snprintf(state.options.transient_storage_dir,
             sizeof(state.options.transient_storage_dir), "%s", transient);
    assert(start_selected_probe(&state) == 0);
    wait_probe(&state);
    assert(strcmp(state.probe.result, "completed") == 0);
    assert(snprintf(probe_dir, sizeof(probe_dir), "%s/geisa-test-probes",
                    transient) < (int)sizeof(probe_dir));
    errno = 0;
    assert(access(probe_dir, F_OK) != 0 && errno == ENOENT);
    assert(access(transient, F_OK) == 0);
    assert(rmdir(persistent) == 0);
    assert(rmdir(transient) == 0);
    assert(rmdir(root_template) == 0);
    puts("geisa_test_probe_smoke passed");
    return 0;
}
