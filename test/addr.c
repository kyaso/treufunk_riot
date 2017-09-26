/**
 * K. Yasotharan
 * Adapted after test program for AT86RF2XX
 */

#include <stdio.h>
#include <stdint.h>

#include "common.h"

void print_addr(uint8_t *addr, size_t addr_len)
{
    for (int i = 0; i < addr_len; i++) {
        if (i != 0) {
            printf(":");
        }
        printf("%02x", (unsigned)addr[i]);
    }
}
