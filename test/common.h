/**
 * K. Yasotharan
 * Adapted after test program for AT86RF2XX
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

#include "treufunk.h"
// #include "at86rf2xx_params.h"
#include "net/netdev.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Application-internal functions and variables for at86rf2xx tests
 * @internal
 * @{
 */
#define TREUFUNK_NUM   2//(sizeof(at86rf2xx_params) / sizeof(at86rf2xx_params[0]))

extern treufunk_t devs[TREUFUNK_NUM];

void recv(netdev_t *dev);
int ifconfig(int argc, char **argv);
int txtsnd(int argc, char **argv);
int change_state(int argc, char **argv);
int change_fifo(int argc, char **argv);
void print_addr(uint8_t *addr, size_t addr_len);
/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* COMMON_H */
