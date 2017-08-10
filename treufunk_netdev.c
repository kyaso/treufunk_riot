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
static void _isr(netdev_t *netdev);
static int _get(netdev_t *netdev, netopt_t opt, void *val, size_t max_len);
static int _set(netdev_t *netdev, netopt_t opt, void *val, size_t len);

const netdev_driver_t treufunk_driver = {
    .send = _send,
    //.recv = _recv,
    .init = _init,
    .isr = _isr,
    .get = _get,
    .set = _set,
};


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

static void _isr(netdev_t *netdev)
{
    /* TODO: Obviously we don't need an ISR because Treufunk has no interrupt functionality. But we still have to implement it, therefore just return

    Or do this instead: netdev->event_callback(netdev, NETDEV_EVENT_RX_COMPLETE);
    This activates the event_callback (implemented in user-space). The callback then calls treufunk_netdevs _recv() function to fetch the received packet.
    */
    return;
}

static int _send(netdev_t *netdev, const struct iovec *vector, unsigned count)
{
    DEBUG("netdev/_send()...\n");
    treufunk_t *dev = (treufunk_t *)netdev;
    size_t len = 0;

    treufunk_tx_prepare(dev);

    /* TODO (_send): Maybe check for length first, instead of immediately starting to write into FIFO and checking for length while doing so */
    /* load data into FIFO */
    for(unsigned i = 0; i < count; i++, vector++)
    {
        if((len + vector->iov_len + 2) > TREUFUNK_MAX_PKT_LENGTH)
        {
            /* current packet data + FCS too long */
            DEBUG("[treufunk] error: packet too large (%u bytes) to be send\n", (unsigned)len + 2);
            return -EOVERFLOW;
        }
        len += treufunk_tx_load(dev, vector->iov_base, vector->iov_len);
    }

    /* send out data */
    treufunk_tx_exec(dev);

    /* return number of bytes that were send out */
    return (int)len;
}

static int _set_state(treufunk_t *dev, netopt_state_t state)
{
    switch(state)
    {
        case NETOPT_STATE_SLEEP:
            treufunk_set_state(dev, STATE_CMD_SLEEP);
            break;
        case NETOPT_STATE_TX:
            treufunk_tx_exec(dev);
            break;
        case NETOPT_STATE_RESET:
            treufunk_reset(dev);
            break;
        default:
            return -ENOTSUP;
    }

    return sizeof(netopt_state_t);
}

/* Some NETOPT states where added manually to include all possible states of Treufunk. See netopt.h */
netopt_state_t _get_state(treufunk_t *dev)
{
    switch(treufunk_get_state(dev))
    {
        case DEEP_SLEEP:
            return NETOPT_STATE_DEEPSLEEP;
        case SLEEP:
            return NETOPT_STATE_SLEEP;
        case BUSY:
            return NETOPT_STATE_BUSY;
        case TX_RDY:
            return NETOPT_STATE_TXRDY;
        case SENDING:
            return NETOPT_STATE_RX;
        case RX_RDY:
            return NETOPT_STATE_RXRDY;
        case RECEIVING:
            return NETOPT_STATE_RX;
        default:
            return NETOPT_STATE_IDLE;
    }
}

static int _get(netdev_t *netdev, netopt_t opt, void *val, size_t max_len)
{
    int res = -ENOTSUP;
    if(netdev == NULL) return -ENODEV;

    treufunk_t *dev = (treufunk_t *)netdev;

    switch(opt)
    {
        case NETOPT_CHANNEL:
            /* TODO (_get) */
            break;

        case NETOPT_TX_POWER:
            assert(max_len >= sizeof(uint16_t));
            *((uint16_t *)val) = treufunk_get_txpower(dev);
            res = sizeof(uint16_t);
            break;

        case NETOPT_STATE:
            assert(max_len >= sizeof(netopt_state_t));
            *((netopt_state_t *)val) = _get_state(dev);
            res = sizeof(netopt_state_t);
            break;

        case NETOPT_RX_START_IRQ:
            *((netopt_enable_t *)val) = !!(dev->netdev.flags & TREUFUNK_OPT_TELL_RX_START); /* !!(s) converts s into a boolean */
            res = sizeof(netopt_enable_t);
            break;

        case NETOPT_RX_END_IRQ:
            *((netopt_enable_t *)val) = !!(dev->netdev.flags & TREUFUNK_OPT_TELL_RX_END);
            res = sizeof(netopt_enable_t);
            break;

        case NETOPT_TX_START_IRQ:
            *((netopt_enable_t *)val) = !!(dev->netdev.flags & TREUFUNK_OPT_TELL_TX_START);
            res = sizeof(netopt_enable_t);
            break;

        case NETOPT_TX_END_IRQ:
            *((netopt_enable_t *)val) = !!(dev->netdev.flags & TREUFUNK_OPT_TELL_TX_END);
            res = sizeof(netopt_enable_t);
            break;

        default:
            break;
    }

    /* fall back to IEEE802154 */
    if(res == -ENOTSUP)
    {
        res = netdev_ieee802154_get(&(dev->netdev), opt, val, max_len);
    }

    return res;
}

static int _set(netdev_t *netdev, netopt_t opt, void *val, size_t len)
{
    int res = -ENOTSUP;
    if(netdev == NULL) return -ENODEV;

    treufunk_t *dev = (treufunk_t *) netdev;

    switch(opt)
    {
        case NETOPT_CHANNEL:
            assert(len == sizeof(uint8_t));
            uint8_t chan = ((uint8_t *)val)[0];
            if((chan < TREUFUNK_MIN_CHANNEL) ||
                (chan > TREUFUNK_MAX_CHANNEL)) {
                    res = -EINVAL;
                    break;
            }
            treufunk_set_chan(dev, chan);
            break;

        case NETOPT_TX_POWER:
            assert(len == sizeof(uint8_t));
            treufunk_set_txpower(dev, *((uint8_t *)val));
            res = sizeof(uint8_t);
            break;

        case NETOPT_STATE:
            assert(len == sizeof(netopt_state_t));
            res = _set_state(dev, *((netopt_state_t *)val));
            break;

        /**
         *  Altough the Treufunk does not support interrupts,
         *  the following flags can be used by the driver to
         *  signal the MAC layer that one of the four events
         *  happened.
         */
        case NETOPT_RX_START_IRQ:
            treufunk_set_option(dev, TREUFUNK_OPT_TELL_RX_START, *((bool *)val));
            res = sizeof(netopt_enable_t);
            break;

        case NETOPT_RX_END_IRQ:
            treufunk_set_option(dev, TREUFUNK_OPT_TELL_RX_END, *((bool *)val));
            res = sizeof(netopt_enable_t);
            break;

        case NETOPT_TX_START_IRQ:
            treufunk_set_option(dev, TREUFUNK_OPT_TELL_TX_START, *((bool *)val));
            res = sizeof(netopt_enable_t);
            break;

        case NETOPT_TX_END_IRQ:
            treufunk_set_option(dev, TREUFUNK_OPT_TELL_TX_END, *((bool *)val));
            res = sizeof(netopt_enable_t);
            break;

        default:
            break;
    }

    /* fall back to IEEE802154 */
    if(res == -ENOTSUP)
    {
        res = netdev_ieee802154_set(&(dev->netdev), opt, val, len);
    }

    return res;
}
