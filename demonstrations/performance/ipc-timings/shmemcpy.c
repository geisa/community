//
// shmemcpy.c
//
// For questions/support: norman.mcentire@gmail.com
//
// To build: gcc -Wall shmemcpy.c -o shmemcpy
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
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>

#define SHM_NAME "/my_shared_buf"

typedef struct {
    struct timeval start;
    struct timeval end;
    uint32_t size;
    uint8_t data[];
} buf_data_t;

volatile sig_atomic_t sigusr1_received = 0;
volatile sig_atomic_t sigio_received = 0;

void handle_sigusr1(int sig) {
    sigusr1_received = 1;
}

void handle_sigio(int sig) {
    sigio_received = 1;
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

    // Allocate src in heap
    buf_data_t *src = malloc(total_size);
    if (!src) {
        perror("malloc");
        return EXIT_FAILURE;
    }
    src->size = size;
    for (int i = 0; i < size; i++) {
        src->data[i] = (uint8_t)i;
    }

    // Create and set up shared memory
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd < 0) {
        perror("shm_open");
        free(src);
        return EXIT_FAILURE;
    }

    if (ftruncate(shm_fd, total_size) < 0) {
        perror("ftruncate");
        shm_unlink(SHM_NAME);
        free(src);
        return EXIT_FAILURE;
    }

    pid_t child_pid = fork();
    if (child_pid < 0) {
        perror("fork");
        shm_unlink(SHM_NAME);
        free(src);
        return EXIT_FAILURE;
    }

    if (child_pid == 0) {
        // --- Child Process ---
        signal(SIGIO, handle_sigio);

        // Open and map shared memory
        int fd = shm_open(SHM_NAME, O_RDWR, 0666);
        if (fd < 0) {
            perror("child shm_open");
            exit(EXIT_FAILURE);
        }

        buf_data_t *dst = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (dst == MAP_FAILED) {
            perror("child mmap");
            exit(EXIT_FAILURE);
        }

        // Notify parent process that we're ready
        kill(getppid(), SIGUSR1);

        // Wait for SIGIO from parent
        while (!sigio_received) pause();

        // Record end time
        gettimeofday(&dst->end, NULL);

        // Compute and display metrics
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

        munmap(dst, total_size);
        close(fd);
        exit(EXIT_SUCCESS);
    } else {
        // --- Parent Process ---
        signal(SIGUSR1, handle_sigusr1);

        // Wait for SIGUSR1 from child
        while (!sigusr1_received) pause();

        // Map the shared memory
        buf_data_t *dst = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
        if (dst == MAP_FAILED) {
            perror("parent mmap");
            shm_unlink(SHM_NAME);
            free(src);
            return EXIT_FAILURE;
        }

        // Record start time and copy to shared memory
        gettimeofday(&src->start, NULL);
        memcpy(dst, src, total_size);

        // Notify child
        kill(child_pid, SIGIO);

        // Cleanup
        wait(NULL);
        munmap(dst, total_size);
        close(shm_fd);
        shm_unlink(SHM_NAME);
        free(src);
    }

    return EXIT_SUCCESS;
}

