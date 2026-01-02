#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    unsigned char input[256];
    size_t len = fread(input, 1, sizeof(input), stdin);

    if (len < 1) return 0;

    if (input[0] == 'A') {
        if (len > 10) {
            if (input[1] == 'B') {
                if (input[2] == 'C') {
                    if (input[3] == 'D') {
                        volatile int *null_ptr = NULL;
                        return *null_ptr;
                    }
                }
            }
        }
    }

    if (len > 5 && input[0] == 'X') {
        char small_buf[4];
        strcpy(small_buf, (const char *)input);
        return 0;
    }

    return 0;
}
