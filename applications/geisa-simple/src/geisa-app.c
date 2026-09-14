/*
 * File: src/geisa-app.c
 * Project: geisa-simple
 * Purpose: Implements geisa-simple's application lifecycle, MQTT integration,
 *          GEISA API exchanges, configuration handling, and shutdown behavior.
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

#include <mosquitto.h>

#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "geisa-protobuf.h"
#include "geisa-simple-config.h"

#define CONFIG_PATH "/etc/geisa/mqtt.conf"
#define STATUS_INTERVAL_SECONDS 10
#define STATUS_TIMEOUT_SECONDS 25
#define REQUEST_TIMEOUT_SECONDS 10
#define EVENT_TTL_SECONDS 300
#define MQTT_QOS_AT_MOST_ONCE 0
#define MQTT_QOS_AT_LEAST_ONCE 1
#define MQTT_SUBACK_FAILURE 0x80
#define MQTT_KEEPALIVE_SECONDS 60
#define STARTUP_SUBSCRIPTION_COUNT 6U

enum {
    GEISA_SIMPLE_EXIT_SETUP_FAILURE = 2,
    GEISA_SIMPLE_EXIT_CONNECTION_FAILURE = 3
};

enum startup_subscription_result {
    STARTUP_SUBSCRIPTION_REJECTED = -1,
    STARTUP_SUBSCRIPTION_PENDING = 0,
    STARTUP_SUBSCRIPTIONS_READY = 1
};

#define PLATFORM_STATUS_TOPIC_PREFIX "geisa/api/platform/app/status/"
#define GLOBAL_PLATFORM_STATUS_TOPIC "geisa/api/platform/status"
#define APP_STATUS_TOPIC_PREFIX "geisa/api/app/platform/status/"
#define MANIFEST_REQUEST_TOPIC_PREFIX "geisa/api/app/manifest/req/"
#define MANIFEST_RESPONSE_TOPIC_PREFIX "geisa/api/app/manifest/rsp/"
#define DISCOVERY_REQUEST_TOPIC_PREFIX "geisa/api/platform/discovery/req/"
#define DISCOVERY_RESPONSE_TOPIC_PREFIX "geisa/api/platform/discovery/rsp/"
#define UPSTREAM_REQUEST_TOPIC_PREFIX "geisa/api/message/upstream/req/"
#define UPSTREAM_RESPONSE_TOPIC_PREFIX "geisa/api/message/upstream/rsp/"
#define DOWNSTREAM_REQUEST_TOPIC_PREFIX "geisa/api/message/downstream/req/"
#define DOWNSTREAM_RESPONSE_TOPIC_PREFIX "geisa/api/message/downstream/rsp/"

struct mqtt_config {
    char host[256];
    int port;
    char userid[256];
    char password[256];
    int has_password;
    char client_id[263];
    char platform_status_topic[512];
    char app_status_topic[512];
    char manifest_request_topic[512];
    char manifest_response_topic[512];
    char discovery_request_topic[512];
    char discovery_response_topic[512];
    char upstream_request_topic[512];
    char upstream_response_topic[512];
    char downstream_request_topic[512];
    char downstream_response_topic[512];
};

struct app_state {
    struct mqtt_config mqtt;
    struct geisa_simple_config config;
    struct mosquitto *mosq;
    int connected;
    int done;
    int shutdown_requested;
    struct {
        int failed;
        int discovery_succeeded;
        int manifest_succeeded;
    } outcome;
    uint64_t next_message_ms;
    uint64_t next_status_ms;
    uint64_t pending_deadline_ms;
    uint64_t manifest_deadline_ms;
    uint64_t discovery_deadline_ms;
    unsigned message_sequence;
    unsigned unmatched_responses;
    char pending_request_id[192];
    int manifest_pending;
    int discovery_pending;
    int startup_status_published;
    unsigned startup_subscription_acks;
    int startup_subscription_mids[STARTUP_SUBSCRIPTION_COUNT];
    int startup_subscription_acked[STARTUP_SUBSCRIPTION_COUNT];
};

static volatile sig_atomic_t g_stop = 0;

static uint64_t epoch_ms(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) return 0;
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)(ts.tv_nsec / 1000000ULL);
}

static uint64_t monotonic_ms(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)(ts.tv_nsec / 1000000ULL);
}

static void on_signal(int signal_number) {
    (void)signal_number;
    g_stop = 1;
}

static void trim(char *value) {
    size_t length = strlen(value);
    size_t start = 0;
    while (length > 0 &&
           (value[length - 1] == ' ' || value[length - 1] == '\t' ||
            value[length - 1] == '\r' || value[length - 1] == '\n')) {
        value[--length] = '\0';
    }
    while (value[start] == ' ' || value[start] == '\t')
        start++;
    if (start > 0) memmove(value, value + start, strlen(value + start) + 1);
}

static void derive_topic(char *out, size_t out_size, const char *prefix,
                         const char *userid) {
    snprintf(out, out_size, "%s%s", prefix, userid);
}

/* Derives GEISA topic names from the app user ID assigned by the LEE */
static void derive_topics(struct mqtt_config *config) {
    snprintf(config->client_id, sizeof(config->client_id), "%s.simple",
             config->userid);
    derive_topic(config->platform_status_topic,
                 sizeof(config->platform_status_topic),
                 PLATFORM_STATUS_TOPIC_PREFIX, config->userid);
    derive_topic(config->app_status_topic, sizeof(config->app_status_topic),
                 APP_STATUS_TOPIC_PREFIX, config->userid);
    derive_topic(config->manifest_request_topic,
                 sizeof(config->manifest_request_topic),
                 MANIFEST_REQUEST_TOPIC_PREFIX, config->userid);
    derive_topic(config->manifest_response_topic,
                 sizeof(config->manifest_response_topic),
                 MANIFEST_RESPONSE_TOPIC_PREFIX, config->userid);
    derive_topic(config->discovery_request_topic,
                 sizeof(config->discovery_request_topic),
                 DISCOVERY_REQUEST_TOPIC_PREFIX, config->userid);
    derive_topic(config->discovery_response_topic,
                 sizeof(config->discovery_response_topic),
                 DISCOVERY_RESPONSE_TOPIC_PREFIX, config->userid);
    derive_topic(config->upstream_request_topic,
                 sizeof(config->upstream_request_topic),
                 UPSTREAM_REQUEST_TOPIC_PREFIX, config->userid);
    derive_topic(config->upstream_response_topic,
                 sizeof(config->upstream_response_topic),
                 UPSTREAM_RESPONSE_TOPIC_PREFIX, config->userid);
    derive_topic(config->downstream_request_topic,
                 sizeof(config->downstream_request_topic),
                 DOWNSTREAM_REQUEST_TOPIC_PREFIX, config->userid);
    derive_topic(config->downstream_response_topic,
                 sizeof(config->downstream_response_topic),
                 DOWNSTREAM_RESPONSE_TOPIC_PREFIX, config->userid);
}

/*
 * Reads the LEE-provided MQTT settings, validate required fields, and derives
 * geisa-simple's topics.
 */
static int load_mqtt_config(struct mqtt_config *config) {
    FILE *file;
    char line[1024];
    int has_host = 0;
    int has_userid = 0;
    int has_password = 0;
    const char *config_path = getenv("GEISA_MQTT_CONFIG");

    memset(config, 0, sizeof(*config));
    if (!config_path || config_path[0] == '\0') config_path = CONFIG_PATH;
    file = fopen(config_path, "r");
    if (!file) {
        fprintf(stderr,
                "Required GEISA MQTT configuration is unavailable: %s\n",
                config_path);
        return -1;
    }
    {
        while (fgets(line, sizeof(line), file)) {
            char *equals = strchr(line, '=');
            if (!equals) continue;
            *equals = '\0';
            trim(line);
            trim(equals + 1);
            if (strcmp(line, "HOST") == 0 && equals[1] != '\0') {
                snprintf(config->host, sizeof(config->host), "%s", equals + 1);
                has_host = 1;
            } else if (strcmp(line, "PORT") == 0) {
                int port = atoi(equals + 1);
                if (port > 0 && port <= 65535) config->port = port;
            } else if (strcmp(line, "USERID") == 0 && equals[1] != '\0') {
                snprintf(config->userid, sizeof(config->userid), "%s",
                         equals + 1);
                has_userid = 1;
            } else if (strcmp(line, "PASSWORD") == 0) {
                snprintf(config->password, sizeof(config->password), "%s",
                         equals + 1);
                config->has_password = 1;
                has_password = equals[1] != '\0';
            }
        }
        fclose(file);
    }
    if (!has_host || config->port <= 0 || !has_userid || !has_password) {
        fprintf(stderr, "invalid GEISA MQTT configuration: %s\n", config_path);
        return -1;
    }
    derive_topics(config);
    return 0;
}

static int publish_wire(struct app_state *state, const char *topic,
                        const struct geisa_proto_bytes *wire, int qos) {
    int rc = mosquitto_publish(state->mosq, NULL, topic, (int)wire->len,
                               wire->data, qos, false);
    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "publish failed topic=%s error=%s\n", topic,
                mosquitto_strerror(rc));
    }
    return rc;
}

/*
 * Publishes a lifecycle/status message such as RUNNING or SHUTTING_DOWN with
 * the next-status interval advertised to the platform.
 */
static int publish_app_status(struct app_state *state, unsigned type) {
    struct geisa_proto_bytes wire = {0};
    int rc = geisa_proto_encode_app_status(type, STATUS_INTERVAL_SECONDS,
                                           STATUS_TIMEOUT_SECONDS, &wire);
    if (rc == 0)
        rc = publish_wire(state, state->mqtt.app_status_topic, &wire,
                          MQTT_QOS_AT_MOST_ONCE);
    if (rc == 0) {
        printf("published app status type=%s topic=%s qos=0\n",
               geisa_proto_app_status_type_name(type),
               state->mqtt.app_status_topic);
    }
    geisa_proto_bytes_free(&wire);
    return rc;
}

/*
 * Request geisa-simple's current Deployment Manifest and track its pending
 * response until/if the request timeout expires.
 */
static int request_manifest(struct app_state *state) {
    struct geisa_proto_bytes wire = {0};
    int rc = geisa_proto_encode_manifest_request(&wire);
    if (rc == 0) {
        state->manifest_pending = 1;
        state->manifest_deadline_ms = 0U;
        rc = publish_wire(state, state->mqtt.manifest_request_topic, &wire,
                          MQTT_QOS_AT_LEAST_ONCE);
    }
    if (rc == 0) {
        state->manifest_deadline_ms =
            monotonic_ms() + REQUEST_TIMEOUT_SECONDS * 1000ULL;
        printf("requested deployed manifest topic=%s qos=1\n",
               state->mqtt.manifest_request_topic);
    }
    if (rc != 0) state->manifest_pending = 0;
    geisa_proto_bytes_free(&wire);
    return rc;
}

/*
 * Requests required Platform Discovery during startup and checks
 * pending/timeout state before publishing so an immediate response can be
 * handled safely.
 */
static int request_discovery(struct app_state *state) {
    struct geisa_proto_bytes wire = {0};
    int rc = geisa_proto_encode_discovery_request(&wire);
    if (rc == 0) {
        state->discovery_pending = 1;
        state->discovery_deadline_ms = 0U;
        rc = publish_wire(state, state->mqtt.discovery_request_topic, &wire,
                          MQTT_QOS_AT_LEAST_ONCE);
    }
    if (rc == 0) {
        state->discovery_deadline_ms =
            monotonic_ms() + REQUEST_TIMEOUT_SECONDS * 1000ULL;
        printf("requested platform discovery topic=%s qos=1\n",
               state->mqtt.discovery_request_topic);
    }
    if (rc != 0) state->discovery_pending = 0;
    geisa_proto_bytes_free(&wire);
    return rc;
}

/*
 * Publishes the recurring geisa-simple EVENT, records its request ID for
 * correlation, and starts the response timeout.
 */
static int publish_event(struct app_state *state) {
    struct geisa_proto_bytes wire = {0};
    char request_id[192];
    static const char payload[] =
        "{\"event_type\":\"geisa-simple-example\",\"severity\":\"low\","
        "\"description\":\"basic GEISA application event\"}";
    int rc;

    snprintf(request_id, sizeof(request_id), "geisa-simple-event-%llu-%u",
             (unsigned long long)epoch_ms(), ++state->message_sequence);
    rc = geisa_proto_encode_app_request(
        request_id, GEISA_PROTO_PRIORITY_BEST_EFFORT, GEISA_PROTO_MESSAGE_EVENT,
        epoch_ms(), EVENT_TTL_SECONDS, "application/json",
        (const unsigned char *)payload, strlen(payload), &wire);
    if (rc == 0) {
        snprintf(state->pending_request_id, sizeof(state->pending_request_id),
                 "%s", request_id);
        state->pending_deadline_ms = 0U;
        rc = publish_wire(state, state->mqtt.upstream_request_topic, &wire,
                          MQTT_QOS_AT_LEAST_ONCE);
    }
    if (rc == 0) {
        state->pending_deadline_ms =
            monotonic_ms() + REQUEST_TIMEOUT_SECONDS * 1000ULL;
        printf("published EVENT request_id=%s topic=%s qos=1\n", request_id,
               state->mqtt.upstream_request_topic);
    }
    if (rc != 0) state->pending_request_id[0] = '\0';
    geisa_proto_bytes_free(&wire);
    return rc;
}

/* Sends the GEISA app-message response for a CONFIG request */
static int publish_config_response(struct app_state *state,
                                   const char *request_id, unsigned status,
                                   const char *text) {
    struct geisa_proto_bytes wire = {0};
    int rc = geisa_proto_encode_app_response(request_id, status, text,
                                             epoch_ms(), &wire);
    if (rc == 0) {
        rc = publish_wire(state, state->mqtt.downstream_response_topic, &wire,
                          MQTT_QOS_AT_LEAST_ONCE);
    }
    geisa_proto_bytes_free(&wire);
    return rc;
}

static void
log_platform_status(const struct geisa_proto_platform_to_app_status *status) {
    printf("platform status cpu=%.2f/%.2f memory=%llu/%llu persist=%llu/%llu "
           "transient=%llu/%llu messages=%llu/%llu remaining=%llu\n",
           (double)status->cpu_usage, (double)status->cpu_limit,
           (unsigned long long)status->memory_usage,
           (unsigned long long)status->memory_limit,
           (unsigned long long)status->persist_storage_usage,
           (unsigned long long)status->persist_storage_limit,
           (unsigned long long)status->nonpersist_storage_usage,
           (unsigned long long)status->nonpersist_storage_limit,
           (unsigned long long)status->conn_msg.today_used,
           (unsigned long long)status->conn_msg.today_limit,
           (unsigned long long)status->conn_msg.today_remaining);
}

/*
 * Logs a valid non-application-specific status broadcast without taking action.
 */
static void handle_global_platform_status(const unsigned char *payload,
                                          size_t length) {
    struct geisa_proto_global_platform_status status;

    if (geisa_proto_decode_global_platform_status(payload, length, &status) !=
        0) {
        fprintf(stderr, "malformed global platform status ignored\n");
        return;
    }
    printf("global platform status timestamp=%llu mode=%u conn-msg=%u "
           "alerts=%d%d%d%d%d%d%d\n",
           (unsigned long long)status.timestamp_ms, status.mode,
           status.conn_msg, status.sys_over_temp, status.sys_high_cpu,
           status.sys_low_mem, status.sys_power_degraded, status.sys_power_loss,
           status.sys_reboot_soon, status.sys_shutdown_soon);
}

/*
 * Handles platform status/control messages, including immediate status and
 * platform-initiated shutdown requests.
 */
static void handle_platform_status(struct app_state *state,
                                   const unsigned char *payload,
                                   size_t length) {
    struct geisa_proto_platform_to_app_status status;
    if (geisa_proto_decode_platform_to_app_status(payload, length, &status) !=
        0) {
        fprintf(stderr, "malformed platform-to-app status ignored\n");
        return;
    }
    log_platform_status(&status);
    if (status.cmd_shut_down && !state->shutdown_requested) {
        state->shutdown_requested = 1;
        if (publish_app_status(state, GEISA_PROTO_APP_STATUS_SHUTTING_DOWN) !=
            0) {
            state->outcome.failed = 1;
        }
        state->done = 1;
    } else if (status.cmd_send_status) {
        if (publish_app_status(state, GEISA_PROTO_APP_STATUS_RUNNING) != 0) {
            state->outcome.failed = 1;
            state->done = 1;
        }
    }
}

/*
 * Validates the expected Manifest response; malformed or failed retrieval ends
 * this run if during startup.
 */
static void handle_manifest_response(struct app_state *state,
                                     const unsigned char *payload,
                                     size_t length) {
    struct geisa_proto_manifest_response_view response;
    if (!state->manifest_pending) {
        fprintf(stderr, "unmatched deployment-manifest response ignored\n");
        return;
    }
    if (geisa_proto_decode_manifest_response(payload, length, &response) != 0) {
        fprintf(stderr, "malformed deployment-manifest response\n");
        state->outcome.failed = 1;
        state->done = 1;
        return;
    }
    state->manifest_pending = 0;
    if (response.status_code != GEISA_PROTO_STATUS_SUCCESS) {
        fprintf(stderr, "deployment-manifest request failed status=%u\n",
                response.status_code);
        state->outcome.failed = 1;
        state->done = 1;
        return;
    }
    state->outcome.manifest_succeeded = 1;
    printf("received deployed manifest status=success bytes=%zu:\n%.*s\n",
           response.manifest_len, (int)response.manifest_len,
           response.manifest);
}

/*
 * Validates the pending Discovery response. Malformed or failed retrieval stops
 * startup.
 */
static void handle_discovery_response(struct app_state *state,
                                      const unsigned char *payload,
                                      size_t length) {
    struct geisa_proto_discovery_response_view response;
    if (!state->discovery_pending) {
        fprintf(stderr, "unmatched platform discovery response ignored\n");
        return;
    }
    if (geisa_proto_decode_discovery_response(payload, length, &response) !=
            0 ||
        response.status_code != GEISA_PROTO_STATUS_SUCCESS) {
        fprintf(stderr, "platform discovery request failed\n");
        state->outcome.failed = 1;
        state->done = 1;
        return;
    }
    state->discovery_pending = 0;
    state->outcome.discovery_succeeded = 1;
    if (response.has_geisa_version) {
        printf("received platform discovery GEISA version=%u.%u.%u\n",
               response.geisa_major, response.geisa_minor,
               response.geisa_revision);
    } else {
        puts("received platform discovery");
    }
}

/* Correlates an EVENT response by request ID and clears its pending state. */
static void handle_upstream_response(struct app_state *state,
                                     const unsigned char *payload,
                                     size_t length) {
    struct geisa_proto_app_response_view response;
    if (geisa_proto_decode_app_response(payload, length, &response) != 0) {
        fprintf(stderr, "malformed upstream app-message response ignored\n");
        if (state->pending_request_id[0] != '\0') {
            state->outcome.failed = 1;
            state->done = 1;
        }
        return;
    }
    if (state->pending_request_id[0] == '\0' ||
        strcmp(response.request_id, state->pending_request_id) != 0) {
        state->unmatched_responses++;
        fprintf(stderr, "unmatched upstream response request_id=%s ignored\n",
                response.request_id);
        return;
    }
    printf("received upstream response request_id=%s status=%u text=%s\n",
           response.request_id, response.status, response.status_text);
    state->pending_request_id[0] = '\0';
    state->pending_deadline_ms = 0;
}

/*
 * Decodes JSON CONFIG carried in a GEISA app-message request. Applies changes
 * to a copy, preserving timing for read-only requests, and reschedules
 * changed reporting intervals.
 */
static void handle_config_request(struct app_state *state,
                                  const unsigned char *payload, size_t length) {
    struct geisa_proto_app_request_view request;
    struct geisa_simple_config candidate;
    char error[160];
    char effective[256];
    int result;

    /* GEISA carries geisa-simple's JSON CONFIG doc inside the app message */
    if (geisa_proto_decode_app_request(payload, length, &request) != 0) {
        fprintf(stderr, "malformed downstream app-message ignored\n");
        return;
    }
    if (request.message_type == GEISA_PROTO_MESSAGE_COMMAND) {
        (void)publish_config_response(
            state, request.request_id, GEISA_PROTO_MESSAGE_STATUS_REJECTED,
            "geisa-simple has no application COMMAND messages");
        return;
    }
    if (request.message_type != GEISA_PROTO_MESSAGE_CONFIG ||
        strcmp(request.content_type, "application/json") != 0) {
        (void)publish_config_response(
            state, request.request_id, GEISA_PROTO_MESSAGE_STATUS_REJECTED,
            "only application/json CONFIG is supported");
        return;
    }
    candidate = state->config;
    error[0] = '\0';
    result = geisa_simple_config_apply_json((const char *)request.payload,
                                            request.payload_len, &candidate,
                                            error, sizeof(error));
    if (result < 0) {
        (void)publish_config_response(state, request.request_id,
                                      GEISA_PROTO_MESSAGE_STATUS_REJECTED,
                                      error[0] ? error : "invalid CONFIG");
        return;
    }
    {
        int reporting_changed =
            candidate.reporting_enabled != state->config.reporting_enabled ||
            candidate.reporting_interval_seconds !=
                state->config.reporting_interval_seconds;
        state->config = candidate;
        if (result == GEISA_SIMPLE_CONFIG_APPLIED && reporting_changed) {
            state->next_message_ms = monotonic_ms();
        }
    }
    (void)geisa_simple_config_format(&state->config, effective,
                                     sizeof(effective));
    if (publish_config_response(state, request.request_id,
                                GEISA_PROTO_MESSAGE_STATUS_ACCEPTED,
                                result == GEISA_SIMPLE_CONFIG_READ
                                    ? "configuration read; effective "
                                      "configuration logged locally"
                                    : "configuration applied") != 0) {
        state->outcome.failed = 1;
        state->done = 1;
    }
    printf("CONFIG %s effective=%s\n",
           result == GEISA_SIMPLE_CONFIG_READ ? "read" : "applied", effective);
}

static int queue_startup_subscription(struct mosquitto *mosq,
                                      struct app_state *state, unsigned slot,
                                      const char *topic) {
    int mid = 0;
    int rc;

    if (slot >= STARTUP_SUBSCRIPTION_COUNT) return MOSQ_ERR_INVAL;
    rc = mosquitto_subscribe(mosq, &mid, topic, MQTT_QOS_AT_LEAST_ONCE);
    if (rc == MOSQ_ERR_SUCCESS) state->startup_subscription_mids[slot] = mid;
    return rc;
}

/*
 * Track startup SUBACKs and report pending, ready, or rejected state.
 */
static int record_startup_subscription_ack(struct app_state *state, int mid,
                                           int qos_count,
                                           const int *granted_qos) {
    unsigned slot;

    for (slot = 0U; slot < STARTUP_SUBSCRIPTION_COUNT; slot++) {
        if (state->startup_subscription_mids[slot] == mid) break;
    }
    if (slot == STARTUP_SUBSCRIPTION_COUNT ||
        state->startup_subscription_acked[slot]) {
        return STARTUP_SUBSCRIPTION_PENDING;
    }
    if (qos_count != 1 || granted_qos == NULL || granted_qos[0] < 0 ||
        granted_qos[0] == MQTT_SUBACK_FAILURE) {
        return STARTUP_SUBSCRIPTION_REJECTED;
    }
    state->startup_subscription_acked[slot] = 1;
    state->startup_subscription_acks++;
    return state->startup_subscription_acks == STARTUP_SUBSCRIPTION_COUNT
               ? STARTUP_SUBSCRIPTIONS_READY
               : STARTUP_SUBSCRIPTION_PENDING;
}

/*
 * Publishes RUNNING, requests Discovery, requests the deploy manifest, then
 * initializes EVENT timing.
 */
static int publish_startup_transactions(struct app_state *state) {
    if (publish_app_status(state, GEISA_PROTO_APP_STATUS_RUNNING) != 0) {
        return -1;
    }
    state->startup_status_published = 1;
    state->next_status_ms = monotonic_ms() + STATUS_INTERVAL_SECONDS * 1000ULL;
    if (request_discovery(state) != 0 || request_manifest(state) != 0) {
        return -1;
    }
    state->next_message_ms = monotonic_ms() + 1000;
    return 0;
}

/* Queues subscriptions we need; startup waits for successful SUBACKs */
static void on_connect(struct mosquitto *mosq, void *userdata, int result) {
    struct app_state *state = userdata;
    if (result != 0) {
        fprintf(stderr, "MQTT connection failed result=%d\n", result);
        state->outcome.failed = 1;
        state->done = 1;
        return;
    }
    state->connected = 1;
    state->startup_subscription_acks = 0U;
    memset(state->startup_subscription_mids, 0,
           sizeof(state->startup_subscription_mids));
    memset(state->startup_subscription_acked, 0,
           sizeof(state->startup_subscription_acked));
    if (queue_startup_subscription(mosq, state, 0U,
                                   GLOBAL_PLATFORM_STATUS_TOPIC) !=
            MOSQ_ERR_SUCCESS ||
        queue_startup_subscription(mosq, state, 1U,
                                   state->mqtt.platform_status_topic) !=
            MOSQ_ERR_SUCCESS ||
        queue_startup_subscription(mosq, state, 2U,
                                   state->mqtt.manifest_response_topic) !=
            MOSQ_ERR_SUCCESS ||
        queue_startup_subscription(mosq, state, 3U,
                                   state->mqtt.discovery_response_topic) !=
            MOSQ_ERR_SUCCESS ||
        queue_startup_subscription(mosq, state, 4U,
                                   state->mqtt.upstream_response_topic) !=
            MOSQ_ERR_SUCCESS ||
        queue_startup_subscription(mosq, state, 5U,
                                   state->mqtt.downstream_request_topic) !=
            MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "MQTT subscription setup failed\n");
        state->outcome.failed = 1;
        state->done = 1;
        return;
    }
}

/* Starts app work only after every required subscription is acked */
static void on_subscribe(struct mosquitto *mosq, void *userdata, int mid,
                         int qos_count, const int *granted_qos) {
    struct app_state *state = userdata;
    int ready;

    (void)mosq;
    ready = record_startup_subscription_ack(state, mid, qos_count, granted_qos);
    if (ready == STARTUP_SUBSCRIPTION_REJECTED) {
        fprintf(stderr, "required MQTT startup subscription was rejected\n");
        state->outcome.failed = 1;
        state->done = 1;
        return;
    }
    if (ready == STARTUP_SUBSCRIPTIONS_READY) {
        if (publish_startup_transactions(state) != 0) {
            state->outcome.failed = 1;
            state->done = 1;
        }
    }
}

/* Routes platform status, Discovery, Manifest, EVENT, and CONFIG messages */
static void on_message(struct mosquitto *mosq, void *userdata,
                       const struct mosquitto_message *message) {
    struct app_state *state = userdata;
    const unsigned char *payload;
    size_t length;
    (void)mosq;
    if (!message || !message->topic || message->payloadlen < 0) return;
    payload = message->payload;
    length = (size_t)message->payloadlen;
    if (strcmp(message->topic, GLOBAL_PLATFORM_STATUS_TOPIC) == 0) {
        handle_global_platform_status(payload, length);
    } else if (strcmp(message->topic, state->mqtt.platform_status_topic) == 0) {
        handle_platform_status(state, payload, length);
    } else if (strcmp(message->topic, state->mqtt.manifest_response_topic) ==
               0) {
        handle_manifest_response(state, payload, length);
    } else if (strcmp(message->topic, state->mqtt.discovery_response_topic) ==
               0) {
        handle_discovery_response(state, payload, length);
    } else if (strcmp(message->topic, state->mqtt.upstream_response_topic) ==
               0) {
        handle_upstream_response(state, payload, length);
    } else if (strcmp(message->topic, state->mqtt.downstream_request_topic) ==
               0) {
        handle_config_request(state, payload, length);
    }
}

/* Loads configuration, runs the MQTT lifecycle, and cleans up on exit or failure */
int main(void) {
    struct app_state state = {0};
    int rc;
    if (load_mqtt_config(&state.mqtt) != 0) {
        return GEISA_SIMPLE_EXIT_SETUP_FAILURE;
    }
    geisa_simple_config_defaults(&state.config);
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);
    printf("starting geisa-simple userid=%s broker=%s:%d\n", state.mqtt.userid,
           state.mqtt.host, state.mqtt.port);

    mosquitto_lib_init();
    state.mosq = mosquitto_new(state.mqtt.client_id, true, &state);
    if (!state.mosq) {
        mosquitto_lib_cleanup();
        return GEISA_SIMPLE_EXIT_SETUP_FAILURE;
    }
    rc = mosquitto_int_option(state.mosq, MOSQ_OPT_PROTOCOL_VERSION,
                              MQTT_PROTOCOL_V5);
    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "MQTT v5 setup failed: %s\n", mosquitto_strerror(rc));
        mosquitto_destroy(state.mosq);
        mosquitto_lib_cleanup();
        return GEISA_SIMPLE_EXIT_SETUP_FAILURE;
    }
    if (state.mqtt.has_password) {
        rc = mosquitto_username_pw_set(state.mosq, state.mqtt.userid,
                                       state.mqtt.password);
        if (rc != MOSQ_ERR_SUCCESS) {
            fprintf(stderr, "MQTT credentials setup failed\n");
            mosquitto_destroy(state.mosq);
            mosquitto_lib_cleanup();
            return GEISA_SIMPLE_EXIT_SETUP_FAILURE;
        }
    }
    mosquitto_connect_callback_set(state.mosq, on_connect);
    mosquitto_subscribe_callback_set(state.mosq, on_subscribe);
    mosquitto_message_callback_set(state.mosq, on_message);
    rc = mosquitto_connect(state.mosq, state.mqtt.host, state.mqtt.port,
                           MQTT_KEEPALIVE_SECONDS);
    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "MQTT connect failed: %s\n", mosquitto_strerror(rc));
        mosquitto_destroy(state.mosq);
        mosquitto_lib_cleanup();
        return GEISA_SIMPLE_EXIT_CONNECTION_FAILURE;
    }

    while (!state.done && !g_stop) {
        uint64_t now = monotonic_ms();
        rc = mosquitto_loop(state.mosq, 200, 1);
        if (rc != MOSQ_ERR_SUCCESS) {
            fprintf(stderr, "MQTT loop failed: %s\n", mosquitto_strerror(rc));
            state.outcome.failed = 1;
            break;
        }
        now = monotonic_ms();
        if (state.connected && state.startup_status_published &&
            now >= state.next_status_ms) {
            if (publish_app_status(&state, GEISA_PROTO_APP_STATUS_RUNNING) !=
                0) {
                state.outcome.failed = 1;
                state.done = 1;
            }
            state.next_status_ms = now + STATUS_INTERVAL_SECONDS * 1000ULL;
        }
        if (state.pending_request_id[0] && now >= state.pending_deadline_ms) {
            fprintf(stderr, "upstream response timeout request_id=%s\n",
                    state.pending_request_id);
            state.pending_request_id[0] = '\0';
            state.pending_deadline_ms = 0;
            state.outcome.failed = 1;
            state.done = 1;
        }
        if (state.discovery_pending && now >= state.discovery_deadline_ms) {
            fprintf(stderr, "platform discovery response timeout\n");
            state.discovery_pending = 0;
            state.outcome.failed = 1;
            state.done = 1;
        }
        if (state.manifest_pending && now >= state.manifest_deadline_ms) {
            fprintf(stderr, "deployment-manifest response timeout\n");
            state.manifest_pending = 0;
            state.outcome.failed = 1;
            state.done = 1;
        }
        if (state.connected && state.config.reporting_enabled &&
            state.startup_status_published &&
            state.outcome.discovery_succeeded &&
            state.outcome.manifest_succeeded &&
            state.pending_request_id[0] == '\0' &&
            now >= state.next_message_ms) {
            if (publish_event(&state) != MOSQ_ERR_SUCCESS) {
                state.outcome.failed = 1;
                break;
            }
            state.next_message_ms =
                now +
                (uint64_t)state.config.reporting_interval_seconds * 1000ULL;
        }
    }

    if (state.shutdown_requested) {
        (void)mosquitto_loop(state.mosq, 1000, 1);
    } else if (g_stop && state.connected) {
        if (publish_app_status(&state, GEISA_PROTO_APP_STATUS_SHUTTING_DOWN) !=
            0) {
            state.outcome.failed = 1;
        }
        (void)mosquitto_loop(state.mosq, 1000, 1);
    }
    mosquitto_disconnect(state.mosq);
    mosquitto_destroy(state.mosq);
    mosquitto_lib_cleanup();
    return state.outcome.failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
