#include "coverage.h"
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

u8 coverage_history[COVERAGE_MAP_SIZE];
static u8 *coverage_map_ptr = NULL;

void coverage_init(void) {
    int fd = shm_open(COVERAGE_SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        return;
    }

    if (ftruncate(fd, COVERAGE_MAP_SIZE) == -1) {
        perror("ftruncate");
        close(fd);
        return;
    }

    coverage_map_ptr = (u8 *)mmap(NULL, COVERAGE_MAP_SIZE, PROT_READ | PROT_WRITE,
                                   MAP_SHARED, fd, 0);
    if (coverage_map_ptr == MAP_FAILED) {
        perror("mmap");
        coverage_map_ptr = NULL;   /* MAP_FAILED is not NULL; callers only check NULL */
        close(fd);
        return;
    }

    close(fd);
    memset(coverage_history, 0, COVERAGE_MAP_SIZE);
    memset(coverage_map_ptr, 0, COVERAGE_MAP_SIZE);
}

void coverage_cleanup(void) {
    if (coverage_map_ptr) {
        munmap(coverage_map_ptr, COVERAGE_MAP_SIZE);
        coverage_map_ptr = NULL;
    }

    /* Without this the segment survives in /dev/shm after every run. */
    shm_unlink(COVERAGE_SHM_NAME);
}

void coverage_reset(void) {
    if (coverage_map_ptr) {
        memset(coverage_map_ptr, 0, COVERAGE_MAP_SIZE);
    }
}

int coverage_has_new_paths(const u8 *curr_map, u32 map_size) {
    if (!curr_map) return 0;

    if (map_size > COVERAGE_MAP_SIZE) {
        map_size = COVERAGE_MAP_SIZE;
    }

    for (u32 i = 0; i < map_size; i++) {
        if (curr_map[i] && !coverage_history[i]) {
            return 1;
        }
    }

    return 0;
}

u32 coverage_count_edges(const u8 *map, u32 map_size) {
    if (!map) return 0;

    u32 count = 0;

    if (map_size > COVERAGE_MAP_SIZE) {
        map_size = COVERAGE_MAP_SIZE;
    }

    for (u32 i = 0; i < map_size; i++) {
        if (map[i]) {
            count++;
        }
    }

    return count;
}

void coverage_update_history(const u8 *map, u32 map_size) {
    if (!map) return;

    if (map_size > COVERAGE_MAP_SIZE) {
        map_size = COVERAGE_MAP_SIZE;
    }

    for (u32 i = 0; i < map_size; i++) {
        if (map[i]) {
            coverage_history[i] = 1;
        }
    }
}

u64 coverage_hash(const u8 *map, u32 map_size) {
    if (!map) return 0;

    u64 hash = 5381;

    if (map_size > COVERAGE_MAP_SIZE) {
        map_size = COVERAGE_MAP_SIZE;
    }

    for (u32 i = 0; i < map_size; i++) {
        if (map[i]) {
            hash = ((hash << 5) + hash) ^ map[i];
        }
    }

    return hash;
}

u8 *coverage_get_map(void) {
    return coverage_map_ptr;
}
