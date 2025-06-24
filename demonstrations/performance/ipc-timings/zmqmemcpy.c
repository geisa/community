// memcpy.c
//
// For questions/support: norman.mcentire@gmail.com
//
// To build: gcc -Wall zmqmemcpy.c -o zmqmemcpy -lzmq
//
#define _GNU_SOURCE
#include <zmq.h>
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
        // --- Child Process (Receiver) ---
        void *context = zmq_ctx_new();
        void *receiver = zmq_socket(context, ZMQ_PULL);
        int rc = zmq_bind(receiver, "tcp://127.0.0.1:5555");
        if (rc != 0) {
            perror("zmq_bind");
            exit(EXIT_FAILURE);
        }

        kill(getppid(), SIGUSR1);  // Notify parent

        // Allocate buffer
        size_t max_recv = sizeof(struct timeval) + size;
        uint8_t *recv_buf = malloc(max_recv);
        if (!recv_buf) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }

        int received = zmq_recv(receiver, recv_buf, max_recv, 0);
        if (received < (int)sizeof(struct timeval)) {
            fprintf(stderr, "[Child] Incomplete data received\n");
            exit(EXIT_FAILURE);
        }

        struct timeval start, end;
        memcpy(&start, recv_buf, sizeof(struct timeval));
        gettimeofday(&end, NULL);

        long sec = end.tv_sec - start.tv_sec;
        long usec = end.tv_usec - start.tv_usec;
        if (usec < 0) {
            sec--;
            usec += 1000000;
        }

        double elapsed = sec + usec / 1e6;
        double bps = elapsed > 0 ? (size / elapsed) : 0;
        double mbps = bps / 1e6;

        printf("[Child] Elapsed Time: %.6f seconds\n", elapsed);
        printf("[Child] Transferred:  %d bytes\n", size);
        printf("[Child] Throughput:   %.2f bytes/sec (%.2f MB/sec)\n", bps, mbps);

        free(recv_buf);
        zmq_close(receiver);
        zmq_ctx_term(context);
        exit(EXIT_SUCCESS);
    } else {
        // --- Parent Process (Sender) ---
        signal(SIGUSR1, handle_sigusr1);
        while (!sigusr1_received) pause();

        void *context = zmq_ctx_new();
        void *sender = zmq_socket(context, ZMQ_PUSH);
        if (zmq_connect(sender, "tcp://127.0.0.1:5555") != 0) {
            perror("zmq_connect");
            free(src);
            return EXIT_FAILURE;
        }

        // Create payload: [start_time][data]
        gettimeofday(&src->start, NULL);
        size_t payload_size = sizeof(struct timeval) + size;
        uint8_t *payload = malloc(payload_size);
        memcpy(payload, &src->start, sizeof(struct timeval));
        memcpy(payload + sizeof(struct timeval), src->data, size);

        zmq_send(sender, payload, payload_size, 0);

        free(payload);
        zmq_close(sender);
        zmq_ctx_term(context);
        free(src);
        wait(NULL);
    }

    return EXIT_SUCCESS;
}

