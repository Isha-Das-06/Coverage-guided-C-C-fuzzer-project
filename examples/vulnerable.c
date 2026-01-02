#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

struct header {
    uint32_t magic;
    uint32_t size;
    char data[64];
};

int process(const unsigned char *buf, size_t len) {
    if (len < 8) return -1;

    struct header *hdr = (struct header *)buf;

    if (hdr->magic == 0xDEADBEEF) {
        if (hdr->size < 64) {
            char local[32];
            memcpy(local, hdr->data, hdr->size);
            if (hdr->size > 10 && local[10] == 'X') {
                return 1;
            }
        }
    }

    if (hdr->magic == 0x12345678 && len > 16) {
        volatile int *bad = (int *)(intptr_t)buf[8];
        return *bad;
    }

    return 0;
}

int main(void) {
    unsigned char input[256];
    size_t len = fread(input, 1, sizeof(input), stdin);
    if (len > 0) {
        process(input, len);
    }
    return 0;
}
