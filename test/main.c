/**
 * K. Yasotharan
 * Adapted after test program for AT86RF2XX
 */
#include <stdio.h>

#include "net/netdev.h"
#include "shell.h"
#include "shell_commands.h"
#include "thread.h"
#include "xtimer.h"

#include "common.h"

#define _STACKSIZE      (THREAD_STACKSIZE_DEFAULT + THREAD_EXTRA_STACKSIZE_PRINTF)
#define MSG_TYPE_ISR    (0x3456)

static char stack[_STACKSIZE];
static kernel_pid_t _recv_pid;

treufunk_t devs[TREUFUNK_NUM];

static const shell_command_t shell_commands[] = {
    { "ifconfig", "Configure netdev", ifconfig },
    { "txtsnd", "Send IEEE 802.15.4 packet", txtsnd },
    { "changestate", "Simulate state change due to packet reception or transmission", change_state },
    { "changefifo", "Simulate fifo change due to packet reception or transmission", change_fifo },
    { NULL, NULL, NULL }
};

static void _event_cb(netdev_t *dev, netdev_event_t event)
{
    if (event == NETDEV_EVENT_ISR) {
        msg_t msg;

        msg.type = MSG_TYPE_ISR;
        msg.content.ptr = dev;

        if (msg_send(&msg, _recv_pid) <= 0) {
            puts("gnrc_netdev: possibly lost interrupt.");
        }
    }
    else {
        switch (event) {
            case NETDEV_EVENT_RX_COMPLETE:
            {
                recv(dev);

                break;
            }
            default:
                puts("Unexpected event received");
                break;
        }
    }
}

void *_recv_thread(void *arg)
{
    while (1) {
        msg_t msg;
        msg_receive(&msg);
        if (msg.type == MSG_TYPE_ISR) {
            netdev_t *dev = msg.content.ptr;
            dev->driver->isr(dev);
        }
        else {
            puts("unexpected message type");
        }
    }
}

int main(void)
{
    puts("Treufunk device driver test");
    xtimer_init();

    for (unsigned i = 0; i < TREUFUNK_NUM; i++) {
        netdev_t *dev = (netdev_t *)(&devs[i]);

        printf("Initializing Treufunk radio\n");
        treufunk_setup(&devs[i]);
        dev->event_callback = _event_cb;
        dev->driver->init(dev);
    }

    _recv_pid = thread_create(stack, sizeof(stack), THREAD_PRIORITY_MAIN - 1,
                              THREAD_CREATE_STACKTEST, _recv_thread, NULL,
                              "recv_thread");

    if (_recv_pid <= KERNEL_PID_UNDEF) {
        puts("Creation of receiver thread failed");
        return 1;
    }

    /* start the shell */
    puts("Initialization successful - starting the shell now");

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
