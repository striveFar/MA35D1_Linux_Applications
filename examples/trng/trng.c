// SPDX-License-Identifier: GPL-2.0
/*
 * MA35D1 TRNG Driver Test/Example Program
 *
 * The sample program for MA35D1 TRNG
 *
 * Copyright (c) 2022 Nuvoton technology corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation;version 2 of the License.
 */

#include <stdio.h>
#include <string.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <signal.h>


int main(int argc, char **argv)
{
	char    trng_dev[] = "/dev/hwrng";
	__u32   trng_buff[1024];
	int	fd;
	int	i, loop;

	fd = open(trng_dev, O_RDONLY, 0666);
	if (fd < 0) {
		printf("open %s faild!!!\n", trng_dev);
		return -1;
	}
	for (loop = 0; loop < 3; loop++) {
		read(fd, trng_buff, 64);
		printf("TRNG DATA ==>\n");
		for (i = 0; i < 64/4; i++)
			printf("%x ", trng_buff[i]);
		printf("\n");
	}
	close(fd);
	return 0;
} /* end main */

