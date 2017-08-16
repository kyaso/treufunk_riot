/*
* treufunk_getset.c
*
* Kirthihan Yasotharan
*
* Getter and setter functions for the Trefunk driver
*
*/

#include "treufunk.h"
#include "treufunk_internal.h"
#include "treufunk_registers.h"
#include "periph/spi.h"

// #define ENABLE_DEBUG (0)
// #include "debug.h"

/* TODO (tx_pow_to_dbm) */
static const int8_t tx_pow_to_dbm[??] = { ... };

int16_t treufunk_get_txpower(treufunk_t *dev)
{
    uint8_t txpower = treufunk_sub_reg_read(dev, SR_TX_PWR_CTRL);
    /* TODO (get_txpower): Convert to dBm */
    return tx_pow_to_dbm[txpower];
}

uint16_t treufunk_get_addr_short(treufunk_t *dev)
{
    return (dev->netdev.short_addr[0] << 8) | dev->netdev.short_addr[1];
}

void treufunk_set_addr_short(treufunk_t *dev, uint16_t addr)
{
    dev->netdev.short_addr[0] = (uint8_t)(addr);
    dev->netdev.short_addr[1] = (uint8_t)(addr >> 8);
#ifdef MODULE_SIXLOWPAN
    /* https://tools.ietf.org/html/rfc4944#section-12 requires the first bit to
     * 0 for unicast addresses */
    dev->netdev.short_addr[0] &= 0x7F;
#endif
}

uint64_t treufunk_get_addr_long(treufunk_t *dev)
{
    uint64_t addr;
    uint8_t *ap = (uint8_t *)(&addr);
    for (int i = 0; i < 8; i++) {
        ap[i] = dev->netdev.long_addr[i];
    }
    return addr;
}

void treufunk_set_addr_long(treufunk_t *dev, uint64_t addr)
{
    for (int i = 0; i < 8; i++) {
        dev->netdev.long_addr[i] = (uint8_t)(addr >> (i * 8));
    }
}

uint16_t treufunk_get_pan(treufunk_t *dev)
{
    return dev->netdev.pan;
}

void treufunk_set_pan(treufunk_t *dev, uint16_t pan)
{
    dev->netdev.pan = pan;
}

/**
 * Sets the desired TX Power by writing into the SM_TX_POWER_CTRL (0x8B) register.
 * @param txpower The desired TX power. Valid values: 0 .. 15 (0dBm .. 15dBm)
 */
void treufunk_set_txpower(treufunk_t *dev, uint8_t txpower)
{
    DEBUG("Setting tx_power...\n");
    if(txpower < 0) txpower = 0;
    else if(txpower > 15) txpower = 15;

    treufunk_sub_reg_write(dev, SR_TX_PWR_CTRL, txpower);

}


/**
 * Returns the state value corresponding to Tab. 3.4, SysSpec (phy_status)
 */
uint8_t treufunk_get_state(treufunk_t *dev)
{
    /* right shift 5 neccessary because state bits are the first three bits of phy_status */
    DEBUG("Getting current state...\n");
    uint8_t state = (treufunk_get_phy_status(dev) & PHY_SM_STATUS) >> 5;
    DEBUG("treufunk_get_state(): STATE = 0x%03x\n", state);
    return (state);
}


/**
 * Everytime the chip is put into RX or TX state, some manual resets have to be done.
 */
static void _rx_resets(treufunk_t *dev)
{
    DEBUG("Doing rx_resets...\n");
    /* Reset FIFO and SM */
    treufunk_reg_write(dev, RG_SM_MAIN, 0x05);
    treufunk_reg_write(dev, RG_SM_MAIN, 0x0F);

    /* Reset demodulator */
    treufunk_sub_reg_write(dev, SR_DEM_RESETB, 0);
    treufunk_sub_reg_write(dev, SR_DEM_RESETB, 1);


    /* Do we need to wait some time before setting the reset bit back to 1 ?
    because during init this isn't done neither.

    Answer: In theory, yes. But the SPI takes quite some time (relative to the settling time of the demod),
    and therefore the solution above should be sufficient.
    */
}

/**
 * Converts a state (phy_status) to the corresponding STATE_CMD (for SM_MAIN)
 */
static uint8_t state_to_statecmd(uint8_t state)
{
    switch(state)
    {
        case SLEEP:
            return STATE_CMD_SLEEP;
        case DEEP_SLEEP:
            return STATE_CMD_DEEPSLEEP;
        case SENDING:
            return STATE_CMD_TX;
        case TX_RDY:
            return STATE_CMD_TXIDLE;
        case RECEIVING:
            return STATE_CMD_RX;
        case RX_RDY:
            return STATE_CMD_RXHOLD;

        default:
            return -1;
    }
}

/**
 * Sets the state of the state machine.
 * Note: Before changing to TX the SM needs to be put into SLEEP first. Then the frame is written
 * into the FIFO. Finally transition to TX is initiated. This all happens in treufunk.c (prepare, load, exec).
 *
 * @param state: state as in phy_status
 */
void treufunk_set_state(treufunk_t *dev, uint8_t state)
{
    DEBUG("Setting state %d...\n", state);
    uint8_t state_cmd = state_to_statecmd(state);
    if(dev->state == state) return;
    else if(state_cmd == -1) return;

    /* Before transitioning into RX/TX, some manual resets have to be done */
    if(state_cmd == STATE_CMD_RX || state_cmd == STATE_CMD_TX) {
        _rx_resets(dev);
        DEBUG("rx_resets done.\n");
    }

    /* write state_cmd to SM_COMMAND sub_reg of SM_MAIN */
    DEBUG("Writing state_cmd (%d) into SM_MAIN reg...\n", state_cmd);
    treufunk_sub_reg_write(dev, SR_SM_COMMAND, state_cmd);
    /* Wait until state transition is complete */
    DEBUG("Waiting until state transition is finished...\n");
    while(treufunk_get_state(dev) == BUSY);
    /* set SM_COMMAND back to CMD_NONE */
    DEBUG("Resetting state_cmd sub-reg (SM_MAIN)\n");
    treufunk_sub_reg_write(dev, SR_SM_COMMAND, STATE_CMD_NONE);

    /* set state attribute of the Treufunk device descriptor */
    dev->state = treufunk_get_state(dev);
}


/**
 * set sm_resetb in SM_MAIN to 0 in order to reset the SM
 */
/* TODO (reset_state_machine): This function still needed? */
void treufunk_reset_state_machine(treufunk_t *dev)
{
    treufunk_sub_reg_write(dev, SR_SM_RESETB, 0);
    treufunk_sub_reg_write(dev, SR_SM_RESETB, 1);

    /* TODO (reset_state_machine): Wait for SM to settle? */
    dev->state = treufunk_get_state(dev);
}

/* TODO (reset_fifo): Do we need this extra function, or would the two lines
be enough when called inside reset() ?
*/
void treufunk_reset_fifo(treufunk_t *dev)
{
    treufunk_sub_reg_write(dev, SR_FIFO_RESETB, 0);
    treufunk_sub_reg_write(dev, SR_FIFO_RESETB, 1);
}

uint8_t treufunk_get_chan(treufunk_t *dev)
{
    return dev->netdev.chan;
}

/* TODO (set_chan) */
void treufunk_set_chan(treufunk_t *dev, uint8_t chan)
{
    if((chan < IEEE802154_CHANNEL_MIN) ||
        (chan > IEEE802154_CHANNEL_MAX) ||
        (dev->netdev.chan == chan)) {
            return;
    }

    dev->netdev.chan = chan;

    /* Calculate channel center frequency in MHz according to the ieee802154 standard (channel page 0) */
    int center_freq = 2405 + 5 * (chan - 11);

    /* TODO (set_chan): */

    /* Calculate PLL values for RX and TX */

    /* Calculate VCO tune */
}

void treufunk_set_option(treufunk_t *dev, uint16_t option, bool state)
{
    if(state)
    {
        dev->netdev.flags |= option;
    }
    else
    {
        dev->netdev.flags &= ~(option);
    }
}
