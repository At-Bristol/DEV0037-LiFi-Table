/*! 
 * \file      purelifi_mac_driver_usb_interface.h
 * \brief     pureLiFi driver USB interface
 * \details   USB MAC-Driver interface definition.
 * \author    Angelos Spanos
 * \version   1.0
 * \date      24-Apr-2015
 * \copyright Pure LiFi
 */

#ifndef _PURELIFI_MAC_DRIVER_USB_INTERFACE
#define _PURELIFI_MAC_DRIVER_USB_INTERFACE
#define PURELIFI_BYTE_NUM_ALIGNMENT 4
#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif
#define AP_USER_LIMIT 8

struct rx_status {
    u16 rssi;
    u8  rate_idx;
    u8  pad;
    u64 crc_error_count;
} __attribute__((packed));

typedef struct rx_status rx_status_t;

typedef enum {
    USB_REQ_TEST_WR            = 0,
    USB_REQ_MAC_WR             = 1,
    USB_REQ_POWER_WR           = 2,
    USB_REQ_RXTX_WR            = 3,
    USB_REQ_BEACON_WR          = 4,
    USB_REQ_BEACON_INTERVAL_WR = 5,
    USB_REQ_RTS_CTS_RATE_WR    = 6,
    USB_REQ_HASH_WR            = 7,
    USB_REQ_DATA_TX            = 8,
    USB_REQ_RATE_WR            = 9
} usb_req_enum_t;

typedef struct {
    usb_req_enum_t id;
    u32            len;
    u8             buf[512];
} usb_req_t;

typedef struct {
    rx_status_t status;
    u32         buf_len;
    u8          buf [4*512];
} usb_rx_packet_t;
#endif
