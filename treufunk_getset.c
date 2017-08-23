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
 * Get current TX power
 */
uint8_t treufunk_get_txpower(treufunk_t *dev)
{
    return treufunk_sub_reg_read(dev, SR_TX_PWR_CTRL);
}

/**
 * Sets the desired TX Power by writing into the SM_TX_POWER_CTRL (0x8B) register.
 * @param txpower The desired TX power. Valid values: 0 .. 15 (0dBm .. 15dBm?)
 */
void treufunk_set_txpower(treufunk_t *dev, uint8_t txpower)
{
    DEBUG("(set_txpower(): Setting tx_power...\n");
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
    DEBUG("get_state(): Getting current state...\n");
    uint8_t state = PHY_SM_STATUS(treufunk_get_phy_status(dev));
    DEBUG("get_statE(): treufunk_get_state(): STATE = 0x%03x\n", state);
    return (state);
}

/**
 * Everytime the chip is put into RX or TX state, some manual resets have to be done.
 */
static void _rx_resets(treufunk_t *dev)
{
    DEBUG("_rx_resets(): Doing rx_resets...\n");
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
    DEBUG("set_state(): Setting state %d...\n", state);
    uint8_t state_cmd = state_to_statecmd(state);
    //if(dev->state == state) return; /* TODO (set_state): Problem: Automatic transition TX-> RX. This variable does not get changed */
    if(state_cmd == -1) return;

    /* Before transitioning into RX/TX, some manual resets have to be done */
    if(state_cmd == STATE_CMD_RX || state_cmd == STATE_CMD_TX) {
        _rx_resets(dev);
        DEBUG("set_state(): rx_resets done.\n");
    }

    /* write state_cmd to SM_COMMAND sub_reg of SM_MAIN */
    DEBUG("set_state(): Writing state_cmd (%d) into SM_MAIN reg...\n", state_cmd);
    treufunk_sub_reg_write(dev, SR_SM_COMMAND, state_cmd);

    /* Wait until state transition is complete */
    DEBUG("set_state(): Waiting until state transition is finished...\n");
    while(treufunk_get_state(dev) == BUSY);

    /* set SM_COMMAND back to CMD_NONE */
    DEBUG("set_state(): Resetting state_cmd sub-reg (SM_MAIN)\n");
    treufunk_sub_reg_write(dev, SR_SM_COMMAND, STATE_CMD_NONE);

    /* set state attribute of the Treufunk device descriptor */
    dev->state = treufunk_get_state(dev);

    DEBUG("set_state(): New state = %d\n", dev->state);

    /* Start polling timer for RX and TX */
    if(state == RECEIVING || state == SENDING)
    {
        DEBUG("set_state(): Setting timer...\n");
        xtimer_set(&(dev->poll_timer), RX_POLLING_INTERVAL); /* TODO (set_state): Discuss polling interval */
    }
    /* Remove polling timer when SLEEP */
    else if(state == SLEEP)
    {
        DEBUG("set_state(): Disabling timer...\n");
        xtimer_remove(&(dev->poll_timer));
    }
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

static uint32_t _calc_vco_tune(uint8_t channel)
{
    switch(channel)
    {
        case 11: return 237;
        case 12: return 235;
        case 13: return 234;
        case 14: return 232;
        case 15: return 231;
        case 16: return 223;
        case 17: return 222;
        case 18: return 220;
        case 19: return 213;
        case 20: return 212;
        case 21: return 210;
        case 22: return 209;
        case 23: return 207;
        case 24: return 206;
        case 25: return 206; /* TODO (_calc_vco): same value?? */
        case 26: return 204;
        default: return 0;
    }
}

static int _calculate_pll_values(uint32_t rf_freq,
                                uint32_t if_freq,
                                uint32_t *int_val,
                                uint32_t *frac_val)
{
    uint32_t f_lo = 0;
    //int frac_correction = 0;

    /* 2.4 GHz */
    if(rf_freq > 2000000000)
    {
        f_lo = (rf_freq - if_freq) / 3 * 2;
        *int_val = f_lo / 16000000;

        /* TODO (_calc_pll) */
        *frac_val = (f_lo % 16000000) * 228/3479; //+ frac_correction;
        return 0;
    }

    return -EINVAL;
}

/* TODO (set_chan) */
void treufunk_set_chan(treufunk_t *dev, uint8_t chan)
{
    if((chan < IEEE802154_CHANNEL_MIN) ||
        (chan > IEEE802154_CHANNEL_MAX) ||
        (dev->netdev.chan == chan)) {
            DEBUG("set_chan(): Invalid channel or channel already set: %d\n", chan);
            return;
    }

    dev->netdev.chan = chan;

    uint32_t pll_int = 0;
    uint32_t pll_frac = 0;
    uint32_t vco_tune = 0;

    /* Calculate channel center frequency in MHz according to the ieee802154 standard (channel page 0) */
    uint32_t center_freq = (2405 + 5 * (chan - 11)) * 1000000U;
    DEBUG("set_chan(): Setting to center freq = %lu Hz\n", center_freq);

    /* TODO (set_chan): */

    /* Calculate PLL values for RX ... */
    _calculate_pll_values(center_freq, 1000000, &pll_int, &pll_frac);
    treufunk_sub_reg_write(dev, SR_RX_CHAN_INT, pll_int);
    treufunk_sub_reg_write(dev, SR_RX_CHAN_FRAC_H, BIT24_H_BYTE(pll_frac));
    treufunk_sub_reg_write(dev, SR_RX_CHAN_FRAC_M, BIT24_M_BYTE(pll_frac));
    treufunk_sub_reg_write(dev, SR_RX_CHAN_FRAC_L, BIT24_L_BYTE(pll_frac));
    DEBUG("set_chan(): Set RX PLL values to int=%ld and frac=0x%lu\n", pll_int, pll_frac);
    DEBUG("set_chan(): RX HByte = 0x%02x, MByte = 0x%02x, LByte = 0x%02x\n", BIT24_H_BYTE(pll_frac), BIT24_M_BYTE(pll_frac), BIT24_L_BYTE(pll_frac));

    /* ... and TX */
    _calculate_pll_values(center_freq, 0, &pll_int, &pll_frac);
    treufunk_sub_reg_write(dev, SR_TX_CHAN_INT, pll_int);
    treufunk_sub_reg_write(dev, SR_TX_CHAN_FRAC_H, BIT24_H_BYTE(pll_frac));
    treufunk_sub_reg_write(dev, SR_TX_CHAN_FRAC_M, BIT24_M_BYTE(pll_frac));
    treufunk_sub_reg_write(dev, SR_TX_CHAN_FRAC_L, BIT24_L_BYTE(pll_frac));
    DEBUG("set_chan(): Set TX PLL values to int=%lu and frac=0x%lu\n", pll_int, pll_frac);
    DEBUG("set_chan(): TX HByte = 0x%02x, MByte = 0x%02x, LByte = 0x%02x\n", BIT24_H_BYTE(pll_frac), BIT24_M_BYTE(pll_frac), BIT24_L_BYTE(pll_frac));

    /* Calculate VCO tune */
    vco_tune = _calc_vco_tune(chan);
    treufunk_sub_reg_write(dev, SR_PLL_VCO_TUNE, vco_tune);
    DEBUG("set_chan(): Set VCO TUNE to %lu\n", vco_tune);
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
