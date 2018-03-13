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

#ifndef _PURELIFI_DEF_H
#define _PURELIFI_DEF_H

#include <linux/kernel.h>
#include <linux/stringify.h>
#include <linux/device.h>

typedef u16 __nocast purelifi_addr_t;

#define dev_printk_f(level, dev, fmt, args...) \
	dev_printk(level, dev, "%s() " fmt, __func__, ##args)

#ifdef DEBUG
#  define dev_dbg_f(dev, fmt, args...) \
	  dev_printk_f(KERN_DEBUG, dev, fmt, ## args)
#  define dev_dbg_f_limit(dev, fmt, args...) do { \
	if (net_ratelimit()) \
		dev_printk_f(KERN_DEBUG, dev, fmt, ## args); \
} while (0)
#  define dev_dbg_f_cond(dev, cond, fmt, args...) ({ \
	bool __cond = !!(cond); \
	if (unlikely(__cond)) \
		dev_printk_f(KERN_DEBUG, dev, fmt, ## args); \
})
#else
#  define dev_dbg_f(dev, fmt, args...) do { (void)(dev); } while (0)
#  define dev_dbg_f_limit(dev, fmt, args...) do { (void)(dev); } while (0)
#  define dev_dbg_f_cond(dev, cond, fmt, args...) do { (void)(dev); } while (0)
#endif /* DEBUG */

#ifdef DEBUG
#  define PURELIFI_ASSERT(x) \
do { \
	if (unlikely(!(x))) { \
		pr_debug("%s:%d ASSERT %s VIOLATED!\n", \
			__FILE__, __LINE__, __stringify(x)); \
		dump_stack(); \
	} \
} while (0)
#else
#  define PURELIFI_ASSERT(x) do { } while (0)
#endif

#ifdef DEBUG
#  define PURELIFI_MEMCLEAR(pointer, size) memset((pointer), 0xff, (size))
#else
#  define PURELIFI_MEMCLEAR(pointer, size) do { } while (0)
#endif

#endif /* _PURELIFI_DEF_H */
