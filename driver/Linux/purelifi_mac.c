/* 
 * pureLiFi-X USB-WLAN driver for Linux.
 * Driver has been based on the zd1211rw driver.
 *
 * Copyright (C) 2015-2016 pureLiFi <info@purelifi.com>
 * Copyright (C) 2007-2008 Luis R. Rodriguez <mcgrof@winlab.rutgers.edu>
 * Copyright (C) 2006-2007 Daniel Drake <dsd@gentoo.org>
 * Copyright (C) 2006-2007 Michael Wu <flamingice@sourmilk.net>
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/slab.h>
#include <linux/usb.h>
#include <linux/gpio.h>
#include <linux/jiffies.h>
#include <net/ieee80211_radiotap.h>

#include "purelifi_def.h"
#include "purelifi_chip.h"
#include "purelifi_mac.h"
#include "purelifi_log.h"

#ifndef UBUNTU
   #ifndef CENTOS
      #define OPENWRT
   #endif
#endif

#ifndef IEEE80211_BAND_2GHZ
   #define IEEE80211_BAND_2GHZ NL80211_BAND_2GHZ
#endif

static const struct ieee80211_rate purelifi_rates[] = {
	{ .bitrate = 10,
	  .hw_value = PURELIFI_CCK_RATE_1M, },
	{ .bitrate = 20,
	  .hw_value = PURELIFI_CCK_RATE_2M,
	  .hw_value_short = PURELIFI_CCK_RATE_2M | PURELIFI_CCK_PREA_SHORT,
	  .flags = IEEE80211_RATE_SHORT_PREAMBLE },
	{ .bitrate = 55,
	  .hw_value = PURELIFI_CCK_RATE_5_5M,
	  .hw_value_short = PURELIFI_CCK_RATE_5_5M | PURELIFI_CCK_PREA_SHORT,
	  .flags = IEEE80211_RATE_SHORT_PREAMBLE },
	{ .bitrate = 110,
	  .hw_value = PURELIFI_CCK_RATE_11M,
	  .hw_value_short = PURELIFI_CCK_RATE_11M | PURELIFI_CCK_PREA_SHORT,
	  .flags = IEEE80211_RATE_SHORT_PREAMBLE },
	{ .bitrate = 60,
	  .hw_value = PURELIFI_OFDM_RATE_6M,
	  .flags = 0 },
	{ .bitrate = 90,
	  .hw_value = PURELIFI_OFDM_RATE_9M,
	  .flags = 0 },
	{ .bitrate = 120,
	  .hw_value = PURELIFI_OFDM_RATE_12M,
	  .flags = 0 },
	{ .bitrate = 180,
	  .hw_value = PURELIFI_OFDM_RATE_18M,
	  .flags = 0 },
	{ .bitrate = 240,
	  .hw_value = PURELIFI_OFDM_RATE_24M,
	  .flags = 0 },
	{ .bitrate = 360,
	  .hw_value = PURELIFI_OFDM_RATE_36M,
	  .flags = 0 },
	{ .bitrate = 480,
	  .hw_value = PURELIFI_OFDM_RATE_48M,
	  .flags = 0 },
	{ .bitrate = 540,
	  .hw_value = PURELIFI_OFDM_RATE_54M,
	  .flags = 0 },
};
static const struct ieee80211_channel purelifi_channels[] = {
	{ .center_freq = 2412, .hw_value = 1 },
	{ .center_freq = 2417, .hw_value = 2 },
	{ .center_freq = 2422, .hw_value = 3 },
	{ .center_freq = 2427, .hw_value = 4 },
	{ .center_freq = 2432, .hw_value = 5 },
	{ .center_freq = 2437, .hw_value = 6 },
	{ .center_freq = 2442, .hw_value = 7 },
	{ .center_freq = 2447, .hw_value = 8 },
	{ .center_freq = 2452, .hw_value = 9 },
	{ .center_freq = 2457, .hw_value = 10 },
	{ .center_freq = 2462, .hw_value = 11 },
	{ .center_freq = 2467, .hw_value = 12 },
	{ .center_freq = 2472, .hw_value = 13 },
	{ .center_freq = 2484, .hw_value = 14 },
};

static int purelifi_mac_config_beacon(struct ieee80211_hw *hw,
				struct sk_buff *beacon, bool in_intr);

int purelifi_mac_preinit_hw(struct ieee80211_hw *hw, unsigned char *hw_adress)
{
	SET_IEEE80211_PERM_ADDR(hw, hw_adress);
	return 0;
}

void block_queue(struct purelifi_usb *usb, const u8 *mac, bool block) {
    #ifdef TYPE_AP
        struct ieee80211_vif *vif = purelifi_usb_to_mac(usb)->vif;
        struct ieee80211_sta *sta = ieee80211_find_sta(vif, mac);
        if (!sta) {
//            pl_dev_warn(
//                &usb->intf->dev, 
//                KERN_WARNING
//                "%s::MAC[%x:%x:%x:%x:%x:%x] not found\n", 
//                __FUNCTION__, 
//                mac[0], mac[1], mac[2],
//                mac[3], mac[4], mac[5]
//            );
        }
        else {
            ieee80211_sta_block_awake(purelifi_usb_to_hw(usb), sta, block);
        }
    #else
        if (block) {
            ieee80211_stop_queues(purelifi_usb_to_hw(usb));
        }
        else {
            ieee80211_wake_queues(purelifi_usb_to_hw(usb));
        }
    #endif
}

int purelifi_mac_init_hw(struct ieee80211_hw *hw)
{
	int r;
	struct purelifi_mac *mac = purelifi_hw_mac(hw);
	struct purelifi_chip *chip = &mac->chip;

	r = purelifi_chip_init_hw(chip);
	if (r) {
		pl_dev_warn(purelifi_mac_dev(mac), "%s::purelifi_chip_init_hw failed. (%d)\n", __FUNCTION__, r);
		goto out;
	}
	PURELIFI_ASSERT(!irqs_disabled());

	r = regulatory_hint(hw->wiphy, "CA");
out:
	return r;
}

void purelifi_mac_clear(struct purelifi_mac *mac)
{
	purelifi_chip_clear(&mac->chip);
	PURELIFI_ASSERT(!spin_is_locked(&mac->lock));
	PURELIFI_MEMCLEAR(mac, sizeof(struct purelifi_mac));
}

int purelifi_op_start(struct ieee80211_hw *hw) {
	regulatory_hint(hw->wiphy, "EU");
	purelifi_hw_mac(hw)->chip.usb.initialized = 1;
	return 0;
}

void purelifi_op_stop(struct ieee80211_hw *hw) {
	struct purelifi_mac *mac = purelifi_hw_mac(hw);
	struct purelifi_chip *chip = &mac->chip;
	struct sk_buff *skb;
	struct sk_buff_head *ack_wait_queue = &mac->ack_wait_queue;

	pl_dev_info(purelifi_mac_dev(mac), "%s", __FUNCTION__);

	clear_bit(PURELIFI_DEVICE_RUNNING, &mac->flags);

	/* The order here deliberately is a little different from the open()
	 * method, since we need to make sure there is no opportunity for RX
	 * frames to be processed by mac80211 after we have stopped it.
	 */

	return;  //don't allow stop for debugging rx_urb_complete failure
	purelifi_chip_switch_radio_off(chip);

	while ((skb = skb_dequeue(ack_wait_queue)))
		dev_kfree_skb_any(skb);
	return;  //don't allow stop for debugging rx_urb_complete failure
}

int purelifi_restore_settings(struct purelifi_mac *mac) {
	struct sk_buff *beacon;
	struct purelifi_mc_hash multicast_hash;
	int beacon_interval, beacon_period;

	dev_dbg_f(purelifi_mac_dev(mac), "\n");

	spin_lock_irq(&mac->lock);
	multicast_hash = mac->multicast_hash;
	beacon_interval = mac->beacon.interval;
	beacon_period = mac->beacon.period;
	spin_unlock_irq(&mac->lock);

	if (mac->type == NL80211_IFTYPE_MESH_POINT ||
		mac->type == NL80211_IFTYPE_ADHOC ||
		mac->type == NL80211_IFTYPE_AP) {
		if (mac->vif != NULL) {
			beacon = ieee80211_beacon_get(mac->hw, mac->vif);
			if (beacon)
			{
				purelifi_mac_config_beacon(mac->hw, beacon, false);
				kfree_skb(beacon);//Returned skb is used only once and low-level driver is responsible for freeing it.
			} 
		}

		purelifi_set_beacon_interval(&mac->chip, beacon_interval, beacon_period, mac->type);

		spin_lock_irq(&mac->lock);
		mac->beacon.last_update = jiffies;
		spin_unlock_irq(&mac->lock);
	}

	return 0;
}

/**
 * purelifi_mac_tx_status - reports tx status of a packet if required
 * @hw - a &struct ieee80211_hw pointer
 * @skb - a sk-buffer
 * @flags: extra flags to set in the TX status info
 * @ackssi: ACK signal strength
 * @success - True for successful transmission of the frame
 *
 * This information calls ieee80211_tx_status_irqsafe() if required by the
 * control information. It copies the control information into the status
 * information.
 *
 * If no status information has been requested, the skb is freed.
 */
static void purelifi_mac_tx_status(struct ieee80211_hw *hw, struct sk_buff *skb,
		      int ackssi, struct tx_status *tx_status)
{
	struct ieee80211_tx_info *info = IEEE80211_SKB_CB(skb);
	int success = 1, retry = 1;

	ieee80211_tx_info_clear_status(info);

	if (tx_status) {
		success = !tx_status->failure;
		retry = tx_status->retry + success;
	}

	if (success) {
		info->flags |= IEEE80211_TX_STAT_ACK;
	} else {
		info->flags &= ~IEEE80211_TX_STAT_ACK;
	}

	info->status.ack_signal = 50;
	ieee80211_tx_status_irqsafe(hw, skb);
}

/**
 * purelifi_mac_tx_to_dev - callback for USB layer
 * @skb: a &sk_buff pointer
 * @error: error value, 0 if transmission successful
 *
 * Informs the MAC layer that the frame has successfully transferred to the
 * device. If an ACK is required and the transfer to the device has been
 * successful, the packets are put on the @ack_wait_queue with
 * the control set removed.
 */
void purelifi_mac_tx_to_dev(struct sk_buff *skb, int error)
{
	struct ieee80211_tx_info *info = IEEE80211_SKB_CB(skb);
	struct ieee80211_hw *hw = info->rate_driver_data[0];
	struct purelifi_mac *mac = purelifi_hw_mac(hw);

	ieee80211_tx_info_clear_status(info);
	skb_pull(skb, sizeof(struct purelifi_ctrlset));
        
	if (unlikely(error ||
	    (info->flags & IEEE80211_TX_CTL_NO_ACK))) {
		/*
		 * FIXME : do we need to fill in anything ?
		 */
		ieee80211_tx_status_irqsafe(hw, skb);
	} else {
                // ieee80211_tx_status_irqsafe(hw, skb);
		struct sk_buff_head *q = &mac->ack_wait_queue;

		skb_queue_tail(q, skb);
		while (skb_queue_len(q)/* > PURELIFI_MAC_MAX_ACK_WAITERS*/) {
			purelifi_mac_tx_status(hw, skb_dequeue(q),
					 mac->ack_pending ? mac->ack_signal : 0,
					 NULL);
			mac->ack_pending = 0;
		}
	}
}


static int purelifi_mac_config_beacon(struct ieee80211_hw *hw, struct sk_buff *beacon, bool in_intr)
{
    return usb_write_req((const u8*)beacon->data, beacon->len, USB_REQ_BEACON_WR);
}

static int fill_ctrlset(struct purelifi_mac *mac, struct sk_buff *skb)
{
    unsigned int frag_len = skb->len;
    unsigned int tmp;
    struct purelifi_ctrlset *cs; 
    if(skb_headroom (skb) >= sizeof(struct purelifi_ctrlset))
	{
        cs = (struct purelifi_ctrlset *) skb_push(skb, sizeof(struct purelifi_ctrlset));
	}
    else
	{
        pl_dev_info(purelifi_mac_dev(mac), "fill_ctrlset: !!!!!!!!!!!!!!!!  not enough headroom(1)\n");
	return 1;
	}

    cs->id = USB_REQ_DATA_TX;
    cs->payload_len_nw = frag_len;
    cs->len = cs->payload_len_nw + sizeof(struct purelifi_ctrlset) - sizeof(cs->id) - sizeof(cs->len);

    //data packet lengths must be multiple of four bytes and must not be a multiple of 512 
	//bytes. First, it is attempted to append the data packet in the tailroom of the skb. In rare
	//ocasions, the tailroom is too small. In this case, the content of the packet is shifted into
	//the headroom of the skb by memcpy. Headroom is allocated at startup (below in this file). Therefore,
	//there will be always enough headroom. The call skb_headroom is an additional safety which might be 
	//dropped. 

    //check if 32 bit aligned
    tmp = skb->len & 3;
    if(tmp)
	{
	if(skb_tailroom (skb) >= (4 - tmp))
	    {
            skb_put(skb, 4 - tmp);
	    }
	else 
	    {
            pl_dev_info(purelifi_mac_dev(mac), "fill_ctrlset: !!!!!!!!!!!!!!!!  not enough tailroom(2), trying to shift content\n");
            if(skb_headroom (skb) >= 4 - tmp)
        	{
                u16 i;
                u8 len;
                u8 *src_pt;
                u8 *dest_pt;
		        pl_dev_info(purelifi_mac_dev(mac), "fill_ctrlset: before: len=%d\n",skb->len );
                for(i=0; i < skb->len; i++)
                    pl_dev_info(purelifi_mac_dev(mac), "%x ", skb->data[i]);
                pl_dev_info(purelifi_mac_dev(mac), "//////////////////////////////////////////\n");

                len = skb->len;
                src_pt = skb->data;
                dest_pt = skb_push(skb, 4 - tmp );
                memcpy(dest_pt, src_pt, len);

		        pl_dev_info(purelifi_mac_dev(mac), "fill_ctrlset: after: len=%d\n",skb->len );
                for(i=0; i < skb->len; i++)
                    pl_dev_info(purelifi_mac_dev(mac), "%x ", skb->data[i]);
                pl_dev_info(purelifi_mac_dev(mac), "//////////////////////////////////////////\n");
        	}
            else
        	{
                //should never happen b/c sufficient headroom was reserved
                pl_dev_err(purelifi_mac_dev(mac), "fill_ctrlset: !!!!!!!!!!!!!!!!  not enough tailroom(2), shifting content failed\n");
        	return 1;
        	}            
	    }
        cs->len +=  4 - tmp;
	}

    //check if not multiple of 512
    tmp = skb->len & 0x1ff;
    if(!tmp)
	{
	if(skb_tailroom (skb) >= 4)
	    {
            skb_put(skb, 4);
	    }
        else 
	    {
            pl_dev_info(
                purelifi_mac_dev(mac), 
                "fill_ctrlset: !!!!!!!!!!!!!!!!  not enough tailroom(3), trying to shift content\n"
            );
            if(skb_headroom (skb) >= 4)
        	{
                u8 len = skb->len;
                u8 *src_pt = skb->data;
                u8 *dest_pt = skb_push(skb, 4);
                memcpy(dest_pt, src_pt, len);
        	}
            else
        	{
                //should never happen b/c sufficient headroom was reserved
                pl_dev_info(
                    purelifi_mac_dev(mac), 
                    "fill_ctrlset: !!!!!!!!!!!!!!!!  not enough tailroom(3), shifting content failed\n"
                );
        	return 1;
        	}    
	    }

        cs->len +=  4;
	}


    cs->id = TO_NETWORK(cs->id);
    cs->len = TO_NETWORK(cs->len);
    cs->payload_len_nw = TO_NETWORK(cs->payload_len_nw);

    return 0;
}

/**
 * purelifi_op_tx - transmits a network frame to the device
 *
 * @dev: mac80211 hardware device
 * @skb: socket buffer
 * @control: the control structure
 *
 * This function transmit an IEEE 802.11 network frame to the device. The
 * control block of the skbuff will be initialized. If necessary the incoming
 * mac80211 queues will be stopped.
 */
static void purelifi_op_tx(struct ieee80211_hw *hw, struct ieee80211_tx_control *control, struct sk_buff *skb) {
    struct purelifi_mac *mac = purelifi_hw_mac(hw);
    struct purelifi_usb *usb = &mac->chip.usb;
    struct ieee80211_tx_info *info = IEEE80211_SKB_CB(skb);
    unsigned long flags;
    int r;

    r = fill_ctrlset(mac, skb);
    if (r)
        goto fail;
    info->rate_driver_data[0] = hw;

    if (skb->data[24] != 0x08 && skb->len == 116) {
        pl_dev_info(purelifi_mac_dev(mac), "%s::ping\n", __FUNCTION__);
    }

    if (skb->data[24] != 0x08) {
        spin_lock_irqsave(&usb->tx.lock, flags);
        r = usb_write_req_async(&mac->chip.usb, skb->data, skb->len, USB_REQ_DATA_TX, tx_urb_complete, skb);
        spin_unlock_irqrestore(&usb->tx.lock, flags); 
        if (r)
        	goto fail;
    }
    else {
        u8 dst_mac[ETH_ALEN];
        u8 station_index;
        bool found = false;
        memcpy(dst_mac, &skb->data[28], ETH_ALEN);
        for (station_index = 0; station_index < MAX_STA_NUM; station_index++) {
            if (usb->tx.station[station_index].flag & STATION_CONNECTED_FLAG) {
                if (!memcmp(usb->tx.station[station_index].mac, dst_mac, ETH_ALEN)) {
                    found = true;
                    break;
                }
            }
        }

        // Default to broadcast address for unknown MACs.
       
        if(!found){
            station_index = STA_BROADCAST_INDEX;
        }

        // Stop OS from sending any packets, if the queue is half full.
        if (skb_queue_len(&usb->tx.station[station_index].data_list) > 60/*128*/) {
            block_queue(usb, usb->tx.station[station_index].mac, true);
        }
        else {
        }

        // Schedule packet for transmission if queue is not full
        if (skb_queue_len(&usb->tx.station[station_index].data_list) < 256) {
            skb_queue_tail(&usb->tx.station[station_index].data_list, skb);
        }
        else {
            dev_kfree_skb(skb);
            // pl_dev_info(&usb->intf->dev,  "%s::tx_data_list full\n", __FUNCTION__);
        }
    }
	return;

fail:
	dev_kfree_skb(skb);
}

/**
 * filter_ack - filters incoming packets for acknowledgements
 * @dev: the mac80211 device
 * @rx_hdr: received header
 * @stats: the status for the received packet
 *
 * This functions looks for ACK packets and tries to match them with the
 * frames in the tx queue. If a match is found the frame will be dequeued and
 * the upper layers is informed about the successful transmission. If
 * mac80211 queues have been stopped and the number of frames still to be
 * transmitted is low the queues will be opened again.
 *
 * Returns 1 if the frame was an ACK, 0 if it was ignored.
 */
static int filter_ack(struct ieee80211_hw *hw, struct ieee80211_hdr *rx_hdr,
		      struct ieee80211_rx_status *stats)
{
	struct purelifi_mac *mac = purelifi_hw_mac(hw);
	struct sk_buff *skb;
	struct sk_buff_head *q;
	unsigned long flags;
	int found = 0;
	int i, position = 0;

	if (!ieee80211_is_ack(rx_hdr->frame_control))
		return 0;
        pl_dev_info(purelifi_mac_dev(mac), "%s::ACK Received", __FUNCTION__);
        return 0;

	q = &mac->ack_wait_queue;
	spin_lock_irqsave(&q->lock, flags);
	skb_queue_walk(q, skb) {
		struct ieee80211_hdr *tx_hdr;

		position ++;

		if (mac->ack_pending && skb_queue_is_first(q, skb))
		    continue;

		tx_hdr = (struct ieee80211_hdr *)skb->data;
		if (likely(ether_addr_equal(tx_hdr->addr2, rx_hdr->addr1)))
		{
			found = 1;
			break;
		}
	}

	if (found) {
		for (i=1; i<position; i++) {
			skb = __skb_dequeue(q);
			purelifi_mac_tx_status(hw, skb,
					 mac->ack_pending ? mac->ack_signal : 0,
					 NULL);
			mac->ack_pending = 0;
		}

		mac->ack_pending = 1;
		mac->ack_signal = stats->signal;

		/* Prevent pending tx-packet on AP-mode */
		if (mac->type == NL80211_IFTYPE_AP) {
			skb = __skb_dequeue(q);
			purelifi_mac_tx_status(hw, skb, mac->ack_signal, NULL);
			mac->ack_pending = 0;
		}
	}

	spin_unlock_irqrestore(&q->lock, flags);
	return 1;
}

int purelifi_mac_rx(struct ieee80211_hw *hw, const u8 *buffer, unsigned int length)
{
    struct purelifi_mac *mac = purelifi_hw_mac(hw);
    struct ieee80211_rx_status stats;
    const struct rx_status *status;
    struct sk_buff *skb;
    int bad_frame = 0;
    __le16 fc;
    int need_padding;
    unsigned int payload_length;
    static unsigned short int min_exp_seq_nmb = 0;
    u32 crc_error_cnt_low, crc_error_cnt_high;

    // Packet blockade during disabled interface.
    if (mac->vif == NULL) {
        return 0;
    }

    memset(&stats, 0, sizeof(stats));
    status = (struct rx_status *) buffer;

    stats.flag     = 0;
    stats.freq     = 2412;
    stats.band     = IEEE80211_BAND_2GHZ;
    mac->rssi      = 100-(2*ntohs(status->rssi));
    if (mac->rssi < 0) {
        mac->rssi = 0;
    }

    stats.signal   = mac->rssi;
    if (status->rate_idx > 7) {
        stats.rate_idx = 0;
    }
    else {
        stats.rate_idx = status->rate_idx;
    }

    crc_error_cnt_low = status-> crc_error_count;
    crc_error_cnt_high = status-> crc_error_count >> 32;   
    mac->crc_errors = ((u64)ntohl(crc_error_cnt_low) << 32) | ntohl(crc_error_cnt_high);

    if (!bad_frame &&
            filter_ack(hw, (struct ieee80211_hdr *)buffer, &stats)
            && !mac->pass_ctrl)
        return 0;

    buffer += sizeof(struct rx_status);
    payload_length = TO_HOST(*(u32*)buffer);

    // MTU = 1500, MAC header = 36, CRC = 4, sum = 1540
    if (payload_length > 1560) {
        pl_dev_info(purelifi_mac_dev(mac), "WRN::%s::payload_length=%u\n", __FUNCTION__, payload_length);
        return 0;
    }
    buffer += sizeof(u32);

    fc = get_unaligned((__le16*)buffer);
    need_padding = ieee80211_is_data_qos(fc) ^ ieee80211_has_a4(fc);

    {
        int station_index; 
        for (station_index = 0; station_index < MAX_STA_NUM-1; station_index++) {
            if (!memcmp(&buffer[10], mac->chip.usb.tx.station[station_index].mac, ETH_ALEN)) {
                if (mac->chip.usb.tx.station[station_index].flag & STATION_CONNECTED_FLAG) {
                    mac->chip.usb.tx.station[station_index].flag |= STATION_HEARTBEAT_FLAG;
                    break;
                }
            }
        }

        if (station_index == MAX_STA_NUM -1) {
            for (station_index = 0; station_index < MAX_STA_NUM-1; station_index++) {
                if (!(mac->chip.usb.tx.station[station_index].flag & STATION_CONNECTED_FLAG)) {
                    memcpy(mac->chip.usb.tx.station[station_index].mac, &buffer[10], ETH_ALEN);
                    mac->chip.usb.tx.station[station_index].flag |= STATION_CONNECTED_FLAG;
                    mac->chip.usb.tx.station[station_index].flag |= STATION_HEARTBEAT_FLAG;
                    break;
                }
            }
        }
    }

    if (buffer[0] == 0xb0) {
        pl_dev_info(purelifi_mac_dev(mac), "%s:Authentication request\n", __FUNCTION__);
    }
    else if (buffer[0] == 0x40) {
        pl_dev_info(purelifi_mac_dev(mac), "%s:Probe request\n", __FUNCTION__);
    }
    else if (buffer[0] == 0x0) {
        pl_dev_info(purelifi_mac_dev(mac), "%s:Association request\n", __FUNCTION__);
    }

    if (buffer[0] == 0xb0) {
        min_exp_seq_nmb = 0;
    }
    else if (buffer[0] == 0x08) {
        unsigned short int seq_nmb = (buffer[23] << 4) | ((buffer[22] & 0xF0) >> 4);
        if (seq_nmb < min_exp_seq_nmb && ((min_exp_seq_nmb - seq_nmb) < 3000)) {
            // return 0;
        }
        else {
            min_exp_seq_nmb = (seq_nmb + 1) % 4096;
        }
    }

    skb = dev_alloc_skb(payload_length + (need_padding ? 2 : 0));
    if (skb == NULL)
        return -ENOMEM;
    if (need_padding) {
        /* Make sure the the payload data is 4 byte aligned. */
        skb_reserve(skb, 2);
    }

    /* FIXME : could we avoid this big memcpy ? */
    memcpy(skb_put(skb, payload_length), buffer, payload_length);
    memcpy(IEEE80211_SKB_RXCB(skb), &stats, sizeof(stats));
    ieee80211_rx_irqsafe(hw, skb);
    return 0;
}

static int purelifi_op_add_interface(struct ieee80211_hw *hw,
				struct ieee80211_vif *vif)
{
	struct purelifi_mac *mac = purelifi_hw_mac(hw);
	pl_dev_info(purelifi_mac_dev(mac), "%s\n", __FUNCTION__);

	if (mac->type != NL80211_IFTYPE_UNSPECIFIED)
		return -EOPNOTSUPP;

	switch (vif->type) {
	case NL80211_IFTYPE_MONITOR:
            pl_dev_info(purelifi_mac_dev(mac), "%s::vif->type=NL80211_IFTYPE_MONITOR\n", __FUNCTION__);
            break;
	case NL80211_IFTYPE_MESH_POINT:
            pl_dev_info(purelifi_mac_dev(mac), "%s::vif->type=NL80211_IFTYPE_MESH_POINT\n", __FUNCTION__);
            break;
	case NL80211_IFTYPE_STATION:
            pl_dev_info(purelifi_mac_dev(mac), "%s::vif->type=NL80211_IFTYPE_STATION\n", __FUNCTION__);
            break;
	case NL80211_IFTYPE_ADHOC:
            pl_dev_info(purelifi_mac_dev(mac), "%s::vif->type=NL80211_IFTYPE_ADHOC\n", __FUNCTION__);
            break;
	case NL80211_IFTYPE_AP:
            pl_dev_info(purelifi_mac_dev(mac), "%s::vif->type=NL80211_IFTYPE_AP\n", __FUNCTION__);
	    break;
	default:
		return -EOPNOTSUPP;
	}
	mac->type = vif->type;
        mac->vif = vif;
	return 0;
}

static void purelifi_op_remove_interface(struct ieee80211_hw *hw,
				    struct ieee80211_vif *vif)
{
	struct purelifi_mac *mac = purelifi_hw_mac(hw);
	pl_dev_info(purelifi_mac_dev(mac), "%s\n", __FUNCTION__);
	mac->type = NL80211_IFTYPE_UNSPECIFIED;
	mac->vif = NULL;
}

static int purelifi_op_config(struct ieee80211_hw *hw, u32 changed)
{
	struct purelifi_mac *mac = purelifi_hw_mac(hw);

	spin_lock_irq(&mac->lock);
	spin_unlock_irq(&mac->lock);

	return 0;
}

#define SUPPORTED_FIF_FLAGS \
	(FIF_ALLMULTI | FIF_FCSFAIL | FIF_CONTROL | \
	FIF_OTHER_BSS | FIF_BCN_PRBRESP_PROMISC)
static void purelifi_op_configure_filter(struct ieee80211_hw *hw,
			unsigned int changed_flags,
			unsigned int *new_flags,
			u64 multicast)
{
	struct purelifi_mc_hash hash = {
		.low = multicast,
		.high = multicast >> 32,
	};
	struct purelifi_mac *mac = purelifi_hw_mac(hw);
	unsigned long flags;
	int r;

	/* Only deal with supported flags */
	changed_flags &= SUPPORTED_FIF_FLAGS;
	*new_flags &= SUPPORTED_FIF_FLAGS;

	/*
	 * If multicast parameter (as returned by purelifi_op_prepare_multicast)
	 * has changed, no bit in changed_flags is set. To handle this
	 * situation, we do not return if changed_flags is 0. If we do so,
	 * we will have some issue with IPv6 which uses multicast for link
	 * layer address resolution.
	 */
	if (*new_flags & (FIF_ALLMULTI))
		purelifi_mc_add_all(&hash);

	spin_lock_irqsave(&mac->lock, flags);
	mac->pass_failed_fcs = !!(*new_flags & FIF_FCSFAIL);
	mac->pass_ctrl = !!(*new_flags & FIF_CONTROL);
	mac->multicast_hash = hash;
	spin_unlock_irqrestore(&mac->lock, flags);

	//purelifi_chip_set_multicast_hash(&mac->chip, &hash);

	if (changed_flags & FIF_CONTROL) {
                /* TODO: Re-enable these lines?
		r = set_rx_filter(mac);
		if (r)
			pl_dev_err(purelifi_mac_dev(mac), "set_rx_filter error %d\n", r);
                */
                r = 0;
	}

	/* no handling required for FIF_OTHER_BSS as we don't currently
	 * do BSSID filtering */
	/* FIXME: in future it would be nice to enable the probe response
	 * filter (so that the driver doesn't see them) until
	 * FIF_BCN_PRBRESP_PROMISC is set. however due to atomicity here, we'd
	 * have to schedule work to enable prbresp reception, which might
	 * happen too late. For now we'll just listen and forward them all the
	 * time. */
}

static void purelifi_op_bss_info_changed(struct ieee80211_hw *hw,
				   struct ieee80211_vif *vif,
				   struct ieee80211_bss_conf *bss_conf,
				   u32 changes)
{
	struct purelifi_mac *mac = purelifi_hw_mac(hw);
	int associated;
	pl_dev_info(purelifi_mac_dev(mac), "%s\n", __FUNCTION__);
	pl_dev_info(purelifi_mac_dev(mac), "changes: %x\n", changes);

	if (mac->type == NL80211_IFTYPE_MESH_POINT ||
	    mac->type == NL80211_IFTYPE_ADHOC ||
	    mac->type == NL80211_IFTYPE_AP) {
		associated = true;
		if (changes & BSS_CHANGED_BEACON) {
			struct sk_buff *beacon = ieee80211_beacon_get(hw, vif);

			if (beacon) {
			    purelifi_mac_config_beacon(hw, beacon, false);
			    kfree_skb(beacon);//Returned skb is used only once and low-level driver is responsible for freeing it. 
			}
		}

		if (changes & BSS_CHANGED_BEACON_ENABLED) {
			u16 interval = 0;
			u8 period = 0;

			if (bss_conf->enable_beacon) {
				period = bss_conf->dtim_period;
				interval = bss_conf->beacon_int;
			}

			spin_lock_irq(&mac->lock);
			mac->beacon.period = period;
			mac->beacon.interval = interval;
			mac->beacon.last_update = jiffies;
			spin_unlock_irq(&mac->lock);
                        
			purelifi_set_beacon_interval(&mac->chip, interval, period, mac->type);
		}
	} else
		associated = is_valid_ether_addr(bss_conf->bssid);

	spin_lock_irq(&mac->lock);
	mac->associated = associated;
	spin_unlock_irq(&mac->lock);
}

static int purelifi_get_stats(struct ieee80211_hw *hw, struct ieee80211_low_level_stats *stats) {
    stats->dot11ACKFailureCount = 0;
    stats->dot11RTSFailureCount = 0;
    stats->dot11FCSErrorCount = 0;
    stats->dot11RTSSuccessCount = 0;
    return 0;
}

static s8 get_rssi_scaled(struct ieee80211_hw *hw) {
    #define MAX_REPORTED_RSSI (100)
    #define MIN_RECEIVED_RSSI (50)
    struct purelifi_mac *mac = purelifi_hw_mac(hw);

    return (MIN_RECEIVED_RSSI - mac->rssi)*(MAX_REPORTED_RSSI/MIN_RECEIVED_RSSI);
}

#ifndef OPENWRT
    static int purelifi_get_rssi(struct ieee80211_hw *hw, struct ieee80211_vif *vif, struct ieee80211_sta *sta, s8 *rssi_dbm) {
        *rssi_dbm = get_rssi_scaled(hw);
        return 0;
    }
#endif

const char et_strings[][ETH_GSTRING_LEN] = {
    "phy_rssi",
    "phy_rx_crc_err"
};

static int purelifi_get_et_sset_count(struct ieee80211_hw *hw, struct ieee80211_vif *vif, int sset) {
    if (sset == ETH_SS_STATS) {
        return ARRAY_SIZE(et_strings);
    }
    return 0;
}

static void purelifi_get_et_strings(struct ieee80211_hw *hw, struct ieee80211_vif *vif, u32 sset, u8 *data) {
    if (sset == ETH_SS_STATS)
        memcpy(data, *et_strings, sizeof(et_strings));
}

static void purelifi_get_et_stats(struct ieee80211_hw *hw,
    struct ieee80211_vif *vif,
    struct ethtool_stats *stats, u64 *data
) {
    struct purelifi_mac *mac = purelifi_hw_mac(hw);
    data[0] = get_rssi_scaled(hw);
    data[1] = mac->crc_errors;
}

static int purelifi_set_rts_threshold(struct ieee80211_hw *hw, u32 value) {
    return 0;
}

static const struct ieee80211_ops purelifi_ops = {
    .tx                 = purelifi_op_tx,
    .start              = purelifi_op_start,
    .stop               = purelifi_op_stop,
    .add_interface      = purelifi_op_add_interface,
    .remove_interface   = purelifi_op_remove_interface,
	 .set_rts_threshold  = purelifi_set_rts_threshold,
    .config             = purelifi_op_config,
    .configure_filter   = purelifi_op_configure_filter,
    .bss_info_changed   = purelifi_op_bss_info_changed,
    .get_stats          = purelifi_get_stats,
#ifndef OPENWRT
    .get_rssi           = purelifi_get_rssi,
#endif
    .get_et_sset_count  = purelifi_get_et_sset_count,
    .get_et_stats       = purelifi_get_et_stats,
    .get_et_strings     = purelifi_get_et_strings,
};

struct ieee80211_hw *purelifi_mac_alloc_hw(struct usb_interface *intf)
{
    struct purelifi_mac *mac;
    struct ieee80211_hw *hw;
    
    hw = ieee80211_alloc_hw(sizeof(struct purelifi_mac), &purelifi_ops);
    if (!hw) {
       dev_dbg_f(&intf->dev, "out of memory\n");
       return NULL;
    }
    set_wiphy_dev(hw->wiphy, &intf->dev);
   
    mac = purelifi_hw_mac(hw);
   
    memset(mac, 0, sizeof(*mac));
    spin_lock_init(&mac->lock);
    mac->hw = hw;
   
    mac->type = NL80211_IFTYPE_UNSPECIFIED;
   
    memcpy(mac->channels, purelifi_channels, sizeof(purelifi_channels));
    memcpy(mac->rates, purelifi_rates, sizeof(purelifi_rates));
    mac->band.n_bitrates = ARRAY_SIZE(purelifi_rates);
    mac->band.bitrates = mac->rates;
    mac->band.n_channels = ARRAY_SIZE(purelifi_channels);
    mac->band.channels = mac->channels;
   
    hw->wiphy->bands[IEEE80211_BAND_2GHZ] = &mac->band;
    
    #ifdef ieee80211_hw_set
       _ieee80211_hw_set(hw, IEEE80211_HW_RX_INCLUDES_FCS);
       _ieee80211_hw_set(hw, IEEE80211_HW_SIGNAL_UNSPEC);
       _ieee80211_hw_set(hw, IEEE80211_HW_HOST_BROADCAST_PS_BUFFERING);
       _ieee80211_hw_set(hw, IEEE80211_HW_MFP_CAPABLE);
    #else
       hw->flags = IEEE80211_HW_RX_INCLUDES_FCS | 
          IEEE80211_HW_SIGNAL_UNSPEC |
          IEEE80211_HW_HOST_BROADCAST_PS_BUFFERING |
          IEEE80211_HW_MFP_CAPABLE;
    #endif

    hw->wiphy->interface_modes =
       BIT(NL80211_IFTYPE_MESH_POINT) |
       BIT(NL80211_IFTYPE_STATION) |
       BIT(NL80211_IFTYPE_ADHOC) |
       BIT(NL80211_IFTYPE_AP);
   
    hw->max_signal = 100;
    hw->queues = 1;
    hw->extra_tx_headroom = sizeof(struct purelifi_ctrlset) + 4; //4 for 32 bit alignment if no tailroom
   
    /*
     * Tell mac80211 that we support multi rate retries
     */
    hw->max_rates = IEEE80211_TX_MAX_RATES;
    hw->max_rate_tries = 18;   /* 9 rates * 2 retries/rate */
   
    skb_queue_head_init(&mac->ack_wait_queue);
    mac->ack_pending = 0;
   
    purelifi_chip_init(&mac->chip, hw, intf);
   
    SET_IEEE80211_DEV(hw, &intf->dev);
    return hw;
}
