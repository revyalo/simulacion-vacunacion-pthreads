#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
    uint64_t begin;
    uint64_t end;
    uint64_t rounds;
    uint64_t checksum;
} Work;

static uint64_t process_records(uint64_t begin, uint64_t end, uint64_t rounds) {
    uint64_t checksum = 0;
    uint64_t record;

    for (record = begin; record < end; record++) {
        uint64_t value = record + UINT64_C(0x9e3779b97f4a7c15);
        uint64_t round;
        for (round = 0; round < rounds; round++) {
            value ^= value >> 30;
            value *= UINT64_C(0xbf58476d1ce4e5b9);
            value ^= value >> 27;
            value *= UINT64_C(0x94d049bb133111eb);
            value ^= value >> 31;
            value += round;
        }
        checksum ^= value;
    }
    return checksum;
}

static void *worker(void *argument) {
    Work *work = argument;

    work->checksum = process_records(work->begin, work->end, work->rounds);
    return NULL;
}

static double now_seconds(void) {
    struct timespec now;

    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }
    return (double)now.tv_sec + (double)now.tv_nsec / 1000000000.0;
}

static int parse_positive(const char *text, uint64_t *value) {
    char *end;
    unsigned long long parsed;

    errno = 0;
    if (text[0] == '-') {
        return 0;
    }
    parsed = strtoull(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || parsed == 0) {
        return 0;
    }
    *value = (uint64_t)parsed;
    return 1;
}

static int parallel_run(uint64_t thread_count, uint64_t items, uint64_t rounds, uint64_t *checksum) {
    pthread_t *threads = calloc((size_t)thread_count, sizeof(*threads));
    Work *works = calloc((size_t)thread_count, sizeof(*works));
    uint64_t created = 0;
    uint64_t index;

    if (threads == NULL || works == NULL) {
        perror("calloc");
        free(threads);
        free(works);
        return 0;
    }
    for (index = 0; index < thread_count; index++) {
        int result;
        uint64_t base = items / thread_count;
        uint64_t remainder = items % thread_count;
        works[index].begin = base * index + (index < remainder ? index : remainder);
        works[index].end = works[index].begin + base + (index < remainder ? 1 : 0);
        works[index].rounds = rounds;
        result = pthread_create(&threads[index], NULL, worker, &works[index]);
        if (result != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(result));
            break;
        }
        created++;
    }
    if (created != thread_count) {
        for (index = 0; index < created; index++) {
            int result = pthread_join(threads[index], NULL);
            if (result != 0) {
                fprintf(stderr, "pthread_join: %s\n", strerror(result));
            }
        }
        free(threads);
        free(works);
        return 0;
    }
    *checksum = 0;
    for (index = 0; index < thread_count; index++) {
        int result = pthread_join(threads[index], NULL);
        if (result != 0) {
            fprintf(stderr, "pthread_join: %s\n", strerror(result));
            free(threads);
            free(works);
            return 0;
        }
        *checksum ^= works[index].checksum;
    }
    free(threads);
    free(works);
    return 1;
}

int main(int argc, char *argv[]) {
    uint64_t threads = 4;
    uint64_t items = UINT64_C(3000000);
    uint64_t rounds = 20;
    uint64_t sequential_checksum;
    uint64_t parallel_checksum;
    double sequential_start;
    double sequential_seconds;
    double parallel_start;
    double parallel_seconds;
    int csv = 0;
    int index;

    for (index = 1; index < argc; index++) {
        uint64_t *destination = NULL;
        if (strcmp(argv[index], "--threads") == 0) {
            destination = &threads;
        } else if (strcmp(argv[index], "--items") == 0) {
            destination = &items;
        } else if (strcmp(argv[index], "--rounds") == 0) {
            destination = &rounds;
        } else if (strcmp(argv[index], "--csv") == 0) {
            csv = 1;
            continue;
        } else {
            fprintf(stderr, "Opcion desconocida: %s\n", argv[index]);
            return 2;
        }
        if (++index >= argc || !parse_positive(argv[index], destination)) {
            fprintf(stderr, "Valor positivo requerido para %s\n", argv[index - 1]);
            return 2;
        }
    }
    if (threads > 256) {
        fprintf(stderr, "El benchmark admite como maximo 256 hilos\n");
        return 2;
    }

    sequential_start = now_seconds();
    sequential_checksum = process_records(0, items, rounds);
    sequential_seconds = now_seconds() - sequential_start;

    parallel_start = now_seconds();
    if (!parallel_run(threads, items, rounds, &parallel_checksum)) {
        return 1;
    }
    parallel_seconds = now_seconds() - parallel_start;
    if (parallel_checksum != sequential_checksum) {
        fprintf(stderr, "Checksum diferente entre ejecucion secuencial y paralela\n");
        return 1;
    }

    if (csv) {
        printf("%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%.9f,%.9f,%.4f,%" PRIu64 "\n",
            threads, items, rounds, sequential_seconds, parallel_seconds,
            sequential_seconds / parallel_seconds, parallel_checksum);
    } else {
        printf("hilos=%" PRIu64 " elementos=%" PRIu64 " rondas=%" PRIu64 "\n", threads, items, rounds);
        printf("secuencial=%.6f s paralelo=%.6f s speedup=%.3fx checksum=%" PRIu64 "\n",
            sequential_seconds, parallel_seconds, sequential_seconds / parallel_seconds,
            parallel_checksum);
    }
    return 0;
}
