#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#define COVERAGE_MAP_SIZE 65536
#define COVERAGE_SHM_NAME "/fuzzer_coverage"

static uint8_t *coverage_map = NULL;
static int coverage_fd = -1;

static void coverage_init_map(void) {
    if (coverage_map != NULL) return;

    coverage_fd = shm_open(COVERAGE_SHM_NAME, O_RDWR, 0666);
    if (coverage_fd == -1) {
        coverage_fd = shm_open(COVERAGE_SHM_NAME, O_CREAT | O_RDWR, 0666);
        if (coverage_fd == -1) {
            return;
        }
        ftruncate(coverage_fd, COVERAGE_MAP_SIZE);
    }

    coverage_map = (uint8_t *)mmap(NULL, COVERAGE_MAP_SIZE, PROT_READ | PROT_WRITE,
                                   MAP_SHARED, coverage_fd, 0);
    if (coverage_map == MAP_FAILED) {
        coverage_map = NULL;
        close(coverage_fd);
        coverage_fd = -1;
    }
}

void __sanitizer_cov_trace_pc_guard_init(uint32_t *start, uint32_t *stop) {
    /* Each guard is zero-initialized by the compiler; this callback must hand
       every one a distinct nonzero id, which is then used as the index into
       the coverage map. Without this, *every* edge writes to coverage_map[0]
       and the fuzzer sees just a single edge - coverage guidance is dead. */
    static uint32_t next_id = 1;

    coverage_init_map();

    if (start == stop || *start) return;  /* already initialized */

    for (uint32_t *p = start; p < stop; p++) {
        *p = next_id;
        next_id++;
        /* Keep ids inside the map; 0 stays reserved as "unassigned". */
        if (next_id >= COVERAGE_MAP_SIZE) next_id = 1;
    }
}

void __sanitizer_cov_trace_pc_guard(uint32_t *guard) {
    if (coverage_map && *guard < COVERAGE_MAP_SIZE) {
        if (coverage_map[*guard] < 255) {
            coverage_map[*guard]++;
        }
    }
}
