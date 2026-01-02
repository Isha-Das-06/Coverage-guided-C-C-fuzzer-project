#include "corpus.h"
#include "mutator.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_CORPUS_ENTRIES 10000
#define MAX_ENTRY_SIZE 256000

static corpus_entry_t corpus[MAX_CORPUS_ENTRIES];
static u32 corpus_count = 0;

void corpus_init(void) {
    corpus_count = 0;
    memset(corpus, 0, sizeof(corpus));
}

void corpus_load(const char *directory) {
    DIR *dir = opendir(directory);

    if (!dir) {
        fprintf(stderr, "[!] Could not open input directory: %s\n", directory);
        return;
    }

    struct dirent *entry;
    u32 loaded = 0;

    while ((entry = readdir(dir)) != NULL && corpus_count < MAX_CORPUS_ENTRIES) {
        if (entry->d_name[0] == '.') continue;

        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", directory, entry->d_name);

        /* Skip subdirectories and other non-regular entries; opening a
           directory with fopen("rb") succeeds on some systems. */
        struct stat st;

        if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) continue;

        FILE *f = fopen(path, "rb");

        if (!f) continue;

        fseek(f, 0, SEEK_END);
        long file_size = ftell(f);
        fseek(f, 0, SEEK_SET);

        if (file_size <= 0 || file_size > MAX_ENTRY_SIZE) {
            fclose(f);
            continue;
        }

        u8 *data = (u8 *)malloc(file_size);

        if (!data) {
            fclose(f);
            continue;
        }

        size_t read = fread(data, 1, file_size, f);
        fclose(f);

        if ((long)read == file_size) {
            corpus[corpus_count].data = data;
            corpus[corpus_count].size = (u32)file_size;
            corpus[corpus_count].id = corpus_count;
            corpus[corpus_count].exec_count = 0;
            corpus[corpus_count].coverage_hash = 0;
            corpus_count++;
            loaded++;
        } else {
            free(data);
        }
    }

    closedir(dir);
    printf("[+] Loaded %u seed(s) from %s\n", loaded, directory);
}

void corpus_add(const u8 *data, u32 size, u64 cov_hash) {
    if (corpus_count >= MAX_CORPUS_ENTRIES) {
        return;
    }

    if (size > MAX_ENTRY_SIZE) {
        size = MAX_ENTRY_SIZE;
    }

    for (u32 i = 0; i < corpus_count; i++) {
        if (corpus[i].coverage_hash == cov_hash &&
            corpus[i].size == size &&
            memcmp(corpus[i].data, data, size) == 0) {
            return;
        }
    }

    u8 *new_data = (u8 *)malloc(size);

    if (!new_data) return;

    memcpy(new_data, data, size);
    corpus[corpus_count].data = new_data;
    corpus[corpus_count].size = size;
    corpus[corpus_count].id = corpus_count;
    corpus[corpus_count].exec_count = 0;
    corpus[corpus_count].coverage_hash = cov_hash;
    corpus_count++;
}

corpus_entry_t *corpus_pick_random(void) {
    if (corpus_count == 0) {
        return NULL;
    }

    u32 idx = random_range(0, corpus_count - 1);
    return &corpus[idx];
}

u32 corpus_size(void) {
    return corpus_count;
}

void corpus_get_entry(u32 index, corpus_entry_t *out) {
    if (index < corpus_count && out) {
        memcpy(out, &corpus[index], sizeof(corpus_entry_t));
    }
}

void corpus_save(const char *directory) {
    for (u32 i = 0; i < corpus_count; i++) {
        char path[1024];
        snprintf(path, sizeof(path), "%s/id:%06u,cov:%016llx", directory, i,
                 (unsigned long long)corpus[i].coverage_hash);

        FILE *f = fopen(path, "wb");

        if (f) {
            fwrite(corpus[i].data, 1, corpus[i].size, f);
            fclose(f);
        }
    }
}

void corpus_cleanup(void) {
    for (u32 i = 0; i < corpus_count; i++) {
        free(corpus[i].data);
        corpus[i].data = NULL;
    }

    corpus_count = 0;
}

void corpus_stats(void) {
    u32 total_size = 0;

    for (u32 i = 0; i < corpus_count; i++) {
        total_size += corpus[i].size;
    }

    printf("[*] Corpus: %u entries, %.1f KB total\n", corpus_count, total_size / 1024.0);
}
