/*
* treufunk_netdev.h
*
* Kirthihan Yasotharan
*
* Netdev adaption for the Treufunk driver
*
*/

#include <string.h>
#include <assert.h>
#include <errno.h>

#include "net/eui64.h"
#include "net/ieee802154.h"
#include "net/netdev.h"
#include "net/netdev/ieee802154.h"

#include "treufunk.h"
#include "treufunk_netdev.h"
#include "treufunk_internal.h"
#include "treufunk_registers.h"

// #define ENABLE_DEBUG (0)
// #include "debug.h"

//#define _MAX_MHR_OVERHEAD   (25)

/* TODO (netdev.c): Wieder auskommentieren; TEMP */
static int _send(netdev_t *netdev, const struct iovec *vector, unsigned count);
//static int _recv(netdev_t *netdev, void *buf, size_t len, void *info);
static int _init(netdev_t *netdev);
//static void _isr(netdev_t *netdev);
//static int _get(netdev_t *netdev, netopt_t opt, void *val, size_t max_len);
//static int _set(netdev_t *netdev, netopt_t opt, void *val, size_t len);

/* TEMP_BEGIN (treufunk_driver) */
const netdev_driver_t treufunk_driver = {
    .send = _send,
    //.recv = _recv,
    .init = _init,
    //.isr = _isr,
    //.get = _get,
    //.set = _set,
};
/* TEMP_END */

/* TEMP_BEGIN (_irq_handler) */
// static void _irq_handler(void *arg)
// {
//     netdev_t *dev = (netdev_t *) arg;
//
//     if (dev->event_callback) {
//         dev->event_callback(dev, NETDEV_EVENT_ISR);
//     }
// }
/* TEMP_END */

static int _init(netdev_t *netdev)
{
    DEBUG("netdev/_init()...\n");
    treufunk_t *dev = (treufunk_t *) netdev;

    /* init gpios */
    spi_init_cs(dev->params.spi, dev->params.cs_pin);
    /* For parallel out */
    // #if DUE_SR_MODE
    //     due_init_gpio();
    // #endif
    /* TODO (_init): Maybe also hardware reset pin */

    /**
     * Check if SPI is working correctly by reading the two registers
     * CHIP_ID_L and CHIP_ID_H. They contain predefined values: 0x51, 0x1A
     */
    DEBUG("Reading chip id l (at 0x04)...\n");
    uint8_t id_l = treufunk_reg_read(dev, RG_CHIP_ID_L); /* 0x04 */
    DEBUG("Reading chip id h (at 0x05)...\n");
    uint8_t id_h = treufunk_reg_read(dev, RG_CHIP_ID_H); /* 0x05 */
    DEBUG("Chip ID l = 0x%02x, Chip ID h = 0x%02x\n", id_l, id_h);
    if(id_l != 0x51 || id_h != 0x1A)
    {
        DEBUG("[treufunk] error: unable to read correct chip id\n");
        return -1;
    }

    DEBUG("Success: chip id correct! Doing reset now...\n");

    /* TEMP_BEGIN (_init): Remove the comment if you just want to read out the chip id and
    skip the whole init process */
    return -1;
    /* TEMP_END */

    /* reset Treufunk to default values and put it into RX state */
    return treufunk_reset(dev);
}

static int _send(netdev_t *netdev, const struct iovec *vector, unsigned count)
{
    DEBUG("netdev/_send()...\n");
    treufunk_t *dev = (treufunk_t *)netdev;
    const struct iovec *ptr = vector;
    size_t len = 0;

    treufunk_tx_prepare(dev);

    /* load data into FIFO */
    for(unsigned i = 0; i < count; i++, ptr++)
    {
        /* TODO (_send): Check for max length */
        len = treufunk_tx_load(dev, ptr->iov_base, ptr->iov_len);
    }

    /* send out data */
    treufunk_tx_exec(dev);

    /* return number of bytes that were send out */
    return (int)len;
}
