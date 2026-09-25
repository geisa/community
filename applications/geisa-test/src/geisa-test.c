/*
 * geisa-test.c
 *
 * Runs configured GEISA application-message and bounded resource tests, and
 * handles MQTT lifecycle, CONFIG, COMMAND, and platform control traffic.
 *
 * Copyright 2026 PragSol Consulting LLC.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <fcntl.h>
#include <mosquitto.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "geisa-protobuf.h"
#include "geisa-test-message-contract.h"

#define DEFAULT_HOST "localhost"
#define DEFAULT_PORT 1883
#define DEFAULT_USERID "com.pragsol.geisa-test"
#define CONFIG_PATH "/etc/geisa/mqtt.conf"
#define DEFAULT_REPORTING_ENABLED 1
#define DEFAULT_REPORTING_INTERVAL_SECONDS 3600
#define DEFAULT_TIMEOUT_SECONDS 10
#define DEFAULT_TTL_SECONDS 300
#define DEFAULT_BURST_COUNT 4
#define DEFAULT_BURST_INTERVAL_MS 1000
#define DEFAULT_MAX_ATTEMPTS 10
#define DEFAULT_REPORTING_MESSAGE_TYPES "EVENT,ALARM,APP_DATA,TELEMETRY"
#define DEFAULT_PAYLOAD_SIZE_BYTES 256
#define DEFAULT_PROBE_DURATION_MS 1000
#define DEFAULT_PROBE_TARGET_BYTES 1048576
#define DEFAULT_PROBE_CHUNK_BYTES 65536
#define DEFAULT_PROBE_INTENSITY_PERCENT 100
#define MAX_PROBE_DURATION_MS 30000
#define MAX_PROBE_TARGET_BYTES (64 * 1024 * 1024)
#define MAX_PROBE_CHUNK_BYTES (1024 * 1024)
#define DEFAULT_REQUEST_ID_PREFIX "geisa-test"
#define DEFAULT_PRIORITY "GEISA_APP_MESSAGE_PRIORITY_BEST_EFFORT"
#define DEFAULT_PAYLOAD_JSON                                                   \
    "{\"status\":\"ready\",\"detail\":\"default geisa-test report payload\"}"
#define MAX_PAYLOAD_BYTES 65536
#define MAX_RESPONSE_STATUS_COUNTS 32
#define DISCOVERY_REQUEST_TOPIC_PREFIX "geisa/api/platform/discovery/req/"
#define DISCOVERY_RESPONSE_TOPIC_PREFIX "geisa/api/platform/discovery/rsp/"
#define PLATFORM_STATUS_TOPIC_PREFIX "geisa/api/platform/app/status/"
#define APP_STATUS_TOPIC_PREFIX "geisa/api/app/platform/status/"
#define MANIFEST_REQUEST_TOPIC_PREFIX "geisa/api/app/manifest/req/"
#define MANIFEST_RESPONSE_TOPIC_PREFIX "geisa/api/app/manifest/rsp/"
#define APP_MESSAGE_REQUEST_TOPIC_PREFIX GEISA_APP_MESSAGE_REQUEST_TOPIC_PREFIX
#define APP_MESSAGE_RESPONSE_TOPIC_PREFIX                                      \
    GEISA_APP_MESSAGE_RESPONSE_TOPIC_PREFIX
#define DOWNSTREAM_MESSAGE_REQUEST_TOPIC_PREFIX                                \
    GEISA_APP_MESSAGE_DOWNSTREAM_REQUEST_TOPIC_PREFIX
#define DOWNSTREAM_MESSAGE_RESPONSE_TOPIC_PREFIX                               \
    GEISA_APP_MESSAGE_DOWNSTREAM_RESPONSE_TOPIC_PREFIX
#define STATUS_ACCEPTED GEISA_APP_MESSAGE_STATUS_ACCEPTED
#define STATUS_QUOTA_EXCEEDED GEISA_APP_MESSAGE_STATUS_QUOTA_EXCEEDED
#define SERVER_MESSAGE_TYPE_CONFIG 1
#define PENDING_KIND_NONE 0
#define PENDING_KIND_REPORT 1
#define STARTUP_SUBSCRIPTION_COUNT 6U
#define GLOBAL_PLATFORM_STATUS_TOPIC "geisa/api/platform/status"
#define STATUS_INTERVAL_MS 60000ULL

#define APP_MODE_NORMAL 0
#define APP_MODE_QUOTA_EXCEED 1
#define APP_MODE_BURST 2
#define APP_MODE_PAYLOAD_SIZE 3
#define APP_MODE_LEE_EXCEED_CPU 5
#define APP_MODE_LEE_EXCEED_MEM 6
#define APP_MODE_LEE_EXCEED_PERSISTENT_STORAGE 7
#define APP_MODE_LEE_EXCEED_TRANSIENT_STORAGE 8
#define APP_MODE_LEE_ALL 9
#define APP_MODE_API_ALL 10
#define APP_MODE_ALL 12

#define APP_RUN_MODE_NORMAL 0
#define APP_RUN_MODE_TEST 1

#define REPORTING_MESSAGE_TYPE_EVENT_MASK (1U << 0)
#define REPORTING_MESSAGE_TYPE_ALARM_MASK (1U << 1)
#define REPORTING_MESSAGE_TYPE_APP_DATA_MASK (1U << 2)
#define REPORTING_MESSAGE_TYPE_TELEMETRY_MASK (1U << 3)

struct mqtt_cfg {
    char host[256];
    int port;
    char userid[256];
    char client_id[256];
    char discovery_request_topic[512];
    char discovery_response_topic[512];
    char app_message_request_topic[512];
    char app_message_response_topic[512];
    char downstream_message_request_topic[512];
    char downstream_message_response_topic[512];
    char password[256];
    int has_password;
};

struct app_options {
    int reporting_enabled;
    int stay_running;
    int offline_preview;
    int reporting_interval_seconds;
    int timeout_seconds;
    int burst_count;
    int burst_interval_ms;
    int max_attempts;
    int message_count;
    char payload_pattern[16];
    char expected_status[160];
    int duration_ms;
    int target_bytes;
    int chunk_bytes;
    int intensity_percent;
    int ttl_seconds;
    int payload_size_bytes;
    unsigned reporting_message_types_mask;
    int host_override_set;
    int port_override_set;
    int userid_override_set;
    char host_override[256];
    int port_override;
    char userid_override[256];
    int operational_mode;
    int application_mode;
    char test_name[32];
    int quota_interval_ms;
    char reporting_message_types[256];
    char request_id_prefix[128];
    char config_file[512];
    char persistent_storage_dir[512];
    char transient_storage_dir[512];
};

struct probe_state {
    pthread_t thread;
    volatile int running;
    volatile int cancel;
    volatile int done;
    unsigned invocations;
    int mode;
    int requested_duration_ms;
    int requested_target_bytes;
    int requested_chunk_bytes;
    int requested_intensity_percent;
    uint64_t started_ms;
    uint64_t elapsed_ms;
    uint64_t attempted_bytes;
    uint64_t completed_bytes;
    uint64_t touched_bytes;
    uint64_t cpu_time_ms;
    unsigned failures;
    int interrupted;
    int cleaned_up;
    char result[32];
    char error[96];
    char storage_dir[512];
};

struct app_state {
    struct mqtt_cfg cfg;
    struct app_options options;
    uint64_t next_send_ms;
    uint64_t pending_deadline_ms;
    uint64_t discovery_deadline_ms;
    uint64_t next_status_ms;
    unsigned send_attempt;
    unsigned accepted_count;
    unsigned rejected_count;
    unsigned other_count;
    unsigned timeout_count;
    unsigned duplicate_response_count;
    unsigned unmatched_response_count;
    unsigned malformed_response_count;
    unsigned status_mismatch_count;
    unsigned published_count;
    struct {
        char status[160];
        unsigned count;
    } response_status_counts[MAX_RESPONSE_STATUS_COUNTS];
    size_t response_status_count;
    unsigned cycle_remaining;
    size_t reporting_profile_index;
    size_t reporting_profile_count;
    const struct app_message_profile *reporting_profiles[4];
    size_t last_payload_size;
    int connected;
    unsigned startup_subscription_acks;
    int startup_subscriptions_ready;
    int startup_subscription_mids[STARTUP_SUBSCRIPTION_COUNT];
    int startup_subscription_requested_qos[STARTUP_SUBSCRIPTION_COUNT];
    int startup_subscription_acked[STARTUP_SUBSCRIPTION_COUNT];
    int done;
    int success;
    int discovery_completed;
    int pending_app_response;
    int pending_response_kind;
    int quota_rejected;
    int platform_message_quota_known;
    int platform_message_quota_exhausted;
    uint64_t platform_message_today_used;
    uint64_t platform_message_today_limit;
    uint64_t platform_message_today_remaining;
    uint64_t platform_message_next_reset_ms;
    unsigned config_apply_count;
    unsigned config_reject_count;
    unsigned downstream_response_publish_failures;
    unsigned command_received_count;
    unsigned command_accepted_count;
    unsigned command_rejected_count;
    unsigned command_executed_count;
    unsigned downstream_malformed_count;
    int shutdown_requested;
    int test_run_pending;
    unsigned aggregate_index;
    unsigned aggregate_count;
    unsigned aggregate_completed;
    int aggregate_failed;
    int aggregate_modes[8];
    struct probe_state probe;
    char last_request_id[192];
    int response_seen;
    char platform_status_topic[512];
    char app_status_topic[512];
    char manifest_request_topic[512];
    char manifest_response_topic[512];
    unsigned lifecycle_running_reports;
    unsigned lifecycle_shutdown_reports;
    unsigned manifest_requests;
    unsigned manifest_successes;
    unsigned manifest_failures;
    unsigned manifest_malformed;
    unsigned manifest_unmatched;
    unsigned manifest_timeouts;
    unsigned lifecycle_send_status_commands;
    unsigned lifecycle_shutdown_commands;
    unsigned lifecycle_control_malformed;
    int manifest_pending;
    uint64_t manifest_deadline_ms;
    int lifecycle_state;
};

struct app_message_profile {
    const char *message_type;
    const char *priority;
    const char *content_type;
    const char *payload_json;
};

static volatile sig_atomic_t g_stop = 0;
static uint64_t now_ms(void);
static void on_signal(int sig) {
    (void)sig;
    g_stop = 1;
}

static int publish_lifecycle_status(struct mosquitto *mosq,
                                    struct app_state *state, unsigned type) {
    struct geisa_proto_bytes encoded = {0};
    int encode_rc = geisa_proto_encode_app_status(type, 60, 150, &encoded);
    int rc;
    if (encode_rc != 0) return MOSQ_ERR_NOMEM;
    /* Lifecycle status is advisory; QoS 0 is the v0.9.0 contract. */
    rc = mosquitto_publish(mosq, NULL, state->app_status_topic,
                           (int)encoded.len, encoded.data, 0, false);
    geisa_proto_bytes_free(&encoded);
    if (rc == MOSQ_ERR_SUCCESS) {
        if (type == GEISA_PROTO_APP_STATUS_RUNNING)
            state->lifecycle_running_reports++;
        if (type == GEISA_PROTO_APP_STATUS_SHUTTING_DOWN)
            state->lifecycle_shutdown_reports++;
        state->lifecycle_state = (int)type;
        if (type == GEISA_PROTO_APP_STATUS_RUNNING)
            state->next_status_ms = now_ms() + STATUS_INTERVAL_MS;
    }
    return rc;
}

static int decode_manifest_response(const void *payload, size_t len,
                                    unsigned *status_code,
                                    const unsigned char **manifest,
                                    size_t *manifest_len) {
    static _Thread_local struct geisa_proto_manifest_response_view decoded;
    if (geisa_proto_decode_manifest_response(payload, len, &decoded) != 0)
        return -1;
    *status_code = decoded.status_code;
    *manifest = (const unsigned char *)decoded.manifest;
    *manifest_len = decoded.manifest_len;
    return 0;
}

struct config_update {
    unsigned present_mask;
    int reporting_enabled;
    int reporting_interval_seconds;
    unsigned reporting_message_types_mask;
    int payload_size_bytes;
    int application_mode;
    char test_name[32];
    char reporting_message_types[256];
    int burst_count;
    int burst_interval_ms;
    int max_attempts;
    int message_count;
    char payload_pattern[16];
    char expected_status[160];
    int duration_ms;
    int target_bytes;
    int chunk_bytes;
    int intensity_percent;
    int quota_interval_ms;
};

struct config_request {
    int operation;
    int subset_requested;
    unsigned requested_keys_mask;
    struct config_update update;
};

#define CONFIG_UPDATE_REPORTING_ENABLED (1U << 0)
#define CONFIG_UPDATE_REPORTING_INTERVAL_SECONDS (1U << 1)
#define CONFIG_UPDATE_REPORTING_MESSAGE_TYPES (1U << 2)
#define CONFIG_UPDATE_PAYLOAD_SIZE_BYTES (1U << 3)
#define CONFIG_UPDATE_BURST_COUNT (1U << 5)
#define CONFIG_UPDATE_BURST_INTERVAL_MS (1U << 6)
#define CONFIG_UPDATE_MAX_ATTEMPTS (1U << 7)
#define CONFIG_UPDATE_PAYLOAD_PATTERN (1U << 8)
#define CONFIG_UPDATE_MESSAGE_COUNT (1U << 9)
#define CONFIG_UPDATE_EXPECTED_STATUS (1U << 10)
#define CONFIG_UPDATE_DURATION_MS (1U << 11)
#define CONFIG_UPDATE_TARGET_BYTES (1U << 12)
#define CONFIG_UPDATE_CHUNK_BYTES (1U << 13)
#define CONFIG_UPDATE_INTENSITY_PERCENT (1U << 14)
#define CONFIG_UPDATE_MODE (1U << 15)
#define CONFIG_UPDATE_TEST (1U << 16)
#define CONFIG_UPDATE_QUOTA_INTERVAL_MS (1U << 17)

#define CONFIG_OPERATION_SET 1
#define CONFIG_OPERATION_GET_EFFECTIVE 2

static void populate_reporting_profiles(struct app_state *state);
static void begin_reporting_cycle(struct app_state *state);
static void schedule_next_send(struct app_state *state, uint64_t delay_ms);

static uint64_t now_ms(void) {
    struct timespec ts;

    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        return 0;
    }

    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)(ts.tv_nsec / 1000000ULL);
}

static void trim(char *s) {
    size_t len = strlen(s);
    size_t start = 0;

    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r' ||
                       s[len - 1] == ' ' || s[len - 1] == '\t')) {
        s[--len] = '\0';
    }

    while (s[start] == ' ' || s[start] == '\t') {
        start++;
    }

    if (start > 0) {
        memmove(s, s + start, strlen(s + start) + 1);
    }
}

static void derive_client_id(char *out, size_t out_sz, const char *userid,
                             const char *suffix) {
    size_t suffix_len;
    size_t user_budget;

    if (out_sz == 0) {
        return;
    }

    suffix_len = strlen(suffix);
    if (suffix_len >= out_sz) {
        snprintf(out, out_sz, "%s", suffix);
        return;
    }

    user_budget = out_sz - suffix_len - 1;
    snprintf(out, out_sz, "%.*s%s", (int)user_budget, userid, suffix);
}

static void derive_topic(char *out, size_t out_sz, const char *prefix,
                         const char *userid) {
    if (out_sz == 0) {
        return;
    }

    if (userid == NULL || userid[0] == '\0') {
        snprintf(out, out_sz, "%s%s", prefix, DEFAULT_USERID);
        return;
    }

    if (geisa_app_message_build_topic(out, out_sz, prefix, userid) != 0) {
        out[0] = '\0';
    }
}

static void load_defaults(struct mqtt_cfg *cfg) {
    memset(cfg, 0, sizeof(*cfg));
    snprintf(cfg->host, sizeof(cfg->host), "%s", DEFAULT_HOST);
    cfg->port = DEFAULT_PORT;
    snprintf(cfg->userid, sizeof(cfg->userid), "%s", DEFAULT_USERID);
    derive_client_id(cfg->client_id, sizeof(cfg->client_id), cfg->userid,
                     ".app");
    derive_topic(cfg->discovery_request_topic,
                 sizeof(cfg->discovery_request_topic),
                 DISCOVERY_REQUEST_TOPIC_PREFIX, cfg->userid);
    derive_topic(cfg->discovery_response_topic,
                 sizeof(cfg->discovery_response_topic),
                 DISCOVERY_RESPONSE_TOPIC_PREFIX, cfg->userid);
    derive_topic(cfg->app_message_request_topic,
                 sizeof(cfg->app_message_request_topic),
                 APP_MESSAGE_REQUEST_TOPIC_PREFIX, cfg->userid);
    derive_topic(cfg->app_message_response_topic,
                 sizeof(cfg->app_message_response_topic),
                 APP_MESSAGE_RESPONSE_TOPIC_PREFIX, cfg->userid);
}

static void load_mqtt_config(struct mqtt_cfg *cfg) {
    FILE *f = fopen(CONFIG_PATH, "r");
    char line[1024];

    if (!f) {
        if (errno != ENOENT) {
            fprintf(stderr, "warning: failed to read %s: %s\n", CONFIG_PATH,
                    strerror(errno));
        }
        return;
    }

    while (fgets(line, sizeof(line), f)) {
        char *eq = strchr(line, '=');
        char *key;
        char *val;

        if (!eq) {
            continue;
        }

        *eq = '\0';
        key = line;
        val = eq + 1;
        trim(key);
        trim(val);

        if (strcmp(key, "HOST") == 0 && val[0] != '\0') {
            snprintf(cfg->host, sizeof(cfg->host), "%s", val);
        } else if (strcmp(key, "PORT") == 0 && val[0] != '\0') {
            char *end = NULL;
            long p = strtol(val, &end, 10);
            if (end != val && *end == '\0' && p > 0 && p <= 65535) {
                cfg->port = (int)p;
            }
        } else if (strcmp(key, "USERID") == 0 && val[0] != '\0') {
            snprintf(cfg->userid, sizeof(cfg->userid), "%s", val);
        } else if (strcmp(key, "PASSWORD") == 0) {
            snprintf(cfg->password, sizeof(cfg->password), "%s", val);
            cfg->has_password = 1;
        }
    }

    fclose(f);

    derive_client_id(cfg->client_id, sizeof(cfg->client_id), cfg->userid,
                     ".app");
    derive_topic(cfg->discovery_request_topic,
                 sizeof(cfg->discovery_request_topic),
                 DISCOVERY_REQUEST_TOPIC_PREFIX, cfg->userid);
    derive_topic(cfg->discovery_response_topic,
                 sizeof(cfg->discovery_response_topic),
                 DISCOVERY_RESPONSE_TOPIC_PREFIX, cfg->userid);
    derive_topic(cfg->app_message_request_topic,
                 sizeof(cfg->app_message_request_topic),
                 APP_MESSAGE_REQUEST_TOPIC_PREFIX, cfg->userid);
    derive_topic(cfg->app_message_response_topic,
                 sizeof(cfg->app_message_response_topic),
                 APP_MESSAGE_RESPONSE_TOPIC_PREFIX, cfg->userid);
}

static int env_flag_is_true(const char *name) {
    const char *value = getenv(name);

    return value != NULL &&
           (strcmp(value, "1") == 0 || strcmp(value, "true") == 0 ||
            strcmp(value, "TRUE") == 0 || strcmp(value, "yes") == 0 ||
            strcmp(value, "YES") == 0);
}

static int parse_int_or_default(const char *value, int min_value, int max_value,
                                int fallback) {
    char *end = NULL;
    long parsed;

    if (value == NULL || value[0] == '\0') {
        return fallback;
    }

    parsed = strtol(value, &end, 10);
    if (end == value || *end != '\0' || parsed < min_value ||
        parsed > max_value) {
        return fallback;
    }

    return (int)parsed;
}

static int string_is_true(const char *value) {
    return value != NULL &&
           (strcmp(value, "1") == 0 || strcmp(value, "true") == 0 ||
            strcmp(value, "TRUE") == 0 || strcmp(value, "yes") == 0 ||
            strcmp(value, "YES") == 0);
}

static int reporting_message_type_is_supported(const char *value) {
    static const char *supported[] = {
        "EVENT",
        "ALARM",
        "APP_DATA",
        "TELEMETRY",
    };
    size_t i;

    if (value == NULL || value[0] == '\0') {
        return 0;
    }

    for (i = 0; i < sizeof(supported) / sizeof(supported[0]); i++) {
        if (strcmp(value, supported[i]) == 0) {
            return 1;
        }
    }

    return 0;
}

static unsigned reporting_message_type_mask_for_name(const char *value) {
    if (value == NULL || value[0] == '\0') {
        return 0;
    }
    if (strcmp(value, "EVENT") == 0) {
        return REPORTING_MESSAGE_TYPE_EVENT_MASK;
    }
    if (strcmp(value, "ALARM") == 0) {
        return REPORTING_MESSAGE_TYPE_ALARM_MASK;
    }
    if (strcmp(value, "APP_DATA") == 0) {
        return REPORTING_MESSAGE_TYPE_APP_DATA_MASK;
    }
    if (strcmp(value, "TELEMETRY") == 0) {
        return REPORTING_MESSAGE_TYPE_TELEMETRY_MASK;
    }
    return 0;
}

static int parse_reporting_message_types_mask(const char *value,
                                              unsigned *mask_out) {
    char buffer[512];
    char *token;
    char *cursor = NULL;
    unsigned mask = 0;

    if (!mask_out || value == NULL || value[0] == '\0') {
        return -1;
    }

    snprintf(buffer, sizeof(buffer), "%s", value);
    token = strtok_r(buffer, ",", &cursor);
    while (token != NULL) {
        char *item = token;
        while (*item == ' ' || *item == '\t') {
            item++;
        }
        trim(item);
        if (item[0] == '\0') {
            token = strtok_r(NULL, ",", &cursor);
            continue;
        }
        if (!reporting_message_type_is_supported(item)) {
            return -1;
        }
        mask |= reporting_message_type_mask_for_name(item);
        token = strtok_r(NULL, ",", &cursor);
    }

    if (mask == 0) {
        return -1;
    }

    *mask_out = mask;
    return 0;
}

static int test_mode_from_string(const char *value) {
    if (value == NULL || value[0] == '\0') return -1;
    if (strcmp(value, "cpu") == 0) return APP_MODE_LEE_EXCEED_CPU;
    if (strcmp(value, "memory") == 0) return APP_MODE_LEE_EXCEED_MEM;
    if (strcmp(value, "persistent_storage") == 0)
        return APP_MODE_LEE_EXCEED_PERSISTENT_STORAGE;
    if (strcmp(value, "transient_storage") == 0)
        return APP_MODE_LEE_EXCEED_TRANSIENT_STORAGE;
    if (strcmp(value, "burst") == 0) return APP_MODE_BURST;
    if (strcmp(value, "quota") == 0) return APP_MODE_QUOTA_EXCEED;
    if (strcmp(value, "payload") == 0) return APP_MODE_PAYLOAD_SIZE;
    if (strcmp(value, "lee_all") == 0) return APP_MODE_LEE_ALL;
    if (strcmp(value, "api_all") == 0) return APP_MODE_API_ALL;
    if (strcmp(value, "all") == 0) return APP_MODE_ALL;
    return -1;
}

static const char *test_mode_to_string(int mode) {
    switch (mode) {
    case APP_MODE_QUOTA_EXCEED: return "quota";
    case APP_MODE_BURST: return "burst";
    case APP_MODE_PAYLOAD_SIZE: return "payload";
    case APP_MODE_LEE_EXCEED_CPU: return "cpu";
    case APP_MODE_LEE_EXCEED_MEM: return "memory";
    case APP_MODE_LEE_EXCEED_PERSISTENT_STORAGE: return "persistent_storage";
    case APP_MODE_LEE_EXCEED_TRANSIENT_STORAGE: return "transient_storage";
    case APP_MODE_LEE_ALL: return "lee_all";
    case APP_MODE_API_ALL: return "api_all";
    case APP_MODE_ALL: return "all";
    default: return "none";
    }
}

static const char *application_mode_to_string(int mode) {
    return mode == APP_RUN_MODE_TEST ? "test" : "normal";
}

static int apply_runtime_binding_kv(struct app_options *options,
                                    const char *key, const char *value) {
    if (strcmp(key, "OFFLINE_PREVIEW") == 0) {
        options->offline_preview = string_is_true(value);
    } else if (strcmp(key, "TIMEOUT_SECONDS") == 0) {
        options->timeout_seconds =
            parse_int_or_default(value, 1, 300, options->timeout_seconds);
    } else if (strcmp(key, "HOST") == 0) {
        if (value != NULL && value[0] != '\0') {
            options->host_override_set = 1;
            snprintf(options->host_override, sizeof(options->host_override),
                     "%s", value);
        }
    } else if (strcmp(key, "PORT") == 0) {
        if (value != NULL && value[0] != '\0') {
            options->port_override =
                parse_int_or_default(value, 1, 65535, DEFAULT_PORT);
            options->port_override_set = 1;
        }
    } else if (strcmp(key, "USERID") == 0) {
        if (value != NULL && value[0] != '\0') {
            options->userid_override_set = 1;
            snprintf(options->userid_override, sizeof(options->userid_override),
                     "%s", value);
        }
    } else if (strcmp(key, "REQUEST_ID_PREFIX") == 0) {
        if (value != NULL && value[0] != '\0') {
            snprintf(options->request_id_prefix,
                     sizeof(options->request_id_prefix), "%s", value);
        }
    } else if (strcmp(key, "MESSAGE_COUNT") == 0) {
        options->message_count =
            parse_int_or_default(value, 0, 1000000, options->message_count);
    } else if (strcmp(key, "PAYLOAD_PATTERN") == 0) {
        if (value != NULL && value[0] != '\0') {
            snprintf(options->payload_pattern, sizeof(options->payload_pattern),
                     "%s", value);
        }
    } else if (strcmp(key, "EXPECTED_STATUS") == 0) {
        if (value != NULL && value[0] != '\0') {
            snprintf(options->expected_status, sizeof(options->expected_status),
                     "%s", value);
        }
    } else {
        return -1;
    }
    return 0;
}

static int apply_config_kv(struct app_options *options, const char *key,
                           const char *value) {
    if (strcmp(key, "REPORTING_ENABLED") == 0 ||
        strcmp(key, "reporting_enabled") == 0) {
        options->reporting_enabled = string_is_true(value);
    } else if (strcmp(key, "REPORTING_INTERVAL_SECONDS") == 0 ||
               strcmp(key, "reporting_interval_seconds") == 0) {
        options->reporting_interval_seconds = parse_int_or_default(
            value, 60, 86400, options->reporting_interval_seconds);
    } else if (strcmp(key, "REPORTING_MESSAGE_TYPES") == 0 ||
               strcmp(key, "reporting_message_types") == 0) {
        if (value != NULL && value[0] != '\0') {
            snprintf(options->reporting_message_types,
                     sizeof(options->reporting_message_types), "%s", value);
        }
    } else if (strcmp(key, "PAYLOAD_SIZE_BYTES") == 0 ||
               strcmp(key, "payload_size_bytes") == 0) {
        options->payload_size_bytes = parse_int_or_default(
            value, 0, MAX_PAYLOAD_BYTES, options->payload_size_bytes);
    } else if (strcmp(key, "MESSAGE_COUNT") == 0 ||
               strcmp(key, "message_count") == 0) {
        options->message_count =
            parse_int_or_default(value, 0, 1000000, options->message_count);
    } else if (strcmp(key, "PAYLOAD_PATTERN") == 0 ||
               strcmp(key, "payload_pattern") == 0) {
        if (value != NULL && value[0] != '\0') {
            snprintf(options->payload_pattern, sizeof(options->payload_pattern),
                     "%s", value);
        }
    } else if (strcmp(key, "EXPECTED_STATUS") == 0 ||
               strcmp(key, "expected_status") == 0) {
        if (value != NULL && value[0] != '\0') {
            snprintf(options->expected_status, sizeof(options->expected_status),
                     "%s", value);
        }
    } else if (strcmp(key, "MODE") == 0 || strcmp(key, "mode") == 0) {
        if (value != NULL && value[0] != '\0') {
            options->application_mode = strcmp(value, "test") == 0
                                            ? APP_RUN_MODE_TEST
                                            : APP_RUN_MODE_NORMAL;
        }
    } else if (strcmp(key, "TEST") == 0 || strcmp(key, "test") == 0) {
        if (value != NULL && value[0] != '\0') {
            snprintf(options->test_name, sizeof(options->test_name), "%s",
                     value);
        }
    } else if (strcmp(key, "BURST_COUNT") == 0 ||
               strcmp(key, "burst_count") == 0) {
        options->burst_count =
            parse_int_or_default(value, 1, 1000, options->burst_count);
    } else if (strcmp(key, "BURST_INTERVAL_MS") == 0 ||
               strcmp(key, "burst_interval_ms") == 0) {
        options->burst_interval_ms =
            parse_int_or_default(value, 0, 60000, options->burst_interval_ms);
    } else if (strcmp(key, "QUOTA_MAX_ATTEMPTS") == 0 ||
               strcmp(key, "quota_max_attempts") == 0) {
        options->max_attempts =
            parse_int_or_default(value, 1, 100000, options->max_attempts);
    } else if (strcmp(key, "QUOTA_INTERVAL_MS") == 0 ||
               strcmp(key, "quota_interval_ms") == 0) {
        options->quota_interval_ms =
            parse_int_or_default(value, 0, 60000, options->quota_interval_ms);
    } else if (strcmp(key, "TEST_DURATION_MS") == 0 ||
               strcmp(key, "test_duration_ms") == 0) {
        options->duration_ms = parse_int_or_default(
            value, 1, MAX_PROBE_DURATION_MS, options->duration_ms);
    } else if (strcmp(key, "TEST_TARGET_BYTES") == 0 ||
               strcmp(key, "test_target_bytes") == 0) {
        options->target_bytes = parse_int_or_default(
            value, 1, MAX_PROBE_TARGET_BYTES, options->target_bytes);
    } else if (strcmp(key, "TEST_CHUNK_BYTES") == 0 ||
               strcmp(key, "test_chunk_bytes") == 0) {
        options->chunk_bytes = parse_int_or_default(
            value, 1, MAX_PROBE_CHUNK_BYTES, options->chunk_bytes);
    } else if (strcmp(key, "CPU_INTENSITY_PERCENT") == 0 ||
               strcmp(key, "cpu_intensity_percent") == 0) {
        options->intensity_percent =
            parse_int_or_default(value, 1, 100, options->intensity_percent);
    } else {
        return -1;
    }
    return 0;
}

static const struct app_message_profile *default_profiles(size_t *count) {
    static const struct app_message_profile profiles[] = {
        {
            .message_type = "EVENT",
            .priority = "GEISA_APP_MESSAGE_PRIORITY_BEST_EFFORT",
            .content_type = "application/json",
            .payload_json = "{\"event_type\":\"Test "
                            "Event\",\"severity\":\"low\",\"description\":"
                            "\"test app event payload\"}",
        },
        {
            .message_type = "ALARM",
            .priority = "GEISA_APP_MESSAGE_PRIORITY_URGENT",
            .content_type = "application/json",
            .payload_json = "{\"event_type\":\"Test "
                            "Alarm\",\"severity\":\"high\",\"description\":"
                            "\"test app alarm payload\"}",
        },
        {
            .message_type = "APP_DATA",
            .priority = "GEISA_APP_MESSAGE_PRIORITY_BEST_EFFORT",
            .content_type = "application/json",
            .payload_json = "{\"sample\":\"app-data\",\"value\":123}",
        },
        {
            .message_type = "TELEMETRY",
            .priority = "GEISA_APP_MESSAGE_PRIORITY_LATEST",
            .content_type = "application/json",
            .payload_json =
                "{\"metric\":\"sample-watts\",\"value\":12.34,\"unit\":\"W\"}",
        },
    };

    *count = sizeof(profiles) / sizeof(profiles[0]);
    return profiles;
}

static void load_default_options(struct app_options *options) {
    const char *value;

    memset(options, 0, sizeof(*options));
    options->reporting_enabled = DEFAULT_REPORTING_ENABLED;
    options->reporting_interval_seconds = DEFAULT_REPORTING_INTERVAL_SECONDS;
    options->timeout_seconds = DEFAULT_TIMEOUT_SECONDS;
    options->burst_count = DEFAULT_BURST_COUNT;
    options->burst_interval_ms = DEFAULT_BURST_INTERVAL_MS;
    options->quota_interval_ms = DEFAULT_BURST_INTERVAL_MS;
    options->max_attempts = DEFAULT_MAX_ATTEMPTS;
    options->ttl_seconds = DEFAULT_TTL_SECONDS;
    options->payload_size_bytes = DEFAULT_PAYLOAD_SIZE_BYTES;
    options->duration_ms = DEFAULT_PROBE_DURATION_MS;
    options->target_bytes = DEFAULT_PROBE_TARGET_BYTES;
    options->chunk_bytes = DEFAULT_PROBE_CHUNK_BYTES;
    options->intensity_percent = DEFAULT_PROBE_INTENSITY_PERCENT;
    options->message_count = 0;
    snprintf(options->expected_status, sizeof(options->expected_status), "%s",
             STATUS_ACCEPTED);
    options->operational_mode = APP_MODE_NORMAL;
    options->application_mode = APP_RUN_MODE_NORMAL;
    options->test_name[0] = '\0';
    snprintf(options->payload_pattern, sizeof(options->payload_pattern), "%s",
             "alpha");
    snprintf(options->reporting_message_types,
             sizeof(options->reporting_message_types), "%s",
             DEFAULT_REPORTING_MESSAGE_TYPES);
    snprintf(options->request_id_prefix, sizeof(options->request_id_prefix),
             "%s", DEFAULT_REQUEST_ID_PREFIX);
    value = getenv("GEISA_TEST_PERSISTENT_STORAGE_DIR");
    if (value != NULL && value[0] != '\0') {
        snprintf(options->persistent_storage_dir,
                 sizeof(options->persistent_storage_dir), "%s", value);
    }
    value = getenv("GEISA_TEST_TRANSIENT_STORAGE_DIR");
    if (value != NULL && value[0] != '\0') {
        snprintf(options->transient_storage_dir,
                 sizeof(options->transient_storage_dir), "%s", value);
    }

    value = getenv("GEISA_TEST_APP_REPORTING_ENABLED");
    if (value != NULL) {
        options->reporting_enabled = string_is_true(value);
    }
    options->stay_running = 1;
    options->offline_preview =
        env_flag_is_true("GEISA_TEST_APP_OFFLINE_PREVIEW");

    value = getenv("GEISA_TEST_APP_REPORTING_INTERVAL_SECONDS");
    options->reporting_interval_seconds = parse_int_or_default(
        value, 60, 86400, options->reporting_interval_seconds);
    value = getenv("GEISA_TEST_APP_REPORTING_MESSAGE_TYPES");
    if (value != NULL && value[0] != '\0') {
        snprintf(options->reporting_message_types,
                 sizeof(options->reporting_message_types), "%s", value);
    }
    value = getenv("GEISA_TEST_APP_TEST");
    if (value != NULL && value[0] != '\0') {
        snprintf(options->test_name, sizeof(options->test_name), "%s", value);
    }
    value = getenv("GEISA_TEST_APP_MODE");
    if (value != NULL && value[0] != '\0') {
        options->application_mode = strcmp(value, "test") == 0
                                        ? APP_RUN_MODE_TEST
                                        : APP_RUN_MODE_NORMAL;
    }
    value = getenv("GEISA_TEST_APP_TIMEOUT_SECONDS");
    options->timeout_seconds =
        parse_int_or_default(value, 1, 300, options->timeout_seconds);
    value = getenv("GEISA_TEST_APP_BURST_COUNT");
    options->burst_count =
        parse_int_or_default(value, 1, 1000, options->burst_count);
    value = getenv("GEISA_TEST_APP_BURST_INTERVAL_MS");
    options->burst_interval_ms =
        parse_int_or_default(value, 0, 60000, options->burst_interval_ms);
    value = getenv("GEISA_TEST_APP_QUOTA_INTERVAL_MS");
    options->quota_interval_ms =
        parse_int_or_default(value, 0, 60000, options->quota_interval_ms);
    value = getenv("GEISA_TEST_APP_QUOTA_MAX_ATTEMPTS");
    options->max_attempts =
        parse_int_or_default(value, 1, 100000, options->max_attempts);
    value = getenv("GEISA_TEST_APP_PAYLOAD_SIZE_BYTES");
    options->payload_size_bytes = parse_int_or_default(
        value, 0, MAX_PAYLOAD_BYTES, options->payload_size_bytes);
    value = getenv("GEISA_TEST_APP_TEST_DURATION_MS");
    options->duration_ms = parse_int_or_default(value, 1, MAX_PROBE_DURATION_MS,
                                                options->duration_ms);
    value = getenv("GEISA_TEST_APP_TEST_TARGET_BYTES");
    options->target_bytes = parse_int_or_default(
        value, 1, MAX_PROBE_TARGET_BYTES, options->target_bytes);
    value = getenv("GEISA_TEST_APP_TEST_CHUNK_BYTES");
    options->chunk_bytes = parse_int_or_default(value, 1, MAX_PROBE_CHUNK_BYTES,
                                                options->chunk_bytes);
    value = getenv("GEISA_TEST_APP_CPU_INTENSITY_PERCENT");
    options->intensity_percent =
        parse_int_or_default(value, 1, 100, options->intensity_percent);
    value = getenv("GEISA_TEST_APP_MESSAGE_COUNT");
    options->message_count =
        parse_int_or_default(value, 0, 1000000, options->message_count);
    value = getenv("GEISA_TEST_APP_PAYLOAD_PATTERN");
    if (value != NULL && value[0] != '\0') {
        snprintf(options->payload_pattern, sizeof(options->payload_pattern),
                 "%s", value);
    }
    value = getenv("GEISA_TEST_APP_EXPECTED_STATUS");
    if (value != NULL && value[0] != '\0') {
        snprintf(options->expected_status, sizeof(options->expected_status),
                 "%s", value);
    }
    value = getenv("GEISA_TEST_APP_REQUEST_ID_PREFIX");
    if (value != NULL && value[0] != '\0') {
        snprintf(options->request_id_prefix, sizeof(options->request_id_prefix),
                 "%s", value);
    }
    value = getenv("GEISA_TEST_APP_CONFIG_FILE");
    if (value != NULL && value[0] != '\0') {
        snprintf(options->config_file, sizeof(options->config_file), "%s",
                 value);
    }
    value = getenv("GEISA_TEST_APP_HOST");
    if (value != NULL && value[0] != '\0') {
        options->host_override_set = 1;
        snprintf(options->host_override, sizeof(options->host_override), "%s",
                 value);
    }
    value = getenv("GEISA_TEST_APP_PORT");
    if (value != NULL && value[0] != '\0') {
        options->port_override =
            parse_int_or_default(value, 1, 65535, DEFAULT_PORT);
        options->port_override_set = 1;
    }
    value = getenv("GEISA_TEST_APP_USERID");
    if (value != NULL && value[0] != '\0') {
        options->userid_override_set = 1;
        snprintf(options->userid_override, sizeof(options->userid_override),
                 "%s", value);
    }
}

static void apply_mqtt_overrides(struct mqtt_cfg *cfg,
                                 const struct app_options *options) {
    if (options->host_override_set) {
        snprintf(cfg->host, sizeof(cfg->host), "%s", options->host_override);
    }
    if (options->port_override_set) {
        cfg->port = options->port_override;
    }
    if (options->userid_override_set) {
        snprintf(cfg->userid, sizeof(cfg->userid), "%s",
                 options->userid_override);
    }

    derive_client_id(cfg->client_id, sizeof(cfg->client_id), cfg->userid,
                     ".app");
    derive_topic(cfg->discovery_request_topic,
                 sizeof(cfg->discovery_request_topic),
                 DISCOVERY_REQUEST_TOPIC_PREFIX, cfg->userid);
    derive_topic(cfg->discovery_response_topic,
                 sizeof(cfg->discovery_response_topic),
                 DISCOVERY_RESPONSE_TOPIC_PREFIX, cfg->userid);
    derive_topic(cfg->app_message_request_topic,
                 sizeof(cfg->app_message_request_topic),
                 APP_MESSAGE_REQUEST_TOPIC_PREFIX, cfg->userid);
    derive_topic(cfg->app_message_response_topic,
                 sizeof(cfg->app_message_response_topic),
                 APP_MESSAGE_RESPONSE_TOPIC_PREFIX, cfg->userid);
    derive_topic(cfg->downstream_message_request_topic,
                 sizeof(cfg->downstream_message_request_topic),
                 DOWNSTREAM_MESSAGE_REQUEST_TOPIC_PREFIX, cfg->userid);
    derive_topic(cfg->downstream_message_response_topic,
                 sizeof(cfg->downstream_message_response_topic),
                 DOWNSTREAM_MESSAGE_RESPONSE_TOPIC_PREFIX, cfg->userid);
}

static int validate_options(struct app_options *options) {
    unsigned mask = 0U;
    int mode;

    if (!options) {
        return -1;
    }

    if (options->application_mode == APP_RUN_MODE_TEST) {
        mode = test_mode_from_string(options->test_name);
        if (mode < 0) {
            fprintf(stderr, "test mode requires a supported test name: %s\n",
                    options->test_name);
            return -1;
        }
        options->operational_mode = mode;
    } else {
        options->operational_mode = APP_MODE_NORMAL;
    }

    if (parse_reporting_message_types_mask(options->reporting_message_types,
                                           &mask) != 0) {
        fprintf(stderr, "unsupported reporting_message_types: %s\n",
                options->reporting_message_types);
        return -1;
    }
    options->reporting_message_types_mask = mask;

    if (options->burst_count < 1 || options->burst_count > 1000) {
        fprintf(stderr, "burst_count out of range: %d\n", options->burst_count);
        return -1;
    }
    if (options->burst_interval_ms < 0 || options->burst_interval_ms > 60000) {
        fprintf(stderr, "burst_interval_ms out of range: %d\n",
                options->burst_interval_ms);
        return -1;
    }
    if (options->max_attempts < 1 || options->max_attempts > 100000) {
        fprintf(stderr, "max_attempts out of range: %d\n",
                options->max_attempts);
        return -1;
    }
    if (options->reporting_interval_seconds < 60 ||
        options->reporting_interval_seconds > 86400) {
        fprintf(stderr, "reporting_interval_seconds out of range: %d\n",
                options->reporting_interval_seconds);
        return -1;
    }
    if (options->payload_size_bytes < 0 ||
        options->payload_size_bytes > MAX_PAYLOAD_BYTES) {
        fprintf(stderr, "payload_size_bytes out of range: %d\n",
                options->payload_size_bytes);
        return -1;
    }
    if (strcmp(options->payload_pattern, "alpha") != 0 &&
        strcmp(options->payload_pattern, "zero") != 0 &&
        strcmp(options->payload_pattern, "index") != 0) {
        fprintf(stderr, "payload_pattern must be alpha, zero, or index: %s\n",
                options->payload_pattern);
        return -1;
    }
    if (options->message_count < 0 || options->message_count > 1000000) {
        fprintf(stderr, "message_count out of range: %d\n",
                options->message_count);
        return -1;
    }
    if (options->expected_status[0] == '\0') {
        fprintf(stderr, "expected_status must not be empty\n");
        return -1;
    }

    return 0;
}

static void populate_reporting_profiles(struct app_state *state) {
    size_t total = 0;
    size_t i;
    const struct app_message_profile *profiles = default_profiles(&total);

    state->reporting_profile_count = 0U;
    state->reporting_profile_index = 0U;

    for (i = 0; i < total && state->reporting_profile_count < 4U; i++) {
        unsigned mask =
            reporting_message_type_mask_for_name(profiles[i].message_type);
        if ((state->options.reporting_message_types_mask & mask) != 0U) {
            state->reporting_profiles[state->reporting_profile_count++] =
                &profiles[i];
        }
    }

    if (state->reporting_profile_count == 0U) {
        state->reporting_profiles[state->reporting_profile_count++] =
            &profiles[0];
    }
}

static void print_usage(const char *argv0) {
    printf("Usage: %s [--help]\n", argv0);
    printf("\n");
    printf("This app has no operational runtime CLI.\n");
    printf("Operational behavior is driven by configuration or upstream "
           "channels:\n");
    printf("  - GEISA_TEST_APP_* local development environment variables\n");
    printf("  - GEISA_TEST_APP_CONFIG_FILE local KEY=VALUE config input\n");
    printf("  - released upstream protobuf CONFIG and COMMAND channels\n");
    printf("\n");
    printf(
        "Current local config keys include mode, test, reporting_enabled,\n");
    printf("reporting_interval_seconds, reporting_message_types, "
           "payload_size_bytes,\n");
    printf("test_duration_ms, test_target_bytes, test_chunk_bytes,\n");
    printf("cpu_intensity_percent, burst_count, burst_interval_ms, "
           "quota_interval_ms,\n");
    printf("quota_max_attempts, payload_pattern, message_count, and "
           "expected_status.\n");
    printf("Runtime bindings such as HOST, PORT, and USERID remain separate "
           "from app config.\n");
    printf("OFFLINE_PREVIEW and REQUEST_ID_PREFIX remain separate from app "
           "config.\n");
}

static int parse_args(int argc, char **argv) {
    int i;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 1;
        }

        fprintf(stderr,
                "unsupported runtime process argument: %s\n"
                "use configuration or upstream command channels instead of app "
                "CLI controls\n",
                argv[i]);
        return -1;
    }

    return 0;
}

static int load_local_config_file(const char *path,
                                  struct app_options *options) {
    FILE *f;
    char line[4096];

    if (path == NULL || path[0] == '\0') {
        return 0;
    }

    f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "failed to read config file %s: %s\n", path,
                strerror(errno));
        return -1;
    }

    while (fgets(line, sizeof(line), f)) {
        char *eq = strchr(line, '=');
        char *key;
        char *value;

        if (!eq) {
            continue;
        }
        *eq = '\0';
        key = line;
        value = eq + 1;
        trim(key);
        trim(value);
        if (key[0] == '\0' || key[0] == '#') {
            continue;
        }
        if (strncmp(key, "GEISA_TEST_APP_", 15) == 0) {
            if (apply_runtime_binding_kv(options, key + 15, value) != 0) {
                fprintf(stderr, "unsupported runtime binding in %s: %s\n", path,
                        key);
                fclose(f);
                return -1;
            }
            continue;
        }
        if (apply_runtime_binding_kv(options, key, value) == 0) {
            continue;
        }
        if (apply_config_kv(options, key, value) != 0) {
            fprintf(stderr, "unsupported config key in %s: %s\n", path, key);
            fclose(f);
            return -1;
        }
    }

    fclose(f);
    return 0;
}

static unsigned current_payload_size(const struct app_options *options) {
    if (options == NULL || options->payload_size_bytes <= 0) {
        return 0U;
    }
    return (unsigned)options->payload_size_bytes;
}

static int build_payload_bytes(const struct app_options *options,
                               const struct app_message_profile *profile,
                               unsigned char **out_data, size_t *out_len,
                               size_t *resolved_size) {
    unsigned generated_size = options->operational_mode == APP_MODE_PAYLOAD_SIZE
                                  ? current_payload_size(options)
                                  : 0U;

    *out_data = NULL;
    *out_len = 0;
    *resolved_size = 0;

    if (generated_size > 0U) {
        if (geisa_app_message_build_payload(
                generated_size, options->payload_pattern, "application/json",
                out_data, out_len) != 0) {
            return -1;
        }
        *resolved_size = *out_len;
        return 0;
    }

    {
        const char *payload =
            profile != NULL ? profile->payload_json : DEFAULT_PAYLOAD_JSON;
        *out_len = strlen(payload);
        *out_data = (unsigned char *)malloc(*out_len + 1U);
        if (!*out_data) {
            return -1;
        }
        memcpy(*out_data, payload, *out_len + 1U);
        *resolved_size = *out_len;
        return 0;
    }
}

static const char *find_json_value_start(const char *json, const char *key,
                                         char *quote_expected) {
    static char pattern[128];
    char *key_loc;
    char *colon;

    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    key_loc = strstr((char *)json, pattern);
    if (!key_loc) {
        return NULL;
    }

    colon = strchr(key_loc + strlen(pattern), ':');
    if (!colon) {
        return NULL;
    }

    colon++;
    while (*colon == ' ' || *colon == '\t' || *colon == '\n' ||
           *colon == '\r') {
        colon++;
    }

    *quote_expected = (*colon == '"') ? 1 : 0;
    return colon;
}

static int json_extract_string(const char *json, const char *key, char *out,
                               size_t out_sz) {
    char quote = 0;
    const char *start = find_json_value_start(json, key, &quote);
    const char *end;
    size_t len;

    if (!start || !quote) {
        return -1;
    }

    start++;
    end = strchr(start, '"');
    if (!end) {
        return -1;
    }

    len = (size_t)(end - start);
    if (len >= out_sz) {
        len = out_sz - 1;
    }
    memcpy(out, start, len);
    out[len] = '\0';
    return 0;
}

static void skip_json_ws(const char **cursor) {
    while (**cursor == ' ' || **cursor == '\t' || **cursor == '\r' ||
           **cursor == '\n') {
        (*cursor)++;
    }
}

static int parse_json_string_token(const char **cursor, char *out,
                                   size_t out_sz) {
    size_t len = 0;
    const char *p = *cursor;

    skip_json_ws(&p);
    if (*p != '"') {
        return -1;
    }
    p++;
    while (*p != '\0' && *p != '"') {
        if (*p == '\\') {
            p++;
            if (*p == '\0') {
                return -1;
            }
        }
        if (len + 1 >= out_sz) {
            return -1;
        }
        out[len++] = *p++;
    }
    if (*p != '"') {
        return -1;
    }
    out[len] = '\0';
    p++;
    *cursor = p;
    return 0;
}

static int parse_json_bool_token(const char **cursor, int *out_value) {
    const char *p = *cursor;

    skip_json_ws(&p);
    if (strncmp(p, "true", 4) == 0) {
        *out_value = 1;
        *cursor = p + 4;
        return 0;
    }
    if (strncmp(p, "false", 5) == 0) {
        *out_value = 0;
        *cursor = p + 5;
        return 0;
    }
    return -1;
}

static int parse_json_int_token(const char **cursor, int min_value,
                                int max_value, int *out_value) {
    const char *p = *cursor;
    char *end = NULL;
    long value;

    skip_json_ws(&p);
    value = strtol(p, &end, 10);
    if (end == p) {
        return -1;
    }
    if (value < min_value || value > max_value) {
        return -1;
    }
    *out_value = (int)value;
    *cursor = end;
    return 0;
}

static int parse_json_message_types_array(const char **cursor,
                                          unsigned *mask_out, char *csv_out,
                                          size_t csv_out_sz) {
    const char *p = *cursor;
    unsigned mask = 0U;
    int first = 1;
    size_t csv_len = 0;

    skip_json_ws(&p);
    if (*p != '[') {
        return -1;
    }
    p++;
    skip_json_ws(&p);
    while (*p != '\0' && *p != ']') {
        char item[64];
        unsigned item_mask;
        size_t item_len;

        if (parse_json_string_token(&p, item, sizeof(item)) != 0) {
            return -1;
        }
        if (!reporting_message_type_is_supported(item)) {
            return -1;
        }
        item_mask = reporting_message_type_mask_for_name(item);
        if ((mask & item_mask) == 0U) {
            item_len = strlen(item);
            if (!first) {
                if (csv_len + 1 >= csv_out_sz) {
                    return -1;
                }
                csv_out[csv_len++] = ',';
            }
            if (csv_len + item_len >= csv_out_sz) {
                return -1;
            }
            memcpy(csv_out + csv_len, item, item_len);
            csv_len += item_len;
            mask |= item_mask;
            first = 0;
        }
        skip_json_ws(&p);
        if (*p == ',') {
            p++;
            skip_json_ws(&p);
            continue;
        }
        if (*p != ']') {
            return -1;
        }
    }
    if (*p != ']') {
        return -1;
    }
    p++;
    if (mask == 0U) {
        return -1;
    }
    csv_out[csv_len] = '\0';
    *mask_out = mask;
    *cursor = p;
    return 0;
}

static int config_key_mask_for_name(const char *value, unsigned *mask_out) {
    if (!value || !mask_out) {
        return -1;
    }
    if (strcmp(value, "reporting_enabled") == 0) {
        *mask_out = CONFIG_UPDATE_REPORTING_ENABLED;
        return 0;
    }
    if (strcmp(value, "reporting_interval_seconds") == 0) {
        *mask_out = CONFIG_UPDATE_REPORTING_INTERVAL_SECONDS;
        return 0;
    }
    if (strcmp(value, "reporting_message_types") == 0) {
        *mask_out = CONFIG_UPDATE_REPORTING_MESSAGE_TYPES;
        return 0;
    }
    if (strcmp(value, "payload_size_bytes") == 0) {
        *mask_out = CONFIG_UPDATE_PAYLOAD_SIZE_BYTES;
        return 0;
    }
    if (strcmp(value, "mode") == 0) {
        *mask_out = CONFIG_UPDATE_MODE;
        return 0;
    }
    if (strcmp(value, "test") == 0) {
        *mask_out = CONFIG_UPDATE_TEST;
        return 0;
    }
    if (strcmp(value, "burst_count") == 0) {
        *mask_out = CONFIG_UPDATE_BURST_COUNT;
        return 0;
    }
    if (strcmp(value, "burst_interval_ms") == 0) {
        *mask_out = CONFIG_UPDATE_BURST_INTERVAL_MS;
        return 0;
    }
    if (strcmp(value, "quota_max_attempts") == 0) {
        *mask_out = CONFIG_UPDATE_MAX_ATTEMPTS;
        return 0;
    }
    if (strcmp(value, "quota_interval_ms") == 0) {
        *mask_out = CONFIG_UPDATE_QUOTA_INTERVAL_MS;
        return 0;
    }
    if (strcmp(value, "payload_pattern") == 0) {
        *mask_out = CONFIG_UPDATE_PAYLOAD_PATTERN;
        return 0;
    }
    if (strcmp(value, "message_count") == 0) {
        *mask_out = CONFIG_UPDATE_MESSAGE_COUNT;
        return 0;
    }
    if (strcmp(value, "expected_status") == 0) {
        *mask_out = CONFIG_UPDATE_EXPECTED_STATUS;
        return 0;
    }
    if (strcmp(value, "test_duration_ms") == 0) {
        *mask_out = CONFIG_UPDATE_DURATION_MS;
        return 0;
    }
    if (strcmp(value, "test_target_bytes") == 0) {
        *mask_out = CONFIG_UPDATE_TARGET_BYTES;
        return 0;
    }
    if (strcmp(value, "test_chunk_bytes") == 0) {
        *mask_out = CONFIG_UPDATE_CHUNK_BYTES;
        return 0;
    }
    if (strcmp(value, "cpu_intensity_percent") == 0) {
        *mask_out = CONFIG_UPDATE_INTENSITY_PERCENT;
        return 0;
    }
    return -1;
}

static int parse_config_update_value(const char *key, const char **cursor,
                                     struct config_update *update,
                                     unsigned *seen_mask, char *error,
                                     size_t error_sz) {
    unsigned key_mask = 0U;

    if (config_key_mask_for_name(key, &key_mask) != 0) {
        snprintf(error, error_sz, "unsupported config key: %s", key);
        return -1;
    }
    if ((*seen_mask & key_mask) != 0U) {
        snprintf(error, error_sz, "duplicate config key: %s", key);
        return -1;
    }

    if (key_mask == CONFIG_UPDATE_REPORTING_ENABLED) {
        if (parse_json_bool_token(cursor, &update->reporting_enabled) != 0) {
            snprintf(error, error_sz, "reporting_enabled must be boolean");
            return -1;
        }
    } else if (key_mask == CONFIG_UPDATE_REPORTING_INTERVAL_SECONDS) {
        if (parse_json_int_token(cursor, 60, 86400,
                                 &update->reporting_interval_seconds) != 0) {
            snprintf(error, error_sz,
                     "reporting_interval_seconds must be an integer between 60 "
                     "and 86400");
            return -1;
        }
    } else if (key_mask == CONFIG_UPDATE_REPORTING_MESSAGE_TYPES) {
        if (parse_json_message_types_array(
                cursor, &update->reporting_message_types_mask,
                update->reporting_message_types,
                sizeof(update->reporting_message_types)) != 0) {
            snprintf(error, error_sz,
                     "reporting_message_types must be a non-empty array of "
                     "EVENT, ALARM, APP_DATA, or TELEMETRY");
            return -1;
        }
    } else if (key_mask == CONFIG_UPDATE_PAYLOAD_SIZE_BYTES) {
        if (parse_json_int_token(cursor, 0, MAX_PAYLOAD_BYTES,
                                 &update->payload_size_bytes) != 0) {
            snprintf(
                error, error_sz,
                "payload_size_bytes must be an integer between 0 and 65536");
            return -1;
        }
    } else if (key_mask == CONFIG_UPDATE_MODE) {
        char mode_name[16];
        if (parse_json_string_token(cursor, mode_name, sizeof(mode_name)) !=
                0 ||
            (strcmp(mode_name, "normal") != 0 &&
             strcmp(mode_name, "test") != 0)) {
            snprintf(error, error_sz, "mode must be normal or test");
            return -1;
        }
        update->application_mode = strcmp(mode_name, "test") == 0
                                       ? APP_RUN_MODE_TEST
                                       : APP_RUN_MODE_NORMAL;
    } else if (key_mask == CONFIG_UPDATE_TEST) {
        if (parse_json_string_token(cursor, update->test_name,
                                    sizeof(update->test_name)) != 0 ||
            test_mode_from_string(update->test_name) < 0) {
            snprintf(error, error_sz,
                     "test must be cpu, memory, persistent_storage, "
                     "transient_storage, burst, quota, payload, lee_all, "
                     "api_all, or all");
            return -1;
        }
    } else if (key_mask == CONFIG_UPDATE_BURST_COUNT) {
        if (parse_json_int_token(cursor, 1, 1000, &update->burst_count) != 0) {
            snprintf(error, error_sz,
                     "burst_count must be an integer between 1 and 1000");
            return -1;
        }
    } else if (key_mask == CONFIG_UPDATE_BURST_INTERVAL_MS) {
        if (parse_json_int_token(cursor, 0, 60000,
                                 &update->burst_interval_ms) != 0) {
            snprintf(
                error, error_sz,
                "burst_interval_ms must be an integer between 0 and 60000");
            return -1;
        }
    } else if (key_mask == CONFIG_UPDATE_MAX_ATTEMPTS) {
        if (parse_json_int_token(cursor, 1, 100000, &update->max_attempts) !=
            0) {
            snprintf(
                error, error_sz,
                "quota_max_attempts must be an integer between 1 and 100000");
            return -1;
        }
    } else if (key_mask == CONFIG_UPDATE_QUOTA_INTERVAL_MS) {
        if (parse_json_int_token(cursor, 0, 60000,
                                 &update->quota_interval_ms) != 0) {
            snprintf(
                error, error_sz,
                "quota_interval_ms must be an integer between 0 and 60000");
            return -1;
        }
    } else if (key_mask == CONFIG_UPDATE_PAYLOAD_PATTERN) {
        if (parse_json_string_token(cursor, update->payload_pattern,
                                    sizeof(update->payload_pattern)) != 0 ||
            (strcmp(update->payload_pattern, "alpha") != 0 &&
             strcmp(update->payload_pattern, "zero") != 0 &&
             strcmp(update->payload_pattern, "index") != 0)) {
            snprintf(error, error_sz,
                     "payload_pattern must be alpha, zero, or index");
            return -1;
        }
    } else if (key_mask == CONFIG_UPDATE_MESSAGE_COUNT) {
        if (parse_json_int_token(cursor, 0, 1000000, &update->message_count) !=
            0) {
            snprintf(error, error_sz,
                     "message_count must be an integer between 0 and 1000000");
            return -1;
        }
    } else if (key_mask == CONFIG_UPDATE_EXPECTED_STATUS) {
        if (parse_json_string_token(cursor, update->expected_status,
                                    sizeof(update->expected_status)) != 0 ||
            update->expected_status[0] == '\0') {
            snprintf(error, error_sz,
                     "expected_status must be a non-empty string");
            return -1;
        }
    } else if (key_mask == CONFIG_UPDATE_DURATION_MS) {
        if (parse_json_int_token(cursor, 1, MAX_PROBE_DURATION_MS,
                                 &update->duration_ms) != 0) {
            snprintf(error, error_sz, "test_duration_ms out of range");
            return -1;
        }
    } else if (key_mask == CONFIG_UPDATE_TARGET_BYTES) {
        if (parse_json_int_token(cursor, 1, MAX_PROBE_TARGET_BYTES,
                                 &update->target_bytes) != 0) {
            snprintf(error, error_sz, "test_target_bytes out of range");
            return -1;
        }
    } else if (key_mask == CONFIG_UPDATE_CHUNK_BYTES) {
        if (parse_json_int_token(cursor, 1, MAX_PROBE_CHUNK_BYTES,
                                 &update->chunk_bytes) != 0) {
            snprintf(error, error_sz, "test_chunk_bytes out of range");
            return -1;
        }
    } else if (key_mask == CONFIG_UPDATE_INTENSITY_PERCENT) {
        if (parse_json_int_token(cursor, 1, 100, &update->intensity_percent) !=
            0) {
            snprintf(error, error_sz, "cpu_intensity_percent out of range");
            return -1;
        }
    }

    *seen_mask |= key_mask;
    return 0;
}

static int parse_config_update_object(const char **cursor,
                                      struct config_update *update, char *error,
                                      size_t error_sz) {
    unsigned seen_mask = 0U;

    memset(update, 0, sizeof(*update));
    skip_json_ws(cursor);
    if (**cursor != '{') {
        snprintf(error, error_sz, "CONFIG values must be a JSON object");
        return -1;
    }
    (*cursor)++;
    skip_json_ws(cursor);
    if (**cursor == '}') {
        snprintf(error, error_sz, "CONFIG values must not be empty");
        return -1;
    }

    while (**cursor != '\0') {
        char key[128];

        if (parse_json_string_token(cursor, key, sizeof(key)) != 0) {
            snprintf(error, error_sz, "CONFIG payload contains an invalid key");
            return -1;
        }
        skip_json_ws(cursor);
        if (**cursor != ':') {
            snprintf(error, error_sz,
                     "CONFIG payload is missing ':' after key %s", key);
            return -1;
        }
        (*cursor)++;
        skip_json_ws(cursor);

        if (parse_config_update_value(key, cursor, update, &seen_mask, error,
                                      error_sz) != 0) {
            return -1;
        }

        skip_json_ws(cursor);
        if (**cursor == ',') {
            (*cursor)++;
            skip_json_ws(cursor);
            continue;
        }
        if (**cursor == '}') {
            (*cursor)++;
            break;
        }
        snprintf(error, error_sz, "CONFIG payload has invalid JSON syntax");
        return -1;
    }

    skip_json_ws(cursor);
    if (seen_mask == 0U) {
        snprintf(error, error_sz, "CONFIG values must not be empty");
        return -1;
    }
    update->present_mask = seen_mask;
    return 0;
}

static int parse_config_keys_array(const char **cursor, unsigned *mask_out,
                                   char *error, size_t error_sz) {
    unsigned mask = 0U;
    const char *p = *cursor;

    skip_json_ws(&p);
    if (*p != '[') {
        snprintf(error, error_sz,
                 "CONFIG keys must be an array of config key names");
        return -1;
    }
    p++;
    skip_json_ws(&p);
    while (*p != '\0' && *p != ']') {
        char key[128];
        unsigned key_mask = 0U;

        if (parse_json_string_token(&p, key, sizeof(key)) != 0) {
            snprintf(error, error_sz, "CONFIG keys must contain only strings");
            return -1;
        }
        if (config_key_mask_for_name(key, &key_mask) != 0) {
            snprintf(error, error_sz,
                     "unsupported config key in keys array: %s", key);
            return -1;
        }
        mask |= key_mask;
        skip_json_ws(&p);
        if (*p == ',') {
            p++;
            skip_json_ws(&p);
            continue;
        }
        if (*p != ']') {
            snprintf(error, error_sz,
                     "CONFIG keys array has invalid JSON syntax");
            return -1;
        }
    }
    if (*p != ']') {
        snprintf(error, error_sz, "CONFIG keys array is not terminated");
        return -1;
    }
    p++;
    *cursor = p;
    *mask_out = mask;
    return 0;
}

static void apply_config_update(struct app_options *options,
                                const struct config_update *update) {
    if ((update->present_mask & CONFIG_UPDATE_REPORTING_ENABLED) != 0U) {
        options->reporting_enabled = update->reporting_enabled;
    }
    if ((update->present_mask & CONFIG_UPDATE_REPORTING_INTERVAL_SECONDS) !=
        0U) {
        options->reporting_interval_seconds =
            update->reporting_interval_seconds;
    }
    if ((update->present_mask & CONFIG_UPDATE_REPORTING_MESSAGE_TYPES) != 0U) {
        options->reporting_message_types_mask =
            update->reporting_message_types_mask;
        snprintf(options->reporting_message_types,
                 sizeof(options->reporting_message_types), "%s",
                 update->reporting_message_types);
    }
    if ((update->present_mask & CONFIG_UPDATE_PAYLOAD_SIZE_BYTES) != 0U) {
        options->payload_size_bytes = update->payload_size_bytes;
    }
    if ((update->present_mask & CONFIG_UPDATE_MODE) != 0U) {
        options->application_mode = update->application_mode;
        if (options->application_mode == APP_RUN_MODE_NORMAL) {
            options->operational_mode = APP_MODE_NORMAL;
        }
    }
    if ((update->present_mask & CONFIG_UPDATE_TEST) != 0U) {
        snprintf(options->test_name, sizeof(options->test_name), "%s",
                 update->test_name);
        options->operational_mode = test_mode_from_string(options->test_name);
    }
    if ((update->present_mask & CONFIG_UPDATE_BURST_COUNT) != 0U) {
        options->burst_count = update->burst_count;
    }
    if ((update->present_mask & CONFIG_UPDATE_BURST_INTERVAL_MS) != 0U) {
        options->burst_interval_ms = update->burst_interval_ms;
    }
    if ((update->present_mask & CONFIG_UPDATE_MAX_ATTEMPTS) != 0U) {
        options->max_attempts = update->max_attempts;
    }
    if ((update->present_mask & CONFIG_UPDATE_QUOTA_INTERVAL_MS) != 0U) {
        options->quota_interval_ms = update->quota_interval_ms;
    }
    if ((update->present_mask & CONFIG_UPDATE_PAYLOAD_PATTERN) != 0U) {
        snprintf(options->payload_pattern, sizeof(options->payload_pattern),
                 "%s", update->payload_pattern);
    }
    if ((update->present_mask & CONFIG_UPDATE_MESSAGE_COUNT) != 0U) {
        options->message_count = update->message_count;
    }
    if ((update->present_mask & CONFIG_UPDATE_EXPECTED_STATUS) != 0U) {
        snprintf(options->expected_status, sizeof(options->expected_status),
                 "%s", update->expected_status);
    }
    if ((update->present_mask & CONFIG_UPDATE_DURATION_MS) != 0U) {
        options->duration_ms = update->duration_ms;
    }
    if ((update->present_mask & CONFIG_UPDATE_TARGET_BYTES) != 0U) {
        options->target_bytes = update->target_bytes;
    }
    if ((update->present_mask & CONFIG_UPDATE_CHUNK_BYTES) != 0U) {
        options->chunk_bytes = update->chunk_bytes;
    }
    if ((update->present_mask & CONFIG_UPDATE_INTENSITY_PERCENT) != 0U) {
        options->intensity_percent = update->intensity_percent;
    }
}

static int parse_config_request_payload(const char *json,
                                        struct config_request *request,
                                        char *error, size_t error_sz) {
    /* Reject unknown or repeated keys instead of hiding bad test input. */
    const char *cursor = json;
    int operation_seen = 0;
    int values_seen = 0;
    int keys_seen = 0;
    int legacy_set = 0;

    memset(request, 0, sizeof(*request));
    request->operation = CONFIG_OPERATION_SET;

    skip_json_ws(&cursor);
    if (*cursor != '{') {
        snprintf(error, error_sz, "CONFIG payload must be a JSON object");
        return -1;
    }
    cursor++;
    skip_json_ws(&cursor);
    if (*cursor == '}') {
        snprintf(error, error_sz, "CONFIG payload must not be empty");
        return -1;
    }

    while (*cursor != '\0') {
        char key[128];

        if (parse_json_string_token(&cursor, key, sizeof(key)) != 0) {
            snprintf(error, error_sz, "CONFIG payload contains an invalid key");
            return -1;
        }
        skip_json_ws(&cursor);
        if (*cursor != ':') {
            snprintf(error, error_sz,
                     "CONFIG payload is missing ':' after key %s", key);
            return -1;
        }
        cursor++;
        skip_json_ws(&cursor);

        if (strcmp(key, "operation") == 0) {
            char operation_name[64];
            if (operation_seen ||
                parse_json_string_token(&cursor, operation_name,
                                        sizeof(operation_name)) != 0) {
                snprintf(error, error_sz, "operation must be a string");
                return -1;
            }
            if (strcmp(operation_name, "set_configuration") == 0) {
                request->operation = CONFIG_OPERATION_SET;
            } else if (strcmp(operation_name, "get_effective_configuration") ==
                       0) {
                request->operation = CONFIG_OPERATION_GET_EFFECTIVE;
            } else {
                snprintf(error, error_sz,
                         "operation must be set_configuration or "
                         "get_effective_configuration");
                return -1;
            }
            operation_seen = 1;
        } else if (strcmp(key, "values") == 0) {
            if (values_seen) {
                snprintf(error, error_sz, "duplicate CONFIG values object");
                return -1;
            }
            if (parse_config_update_object(&cursor, &request->update, error,
                                           error_sz) != 0) {
                return -1;
            }
            values_seen = 1;
        } else if (strcmp(key, "keys") == 0) {
            if (keys_seen) {
                snprintf(error, error_sz, "duplicate CONFIG keys array");
                return -1;
            }
            if (parse_config_keys_array(&cursor, &request->requested_keys_mask,
                                        error, error_sz) != 0) {
                return -1;
            }
            request->subset_requested = 1;
            keys_seen = 1;
        } else {
            if (operation_seen || values_seen || keys_seen) {
                snprintf(error, error_sz,
                         "unsupported CONFIG top-level key: %s", key);
                return -1;
            }
            if (parse_config_update_value(key, &cursor, &request->update,
                                          &request->update.present_mask, error,
                                          error_sz) != 0) {
                return -1;
            }
            legacy_set = 1;
        }

        skip_json_ws(&cursor);
        if (*cursor == ',') {
            cursor++;
            skip_json_ws(&cursor);
            continue;
        }
        if (*cursor == '}') {
            cursor++;
            break;
        }
        snprintf(error, error_sz, "CONFIG payload has invalid JSON syntax");
        return -1;
    }

    skip_json_ws(&cursor);
    if (*cursor != '\0') {
        snprintf(error, error_sz, "CONFIG payload has trailing content");
        return -1;
    }

    if (!operation_seen && !legacy_set && !values_seen && !keys_seen) {
        snprintf(error, error_sz, "CONFIG payload must not be empty");
        return -1;
    }

    if (legacy_set && (operation_seen || values_seen || keys_seen)) {
        snprintf(error, error_sz,
                 "legacy CONFIG payload keys cannot be combined with "
                 "operation, values, or keys");
        return -1;
    }

    if (request->operation == CONFIG_OPERATION_SET) {
        if (request->update.present_mask == 0U) {
            snprintf(
                error, error_sz,
                "set_configuration requires non-empty values or config keys");
            return -1;
        }
        if (keys_seen) {
            snprintf(error, error_sz,
                     "set_configuration does not accept keys array");
            return -1;
        }
    } else if (request->operation == CONFIG_OPERATION_GET_EFFECTIVE) {
        if (request->update.present_mask != 0U) {
            snprintf(
                error, error_sz,
                "get_effective_configuration does not accept config values");
            return -1;
        }
    }

    return 0;
}

static void append_json_string(char *buffer, size_t buffer_sz, size_t *offset,
                               const char *text) {
    const unsigned char *cursor = (const unsigned char *)(text ? text : "");

    if (*offset >= buffer_sz) {
        return;
    }
    if (*offset + 1U < buffer_sz) {
        buffer[(*offset)++] = '"';
    }
    while (*cursor != '\0' && *offset + 2U < buffer_sz) {
        unsigned char ch = *cursor++;
        if (ch == '"' || ch == '\\') {
            buffer[(*offset)++] = '\\';
            buffer[(*offset)++] = (char)ch;
        } else if (ch == '\n') {
            buffer[(*offset)++] = '\\';
            buffer[(*offset)++] = 'n';
        } else if (ch == '\r') {
            buffer[(*offset)++] = '\\';
            buffer[(*offset)++] = 'r';
        } else if (ch == '\t') {
            buffer[(*offset)++] = '\\';
            buffer[(*offset)++] = 't';
        } else {
            buffer[(*offset)++] = (char)ch;
        }
    }
    if (*offset + 1U < buffer_sz) {
        buffer[(*offset)++] = '"';
    }
    if (*offset < buffer_sz) {
        buffer[*offset] = '\0';
    }
}

static void append_json_key_prefix(char *buffer, size_t buffer_sz,
                                   size_t *offset, int *first,
                                   const char *key) {
    if (!*first && *offset + 1U < buffer_sz) {
        buffer[(*offset)++] = ',';
    }
    *first = 0;
    append_json_string(buffer, buffer_sz, offset, key);
    if (*offset + 1U < buffer_sz) {
        buffer[(*offset)++] = ':';
    }
    if (*offset < buffer_sz) {
        buffer[*offset] = '\0';
    }
}

static void append_reporting_message_types_json(char *buffer, size_t buffer_sz,
                                                size_t *offset, unsigned mask) {
    static const struct {
        const char *name;
        unsigned mask;
    } items[] = {
        {"EVENT", REPORTING_MESSAGE_TYPE_EVENT_MASK},
        {"ALARM", REPORTING_MESSAGE_TYPE_ALARM_MASK},
        {"APP_DATA", REPORTING_MESSAGE_TYPE_APP_DATA_MASK},
        {"TELEMETRY", REPORTING_MESSAGE_TYPE_TELEMETRY_MASK},
    };
    size_t i;
    int first = 1;

    if (*offset + 1U < buffer_sz) {
        buffer[(*offset)++] = '[';
    }
    for (i = 0; i < sizeof(items) / sizeof(items[0]); i++) {
        if ((mask & items[i].mask) == 0U) {
            continue;
        }
        if (!first && *offset + 1U < buffer_sz) {
            buffer[(*offset)++] = ',';
        }
        first = 0;
        append_json_string(buffer, buffer_sz, offset, items[i].name);
    }
    if (*offset + 1U < buffer_sz) {
        buffer[(*offset)++] = ']';
    }
    if (*offset < buffer_sz) {
        buffer[*offset] = '\0';
    }
}

static void build_effective_config_json(const struct app_options *options,
                                        unsigned key_mask, char *buffer,
                                        size_t buffer_sz) {
    size_t offset = 0U;
    int first = 1;

    if (key_mask == 0U) {
        key_mask =
            CONFIG_UPDATE_REPORTING_ENABLED |
            CONFIG_UPDATE_REPORTING_INTERVAL_SECONDS |
            CONFIG_UPDATE_REPORTING_MESSAGE_TYPES |
            CONFIG_UPDATE_PAYLOAD_SIZE_BYTES | CONFIG_UPDATE_MODE |
            CONFIG_UPDATE_TEST | CONFIG_UPDATE_BURST_COUNT |
            CONFIG_UPDATE_BURST_INTERVAL_MS | CONFIG_UPDATE_MAX_ATTEMPTS |
            CONFIG_UPDATE_QUOTA_INTERVAL_MS | CONFIG_UPDATE_PAYLOAD_PATTERN |
            CONFIG_UPDATE_MESSAGE_COUNT | CONFIG_UPDATE_EXPECTED_STATUS |
            CONFIG_UPDATE_DURATION_MS | CONFIG_UPDATE_TARGET_BYTES |
            CONFIG_UPDATE_CHUNK_BYTES | CONFIG_UPDATE_INTENSITY_PERCENT;
    }

    if (buffer_sz == 0U) {
        return;
    }
    buffer[0] = '\0';
    buffer[offset++] = '{';
    buffer[offset] = '\0';

    if ((key_mask & CONFIG_UPDATE_REPORTING_ENABLED) != 0U) {
        append_json_key_prefix(buffer, buffer_sz, &offset, &first,
                               "reporting_enabled");
        offset +=
            (size_t)snprintf(buffer + offset, buffer_sz - offset, "%s",
                             options->reporting_enabled ? "true" : "false");
    }
    if ((key_mask & CONFIG_UPDATE_REPORTING_INTERVAL_SECONDS) != 0U) {
        append_json_key_prefix(buffer, buffer_sz, &offset, &first,
                               "reporting_interval_seconds");
        offset += (size_t)snprintf(buffer + offset, buffer_sz - offset, "%d",
                                   options->reporting_interval_seconds);
    }
    if ((key_mask & CONFIG_UPDATE_REPORTING_MESSAGE_TYPES) != 0U) {
        append_json_key_prefix(buffer, buffer_sz, &offset, &first,
                               "reporting_message_types");
        append_reporting_message_types_json(
            buffer, buffer_sz, &offset, options->reporting_message_types_mask);
    }
    if ((key_mask & CONFIG_UPDATE_PAYLOAD_SIZE_BYTES) != 0U) {
        append_json_key_prefix(buffer, buffer_sz, &offset, &first,
                               "payload_size_bytes");
        offset += (size_t)snprintf(buffer + offset, buffer_sz - offset, "%d",
                                   options->payload_size_bytes);
    }
    if ((key_mask & CONFIG_UPDATE_MODE) != 0U) {
        append_json_key_prefix(buffer, buffer_sz, &offset, &first, "mode");
        append_json_string(
            buffer, buffer_sz, &offset,
            application_mode_to_string(options->application_mode));
    }
    if ((key_mask & CONFIG_UPDATE_TEST) != 0U) {
        append_json_key_prefix(buffer, buffer_sz, &offset, &first, "test");
        append_json_string(buffer, buffer_sz, &offset, options->test_name);
    }
    if ((key_mask & CONFIG_UPDATE_BURST_COUNT) != 0U) {
        append_json_key_prefix(buffer, buffer_sz, &offset, &first,
                               "burst_count");
        offset += (size_t)snprintf(buffer + offset, buffer_sz - offset, "%d",
                                   options->burst_count);
    }
    if ((key_mask & CONFIG_UPDATE_BURST_INTERVAL_MS) != 0U) {
        append_json_key_prefix(buffer, buffer_sz, &offset, &first,
                               "burst_interval_ms");
        offset += (size_t)snprintf(buffer + offset, buffer_sz - offset, "%d",
                                   options->burst_interval_ms);
    }
    if ((key_mask & CONFIG_UPDATE_MAX_ATTEMPTS) != 0U) {
        append_json_key_prefix(buffer, buffer_sz, &offset, &first,
                               "quota_max_attempts");
        offset += (size_t)snprintf(buffer + offset, buffer_sz - offset, "%d",
                                   options->max_attempts);
    }
    if ((key_mask & CONFIG_UPDATE_PAYLOAD_PATTERN) != 0U) {
        append_json_key_prefix(buffer, buffer_sz, &offset, &first,
                               "payload_pattern");
        append_json_string(buffer, buffer_sz, &offset,
                           options->payload_pattern);
    }
    if ((key_mask & CONFIG_UPDATE_MESSAGE_COUNT) != 0U) {
        append_json_key_prefix(buffer, buffer_sz, &offset, &first,
                               "message_count");
        offset += (size_t)snprintf(buffer + offset, buffer_sz - offset, "%d",
                                   options->message_count);
    }
    if ((key_mask & CONFIG_UPDATE_EXPECTED_STATUS) != 0U) {
        append_json_key_prefix(buffer, buffer_sz, &offset, &first,
                               "expected_status");
        append_json_string(buffer, buffer_sz, &offset,
                           options->expected_status);
    }
    if ((key_mask & CONFIG_UPDATE_QUOTA_INTERVAL_MS) != 0U) {
        append_json_key_prefix(buffer, buffer_sz, &offset, &first,
                               "quota_interval_ms");
        offset += (size_t)snprintf(buffer + offset, buffer_sz - offset, "%d",
                                   options->quota_interval_ms);
    }
    if ((key_mask & CONFIG_UPDATE_DURATION_MS) != 0U) {
        append_json_key_prefix(buffer, buffer_sz, &offset, &first,
                               "test_duration_ms");
        offset += (size_t)snprintf(buffer + offset, buffer_sz - offset, "%d",
                                   options->duration_ms);
    }
    if ((key_mask & CONFIG_UPDATE_TARGET_BYTES) != 0U) {
        append_json_key_prefix(buffer, buffer_sz, &offset, &first,
                               "test_target_bytes");
        offset += (size_t)snprintf(buffer + offset, buffer_sz - offset, "%d",
                                   options->target_bytes);
    }
    if ((key_mask & CONFIG_UPDATE_CHUNK_BYTES) != 0U) {
        append_json_key_prefix(buffer, buffer_sz, &offset, &first,
                               "test_chunk_bytes");
        offset += (size_t)snprintf(buffer + offset, buffer_sz - offset, "%d",
                                   options->chunk_bytes);
    }
    if ((key_mask & CONFIG_UPDATE_INTENSITY_PERCENT) != 0U) {
        append_json_key_prefix(buffer, buffer_sz, &offset, &first,
                               "cpu_intensity_percent");
        offset += (size_t)snprintf(buffer + offset, buffer_sz - offset, "%d",
                                   options->intensity_percent);
    }

    if (offset + 1U < buffer_sz) {
        buffer[offset++] = '}';
    }
    if (offset < buffer_sz) {
        buffer[offset] = '\0';
    } else {
        buffer[buffer_sz - 1U] = '\0';
    }
}

static void build_requested_keys_json(unsigned key_mask, char *buffer,
                                      size_t buffer_sz) {
    static const struct {
        const char *name;
        unsigned mask;
    } items[] = {
        {"reporting_enabled", CONFIG_UPDATE_REPORTING_ENABLED},
        {"reporting_interval_seconds",
         CONFIG_UPDATE_REPORTING_INTERVAL_SECONDS},
        {"reporting_message_types", CONFIG_UPDATE_REPORTING_MESSAGE_TYPES},
        {"payload_size_bytes", CONFIG_UPDATE_PAYLOAD_SIZE_BYTES},
        {"mode", CONFIG_UPDATE_MODE},
        {"test", CONFIG_UPDATE_TEST},
        {"burst_count", CONFIG_UPDATE_BURST_COUNT},
        {"burst_interval_ms", CONFIG_UPDATE_BURST_INTERVAL_MS},
        {"quota_max_attempts", CONFIG_UPDATE_MAX_ATTEMPTS},
        {"quota_interval_ms", CONFIG_UPDATE_QUOTA_INTERVAL_MS},
        {"payload_pattern", CONFIG_UPDATE_PAYLOAD_PATTERN},
        {"message_count", CONFIG_UPDATE_MESSAGE_COUNT},
        {"expected_status", CONFIG_UPDATE_EXPECTED_STATUS},
        {"test_duration_ms", CONFIG_UPDATE_DURATION_MS},
        {"test_target_bytes", CONFIG_UPDATE_TARGET_BYTES},
        {"test_chunk_bytes", CONFIG_UPDATE_CHUNK_BYTES},
        {"cpu_intensity_percent", CONFIG_UPDATE_INTENSITY_PERCENT},
    };
    size_t offset = 0U;
    size_t i;
    int first = 1;

    if (buffer_sz == 0U) {
        return;
    }
    buffer[0] = '\0';
    buffer[offset++] = '[';
    buffer[offset] = '\0';
    for (i = 0; i < sizeof(items) / sizeof(items[0]); i++) {
        if ((key_mask & items[i].mask) == 0U) {
            continue;
        }
        if (!first && offset + 1U < buffer_sz) {
            buffer[offset++] = ',';
        }
        first = 0;
        append_json_string(buffer, buffer_sz, &offset, items[i].name);
    }
    if (offset + 1U < buffer_sz) {
        buffer[offset++] = ']';
    }
    if (offset < buffer_sz) {
        buffer[offset] = '\0';
    } else {
        buffer[buffer_sz - 1U] = '\0';
    }
}

static int status_is(const char *status, const char *expected) {
    return status != NULL && expected != NULL && strcmp(status, expected) == 0;
}

static void record_response_status(struct app_state *state,
                                   const char *status) {
    size_t i;
    for (i = 0; i < state->response_status_count; i++) {
        if (strcmp(state->response_status_counts[i].status, status) == 0) {
            state->response_status_counts[i].count++;
            return;
        }
    }
    if (state->response_status_count < MAX_RESPONSE_STATUS_COUNTS) {
        snprintf(
            state->response_status_counts[state->response_status_count].status,
            sizeof(state->response_status_counts[0].status), "%s", status);
        state->response_status_counts[state->response_status_count++].count =
            1U;
    }
}

static int write_json_string(FILE *output, const char *value) {
    const unsigned char *cursor =
        (const unsigned char *)(value == NULL ? "" : value);
    if (fputc('"', output) == EOF) return -1;
    while (*cursor != '\0') {
        unsigned char ch = *cursor++;
        switch (ch) {
        case '"':
            if (fputs("\\\"", output) == EOF) return -1;
            break;
        case '\\':
            if (fputs("\\\\", output) == EOF) return -1;
            break;
        case '\n':
            if (fputs("\\n", output) == EOF) return -1;
            break;
        case '\r':
            if (fputs("\\r", output) == EOF) return -1;
            break;
        case '\t':
            if (fputs("\\t", output) == EOF) return -1;
            break;
        default:
            if (ch < 0x20U) {
                if (fprintf(output, "\\u%04x", (unsigned)ch) < 0) return -1;
            } else if (fputc(ch, output) == EOF) {
                return -1;
            }
        }
    }
    return fputc('"', output) == EOF ? -1 : 0;
}

static int
publish_message_request(struct mosquitto *mosq, struct app_state *state,
                        const char *request_id, const char *message_type,
                        const char *priority, const char *content_type,
                        const unsigned char *payload_bytes, size_t payload_len,
                        size_t resolved_payload_size, int pending_kind) {
    unsigned char *request_payload;
    size_t request_payload_len = 0U;
    uint64_t timestamp_ms = now_ms();
    int rc;

    if (geisa_app_message_build_request(
            request_id, priority, message_type, timestamp_ms,
            (uint32_t)state->options.ttl_seconds, content_type, payload_bytes,
            payload_len, &request_payload, &request_payload_len) != 0) {
        return MOSQ_ERR_NOMEM;
    }

    state->pending_app_response = 1;
    state->pending_response_kind = pending_kind;
    snprintf(state->last_request_id, sizeof(state->last_request_id), "%s",
             request_id);
    state->response_seen = 0;
    state->pending_deadline_ms = 0U;

    rc = mosquitto_publish(mosq, NULL, state->cfg.app_message_request_topic,
                           (int)request_payload_len, request_payload, 1, false);
    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(
            stderr,
            "publish failed request_id=%s topic=%s message_type=%s error=%s\n",
            request_id, state->cfg.app_message_request_topic, message_type,
            mosquitto_strerror(rc));
        state->pending_app_response = 0;
        state->pending_response_kind = PENDING_KIND_NONE;
        state->last_request_id[0] = '\0';
        state->response_seen = 0;
        free(request_payload);
        return rc;
    }

    state->published_count++;
    state->pending_deadline_ms =
        now_ms() + (uint64_t)state->options.timeout_seconds * 1000ULL;
    if (pending_kind == PENDING_KIND_REPORT) {
        state->last_payload_size = resolved_payload_size;
    }

    printf("published app-message request_id=%s topic=%s message_type=%s "
           "priority=%s payload_bytes=%lu attempt=%u\n",
           request_id, state->cfg.app_message_request_topic, message_type,
           priority, (unsigned long)resolved_payload_size, state->send_attempt);

    free(request_payload);
    return MOSQ_ERR_SUCCESS;
}

static int publish_one_message(struct mosquitto *mosq, struct app_state *state,
                               const struct app_message_profile *profile) {
    const char *message_type =
        profile != NULL ? profile->message_type : "EVENT";
    const char *priority = profile != NULL
                               ? profile->priority
                               : "GEISA_APP_MESSAGE_PRIORITY_BEST_EFFORT";
    const char *content_type =
        profile != NULL ? profile->content_type : "application/json";
    unsigned char *payload_bytes = NULL;
    size_t payload_len = 0;
    size_t resolved_payload_size = 0;
    char request_id[192];
    int rc;

    state->send_attempt++;

    if (build_payload_bytes(&state->options, profile, &payload_bytes,
                            &payload_len, &resolved_payload_size) != 0) {
        fprintf(stderr, "failed to build payload bytes\n");
        return MOSQ_ERR_PAYLOAD_SIZE;
    }

    if (geisa_app_message_build_request_id(request_id, sizeof(request_id),
                                           state->options.request_id_prefix,
                                           state->send_attempt) != 0) {
        free(payload_bytes);
        return MOSQ_ERR_INVAL;
    }
    rc = publish_message_request(
        mosq, state, request_id, message_type, priority, content_type,
        payload_bytes, payload_len, resolved_payload_size, PENDING_KIND_REPORT);
    free(payload_bytes);
    return rc;
}

static void after_config_apply(struct app_state *state) {
    populate_reporting_profiles(state);
    state->cycle_remaining = 0U;
    state->reporting_profile_index = 0U;

    if (state->options.application_mode == APP_RUN_MODE_TEST) {
        state->next_send_ms = 0;
        return;
    }

    if (!state->options.reporting_enabled) {
        state->next_send_ms = 0;
        return;
    }

    if (state->discovery_completed && !state->pending_app_response) {
        begin_reporting_cycle(state);
        schedule_next_send(state, 0U);
    }
}

static int is_lee_probe_mode(int mode) {
    return mode >= APP_MODE_LEE_EXCEED_CPU &&
           mode <= APP_MODE_LEE_EXCEED_TRANSIENT_STORAGE;
}

static int validate_config_candidate(struct app_options *options, char *error,
                                     size_t error_sz) {
    if (options->application_mode == APP_RUN_MODE_TEST) {
        if (options->test_name[0] == '\0' ||
            test_mode_from_string(options->test_name) < 0) {
            snprintf(error, error_sz, "mode=test requires a supported test");
            return -1;
        }
        options->operational_mode = test_mode_from_string(options->test_name);
    } else {
        options->operational_mode = APP_MODE_NORMAL;
    }
    return 0;
}

static int apply_config_update_to_state(struct app_state *state,
                                        const struct config_update *update,
                                        char *error, size_t error_sz) {
    /* Validate first; rejection must leave active options unchanged. */
    struct app_options candidate = state->options;
    int test_changed;

    apply_config_update(&candidate, update);
    if (validate_config_candidate(&candidate, error, error_sz) != 0) {
        return -1;
    }
    test_changed =
        candidate.application_mode != state->options.application_mode ||
        strcmp(candidate.test_name, state->options.test_name) != 0;
    state->options = candidate;
    if (test_changed && state->options.application_mode == APP_RUN_MODE_TEST) {
        state->test_run_pending = 1;
        state->aggregate_index = 0U;
    }
    after_config_apply(state);
    return 0;
}

static int probe_write_file(struct probe_state *probe, const char *root) {
    char directory[512];
    char path[640];
    unsigned char *chunk;
    int fd;
    struct stat st;
    /* The test owns this subdirectory, not the caller-provided storage root. */
    snprintf(directory, sizeof(directory), "%s/geisa-test-probes", root);
    if (mkdir(directory, 0700) != 0 && errno != EEXIST) {
        snprintf(probe->error, sizeof(probe->error), "mkdir:%s",
                 strerror(errno));
        probe->failures++;
        return -1;
    }
    snprintf(path, sizeof(path), "%s/probe-%u.bin", directory,
             probe->invocations);
    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) {
        snprintf(probe->error, sizeof(probe->error), "open:%s",
                 strerror(errno));
        probe->failures++;
        goto cleanup_directory;
    }
    chunk = calloc(1, (size_t)probe->requested_chunk_bytes);
    if (!chunk) {
        snprintf(probe->error, sizeof(probe->error), "chunk:ENOMEM");
        probe->failures++;
        close(fd);
        goto cleanup_file;
    }
    while (probe->completed_bytes < (uint64_t)probe->requested_target_bytes &&
           !probe->cancel) {
        size_t remaining = (size_t)((uint64_t)probe->requested_target_bytes -
                                    probe->completed_bytes);
        size_t chunk_size = remaining < (size_t)probe->requested_chunk_bytes
                                ? remaining
                                : (size_t)probe->requested_chunk_bytes;
        ssize_t written;
        int write_errno;
        probe->attempted_bytes += chunk_size;
        errno = 0;
        written = write(fd, chunk, chunk_size);
        write_errno = errno;
        if (written <= 0) {
            snprintf(probe->error, sizeof(probe->error), "write:%s",
                     strerror(write_errno));
            probe->failures++;
            break;
        }
        probe->completed_bytes += (uint64_t)written;
        if ((size_t)written != chunk_size) {
            if (write_errno != 0) {
                snprintf(probe->error, sizeof(probe->error), "write_partial:%s",
                         strerror(write_errno));
            } else {
                snprintf(probe->error, sizeof(probe->error), "write_partial");
            }
            probe->failures++;
            break;
        }
    }
    if (!probe->cancel && fsync(fd) != 0) {
        snprintf(probe->error, sizeof(probe->error), "fsync:%s",
                 strerror(errno));
        probe->failures++;
    }
    if (fstat(fd, &st) == 0) {
        probe->touched_bytes = (uint64_t)st.st_size;
    } else {
        snprintf(probe->error, sizeof(probe->error), "stat:%s",
                 strerror(errno));
        probe->failures++;
    }
    close(fd);
    free(chunk);
cleanup_file:
    if (unlink(path) == 0 || errno == ENOENT) {
        probe->cleaned_up = 1;
    } else {
        if (probe->error[0] == '\0') {
            snprintf(probe->error, sizeof(probe->error), "unlink:%s",
                     strerror(errno));
        }
        probe->failures++;
    }
cleanup_directory:
    if (rmdir(directory) != 0 && errno != ENOENT && errno != ENOTEMPTY) {
        if (probe->error[0] == '\0') {
            snprintf(probe->error, sizeof(probe->error), "rmdir:%s",
                     strerror(errno));
        }
        probe->failures++;
    }
    return probe->failures ? -1 : 0;
}

static void *run_probe(void *userdata) {
    struct app_state *state = userdata;
    struct probe_state *probe = &state->probe;
    /* Isolate resource work so the MQTT loop can keep servicing controls. */
    uint64_t deadline = now_ms() + (uint64_t)probe->requested_duration_ms;
    clock_t cpu_start = clock();
    snprintf(probe->result, sizeof(probe->result), "running");
    if (probe->mode == APP_MODE_LEE_EXCEED_CPU) {
        volatile uint64_t accumulator = 0;
        while (!probe->cancel && now_ms() < deadline) {
            unsigned i;
            for (i = 0; i < 100000U; i++)
                accumulator = accumulator * 33U + i;
            probe->attempted_bytes += 100000U;
            if (probe->requested_intensity_percent < 100) {
                struct timespec pause = {
                    0, (long)(1000000U *
                              (100 - probe->requested_intensity_percent))};
                nanosleep(&pause, NULL);
            }
        }
        probe->completed_bytes = probe->attempted_bytes;
        (void)accumulator;
    } else if (probe->mode == APP_MODE_LEE_EXCEED_MEM) {
        unsigned char **chunks = NULL;
        size_t count = 0U;
        size_t capacity = 0U;

        while (!probe->cancel && probe->completed_bytes <
                                     (uint64_t)probe->requested_target_bytes) {
            size_t size = (size_t)probe->requested_chunk_bytes;
            unsigned char *chunk;
            uint64_t remaining = (uint64_t)probe->requested_target_bytes -
                                 probe->completed_bytes;

            if ((uint64_t)size > remaining) {
                size = (size_t)remaining;
            }
            probe->attempted_bytes += size;
            chunk = malloc(size);
            if (!chunk) {
                snprintf(probe->error, sizeof(probe->error),
                         "allocation:ENOMEM");
                probe->failures++;
                break;
            }
            memset(chunk, 0xA5, size);
            if (count == capacity) {
                size_t next = capacity ? capacity * 2U : 8U;
                unsigned char **expanded =
                    realloc(chunks, next * sizeof(*chunks));

                if (!expanded) {
                    free(chunk);
                    snprintf(probe->error, sizeof(probe->error),
                             "tracking:ENOMEM");
                    probe->failures++;
                    break;
                }
                chunks = expanded;
                capacity = next;
            }
            chunks[count++] = chunk;
            probe->completed_bytes += size;
            probe->touched_bytes += size;
        }
        while (!probe->cancel && now_ms() < deadline) {
            struct timespec pause = {0, 1000000L};
            nanosleep(&pause, NULL);
        }
        while (count > 0U) {
            free(chunks[--count]);
        }
        free(chunks);
        probe->cleaned_up = 1;
    } else if (probe->mode == APP_MODE_LEE_EXCEED_PERSISTENT_STORAGE ||
               probe->mode == APP_MODE_LEE_EXCEED_TRANSIENT_STORAGE) {
        probe_write_file(probe, probe->storage_dir);
    }
    probe->cpu_time_ms =
        (uint64_t)((clock() - cpu_start) * 1000 / CLOCKS_PER_SEC);
    probe->elapsed_ms = now_ms() - probe->started_ms;
    probe->interrupted = probe->cancel ? 1 : 0;
    if (probe->interrupted)
        snprintf(probe->result, sizeof(probe->result), "interrupted");
    else if (probe->failures)
        snprintf(probe->result, sizeof(probe->result), "failed");
    else
        snprintf(probe->result, sizeof(probe->result), "completed");
    probe->running = 0;
    probe->done = 1;
    return NULL;
}

static int start_selected_probe(struct app_state *state) {
    struct probe_state *probe = &state->probe;
    unsigned invocations;
    if (!is_lee_probe_mode(state->options.operational_mode) || probe->running)
        return -1;
    invocations = probe->invocations;
    memset(probe, 0, sizeof(*probe));
    probe->mode = state->options.operational_mode;
    probe->invocations = invocations + 1U;
    probe->requested_duration_ms = state->options.duration_ms;
    probe->requested_target_bytes = state->options.target_bytes;
    probe->requested_chunk_bytes = state->options.chunk_bytes;
    probe->requested_intensity_percent = state->options.intensity_percent;
    if (probe->mode == APP_MODE_LEE_EXCEED_PERSISTENT_STORAGE) {
        snprintf(probe->storage_dir, sizeof(probe->storage_dir), "%s",
                 state->options.persistent_storage_dir);
    } else if (probe->mode == APP_MODE_LEE_EXCEED_TRANSIENT_STORAGE) {
        snprintf(probe->storage_dir, sizeof(probe->storage_dir), "%s",
                 state->options.transient_storage_dir);
    }
    if (is_lee_probe_mode(probe->mode) &&
        (probe->mode == APP_MODE_LEE_EXCEED_PERSISTENT_STORAGE ||
         probe->mode == APP_MODE_LEE_EXCEED_TRANSIENT_STORAGE) &&
        probe->storage_dir[0] == '\0') {
        snprintf(probe->error, sizeof(probe->error),
                 "storage test directory is not configured");
        return -1;
    }
    probe->started_ms = now_ms();
    probe->running = 1;
    {
        int rc = pthread_create(&probe->thread, NULL, run_probe, state);
        if (rc != 0) {
            probe->running = 0;
            probe->done = 0;
            probe->failures++;
            snprintf(probe->error, sizeof(probe->error), "pthread_create:%s",
                     strerror(rc));
            snprintf(probe->result, sizeof(probe->result), "failed");
        }
        return rc;
    }
}

static void configure_aggregate(struct app_state *state, int mode) {
    /* Summary order is observable; keep aggregate execution deterministic. */
    state->aggregate_index = 0U;
    state->aggregate_count = 0U;
    state->aggregate_completed = 0U;
    state->aggregate_failed = 0;
    if (mode == APP_MODE_LEE_ALL) {
        state->aggregate_modes[state->aggregate_count++] =
            APP_MODE_LEE_EXCEED_CPU;
        state->aggregate_modes[state->aggregate_count++] =
            APP_MODE_LEE_EXCEED_MEM;
        state->aggregate_modes[state->aggregate_count++] =
            APP_MODE_LEE_EXCEED_PERSISTENT_STORAGE;
        state->aggregate_modes[state->aggregate_count++] =
            APP_MODE_LEE_EXCEED_TRANSIENT_STORAGE;
    } else if (mode == APP_MODE_API_ALL) {
        state->aggregate_modes[state->aggregate_count++] =
            APP_MODE_PAYLOAD_SIZE;
        state->aggregate_modes[state->aggregate_count++] = APP_MODE_BURST;
        state->aggregate_modes[state->aggregate_count++] =
            APP_MODE_QUOTA_EXCEED;
    } else if (mode == APP_MODE_ALL) {
        state->aggregate_modes[state->aggregate_count++] =
            APP_MODE_PAYLOAD_SIZE;
        state->aggregate_modes[state->aggregate_count++] = APP_MODE_BURST;
        state->aggregate_modes[state->aggregate_count++] =
            APP_MODE_LEE_EXCEED_CPU;
        state->aggregate_modes[state->aggregate_count++] =
            APP_MODE_LEE_EXCEED_MEM;
        state->aggregate_modes[state->aggregate_count++] =
            APP_MODE_LEE_EXCEED_PERSISTENT_STORAGE;
        state->aggregate_modes[state->aggregate_count++] =
            APP_MODE_LEE_EXCEED_TRANSIENT_STORAGE;
        state->aggregate_modes[state->aggregate_count++] =
            APP_MODE_QUOTA_EXCEED;
    } else {
        state->aggregate_modes[state->aggregate_count++] = mode;
    }
}

static void complete_current_test(struct app_state *state, int success) {
    state->aggregate_completed++;
    if (!success) state->aggregate_failed = 1;
    state->next_send_ms = 0U;
    if (state->aggregate_index + 1U < state->aggregate_count) {
        state->aggregate_index++;
        state->test_run_pending = 1;
    } else {
        state->success = state->aggregate_failed ? 0 : 1;
        state->test_run_pending = 0;
        state->aggregate_count = 0U;
    }
}

static void finalize_probe_completion(struct app_state *state) {
    if (state->shutdown_requested) {
        state->done = 1;
    } else if (state->options.application_mode == APP_RUN_MODE_TEST) {
        complete_current_test(state,
                              strcmp(state->probe.result, "failed") != 0);
    }
}

static void start_pending_test(struct app_state *state) {
    int selected;

    if (!state->test_run_pending ||
        state->options.application_mode != APP_RUN_MODE_TEST ||
        !state->discovery_completed || state->pending_app_response ||
        state->probe.running) {
        return;
    }
    selected = test_mode_from_string(state->options.test_name);
    if (selected < 0) {
        state->test_run_pending = 0;
        state->success = 0;
        return;
    }
    if (state->aggregate_count == 0U ||
        state->aggregate_index >= state->aggregate_count) {
        configure_aggregate(state, selected);
    }
    state->send_attempt = 0U;
    state->cycle_remaining = 0U;
    state->quota_rejected = 0;
    state->pending_response_kind = PENDING_KIND_NONE;
    state->options.operational_mode =
        state->aggregate_modes[state->aggregate_index];
    state->test_run_pending = 0;
    if (is_lee_probe_mode(state->options.operational_mode)) {
        if (start_selected_probe(state) != 0) {
            complete_current_test(state, 0);
        }
        return;
    }
    state->cycle_remaining = 0U;
    schedule_next_send(state, 0U);
}

static void publish_downstream_disposition(struct mosquitto *mosq,
                                           struct app_state *state,
                                           const char *request_id,
                                           const char *status,
                                           const char *detail) {
    unsigned char *payload = NULL;
    size_t payload_len = 0;
    if (!mosq) return;
    if (geisa_app_message_build_response(request_id, status, detail, now_ms(),
                                         &payload, &payload_len) == 0) {
        if (mosquitto_publish(
                mosq, NULL, state->cfg.downstream_message_response_topic,
                (int)payload_len, payload, 1, false) != MOSQ_ERR_SUCCESS) {
            state->downstream_response_publish_failures++;
        }
    } else {
        state->downstream_response_publish_failures++;
    }
    free(payload);
}

static void
handle_downstream_config(struct mosquitto *mosq, struct app_state *state,
                         const struct geisa_app_message_request_view *request) {
    struct config_request config_request;
    char *json;
    char error[256];
    /* Protobuf payloads are length-bounded, not NUL-terminated. */
    if (strcmp(request->content_type, "application/json") != 0 ||
        request->payload_len > 8192U ||
        !(json = malloc(request->payload_len + 1U))) {
        state->config_reject_count++;
        publish_downstream_disposition(mosq, state, request->request_id,
                                       "GEISA_APP_MESSAGE_STATUS_REJECTED",
                                       "invalid CONFIG payload");
        return;
    }
    memcpy(json, request->payload, request->payload_len);
    json[request->payload_len] = '\0';
    if (parse_config_request_payload(json, &config_request, error,
                                     sizeof(error)) != 0) {
        state->config_reject_count++;
        publish_downstream_disposition(mosq, state, request->request_id,
                                       "GEISA_APP_MESSAGE_STATUS_REJECTED",
                                       error);
    } else {
        if (config_request.operation == CONFIG_OPERATION_SET) {
            if (apply_config_update_to_state(state, &config_request.update,
                                             error, sizeof(error)) != 0) {
                state->config_reject_count++;
                publish_downstream_disposition(
                    mosq, state, request->request_id,
                    "GEISA_APP_MESSAGE_STATUS_REJECTED", error);
                free(json);
                return;
            }
            state->config_apply_count++;
        } else {
            char effective[2048];
            char requested_keys[512];
            build_effective_config_json(&state->options,
                                        config_request.requested_keys_mask,
                                        effective, sizeof(effective));
            build_requested_keys_json(config_request.requested_keys_mask,
                                      requested_keys, sizeof(requested_keys));
            printf("{\"geisa-test-effective-configuration\":%s,\"requested_"
                   "keys\":%s}\n",
                   effective, requested_keys);
        }
        publish_downstream_disposition(mosq, state, request->request_id,
                                       "GEISA_APP_MESSAGE_STATUS_ACCEPTED",
                                       config_request.operation ==
                                               CONFIG_OPERATION_SET
                                           ? "config_applied"
                                           : "effective_configuration");
    }
    free(json);
}

static void handle_downstream_command(
    struct mosquitto *mosq, struct app_state *state,
    const struct geisa_app_message_request_view *request) {
    char command[64];
    char payload[256];
    state->command_received_count++;
    if (strcmp(request->content_type, "application/json") != 0 ||
        request->payload_len >= sizeof(payload)) {
        state->command_rejected_count++;
        publish_downstream_disposition(mosq, state, request->request_id,
                                       "GEISA_APP_MESSAGE_STATUS_REJECTED",
                                       "invalid COMMAND payload");
        return;
    }
    memcpy(payload, request->payload, request->payload_len);
    payload[request->payload_len] = '\0';
    if (json_extract_string(payload, "command", command, sizeof(command)) !=
        0) {
        state->command_rejected_count++;
        publish_downstream_disposition(mosq, state, request->request_id,
                                       "GEISA_APP_MESSAGE_STATUS_REJECTED",
                                       "invalid COMMAND payload");
        return;
    }
    if (strcmp(command, "run_once") == 0) {
        if (state->options.application_mode == APP_RUN_MODE_TEST) {
            state->test_run_pending = 1;
            state->aggregate_index = 0U;
            state->aggregate_count = 0U;
        } else if (is_lee_probe_mode(state->options.operational_mode)) {
            if (start_selected_probe(state) != 0) {
                state->command_rejected_count++;
                publish_downstream_disposition(
                    mosq, state, request->request_id,
                    "GEISA_APP_MESSAGE_STATUS_REJECTED",
                    "probe unavailable or already running");
                return;
            }
        } else if (state->discovery_completed && !state->pending_app_response) {
            schedule_next_send(state, 0U);
        }
    } else if (strcmp(command, "report_counters") == 0) {
        printf("geisa-test command counters attempted=%u accepted=%u "
               "rejected=%u\n",
               state->send_attempt, state->accepted_count,
               state->rejected_count);
    } else if (strcmp(command, "reset_counters") == 0) {
        state->send_attempt = state->accepted_count = state->rejected_count =
            0U;
        if (!state->probe.running)
            memset(&state->probe, 0, sizeof(state->probe));
    } else {
        state->command_rejected_count++;
        publish_downstream_disposition(mosq, state, request->request_id,
                                       "GEISA_APP_MESSAGE_STATUS_REJECTED",
                                       "unsupported COMMAND");
        return;
    }
    state->command_accepted_count++;
    state->command_executed_count++;
    publish_downstream_disposition(mosq, state, request->request_id,
                                   "GEISA_APP_MESSAGE_STATUS_ACCEPTED",
                                   command);
}

static const struct app_message_profile *
next_reporting_profile(struct app_state *state) {
    if (state->reporting_profile_count == 0) {
        return NULL;
    }
    if (state->reporting_profile_index >= state->reporting_profile_count) {
        state->reporting_profile_index = 0U;
    }
    return state->reporting_profiles[state->reporting_profile_index++];
}

static void begin_reporting_cycle(struct app_state *state) {
    if (state->options.operational_mode == APP_MODE_QUOTA_EXCEED) {
        return;
    }
    state->reporting_profile_index = 0U;
    if (state->options.operational_mode == APP_MODE_NORMAL) {
        state->cycle_remaining = (unsigned)state->reporting_profile_count;
    } else if (state->options.operational_mode == APP_MODE_PAYLOAD_SIZE) {
        state->cycle_remaining = 1U;
    } else {
        state->cycle_remaining = (unsigned)state->options.burst_count;
    }
}

static void schedule_next_send_at(struct app_state *state, uint64_t now,
                                  uint64_t delay_ms) {
    state->next_send_ms = now + delay_ms;
}

static void schedule_next_send(struct app_state *state, uint64_t delay_ms) {
    schedule_next_send_at(state, now_ms(), delay_ms);
}

static uint64_t reporting_interval_ms(const struct app_options *options) {
    return (uint64_t)options->reporting_interval_seconds * 1000ULL;
}

static void schedule_next_reporting_cycle_at(struct app_state *state,
                                             uint64_t now) {
    begin_reporting_cycle(state);
    schedule_next_send_at(state, now, reporting_interval_ms(&state->options));
}

static int dispatch_next_message(struct mosquitto *mosq,
                                 struct app_state *state) {
    if (state->pending_app_response) {
        return MOSQ_ERR_SUCCESS;
    }

    if (state->options.application_mode != APP_RUN_MODE_TEST &&
        !state->options.reporting_enabled) {
        if (!state->options.stay_running) {
            state->done = 1;
            state->success = 1;
        }
        return MOSQ_ERR_SUCCESS;
    }

    if (state->options.stay_running &&
        state->options.operational_mode == APP_MODE_NORMAL &&
        state->platform_message_quota_known &&
        state->platform_message_quota_exhausted) {
        return MOSQ_ERR_SUCCESS;
    }

    if (state->options.operational_mode == APP_MODE_NORMAL &&
        state->options.message_count > 0) {
        if (state->send_attempt >= (unsigned)state->options.message_count) {
            if (state->options.application_mode == APP_RUN_MODE_TEST) {
                complete_current_test(state, 1);
            } else {
                state->done = 1;
                state->success = 1;
            }
            return MOSQ_ERR_SUCCESS;
        }
        return publish_one_message(mosq, state, next_reporting_profile(state));
    }

    if (state->options.operational_mode == APP_MODE_QUOTA_EXCEED) {
        if (state->quota_rejected) {
            if (!state->options.stay_running) {
                if (state->options.application_mode == APP_RUN_MODE_TEST) {
                    complete_current_test(state, 1);
                } else {
                    state->done = 1;
                    state->success = 1;
                }
            }
            return MOSQ_ERR_SUCCESS;
        }
        if (state->send_attempt >= (unsigned)state->options.max_attempts) {
            fprintf(stderr, "quota_exceed exhausted max_attempts without quota "
                            "rejection\n");
            if (state->options.application_mode == APP_RUN_MODE_TEST) {
                complete_current_test(state, 0);
            } else {
                state->done = 1;
                state->success = 0;
            }
            return MOSQ_ERR_SUCCESS;
        }
        return publish_one_message(mosq, state, next_reporting_profile(state));
    }

    if (state->cycle_remaining == 0U) {
        begin_reporting_cycle(state);
    }

    if (state->cycle_remaining == 0U) {
        if (state->options.application_mode == APP_RUN_MODE_TEST) {
            complete_current_test(state, 1);
        } else {
            state->done = 1;
            state->success = 1;
        }
        return MOSQ_ERR_SUCCESS;
    }

    state->cycle_remaining--;
    return publish_one_message(mosq, state, next_reporting_profile(state));
}

static void print_plan_preview(const struct mqtt_cfg *cfg,
                               const struct app_options *options) {
    size_t profile_count = 0;
    const struct app_message_profile *profiles =
        default_profiles(&profile_count);
    size_t i;

    printf("offline preview host=%s port=%d userid=%s mode=%s test=%s "
           "reporting_enabled=%s message_count=%d payload_pattern=%s "
           "expected_status=%s burst_count=%d burst_interval_ms=%d "
           "quota_interval_ms=%d quota_max_attempts=%d ttl_seconds=%d "
           "reporting_interval_seconds=%d request_id_prefix=%s\n",
           cfg->host, cfg->port, cfg->userid,
           application_mode_to_string(options->application_mode),
           options->test_name, options->reporting_enabled ? "true" : "false",
           options->message_count, options->payload_pattern,
           options->expected_status, options->burst_count,
           options->burst_interval_ms, options->quota_interval_ms,
           options->max_attempts, options->ttl_seconds,
           options->reporting_interval_seconds, options->request_id_prefix);
    printf("topics discovery_req=%s discovery_rsp=%s app_req=%s app_rsp=%s\n",
           cfg->discovery_request_topic, cfg->discovery_response_topic,
           cfg->app_message_request_topic, cfg->app_message_response_topic);

    if (options->reporting_message_types_mask != 0U) {
        printf("planned reporting message types:\n");
        for (i = 0; i < profile_count; i++) {
            unsigned mask =
                reporting_message_type_mask_for_name(profiles[i].message_type);
            if ((options->reporting_message_types_mask & mask) == 0U) {
                continue;
            }
            printf("  - %s priority=%s content_type=%s\n",
                   profiles[i].message_type, profiles[i].priority,
                   profiles[i].content_type);
        }
    } else {
        printf("planned payload_size_bytes=%d\n", options->payload_size_bytes);
    }
}

static int emit_machine_summary_to(FILE *output, const struct app_state *state,
                                   int final_pass,
                                   const char *completion_reason) {
    /* One JSON record lets callers parse stdout without scraping logs. */
    size_t i;

    fputs(
        "{\"geisa-test-summary\":{\"schema_version\":1,\"request_id_prefix\":",
        output);
    write_json_string(output, state->options.request_id_prefix);
    fprintf(
        output,
        ",\"message_count\":%d,\"payload_size_bytes\":%d,\"payload_pattern\":",
        state->options.message_count, state->options.payload_size_bytes);
    write_json_string(output, state->options.payload_pattern);
    fprintf(
        output,
        ",\"burst_interval_ms\":%d,\"timeout_seconds\":%d,\"expected_status\":",
        state->options.burst_interval_ms, state->options.timeout_seconds);
    write_json_string(output, state->options.expected_status);
    fprintf(output,
            ",\"attempted\":%u,\"published\":%u,\"accepted\":%u,"
            "\"rejected\":%u,\"other\":%u,\"timed_out\":%u,"
            "\"duplicate_responses\":%u,\"unmatched_responses\":%u,"
            "\"malformed_responses\":%u,\"status_mismatches\":%u,"
            "\"final_pass\":%s,\"completion_reason\":",
            state->send_attempt, state->published_count, state->accepted_count,
            state->rejected_count, state->other_count, state->timeout_count,
            state->duplicate_response_count, state->unmatched_response_count,
            state->malformed_response_count, state->status_mismatch_count,
            final_pass ? "true" : "false");
    write_json_string(output, completion_reason);
    fprintf(output,
            ",\"lifecycle\":{\"state\":%d,\"running_reports\":%u,"
            "\"shutdown_reports\":%u,\"send_status_commands\":%u,"
            "\"shutdown_commands\":%u,\"control_malformed\":%u},"
            "\"manifest\":{\"requests\":%u,\"successes\":%u,"
            "\"failures\":%u,\"malformed\":%u,\"unmatched\":%u,"
            "\"timeouts\":%u}",
            state->lifecycle_state, state->lifecycle_running_reports,
            state->lifecycle_shutdown_reports,
            state->lifecycle_send_status_commands,
            state->lifecycle_shutdown_commands,
            state->lifecycle_control_malformed, state->manifest_requests,
            state->manifest_successes, state->manifest_failures,
            state->manifest_malformed, state->manifest_unmatched,
            state->manifest_timeouts);
    fprintf(output,
            ",\"config\":{\"applied\":%u,\"rejected\":%u,\"response_publish_"
            "failures\":%u,\"mode\":",
            state->config_apply_count, state->config_reject_count,
            state->downstream_response_publish_failures);
    write_json_string(
        output, application_mode_to_string(state->options.application_mode));
    fputs(",\"test\":", output);
    write_json_string(output, state->options.test_name);
    fprintf(output,
            "},\"command\":{\"received\":%u,\"accepted\":%u,\"rejected\":%u,"
            "\"executed\":%u,\"downstream_malformed\":%u}",
            state->command_received_count, state->command_accepted_count,
            state->command_rejected_count, state->command_executed_count,
            state->downstream_malformed_count);
    fprintf(output,
            ",\"test_execution\":{\"aggregate_completed\":%u,\"aggregate_"
            "failed\":%s,\"pending\":%s}",
            state->aggregate_completed,
            state->aggregate_failed ? "true" : "false",
            state->test_run_pending ? "true" : "false");
    fprintf(output,
            ",\"platform_message_quota\":{\"known\":%s,"
            "\"exhausted\":%s,\"today_used\":%llu,"
            "\"today_limit\":%llu,\"today_remaining\":%llu,"
            "\"next_reset_ms\":%llu,\"rejections\":%u}",
            state->platform_message_quota_known ? "true" : "false",
            state->platform_message_quota_exhausted ? "true" : "false",
            (unsigned long long)state->platform_message_today_used,
            (unsigned long long)state->platform_message_today_limit,
            (unsigned long long)state->platform_message_today_remaining,
            (unsigned long long)state->platform_message_next_reset_ms,
            state->quota_rejected ? 1U : 0U);
    fprintf(output, ",\"probe\":{\"test\":");
    write_json_string(output, test_mode_to_string(state->probe.mode));
    fprintf(output,
            ",\"invocations\":%u,\"running\":%s,\"duration_ms\":%d,"
            "\"target_bytes\":%d,\"chunk_bytes\":%d,"
            "\"intensity_percent\":%d,\"elapsed_ms\":%llu,"
            "\"attempted_bytes\":%llu,\"completed_bytes\":%llu,"
            "\"touched_bytes\":%llu,\"cpu_time_ms\":%llu,"
            "\"failures\":%u,\"interrupted\":%s,"
            "\"cleaned_up\":%s,\"result\":",
            state->probe.invocations, state->probe.running ? "true" : "false",
            state->probe.requested_duration_ms,
            state->probe.requested_target_bytes,
            state->probe.requested_chunk_bytes,
            state->probe.requested_intensity_percent,
            (unsigned long long)state->probe.elapsed_ms,
            (unsigned long long)state->probe.attempted_bytes,
            (unsigned long long)state->probe.completed_bytes,
            (unsigned long long)state->probe.touched_bytes,
            (unsigned long long)state->probe.cpu_time_ms, state->probe.failures,
            state->probe.interrupted ? "true" : "false",
            state->probe.cleaned_up ? "true" : "false");
    write_json_string(output, state->probe.result);
    fputs(",\"error\":", output);
    write_json_string(output, state->probe.error);
    fputs("}", output);
    fputs(",\"rejected_by_status\":{", output);
    {
        int first_rejected = 1;

        for (i = 0; i < state->response_status_count; i++) {
            if (status_is(state->response_status_counts[i].status,
                          STATUS_ACCEPTED)) {
                continue;
            }
            if (!first_rejected) {
                fputc(',', output);
            }
            first_rejected = 0;
            write_json_string(output, state->response_status_counts[i].status);
            fprintf(output, ":%u", state->response_status_counts[i].count);
        }
    }
    fputs("}}}\n", output);
    return ferror(output) ? -1 : 0;
}

static int emit_machine_summary(const struct app_state *state, int final_pass,
                                const char *completion_reason) {
    if (emit_machine_summary_to(stdout, state, final_pass, completion_reason) !=
            0 ||
        fflush(stdout) != 0) {
        return -1;
    }
    return 0;
}

static int expire_pending_response(struct app_state *state,
                                   uint64_t current_ms) {
    if (!state->pending_app_response || state->pending_deadline_ms == 0U ||
        current_ms <= state->pending_deadline_ms) {
        return 0;
    }
    fprintf(stderr, "timed out waiting for app-message response\n");
    state->pending_app_response = 0;
    state->pending_response_kind = PENDING_KIND_NONE;
    state->timeout_count++;
    state->success = 0;
    return -1;
}

static int queue_startup_subscription(struct mosquitto *mosq,
                                      struct app_state *state, unsigned slot,
                                      const char *topic, int qos) {
    int mid = 0;
    int rc;

    if (slot >= STARTUP_SUBSCRIPTION_COUNT) {
        return MOSQ_ERR_INVAL;
    }
    rc = mosquitto_subscribe(mosq, &mid, topic, qos);
    if (rc == MOSQ_ERR_SUCCESS) {
        state->startup_subscription_mids[slot] = mid;
        state->startup_subscription_requested_qos[slot] = qos;
    }
    return rc;
}

static int record_startup_subscription_ack(struct app_state *state, int mid,
                                           int qos_count,
                                           const int *granted_qos) {
    unsigned slot;

    for (slot = 0U; slot < STARTUP_SUBSCRIPTION_COUNT; slot++) {
        if (state->startup_subscription_mids[slot] == mid) break;
    }
    if (slot == STARTUP_SUBSCRIPTION_COUNT ||
        state->startup_subscription_acked[slot]) {
        return 0;
    }
    /* MQTT allows a lower grant; a grant above the requested QoS is invalid. */
    if (qos_count != 1 || granted_qos == NULL || granted_qos[0] < 0 ||
        granted_qos[0] == 0x80 ||
        granted_qos[0] > state->startup_subscription_requested_qos[slot]) {
        return -1;
    }
    state->startup_subscription_acked[slot] = 1;
    state->startup_subscription_acks++;
    return state->startup_subscription_acks == STARTUP_SUBSCRIPTION_COUNT ? 1
                                                                          : 0;
}

static int publish_startup_transactions(struct mosquitto *mosq,
                                        struct app_state *state) {
    struct geisa_proto_bytes discovery_request = {0};
    struct geisa_proto_bytes manifest_request = {0};
    int err;

    /* RUNNING waits until every control and response subscription is live. */
    if (publish_lifecycle_status(mosq, state, GEISA_PROTO_APP_STATUS_RUNNING) !=
        MOSQ_ERR_SUCCESS) {
        return -1;
    }
    if (geisa_proto_encode_discovery_request(&discovery_request) != 0) {
        return -1;
    }
    err = mosquitto_publish(mosq, NULL, state->cfg.discovery_request_topic,
                            (int)discovery_request.len, discovery_request.data,
                            1, false);
    geisa_proto_bytes_free(&discovery_request);
    if (err != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "publish failed: %s\n", mosquitto_strerror(err));
        return -1;
    }
    state->discovery_deadline_ms =
        now_ms() + (uint64_t)state->options.timeout_seconds * 1000ULL;
    printf("published protobuf discovery request on %s\n",
           state->cfg.discovery_request_topic);

    if (geisa_proto_encode_manifest_request(&manifest_request) != 0) {
        return -1;
    }
    state->manifest_pending = 1;
    state->manifest_deadline_ms = 0U;
    err = mosquitto_publish(mosq, NULL, state->manifest_request_topic,
                            (int)manifest_request.len, manifest_request.data, 1,
                            false);
    geisa_proto_bytes_free(&manifest_request);
    if (err != MOSQ_ERR_SUCCESS) {
        state->manifest_pending = 0;
        return -1;
    }
    state->manifest_requests++;
    state->manifest_deadline_ms =
        now_ms() + (uint64_t)state->options.timeout_seconds * 1000ULL;
    return 0;
}

static void on_connect(struct mosquitto *mosq, void *userdata, int rc) {
    struct app_state *state = (struct app_state *)userdata;
    int err;

    if (rc != 0) {
        fprintf(stderr, "mqtt connect failed: rc=%d\n", rc);
        state->done = 1;
        state->success = 0;
        return;
    }

    state->connected = 1;
    state->startup_subscription_acks = 0U;
    state->startup_subscriptions_ready = 0;
    memset(state->startup_subscription_mids, 0,
           sizeof(state->startup_subscription_mids));
    memset(state->startup_subscription_acked, 0,
           sizeof(state->startup_subscription_acked));
    derive_topic(state->platform_status_topic,
                 sizeof(state->platform_status_topic),
                 PLATFORM_STATUS_TOPIC_PREFIX, state->cfg.userid);
    derive_topic(state->app_status_topic, sizeof(state->app_status_topic),
                 APP_STATUS_TOPIC_PREFIX, state->cfg.userid);
    derive_topic(state->manifest_request_topic,
                 sizeof(state->manifest_request_topic),
                 MANIFEST_REQUEST_TOPIC_PREFIX, state->cfg.userid);
    derive_topic(state->manifest_response_topic,
                 sizeof(state->manifest_response_topic),
                 MANIFEST_RESPONSE_TOPIC_PREFIX, state->cfg.userid);
    if (queue_startup_subscription(mosq, state, 0U,
                                   GLOBAL_PLATFORM_STATUS_TOPIC,
                                   0) != MOSQ_ERR_SUCCESS ||
        queue_startup_subscription(mosq, state, 1U,
                                   state->platform_status_topic,
                                   0) != MOSQ_ERR_SUCCESS ||
        queue_startup_subscription(mosq, state, 2U,
                                   state->manifest_response_topic,
                                   1) != MOSQ_ERR_SUCCESS ||
        queue_startup_subscription(mosq, state, 3U,
                                   state->cfg.downstream_message_request_topic,
                                   1) != MOSQ_ERR_SUCCESS) {
        state->done = 1;
        state->success = 0;
        return;
    }
    err = queue_startup_subscription(mosq, state, 4U,
                                     state->cfg.discovery_response_topic, 1);
    if (err != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "subscribe failed: %s\n",
                state->cfg.discovery_response_topic);
        state->done = 1;
        state->success = 0;
        return;
    }

    err = queue_startup_subscription(mosq, state, 5U,
                                     state->cfg.app_message_response_topic, 1);
    if (err != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "subscribe failed: %s\n",
                state->cfg.app_message_response_topic);
        state->done = 1;
        state->success = 0;
        return;
    }
}

static void on_subscribe(struct mosquitto *mosq, void *userdata, int mid,
                         int qos_count, const int *granted_qos) {
    struct app_state *state = (struct app_state *)userdata;
    int ready;

    ready = record_startup_subscription_ack(state, mid, qos_count, granted_qos);
    if (ready < 0) {
        fprintf(stderr, "startup subscription rejected\n");
        state->done = 1;
        state->success = 0;
        return;
    }
    if (ready == 1) {
        state->startup_subscriptions_ready = 1;
        printf("mqtt startup subscriptions ready count=%u\n",
               state->startup_subscription_acks);
        if (publish_startup_transactions(mosq, state) != 0) {
            state->done = 1;
            state->success = 0;
        }
    }
}

static void on_message(struct mosquitto *mosq, void *userdata,
                       const struct mosquitto_message *msg) {
    struct app_state *state = (struct app_state *)userdata;
    char request_id[160];
    char status[160];
    char status_text[256];
    int has_status_text;

    (void)mosq;

    /* MQTT topics route control; CONFIG and COMMAND handlers parse the opaque
     * application/json payloads after the protobuf envelope is decoded. */

    if (!msg || !msg->topic) {
        return;
    }
    if (msg->payloadlen < 0 || (msg->payloadlen > 0 && !msg->payload)) {
        if (strcmp(msg->topic, state->platform_status_topic) == 0 ||
            strcmp(msg->topic, GLOBAL_PLATFORM_STATUS_TOPIC) == 0) {
            state->lifecycle_control_malformed++;
        } else if (strcmp(msg->topic,
                          state->cfg.downstream_message_request_topic) == 0) {
            state->downstream_malformed_count++;
        } else if (strcmp(msg->topic, state->manifest_response_topic) == 0) {
            state->manifest_malformed++;
            state->manifest_failures++;
        } else if (strcmp(msg->topic, state->cfg.app_message_response_topic) ==
                   0) {
            state->malformed_response_count++;
        }
        return;
    }

    /* Route by subscribed topic before decoding; content type alone does not
     * distinguish control messages from other application JSON. */
    if (strcmp(msg->topic, state->platform_status_topic) == 0 ||
        strcmp(msg->topic, GLOBAL_PLATFORM_STATUS_TOPIC) == 0) {
        struct geisa_proto_platform_to_app_status control;

        if (msg->payloadlen < 0 ||
            geisa_proto_decode_platform_to_app_status(
                msg->payload, (size_t)msg->payloadlen, &control) != 0) {
            state->lifecycle_control_malformed++;
            return;
        }
        if (control.cmd_send_status) {
            state->lifecycle_send_status_commands++;
            publish_lifecycle_status(mosq, state,
                                     GEISA_PROTO_APP_STATUS_RUNNING);
        }
        if (control.cmd_clear_pii) {
            /* No PII is retained; leave unrelated app state alone. */
            (void)publish_lifecycle_status(mosq, state,
                                           GEISA_PROTO_APP_STATUS_CLEARED_PII);
        }
        state->platform_message_quota_known = control.conn_msg.today_limit > 0U;
        state->platform_message_today_used = control.conn_msg.today_used;
        state->platform_message_today_limit = control.conn_msg.today_limit;
        state->platform_message_today_remaining =
            control.conn_msg.today_remaining;
        state->platform_message_next_reset_ms =
            control.conn_msg.next_daily_reset_ms;
        state->platform_message_quota_exhausted =
            state->platform_message_quota_known &&
            control.conn_msg.today_remaining == 0U;
        if (!state->platform_message_quota_exhausted &&
            state->options.reporting_enabled && state->options.stay_running &&
            state->options.operational_mode == APP_MODE_NORMAL &&
            state->discovery_completed && !state->pending_app_response &&
            state->next_send_ms == 0U) {
            begin_reporting_cycle(state);
            schedule_next_send(state, 0U);
        }
        if (control.cmd_shut_down) {
            state->lifecycle_shutdown_commands++;
            publish_lifecycle_status(mosq, state,
                                     GEISA_PROTO_APP_STATUS_SHUTTING_DOWN);
            state->shutdown_requested = 1;
            state->probe.cancel = 1;
            if (!state->probe.running) {
                state->done = 1;
            }
        }
        return;
    }
    if (strcmp(msg->topic, state->manifest_response_topic) == 0) {
        unsigned status_code = 0U;
        const unsigned char *manifest;
        size_t manifest_len;
        if (!state->manifest_pending) {
            state->manifest_unmatched++;
            return;
        }
        state->manifest_pending = 0;
        state->manifest_deadline_ms = 0;
        if (decode_manifest_response(msg->payload, (size_t)msg->payloadlen,
                                     &status_code, &manifest,
                                     &manifest_len) != 0) {
            state->manifest_malformed++;
            state->manifest_failures++;
        } else if (status_code == 1U) {
            state->manifest_successes++;
            printf("received deployed manifest on %s:\n%.*s\n", msg->topic,
                   (int)manifest_len, manifest ? (const char *)manifest : "");
        } else {
            state->manifest_failures++;
        }
        return;
    }
    if (strcmp(msg->topic, state->cfg.downstream_message_request_topic) == 0) {
        struct geisa_app_message_request_view request;
        if (geisa_app_message_decode_request(
                (const unsigned char *)msg->payload, (size_t)msg->payloadlen,
                &request) != 0) {
            state->downstream_malformed_count++;
            return;
        }
        if (request.message_type == 1U) {
            handle_downstream_config(mosq, state, &request);
        } else if (request.message_type == 2U) {
            handle_downstream_command(mosq, state, &request);
        } else {
            state->downstream_malformed_count++;
            publish_downstream_disposition(
                mosq, state, request.request_id,
                "GEISA_APP_MESSAGE_STATUS_REJECTED",
                "unsupported downstream message_type");
        }
        return;
    }

    if (strcmp(msg->topic, state->cfg.discovery_response_topic) == 0) {
        if (!geisa_platform_discovery_response_is_success(
                (const unsigned char *)msg->payload, (size_t)msg->payloadlen)) {
            fprintf(stderr,
                    "received invalid protobuf discovery response on %s\n",
                    state->cfg.discovery_response_topic);
            state->success = 0;
            return;
        }
        printf("received successful protobuf discovery response on %s\n",
               state->cfg.discovery_response_topic);
        state->discovery_completed = 1;
        state->success = 1;

        if (state->options.application_mode == APP_RUN_MODE_TEST) {
            state->test_run_pending = 1;
            return;
        }
        if (!state->options.reporting_enabled) {
            if (!state->options.stay_running) {
                state->done = 1;
            }
            return;
        }
        if (state->options.operational_mode != APP_MODE_QUOTA_EXCEED) {
            begin_reporting_cycle(state);
        }
        schedule_next_send(state, 0U);
        return;
    }

    if (strcmp(msg->topic, state->cfg.app_message_response_topic) != 0) {
        return;
    }

    request_id[0] = '\0';
    status[0] = '\0';
    status_text[0] = '\0';

    if (geisa_app_message_parse_response(
            (const unsigned char *)msg->payload, (size_t)msg->payloadlen,
            request_id, sizeof(request_id), status, sizeof(status), status_text,
            sizeof(status_text)) == GEISA_APP_MESSAGE_RESPONSE_MALFORMED) {
        state->malformed_response_count++;
        return;
    }
    if (strcmp(request_id, state->last_request_id) != 0) {
        state->unmatched_response_count++;
        return;
    }
    if (state->response_seen) {
        state->duplicate_response_count++;
        return;
    }
    state->response_seen = 1;
    record_response_status(state, status);
    has_status_text = status_text[0] != '\0';

    if (has_status_text) {
        printf("received app-message response request_id=%s status=%s "
               "status_text=%s\n",
               request_id, status, status_text);
    } else {
        printf("received app-message response request_id=%s status=%s\n",
               request_id, status);
    }

    state->pending_app_response = 0;
    state->pending_response_kind = PENDING_KIND_NONE;

    /* Quota expects rejection; other tests compare each response with the
     * configured status and count mismatches separately from the outcome. */
    if (state->options.operational_mode != APP_MODE_QUOTA_EXCEED &&
        !status_is(status, state->options.expected_status)) {
        state->status_mismatch_count++;
    }

    if (!status_is(status, STATUS_ACCEPTED) &&
        status_is(status, state->options.expected_status)) {
        state->rejected_count++;
        if (state->options.operational_mode == APP_MODE_NORMAL &&
            state->options.message_count > 0 &&
            state->send_attempt < (unsigned)state->options.message_count) {
            schedule_next_send(state,
                               (uint64_t)state->options.burst_interval_ms);
        } else {
            if (state->options.application_mode == APP_RUN_MODE_TEST) {
                complete_current_test(state, 1);
            } else {
                state->done = 1;
                state->success = 1;
            }
        }
        return;
    }

    if (status_is(status, STATUS_ACCEPTED)) {
        state->accepted_count++;

        if (state->options.operational_mode == APP_MODE_NORMAL &&
            state->options.message_count > 0) {
            if (state->send_attempt >= (unsigned)state->options.message_count) {
                if (state->options.application_mode == APP_RUN_MODE_TEST) {
                    complete_current_test(state, 1);
                } else {
                    state->done = 1;
                    state->success = 1;
                }
            } else {
                schedule_next_send(state,
                                   (uint64_t)state->options.burst_interval_ms);
            }
            return;
        }

        if (state->options.operational_mode == APP_MODE_QUOTA_EXCEED) {
            schedule_next_send(state,
                               (uint64_t)state->options.quota_interval_ms);
            return;
        }

        if (state->cycle_remaining > 0U) {
            schedule_next_send(state,
                               (uint64_t)state->options.burst_interval_ms);
            return;
        }

        if (state->options.application_mode == APP_RUN_MODE_TEST) {
            complete_current_test(state, 1);
            return;
        }

        if (state->options.stay_running) {
            schedule_next_reporting_cycle_at(state, now_ms());
            return;
        }

        state->done = 1;
        state->success = 1;
        return;
    }

    if (status_is(status, STATUS_QUOTA_EXCEEDED)) {
        state->rejected_count++;
        state->quota_rejected = 1;
        state->platform_message_quota_known = 1;
        state->platform_message_quota_exhausted = 1;
        state->platform_message_today_remaining = 0U;
        if (state->options.stay_running &&
            state->options.operational_mode == APP_MODE_NORMAL) {
            return;
        }
        if (state->options.application_mode == APP_RUN_MODE_TEST) {
            complete_current_test(
                state,
                state->options.operational_mode == APP_MODE_QUOTA_EXCEED ||
                    status_is(status, state->options.expected_status));
        } else {
            state->done = 1;
            state->success = status_is(status, state->options.expected_status);
        }
        return;
    }

    state->other_count++;
    state->done = 1;
    state->success = 0;
    fprintf(stderr, "unexpected app-message response status=%s\n", status);
}

int main(int argc, char **argv) {
    struct app_state state;
    struct mosquitto *mosq;
    int parse_rc;
    int loop_rc = 0;
    int final_pass;
    const char *completion_reason;

    memset(&state, 0, sizeof(state));
    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    load_defaults(&state.cfg);
    load_mqtt_config(&state.cfg);
    load_default_options(&state.options);

    if (state.options.config_file[0] != '\0' &&
        load_local_config_file(state.options.config_file, &state.options) !=
            0) {
        return 1;
    }

    parse_rc = parse_args(argc, argv);
    if (parse_rc > 0) {
        return 0;
    }
    if (parse_rc < 0) {
        print_usage(argv[0]);
        return 2;
    }

    if (validate_options(&state.options) != 0) {
        return 1;
    }
    populate_reporting_profiles(&state);

    apply_mqtt_overrides(&state.cfg, &state.options);

    printf("starting geisa-test host=%s port=%d userid=%s client_id=%s mode=%s "
           "test=%s reporting_enabled=%s message_count=%d payload_pattern=%s "
           "expected_status=%s burst_count=%d burst_interval_ms=%d "
           "quota_interval_ms=%d quota_max_attempts=%d "
           "reporting_interval_seconds=%d ttl_seconds=%d "
           "reporting_message_types=%s\n",
           state.cfg.host, state.cfg.port, state.cfg.userid,
           state.cfg.client_id,
           application_mode_to_string(state.options.application_mode),
           state.options.test_name,
           state.options.reporting_enabled ? "true" : "false",
           state.options.message_count, state.options.payload_pattern,
           state.options.expected_status, state.options.burst_count,
           state.options.burst_interval_ms, state.options.quota_interval_ms,
           state.options.max_attempts, state.options.reporting_interval_seconds,
           state.options.ttl_seconds, state.options.reporting_message_types);
    printf("topics discovery_req=%s discovery_rsp=%s app_req=%s app_rsp=%s\n",
           state.cfg.discovery_request_topic,
           state.cfg.discovery_response_topic,
           state.cfg.app_message_request_topic,
           state.cfg.app_message_response_topic);

    if (state.options.offline_preview) {
        print_plan_preview(&state.cfg, &state.options);
        return 0;
    }

    mosquitto_lib_init();
    mosq = mosquitto_new(state.cfg.client_id, true, &state);
    if (!mosq) {
        fprintf(stderr, "failed to create mosquitto client\n");
        mosquitto_lib_cleanup();
        return 1;
    }

    if (mosquitto_int_option(mosq, MOSQ_OPT_PROTOCOL_VERSION,
                             MQTT_PROTOCOL_V5) != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "failed to configure MQTT v5\n");
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return 1;
    }

    if (state.cfg.has_password) {
        mosquitto_username_pw_set(mosq, state.cfg.userid, state.cfg.password);
    } else {
        mosquitto_username_pw_set(mosq, state.cfg.userid, NULL);
    }

    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_subscribe_callback_set(mosq, on_subscribe);
    mosquitto_message_callback_set(mosq, on_message);

    if (mosquitto_connect(mosq, state.cfg.host, state.cfg.port, 30) !=
        MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "failed to connect to %s:%d\n", state.cfg.host,
                state.cfg.port);
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return 1;
    }

    while (!g_stop && !state.done) {
        uint64_t current_ms = now_ms();

        loop_rc = mosquitto_loop(mosq, 200, 1);
        if (loop_rc != MOSQ_ERR_SUCCESS) {
            fprintf(stderr, "mqtt loop error: %s\n",
                    mosquitto_strerror(loop_rc));
            state.success = 0;
            break;
        }

        if (state.connected && !state.discovery_completed &&
            state.discovery_deadline_ms > 0 &&
            current_ms > state.discovery_deadline_ms) {
            fprintf(stderr, "timed out waiting for discovery response\n");
            state.success = 0;
            break;
        }

        if (state.connected && state.next_status_ms != 0U &&
            current_ms >= state.next_status_ms && !state.shutdown_requested) {
            (void)publish_lifecycle_status(mosq, &state,
                                           GEISA_PROTO_APP_STATUS_RUNNING);
        }

        if (state.manifest_pending && state.manifest_deadline_ms > 0 &&
            current_ms > state.manifest_deadline_ms) {
            fprintf(stderr,
                    "timed out waiting for deployment manifest response\n");
            state.manifest_pending = 0;
            state.manifest_deadline_ms = 0;
            state.manifest_timeouts++;
            state.manifest_failures++;
        }

        if (state.probe.done && !state.probe.running) {
            pthread_join(state.probe.thread, NULL);
            state.probe.done = 0;
            finalize_probe_completion(&state);
        }

        if (expire_pending_response(&state, current_ms) != 0 &&
            state.timeout_count > 0U) {
            break;
        }

        if (state.discovery_completed) {
            start_pending_test(&state);
        }

        if (state.discovery_completed && !state.pending_app_response &&
            state.next_send_ms > 0 && current_ms >= state.next_send_ms) {
            state.next_send_ms = 0;
            if (dispatch_next_message(mosq, &state) != MOSQ_ERR_SUCCESS) {
                state.success = 0;
                break;
            }
        }
    }

    if (state.probe.running || state.probe.done) {
        state.probe.cancel = 1;
        pthread_join(state.probe.thread, NULL);
        state.probe.running = 0;
        state.probe.done = 0;
    }
    if (state.shutdown_requested || g_stop) {
        unsigned flush_attempt;

        if (state.connected && !state.shutdown_requested) {
            (void)publish_lifecycle_status(
                mosq, &state, GEISA_PROTO_APP_STATUS_SHUTTING_DOWN);
        }
        for (flush_attempt = 0U; state.connected && flush_attempt < 3U;
             flush_attempt++) {
            if (mosquitto_loop(mosq, 0, 1) != MOSQ_ERR_SUCCESS) {
                break;
            }
        }
    }
    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();

    if (g_stop) {
        printf("geisa-test received shutdown signal; exiting cleanly\n");
    }

    printf("summary accepted=%u rejected=%u other=%u timed_out=%u "
           "duplicate_responses=%u unmatched_responses=%u "
           "malformed_responses=%u expected_status=%s quota_rejected=%s "
           "attempts=%u last_payload_bytes=%lu config_applied=%u "
           "config_rejected=%u downstream_response_publish_failures=%u\n",
           state.accepted_count, state.rejected_count, state.other_count,
           state.timeout_count, state.duplicate_response_count,
           state.unmatched_response_count, state.malformed_response_count,
           state.options.expected_status,
           state.quota_rejected ? "true" : "false", state.send_attempt,
           (unsigned long)state.last_payload_size, state.config_apply_count,
           state.config_reject_count,
           state.downstream_response_publish_failures);

    if (g_stop) {
        completion_reason = "signal";
    } else if (state.timeout_count > 0U) {
        completion_reason = "response_timeout";
    } else if (state.options.operational_mode == APP_MODE_NORMAL &&
               state.options.message_count > 0 &&
               state.send_attempt >= (unsigned)state.options.message_count) {
        completion_reason = "message_count";
    } else if (state.done) {
        completion_reason = "completed";
    } else {
        completion_reason = "stopped";
    }
    final_pass =
        state.success && state.status_mismatch_count == 0U &&
        state.timeout_count == 0U && state.duplicate_response_count == 0U &&
        state.unmatched_response_count == 0U &&
        state.malformed_response_count == 0U && state.manifest_failures == 0U &&
        state.manifest_malformed == 0U && state.manifest_timeouts == 0U &&
        !state.manifest_pending;
    if (emit_machine_summary(&state, final_pass, completion_reason) != 0) {
        final_pass = 0;
    }
    return final_pass ? 0 : 1;
}
