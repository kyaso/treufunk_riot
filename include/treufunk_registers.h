/*
*   treufunk_registers.h
*   Kirthihan Yasotharan

*   Register definitions
*/

#ifndef TREUFUNK_REGISTERS_H
#define TREUFUNK_REGISTERS_H

#ifdef __cplusplus
extern "C" {
#endif


// SPI
#define TREUFUNK_ACCSESS_REG_READ 	    (0x80)
#define TREUFUNK_ACCSESS_REG_WRITE	    (0xC0)
#define TREUFUNK_ACCSESS_FRAME_READ	    (0x20)
#define TREUFUNK_ACCSESS_FRAME_WRITE	(0x60)



// registers
#include "lprf_registers.h"



#ifdef __cplusplus
}
#endif

#endif /* TREUFUNK_REGISTERS_H */
