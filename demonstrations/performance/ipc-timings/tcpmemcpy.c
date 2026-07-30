//
// tcpmemcpy.c
//
// For questions/support: norman.mcentire@gmail.com
//
// To build: gcc -Wall tcpmemcpy.c -o tcpmemcpy
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

#define TCP_PORT 54321
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

ssize_t full_write(int fd, const void *buf, size_t count) {
    size_t written = 0;
    while (written < count) {
        ssize_t res = write(fd, (char *)buf + written, count - written);
        if (res <= 0) return res;
        written += res;
    }
    return written;
}

ssize_t full_read(int fd, void *buf, size_t count) {
    size_t read_bytes = 0;
    while (read_bytes < count) {
        ssize_t res = read(fd, (char *)buf + read_bytes, count - read_bytes);
        if (res <= 0) return res;
        read_bytes += res;
    }
    return read_bytes;
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
        // --- Child Process (TCP Server) ---
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            perror("Child socket");
            exit(EXIT_FAILURE);
        }

        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = htons(TCP_PORT);

        if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            perror("Child bind");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        if (listen(server_fd, 1) < 0) {
            perror("Child listen");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        kill(getppid(), SIGUSR1); // Notify parent

        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            perror("Child accept");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        buf_data_t *dst = malloc(total_size);
        if (!dst) {
            perror("Child malloc");
            close(client_fd);
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        ssize_t n = full_read(client_fd, dst, total_size);
        if (n != total_size) {
            fprintf(stderr, "Child: Failed to read complete buffer\n");
            free(dst);
            close(client_fd);
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        gettimeofday(&dst->end, NULL);

        long sec = dst->end.tv_sec - dst->start.tv_sec;
        long usec = dst->end.tv_usec - dst->start.tv_usec;
        if (usec < 0) {
            sec--;
            usec += 1000000;
        }

        double elapsed = sec + usec / 1e6;
        double bps = elapsed > 0 ? (dst->size / elapsed) : 0;
        double mbps = bps / 1e6;

        printf("[Child] Elapsed Time: %.6f seconds\n", elapsed);
        printf("[Child] Transferred:  %u bytes\n", dst->size);
        printf("[Child] Throughput:   %.2f bytes/sec (%.2f MB/sec)\n", bps, mbps);

        free(dst);
        close(client_fd);
        close(server_fd);
        exit(EXIT_SUCCESS);
    } else {
        // --- Parent Process (TCP Client) ---
        signal(SIGUSR1, handle_sigusr1);

        while (!sigusr1_received) pause(); // Wait for child to bind

        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("Parent socket");
            free(src);
            return EXIT_FAILURE;
        }

        struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(TCP_PORT);
        inet_pton(AF_INET, LOCALHOST, &serv_addr.sin_addr);

        if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            perror("Parent connect");
            close(sockfd);
            free(src);
            return EXIT_FAILURE;
        }

        gettimeofday(&src->start, NULL);

        ssize_t sent = full_write(sockfd, src, total_size);
        if (sent != total_size) {
            fprintf(stderr, "Parent: Failed to send complete buffer\n");
        }

        close(sockfd);
        wait(NULL);
        free(src);
    }

    return EXIT_SUCCESS;
}

