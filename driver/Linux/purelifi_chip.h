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

#ifndef _LF_X_CHIP_H
#define _LF_X_CHIP_H

#include <net/mac80211.h>

#include "purelifi_usb.h"

enum unit_type_t {
    STA = 0,
    AP = 1,
};

struct purelifi_chip {
	struct purelifi_usb usb;
	struct mutex mutex;
    enum unit_type_t unit_type;
	u16 link_led;
    u8  beacon_set;
    u16 beacon_interval;
};

static inline struct purelifi_chip *purelifi_usb_to_chip(struct purelifi_usb *usb)
{
	return container_of(usb, struct purelifi_chip, usb);
}

#define purelifi_chip_dev(chip) (&(chip)->usb.intf->dev)

void purelifi_chip_init(struct purelifi_chip *chip,
	         struct ieee80211_hw *hw,
	         struct usb_interface *intf);
void purelifi_chip_clear(struct purelifi_chip *chip);
int purelifi_chip_init_hw(struct purelifi_chip *chip);


/* Locking functions for reading and writing registers.
 * The different parameters are intentional.
 */
int purelifi_write_mac_addr(struct purelifi_chip *chip, const u8 *mac_addr);
int purelifi_chip_switch_radio_on(struct purelifi_chip *chip);
int purelifi_chip_switch_radio_off(struct purelifi_chip *chip);
int purelifi_chip_enable_rxtx(struct purelifi_chip *chip);
void purelifi_chip_disable_rxtx(struct purelifi_chip *chip);
int purelifi_chip_set_rate(struct purelifi_chip *chip, u32 rate);
int purelifi_set_beacon_interval(struct purelifi_chip *chip, u16 interval, u8 dtim_period, int type);
struct purelifi_mc_hash {
	u32 low;
	u32 high;
};

static inline void purelifi_mc_clear(struct purelifi_mc_hash *hash)
{
	hash->low = 0;
	/* The interfaces must always received broadcasts.
	 * The hash of the broadcast address ff:ff:ff:ff:ff:ff is 63.
	 */
	hash->high = 0x80000000;
}

static inline void purelifi_mc_add_all(struct purelifi_mc_hash *hash)
{
	hash->low = hash->high = 0xffffffff;
}

static inline void purelifi_mc_add_addr(struct purelifi_mc_hash *hash, u8 *addr)
{
	unsigned int i = addr[5] >> 2;
	if (i < 32) {
		hash->low |= 1 << i;
	} else {
		hash->high |= 1 << (i-32);
	}
}
#endif /* _LF_X_CHIP_H */
