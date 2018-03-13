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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <linux/kernel.h>
#include <linux/errno.h>

#include "purelifi_def.h"
#include "purelifi_chip.h"
#include "purelifi_mac.h"
#include "purelifi_usb.h"
#include "purelifi_log.h"

void purelifi_chip_init(struct purelifi_chip *chip,
	         struct ieee80211_hw *hw,
		 struct usb_interface *intf)
{
	memset(chip, 0, sizeof(*chip));
	mutex_init(&chip->mutex);
	purelifi_usb_init(&chip->usb, hw, intf);
}

void purelifi_chip_clear(struct purelifi_chip *chip)
{
	PURELIFI_ASSERT(!mutex_is_locked(&chip->mutex));
	purelifi_usb_clear(&chip->usb);
	mutex_destroy(&chip->mutex);
	PURELIFI_MEMCLEAR(chip, sizeof(*chip));
}

static int scnprint_mac_oui(struct purelifi_chip *chip, char *buffer, size_t size)
{
	u8 *addr = purelifi_mac_get_perm_addr(purelifi_chip_to_mac(chip));
	return scnprintf(buffer, size, "%02x-%02x-%02x",
		         addr[0], addr[1], addr[2]);
}

/* Prints an identifier line, which will support debugging. */
static int scnprint_id(struct purelifi_chip *chip, char *buffer, size_t size)
{
	int i = 0;
	i = scnprintf(buffer, size, "purelifi%s chip ", "");
	i += purelifi_usb_scnprint_id(&chip->usb, buffer+i, size-i);
	i += scnprintf(buffer+i, size-i, " ");
	i += scnprint_mac_oui(chip, buffer+i, size-i);
	i += scnprintf(buffer+i, size-i, " ");
	return i;
}

static void print_id(struct purelifi_chip *chip)
{
    char buffer[80];

    scnprint_id(chip, buffer, sizeof(buffer));
    buffer[sizeof(buffer)-1] = 0;
    pl_dev_info(purelifi_chip_dev(chip), "%s\n", buffer);
}


/* MAC address: if custom mac addresses are to be used CR_MAC_ADDR_P1 and
 *              CR_MAC_ADDR_P2 must be overwritten
 */
int purelifi_write_mac_addr(struct purelifi_chip *chip, const u8 *mac_addr)
{
    int r;
    r = usb_write_req(mac_addr, ETH_ALEN, USB_REQ_MAC_WR);
    return r;
}

int purelifi_set_beacon_interval(struct purelifi_chip *chip, u16 interval, u8 dtim_period, int type)
{
    int r;
    if (!interval || (chip->beacon_set && chip->beacon_interval == interval)) {
        return 0;
    }
    else {
        chip->beacon_interval = interval;
        chip->beacon_set = true;
        r = usb_write_req((const u8*)&chip->beacon_interval, sizeof(chip->beacon_interval), USB_REQ_BEACON_INTERVAL_WR);
        return r;
    }
}

static int hw_init(struct purelifi_chip *chip)
{
	return purelifi_set_beacon_interval(chip, 100, 0, 0);
}

int purelifi_chip_init_hw(struct purelifi_chip *chip)
{
	int r;
	r = hw_init(chip);
	if (r)
		goto out;

	print_id(chip);
out:
	return r;
}

int purelifi_chip_switch_radio(struct purelifi_chip *chip, u32 value)
{
	int r;
        r = usb_write_req((const u8*)&value, sizeof(value), USB_REQ_POWER_WR);
        if ( r )
                pl_dev_err(purelifi_chip_dev(chip), "%s::r=%d", __FUNCTION__, r);
	return r;
}

int purelifi_chip_switch_radio_on(struct purelifi_chip *chip)
{
	return purelifi_chip_switch_radio(chip, 1);
}

int purelifi_chip_switch_radio_off(struct purelifi_chip *chip)
{
	return purelifi_chip_switch_radio(chip, 0);
}

void purelifi_chip_enable_rxtx_urb_complete(struct urb * urb_struct) {
}

int purelifi_chip_enable_rxtx(struct purelifi_chip *chip) {
	purelifi_usb_enable_tx(&chip->usb);
	return purelifi_usb_enable_rx(&chip->usb);
}

void purelifi_chip_disable_rxtx(struct purelifi_chip *chip)
{
        u32 value;
        value = 0;
	usb_write_req((const u8*)&value, sizeof(value), USB_REQ_RXTX_WR);
	purelifi_usb_disable_rx(&chip->usb);
	purelifi_usb_disable_tx(&chip->usb);
}

int purelifi_chip_set_rate(struct purelifi_chip *chip, u32 rate)
{
	int r;
    static struct purelifi_chip *chip_p;

    if (chip) {
        chip_p = chip;
    }

    if (!chip_p) {
        return -EINVAL;
    }

    r = usb_write_req((const u8*)&rate, sizeof(rate), USB_REQ_RATE_WR);
    if ( r )
       pl_dev_err(purelifi_chip_dev(chip), "%s::r=%d", __FUNCTION__, r);
	return r;
}

