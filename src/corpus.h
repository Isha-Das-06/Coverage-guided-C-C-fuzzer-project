#ifndef CORPUS_H
#define CORPUS_H

#include <stdint.h>
#include <stddef.h>

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;

typedef struct {
    u8 *data;
    u32 size;
    u32 id;
    u32 exec_count;
    u64 coverage_hash;
} corpus_entry_t;

void corpus_init(void);
void corpus_load(const char *directory);
void corpus_add(const u8 *data, u32 size, u64 cov_hash);
corpus_entry_t *corpus_pick_random(void);
u32 corpus_size(void);
void corpus_get_entry(u32 index, corpus_entry_t *out);
void corpus_save(const char *directory);
void corpus_cleanup(void);
void corpus_stats(void);

#endif
