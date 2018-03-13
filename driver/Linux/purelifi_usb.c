/* 
 * pureLiFi-X USB-WLAN driver for Linux
 * Driver has been based on the zd1211rw driver.
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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/firmware.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/skbuff.h>
#include <linux/usb.h>
#include <linux/workqueue.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <net/mac80211.h>
#include <asm/unaligned.h>
#include <linux/gpio.h>
#include <linux/version.h>

#include "purelifi_def.h"
#include "purelifi_mac.h"
#include "purelifi_usb.h"
#include "purelifi_log.h"

#ifndef FCS_LEN
#define FCS_LEN 4
#endif

#define PURELIFI_X_VENDOR_ID_0  0x16C1
#define PURELIFI_X_PRODUCT_ID_0 0x1CDE
#define PURELIFI_XC_VENDOR_ID_0  0x2EF5
#define PURELIFI_XC_PRODUCT_ID_0 0x0008

#define PIN_TO_GPIO(bank_index, pin_index) ((bank_index*32)+pin_index)
#define EXPANSION_PORT0_1  PIN_TO_GPIO(2, 20)
#define EXPANSION_PORT0_6  PIN_TO_GPIO(1,  9)
#define EXPANSION_PORT1_1  PIN_TO_GPIO(3,  9)
#define EXPANSION_PORT1_6  PIN_TO_GPIO(2, 19)
#define GPIO_GREEN         PIN_TO_GPIO(0, 16)
#define GPIO_RED           99
#define GPIO_LOW                0
#define GPIO_HIGH               1

#undef  LOAD_MAC_AND_SERIAL_FROM_FILE
#undef  LOAD_MAC_AND_SERIAL_FROM_FLASH
#define LOAD_MAC_AND_SERIAL_FROM_EP0

#define PROC_FOLDER "purelifi"
#define MODULATION_FILENAME "modulation"
#define QUEUE_MANAGEMENT_DEBUG 0
struct proc_dir_entry * proc_folder;
struct proc_dir_entry * modulation_proc_entry;
static atomic_t data_queue_flag;

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,8,0)
    #define USB_ALLOC_URB(a, b) usb_alloc_urb(a, b)
    #define USB_ALLOC_COHERENT(a, b, c, d) usb_buffer_alloc(a, b, c, d)
    #define USB_FREE_COHERENT(a, b, c, d) usb_buffer_free(a, b, c, d)
    #define USB_FREE_URB(a) usb_free_urb(a)
#else
    #define USB_ALLOC_URB(a, b) usb_alloc_urb(a, b)
    #define USB_ALLOC_COHERENT(a, b, c, d) usb_alloc_coherent(a, b, c, d)
    #define USB_FREE_COHERENT(a, b, c, d) usb_free_coherent(a, b, c, d)
    #define USB_FREE_URB(a) usb_free_urb(a)
#endif
#ifdef OPENWRT
    #define GPIO_REQUEST(x, ...) gpio_request(x, ##__VA_ARGS__)
    #define GPIO_DIRECTION_OUTPUT(x, ...) gpio_direction_output(x, ##__VA_ARGS__)
    #define GPIO_SET_VALUE(x, ...) gpio_set_value(x, ##__VA_ARGS__)
#else
    #define GPIO_REQUEST(x, ...)		0
    #define GPIO_DIRECTION_OUTPUT(x, ...)	0
    #define GPIO_SET_VALUE(x, ...)		
#endif

/*
 * Define the duration of the Tx retry backoff timer (in milliseconds).
 */
# define TX_RETRY_BACKOFF_MS 10
# define STA_QUEUE_CLEANUP_MS 5000
/*
 * Define the duration of the Tx retry backoff timer (in jiffies).
 */
# define TX_RETRY_BACKOFF_JIFF ((TX_RETRY_BACKOFF_MS * HZ) / 1000)
# define STA_QUEUE_CLEANUP_JIFF ((STA_QUEUE_CLEANUP_MS * HZ) / 1000)

static struct usb_device_id usb_ids[] = {
    { USB_DEVICE(PURELIFI_X_VENDOR_ID_0, PURELIFI_X_PRODUCT_ID_0), .driver_info = DEVICE_LIFI_X},
    { USB_DEVICE(PURELIFI_XC_VENDOR_ID_0, PURELIFI_XC_PRODUCT_ID_0), .driver_info = DEVICE_LIFI_XC},
    {}
};

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("USB driver for devices with the pureLiFi-X chip.");
MODULE_AUTHOR("Angelos Spanos");
MODULE_VERSION("1.0");
MODULE_DEVICE_TABLE(usb, usb_ids);

struct usb_interface *ez_usb_interface;

static inline u16 get_bcdDevice(const struct usb_device *udev)
{
    return le16_to_cpu(udev->descriptor.bcdDevice);
}

/* Ensures that MAX_TRANSFER_SIZE is even. */
#define MAX_TRANSFER_SIZE (USB_MAX_TRANSFER_SIZE & ~1)

MODULE_FIRMWARE("purelifi/li_fi_x/fpga.bit");

#define urb_dev(urb) (&(urb)->dev->dev)


static void send_packet_from_data_queue(struct purelifi_usb *usb)
{
    struct sk_buff *skb = NULL;
    unsigned long flags;
    static u8 station_index = 0;
    u8 last_served_station_index;

    spin_lock_irqsave(&usb->tx.lock, flags);
    last_served_station_index = station_index;
    do {
        station_index = (station_index + 1) % MAX_STA_NUM;
        if ((usb->tx.station[station_index].flag & STATION_CONNECTED_FLAG)) {
            if (!(usb->tx.station[station_index].flag & STATION_FIFO_FULL_FLAG)) {
                skb = skb_peek(&usb->tx.station[station_index].data_list);
            }
        }
    }
    while ((station_index != last_served_station_index) && (skb == NULL));

    if (skb != NULL) {
        skb = skb_dequeue(&usb->tx.station[station_index].data_list);
        usb_write_req_async(usb, skb->data, skb->len, USB_REQ_DATA_TX, tx_urb_complete, skb);
        if (skb_queue_len(&usb->tx.station[station_index].data_list) <= 60) {
            GPIO_SET_VALUE(GPIO_RED, GPIO_LOW);
            block_queue(usb, usb->tx.station[station_index].mac, false);
        }
    }
    spin_unlock_irqrestore(&usb->tx.lock, flags);
}

static void handle_rx_packet(struct purelifi_usb *usb, const u8 *buffer,
                 unsigned int length)
{
    purelifi_mac_rx(purelifi_usb_to_hw(usb), buffer, length);
}

#define STATION_FIFO_ALMOST_FULL_MESSAGE     0
#define STATION_FIFO_ALMOST_FULL_NOT_MESSAGE 1
#define STATION_CONNECT_MESSAGE              2
#define STATION_DISCONNECT_MESSAGE           3
int rx_usb_enabled = 0;
extern int init;
static void rx_urb_complete(struct urb *urb)
{
    int r;
    struct purelifi_usb *usb;
    struct purelifi_usb_rx *rx;
    struct purelifi_usb_tx *tx;
    const u8 *buffer;
    static u8 fpga_link_connection_f = 0;
    unsigned int length;
    

    if (!urb) {
        printk(KERN_INFO "%s:URB is NULL.\n", __FUNCTION__);
        return;
    }
    else if (!urb->context) {
        printk(KERN_INFO "%s:!urb->context is NULL.\n", __FUNCTION__);
        return;
    }
    usb = urb->context;

    if (usb->initialized != 1) {
        return;
    }

    
    switch (urb->status) {
    case 0:
        break;
    case -ESHUTDOWN:
    case -EINVAL:
    case -ENODEV:
    case -ENOENT:
    case -ECONNRESET:
    case -EPIPE:
        dev_dbg_f(urb_dev(urb), "urb %p error %d\n", urb, urb->status);
        return;
    default:
        dev_dbg_f(urb_dev(urb), "urb %p error %d\n", urb, urb->status);
        goto resubmit;
    }

    buffer = urb->transfer_buffer;
    length = (*(u32 *)(buffer + sizeof(struct rx_status))) + sizeof(u32);

    rx = &usb->rx;
    tx = &usb->tx;

    if (urb->actual_length == 8) {
        u16 status = (buffer[6] << 8) | buffer[7];
        u8 station_index;
        pl_dev_info(&usb->intf->dev, "%s::Received status=%u\n", __FUNCTION__, status);
        pl_dev_info(
            &usb->intf->dev,  
            "%s::Tx packet MAC=%x:%x:%x:%x:%x:%x\n", 
            __FUNCTION__, 
            buffer[0],
            buffer[1],
            buffer[2],
            buffer[3],
            buffer[4],
            buffer[5]
        );
        if (status == STATION_FIFO_ALMOST_FULL_NOT_MESSAGE) {
            pl_dev_info(&usb->intf->dev, "STATION_FIFO_ALMOST_FULL_NOT_MESSAGE packet receipt.\n");
            GPIO_SET_VALUE(EXPANSION_PORT1_1, GPIO_HIGH);
            GPIO_SET_VALUE(GPIO_GREEN, GPIO_HIGH);
            #ifdef TYPE_AP
                for (station_index = 0; station_index < MAX_STA_NUM; station_index++) {
                    if (usb->tx.station[station_index].flag & STATION_CONNECTED_FLAG && !memcmp(usb->tx.station[station_index].mac, buffer, ETH_ALEN)) {
                        pl_dev_info(&usb->intf->dev, "%s::Setting FIFO flag at station_index = %d\n", __FUNCTION__, station_index);
                        usb->tx.station[station_index].flag |= STATION_FIFO_FULL_FLAG;
                        break;
                    }
                    else if (usb->tx.station[station_index].flag & STATION_CONNECTED_FLAG) {
                        pl_dev_info(
                            &usb->intf->dev,  
                            "%s::usb->tx.station[%d].mac=%x:%x:%x:%x:%x:%x\n", 
                            __FUNCTION__, 
                            station_index,
                            usb->tx.station[station_index].mac[0],
                            usb->tx.station[station_index].mac[1],
                            usb->tx.station[station_index].mac[2],
                            usb->tx.station[station_index].mac[3],
                            usb->tx.station[station_index].mac[4],
                            usb->tx.station[station_index].mac[5]
                        );
                    }
                }
                if (station_index == MAX_STA_NUM) {
                    pl_dev_warn(&usb->intf->dev, "%s::Station was not found.\n", __FUNCTION__);
                }
            #else
                tx->mac_fifo_full = 1;
                for (station_index = 0; station_index < MAX_STA_NUM; station_index++) {
                    usb->tx.station[station_index].flag |= STATION_FIFO_FULL_FLAG;
                }
            #endif
        }
        else if (status == STATION_FIFO_ALMOST_FULL_MESSAGE) {
            pl_dev_info(&usb->intf->dev, "STATION_FIFO_ALMOST_FULL_MESSAGE packet receipt.\n");
            GPIO_SET_VALUE(EXPANSION_PORT1_1, GPIO_LOW);
            GPIO_SET_VALUE(GPIO_GREEN, GPIO_LOW);
            #ifdef TYPE_AP
                for (station_index = 0; station_index < MAX_STA_NUM; station_index++) {
                    if (usb->tx.station[station_index].flag & STATION_CONNECTED_FLAG && !memcmp(usb->tx.station[station_index].mac, buffer, ETH_ALEN)) {
                        pl_dev_info(
                            &usb->intf->dev, 
                            "%s::Removing FIFO flag at station_index = %d\n", 
                            __FUNCTION__, 
                            station_index
                        );
                        usb->tx.station[station_index].flag &= 0xFD;
                        break;
                    }
                }
            #else
                for (station_index = 0; station_index < MAX_STA_NUM; station_index++) {
                    usb->tx.station[station_index].flag &= 0xFD;
                }
            #endif
            send_packet_from_data_queue(usb);
        }
        else if (status == STATION_CONNECT_MESSAGE) {
            fpga_link_connection_f = 1;
            pl_dev_info(&usb->intf->dev, "STATION_CONNECT_MESSAGE packet receipt.\n");
        }
        else if (status == STATION_DISCONNECT_MESSAGE) {
            fpga_link_connection_f = 0;
            pl_dev_info(&usb->intf->dev, "STATION_DISCONNECT_MESSAGE packet receipt.\n");
        }
    }
    else {
        if (usb->initialized && fpga_link_connection_f) {
            handle_rx_packet(usb, buffer, length);
        }
    }

resubmit:
    r = usb_submit_urb(urb, GFP_ATOMIC);
    if (r)
        dev_dbg_f(urb_dev(urb), "urb %p resubmit error %d\n", urb, r);
}

static struct urb *alloc_rx_urb(struct purelifi_usb *usb) {
    struct usb_device *udev = purelifi_usb_to_usbdev(usb);
    struct urb *urb;
    void *buffer;

    urb = USB_ALLOC_URB(0, GFP_KERNEL);
    if (!urb)
        return NULL;
    buffer = USB_ALLOC_COHERENT(udev, USB_MAX_RX_SIZE, GFP_KERNEL,
                    &urb->transfer_dma);
    if (!buffer) {

        USB_FREE_URB(urb);
        return NULL;
    }

    usb_fill_bulk_urb(urb, udev, usb_rcvbulkpipe(udev, EP_DATA_IN),
              buffer, USB_MAX_RX_SIZE,
              rx_urb_complete, usb);
    urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

    return urb;
}

static void free_rx_urb(struct urb *urb) {
    if (!urb)
        return;
    USB_FREE_COHERENT(urb->dev, urb->transfer_buffer_length,
              urb->transfer_buffer, urb->transfer_dma);
    USB_FREE_URB(urb);
}

static int __lf_x_usb_enable_rx(struct purelifi_usb *usb)
{
    int i, r;
    struct purelifi_usb_rx *rx = &usb->rx;
    struct urb **urbs;

    pl_dev_info(&usb->intf->dev, "%s:\n", __FUNCTION__);

    dev_dbg_f(purelifi_usb_dev(usb), "\n");

    r = -ENOMEM;
    urbs = kcalloc(RX_URBS_COUNT, sizeof(struct urb *), GFP_KERNEL);
    if (!urbs)
        goto error;
    for (i = 0; i < RX_URBS_COUNT; i++) {
        urbs[i] = alloc_rx_urb(usb);
        if (!urbs[i])
            goto error;
    }

    PURELIFI_ASSERT(!irqs_disabled());
    spin_lock_irq(&rx->lock);
    if (rx->urbs) {
        spin_unlock_irq(&rx->lock);
        r = 0;
        goto error;
    }
    rx->urbs = urbs;
    rx->urbs_count = RX_URBS_COUNT;
    spin_unlock_irq(&rx->lock);

    for (i = 0; i < RX_URBS_COUNT; i++) {
        r = usb_submit_urb(urbs[i], GFP_KERNEL);
        if (r)
            goto error_submit;
    }

    return 0;
error_submit:
    for (i = 0; i < RX_URBS_COUNT; i++) {
        usb_kill_urb(urbs[i]);
    }
    spin_lock_irq(&rx->lock);
    rx->urbs = NULL;
    rx->urbs_count = 0;
    spin_unlock_irq(&rx->lock);
error:
        if (urbs) {
        for (i = 0; i < RX_URBS_COUNT; i++)
            free_rx_urb(urbs[i]);
    }
    return r;
}

int purelifi_usb_enable_rx(struct purelifi_usb *usb)
{
    int r;
    struct purelifi_usb_rx *rx = &usb->rx;

    mutex_lock(&rx->setup_mutex);
    r = __lf_x_usb_enable_rx(usb);
    if (!r) rx_usb_enabled = 1;
    mutex_unlock(&rx->setup_mutex);

    return r;
}

static void __lf_x_usb_disable_rx(struct purelifi_usb *usb)
{
    int i;
    unsigned long flags;
    struct urb **urbs;
    unsigned int count;
    struct purelifi_usb_rx *rx = &usb->rx;

    pl_dev_info(&usb->intf->dev, "%s\n", __FUNCTION__);

    spin_lock_irqsave(&rx->lock, flags);
    urbs = rx->urbs;
    count = rx->urbs_count;
    spin_unlock_irqrestore(&rx->lock, flags);
    if (!urbs)
        return;

    for (i = 0; i < count; i++) {
        usb_kill_urb(urbs[i]);
        free_rx_urb(urbs[i]);
    }
    kfree(urbs);

    spin_lock_irqsave(&rx->lock, flags);
    rx->urbs = NULL;
    rx->urbs_count = 0;
    spin_unlock_irqrestore(&rx->lock, flags);
}

void purelifi_usb_disable_rx(struct purelifi_usb *usb)
{
    struct purelifi_usb_rx *rx = &usb->rx;

    pl_dev_info(&usb->intf->dev, "%s\n", __FUNCTION__);
    mutex_lock(&rx->setup_mutex);
    __lf_x_usb_disable_rx(usb);
    rx_usb_enabled = 0;
    mutex_unlock(&rx->setup_mutex);

    tasklet_kill(&rx->reset_timer_tasklet);
    cancel_delayed_work_sync(&rx->idle_work);
}

/**
 * purelifi_usb_disable_tx - disable transmission
 * @usb: the driver USB structure
 *
 * Frees all URBs in the free list and marks the transmission as disabled.
 */
void purelifi_usb_disable_tx(struct purelifi_usb *usb)
{
    struct purelifi_usb_tx *tx = &usb->tx;
    unsigned long flags;

    atomic_set(&tx->enabled, 0);

    /* kill all submitted tx-urbs */
    usb_kill_anchored_urbs(&tx->submitted);

    spin_lock_irqsave(&tx->lock, flags);
    WARN_ON(!skb_queue_empty(&tx->submitted_skbs));
    WARN_ON(tx->submitted_urbs != 0);
    tx->submitted_urbs = 0;
    spin_unlock_irqrestore(&tx->lock, flags);

    /* The stopped state is ignored, relying on ieee80211_wake_queues()
     * in a potentionally following purelifi_usb_enable_tx().
     */
}

/**
 * purelifi_usb_enable_tx - enables transmission
 * @usb: a &struct purelifi_usb pointer
 *
 * This function enables transmission and prepares the &purelifi_usb_tx data
 * structure.
 */
void purelifi_usb_enable_tx(struct purelifi_usb *usb)
{
    unsigned long flags;
    struct purelifi_usb_tx *tx = &usb->tx;

    spin_lock_irqsave(&tx->lock, flags);
    atomic_set(&tx->enabled, 1);
    tx->submitted_urbs = 0;
    ieee80211_wake_queues(purelifi_usb_to_hw(usb));
    tx->stopped = 0;
    spin_unlock_irqrestore(&tx->lock, flags);
}


/**
 * tx_urb_complete - completes the execution of an URB
 * @urb: a URB
 *
 * This function is called if the URB has been transferred to a device or an
 * error has happened.
 */
void tx_urb_complete(struct urb *urb)
{
	struct sk_buff *skb;
	struct ieee80211_tx_info *info;
	struct purelifi_usb *usb;
	struct purelifi_usb_tx *tx;

    GPIO_SET_VALUE(EXPANSION_PORT1_6, GPIO_HIGH);

	skb = (struct sk_buff *)urb->context;
	info = IEEE80211_SKB_CB(skb);
	/*
	 * grab 'usb' pointer before handing off the skb (since
	 * it might be freed by purelifi_mac_tx_to_dev or mac80211)
	 */
	usb = &purelifi_hw_mac(info->rate_driver_data[0])->chip.usb;
	tx = &usb->tx;

	switch (urb->status) {
	case 0:
		break;
	case -ESHUTDOWN:
	case -EINVAL:
	case -ENODEV:
	case -ENOENT:
	case -ECONNRESET:
	case -EPIPE:
		dev_dbg_f(urb_dev(urb), "urb %p error %d\n", urb, urb->status);
		break;
	default:
		dev_dbg_f(urb_dev(urb), "urb %p error %d\n", urb, urb->status);
        return;
	}

	purelifi_mac_tx_to_dev(skb, urb->status);
    send_packet_from_data_queue(usb);
    USB_FREE_URB(urb);
    GPIO_SET_VALUE(EXPANSION_PORT1_6, GPIO_LOW);
	return;
}



/**
 * purelifi_usb_tx: initiates transfer of a frame of the device
 *
 * @usb: the driver USB structure
 * @skb: a &struct sk_buff pointer
 *
 * This function tranmits a frame to the device. It doesn't wait for
 * completion. The frame must contain the control set and have all the
 * control set information available.
 *
 * The function returns 0 if the transfer has been successfully initiated.
 */
int purelifi_usb_tx(struct purelifi_usb *usb, struct sk_buff *skb)
{
    int r;
    struct ieee80211_tx_info *info = IEEE80211_SKB_CB(skb);
    struct usb_device *udev = purelifi_usb_to_usbdev(usb);
    struct urb *urb;
    struct purelifi_usb_tx *tx = &usb->tx;

    if (!atomic_read(&tx->enabled)) {
        r = -ENOENT;
        goto out;
    }

    urb = USB_ALLOC_URB(0, GFP_ATOMIC);
    if (!urb) {
        r = -ENOMEM;
        goto out;
    }

    usb_fill_bulk_urb(urb, udev, usb_sndbulkpipe(udev, EP_DATA_OUT),
                  skb->data, skb->len, tx_urb_complete, skb);

    info->rate_driver_data[1] = (void *)jiffies;
    skb_queue_tail(&tx->submitted_skbs, skb);
    usb_anchor_urb(urb, &tx->submitted);

    r = usb_submit_urb(urb, GFP_ATOMIC);
    if (r) {
        dev_dbg_f(purelifi_usb_dev(usb), "error submit urb %p %d\n", urb, r);
        usb_unanchor_urb(urb);
        skb_unlink(skb, &tx->submitted_skbs);
        goto error;
    }
    return 0;
error:
    USB_FREE_URB(urb);
out:
    return r;
}

static inline void init_usb_rx(struct purelifi_usb *usb)
{
    struct purelifi_usb_rx *rx = &usb->rx;

    spin_lock_init(&rx->lock);
    mutex_init(&rx->setup_mutex);
    if (interface_to_usbdev(usb->intf)->speed == USB_SPEED_HIGH) {
        rx->usb_packet_size = 512;
    } else {
        rx->usb_packet_size = 64;
    }
    PURELIFI_ASSERT(rx->fragment_length == 0);
}

static inline void init_usb_tx(struct purelifi_usb *usb)
{
    struct purelifi_usb_tx *tx = &usb->tx;

    spin_lock_init(&tx->lock);
    atomic_set(&tx->enabled, 0);
    tx->stopped = 0;
    skb_queue_head_init(&tx->submitted_skbs);
    init_usb_anchor(&tx->submitted);
}

void purelifi_usb_init(struct purelifi_usb *usb, struct ieee80211_hw *hw,
             struct usb_interface *intf)
{
    memset(usb, 0, sizeof(*usb));
    usb->intf = usb_get_intf(intf);
    usb_set_intfdata(usb->intf, hw);
    hw->conf.chandef.width = NL80211_CHAN_WIDTH_20;
    init_usb_tx(usb);
    init_usb_rx(usb);

}

void purelifi_usb_clear(struct purelifi_usb *usb)
{
    usb_set_intfdata(usb->intf, NULL);
    usb_put_intf(usb->intf);
    PURELIFI_MEMCLEAR(usb, sizeof(*usb));
    /* FIXME: usb_interrupt, usb_tx, usb_rx? */
}

static const char *speed(enum usb_device_speed speed)
{
    switch (speed) {
    case USB_SPEED_LOW:
        return "low";
    case USB_SPEED_FULL:
        return "full";
    case USB_SPEED_HIGH:
        return "high";
    default:
        return "unknown speed";
    }
}

static int scnprint_id(struct usb_device *udev, char *buffer, size_t size)
{
    return scnprintf(buffer, size, "%04hx:%04hx v%04hx %s",
        le16_to_cpu(udev->descriptor.idVendor),
        le16_to_cpu(udev->descriptor.idProduct),
        get_bcdDevice(udev),
        speed(udev->speed));
}

int purelifi_usb_scnprint_id(struct purelifi_usb *usb, char *buffer, size_t size)
{
    struct usb_device *udev = interface_to_usbdev(usb->intf);
    return scnprint_id(udev, buffer, size);
}

int purelifi_usb_init_hw(struct purelifi_usb *usb)
{
    int r;
    r = usb_reset_configuration(purelifi_usb_to_usbdev(usb));
    if (r) {
        pl_dev_err(purelifi_usb_dev(usb), "couldn't reset configuration. Error number %d\n", r);
        return r;
    }
    return 0;
}


static int send_vendor_request(struct usb_device *udev, int request, unsigned char *buffer, int buffer_size) {
    return usb_control_msg(
        udev,
        usb_rcvctrlpipe(udev, 0),
        request,
        0xC0,
        0,
        0,
        buffer,
        buffer_size,
        1000
    );
}

static int send_vendor_command(struct usb_device *udev, int request, unsigned char *buffer, int buffer_size) {
    return usb_control_msg(
        udev,
        usb_sndctrlpipe(udev, 0),
        request,
        0x40,
        0,
        0,
        buffer,
        buffer_size,
        1000
    );
}

static int download_fpga(struct usb_interface *intf) {
    int r, actual_length;
    struct firmware *fw = NULL;
    unsigned char *fw_data;
    unsigned char fpga_setting[2];
    unsigned char fpga_state[9];
    const char *fw_name;//[] = "purelifi/li_fi_x/fpga.bit";
    int fw_data_i;
    unsigned char *fpga_dmabuff;
    int bulk_transaction_len = 16384;

    fpga_dmabuff = NULL;
  
    if ((PURELIFI_X_VENDOR_ID_0 == le16_to_cpu(interface_to_usbdev(intf)->descriptor.idVendor)) 
		&& (PURELIFI_X_PRODUCT_ID_0 == le16_to_cpu(interface_to_usbdev(intf)->descriptor.idProduct)))
    {
        fw_name = "purelifi/li_fi_x/fpga.bit";
	pl_dev_info(&intf->dev, "%s::bit file for X selected.\n", __FUNCTION__);
    }
    else if ((PURELIFI_XC_VENDOR_ID_0 == le16_to_cpu(interface_to_usbdev(intf)->descriptor.idVendor)) 
		&& (PURELIFI_XC_PRODUCT_ID_0 == le16_to_cpu(interface_to_usbdev(intf)->descriptor.idProduct)))
    {
        fw_name = "purelifi/li_fi_x/fpga_xc.bit";
	pl_dev_info(&intf->dev, "%s::bit file for XC selected.\n", __FUNCTION__);
    }
    else
    {
	r = -EINVAL;
	goto error;
    }

    r = request_firmware((const struct firmware**)&fw, fw_name, &intf->dev);
    if (r) {
       pl_dev_err(&intf->dev, "%s::request_firmware failed.(%d)\n", __FUNCTION__, r);
       goto error;
    }

    fpga_dmabuff = (unsigned char *)kmalloc(2, GFP_KERNEL);

    if (fpga_dmabuff == NULL)
    {
	r = -ENOMEM;
	goto error_free_fw;
    }

    send_vendor_request(
        interface_to_usbdev(intf), 
        0x33, 
        fpga_dmabuff,//fpga_setting, 
        sizeof(fpga_setting)
    );

    memcpy(fpga_setting, fpga_dmabuff, 2);
    kfree(fpga_dmabuff);

    send_vendor_command(
        interface_to_usbdev(intf), 
        0x34, 
        NULL, 
        0
    );

    if (fpga_setting[0] != 6) {
        pl_dev_err(&intf->dev, "%s::fpga_setting[0] incorrect.\n", __FUNCTION__);
        r = -EINVAL;
        goto error_free_fw;
    }
    
    for (fw_data_i=0; fw_data_i<fw->size;) {
        int trans_buf_i;
        if ((fw->size - fw_data_i) < bulk_transaction_len) {
            bulk_transaction_len = fw->size - fw_data_i;
        }
        fw_data = (unsigned char*)kmalloc(bulk_transaction_len, GFP_KERNEL);

        memcpy(fw_data, &fw->data[fw_data_i], bulk_transaction_len);
        for (trans_buf_i=0; trans_buf_i < bulk_transaction_len; trans_buf_i++) {
            fw_data[trans_buf_i] = 
                ((fw_data[trans_buf_i] & 128) >> 7) |
                ((fw_data[trans_buf_i] &  64) >> 5) |
                ((fw_data[trans_buf_i] &  32) >> 3) |
                ((fw_data[trans_buf_i] &  16) >> 1) |
                ((fw_data[trans_buf_i] &   8) << 1) |
                ((fw_data[trans_buf_i] &   4) << 3) |
                ((fw_data[trans_buf_i] &   2) << 5) |
                ((fw_data[trans_buf_i] &   1) << 7);
        }
        r = usb_bulk_msg (
            interface_to_usbdev(intf),
            usb_sndbulkpipe(interface_to_usbdev(intf), fpga_setting[0] & 0xFF),
            fw_data,
            bulk_transaction_len,
            &actual_length,
            2000
        );

        if (r) {
            pl_dev_err(&intf->dev, "%s::usb_bulk_msg failed. (%d)\n", __FUNCTION__, r);
        }
		
		
		
        kfree(fw_data);

        fw_data_i += bulk_transaction_len;
    }

    fpga_dmabuff = NULL;
    fpga_dmabuff = (unsigned char *)kmalloc(9, GFP_KERNEL);
    if (fpga_dmabuff == NULL)
    {
	r = -ENOMEM;
	goto error_free_fw;
    }
    memset(fpga_dmabuff, 0xFF, 9);

    send_vendor_request(
        interface_to_usbdev(intf), 
        0x30, 
        fpga_dmabuff,//fpga_state, 
        sizeof(fpga_state)
    );
    pl_dev_info(
	&intf->dev,
	"fpga_status %x %x %x %x %x %x %x %x %x\n",
	fpga_dmabuff[0],
	fpga_dmabuff[1],
	fpga_dmabuff[2],
	fpga_dmabuff[3],
	fpga_dmabuff[4],
	fpga_dmabuff[5],
	fpga_dmabuff[6],
	fpga_dmabuff[7],
	fpga_dmabuff[8],
	__func__
    );
    if(fpga_dmabuff[0] != 0)//( fpga_state[0] != 0 )
    {
	r = -EINVAL;
	kfree(fpga_dmabuff);
	goto error_free_fw;
    }

    kfree(fpga_dmabuff);

    send_vendor_command(
        interface_to_usbdev(intf), 
        0x35, 
        NULL, 
        0
    );
	
	//wait because the EZ-USB FW also waits
	msleep(200);
	
error_free_fw:
    release_firmware(fw);
error:
    return r;
}

typedef struct flash_struct {
    unsigned char enabled;
    unsigned int sector_size;
    unsigned int sectors;
    unsigned char ec;
} flash_t;


static void pretty_print_mac(
    struct usb_interface *intf,
    char *serial_number,
    unsigned char *hw_address
) {
    unsigned char i;
    for (i = 0; i < ETH_ALEN; i++) 
        pl_dev_info(&intf->dev, "%s::hw_address[%d]=%x\n", __FUNCTION__, i, hw_address[i]);
}

static int upload_mac_and_serial_number(
    struct usb_interface *intf, 
    unsigned char *hw_address, 
    unsigned char *serial_number
) {
    #ifdef LOAD_MAC_AND_SERIAL_FROM_FILE 
        struct firmware *fw = NULL;
        int r;
        {
            char *mac_file_name = "purelifi/li_fi_x/mac.ascii";
            r = request_firmware((const struct firmware**)&fw, mac_file_name, &intf->dev);
            if (r) {
                pl_dev_err(&intf->dev, "%s::request_firmware fail.(%d)\n", __FUNCTION__, r);
                goto ERROR;
            }
            pl_dev_info(&intf->dev, "%s::fw->data=%s\n", __FUNCTION__, fw->data);
            sscanf(
                fw->data,
                "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                &hw_address[0],
                &hw_address[1],
                &hw_address[2],
                &hw_address[3], 
                &hw_address[4], 
                &hw_address[5]
            );
            release_firmware(fw);
        }
        {
            char *serial_file_name = "purelifi/li_fi_x/serial.ascii";
            r = request_firmware((const struct firmware**)&fw, serial_file_name, &intf->dev);
            if (r) {
                pl_dev_err(&intf->dev, "%s::request_firmware fail.(%d)\n", __FUNCTION__, r);
                goto ERROR;
            }
            pl_dev_info(&intf->dev, "%s::fw->data=%s\n", __FUNCTION__, fw->data);
            strcpy(serial_number, fw->data);
            pl_dev_info(&intf->dev, "%s::serial_number=%s\n", __FUNCTION__, serial_number);
            release_firmware(fw);
        }
        pretty_print_mac(intf, serial_number, hw_address);
    ERROR:
        return r;
    #endif
    #ifdef LOAD_MAC_AND_SERIAL_FROM_FLASH
        flash_t flash;
        unsigned char *flash_sector_buf;
        unsigned int sector_index;
    
        #define FLASH_EC_BUSY 3
        do {
            unsigned char buf[8];
            msleep(200);
    
            send_vendor_request(
                interface_to_usbdev(intf), 
                0x40,
                buf,
                sizeof(buf)
            );
            flash.enabled = buf[0] & 0xFF;
    
            if (flash.enabled) {
                flash.sectors = ((buf[6] & 255) << 24) | 
                                ((buf[5] & 255) << 16) | 
                                ((buf[4] & 255) <<  8) | 
                                ( buf[3] & 255);
                flash.sector_size = ((buf[2] & 255) << 8) | (buf[1] & 255);
            }
            else {
                flash.sectors = 0;
                flash.sector_size = 0;
            }
    
            if (flash.sector_size & 0x8000) {
                flash.sector_size = 1 << (flash.sector_size & 0x7FFF);
            }
    
            flash.ec = buf[7] & 0xFF;
        } while (flash.ec==FLASH_EC_BUSY);
        
        flash_sector_buf = (unsigned char *)kmalloc(flash.sector_size, GFP_KERNEL);
        if (!flash_sector_buf) {
            pl_dev_err(&intf->dev, "%s::flash_sector_buf memory allocation failed\n", __FUNCTION__);
            return -ENOMEM;
        }
        
        if ( !flash.enabled ) {
            pl_dev_err(&intf->dev, "%s:Flash memory is not enabled", __FUNCTION__);
            return -EINVAL;
        }
        
        read_flash(intf, &flash, 0, flash_sector_buf);
        sector_index = *((unsigned int*)flash_sector_buf);
    
        read_flash(intf, &flash, sector_index, flash_sector_buf);
        memcpy(hw_address, flash_sector_buf, ETH_ALEN);
        strncpy(serial_number, &flash_sector_buf[ETH_ALEN], 256);
    
        pretty_print_mac(intf, serial_number, hw_address);
        kfree(flash_sector_buf);
        return 0;
    #endif
    #ifdef LOAD_MAC_AND_SERIAL_FROM_EP0
        #define USB_MAC_VENDOR_REQUEST 0x36
        #define USB_SERIAL_NUMBER_VENDOR_REQUEST 0x37
        #define USB_FIRMWARE_VERSION_VENDOR_REQUEST 0x39
        #define SERIAL_NUMBER_LENGTH 14
	int r = 0;
	unsigned char *dma_buffer = NULL;
        //unsigned char hw_address_dma_buffer[ETH_ALEN];
        //unsigned char serial_number_dma_buffer[SERIAL_NUMBER_LENGTH];
        long long unsigned int firmware_version;

	dma_buffer = (unsigned char *)kmalloc(14, GFP_KERNEL);
	if (dma_buffer == NULL)
	{
		r = -ENOMEM;
		goto error;
	}
        send_vendor_request(
            interface_to_usbdev(intf), 
            USB_MAC_VENDOR_REQUEST, 
            dma_buffer,//hw_address_dma_buffer, 
            ETH_ALEN
        );
	memcpy(hw_address, dma_buffer, ETH_ALEN);

        send_vendor_request(
            interface_to_usbdev(intf), 
            USB_SERIAL_NUMBER_VENDOR_REQUEST, 
            dma_buffer,//serial_number_dma_buffer, 
            14
        );
        send_vendor_request(
            interface_to_usbdev(intf), 
            USB_SERIAL_NUMBER_VENDOR_REQUEST, 
            dma_buffer,//serial_number_dma_buffer, 
            14
        );
	memcpy(serial_number, dma_buffer, SERIAL_NUMBER_LENGTH);

	memset(dma_buffer, 0x00, 14);
        send_vendor_request(
            interface_to_usbdev(intf), 
            USB_FIRMWARE_VERSION_VENDOR_REQUEST, 
            (unsigned char *)dma_buffer,//(unsigned char *)&firmware_version, 
            8
        );
	memcpy(&firmware_version, dma_buffer, sizeof(long long unsigned int));
        dev_info(&intf->dev, "Firmware Version: %llu, $Revision: 10568 $\n", firmware_version);
        //memcpy(hw_address, hw_address_dma_buffer, ETH_ALEN);
        //memcpy(serial_number, serial_number_dma_buffer, SERIAL_NUMBER_LENGTH);
	kfree(dma_buffer);

        pretty_print_mac(intf, serial_number, hw_address);
error:
	return r;//return 0;
    #endif
}

static void get_usb_req(struct usb_device *udev, const u8 * buffer, int buffer_len, usb_req_enum_t usb_req_id, usb_req_t *usb_req) {
    u8 *buffer_dst_p = usb_req->buf;
    const u8 *buffer_src_p = buffer;
    u32 payload_len_nw = TO_NETWORK((buffer_len + FCS_LEN));

    usb_req->id = usb_req_id;
    usb_req->len  = 0;

    // Copy buffer length into the transmitted buffer, as it is important
    // for the Rx MAC to know its exact length.
    if ( usb_req->id == USB_REQ_BEACON_WR ) {
        memcpy(buffer_dst_p, &payload_len_nw, sizeof(payload_len_nw));
        buffer_dst_p += sizeof(payload_len_nw);
        usb_req->len += sizeof(payload_len_nw);
    }

    memcpy(buffer_dst_p, buffer_src_p, buffer_len);
    buffer_dst_p += buffer_len;
    buffer_src_p += buffer_len;
    usb_req->len +=  buffer_len;
    

    // Set the FCS_LEN (4) bytes as 0 for CRC checking.
    memset(buffer_dst_p, 0, FCS_LEN);
    buffer_dst_p += FCS_LEN;
    usb_req->len += FCS_LEN;

    // Round the packet to be transmitted to 4 bytes.
    if (usb_req->len % PURELIFI_BYTE_NUM_ALIGNMENT) {
        memset(buffer_dst_p, 0, PURELIFI_BYTE_NUM_ALIGNMENT - (usb_req->len % PURELIFI_BYTE_NUM_ALIGNMENT));
        buffer_dst_p += PURELIFI_BYTE_NUM_ALIGNMENT - (usb_req->len % PURELIFI_BYTE_NUM_ALIGNMENT);
        usb_req->len += PURELIFI_BYTE_NUM_ALIGNMENT - (usb_req->len % PURELIFI_BYTE_NUM_ALIGNMENT);
    }

    usb_req->id = TO_NETWORK(usb_req->id);
    usb_req->len = TO_NETWORK(usb_req->len);
}

int usb_write_req_async(struct purelifi_usb * usb, const u8 * buffer, int buffer_len, usb_req_enum_t usb_req_id, usb_complete_t complete_fn, void *context) {
    int r;
    struct usb_device *udev = interface_to_usbdev(ez_usb_interface);
    struct urb *urb = usb_alloc_urb(0, GFP_ATOMIC);

    usb_fill_bulk_urb(
        urb, 
        udev, 
        usb_sndbulkpipe(udev, EP_DATA_OUT),
    	(void*)buffer,
	buffer_len,
        complete_fn,
        context
    );

    r = usb_submit_urb(urb, GFP_ATOMIC);

    if ( r ) {
        pl_dev_err(&udev->dev, "%s::r = %d\n", __FUNCTION__, r);
    }
    return r;
}

int usb_write_req(const u8 * buffer, int buffer_len, usb_req_enum_t usb_req_id) {
    int r;
    int actual_length;
    int usb_bulk_msg_len;
    unsigned char *dma_buffer = NULL;
    struct usb_device *udev = interface_to_usbdev(ez_usb_interface/*purelifi_get_usb_interface()*/);
    usb_req_t usb_req;
    get_usb_req(udev, buffer, buffer_len, usb_req_id, &usb_req);
    usb_bulk_msg_len = sizeof(usb_req.id) + sizeof(usb_req.len) + TO_HOST(usb_req.len);

    dma_buffer = kzalloc(usb_bulk_msg_len, GFP_KERNEL);
    if (dma_buffer == NULL)
    {
	r = -ENOMEM;
	goto error;
    }
    memcpy(dma_buffer, &usb_req, usb_bulk_msg_len);

    r = usb_bulk_msg(
        udev,
        usb_sndbulkpipe(udev, EP_DATA_OUT),
        dma_buffer,//&usb_req,
        usb_bulk_msg_len,
        &actual_length,
        USB_BULK_MSG_TIMEOUT_MS
    );
    kfree(dma_buffer);
error:
    if ( r ) {
        pl_dev_err(&udev->dev, "%s::usb_bulk_msg unsuccesful.(%d)\n", __FUNCTION__, r);
    }
    return r;
}

static void slif_data_plane_sap_timer_callb(unsigned long data)
{
    struct purelifi_usb *usb = (struct purelifi_usb *)data;
    GPIO_SET_VALUE(EXPANSION_PORT0_6, GPIO_HIGH);

    send_packet_from_data_queue(usb);

    usb->tx.tx_retry_timer.expires = jiffies + TX_RETRY_BACKOFF_JIFF;
    usb->tx.tx_retry_timer.function = slif_data_plane_sap_timer_callb;
    usb->tx.tx_retry_timer.data = (unsigned long)usb;
    add_timer(&usb->tx.tx_retry_timer);
    GPIO_SET_VALUE(EXPANSION_PORT0_6, GPIO_LOW);
}

#if QUEUE_MANAGEMENT_DEBUG
    static void tx_station_pretty_print(struct purelifi_usb *usb) {
        int station_index;
        pl_dev_info(&usb->intf->dev, " | Queue | Heartbeat | FIFO full | Connected | MAC \n");
        for (station_index = 0; station_index < MAX_STA_NUM; station_index++) {
            pl_dev_info(&usb->intf->dev,
                " |   %d   |     %d     |     %d     |     %d     | %hhx:%hhx:%hhx:%hhx:%hhx:%hhx\n",
                station_index,
                ((usb->tx.station[station_index].flag & STATION_HEARTBEAT_FLAG)!=0),
                ((usb->tx.station[station_index].flag & STATION_FIFO_FULL_FLAG)!=0),
                ((usb->tx.station[station_index].flag & STATION_CONNECTED_FLAG)!=0),
                usb->tx.station[station_index].mac[0] & 0xFF,
                usb->tx.station[station_index].mac[1] & 0xFF,
                usb->tx.station[station_index].mac[2] & 0xFF,
                usb->tx.station[station_index].mac[3] & 0xFF,
                usb->tx.station[station_index].mac[4] & 0xFF,
                usb->tx.station[station_index].mac[5] & 0xFF
            );    
        }
    }
#endif

static void sta_queue_cleanup_timer_callb(unsigned long data) {
    int station_index;
    struct purelifi_usb *usb = (struct purelifi_usb *)data;
    for (station_index = 0; station_index < MAX_STA_NUM-1; station_index++) {
        if (usb->tx.station[station_index].flag & STATION_CONNECTED_FLAG) {
            if (usb->tx.station[station_index].flag & STATION_HEARTBEAT_FLAG) {
                usb->tx.station[station_index].flag ^= STATION_HEARTBEAT_FLAG;
            }
            else {
                memset(usb->tx.station[station_index].mac, 0, ETH_ALEN);
                usb->tx.station[station_index].flag = 0;
            }
        }
    }
    usb->sta_queue_cleanup.expires = jiffies + STA_QUEUE_CLEANUP_JIFF;
    usb->sta_queue_cleanup.function = sta_queue_cleanup_timer_callb;
    usb->sta_queue_cleanup.data = (unsigned long)usb;
    add_timer(&usb->sta_queue_cleanup);
}

static enum unit_type_t get_unit_type(char *serial_number, struct usb_interface *intf) {
    #ifdef TYPE_AP
        pl_dev_info(&intf->dev, "%s::Unit-type is an AP!\n", __FUNCTION__);
        return AP;
    #else
        pl_dev_info(&intf->dev, "%s::Unit-type is an STA!\n", __FUNCTION__);
        return STA;
    #endif
}

static int probe(struct usb_interface *intf, const struct usb_device_id *id) {
    int r=0;
    struct purelifi_usb *usb;
    struct purelifi_chip *chip;
    struct ieee80211_hw *hw = NULL;
    static unsigned char hw_address[ETH_ALEN];
    static unsigned char serial_number[256];
    unsigned int i;

    ez_usb_interface = intf;

    if ((hw = purelifi_mac_alloc_hw(intf)) == NULL) {
        r = -ENOMEM;
        goto error;
    }

    chip = &purelifi_hw_mac(hw)->chip;
    usb = &chip->usb;

    if ((r = upload_mac_and_serial_number(intf, hw_address, serial_number))) {
        pl_dev_err(&intf->dev, "%s:MAC and Serial Number upload failed.(%d)\n", __FUNCTION__, r);
    }
    else {
        pl_dev_info(&intf->dev, "%s:MAC and Serial Number upload succeeded.(%d)\n", __FUNCTION__, r);
    }
    chip->unit_type = get_unit_type(serial_number, intf);


    r = purelifi_mac_preinit_hw(hw, hw_address);
    if (r) {
        dev_dbg_f(&intf->dev, "couldn't initialize mac. line (%d) Error number %d\n", __LINE__, r);
        goto error;
    }

    r = ieee80211_register_hw(hw);
    if (r) {
        dev_dbg_f(&intf->dev, "couldn't register device. Error number %d\n", r);
        goto error;
    }
    pl_dev_info(&intf->dev, "%s\n", wiphy_name(hw->wiphy));

    if ((r = download_fpga(intf))) {
        dev_err(&intf->dev, "%s:FPGA download failure.(%d)\n", __FUNCTION__, r);
        goto error;
    }
    else {
        dev_info(&intf->dev, "%s::FPGA download success.\n", __FUNCTION__);
    }
    usb->tx.mac_fifo_full = 0;
    spin_lock_init(&usb->tx.lock);

    msleep(200);
    r = purelifi_usb_init_hw(usb);
    if (r < 0) {
        pl_dev_info(&intf->dev, "%s::purelifi_usb_init_hw failure. (%d)\n", __FUNCTION__, r);
        goto error;
    }
    else {
        pl_dev_info(&intf->dev, "%s::purelifi_usb_init_hw success. (%d)\n", __FUNCTION__, r);
    }

    msleep(200);
    r = purelifi_chip_switch_radio_on(chip);
    if (r < 0) {
        pl_dev_info(&intf->dev, "%s::purelifi_chip_switch_radio_on failed. (%d)\n", __FUNCTION__, r);
        goto error;
    }
    else {
        pl_dev_info(&intf->dev, "%s::purelifi_chip_switch_radio_on succeeded. (%d)\n", __FUNCTION__, r);
    }

    msleep(200);
    r = purelifi_chip_set_rate(chip, 8);
    if (r < 0) {
        pl_dev_info(&intf->dev, "%s::purelifi_chip_set_rate failed. (%d)\n", __FUNCTION__, r);
        goto error;
    }
    else {
        pl_dev_info(&intf->dev, "%s::purelifi_chip_set_rate succeeded.\n", __FUNCTION__);
    }

    msleep(200);
    r = usb_write_req(hw_address, ETH_ALEN, USB_REQ_MAC_WR);
    if (r < 0) {
        pl_dev_info(&intf->dev, "%s::USB_REQ_MAC_WR failure. (%d)\n", __FUNCTION__, r);
        goto error;
    }
    else {
        pl_dev_info(&intf->dev, "%s::USB_REQ_MAC_WR success. (%d)\n", __FUNCTION__, r);
    }
    purelifi_chip_enable_rxtx(chip);

    /*
     * Initialise the data plane Tx queue.
     */
    atomic_set(&data_queue_flag, 1);
    for (i = 0; i < MAX_STA_NUM; i++) {
        skb_queue_head_init(&usb->tx.station[i].data_list);
        usb->tx.station[i].flag = 0;
    }
    usb->tx.station[STA_BROADCAST_INDEX].flag |= STATION_CONNECTED_FLAG;
    for (i=0; i<ETH_ALEN; i++)
        usb->tx.station[STA_BROADCAST_INDEX].mac[i]=0xFF;
    atomic_set(&data_queue_flag, 0);

    init_timer(&usb->tx.tx_retry_timer);
    usb->tx.tx_retry_timer.expires = jiffies + TX_RETRY_BACKOFF_JIFF;
    usb->tx.tx_retry_timer.function = slif_data_plane_sap_timer_callb;
    usb->tx.tx_retry_timer.data = (unsigned long)usb;
    add_timer(&usb->tx.tx_retry_timer);

    init_timer(&usb->sta_queue_cleanup);
    usb->sta_queue_cleanup.expires = jiffies + STA_QUEUE_CLEANUP_JIFF;
    usb->sta_queue_cleanup.function = sta_queue_cleanup_timer_callb;
    usb->sta_queue_cleanup.data = (unsigned long)usb;
    add_timer(&usb->sta_queue_cleanup);
    usb->initialized = 1;
    return 0;
error:
    if (hw) {
        purelifi_mac_clear(purelifi_hw_mac(hw));
	ieee80211_unregister_hw(hw);
        ieee80211_free_hw(hw);
    }
    return r;
}

static void disconnect(struct usb_interface *intf)
{
    struct ieee80211_hw *hw = purelifi_intf_to_hw(intf);
    struct purelifi_mac *mac;
    struct purelifi_usb *usb;

    pl_dev_info(&intf->dev, "///////////////////////// disconnect  /////////////////////\n"); 

    /* Either something really bad happened, or we're just dealing with
     * a DEVICE_INSTALLER. */
    if (hw == NULL)
        return;

    mac = purelifi_hw_mac(hw);
    usb = &mac->chip.usb;

    del_timer(&usb->tx.tx_retry_timer);
    del_timer(&usb->sta_queue_cleanup);

    dev_dbg_f(purelifi_usb_dev(usb), "\n");

    ieee80211_unregister_hw(hw);

    purelifi_usb_disable_tx(usb);
    purelifi_usb_disable_rx(usb);

    /* 
     * If the disconnect has been caused by a removal of the
     * driver module, the reset allows reloading of the driver. If the
     * reset will not be executed here, the upload of the firmware in the
     * probe function caused by the reloading of the driver will fail.
     *
     */
    usb_reset_device(interface_to_usbdev(intf));

    purelifi_mac_clear(mac);
    ieee80211_free_hw(hw);
}

static void purelifi_usb_resume(struct purelifi_usb *usb) {
    struct purelifi_mac *mac = purelifi_usb_to_mac(usb);
    int r;

    r = purelifi_op_start(purelifi_usb_to_hw(usb));
    if (r < 0) {
        pl_dev_warn(purelifi_usb_dev(usb), "Device resume failed "
             "with error code %d. Retrying...\n", r);
        if (usb->was_running)
            set_bit(PURELIFI_DEVICE_RUNNING, &mac->flags);
        usb_queue_reset_device(usb->intf);
        return;
    }

    if (mac->type != NL80211_IFTYPE_UNSPECIFIED) {
        r = purelifi_restore_settings(mac);
        if (r < 0) {
            dev_dbg(purelifi_usb_dev(usb),
                "failed to restore settings, %d\n", r);
            return;
        }
    }
}

static void purelifi_usb_stop(struct purelifi_usb *usb) {
    pl_dev_info(&usb->intf->dev, "%s\n", __FUNCTION__);

    purelifi_op_stop(purelifi_usb_to_hw(usb));

    purelifi_usb_disable_tx(usb);
    purelifi_usb_disable_rx(usb);

    usb->initialized = 0;
}

static int pre_reset(struct usb_interface *intf)
{
    struct ieee80211_hw *hw = usb_get_intfdata(intf);
    struct purelifi_mac *mac;
    struct purelifi_usb *usb;

    pl_dev_info(&intf->dev, "%s\n", __FUNCTION__);
    if (!hw || intf->condition != USB_INTERFACE_BOUND)
        return 0;

    mac = purelifi_hw_mac(hw);
    usb = &mac->chip.usb;

    usb->was_running = test_bit(PURELIFI_DEVICE_RUNNING, &mac->flags);

    purelifi_usb_stop(usb);

    mutex_lock(&mac->chip.mutex);
    return 0;
}

static int post_reset(struct usb_interface *intf)
{
    struct ieee80211_hw *hw = usb_get_intfdata(intf);
    struct purelifi_mac *mac;
    struct purelifi_usb *usb;

    pl_dev_info(&intf->dev, "%s\n", __FUNCTION__);
    if (!hw || intf->condition != USB_INTERFACE_BOUND)
        return 0;

    mac = purelifi_hw_mac(hw);
    usb = &mac->chip.usb;

    mutex_unlock(&mac->chip.mutex);

    if (usb->was_running)
        purelifi_usb_resume(usb);
    return 0;
}

static struct usb_driver driver = {
    .name        = KBUILD_MODNAME,
    .id_table    = usb_ids,
    .probe        = probe,
    .disconnect    = disconnect,
    .pre_reset    = pre_reset,
    .post_reset    = post_reset,
    .disable_hub_initiated_lpm = 1,
};

struct workqueue_struct *purelifi_workqueue;

static int modulation_open(struct inode *inode_p, struct file * file_p) {
    printk(KERN_INFO "%s\n", __FUNCTION__);
    return 0;
}

static ssize_t modulation_read(struct file * file_p, char __user *user_p, size_t offset, loff_t *off_p) {
    printk(KERN_INFO "%s\n", __FUNCTION__);
    return 0;
}

ssize_t modulation_write(struct file *file_p, const char __user *buffer, size_t count, loff_t *actual_data_written) {
    u32 rate;
    sscanf(buffer, "%d", &rate);

    //if (count > 2 || rate > 9 ) {
    if (0) {
        printk(KERN_WARNING "%s::Modulation options are:\n", __FUNCTION__);
        printk(KERN_WARNING "%s::   BPSK  1/2 ................. (0):\n", __FUNCTION__);
        printk(KERN_WARNING "%s::   BPSK  3/4 ................. (1):\n", __FUNCTION__);
        printk(KERN_WARNING "%s::   QPSK  1/2 ................. (2):\n", __FUNCTION__);
        printk(KERN_WARNING "%s::   QPSK  3/4 ................. (3):\n", __FUNCTION__);
        printk(KERN_WARNING "%s::   QAM16 1/2 ................. (4):\n", __FUNCTION__);
        printk(KERN_WARNING "%s::   QAM16 3/4 ................. (5):\n", __FUNCTION__);
        printk(KERN_WARNING "%s::   QAM64 1/2 ................. (6):\n", __FUNCTION__);
        printk(KERN_WARNING "%s::   QAM64 3/4 ................. (7):\n", __FUNCTION__);
        printk(KERN_WARNING "%s::   Auto ...................... (8):\n", __FUNCTION__);
        printk(KERN_WARNING "%s::   Auto with logging stats ... (9):\n", __FUNCTION__);
    }
    else {
        purelifi_chip_set_rate(NULL, rate);
    }
    return count;
}

static struct file_operations modulation_fops = {
    .owner = THIS_MODULE,
    .open  = modulation_open,
    .read  = modulation_read,
    .write = modulation_write
};

static int create_proc_entry_e(
    struct proc_dir_entry * entry, 
    size_t size, 
    mode_t mode, 
    struct file_operations * fops, 
    const char *entry_name, 
    struct proc_dir_entry *parent
) {
    if (!(entry = proc_create( entry_name, mode, parent, fops))) {
        return -ENOMEM;
    }
    return 0;
}

static int __init usb_init(void)
{
    int r;
    if ( !( proc_folder = proc_mkdir(PROC_FOLDER, NULL) ) ) {
        printk(KERN_ERR "Error creating proc folder.\n");
        r = -ENOMEM;
        goto error;
    }

    if ((r = create_proc_entry_e(
        modulation_proc_entry,
        sizeof(u32),
        0666,
        &modulation_fops,
        MODULATION_FILENAME,
        proc_folder))
    ) {
        goto remove_proc_dir;
    }
    purelifi_workqueue = create_singlethread_workqueue(driver.name);
    if (purelifi_workqueue == NULL) {
        printk(KERN_ERR "%s couldn't create workqueue\n", driver.name);
        r = -ENOMEM;
        goto remove_modulation_proc_file;
    }

    r = usb_register(&driver);
    if (r) {
        destroy_workqueue(purelifi_workqueue);
        printk(KERN_ERR "%s usb_register() failed. Error number %d\n",
               driver.name, r);
        return r;
    }

#ifdef OPENWRT
    if (
        GPIO_REQUEST(EXPANSION_PORT0_1, "EXPANSION_PORT0_1") ||
        GPIO_DIRECTION_OUTPUT(EXPANSION_PORT0_1, GPIO_LOW)
    ) {
        printk(KERN_ERR "%s::Failed to initialize EXPANSION_PORT0_1\n", __FUNCTION__);
    }
    else {
        printk(KERN_ERR "%s::Succeeded to initialize EXPANSION_PORT0_1\n", __FUNCTION__);
    }
    if (
        GPIO_REQUEST(GPIO_GREEN, "GPIO_GREEN") ||
        GPIO_DIRECTION_OUTPUT(GPIO_GREEN, GPIO_LOW)
    ) {
        printk(KERN_ERR "%s::Failed to initialize GPIO_GREEN\n", __FUNCTION__);
    }
    else {
        printk(KERN_ERR "%s::Succeeded to initialize GPIO_GREEN\n", __FUNCTION__);
    }

    if (
        GPIO_REQUEST(GPIO_RED, "GPIO_RED") ||
        GPIO_DIRECTION_OUTPUT(GPIO_RED, GPIO_LOW)
    ) {
        printk(KERN_ERR "%s::Failed to initialize GPIO_RED\n", __FUNCTION__);
    }
    else {
        printk(KERN_ERR "%s::Succeeded to initialize GPIO_RED\n", __FUNCTION__);
    }

    if (
        GPIO_REQUEST(EXPANSION_PORT1_1, "EXPANSION_PORT1_1") ||
        GPIO_DIRECTION_OUTPUT(EXPANSION_PORT1_1, GPIO_LOW)
    ) {
        printk(KERN_ERR "%s::Failed to initialize EXPANSION_PORT1_1\n", __FUNCTION__);
    }
    else {
        printk(KERN_ERR "%s::Succeeded to initialize EXPANSION_PORT1_1\n", __FUNCTION__);
    }

    if (
        GPIO_REQUEST(EXPANSION_PORT0_6, "EXPANSION_PORT0_6") ||
        GPIO_DIRECTION_OUTPUT(EXPANSION_PORT0_6, GPIO_LOW)
    ) {
        printk(KERN_ERR "%s::Failed to initialize EXPANSION_PORT0_6\n", __FUNCTION__);
    }
    else {
        printk(KERN_ERR "%s::Succeeded to initialize EXPANSION_PORT0_6\n", __FUNCTION__);
    }

    if (
        GPIO_REQUEST(EXPANSION_PORT1_6, "EXPANSION_PORT1_6") ||
        GPIO_DIRECTION_OUTPUT(EXPANSION_PORT1_6, GPIO_LOW)
    ) {
        printk(KERN_ERR "%s::Failed to initialize EXPANSION_PORT1_6\n", __FUNCTION__);
    }
    else {
        printk(KERN_ERR "%s::Succeeded to initialize EXPANSION_PORT1_6\n", __FUNCTION__);
    }
#endif
    pr_debug("%s:Initialized\n", driver.name);
    return 0;

remove_modulation_proc_file:
    remove_proc_entry(MODULATION_FILENAME, proc_folder);
remove_proc_dir:
    remove_proc_entry(PROC_FOLDER, NULL);
error:
    return r;
}

static void __exit usb_exit(void)
{
    pr_debug("%s usb_exit()\n", driver.name);
    remove_proc_entry(MODULATION_FILENAME, proc_folder);
    remove_proc_entry(PROC_FOLDER, NULL);
    gpio_free(EXPANSION_PORT0_1);
    gpio_free(EXPANSION_PORT0_6);
    gpio_free(EXPANSION_PORT1_1);
    gpio_free(EXPANSION_PORT1_6);
    usb_deregister(&driver);
    destroy_workqueue(purelifi_workqueue);
}

module_init(usb_init);
module_exit(usb_exit);
