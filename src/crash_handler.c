#include "crash_handler.h"
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

static u32 crash_count = 0;
static u32 hang_count = 0;
static u64 last_crash_hash = 0;

void crash_init(void) {
    crash_count = 0;
    hang_count = 0;
    last_crash_hash = 0;
}

const char *signal_name(int sig) {
    switch (sig) {
        case SIGSEGV: return "SIGSEGV (Segmentation Fault)";
        case SIGABRT: return "SIGABRT (Abort)";
        case SIGILL:  return "SIGILL (Illegal Instruction)";
        case SIGBUS:  return "SIGBUS (Bus Error)";
        case SIGFPE:  return "SIGFPE (Floating-Point Exception)";
        case SIGKILL: return "SIGKILL (Killed)";
        default:      return "Unknown Signal";
    }
}

u64 crash_hash(int sig) {
    return ((u64)sig) * 0x9e3779b97f4a7c15ULL;
}

static int classify_status(int status, u32 *exit_signal) {
    if (WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);

        if (exit_signal) {
            *exit_signal = sig;
        }

        switch (sig) {
            case SIGSEGV:
            case SIGABRT:
            case SIGILL:
            case SIGBUS:
            case SIGFPE:
                crash_count++;
                last_crash_hash = crash_hash(sig);
                return 1;
            default:
                return 0;
        }
    }

    if (WIFEXITED(status)) {
        int exit_code = WEXITSTATUS(status);

        if (exit_signal) {
            *exit_signal = (u32)exit_code;
        }

        return 0;
    }

    return 0;
}

int crash_detect(pid_t pid, u32 *exit_signal) {
    int status;
    pid_t result;

    do {
        result = waitpid(pid, &status, 0);
    } while (result < 0 && errno == EINTR);

    if (result <= 0) {
        return 0;
    }

    return classify_status(status, exit_signal);
}

int crash_detect_timeout(pid_t pid, u32 *exit_signal, int timeout_seconds, int *timed_out) {
    if (timed_out) {
        *timed_out = 0;
    }

    if (timeout_seconds <= 0) {
        return crash_detect(pid, exit_signal);
    }

    /* Poll instead of blocking, so a target stuck in an infinite loop cannot
       wedge the whole fuzzer. */
    const long poll_us = 1000;
    long waited_us = 0;
    long limit_us = (long)timeout_seconds * 1000000L;

    while (waited_us < limit_us) {
        int status;
        pid_t result = waitpid(pid, &status, WNOHANG);

        if (result == pid) {
            return classify_status(status, exit_signal);
        }

        if (result < 0) {
            if (errno == EINTR) continue;
            return 0;
        }

        usleep(poll_us);
        waited_us += poll_us;
    }

    /* Still running: it hung. Kill it and reap so we do not leak zombies. */
    kill(pid, SIGKILL);

    int status;
    while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {
        /* retry */
    }

    hang_count++;

    if (timed_out) {
        *timed_out = 1;
    }

    return 0;
}

void crash_stats(void) {
    printf("[*] Crashes found: %u | Hangs: %u\n", crash_count, hang_count);
}
