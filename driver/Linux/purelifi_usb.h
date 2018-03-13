/* pureLiFi-X USB-WLAN driver for Linux
 *
 * Copyright (C) 2015-2016 pureLiFi <info@purelifi.com>
 * Copyright (C) 2006-2007 Daniel Drake <dsd@gentoo.org>
 * Copyright (C) 2005-2007 Ulrich Kunitz <kune@deine-taler.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _PURELIFI_USB_H
#define _PURELIFI_USB_H

#include <linux/completion.h>
#include <linux/netdevice.h>
#include <linux/spinlock.h>
#include <linux/skbuff.h>
#include <linux/usb.h>

#include "purelifi_def.h"
#include "purelifi_mac_driver_usb_interface.h"

#define TO_NETWORK(X)    (((X & 0xFF000000) >> 24) | ((X & 0xFF0000) >> 8) | ((X & 0xFF00) << 8) | ((X & 0xFF) << 24))
#define TO_NETWORK_16(X) (((X & 0xFF00) >> 8) | ((X & 0x00FF) << 8))
#define TO_NETWORK_32(X) TO_NETWORK(X)

#define TO_HOST(X)    TO_NETWORK(X)
#define TO_HOST_16(X) TO_NETWORK_16(X)
#define TO_HOST_32(X) TO_NETWORK_32(X)

#define USB_BULK_MSG_TIMEOUT_MS 2000

int usb_write_req(const u8 * buffer, int buffer_len, usb_req_enum_t usb_req_id);
void tx_urb_complete(struct urb *urb);

enum {
   USB_MAX_RX_SIZE       = 4800,
   USB_MAX_EP_INT_BUFFER = 64,
};


/* USB interrupt */
struct purelifi_usb_interrupt {
   spinlock_t lock;
   struct urb *urb;
   void *buffer;
   int interval;
};

#define RX_URBS_COUNT 5

struct purelifi_usb_rx {
   spinlock_t lock;
   struct mutex setup_mutex;
   struct delayed_work idle_work;
   struct tasklet_struct reset_timer_tasklet;
   u8 fragment[2 * USB_MAX_RX_SIZE];
   unsigned int fragment_length;
   unsigned int usb_packet_size;
   struct urb **urbs;
   int urbs_count;
};


typedef struct station_str {
   //  7...3    |    2     |     1     |     0     |
   // Reserved  | Hearbeat | FIFO full | Connected |
   unsigned char flag; 
   unsigned char mac[ETH_ALEN];
   struct sk_buff_head data_list;
} station_t;
#define STATION_CONNECTED_FLAG 0x1
#define STATION_FIFO_FULL_FLAG 0x2
#define STATION_HEARTBEAT_FLAG 0x4

/**
 * struct purelifi_usb_tx - structure used for transmitting frames
 * @enabled: atomic enabled flag, indicates whether tx is enabled
 * @lock: lock for transmission
 * @submitted: anchor for URBs sent to device
 * @submitted_urbs: atomic integer that counts the URBs having sent to the
 *   device, which haven't been completed
 * @stopped: indicates whether higher level tx queues are stopped
 */
#define STA_BROADCAST_INDEX (AP_USER_LIMIT)
#define MAX_STA_NUM         (AP_USER_LIMIT + 1)
struct purelifi_usb_tx {
   atomic_t enabled;
   spinlock_t lock;
   u8 mac_fifo_full;
   struct sk_buff_head submitted_skbs;
   struct usb_anchor submitted;
   int submitted_urbs;
   u8 stopped:1;
   struct timer_list tx_retry_timer;
   station_t station[MAX_STA_NUM];
};

/* Contains the usb parts. The structure doesn't require a lock because intf
 * will not be changed after initialization.
 */
struct purelifi_usb {
   struct timer_list sta_queue_cleanup;
   struct purelifi_usb_rx rx;
   struct purelifi_usb_tx tx;
   struct usb_interface *intf;
   u8 req_buf[64]; /* purelifi_usb_iowrite16v needs 62 bytes */
   u8 initialized:1, was_running:1;
};

enum endpoints {
	EP_DATA_IN  = 2,
	EP_DATA_OUT = 8,
};

enum devicetype {
	DEVICE_LIFI_X  = 0,
	DEVICE_LIFI_XC  = 1,
};

int usb_write_req_async(struct purelifi_usb *usb, const u8 * buffer, int buffer_len, usb_req_enum_t usb_req_id, usb_complete_t complete_fn, void * context);

#define purelifi_usb_dev(usb) (&usb->intf->dev)
static inline struct usb_device *purelifi_usb_to_usbdev(struct purelifi_usb *usb) {
   return interface_to_usbdev(usb->intf);
}
static inline struct ieee80211_hw *purelifi_intf_to_hw(struct usb_interface *intf) {
   return usb_get_intfdata(intf);
}
static inline struct ieee80211_hw *purelifi_usb_to_hw(struct purelifi_usb *usb) {
   return purelifi_intf_to_hw(usb->intf);
}

void purelifi_usb_init(struct purelifi_usb *usb, struct ieee80211_hw *hw, struct usb_interface *intf);
int purelifi_usb_init_hw(struct purelifi_usb *usb);
void purelifi_usb_clear(struct purelifi_usb *usb);
int purelifi_usb_scnprint_id(struct purelifi_usb *usb, char *buffer, size_t size);
int purelifi_usb_enable_rx(struct purelifi_usb *usb);
void purelifi_usb_disable_rx(struct purelifi_usb *usb);
void purelifi_usb_enable_tx(struct purelifi_usb *usb);
void purelifi_usb_disable_tx(struct purelifi_usb *usb);
int purelifi_usb_tx(struct purelifi_usb *usb, struct sk_buff *skb);
#endif
