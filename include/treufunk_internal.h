/*
* treufunk_internal.h
* Kirthihan Yasotharan
*
* Internal interfaces for the Treufunk driver
*/


#ifndef TREUFUNK_INTERNAL_H
#define TREUFUNK_INTERNAL_H

#include <stdint.h>

#include "treufunk.h"


#ifdef __cplusplus
extern "C" {
#endif

/* Due to a bug in the interface between Arduino and Treufunk we put a shift register inbetween both. The Treufunk writes into it and Arduino reads it out serially */
// #if DUE_SR_MODE
//     uint8_t due_shift_read(const treufunk_t *dev);
// #endif

uint8_t treufunk_reg_read(const treufunk_t *dev, const uint8_t addr);
void treufunk_reg_write(const treufunk_t *dev, const uint8_t addr, const uint8_t value);

uint8_t treufunk_sub_reg_read(const treufunk_t *dev, const uint8_t reg_addr, const uint8_t sub_reg_mask, const uint8_t offset);
void treufunk_sub_reg_write(const treufunk_t *dev, const uint8_t reg_addr, const uint8_t sub_reg_mask, const uint8_t offset, const uint8_t value);

void treufunk_fifo_read(/*const*/ treufunk_t *dev, uint8_t *buf, const size_t buf_len);
void treufunk_fifo_write(/*const*/ treufunk_t *dev, const uint8_t *data, const size_t len);

uint8_t treufunk_get_phy_status(const treufunk_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* TREUFUNK_INTERNAL_H */
