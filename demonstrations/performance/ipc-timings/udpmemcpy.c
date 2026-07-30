//
// udpmemcpy.c
//
// For questions/support: norman.mcentire@gmail.com
//
// To build: gcc -Wall udpmemcpy.c -o udpmemcpy
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
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define UDP_PORT 54321
#define LOCALHOST "127.0.0.1"

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

    // Allocate and prepare source buffer
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
        // --- Child Process ---
        struct sockaddr_in addr;
        int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            perror("Child socket");
            exit(EXIT_FAILURE);
        }

        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = htons(UDP_PORT);

        if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            perror("Child bind");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        // Notify parent
        kill(getppid(), SIGUSR1);

        // Allocate destination buffer
        buf_data_t *dst = malloc(total_size);
        if (!dst) {
            perror("Child malloc");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        ssize_t received = recvfrom(sockfd, dst, total_size, 0, NULL, NULL);
        if (received < 0) {
            perror("Child recvfrom");
            free(dst);
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        gettimeofday(&dst->end, NULL);

        // Calculate elapsed time
        long sec = dst->end.tv_sec - dst->start.tv_sec;
        long usec = dst->end.tv_usec - dst->start.tv_usec;
        if (usec < 0) {
            sec--;
            usec += 1000000;
        }
        double elapsed = sec + usec / 1e6;
        size_t bytes = dst->size;
        double bps = elapsed > 0 ? (bytes / elapsed) : 0;
        double mbps = bps / 1000000.0;

        printf("[Child] Elapsed Time: %.6f seconds\n", elapsed);
        printf("[Child] Transferred:  %zu bytes\n", bytes);
        printf("[Child] Throughput:   %.2f bytes/sec (%.2f MB/sec)\n", bps, mbps);

        free(dst);
        close(sockfd);
        exit(EXIT_SUCCESS);
    } else {
        // --- Parent Process ---
        signal(SIGUSR1, handle_sigusr1);

        while (!sigusr1_received) pause();

        // Create socket for sending
        int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            perror("Parent socket");
            free(src);
            return EXIT_FAILURE;
        }

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(UDP_PORT);
        addr.sin_addr.s_addr = inet_addr(LOCALHOST);

        // Get start time and send buffer
        gettimeofday(&src->start, NULL);

        ssize_t sent = sendto(sockfd, src, total_size, 0,
                              (struct sockaddr *)&addr, sizeof(addr));
        if (sent < 0) {
            perror("Parent sendto");
        }

        close(sockfd);
        wait(NULL);
        free(src);
    }

    return EXIT_SUCCESS;
}

