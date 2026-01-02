#include "fuzzer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void show_usage(const char *prog) {
    printf("Usage: %s [options]\n\n", prog);
    printf("Options:\n");
    printf("  -i <dir>       Input directory (seed corpus) [REQUIRED]\n");
    printf("  -o <dir>       Output directory (results) [REQUIRED]\n");
    printf("  -t <target>    Target binary to fuzz [REQUIRED]\n");
    printf("  -x <count>     Max iterations (default: infinite)\n");
    printf("  -T <seconds>   Timeout per execution (default: 5)\n");
    printf("  -v             Verbose output\n");
    printf("  -h             Show this help\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s -i seeds/ -o results/ -t ./target\n", prog);
    printf("  %s -i seeds/ -o results/ -t ./target -x 100000\n", prog);
    printf("\n");
}

int main(int argc, char *argv[]) {
    char *input_dir = NULL;
    char *output_dir = NULL;
    char *target = NULL;
    u32 max_iterations = 0;
    int timeout = 5;
    int verbose = 0;

    int opt;

    while ((opt = getopt(argc, argv, "i:o:t:x:T:vh")) != -1) {
        switch (opt) {
            case 'i':
                input_dir = optarg;
                break;
            case 'o':
                output_dir = optarg;
                break;
            case 't':
                target = optarg;
                break;
            case 'x': {
                long v = strtol(optarg, NULL, 10);
                if (v < 0) {
                    fprintf(stderr, "[!] Error: -x must be >= 0\n");
                    return 1;
                }
                max_iterations = (u32)v;
                break;
            }
            case 'T': {
                long v = strtol(optarg, NULL, 10);
                if (v <= 0) {
                    fprintf(stderr, "[!] Error: -T must be > 0\n");
                    return 1;
                }
                timeout = (int)v;
                break;
            }
            case 'v':
                verbose = 1;
                break;
            case 'h':
                show_usage(argv[0]);
                return 0;
            default:
                show_usage(argv[0]);
                return 1;
        }
    }

    if (!input_dir || !output_dir || !target) {
        fprintf(stderr, "[!] Error: -i, -o, and -t are required\n\n");
        show_usage(argv[0]);
        return 1;
    }

    if (access(input_dir, R_OK) != 0) {
        fprintf(stderr, "[!] Error: Cannot read input directory: %s\n", input_dir);
        return 1;
    }

    if (access(target, X_OK) != 0) {
        fprintf(stderr, "[!] Error: Cannot execute target: %s\n", target);
        return 1;
    }

    printf("╔═══════════════════════════════════════════════════════╗\n");
    printf("║     Coverage-Guided Fuzzer (Mini-AFL style)           ║\n");
    printf("╚═══════════════════════════════════════════════════════╝\n");
    printf("\n");

    fuzzer_set_verbose(verbose);
    fuzzer_init(input_dir, output_dir, target);
    fuzzer_run(timeout, max_iterations);
    fuzzer_show_stats();
    fuzzer_cleanup();

    printf("[+] Fuzzing complete!\n");
    printf("[*] Results saved to: %s\n", output_dir);

    return 0;
}
