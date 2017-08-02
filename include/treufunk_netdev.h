/*
*   treufunk_netdev.h
*   Kirthihan Yasotharan
*
*   Netdev interface to the Treufunk driver
*/

#ifndef TREUFUNK_NETDEV_H
#define TREUFUNK_NETDEV_H

#include "net/netdev.h"

#ifdef __cplusplus
extern "C" {
#endif

// Reference to the netdev device driver struct
extern const netdev_driver_t treufunk_driver;

#ifdef __cplusplus
}
#endif

#endif /*TREUFUNK_NETDEV_H */
