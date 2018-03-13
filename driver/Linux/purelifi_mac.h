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

#ifndef _PURELIFI_MAC_H
#define _PURELIFI_MAC_H

#include <linux/kernel.h>
#include <net/mac80211.h>

#include "purelifi_chip.h"

struct purelifi_ctrlset {
    usb_req_enum_t id;
    u32            len;
	u8             modulation;
	u8             control;
	u8             service;
	u8             pad;
	__le16         packet_length;
	__le16         current_length;
	__le16          next_frame_length;
	__le16         tx_length;
    u32            payload_len_nw;
} __packed;

#define PURELIFI_CCK                  0x00
#define PURELIFI_OFDM                 0x10
#define PURELIFI_CCK_PREA_SHORT       0x20
#define PURELIFI_OFDM_PLCP_RATE_6M	0xb
#define PURELIFI_OFDM_PLCP_RATE_9M	0xf
#define PURELIFI_OFDM_PLCP_RATE_12M	0xa
#define PURELIFI_OFDM_PLCP_RATE_18M	0xe
#define PURELIFI_OFDM_PLCP_RATE_24M	0x9
#define PURELIFI_OFDM_PLCP_RATE_36M	0xd
#define PURELIFI_OFDM_PLCP_RATE_48M	0x8
#define PURELIFI_OFDM_PLCP_RATE_54M	0xc
#define PURELIFI_CCK_RATE_1M          (PURELIFI_CCK|0x00)
#define PURELIFI_CCK_RATE_2M          (PURELIFI_CCK|0x01)
#define PURELIFI_CCK_RATE_5_5M        (PURELIFI_CCK|0x02)
#define PURELIFI_CCK_RATE_11M         (PURELIFI_CCK|0x03)
#define PURELIFI_OFDM_RATE_6M         (PURELIFI_OFDM|PURELIFI_OFDM_PLCP_RATE_6M)
#define PURELIFI_OFDM_RATE_9M         (PURELIFI_OFDM|PURELIFI_OFDM_PLCP_RATE_9M)
#define PURELIFI_OFDM_RATE_12M        (PURELIFI_OFDM|PURELIFI_OFDM_PLCP_RATE_12M)
#define PURELIFI_OFDM_RATE_18M        (PURELIFI_OFDM|PURELIFI_OFDM_PLCP_RATE_18M)
#define PURELIFI_OFDM_RATE_24M        (PURELIFI_OFDM|PURELIFI_OFDM_PLCP_RATE_24M)
#define PURELIFI_OFDM_RATE_36M        (PURELIFI_OFDM|PURELIFI_OFDM_PLCP_RATE_36M)
#define PURELIFI_OFDM_RATE_48M        (PURELIFI_OFDM|PURELIFI_OFDM_PLCP_RATE_48M)
#define PURELIFI_OFDM_RATE_54M        (PURELIFI_OFDM|PURELIFI_OFDM_PLCP_RATE_54M)
#define PURELIFI_RX_ERROR			    0x80
#define PURELIFI_RX_CRC32_ERROR		0x10

struct tx_status {
	u8 type;
	u8 id;
	u8 rate;
	u8 pad;
	u8 mac[ETH_ALEN];
	u8 retry;
	u8 failure;
} __packed;

enum mac_flags {
	MAC_FIXED_CHANNEL = 0x01,
};

struct beacon {
	struct delayed_work watchdog_work;
	struct sk_buff *cur_beacon;
	unsigned long last_update;
	u16 interval;
	u8 period;
};

enum purelifi_device_flags {
	PURELIFI_DEVICE_RUNNING,
};

#define PURELIFI_MAC_STATS_BUFFER_SIZE 16

#define PURELIFI_MAC_MAX_ACK_WAITERS 50

struct purelifi_mac {
	struct purelifi_chip chip;
	spinlock_t lock;
	spinlock_t intr_lock;
	struct ieee80211_hw *hw;
	struct ieee80211_vif *vif;
	struct beacon beacon;
	struct work_struct set_rts_cts_work;
	struct work_struct process_intr;
	struct purelifi_mc_hash multicast_hash;
	u8 intr_buffer[USB_MAX_EP_INT_BUFFER];
	u8 regdomain;
	u8 default_regdomain;
	u8 channel;
	int type;
	int associated;
	unsigned long flags;
	struct sk_buff_head ack_wait_queue;
	struct ieee80211_channel channels[14];
	struct ieee80211_rate rates[12];
	struct ieee80211_supported_band band;

	/* whether to pass frames with CRC errors to stack */
	unsigned int pass_failed_fcs:1;

	/* whether to pass control frames to stack */
	unsigned int pass_ctrl:1;

	/* whether we have received a 802.11 ACK that is pending */
	unsigned int ack_pending:1;

	/* signal strength of the last 802.11 ACK received */
	int ack_signal;

    unsigned char hw_address[ETH_ALEN];
    char serial_number[256];
    u64 crc_errors;
    u64 rssi;
};

#define MODULATION_RATE_BPSK_1_2  0
#define MODULATION_RATE_BPSK_3_4  (MODULATION_RATE_BPSK_1_2  + 1)
#define MODULATION_RATE_QPSK_1_2  (MODULATION_RATE_BPSK_3_4  + 1)
#define MODULATION_RATE_QPSK_3_4  (MODULATION_RATE_QPSK_1_2  + 1)
#define MODULATION_RATE_QAM16_1_2 (MODULATION_RATE_QPSK_3_4  + 1)
#define MODULATION_RATE_QAM16_3_4 (MODULATION_RATE_QAM16_1_2 + 1)
#define MODULATION_RATE_QAM64_1_2 (MODULATION_RATE_QAM16_3_4 + 1)
#define MODULATION_RATE_QAM64_3_4 (MODULATION_RATE_QAM64_1_2 + 1)
#define MODULATION_RATE_AUTO      (MODULATION_RATE_QAM64_3_4 + 1)
#define MODULATION_RATE_NUM       (MODULATION_RATE_AUTO      + 1)

static inline struct purelifi_mac *purelifi_hw_mac(struct ieee80211_hw *hw)
{
	return hw->priv;
}

static inline struct purelifi_mac *purelifi_chip_to_mac(struct purelifi_chip *chip)
{
	return container_of(chip, struct purelifi_mac, chip);
}

static inline struct purelifi_mac *purelifi_usb_to_mac(struct purelifi_usb *usb)
{
	return purelifi_chip_to_mac(purelifi_usb_to_chip(usb));
}

static inline u8 *purelifi_mac_get_perm_addr(struct purelifi_mac *mac)
{
	return mac->hw->wiphy->perm_addr;
}

#define purelifi_mac_dev(mac) (purelifi_chip_dev(&(mac)->chip))

struct ieee80211_hw *purelifi_mac_alloc_hw(struct usb_interface *intf);
void purelifi_mac_clear(struct purelifi_mac *mac);

int purelifi_mac_preinit_hw(struct ieee80211_hw *hw, unsigned char *hw_address);
int purelifi_mac_init_hw(struct ieee80211_hw *hw);

int purelifi_mac_rx(struct ieee80211_hw *hw, const u8 *buffer, unsigned int length);
void purelifi_mac_tx_failed(struct urb *urb);
void purelifi_mac_tx_to_dev(struct sk_buff *skb, int error);
int purelifi_op_start(struct ieee80211_hw *hw);
void purelifi_op_stop(struct ieee80211_hw *hw);
int purelifi_restore_settings(struct purelifi_mac *mac);
void block_queue(struct purelifi_usb *usb, const u8 *mac, bool block);
#ifdef DEBUG
void purelifi_dump_rx_status(const struct rx_status *status);
#else
#define purelifi_dump_rx_status(status)
#endif

#endif
