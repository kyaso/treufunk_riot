#include <stdio.h>
#include <stdlib.h> /* strtol(), atoi() */
#include <stdint.h>
#include <string.h> /* strlen() */

#include "shell.h"
#include "shell_commands.h"

#include "treufunk.h"
// maybe also #include "treufunk_internal.h"

#include "net/netdev.h"

treufunk_t myTreufunk;


static int set_phy(int argc, char **argv)
{
    if(argc != 2) return -1;
    uint8_t phy = strtol(argv[1], NULL, 16);
    printf("Setting phy_status to : 0x%02x\n", phy);
    myTreufunk.phy_status = phy;
    return 0;
}

static const shell_command_t shell_commands[] = {
    { "setphy", "simulate phy_status", set_phy },
    { NULL, NULL, NULL }
};

static void _event_cb(netdev_t *dev, netdev_event_t event)
{
    if (event == NETDEV_EVENT_ISR) {
        dev->driver->isr(dev);
    }
    else {
        switch (event) {
            case NETDEV_EVENT_RX_COMPLETE:
            {
                puts("Callback: RX_COMPLETE");

                break;
            }
            default:
                puts("Unexpected event received");
                break;
        }
    }
}

int main(void)
{
    xtimer_init();
    netdev_t *dev = (netdev_t *)(&myTreufunk);
    puts("Initializing Trefunk");
    treufunk_setup(&myTreufunk);
    dev->event_callback = _event_cb;
    dev->driver->init(dev);

    /* start the shell */
    puts("Initialization successful - starting the shell now");

    /* define buffer to be used by the shell */
    char line_buf[SHELL_DEFAULT_BUFSIZE];

    /* define own shell commands */
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
