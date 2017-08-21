#include <stdio.h>
#include <stdlib.h> /* strtol(), atoi() */
#include <stdint.h>
#include <string.h> /* strlen() */

#include "shell.h"
#include "shell_commands.h"

#include "treufunk.h"
#include "treufunk_params.h"
// maybe also #include "treufunk_internal.h"

#include "net/netdev.h"

treufunk_t myTreufunk;

static int reg_read(int argc, char **argv)
{
    if(argc != 2) return -1;
    uint8_t addr = strtol(argv[1], NULL, 16);
    uint8_t val;
    val = treufunk_reg_read(myTreufunk, addr);
    printf("Register 0x%02x: 0x%02x\n", addr, val);
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
    if(argc != 2) return -1;
    uint8_t len = strlen(argv[1]);
    printf("Data: ");
    for(int i = 0; i < len; i++)
    {
        printf("%02x ", (uint8_t)(argv[1][i]));
    }
    puts("");
    printf("Sending %d bytes\n", len);
    treufunk_send(&myTreufunk, (uint8_t *)argv[1], len);
    return 0;
}

static int receive(int argc, char **argv)
{
    uint8_t buf_len = 127;
    uint8_t buf[buf_len];
    treufunk_fifo_read(&myTreufunk, buf, buf_len);
    puts("Received Data:");
    for(int i = 0; i < buf_len; i++)
    {
        printf("0x%02x ", buf[i]);
    }
    puts("");
    return 0;
}

static int get_phy(int argc, char **argv)
{
    printf("phy_status = 0x%02x\n", treufunk_get_phy_status(&myTreufunk));
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

static const shell_command_t shell_commands[] = {
    { "rr", "read a register", reg_read },
    { "rw", "write to a register", reg_write },
    { "tx", "send something over the air", send },
    { "rx", "read FIFO containing data", receive },
    { "phy", "get phy_status", get_phy },
    { "ss", "set state", set_state },
    { "gs", "get current state", get_state },
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
