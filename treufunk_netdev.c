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

static int _send(netdev_t *netdev, const struct iovec *vector, unsigned count);
static int _recv(netdev_t *netdev, void *buf, size_t len, void *info);
static int _init(netdev_t *netdev);
static void _isr(netdev_t *netdev);
static int _get(netdev_t *netdev, netopt_t opt, void *val, size_t max_len);
static int _set(netdev_t *netdev, netopt_t opt, void *val, size_t len);

const netdev_driver_t treufunk_driver = {
    .send = _send,
    .recv = _recv,
    .init = _init,
    .isr = _isr,
    .get = _get,
    .set = _set,
};

static void _irq_handler(void *arg)
{
    netdev_t *dev = (netdev_t *) arg;

    if (dev->event_callback) {
        dev->event_callback(dev, NETDEV_EVENT_ISR);
    }
}


static int _init(netdev_t *netdev)
{
    DEBUG("netdev/_init()...\n");
    treufunk_t *dev = (treufunk_t *) netdev;

    /* Setup polling timer */
    dev->poll_timer.next = NULL;
    dev->poll_timer.callback = _irq_handler;
    dev->poll_timer.arg = (void *)dev;
    /* init gpios */
    // spi_init_cs(dev->params.spi, dev->params.cs_pin);
    /* TODO (_init): Maybe also hardware reset pin */

    /**
     * Check if SPI is working correctly by reading the two registers
     * CHIP_ID_L and CHIP_ID_H. They contain predefined values: 0x51, 0x1A
     */
    DEBUG("Reading chip id l (at 0x04)...\n");
    // uint8_t id_l = treufunk_reg_read(dev, RG_CHIP_ID_L); /* 0x04 */
    DEBUG("Reading chip id h (at 0x05)...\n");
    // uint8_t id_h = treufunk_reg_read(dev, RG_CHIP_ID_H); /* 0x05 */
    // DEBUG("Chip ID l = 0x%02x, Chip ID h = 0x%02x\n", id_l, id_h);
    // if(id_l != 0x51 || id_h != 0x1A)
    // {
    //     DEBUG("[treufunk] error: unable to read correct chip id\n");
    //     return -1;
    // }

    DEBUG("Success: chip id correct! Doing reset now...\n");

    /* TEMP_BEGIN (_init): Remove the comment if you just want to read out the chip id and
    skip the whole init process */
    // return -1;
    /* TEMP_END */

    /* reset Treufunk to default values and put it into RX state */
    return treufunk_reset(dev);
}

/* TODO (_isr) */
static void _isr(netdev_t *netdev)
{
    treufunk_t *dev = (treufunk_t *)netdev;

    uint8_t phy_status = treufunk_get_phy_status(dev);

    /* Check if RX data is available */
    if(PHY_SM_STATUS(phy_status) == SLEEP && !PHY_FIFO_EMPTY(phy_status))
    {
        DEBUG("POLL: EVT - RX_END\n");
        if (!(dev->netdev.flags & TREUFUNK_OPT_TELL_RX_END)) {
            return;
        }
        netdev->event_callback(netdev, NETDEV_EVENT_RX_COMPLETE);
        phy_status = treufunk_get_phy_status(dev);
    }

    /* Change to back RX, if RX data is transferred to driver and chip is sleeping */
    if(PHY_SM_STATUS(phy_status) == SLEEP && PHY_FIFO_EMPTY(phy_status))
    {
        treufunk_set_state(dev, RECEIVING);
        return;
    }

    /* Check if transmission is complete */
    if(PHY_SM_STATUS(phy_status) == RECEIVING && PHY_FIFO_EMPTY(phy_status) && dev->tx_active)
    {
        DEBUG("POLL: EVT - TX_END\n");

        dev->tx_active = false;

        if (!(dev->netdev.flags & TREUFUNK_OPT_TELL_TX_END)) {
            return;
        }
        netdev->event_callback(netdev, NETDEV_EVENT_TX_COMPLETE);

        /* Change back to RX */
        treufunk_set_state(dev, RECEIVING);

        return;
    }

    DEBUG("POLL: nothing happened. Setting timer again...\n");
    /* Set timer again if still listening for packets OR waiting for transmission to finish */
    xtimer_set(&(dev->poll_timer), RX_POLLING_INTERVAL);

}

static int _send(netdev_t *netdev, const struct iovec *vector, unsigned count)
{
    DEBUG("netdev/_send()...\n");
    treufunk_t *dev = (treufunk_t *)netdev;
    size_t len = 2; /* 2 bytes FCS */

    /* determine length of payload. This value is PHR later */
    for(unsigned i = 0; i < count; i++)
    {
        len += vector[i].iov_len;
    }
    /* check if packet is too long */
    if(len > TREUFUNK_MAX_PKT_LENGTH)
    {
        DEBUG("[treufunk] error: packet too large (%u bytes) to be send\n", (unsigned)len + 2);
        return -EOVERFLOW;
    }

    /* put SM into SLEEP and write SHR + PHR into FIFO */
    treufunk_tx_prepare(dev, len);

    /* load payload (PSDU) data into FIFO */
    for(unsigned i = 0; i < count; i++, vector++)
    {
        treufunk_tx_load(dev, vector->iov_base, vector->iov_len);
    }

    /* send out data */
    treufunk_tx_exec(dev);

    /* return number of bytes that were send out */
    return (int)len;
}

/**
 * Calculates the number of equal bits in x1 and x2. Needed for SFD detection (see below)
 */
static inline uint8_t _number_of_equal_bits(uint32_t x1, uint32_t x2, uint8_t number_of_bits)
{
    uint8_t i = 0;
	uint8_t counter = 0;
	uint32_t combined = ~(x1 ^ x2);

	for (i = 0; i < number_of_bits; ++i) {
		counter += combined & 1;
		combined >>= 1;
	}
    return counter;
}

/**
 * Calculates the data shift from the start of frame delimiter
 * @param  data            rx_data received from chip. Bit order and polarity needs to be
 * 	                       already corrected
 * @param  data_length     number of bytes in data
 * @param  sfd             start of frame delimiter, i.e. 0xA7
 * @param  preamble_length number of octets in the preamble. The maximum supported
 * 	                       length is 4 octets
 * @return                 the shift value (6/7/8) or zero if sfd was not found
 *
 * The lprf chip (^= Treufunk) has a hardware preamble detection. However, the hardware
 * preamble detection is limited to a 8 bit preamble (or optionally 5 bit). In
 * the IEEE 802.15.4 standard the preamble has a length of 4 byte.
 * Additionally the data from the chip is sometimes misaligned by one bit.
 * Therefore the additional preamble bits have to be removed and the
 * misalignment has to be adjusted in software.
 */
static int _find_SFD_and_shift_data(uint8_t *data, uint8_t *data_length,
		uint8_t sfd, uint8_t preamble_length)
{
	uint8_t i;
	uint8_t sfd_start_postion = preamble_length - 1;
	uint8_t data_start_position = sfd_start_postion + 1;
	uint8_t shift = 8;
	uint8_t no_shift, one_bit_shift, two_bit_shift;

	if (*data_length < sfd_start_postion + 2)
		return -EFAULT;

	no_shift = _number_of_equal_bits(sfd, data[sfd_start_postion], 8);
	one_bit_shift = _number_of_equal_bits(sfd,
				((data[sfd_start_postion] << 8) |
				data[sfd_start_postion+1]) >> 7, 8);
	two_bit_shift = _number_of_equal_bits(sfd,
				((data[sfd_start_postion] << 8) |
				data[sfd_start_postion+1]) >> 6, 8);

	if(no_shift < 7 && one_bit_shift < 7 && two_bit_shift < 7) {
		DEBUG("SFD not found.\n");
		return 0;
	}

	if (one_bit_shift >= 7)
		shift -= 1;
	else if (two_bit_shift >= 7)
		shift -= 2;

	DEBUG("Data will be shifted by %d bits to the right\n", shift);

	for (i = 0; i < *data_length - sfd_start_postion - 1; ++i) {
		data[i] = ((data[i + data_start_position] << 8) |
				data[i + data_start_position + 1]) >> shift;
	}
	*data_length -= (sfd_start_postion + 1);

	return shift;
}

/**
 * Gets the received data from the FIFO.
 *
 * Since the Treufunk does not handle it automatically, we have
 * to remove the preamble etc. manually.
 *
 * @param  buf    The buffer in which the received data is written
 * @param  len    Length of the receive buffer.
 *                Because we have to read out the whole FIFO first,
 *                the buffer should be at least of size
 *                  TREUFUNK_MAX_PKT_LENGTH + 6 + 2
 *                6 = SHR length
 *                2 additional bytes because of possible misalignment
 * @param  info   [description]
 * @return        [description]
 */
/* TODO (_recv): If buf == NULL this function should just return the size of the frame. For now we don't support this feature */
static int _recv(netdev_t *netdev, void *buf, size_t len, void *info)
{
    /* TODO
     1. read received data from FIFO (into buf)
     2. remove additional preamble bits (because Treufunk only has 8 bit preamble detection) and correct misaligned bits
     3. get length of received packet (PHR)
     4. if (buf) == NULL: just return length of data in FIFO (not supported, see description)
     (5. read out data to (buf))

    */

    treufunk_t *dev = (treufunk_t *)netdev;
    uint8_t phr;
    size_t pkt_len;


    /* read FIFO data into buf */
    treufunk_fifo_read(dev, (uint8_t*)buf, len);

    /* remove preamble and correct alignment */
    if(_find_SFD_and_shift_data(buf, &len, 0xA7, 4) == 0)
    {
        DEBUG("SFD not found, ignoring frame\n");
		return -EINVAL;
    }

    #if ENABLE_DEBUG
        /* print out memory dump of buffer */
        od_hex_dump(buf, len, 16);
    #endif

    /* get PHR (size of received packet) */
    phr = *((uint8_t *)buf);
    pkt_len = (phr & 0x7f) - 2; /* only 7 bits are needed for phr (0-127). Subtract length of FCS as it can be discarded */

    /* Move payload to front of buffer */
    memmove(buf, ((uint8_t *)buf) + 1, pkt_len);

    /* TODO (_recv) */ //if(buf == NULL) return pkt_len;

    // if(pkt_len > len)
    // {
    //     DEBUG("ERRROR _recv(): Not enough space in receive buffer!\n");
    //     return -ENOBUFS;
    // }


    /* do info processing */


    return pkt_len;
}

static int _set_state(treufunk_t *dev, netopt_state_t state)
{
    switch(state)
    {
        case NETOPT_STATE_SLEEP:
            treufunk_set_state(dev, SLEEP);
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
        case NETOPT_CHANNEL_PAGE:
            assert(max_len >= sizeof(uint16_t));
            ((uint8_t *)val)[1] = 0;
            ((uint8_t *)val)[0] = 0;
            res = sizeof(uint16_t);
            break;

        /* TODO (_get) */ case NETOPT_MAX_PACKET_SIZE:
            break;


        // case NETOPT_TX_POWER:
        //     assert(max_len >= sizeof(uint8_t));
        //     *((uint8_t *)val) = treufunk_get_txpower(dev);
        //     res = sizeof(uint8_t);
        //     break;

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
        // case NETOPT_CHANNEL:
        //     assert(len == sizeof(uint8_t));
        //     uint8_t chan = ((uint8_t *)val)[0];
        //     if((chan < IEEE802154_CHANNEL_MIN) ||
        //         (chan > IEEE802154_CHANNEL_MAX)) {
        //             res = -EINVAL;
        //             break;
        //     }
        //     treufunk_set_chan(dev, chan);
        //     break;

        case NETOPT_CHANNEL_PAGE:
            assert(len != sizeof(uint8_t));
            uint8_t page = ((uint8_t *)val)[0];
            /* We only support channel page 0 */
            if(page != 0)
            {
                res = -EINVAL;
            }
            else
            {
                res = sizeof(uint8_t);
            }
            break;

        // case NETOPT_TX_POWER:
        //     assert(len == sizeof(uint8_t));
        //     treufunk_set_txpower(dev, *((uint8_t *)val));
        //     res = sizeof(uint8_t);
        //     break;

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
