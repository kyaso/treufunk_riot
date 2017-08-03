/*
*	treufunk_params.h
*	Kirthihan Yasotharan
*
*	(Default) SPI parameters for Treufunk
*
*
*
*
*
*/

#ifndef TREUFUNK_PARAMS_H
#define TREUFUNK_PARAMS_H

#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

// (default) parameters
#ifndef TREUFUNK_PARAM_SPI
#define TREUFUNK_PARAM_SPI	(SPI_DEV(0))
#endif
#ifndef TREUFUNK_PARAM_SPI_CLK
#define TREUFUNK_PARAM_SPI_CLK	(SPI_CLK_400KHZ) /* for now 1MHz */
#endif
#ifndef TREUFUNK_PARAM_CS
#define TREUFUNK_PARAM_CS	(GPIO_PIN(0, 28)) /* Arduino Due digital pin 10 */
#endif


#define TREUFUNK_PARAMS_DEFAULT	{.spi = TREUFUNK_PARAM_SPI, \
				.spi_clk = TREUFUNK_PARAM_SPI_CLK, \
				.cs_pin = TREUFUNK_PARAM_CS}

// Treufunk configuration
static const treufunk_params_t treufunk_params[] =
{
#ifdef TREUFUNK_PARAMS_BOARD
	TREUFUNK_PARAMS_BOARD,
#else
	TREUFUNK_PARAMS_DEFAULT,
#endif
};

#ifdef __cplusplus
}
#endif

#endif /* TREUFUNK_PARAMS_H */
