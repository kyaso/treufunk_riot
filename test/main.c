#include <stdio.h>

#include "treufunk.h"
#include "treufunk_params.h"
#include "net/netdev.h"

treufunk_t tf;

static void recv(netdev_t *dev)
{
    return;
}

static void _event_cb(netdev_t *dev, netdev_event_t event)
{
    if(event == NETDEV_EVENT_ISR)
    {
        dev->driver->isr(dev);
    }
    else
    {
        switch (event)
        {
            case NETDEV_EVENT_RX_COMPLETE:
                recv(dev);
                break;
            case NETDEV_EVENT_TX_COMPLETE:
                puts("TX complete event");
                break;
            default:
                puts("Unexpected event received");
                break;
        }
    }
}

int main(void)
{
    puts("Treufunk software simulation test");
    xtimer_init();
    
    const treufunk_params_t *p = &treufunk_params[0];
    netdev_t *dev = (netdev_t *)&tf;

    treufunk_setup(&tf, p);
    dev->event_callback = _event_cb;
    dev->driver->init(dev);

    return 0;
}
