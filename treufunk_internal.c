/*
* treufunk_internal.h
*
* Kirthihan Yasotharan
*
* Implementation of driver internal functions
*
*/

// #include "periph/spi.h"
// #include "periph/gpio.h"
/* TEMP_BEGIN (xtimer) */
#include "xtimer.h" // Auto Init must be called once at system boot in order to use xtimer. So don't add "DISABLE_MODULE += auto_init" inside the Makefile
/* TEMP_END */
#include "treufunk_internal.h"
#include "treufunk_registers.h"

// #define SPIDEV  (dev->params.spi)
// #define CSPIN   (dev->params.cs_pin)

// #define ENABLE_DEBUG (0)
// #include "debug.h"

// #if DUE_SR_MODE
//
//     /**
//      * This function reads out the shift register serially.
//      *
//      * Note that we have to read the MSB first manually,
//      * since otherwise it would be kicked out of the SR with the
//      * first rising SPI_CLK edge.
//      */
//     uint8_t due_shift_read(const treufunk_t *dev)
//     {
//         uint8_t recv = 0;
//         uint8_t msb = gpio_read(GPIO_PIN(0,25)); /* Read MSB from MISO (PA25) */
//         spi_transfer_bytes(SPIDEV, SPI_CS_UNDEF, false, NULL, &recv, 1); /* We use SPI_CS_UNDEF because the SR has no latch enable feature; we just need clock and MOSI/SER */
//         recv = (msb << 7) | (recv >> 1); /* correction */
//
//         return recv;
//     }
//
// #endif /* DUE_SR_MODE */


/**
 * Checks if the Register at address "addr" is RO. If so it returns FALSE.
 */
static bool reg_writable(unsigned int addr)
{
  switch (addr) {
    case RG_DEM_PD_OUT:
    case RG_DEM_GC_AOUT:
    case RG_DEM_GC_BOUT:
    case RG_DEM_GC_COUT:
    case RG_DEM_GC_DOUT:
    case RG_DEM_FREQ_OFFSET_OUT:
    case RG_SM_STATE:
    case RG_SM_FIFO:
    case RG_SM_GLOBAL:
    case RG_SM_POWER:
    case RG_SM_RX:
    case RG_SM_WAKEUP_EN:
    case RG_SM_DEM_ADC:
    case RG_SM_PLL_TX:
    case RG_SM_PLL_CHAN_INT:
    case RG_SM_PLL_CHAN_FRAC_H:
    case RG_SM_PLL_CHAN_FRAC_M:
    case RG_SM_PLL_CHAN_FRAC_L:
    case RG_SM_TX433:
    case RG_SM_TX800:
    case RG_SM_TX24:
    case RG_PLL_TPM_GAIN_OUT_L:
    case RG_PLL_TPM_GAIN_OUT_M:
    case RG_PLL_TPM_GAIN_OUT_H:
    return false;
    default:
    return true;
  }
}

// static inline void getbus(const treufunk_t *dev)
// {
//     spi_acquire(SPIDEV, CSPIN, SPI_MODE_0, dev->params.spi_clk);
// }


uint8_t treufunk_reg_read(const treufunk_t *dev,
                          const uint8_t addr)
{
    //uint8_t value;

    // getbus(dev);
    // spi_transfer_byte(SPIDEV, CSPIN, true, TREUFUNK_ACCSESS_REG_READ);
    // spi_transfer_byte(SPIDEV, CSPIN, true, addr);
    //
    // #if DUE_SR_MODE
    //     spi_transfer_byte(SPIDEV, CSPIN, false, 0);
    //     value = due_shift_read(dev);
    // #else
    //     value = spi_transfer_byte(SPIDEV, CSPIN, false, NULL);
    // #endif /* DUE_SR_MODE */
    //
    // spi_release(SPIDEV);

    return 0xAB;
}

void treufunk_reg_write(const treufunk_t *dev,
                         const uint8_t addr,
                         const uint8_t value)
{
    if(!reg_writable(addr))
    {
        DEBUG("ERROR (treufunk_reg_write):\tRegister 0x%08x is READ-ONLY! Aborting...\n", addr);
        return -1;
    }
    // getbus(dev);
    // spi_transfer_byte(SPIDEV, CSPIN, true, TREUFUNK_ACCSESS_REG_WRITE);
    // spi_transfer_byte(SPIDEV, CSPIN, true, addr);
    // spi_transfer_byte(SPIDEV, CSPIN, false, value);
    // spi_release(SPIDEV);

    /**
     * TEMP_BEGIN (reg_write)
     *
     * reg_check (reg_write): Check if values written are correct
     */
    // uint8_t reg_value = treufunk_reg_read(dev, addr);
    // if(reg_value != value)
    // {
    //     DEBUG("ERROR (treufunk_reg_write): Wrong value in register 0x%02x. Value that should have been written: %d (0x%02x). Actual value: %d (0x%02x). Aborting...\n", addr, value, value, reg_value, reg_value);
    //     return -1;
    // }
    //
    // DEBUG("Value %d (0x%02x) written into register 0x%02x (%d).\n", value, value, addr, addr);

    /* TEMP_END */
    DEBUG("reg_write:\tWriting Value 0x%02x into register 0x%02x\n", value, addr);

    return 0;
}



uint8_t treufunk_sub_reg_read(const treufunk_t *dev,
                            const uint8_t reg_addr,
                            const uint8_t sub_reg_mask,
                            const uint8_t offset)
{
    //  uint8_t reg_value;
    //  reg_value = treufunk_reg_read(dev, reg_addr);
    //  return ((reg_value & sub_reg_mask) >> offset);
}

 /**
  * Writes into a sub-register.
  *
  * @param reg_addr     Base address of the main register
  * @param sub_reg_mask Bit-mask for the sub-register
  * @param offset       position of the sub-register relative to LSB of the register
  * @param value        Value to be written in the sub-register. Will be left-shifted by offset, before
  *                     written into the sub-register
  *
  * NOTE: Usually we don't have to worry about the correct values for sub_reg_mask and offset, since
  * they are defined already for all existing sub-registers in "lprf_registers.h".
  */
void treufunk_sub_reg_write(const treufunk_t *dev,
                            const uint8_t reg_addr,
                            const uint8_t sub_reg_mask,
                            const uint8_t offset,
                            const uint8_t value)
{
    // uint8_t reg_value;
    // /* Save current reg content */
    // reg_value = treufunk_reg_read(dev, reg_addr);
    // /* Set current bits of sub_reg to 0 */
    // reg_value &= (~sub_reg_mask);
    // /* Set new bits */
    // reg_value |= (value << offset);
    // /* Write back to reg */
    // treufunk_reg_write(dev, reg_addr, reg_value);

}



/* TODO (fifo_read): Due to the one byte length field only 255 bytes can [...]
be read out the FIFO during one fifo_read access.
If more bytes need to be read out, one or more fifo_read accesses are necessary.
*/
void treufunk_fifo_read(/*const*/ treufunk_t *dev,
                        uint8_t *buf,
                        const size_t buf_len)
{
    DEBUG("fifo_read...\n");
    // size_t len;
    //
    /* Pseusoframe without first byte of preamble, because Treufunk removes it */
    uint8_t pframe[9] = {0x00, 0x00, 0x00, 0xA7, 0x04, 0x01, 0x01, 0x01, 0x01};
    memcpy(buf, pframe, 9);
    dev->fifo_empty = 1;
    // getbus(dev);
    // /* Write FRAME_READ access command */
    // spi_transfer_byte(SPIDEV, CSPIN, true, TREUFUNK_ACCSESS_FRAME_READ);
    // #if DUE_SR_MODE
    //     /* Get number of bytes in FIFO */
    //     spi_transfer_byte(SPIDEV, CSPIN, true, 0);
    //     len = due_shift_read(dev); /* Note: While we shift out the length byte out of the SR, the Treufunk already shifts in the first FIFO byte! */
    //     DEBUG("fifo_read:\t%d bytes in FIFO.\n", len);
    //     if(len > buf_len)
    //     {
    //         DEBUG("ERROR (fifo_read):\tData in FIFO is %d bytes but buffer is only of size %d. Aborting!\n", len, buf_len);
    //         /* Disable CS */
    //         gpio_set((gpio_t)CSPIN);
    //         spi_release(SPIDEV);
    //         return;
    //     }
    //
    //     /* Shift out all bytes in FIFO */
    //     for(int i = 0; i < len; i++)
    //     {
    //         buf[i] = due_shift_read(dev);
    //     }
    //     /* Disable CS */
    //     gpio_set((gpio_t)CSPIN);
    //
    // #else
    //     len = spi_transfer_byte(SPIDEV, CSPIN, true, 0);
    //     if(len > buf_len)
    //     {
    //         DEBUG("ERROR (fifo_read):\tData in FIFO is %d bytes but buffer is only of size %d. Aborting!\n", len, buf_len);
    //         /* Disable CS */
    //         gpio_set((gpio_t)CSPIN);
    //     }
    //     else
    //     {
    //         spi_transfer_bytes(SPIDEV, CSPIN, false, NULL, buf, len);
    //     }
    // #endif /* DUE_SR_MODE */
    //
    // spi_release(SPIDEV);


}


void treufunk_fifo_write(/*const*/ treufunk_t *dev,
                            const uint8_t *data,
                            const size_t len)
{
    DEBUG("fifo_write():\tWriting %d bytes into FIFO\n", len);
    // getbus(dev);
    // spi_transfer_byte(SPIDEV, CSPIN, true, TREUFUNK_ACCSESS_FRAME_WRITE);
    // spi_transfer_byte(SPIDEV, CSPIN, true, len);
    // //spi_transfer_bytes(SPIDEV, CSPIN, false, data, NULL, len);
    // for(int i = 0; i < len; i++)
    // {
    //     spi_transfer_bytes(SPIDEV, CSPIN, true, data+i, NULL, 1);
    // }
    // gpio_set((gpio_t)CSPIN);
    // spi_release(SPIDEV);
    dev->fifo_empty = 0;

}

/**
 * Sends one abitrary byte to the Treufunk, and receives back the phy_status byte.
 */
uint8_t treufunk_get_phy_status(const treufunk_t *dev)
{
    //DEBUG("get_phy_status():\tReading phy_status byte...\n");
    uint8_t phy_status;
    // getbus(dev);
    // /* Just transfer any byte, e.g. 8 ones or all zeros, on MOSI. Treufunk will send back phy_status on MISO */
    // #if DUE_SR_MODE
    //     spi_transfer_byte(SPIDEV, CSPIN, false, 0);
    //     phy_status = due_shift_read(dev);
    // #else
    //     phy_status = spi_transfer_byte(SPIDEV, CSPIN, false, 0);
    // #endif /* DUE_SR_MODE */
    // spi_release(SPIDEV);

    phy_status = (dev->state)<<5 | (dev->fifo_empty)<<3;

    return phy_status;
}
