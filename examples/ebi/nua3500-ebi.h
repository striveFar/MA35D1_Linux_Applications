/*
 *
 * Copyright (c) 2018 Nuvoton technology corporation
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
#ifndef _NUA3500_EBI_H_
#define _NUA3500_EBI_H_

#include <linux/types.h>
#include <linux/ioctl.h>

#define EBI_IOC_MAGIC		'e'


#define EBI_IOC_SET		_IOW(EBI_IOC_MAGIC, 0, unsigned int *)
#define EBI_IOC_SPEED		_IOW(EBI_IOC_MAGIC, 1, unsigned int *)

#define EBI_BUSWIDTH_8BIT       8UL   /*!< EBI bus width is 8-bit 	*/
#define EBI_BUSWIDTH_16BIT      16UL  /*!< EBI bus width is 16-bit	*/

#define EBI_CS_ACTIVE_LOW       0UL    /*!< EBI CS active level is low  */
#define EBI_CS_ACTIVE_HIGH      1UL    /*!< EBI CS active level is high */

#define EBI_TIMING_FASTEST      0x0UL /*!< EBI timing is the fastest */
#define EBI_TIMING_VERYFAST     0x1UL /*!< EBI timing is very fast */
#define EBI_TIMING_FAST         0x2UL /*!< EBI timing is fast */
#define EBI_TIMING_NORMAL       0x3UL /*!< EBI timing is normal */
#define EBI_TIMING_SLOW         0x4UL /*!< EBI timing is slow */
#define EBI_TIMING_VERYSLOW     0x5UL /*!< EBI timing is very slow */
#define EBI_TIMING_SLOWEST      0x6UL /*!< EBI timing is the slowest */

#define EBI_OPMODE_NORMAL       0x0UL /*!< EBI bus operate in normal mode */
#define EBI_OPMODE_CACCESS      (1<<4)/*!< EBI bus operate in Continuous Data Access mode */
#define EBI_OPMODE_ADSEPARATE   (1<<3)/*!< EBI bus operate in AD Separate mode */

struct nua3500_set_ebi {
	unsigned int bank;
	unsigned int busmode;
	unsigned int CSActiveLevel;
	unsigned int base;
	unsigned int size;
	unsigned int width;
};

#endif
