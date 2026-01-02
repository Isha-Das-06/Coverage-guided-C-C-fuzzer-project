#ifndef CRASH_HANDLER_H
#define CRASH_HANDLER_H

#include <stdint.h>
#include <sys/types.h>

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;

typedef struct {
    int signal;
    u32 exit_code;
    const char *description;
    u64 hash;
} crash_info_t;

void crash_init(void);
int crash_detect(pid_t pid, u32 *exit_signal);

/* Like crash_detect, but gives up after timeout_seconds and kills the child.
   Sets *timed_out to 1 in that case (and returns 0, a hang is not a crash). */
int crash_detect_timeout(pid_t pid, u32 *exit_signal, int timeout_seconds, int *timed_out);
const char *signal_name(int sig);
u64 crash_hash(int sig);
void crash_stats(void);

#endif
