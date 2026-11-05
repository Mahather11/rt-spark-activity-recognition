#ifndef WIFI_ISM43362_CONFIG_H
#define WIFI_ISM43362_CONFIG_H

#include "Driver_SPI.h"
#include "cmsis_os2.h"

// CMSIS-Driver 2.10.0 configuration (see the pack's Config template).
#define WIFI_ISM43362_DRV_NUM               0
#define WIFI_ISM43362_SPI_DRV_NUM           2
#define WIFI_ISM43362_MEM_POOL_SIZE         16384

// Number of available sockets
#define WIFI_ISM43362_SOCKETS_NUM           4

// SPI Bus Speed in Hz
#define WIFI_ISM43362_SPI_BUS_SPEED         20000000

// SPI Timeout in milliseconds
#define WIFI_ISM43362_SPI_TIMEOUT           1000

// Maximum SPI Receive Size in bytes
#define WIFI_ISM43362_SPI_RECEIVE_SIZE      32

// Command Timeout in milliseconds
#define WIFI_ISM43362_CMD_TIMEOUT           30000

// Priority of the asynchronous thread
#define WIFI_ISM43362_ASYNC_THREAD_STACK_SIZE 1024
#define WIFI_ISM43362_ASYNC_PRIORITY       osPriorityAboveNormal

// Interval for asynchronous thread polling in milliseconds
#define WIFI_ISM43362_ASYNC_INTERVAL       8

#endif
