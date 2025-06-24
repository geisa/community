//
// memcpy.c
//
// For questions/support: norman.mcentire@gmail.com
//
// To build: gcc -Wall memcpy.c -o memcpy
//
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <getopt.h>

typedef struct {
    struct timeval start;
    struct timeval end;
    uint32_t size;
    uint8_t data[];
} buf_data_t;

int main(int argc, char *argv[]) {
    int size = 0;

    // Parse command-line arguments
    static struct option long_options[] = {
        {"size", required_argument, 0, 's'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int c;

    while ((c = getopt_long(argc, argv, "s:", long_options, &option_index)) != -1) {
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

    // Allocate source and destination buf_data_t buffers
    buf_data_t *src = malloc(sizeof(buf_data_t) + size);
    buf_data_t *dst = malloc(sizeof(buf_data_t) + size);

    if (!src || !dst) {
        fprintf(stderr, "Memory allocation failed.\n");
        free(src);
        free(dst);
        return EXIT_FAILURE;
    }

    // Store size in the src buffer metadata
    src->size = size;

    // Fill the source data buffer with values 0, 1, 2, ...
    for (int i = 0; i < size; i++) {
        src->data[i] = (uint8_t)i;
    }

    gettimeofday(&src->start, NULL);
    printf("Start Time: %ld.%06ld seconds\n", src->start.tv_sec, src->start.tv_usec);

    // Copy the entire source buffer into destination buffer
    memcpy(dst, src, sizeof(buf_data_t) + size);

    gettimeofday(&dst->end, NULL);
    printf("End Time:   %ld.%06ld seconds\n", dst->end.tv_sec, dst->end.tv_usec);

    // Calculate elapsed time
    long seconds = dst->end.tv_sec - src->start.tv_sec;
    long microseconds = dst->end.tv_usec - src->start.tv_usec;
    if (microseconds < 0) {
        seconds--;
        microseconds += 1000000;
    }

    double elapsed_time_sec = seconds + microseconds / 1e6;
    size_t bytes_copied = size;
    double bytes_per_sec = elapsed_time_sec > 0 ? (bytes_copied / elapsed_time_sec) : 0;
    double megabytes_per_sec = bytes_per_sec / 1000000.0;

    printf("Elapsed Time: %.6f seconds\n", elapsed_time_sec);
    printf("Transferred:  %zu bytes\n", bytes_copied);
    printf("Throughput:   %.2f bytes/second\n", bytes_per_sec);
    printf("              %.2f MB/second\n", megabytes_per_sec);

    // Clean up
    free(src);
    free(dst);

    return EXIT_SUCCESS;
}

