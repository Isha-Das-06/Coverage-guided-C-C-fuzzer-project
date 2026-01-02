#ifndef FUZZER_H
#define FUZZER_H

#include <stdint.h>
#include <time.h>

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;

typedef struct {
    u64 iterations;
    u64 execs;
    u32 coverage_edges;
    u32 corpus_size;
    u32 crash_count;
    u32 hang_count;
    time_t start_time;
    double exec_per_sec;
} fuzzer_stats_t;

void fuzzer_set_verbose(int on);
void fuzzer_init(const char *input_dir, const char *output_dir, const char *target);
void fuzzer_run(int timeout_seconds, u32 max_iterations);
void fuzzer_show_stats(void);
void fuzzer_cleanup(void);

extern fuzzer_stats_t fuzzer_stats;

#endif
