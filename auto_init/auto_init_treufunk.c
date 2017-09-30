#ifdef MODULE_TREUFUNK

#include "log.h"
#include "board.h"
#include "net/gnrc/netdev.h"
#include "net/gnrc/netdev/ieee802154.h"
#include "net/gnrc.h"

#include "treufunk.h"
#include "treufunk.h"

/**
 * @brief   MAC layer stack parameters
 * @{
 */
#define TREUFUNK_MAC_STACKSIZE           (THREAD_STACKSIZE_MAIN)
#ifndef TREUFUNK_MAC_PRIO
#define TREUFUNK_MAC_PRIO                (GNRC_NETDEV_MAC_PRIO)
#endif
/** @} */

/**
 * @brief   Get the number of configured TREUFUNK devices
 */
#define TREUFUNK_NUM				    (sizeof(treufunk_params) / sizeof(treufunk_params[0]))

/**
 * @brief   Allocate memory for dev descriptors, stacks, and 802.15.4 adaption
 * @{
 */
static treufunk_t treufunk_devs[TREUFUNK_NUM];
static gnrc_netdev_t gnrc_adpt[TREUFUNK_NUM];
static char _treufunk_stacks[TREUFUNK_NUM][TREUFUNK_MAC_STACKSIZE];
/** @} */

void auto_init_treufunk(void)
{
    for (unsigned i = 0; i < TREUFUNK_NUM; i++) {
        LOG_DEBUG("[auto_init_netif] initializing treufunk #%u\n", i);

        treufunk_setup(&treufunk_devs[i], &treufunk_params[i]);
        int res = gnrc_netdev_ieee802154_init(&gnrc_adpt[i],
                                              (netdev_ieee802154_t *)&treufunk_devs[i]);

        if (res < 0) {
            LOG_ERROR("[auto_init_netif] error initializing treufunk #%u\n", i);
        }
        else {
            gnrc_netdev_init(_treufunk_stacks[i],
                             TREUFUNK_MAC_STACKSIZE,
                             TREUFUNK_MAC_PRIO,
                             "treufunk", &gnrc_adpt[i]);
        }
    }
}

#else
typedef int dont_be_pedantic;
#endif /* MODULE_TREUFUNK */

/** @} */

