/*
* trefunk.h
* Kirthihan Yasotharan
*
* Interface definition for the Treufunk driver
*/


#ifndef TREUFUNK_H
#define TREUFUNK_H

#include <stdint.h>
#include <stdbool.h>

#include "board.h"
#include "periph/spi.h"
#include "periph/gpio.h"
#include "net/netdev.h"
#include "net/netdev/ieee802154.h"
#include "net/gnrc/nettype.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

/* Set this to 1, if you want to use the shift register workaround with Arduino Due */
#define DUE_SR_MODE (0)

/* TEMP, reg_check */
#define RETURN_ON_ERROR(f) ret = (f); \
	if (ret) { \
		return ret; \
}


#ifdef __cplusplus
extern "C" {
#endif

/* TODO: Parameter Defines */

// phy_status masks
#define PHY_SM_STATUS   (0xE0)
#define PHY_SM_ENABLED  (0x10)
#define PHY_FIFO_EMPTY  (0x08)
#define PHY_FIFO_FULL   (0x04)
#define PHY_TRX_RUNNING (0x02)
#define PHY_PLL_ON      (0x01)

// state machine states (see Tab. 3.4, phy_status, SysSpec)
#define DEEP_SLEEP  (1)
#define SLEEP       (2)
#define BUSY        (3)
#define TX_RDY      (4)
#define SENDING     (5)
#define RX_RDY      (6)
#define RECEIVING   (7)


// tx modes, value matches with position of TX_MODE in SM_TX_SET, sub_reg_mask=0x30*/
#define TX_MODE_24    (0x00)
#define TX_MODE_800   (0x10)
#define TX_MODE_433   (0x20)
//#define TX_MODE_433   (0x30)/* 433 appears 2 times in table at SM_TX_SET */

/* 0 dBm */
#define TREUFUNK_DEFAULT_TXPOWER (0)


// Spi parameters
typedef struct treufunk_params {
  spi_t spi; /* SPI bus the device is connected to */
  spi_clk_t spi_clk; /* SPI clock frequency */
  spi_cs_t cs_pin; /* GPIO pin connected to chip select */
} treufunk_params_t;

/* TODO */
// device descriptor
typedef struct {
  netdev_ieee802154_t netdev; /* netdev parent struct; TODO: use ieee802154? */
  treufunk_params_t params; /* spi params for initialization */
  uint8_t state; /* current state of state machine; phy_status, Tab. 3.4 */
  uint8_t tx_frame_len; /* TODO: do we need this? */

  /* TODO */
} treufunk_t;

void treufunk_setup(treufunk_t *dev, const treufunk_params_t *params);
int treufunk_reset(treufunk_t *dev);

/* following getter/setter functions need to/should be implemented in treufunk_getset.h */
//uint8_t treufunk_get_phy_status(treufunk_t *dev);
//void treufunk_set_sub_reg(treufunk_t *dev, uint8_t reg_addr, uint8_t sub_reg_mask, uint8_t value);

/* Adress feature not available on Trefunk */
//uint16_t treufunk_get_addr_short(treufunk_t *dev);
//void treufunk_set_addr_short(treufunk_t *dev, uint16_t addr);
//uint64_t treufunk_get_addr_long(treufunk_t *dev);
//void treufunk_set_addr_long(treufunk_t *dev, uint64_t addr);
/*TODO */ uint8_t treufunk_get_chan(treufunk_t *dev);
/*TODO */ void treufunk_set_chan(treufunk_t *dev, uint8_t chan);
/*TODO */ uint8_t treufunk_get_page(treufunk_t *dev);
/*TODO */ void treufunk_set_page(treufunk_t *dev, uint8_t page);
/*TODO */ uint16_t treufunk_get_pan(treufunk_t *dev);
/*TODO */ void treufunk_set_pan(treufunk_t *dev, uint16_t pan);

/*TODO */ int16_t treufunk_get_txpower(treufunk_t *dev);
void treufunk_set_txpower(treufunk_t *dev, uint8_t txpower);

uint8_t treufunk_get_state(treufunk_t *dev);
void treufunk_set_state(treufunk_t *dev, uint8_t state_cmd);
void treufunk_reset_state_machine(treufunk_t *dev);

void treufunk_reset_fifo(treufunk_t *dev); /* TODO: remove if not needed */


size_t treufunk_send(treufunk_t *dev, uint8_t *data, size_t len);
void treufunk_tx_prepare(treufunk_t *dev);
size_t treufunk_tx_load(treufunk_t *dev, uint8_t *data, size_t len);
void treufunk_tx_exec(treufunk_t *dev);



#ifdef __cplusplus
}
#endif

#endif /* TREUFUNK_H */
