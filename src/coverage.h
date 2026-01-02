#ifndef COVERAGE_H
#define COVERAGE_H

#include <stdint.h>
#include <stddef.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define COVERAGE_MAP_SIZE 65536

/* POSIX shm name (NOT a filesystem path - shm_open prepends /dev/shm itself).
   Must match the copy in coverage_shared.c, which is compiled into targets. */
#define COVERAGE_SHM_NAME "/fuzzer_coverage"

void coverage_init(void);
void coverage_cleanup(void);
void coverage_reset(void);
int coverage_has_new_paths(const u8 *curr_map, u32 map_size);
u32 coverage_count_edges(const u8 *map, u32 map_size);
void coverage_update_history(const u8 *map, u32 map_size);
u64 coverage_hash(const u8 *map, u32 map_size);
u8 *coverage_get_map(void);

extern u8 coverage_history[COVERAGE_MAP_SIZE];

#endif
