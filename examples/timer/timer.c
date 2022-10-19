/*
 * Copyright (c) 2018 Nuvoton technology corporation
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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "nua3500-timer.h"

#define __START_CH		(2)  // 2~11
#define __END_CH		(3)  // 2~11


#define TEST_WK				(0)
#define TEST_PERIODIC			(1)
#define TEST_TOGGLE			(1)
#define TEST_EVENT_COUNTING		(1)
#define TEST_FREE_COUNTING		(1)
#define TEST_TRIGGER_COUNTING_BLOCK	(1)
#define TEST_TRIGGER_COUNTING_POLL	(1)

int main(int argc, char **argv)
{
	int fd[12], i, j, mode = TMR_CAP_EDGE_FF;
	fd_set rfd;
	struct timeval tv;
	unsigned int cap, cnt;
	int ret;
	unsigned int isrcnt = 0;
	char dev[12][14] = {"/dev/timer0",
			"/dev/timer1",
			"/dev/timer2",
			"/dev/timer3",
			"/dev/timer4",
			"/dev/timer5",
			"/dev/timer6",
			"/dev/timer7",
			"/dev/timer8",
			"/dev/timer9",
			"/dev/timer10",
			"/dev/timer11"
			};
	char u8char;
	int period[4] = {10000, 8000, 6000, 4000};// Priod (in us) of 4 timer channels
	int wkperiod[4] = {32768 , 8192, 65536, 163840};
	int timeout = 12000000, lxttimeout = 32768;// 1 sec
	int eventcnt = 50;
	int clksrc = __HXT;
	int clkwksrc = __LIRC;


#if (TEST_WK == 1)
	for(i = __START_CH; i < (__END_CH + 1); i++) 
	{
		//i = __START_CH;
		fd[i] = open(&dev[i][0], O_RDWR);
		if(fd[i] < 0)
		{
			printf("Open Timer%d error !!!\n", i);
			close(fd[i]);
			return -1;
		}

		printf("Channel %d Time-out Wakeup testing\n", i);
		// switch clock, power down mode using 32kHz
		ioctl(fd[i], TMR_IOC_CLKLXT, &clkwksrc);

		ret = ioctl(fd[i], TMR_IOC_PERIODIC_FOR_WKUP, &wkperiod[0]);
		if(ret == -1)
			printf("clock source error\n");
		else {

			printf("\nEnter DPD(mem)\n");
			system("echo mem > /sys/power/state");
			printf("\nWake up from Timer%d.\n",i);
			ioctl(fd[i], TMR_IOC_STOP, NULL);
			close(fd[i]);
		}
	}
#endif


#if (TEST_PERIODIC == 1)
	for(i = __START_CH; i < (__END_CH + 1); i++) 
	{
		//i = __START_CH;
		fd[i] = open(&dev[i][0], O_RDWR);
		if(fd[i] < 0)
		{
			printf("Open Timer%d error !!!\n", i);
			close(fd[i]);
			return -1;
		}

		printf("Channel %d Periodic testing\n", i);

		//switch clock HXT
		ioctl(fd[i], TMR_IOC_CLKHXT, &clksrc);
		ioctl(fd[i], TMR_IOC_PERIODIC, &timeout);
		for(j = 0; j < 3; j++) {
			read(fd[i], &cnt, sizeof(cnt));
			printf("    %d sec\n", cnt);
		}

		printf("hit any key to test next channel\n");
		getchar();
		ioctl(fd[i], TMR_IOC_STOP, NULL);
		close(fd[i]);
	}

	printf("Periodic Done ======\n");
#endif



#if (TEST_TOGGLE == 1)
	for(i = __START_CH; i < (__END_CH + 1); i++) 
	{
		fd[i] = open(&dev[i][0], O_RDWR);
		if(fd[i] < 0)
		{
			printf("Open Timer%d error !!!\n", i);
			close(fd[i]);
			return -1;
		}
		ioctl(fd[i], TMR_IOC_CLKHXT, &clksrc);
		printf("Channel %d Toggle Output testing\n", i);
		//for(j =0; j <4; j++)
		{
			j = 1;
			ioctl(fd[i], TMR_IOC_TOGGLE, &period[j]);
		}

		printf("hit any key to test next channel\n");
		getchar();
		ioctl(fd[i], TMR_IOC_STOP, NULL);
		close(fd[i]);
	}

	printf("Toggle Output Done ======\n");
#endif


#if (TEST_EVENT_COUNTING == 1)
	for(i = __START_CH; i < (__END_CH + 1); i++) 
	{
		fd[i] = open(&dev[i][0], O_RDWR);
		if(fd[i] < 0)
		{
			printf("Open Timer%d error !!!\n", i);
			close(fd[i]);
			return -1;
		}

		printf("Channel %d Event Count testing\n", i);
		//switch clock HXT
		ioctl(fd[i], TMR_IOC_CLKHXT, &clksrc);
		ioctl(fd[i], TMR_IOC_EVENT_COUNTING, &eventcnt);
		for(j = 0; j < 5; j++) {
			read(fd[i], &cnt, sizeof(cnt));
			printf("Generate falling event. Count %d\n", cnt);
		}
		printf("hit any key to test next channel\n");
		getchar();
		ioctl(fd[i], TMR_IOC_STOP, NULL);
		close(fd[i]);
	}

	printf("Event Count Done ======\n");
#endif



#if (TEST_FREE_COUNTING == 1)
	for(i = __START_CH; i < (__END_CH + 1); i++) 
	{
		fd[i] = open(&dev[i][0], O_RDWR);
		if(fd[i] < 0)
		{
			printf("Open Timer%d error !!!\n", i);
			close(fd[i]);
			return -1;
		}

		printf("Channel %d Capture: Free Counting testing\n", i);
		ioctl(fd[i], TMR_IOC_FREE_COUNTING, &mode);
		for(j = 0; j < 5; j++) {
			read(fd[i], &cap, sizeof(cap));
			printf("cap:%d Hz\n", cap);
		}
		printf("hit any key to test next channel\n");
		getchar();
		ioctl(fd[i], TMR_IOC_STOP, NULL);
		close(fd[i]);
	}

	printf("Capture: Free Counting Done ======\n");
#endif


#if (TEST_TRIGGER_COUNTING_BLOCK == 1)
	for(i = __START_CH+0; i < (__END_CH + 1); i++) 
	{
		fd[i] = open(&dev[i][0], O_RDWR);
		if(fd[i] < 0)
		{
			printf("Open Timer%d error !!!\n", i);
			close(fd[i]);
			return -1;
		}

		printf("Channel %d Capture: TRIGGER Counting block testing\n", i);
		ioctl(fd[i], TMR_IOC_TRIGGER_COUNTING, &mode);
		for(j = 0; j < 5; j++) {
			read(fd[i], &cap, sizeof(cap));
			printf("cap: %d us\n", cap);
		}
		printf("hit any key to test next channel\n");
		getchar();
		ioctl(fd[i], TMR_IOC_STOP, NULL);
		close(fd[i]);
	}

	printf("\nCapture: TRIGGER Counting, block Done ======\n");
#endif

#if (TEST_TRIGGER_COUNTING_POLL == 1)
	printf("\nCapture: TRIGGER Counting polling testing\n");
	for(i = __START_CH+0; i < (__END_CH + 1); i++) 
	{
		fd[i] = open(&dev[i][0], O_RDWR);
		if(fd[i] < 0)
		{
			printf("Open Timer%d error !!!\n", i);
			close(fd[i]);
			return -1;
		}

		printf("Channel %d\n", i);
		FD_ZERO(&rfd);
		FD_SET(fd[i], &rfd);

		ioctl(fd[i], TMR_IOC_TRIGGER_COUNTING, &mode);
		for(j = 0; j < 5; j++) {
			ret = select(fd[i] + 1, &rfd, NULL, NULL, NULL);
			if(ret == -1) {
				printf("select error\n");
			} else if(FD_ISSET(fd[i], &rfd)) {
				read(fd[i], &cap, sizeof(cap));
				printf("cap: %d us\n", cap);
			}
		}
		printf("hit any key to test next channel\n");
		getchar();
		ioctl(fd[i], TMR_IOC_STOP, NULL);
		close(fd[i]);
	}

	printf("\nCapture: TRIGGER Counting, polling Done ======\n\n");
#endif

	return 0;
}
