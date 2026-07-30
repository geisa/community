// dbusmemcpy.c
//
// For questions/support: norman.mcentire@gmail.com
//
// To build: gcc -Wall dbusmemcpy.c -o dbusmemcpy $(pkg-config --cflags --libs dbus-1)
//

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <getopt.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <dbus/dbus.h>

typedef struct {
    struct timeval start;
    struct timeval end;
    uint32_t size;
    uint8_t data[];
} buf_data_t;

volatile sig_atomic_t sigusr1_received = 0;

void handle_sigusr1(int sig) {
    sigusr1_received = 1;
}

int main(int argc, char *argv[]) {
    int size = 0;

    static struct option long_options[] = {
        {"size", required_argument, 0, 's'},
        {0, 0, 0, 0}
    };

    while (1) {
        int option_index = 0;
        int c = getopt_long(argc, argv, "s:", long_options, &option_index);
        if (c == -1) break;

        switch (c) {
            case 's':
                size = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s --size NUMBER\n", argv[0]);
                return EXIT_FAILURE;
        }
    }

    if (size <= 0) {
        fprintf(stderr, "Invalid size specified.\n");
        return EXIT_FAILURE;
    }

    size_t total_size = sizeof(buf_data_t) + size;

    buf_data_t *src = malloc(total_size);
    if (!src) {
        perror("malloc");
        return EXIT_FAILURE;
    }

    src->size = size;
    for (int i = 0; i < size; i++) {
        src->data[i] = (uint8_t)i;
    }

    pid_t child_pid = fork();
    if (child_pid < 0) {
        perror("fork");
        free(src);
        return EXIT_FAILURE;
    }

    if (child_pid == 0) {
        // --- Child Process (D-Bus Server) ---
        DBusError err;
        dbus_error_init(&err);

        DBusConnection *conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
        if (!conn) {
            fprintf(stderr, "Failed to connect to the D-Bus session bus: %s\n", err.message);
            dbus_error_free(&err);
            exit(EXIT_FAILURE);
        }

        dbus_bus_request_name(conn, "org.example.DBusTransfer", DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
        if (dbus_error_is_set(&err)) {
            fprintf(stderr, "Failed to request name on D-Bus: %s\n", err.message);
            dbus_error_free(&err);
            dbus_connection_unref(conn);
            exit(EXIT_FAILURE);
        }

        // Signal parent that we are ready
        kill(getppid(), SIGUSR1);

        while (1) {
            dbus_connection_read_write(conn, 100);
            DBusMessage *msg = dbus_connection_pop_message(conn);
            if (!msg) continue;

            if (dbus_message_is_method_call(msg, "org.example.DBusTransfer", "TransferData")) {
                DBusMessageIter args;
                dbus_message_iter_init(msg, &args);

                uint32_t received_size = 0;
                const uint8_t *data_ptr;
                int array_len = 0;

                if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_UINT32) {
                    fprintf(stderr, "Child: Expected uint32_t\n");
                    dbus_message_unref(msg);
                    continue;
                }

                dbus_message_iter_get_basic(&args, &received_size);
                dbus_message_iter_next(&args);

                if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_ARRAY) {
                    fprintf(stderr, "Child: Expected byte array\n");
                    dbus_message_unref(msg);
                    continue;
                }

                DBusMessageIter sub_iter;
                dbus_message_iter_recurse(&args, &sub_iter);
                dbus_message_iter_get_fixed_array(&sub_iter, &data_ptr, &array_len);

                if (array_len < sizeof(struct timeval)) {
                    fprintf(stderr, "Child: Incomplete timing info\n");
                    dbus_message_unref(msg);
                    continue;
                }

                struct timeval start;
                memcpy(&start, data_ptr, sizeof(struct timeval));

                struct timeval end;
                gettimeofday(&end, NULL);

                long sec = end.tv_sec - start.tv_sec;
                long usec = end.tv_usec - start.tv_usec;
                if (usec < 0) {
                    sec--;
                    usec += 1000000;
                }

                double elapsed = sec + usec / 1e6;
                double bps = elapsed > 0 ? (received_size / elapsed) : 0;
                double mbps = bps / 1e6;

                printf("[Child] Elapsed Time: %.6f seconds\n", elapsed);
                printf("[Child] Transferred:  %u bytes\n", received_size);
                printf("[Child] Throughput:   %.2f bytes/sec (%.2f MB/sec)\n", bps, mbps);

                dbus_message_unref(msg);
                break; // One-shot transfer; exit after report
            }

            dbus_message_unref(msg);
        }

        dbus_connection_unref(conn);
        exit(EXIT_SUCCESS);
    } else {
        // --- Parent Process (D-Bus Client) ---
        signal(SIGUSR1, handle_sigusr1);
        while (!sigusr1_received) pause();

        DBusError err;
        dbus_error_init(&err);

        DBusConnection *conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
        if (!conn) {
            fprintf(stderr, "Parent: D-Bus connection failed: %s\n", err.message);
            dbus_error_free(&err);
            free(src);
            return EXIT_FAILURE;
        }

        DBusMessage *msg = dbus_message_new_method_call(
            "org.example.DBusTransfer",
            "/org/example/DBusTransfer",
            "org.example.DBusTransfer",
            "TransferData"
        );
        if (!msg) {
            fprintf(stderr, "Parent: Failed to create message\n");
            dbus_connection_unref(conn);
            free(src);
            return EXIT_FAILURE;
        }

        // Prepare payload: [start_time | data[]]
        gettimeofday(&src->start, NULL);
        size_t payload_size = sizeof(struct timeval) + size;
        uint8_t *payload = malloc(payload_size);
        if (!payload) {
            perror("malloc");
            dbus_message_unref(msg);
            dbus_connection_unref(conn);
            free(src);
            return EXIT_FAILURE;
        }

        memcpy(payload, &src->start, sizeof(struct timeval));
        memcpy(payload + sizeof(struct timeval), src->data, size);

        DBusMessageIter args;
        dbus_message_iter_init_append(msg, &args);
        dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &src->size);

	DBusMessageIter array_iter;
	dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "y", &array_iter);
	dbus_message_iter_append_fixed_array(&array_iter, DBUS_TYPE_BYTE, &payload, payload_size);
	dbus_message_iter_close_container(&args, &array_iter);

        if (!dbus_connection_send(conn, msg, NULL)) {
            fprintf(stderr, "Parent: Failed to send message\n");
        }

        dbus_connection_flush(conn);
        dbus_message_unref(msg);
        dbus_connection_unref(conn);

        free(payload);
        free(src);
        wait(NULL);
    }

    return EXIT_SUCCESS;
}

