#include "fuzzer.h"
#include "mutator.h"
#include "coverage.h"
#include "corpus.h"
#include "crash_handler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>

fuzzer_stats_t fuzzer_stats = {0};

static char *target_path = NULL;
static char *input_dir = NULL;
static char *output_dir = NULL;
static char *crashes_dir = NULL;
static char *corpus_dir = NULL;
static char *hangs_dir = NULL;

static volatile sig_atomic_t should_stop = 0;
static int verbose = 0;

static void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        should_stop = 1;
    }
}

static void create_output_dirs(void) {
    mkdir(output_dir, 0755);
    mkdir(crashes_dir, 0755);
    mkdir(corpus_dir, 0755);
    mkdir(hangs_dir, 0755);
}

static void save_crash(const u8 *input, u32 input_size, int signal) {
    static u32 crash_id = 0;

    char crash_path[1024];
    char crash_info_path[1024];

    snprintf(crash_path, sizeof(crash_path), "%s/crash_%03u", crashes_dir, crash_id);
    snprintf(crash_info_path, sizeof(crash_info_path), "%s/crash_%03u.txt", crashes_dir, crash_id);

    FILE *f = fopen(crash_path, "wb");

    if (f) {
        fwrite(input, 1, input_size, f);
        fclose(f);
    }

    f = fopen(crash_info_path, "w");

    if (f) {
        fprintf(f, "Crash Type: %s\n", signal_name(signal));
        fprintf(f, "Signal: %d\n", signal);
        fprintf(f, "Timestamp: %ld\n", time(NULL));
        fprintf(f, "Input Size: %u bytes\n", input_size);
        fprintf(f, "Input (hex): ");

        for (u32 i = 0; i < input_size && i < 64; i++) {
            fprintf(f, "%02x ", input[i]);
        }

        if (input_size > 64) {
            fprintf(f, "... (%u more bytes)", input_size - 64);
        }

        fprintf(f, "\n");
        fprintf(f, "Coverage: %u edges\n", fuzzer_stats.coverage_edges);
        fclose(f);
    }

    printf("[!] CRASH: %s (saved as crash_%03u)\n", signal_name(signal), crash_id);
    crash_id++;
    fuzzer_stats.crash_count++;
}

static void save_hang(const u8 *input, u32 input_size) {
    static u32 hang_id = 0;

    char hang_path[1024];
    snprintf(hang_path, sizeof(hang_path), "%s/hang_%03u", hangs_dir, hang_id);

    FILE *f = fopen(hang_path, "wb");

    if (f) {
        fwrite(input, 1, input_size, f);
        fclose(f);
    }

    printf("[!] HANG: target timed out (saved as hang_%03u)\n", hang_id);
    hang_id++;
    fuzzer_stats.hang_count++;
}

static void save_stats(void) {
    char stats_path[1024];
    snprintf(stats_path, sizeof(stats_path), "%s/stats.txt", output_dir);

    FILE *f = fopen(stats_path, "a");

    if (f) {
        time_t now = time(NULL);
        fprintf(f, "[%ld] Iterations: %llu | Execs/s: %.1f | Coverage: %u edges | "
                "Corpus: %u | Crashes: %u\n",
                (long)now, (unsigned long long)fuzzer_stats.iterations,
                fuzzer_stats.exec_per_sec,
                fuzzer_stats.coverage_edges, fuzzer_stats.corpus_size,
                fuzzer_stats.crash_count);
        fclose(f);
    }
}

void fuzzer_set_verbose(int on) {
    verbose = on;
}

void fuzzer_init(const char *in_dir, const char *out_dir, const char *target) {
    size_t sub_len = strlen(out_dir) + 20;

    target_path = strdup(target);
    input_dir = strdup(in_dir);
    output_dir = strdup(out_dir);

    crashes_dir = (char *)malloc(sub_len);
    corpus_dir = (char *)malloc(sub_len);
    hangs_dir = (char *)malloc(sub_len);

    if (!target_path || !input_dir || !output_dir ||
        !crashes_dir || !corpus_dir || !hangs_dir) {
        fprintf(stderr, "[!] Out of memory during init\n");
        exit(1);
    }

    snprintf(crashes_dir, sub_len, "%s/crashes", out_dir);
    snprintf(corpus_dir, sub_len, "%s/corpus", out_dir);
    snprintf(hangs_dir, sub_len, "%s/hangs", out_dir);

    create_output_dirs();
    coverage_init();
    corpus_init();
    mutator_init();
    crash_init();
    corpus_load(input_dir);

    fuzzer_stats.start_time = time(NULL);
    fuzzer_stats.iterations = 0;
    fuzzer_stats.execs = 0;
    fuzzer_stats.coverage_edges = 0;
    fuzzer_stats.corpus_size = corpus_size();
    fuzzer_stats.crash_count = 0;
    fuzzer_stats.hang_count = 0;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* A target that exits without reading stdin would otherwise kill the
       fuzzer itself when we write to the pipe. */
    signal(SIGPIPE, SIG_IGN);

    printf("[*] Fuzzer initialized\n");
    printf("[*] Target: %s\n", target_path);
    printf("[*] Input dir: %s\n", input_dir);
    printf("[*] Output dir: %s\n", output_dir);
}

void fuzzer_run(int timeout_seconds, u32 max_iterations) {
    time_t last_stats = time(NULL);

    /* Half a megabyte of buffers does not belong on the stack. */
    static u8 mutated_buffer[MAX_INPUT_SIZE];

    printf("[*] Starting fuzzer...\n");

    while (!should_stop) {
        if (max_iterations > 0 && fuzzer_stats.iterations >= max_iterations) {
            printf("[*] Max iterations reached\n");
            break;
        }

        corpus_entry_t *entry = corpus_pick_random();

        if (!entry) {
            printf("[!] No corpus entries\n");
            break;
        }

        u32 input_size = entry->size;

        if (input_size > MAX_INPUT_SIZE) {
            input_size = MAX_INPUT_SIZE;
        }

        memcpy(mutated_buffer, entry->data, input_size);
        entry->exec_count++;

        u32 mutations = random_range(1, 5);

        for (u32 i = 0; i < mutations; i++) {
            mutate_random(mutated_buffer, &input_size);
        }

        /* The pipe must be created before the fork. Creating it inside the
           child and writing to it there deadlocks as soon as the input
           exceeds the pipe buffer (~64 KB), because nothing is draining it. */
        int input_pipe[2];

        if (pipe(input_pipe) != 0) {
            perror("pipe");
            break;
        }

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            close(input_pipe[0]);
            close(input_pipe[1]);
            break;
        }

        if (pid == 0) {
            close(input_pipe[1]);
            dup2(input_pipe[0], STDIN_FILENO);
            close(input_pipe[0]);

            /* Targets are built with ASan. By default ASan reports the bug and
               calls _exit(1), so the parent sees a normal exit and every
               memory-safety bug is silently missed. abort_on_error makes it
               raise SIGABRT instead, which crash_detect recognises. */
            setenv("ASAN_OPTIONS", "abort_on_error=1:symbolize=0:detect_leaks=0", 1);

            execl(target_path, target_path, (char *)NULL);
            _exit(1);
        } else {
            close(input_pipe[0]);

            /* The target may exit without reading its input; that gives us
               EPIPE rather than killing the fuzzer. */
            ssize_t written = write(input_pipe[1], mutated_buffer, input_size);
            (void)written;
            close(input_pipe[1]);

            u32 exit_signal = 0;
            int timed_out = 0;

            if (crash_detect_timeout(pid, &exit_signal, timeout_seconds, &timed_out)) {
                save_crash(mutated_buffer, input_size, (int)exit_signal);
            } else if (timed_out) {
                save_hang(mutated_buffer, input_size);
            } else {
                u8 *cov_map = coverage_get_map();
                if (cov_map) {
                    u64 cov_hash = coverage_hash(cov_map, COVERAGE_MAP_SIZE);

                    if (coverage_has_new_paths(cov_map, COVERAGE_MAP_SIZE)) {
                        coverage_update_history(cov_map, COVERAGE_MAP_SIZE);
                        corpus_add(mutated_buffer, input_size, cov_hash);

                        if (verbose) {
                            printf("[+] New path found! (corpus size: %u)\n", corpus_size());
                        }
                    }
                }
            }

            coverage_reset();
            fuzzer_stats.execs++;
            fuzzer_stats.iterations++;
            fuzzer_stats.corpus_size = corpus_size();
            fuzzer_stats.coverage_edges =
                coverage_count_edges(coverage_history, COVERAGE_MAP_SIZE);

            time_t now = time(NULL);

            if (now - last_stats >= 5) {
                double elapsed = difftime(now, fuzzer_stats.start_time);

                if (elapsed > 0) {
                    fuzzer_stats.exec_per_sec = fuzzer_stats.execs / elapsed;
                }

                printf("[*] Iteration %llu | Execs/s: %.1f | Coverage: %u | Corpus: %u | "
                       "Crashes: %u\n",
                       (unsigned long long)fuzzer_stats.iterations,
                       fuzzer_stats.exec_per_sec,
                       fuzzer_stats.coverage_edges, fuzzer_stats.corpus_size,
                       fuzzer_stats.crash_count);

                save_stats();
                last_stats = now;
            }
        }
    }

    corpus_save(corpus_dir);
    printf("[*] Fuzzing stopped\n");
}

void fuzzer_show_stats(void) {
    time_t elapsed = time(NULL) - fuzzer_stats.start_time;

    printf("\n");
    printf("═══════════════════════════════════════════════════════\n");
    printf("                   Fuzzer Statistics                    \n");
    printf("═══════════════════════════════════════════════════════\n");
    printf("Total Iterations:     %llu\n", (unsigned long long)fuzzer_stats.iterations);
    printf("Total Executions:     %llu\n", (unsigned long long)fuzzer_stats.execs);
    printf("Execution Speed:      %.1f execs/sec\n", fuzzer_stats.exec_per_sec);
    printf("Elapsed Time:         %ld seconds\n", (long)elapsed);
    printf("Coverage Edges:       %u\n", fuzzer_stats.coverage_edges);
    printf("Corpus Size:          %u\n", fuzzer_stats.corpus_size);
    printf("Crashes Found:        %u\n", fuzzer_stats.crash_count);
    printf("Hangs Detected:       %u\n", fuzzer_stats.hang_count);
    printf("═══════════════════════════════════════════════════════\n");
    printf("\n");
}

void fuzzer_cleanup(void) {
    corpus_cleanup();
    coverage_cleanup();

    free(target_path);
    free(input_dir);
    free(output_dir);
    free(crashes_dir);
    free(corpus_dir);
    free(hangs_dir);

    target_path = input_dir = output_dir = NULL;
    crashes_dir = corpus_dir = hangs_dir = NULL;
}
