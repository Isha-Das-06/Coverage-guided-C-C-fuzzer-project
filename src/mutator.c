#include "mutator.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>

static u32 seed_state = 0x2545f491;

/* xorshift32. The previous LCG was masked to 31 bits and then reduced with
   %, so callers saw only its low bits - and the low bits of an LCG have tiny
   periods (random_range(0,1) strictly alternated 0,1,0,1...). */
static u32 next_rand(void) {
    seed_state ^= seed_state << 13;
    seed_state ^= seed_state >> 17;
    seed_state ^= seed_state << 5;
    return seed_state;
}

void mutator_init(void) {
    random_seed((u32)time(NULL));
}

void random_seed(u32 seed) {
    /* xorshift is stuck at zero forever if seeded with zero. */
    seed_state = seed ? seed : 0x2545f491;
}

u32 random_range(u32 min, u32 max) {
    if (min >= max) return min;

    u32 span = max - min;

    if (span == UINT32_MAX) return next_rand();

    return min + (next_rand() % (span + 1));
}

void mutate_bitflip(u8 *data, u32 size) {
    if (size == 0) return;

    u32 pos = random_range(0, size - 1);
    u32 bit = random_range(0, 7);
    data[pos] ^= (1 << bit);
}

void mutate_byteflip(u8 *data, u32 size) {
    if (size == 0) return;

    u32 pos = random_range(0, size - 1);
    data[pos] ^= 0xFF;
}

void mutate_arithmetic(u8 *data, u32 size) {
    if (size < 2) return;

    u32 pos = random_range(0, size - 2);
    int delta = (int)random_range(1, 16);

    if (random_range(0, 1)) {
        delta = -delta;
    }

    u16 val;
    memcpy(&val, &data[pos], sizeof(u16));
    val = (u16)(val + delta);
    memcpy(&data[pos], &val, sizeof(u16));
}

void mutate_interesting(u8 *data, u32 size) {
    static const u8 interesting[] = {
        0x00, 0xFF, 0x7F, 0x80, 0x01, 0x10, 0x20, 0x7E, 0xFE, 0xFD
    };

    if (size == 0) return;

    u32 pos = random_range(0, size - 1);
    u32 idx = random_range(0, sizeof(interesting) - 1);

    data[pos] = interesting[idx];
}

void mutate_havoc(u8 *data, u32 *size, u32 iterations) {
    u32 i;

    for (i = 0; i < iterations; i++) {
        u32 choice = random_range(0, 6);

        switch (choice) {
            case 0:
                mutate_bitflip(data, *size);
                break;
            case 1:
                mutate_byteflip(data, *size);
                break;
            case 2:
                mutate_arithmetic(data, *size);
                break;
            case 3:
                mutate_interesting(data, *size);
                break;
            case 4:
                if (*size > 0) {
                    u32 pos = random_range(0, *size - 1);
                    data[pos] = (u8)random_range(0, 255);
                }
                break;
            case 5:
                if (*size > 1) {
                    u32 pos1 = random_range(0, *size - 1);
                    u32 pos2 = random_range(0, *size - 1);
                    u8 tmp = data[pos1];
                    data[pos1] = data[pos2];
                    data[pos2] = tmp;
                }
                break;
            case 6:
                /* Byte deletion. This used to take size by value, so the
                   memmove shifted bytes down but the input stayed the same
                   length - it just duplicated the final byte. */
                if (*size > 1) {
                    u32 del_pos = random_range(0, *size - 1);
                    memmove(&data[del_pos], &data[del_pos + 1],
                            *size - del_pos - 1);
                    (*size)--;
                }
                break;
        }
    }
}

void mutate_splice(u8 *data, u32 *size, u32 max_size, const u8 *other, u32 other_size) {
    if (*size == 0 || other_size == 0 || max_size == 0) {
        return;
    }

    u32 split_point = random_range(0, *size - 1);
    u32 split_point_other = random_range(0, other_size - 1);

    /* Bytes available from `other`, clamped to what still fits in `data`.
       Clamping new_size first (as this did) could leave new_size < split_point,
       and `new_size - split_point` then wrapped around into a huge memcpy. */
    u32 tail = other_size - split_point_other;

    if (split_point >= max_size) {
        split_point = max_size - 1;
    }

    if (tail > max_size - split_point) {
        tail = max_size - split_point;
    }

    memcpy(&data[split_point], &other[split_point_other], tail);
    *size = split_point + tail;
}

void mutate_random(u8 *data, u32 *size) {
    u32 choice = random_range(0, 3);

    switch (choice) {
        case 0:
            mutate_bitflip(data, *size);
            break;
        case 1:
            mutate_arithmetic(data, *size);
            break;
        case 2:
            mutate_havoc(data, size, random_range(1, 5));
            break;
        case 3:
            mutate_interesting(data, *size);
            break;
    }
}
