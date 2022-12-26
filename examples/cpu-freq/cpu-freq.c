/*
 * Copyright (c) 2022 Nuvoton technology corporation
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>


#define SET_CPU_FREQ_500M	0x1005
#define SET_CPU_FREQ_600M	0x1006
#define SET_CPU_FREQ_800M	0x1008
#define GET_PMIC_VOLT		0x1101
#define SET_PMIC_VOLT		0x1102

/* valid PMIC voltages are:
 *	1.00V, 1.10V, 1.15V, 1.20V, 1.25V, 1.29V, 1.30V, 1.32V, 1.34V
 */
#define PMIC_VOLT_1_25V		125 /* means 1.25V */
#define PMIC_VOLT_1_10V		110 /* means 1.10V */


void loop_time_measure(int freq)
{
	struct timeval t0, t1;
	int i, j, tt, tm;

	gettimeofday(&t0, NULL);
	for (i = 0; i < 10000; i++) {
		for (j = 0; j < 10000; j++)
			;
	}
	gettimeofday(&t1, NULL);
	tt = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
	printf("CPU %dMHz run 100000K loop elapsed: %d us\n", freq, tt);
}

int main(int argc, char **argv)
{
	int fd;
	int rev;

	fd = open("/dev/ma35_misctrl", O_RDWR);
	if (fd < 0) {
		printf("Failed to open /dev/ma35_misctrl ! errno = %d\n", fd);
		return -ENODEV;
	}

	if (ioctl(fd, SET_PMIC_VOLT, PMIC_VOLT_1_25V)) {
		printf("Failed to restore PMIC voltage 1.25V!\n");
		goto out;
	}
	if (ioctl(fd, SET_CPU_FREQ_800M, 0)) {
		printf("Failed to set CPU clock as 800MHz!\n");
		goto out;
	}
	loop_time_measure(800);

	/*
	 * CPU 600MHz ca run with voltage 1.10V
	 */
	if (ioctl(fd, SET_PMIC_VOLT, PMIC_VOLT_1_10V)) {
		printf("Failed to restore PMIC voltage 1.10V!\n");
		goto out;
	}
	if (ioctl(fd, SET_CPU_FREQ_600M, 0)) {
		printf("Failed to set CPU clock as 600MHz!\n");
		goto out;
	}
	loop_time_measure(600);

	if (ioctl(fd, SET_CPU_FREQ_500M, 0)) {
		printf("Failed to set CPU clock as 500MHz!\n");
		goto out;
	}
	loop_time_measure(500);

	/*
	 * CPU 800MHz should running with voltage 1.25V
	 */
	if (ioctl(fd, SET_PMIC_VOLT, PMIC_VOLT_1_25V)) {
		printf("Failed to restore PMIC voltage 1.25V!\n");
		goto out;
	}
	if (ioctl(fd, SET_CPU_FREQ_800M, 0)) {
		printf("Failed to set CPU clock as 800MHz!\n");
		goto out;
	}
	loop_time_measure(800);

out:
	close(fd);
	exit(0);
}
