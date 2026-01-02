#ifndef MUTATOR_H
#define MUTATOR_H

#include <stdint.h>
#include <stddef.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

/* Capacity of the buffers the fuzzer hands to the mutators. Mutations that
   change the length must never grow an input past this. */
#define MAX_INPUT_SIZE 256000

void mutator_init(void);

void mutate_bitflip(u8 *data, u32 size);
void mutate_byteflip(u8 *data, u32 size);
void mutate_arithmetic(u8 *data, u32 size);
void mutate_interesting(u8 *data, u32 size);
void mutate_havoc(u8 *data, u32 *size, u32 iterations);
void mutate_splice(u8 *data, u32 *size, u32 max_size, const u8 *other, u32 other_size);
void mutate_random(u8 *data, u32 *size);

u32 random_range(u32 min, u32 max);
void random_seed(u32 seed);

#endif
