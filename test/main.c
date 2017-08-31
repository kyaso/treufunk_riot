#include <stdio.h>
#include <stdlib.h> /* strtol(), atoi() */
#include <stdint.h>
#include <string.h> /* strlen() */

#include "shell.h"
#include "shell_commands.h"
#include "od.h"

#include "treufunk.h"
#include "treufunk_params.h"
#include "treufunk_internal.h"
#include "treufunk_registers.h"

#include "net/netdev.h"

treufunk_t myTreufunk;

static int reg_read(int argc, char **argv)
{
    if(argc < 2) return -1;
    for(int i = 1; i < argc; i++)
    {
        uint8_t addr = strtol(argv[i], NULL, 16);
        uint8_t val;
        val = treufunk_reg_read(&myTreufunk, addr);
        printf("Register 0x%02x: 0x%02x\n", addr, val);
    }
    return 0;
}

static int reg_write(int argc, char **argv)
{
    if(argc != 3) return -1;
    uint8_t addr = strtol(argv[1], NULL, 16);
    uint8_t val = strtol(argv[2], NULL, 16);
    treufunk_reg_write(&myTreufunk, addr, val);
    printf("Write 0x%02x into reg 0x%02x\n", val, addr);
    return 0;
}

static int send(int argc, char **argv)
{
    if(argc != 2 && argc != 4) return -1;
    /* Send custom pattern */
    if(argc == 4)
    {
        if(strcmp(argv[1], "-p") == 0)
        {
            uint8_t pattern = strtol(argv[2], NULL ,16);
            int num = atoi(argv[3]);
            uint8_t tx_buf[num];
            memset(tx_buf, pattern, num);
            printf("Sending %d bytes of pattern \"0x%02x\".\n", num, pattern);
            treufunk_send(&myTreufunk, tx_buf, num);
            return 0;
        }
        else puts("usage: tx -p <pattern (hex)> <num of bytes>");
        return -1;
    }
    /* Send ascii string */
    else
    {
        uint8_t len = strlen(argv[1]);
        printf("Data:\n");
        // for(int i = 0; i < len; i++)
        // {
        //     printf("%02x ", (uint8_t)(argv[1][i]));
        // }
        od_hex_dump(argv[1], len, 16);
        puts(" ");
        for(int i = 0; i < len; i++)
        {
            for(int j = 0; j < 8; j++)
            {
                printf("%d", ((uint8_t)argv[1][i] << j)&0x80 ? 1 : 0);
            }

            if((i > 0) && (((i+1) % 8) == 0)) printf("\n");
            else printf(" ");
        }
        puts(" ");
        printf("Sending %d bytes\n", len);
        treufunk_send(&myTreufunk, (uint8_t *)argv[1], len);
        return 0;
    }

}

// static int send_all_ones(int argc, char **argv)
// {
//     if(argc != 2)
//     {
//         puts("usage: txones <number of FF bytes (max 255)>");
//         return -1;
//     }
//     uint8_t num_of_ones = atoi(argv[1]);
//     if(num_of_ones <= 0 || num_of_ones > 255)
//     {
//         puts("invalid num");
//         return -1;
//     }
//     uint8_t ones[num_of_ones];
//     memset(ones, 0xFF, num_of_ones);
//     treufunk_send(&myTreufunk, &ones, num_of_ones);
//     return 0;
// }

static int send_all_ones(int argc, char **argv)
{
    uint8_t ones[255] = {0xAA};
    treufunk_tx_prepare(&myTreufunk);
    for(int i = 0; i < 3; i++)
    {
        treufunk_fifo_write(&myTreufunk, ones, 255);
    }
    treufunk_fifo_write(&myTreufunk, ones, 200);
    treufunk_tx_exec(&myTreufunk);

    return 0;

}

static int receive(int argc, char **argv)
{
    uint8_t buf_len = 140;
    uint8_t buf[buf_len];
    memset(buf, 0, buf_len);
    treufunk_fifo_read(&myTreufunk, buf, buf_len);

    /* Reverse bit order and invert the bits */
    for(int i = 0; i < buf_len; i++)
    {
        buf[i] = ((buf[i] & 0xaa) >> 1) | ((buf[i] & 0x55) << 1);
    	buf[i] = ((buf[i] & 0xcc) >> 2) | ((buf[i] & 0x33) << 2);
    	buf[i] = (buf[i] >> 4) | (buf[i] << 4);

        buf[i] = ~buf[i];
    }

    puts("Received Data:");
    // for(int i = 0; i < buf_len; i++)
    // {
    //     printf("0x%02x ", buf[i]);
    // }

    od_hex_dump(&buf, buf_len, 8);
    puts("");
    printf("0x0000: ");
    for(int i = 0; i < buf_len; i++)
    {
        for(int j = 0; j < 8; j++)
        {
            printf("%d", (buf[i] << j)&0x80 ? 1 : 0);
        }

        if((i > 0) && (((i+1) % 8) == 0))
        {
            printf("\n");
            printf("0x%04x: ", i+1);
        }
        else printf(" ");
    }
    puts("");
    return 0;
}

static int get_phy(int argc, char **argv)
{
    uint8_t phy_status = treufunk_get_phy_status(&myTreufunk);
    printf("phy_status (0x%02x):\n", phy_status);
    printf("SM status   = %d\n", PHY_SM_STATUS(phy_status));
    printf("SM enable   = %d\n", PHY_SM_ENABLED(phy_status));
    printf("FIFO empty  = %d\n", PHY_FIFO_EMPTY(phy_status));
    printf("FIFO full   = %d\n", PHY_FIFO_FULL(phy_status));
    printf("TRX_RUNNING = %d\n", PHY_TRX_RUNNING(phy_status));
    printf("PLL on      = %d\n", PHY_PLL_ON(phy_status));
    return 0;
}

static int set_state(int argc, char **argv)
{
    if(argc != 2) return -1;
    uint8_t to_state = atoi(argv[1]);
    printf("Setting state (acc. to phy_status) to: %d\n", to_state);
    treufunk_set_state(&myTreufunk, to_state);
    return 0;
}

static int get_state(int argc, char **argv)
{
    printf("Current state (acc. to phy_status) = %d\n", treufunk_get_state(&myTreufunk));
    return 0;
}

static int set_channel(int argc, char **argv)
{
    if(argc != 2) return -1;
    uint8_t channel = atoi(argv[1]);
    printf("Setting channel %d\n", channel);
    treufunk_set_chan(&myTreufunk, channel);
    return 0;
}

static int fifo_write(int argc, char **argv)
{
    if(argc != 2) return -1;
    uint8_t len = strlen(argv[1]);
    printf("Data: ");
    for(int i = 0; i < len; i++)
    {
        printf("%02x ", (uint8_t)(argv[1][i]));
    }
    puts("");
    printf("Writing %d bytes to FIFO\n", len);
    treufunk_fifo_write(&myTreufunk, (uint8_t *)argv[1], len);

    return 0;

}

static int pll_frac(int argc, char **argv)
{
    if(argc < 2)
    {
        puts("usage: pllfrac <tx/rx> <frac_val (optional)>");
        return -1;
    }
    if(strcmp(argv[1], "tx") == 0)
    {
        if(argc == 2)
        {
            printf("SM_TX_CHAN_FRAC_H = 0x%02x\n", treufunk_sub_reg_read(&myTreufunk, SR_TX_CHAN_FRAC_H));
            printf("SM_TX_CHAN_FRAC_M = 0x%02x\n", treufunk_sub_reg_read(&myTreufunk, SR_TX_CHAN_FRAC_M));
            printf("SM_TX_CHAN_FRAC_L = 0x%02x\n", treufunk_sub_reg_read(&myTreufunk, SR_TX_CHAN_FRAC_L));
            return 0;
        }
        else if(argc == 3)
        {
            //uint32_t pll_frac = (uint32_t) strtol(argv[2], NULL, 16);
            uint32_t pll_frac = atoi(argv[2]);
            printf("Setting frac of TX pll to %lu (0x%06x)\n", pll_frac, pll_frac);
            treufunk_set_tx_pll_frac(&myTreufunk, pll_frac);
            return 0;
        }
    }
    else if(strcmp(argv[1], "rx") == 0)
    {
        if(argc == 2)
        {
            printf("SM_RX_CHAN_FRAC_H = 0x%02x\n", treufunk_sub_reg_read(&myTreufunk, SR_RX_CHAN_FRAC_H));
            printf("SM_RX_CHAN_FRAC_M = 0x%02x\n", treufunk_sub_reg_read(&myTreufunk, SR_RX_CHAN_FRAC_M));
            printf("SM_RX_CHAN_FRAC_L = 0x%02x\n", treufunk_sub_reg_read(&myTreufunk, SR_RX_CHAN_FRAC_L));
            return 0;
        }
        else if(argc == 3)
        {
            uint32_t pll_frac = atoi(argv[2]);
            printf("Setting frac of RX pll to %lu (0x%06x)\n", pll_frac, pll_frac);
            treufunk_set_rx_pll_frac(&myTreufunk, pll_frac);
            return 0;
        }
    }
    else
        puts("usage: pllfrac <tx/rx> <frac_val (optional)>");

    return 0;
}

static int print_ldo_values(int argc, char **argv)
{
    puts("LDO register values:");
    printf("LDO_A_VOUT    = 0x%02x\n", treufunk_sub_reg_read(&myTreufunk, SR_LDO_A_VOUT));
    printf("LDO_D_VOUT    = 0x%02x\n", treufunk_sub_reg_read(&myTreufunk, SR_LDO_D_VOUT));
    printf("LDO_PLL_VOUT  = 0x%02x\n", treufunk_sub_reg_read(&myTreufunk, SR_LDO_PLL_VOUT));
    printf("LDO_VCO_VOUT  = 0x%02x\n", treufunk_sub_reg_read(&myTreufunk, SR_LDO_VCO_VOUT));
    printf("LDO_TX24_VOUT = 0x%02x\n", treufunk_sub_reg_read(&myTreufunk, SR_LDO_TX24_VOUT));

    return 0;
}

static uint32_t vco = 10;
static int vco_tune(int argc, char **argv)
{
    if(argc == 1)
    {
        vco = treufunk_sub_reg_read(&myTreufunk, SR_PLL_VCO_TUNE);
        printf("SR_PLL_VCO_TUNE = %d\n", vco);
        return 0;
    }
    else if(argc == 2)
    {
        vco = atoi(argv[1]);
        printf("Setting VCO tune to %lu\n", vco);
        treufunk_set_vco_tune(&myTreufunk, vco);
    }
    else
        puts("usage: vco <tune_val(decimal) (optional)>");

    return 0;
}

static int vco_step_up(int argc, char **argv)
{
    if(argc != 2) return -1;
    uint8_t step = atoi(argv[1]);
    vco += step;
    if(vco > 255) vco = 10;
    printf("Setting VCO tune to %lu\n", vco);
    treufunk_set_vco_tune(&myTreufunk, vco);
    return 0;
}

static int vco_step_dn(int argc, char **argv)
{
    if(argc != 2) return -1;
    uint8_t step = atoi(argv[1]);
    vco -= step;
    if(vco <= 0 || vco > 255) vco = 250;
    printf("Setting VCO tune to %lu\n", vco);
    treufunk_set_vco_tune(&myTreufunk, vco);
    return 0;
}

static int pll_int(int argc, char **argv)
{
    if(argc < 2)
    {
        puts("usage: pllint <tx/rx> <int_val(decimal) (optional)>");
        return -1;
    }
    if(strcmp(argv[1], "tx") == 0)
    {
        if(argc == 2)
        {
            printf("SR_TX_CHAN_INT = %d\n", treufunk_sub_reg_read(&myTreufunk, SR_TX_CHAN_INT));
            return 0;
        }
        else if(argc == 3)
        {
            uint32_t pll_int = atoi(argv[2]);
            printf("Setting INT of TX pll to %lu\n", pll_int);
            treufunk_sub_reg_write(&myTreufunk, SR_TX_CHAN_INT, pll_int);
            return 0;
        }
    }
    else if(strcmp(argv[1], "rx") == 0)
    {
        if(argc == 2)
        {
            printf("SR_RX_CHAN_INT = %d\n", treufunk_sub_reg_read(&myTreufunk, SR_RX_CHAN_INT));
            return 0;
        }
        else if(argc == 3)
        {
            uint32_t pll_int = atoi(argv[2]);
            printf("Setting INT of RX pll to %lu\n", pll_int);
            treufunk_sub_reg_write(&myTreufunk, SR_RX_CHAN_INT, pll_int);
            return 0;
        }
    }
    else
        puts("usage: pllint <tx/rx> <int_val (optional)>");

    return 0;
}

static const shell_command_t shell_commands[] = {
    { "rr", "read a register", reg_read },
    { "rw", "write to a register", reg_write },
    { "tx", "send something over the air", send },
    { "txones", "send all ones", send_all_ones },
    { "rx", "read FIFO containing data", receive },
    { "phy", "get phy_status", get_phy },
    { "ss", "set state", set_state },
    { "gs", "get current state", get_state },
    { "setchan", "set channel", set_channel },
    { "fw", "write to fifo", fifo_write },
    { "pllfrac", "get/set frac of rx or tx pll", pll_frac },
    { "ldos", "print LDO values", print_ldo_values },
    { "vco", "get/set VCO tune", vco_tune },
    { "vcostepup", "increase vco tune by x", vco_step_up },
    { "vcostepdn", "decrease vco tune by x", vco_step_dn },
    { "pllint", "get/set PLL int of rx or tx", pll_int },
    { NULL, NULL, NULL }
};

int main(void)
{
    const treufunk_params_t *p = &treufunk_params[0];
    netdev_t *dev = (netdev_t *)(&myTreufunk);
    printf("Initializing Trefunk radio at SPI_%d\n", p->spi);
    treufunk_setup(&myTreufunk, p);
    dev->driver->init(dev);

    /* start the shell */
    puts("Initialization successful - starting the shell now");

    /* define buffer to be used by the shell */
    char line_buf[SHELL_DEFAULT_BUFSIZE];

    /* define own shell commands */
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
