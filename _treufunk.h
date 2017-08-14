/**
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

/**
 *	Maximum frame size (default = 127 bytes) (w/o SHR + PHR)
 */
#define TREUFUNK_MAX_PKT_LENGTH        (IEEE802154_FRAME_LEN_MAX)

/**
 * 	phy_status byte masks
 */
#define PHY_SM_STATUS   (0xE0)
#define PHY_SM_ENABLED  (0x10)
#define PHY_FIFO_EMPTY  (0x08)
#define PHY_FIFO_FULL   (0x04)
#define PHY_TRX_RUNNING (0x02)
#define PHY_PLL_ON      (0x01)

/**
 * 	state machine states as returned by the phy_status byte (see Tab. 3.4, phy_status, SysSpec)
 */
#define DEEP_SLEEP  (1)
#define SLEEP       (2)
#define BUSY        (3)
#define TX_RDY      (4)
#define SENDING     (5)
#define RX_RDY      (6)
#define RECEIVING   (7)


/**
 * 	TX modes, value matches with position of TX_MODE in SM_TX_SET, sub_reg_mask=0x30
 */
#define TX_MODE_24    (0x00)
#define TX_MODE_800   (0x10)
#define TX_MODE_433   (0x20)
//#define TX_MODE_433   (0x30)/* 433 appears 2 times in table at SM_TX_SET */

/**
 * 	Channel configuration
 */
#define TREUFUNK_MIN_CHANNEL 			(IEEE802154_CHANNEL_MIN)
#define TREUFUNK_MAX_CHANNEL           	(IEEE802154_CHANNEL_MAX)
#define TREUFUNK_DEFAULT_CHANNEL       	(IEEE802154_DEFAULT_CHANNEL)

/**
 * 	Default TX power (0dBm)
 */
#define TREUFUNK_DEFAULT_TXPOWER 		(IEEE802154_DEFAULT_TXPOWER)


/**
 * 	Internal device option flags
 * 	'0x00ff' is reserved for general IEEE802154 flags
 * 	(see netdev_ieee802154_t in drivers/include/net/netdev/ieee802154.h)
 *
 * These flags are use for telling the MAC layer that the corresponding event happened.
 */
#define TREUFUNK_OPT_TELL_TX_START 	(0x0100)
#define TREUFUNK_OPT_TELL_TX_END	(0x0200)
#define TREUFUNK_OPT_TELL_RX_START	(0x0400)
#define TREUFUNK_OPT_TELL_RX_END	(0x0800)

/* IEEE 802154 Synchronization header. 4 bytes preamble, 1 byte Start frame delimiter (SFD)
	see IEEE802154 Standard Doc Chap. 12.1 (O-QPSK PHY, PPDU format)
*/
static uint8_t SHR[] = {0x00, 0x00, 0x00, 0x00, 0xA7};

/**
 * 	SPI parameters
 */
typedef struct treufunk_params {
  spi_t spi; /* SPI bus the device is connected to */
  spi_clk_t spi_clk; /* SPI clock frequency */
  spi_cs_t cs_pin; /* GPIO pin connected to chip select */
} treufunk_params_t;

/* TODO (treufunk_t): Maybe add more */
// device descriptor
typedef struct {
  netdev_ieee802154_t netdev; /* netdev parent struct */
  treufunk_params_t params; /* spi params for initialization */
  /* TODO (treufunk_t/state): This is only set when executing set_state() [...]
  Currently there is no mechanism that changes is during the automatic
  transition from TX to RX. Maybe we don't need this variable at all. */
  uint8_t state; /* current state of state machine; phy_status, Tab. 3.4 */
} treufunk_t;



void treufunk_setup(treufunk_t *dev, const treufunk_params_t *params);
int treufunk_reset(treufunk_t *dev);

/** Addresses can only be managed by driver, because Treufunk does not support addresses */
uint16_t treufunk_get_addr_short(treufunk_t *dev);
void treufunk_set_addr_short(treufunk_t *dev, uint16_t addr);
uint64_t treufunk_get_addr_long(treufunk_t *dev);
void treufunk_set_addr_long(treufunk_t *dev, uint64_t addr);

/*TODO */ uint8_t treufunk_get_chan(treufunk_t *dev);
/*TODO */ void treufunk_set_chan(treufunk_t *dev, uint8_t chan);
/*TODO */ uint8_t treufunk_get_page(treufunk_t *dev);
/*TODO */ void treufunk_set_page(treufunk_t *dev, uint8_t page);
/*TODO */ uint16_t treufunk_get_pan(treufunk_t *dev);
/*TODO */ void treufunk_set_pan(treufunk_t *dev, uint16_t pan);

/*TODO */ int16_t treufunk_get_txpower(treufunk_t *dev);
void treufunk_set_txpower(treufunk_t *dev, uint8_t txpower);

void treufunk_set_option(treufunk_t *dev, uint16_t option, bool state);
uint8_t treufunk_get_state(treufunk_t *dev);
void treufunk_set_state(treufunk_t *dev, uint8_t state_cmd);
void treufunk_reset_state_machine(treufunk_t *dev);

void treufunk_reset_fifo(treufunk_t *dev); /* TODO: remove if not needed */


size_t treufunk_send(treufunk_t *dev, uint8_t *data, size_t len);
void treufunk_tx_prepare(treufunk_t *dev, size_t phr);
size_t treufunk_tx_load(treufunk_t *dev, uint8_t *data, size_t len);
void treufunk_tx_exec(treufunk_t *dev);



#ifdef __cplusplus
}
#endif

#endif /* TREUFUNK_H */
