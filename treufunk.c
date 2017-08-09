/*
* treufunk.c
*
* Kirthihan Yasotharan
*
* Implementation of public functions for the Treufunk driver
*
*/

#include "luid.h"
#include "byteorder.h"
#include "net/ieee802154.h"
#include "net/gnrc.h"
#include "treufunk_registers.h"
#include "treufunk_internal.h"
#include "treufunk_netdev.h"

// #define ENABLE_DEBUG (0)
// #include "debug.h"

/**
 * First function to be called during the initialization of the transceiver.
 *
 * Just sets up the driver struct and copies the passed spi parameters to the device struct.
 * Actual init of spi pins happens in netdev.c/_init()
 */
void treufunk_setup(treufunk_t *dev, const treufunk_params_t *params)
{
    DEBUG("treufunk_setup()...\n");
    dev->netdev.netdev.driver = &treufunk_driver;

    memcpy(&dev->params, params, sizeof(treufunk_params_t));
    dev->state = SLEEP;
}

/**
 * Resets Treufunk to initial values
 *
 * Adapted after lprf.c/init_lprf_hardware()
 * https://github.com/ias-aachen/lprf_linux_driver/blob/master/lprf.c#L1647
 */
int treufunk_reset(treufunk_t *dev)
{
    DEBUG("treufunk_reset()...\n");
    // treufunk_hardware_reset(dev);    /* TODO (treufunk_reset): Hardware reset neccessary? */

    /* TEMP_BEGIN (treufunk_reset), reg_check (treufunk_reset) */
    int ret = 0;
    /* TEMP_END */

    /* Reset sequence number and options */
    dev->netdev.seq = 0;
    dev->netdev.flags = 0;

    /* set default options
    TODO: Which of them true/false?
    */
    treufunk_set_option(dev, TREUFUNK_OPT_TELL_RX_START, false);
    treufunk_set_option(dev, TREUFUNK_OPT_TELL_RX_END, true);
    treufunk_set_option(dev, TREUFUNK_OPT_TELL_TX_START, false);
    treufunk_set_option(dev, TREUFUNK_OPT_TELL_TX_END, false);

    /* set default protocol */
    #ifdef MODULE_GNRC_SIXLOWPAN
        dev->netdev.proto = GNRC_NETTYPE_SIXLOWPAN;
    #elif MODULE_GNRC
        dev->netdev.proto = GNRC_NETTYPE_UNDEF;
    #endif

    /* TODO (treufunk_reset): Driver side addresses ? */

    /* Reset all and load initial values */
    DEBUG("Doing global resets...\n");
    RETURN_ON_ERROR(treufunk_reg_write(dev, RG_GLOBAL_RESETB,   0x00));
    RETURN_ON_ERROR(treufunk_reg_write(dev, RG_GLOBAL_RESETB,   0xFF));
    RETURN_ON_ERROR(treufunk_reg_write(dev, RG_GLOBAL_initALL,  0xFF));

    /* Clock reference */
    DEBUG("Setting CLK references...\n");
    RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_CTRL_CLK_CDE_OSC,    0));
    RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_CTRL_CLK_CDE_PAD,    1));
    RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_CTRL_CLK_DIG_OSC,    0));
    RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_CTRL_CLK_DIG_PAD,    1));
    RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_CTRL_CLK_PLL_OSC,    0));
    RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_CTRL_CLK_PLL_PAD,    1));
    RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_CTRL_CLK_C3X_OSC,    0));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_CTRL_CLK_C3X_PAD,    1));
    RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_CTRL_CLK_FALLB,      0));

    /* ADC_CLK */
    DEBUG("Setting ADC_CLK...\n");
    RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_CTRL_CDE_ENABLE,    0));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_CTRL_C3X_ENABLE,    1));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_CTRL_CLK_ADC,       1));
    RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_CTRL_C3X_LTUNE,     1));

    /* LDOs */
    DEBUG("Configuring LDOs...\n");
    RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_LDO_A_VOUT,      21));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_LDO_D_VOUT,      24));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_LDO_PLL_VOUT,    24));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_LDO_VCO_VOUT,    24));
    RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_LDO_TX24_VOUT,   23));

    /* PLL config */
    DEBUG("Configuring PLL...\n");
    RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_IREF_PLL_CTRLB,   0));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_PLL_VCO_TUNE,   235));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_PLL_LPF_C,        0));
    RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_PLL_LPF_R,        9));

    /* Enable 2.4GHz band */
    DEBUG("Enabling 2.4 GHz band...\n");
    RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_RX_RF_MODE,     0));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_RX_LO_EXT,      0));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_LNA24_ISETT,    7));
    RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_LNA24_SPCTRIM, 15));

    /* ADC Settings */
    DEBUG("Configuring ADC...\n");
    RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_CTRL_ADC_MULTIBIT, 0));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_CTRL_ADC_ENABLE,   1));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_CTRL_ADC_BW_SEL,   1));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_CTRL_ADC_BW_TUNE,  5));
    RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_CTRL_ADC_DR_SEL,   2));

    /* Polyphase filter settings */
    DEBUG("Configuring PPF...\n");
    RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_PPF_M0,    0));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_PPF_M1,    0));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_PPF_TRIM,  0));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_PPF_HGAIN, 1));
    RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_PPF_LLIF,  0));

    /* Demodulator settings */
    DEBUG("Configuring demodulator...\n");
    RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_DEM_CLK96_SEL,          1));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_DEM_AGC_EN,             1));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_DEM_FREQ_OFFSET_CAL_EN, 0));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_DEM_OSR_SEL,            0));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_DEM_BTLE_MODE,          1));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_DEM_IF_SEL,             2));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_DEM_DATA_RATE_SEL,      3));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_DEM_IQ_CROSS,           1));
    RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_DEM_IQ_INV,             0));

    /* initial CIC filter gain settings */
    DEBUG("Configuring CIC filter...\n");
    RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_DEM_GC1, 0));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_DEM_GC2, 0));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_DEM_GC3, 1));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_DEM_GC4, 0));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_DEM_GC5, 0));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_DEM_GC6, 1));
    RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_DEM_GC7, 4));

    /* General TX settings */
    DEBUG("Configuring TX path...\n");
    RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_PLL_MOD_DATA_RATE,    3));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_PLL_MOD_FREQ_DEV,    21));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_TX_EN,                1));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_TX_ON_CHIP_MOD,       1));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_TX_UPS,               0));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_TX_ON_CHIP_MOD_SP,    0));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_TX_AMPLI_OUT_MAN_H,   1));
    RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_TX_AMPLI_OUT_MAN_L, 255));

    /*

        STATE MACHINE CONFIG

     */

    /* General state machine settings */
    DEBUG("Setting general SM settings...\n");
    RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_FIFO_MODE_EN,    1));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_WAKEUPONSPI,     1));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_WAKEUPONRX,      0));
    RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_WAKEUP_MODES_EN, 0));

    /* Startup counter Settings */
    DEBUG("Setting up startup counter...\n");
	RETURN_ON_ERROR(treufunk_reg_write(dev, RG_SM_TIME_POWER_TX, 0xff));
	RETURN_ON_ERROR(treufunk_reg_write(dev, RG_SM_TIME_POWER_RX, 0xff));
	RETURN_ON_ERROR(treufunk_reg_write(dev, RG_SM_TIME_PLL_PON,  0xff));
	RETURN_ON_ERROR(treufunk_reg_write(dev, RG_SM_TIME_PLL_SET,  0xff));
	RETURN_ON_ERROR(treufunk_reg_write(dev, RG_SM_TIME_TX,       0xff));
    RETURN_ON_ERROR(treufunk_reg_write(dev, RG_SM_TIME_PD_EN,    0xff));

    /* SM TX */
    DEBUG("configuring SM TX...\n");
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_TX_MODE,          0));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_INVERT_FIFO_CLK,  0));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_DIRECT_RX,        1)); /* Change directly to RX after completed transmission (TX) */
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_TX_ON_FIFO_IDLE,  0));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_TX_ON_FIFO_SLEEP, 0));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_TX_IDLE_MODE_EN,  0));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_TX_PWR_CTRL,     15)); /* Maximum TX output power (15dBm) */
    /* or this? treufunk_set_txpower(dev, TREUFUNK_DEFAULT_TXPOWER); */
    RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_TX_MAXAMP,        0));

    /* SM RX */
    DEBUG("Configuring SM RX...\n");
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_DIRECT_TX,          0));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_DIRECT_TX_IDLE,     0));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_RX_HOLD_MODE_EN,    0));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_RX_TIMEOUT_EN,      0));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_RX_HOLD_ON_TIMEOUT, 0));
    RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_AGC_AUTO_GAIN,      0));

    /* Package counter */
    /**
     * IEEE802.15.4 frame size = 135 bytes. Datarate = 2 Mbit/s
     * => 135bytes / 2Mbit/s = 540us transmit time
     * Maximum RX Counter value = 0x0043C0 = 17344
     * Counter 32 MHz
     * => 17344 / 32MHz ~= 544us
     */
    DEBUG("Setting up RX length counter...\n");
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_RX_LENGTH_H, 0x00));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_RX_LENGTH_M, 0x43));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_RX_LENGTH_L, 0xC0));

    /* Timeout counter */
    /* We don't use the timeout counter, so set all bits to 1 */
    DEBUG("Setting up timeout counter...\n");
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_RX_TIMEOUT_H, 0xFF));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_RX_TIMEOUT_M, 0xFF));
    RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_RX_TIMEOUT_L, 0xFF));

    /* Resets */
    DEBUG("Doing resets...\n");
	 /* TODO (treufunk_reset): Call function reset_fifo() or leave this ? */
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_SM_EN,       1));
    RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_FIFO_RESETB, 0));
    RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_FIFO_RESETB, 1));
	RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_SM_RESETB,   0));
    RETURN_ON_ERROR(treufunk_sub_reg_write(dev, SR_SM_RESETB,   1)); /* TODO (treufunk_reset): Call function reset_state_machine() or leave this ? */



    //go into RX state
    treufunk_set_state(dev, STATE_CMD_RX);

    DEBUG("trefunk_reset(): reset complete.\n");

    return 0;
}


/**
 * Convenience function for simply sending data
 */
size_t treufunk_send(treufunk_t *dev, uint8_t *data, size_t len)
{
    DEBUG("treufunk_send()...\n");
    /* TODO (treufunk_send): Check for max len */
    treufunk_tx_prepare(dev);
    treufunk_tx_load(dev, data, len);
    treufunk_tx_exec(dev);
    return len;
}

/**
 * Prepare for TX.
 * Before put into TX mode, the SM needs to be put into SLEEP first.
 */
void treufunk_tx_prepare(treufunk_t *dev)
{
    DEBUG("prepare...\n");

    uint8_t state;

    /* wait until ongoing transmissions are finished */
    DEBUG("treufunk_tx_prepare(): waiting for Treufunk to become ready...\n");
    do
    {
        state = treufunk_get_state(dev);
    } while(state == SENDING || state == RECEIVING || state == BUSY);


    /* Put SM into SLEEP */
    DEBUG("treufunk_tx_prepare(): putting into SLEEP...\n");
    treufunk_set_state(dev, STATE_CMD_SLEEP);

}

/**
 * Load data to be transmitted over the air into the FIFO.
 *
 * @param  data Pointer to the data
 * @param  len  Length of data
 * @return      Returns the number of bytes written into the FIFO
 */
size_t treufunk_tx_load(treufunk_t *dev, uint8_t *data, size_t len)
{
    DEBUG("load...\n");

    DEBUG("treufunk_tx_load(): writing data into FIFO...\n");
    treufunk_fifo_write(dev, data, len);
    return len;
}

/**
 * Initiate the transmission by putting the SM into TX mode/state.
 */
void treufunk_tx_exec(treufunk_t *dev)
{
    DEBUG("exec...\n");

    DEBUG("treufunk_tx_exec(): putting into TX...\n");
    treufunk_set_state(dev, STATE_CMD_TX);

    if(dev->netdev.netdev.event_callback && (dev->netdev.flags & TREUFUNK_OPT_TELL_TX_START))
    {
        dev->netdev.netdev.event_callback(&(dev->netdev.netdev), NETDEV_EVENT_TX_STARTED);
    }
}
