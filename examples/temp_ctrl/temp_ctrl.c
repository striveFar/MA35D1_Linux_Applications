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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h> 
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <math.h>
#include <sys/ioctl.h>
#include <linux/rtc.h>

#if 1 // for test
#define T_Z1_TO_Z2		55	/* Enter Z2 once larger than T_Z1_TO_Z2 */
#define T_Z2_TO_Z3		56	/* Enter Z3 once larger than T_Z2_TO_Z3 */
#define T_Z3_TO_Z4		57	/* Enter Z4 once larger than T_Z3_TO_Z4 */
#define T_DOWN_TO_Z1		53	/* Once enter Z2/Z3/Z4, keep cooling until lower than T_DOWN_TO_Z1 to back to Zone 1 */
#else
#define T_Z1_TO_Z2		115	/* Enter Z2 once larger than T_Z1_TO_Z2 */
#define T_Z2_TO_Z3		120	/* Enter Z3 once larger than T_Z2_TO_Z3 */
#define T_Z3_TO_Z4		125	/* Enter Z4 once larger than T_Z3_TO_Z4 */
#define T_DOWN_TO_Z1		100	/* Once enter Z2/Z3/Z4, keep cooling until lower than T_DOWN_TO_Z1 to back to Zone 1 */
#endif

#define ZONE4_PD_TIME		10	/* the power down seconds of zone 4 */
#define CHECK_INTERVAL		3	/* temperature check interval */

#define SET_CPU_FREQ_500M	0x1005
#define SET_CPU_FREQ_800M	0x1008
#define SET_CPU_FREQ_1000M	0x1010
#define GET_PMIC_VOLT		0x1101
#define SET_PMIC_VOLT		0x1102
#define SET_EPLL_DIV_BY_2	0x1202
#define SET_EPLL_DIV_BY_4	0x1204
#define SET_EPLL_DIV_BY_8	0x1208
#define SET_EPLL_RESTORE	0x120F
#define SET_SYS_SPD_LOW		0x1301
#define SET_SYS_SPD_RESTORE	0x1302

/* valid PMIC voltages are:
 *	1.00V, 1.10V, 1.15V, 1.20V, 1.25V, 1.29V, 1.30V
 */
#define PMIC_VOLT_NORMAL	125 /* means 1.25V */ 
#define PMIC_VOLT_LS		110 /* means 1.25V */ 

#define RTC_TICK_ON		_IO('p', 0x03)  /* Update int. enable on        */
#define RTC_TICK_OFF		_IO('p', 0x04)  /* ... off                      */
#define RTC_TICK_SET		_IO('p', 0x05)  /* Periodic int. enable on      */
#define RTC_TICK_READ		_IO('p', 0x06)  /* ... off                      */

enum {
	STATE_ZONE1 = 1,
	STATE_ZONE2,
	STATE_ZONE3,
	STATE_ZONE4,
};

volatile int _zone_state;

void alarm_func()
{
	printf("\n\nSystem wake up! \n");
}


/*
*setup_alarm():set up the sec,func
*/
void setup_alarm(int fd, int sec, void(*func)())
{
        int i,retval;
        unsigned long tmp, data;
        struct rtc_time rtc_tm;
        int scale_type = 1; /* HR24 */
        /*read current time*/

	rtc_tm.tm_year = 2021 - 1900;
        rtc_tm.tm_mon = 12 - 1;
        rtc_tm.tm_mday = 1;
        rtc_tm.tm_hour = 12;
        rtc_tm.tm_min = 0;
        rtc_tm.tm_sec = 0;

        retval = ioctl(fd, RTC_SET_TIME, &rtc_tm);
        if (retval <0) {
                printf("ioctl RTC_SET_TIME  faild!!!\n");
                return ;
        }

        rtc_tm.tm_sec = rtc_tm.tm_sec+sec;//alarm to 5 sec in the future
        if (rtc_tm.tm_sec>60) {
                rtc_tm.tm_sec=rtc_tm.tm_sec-60;
                rtc_tm.tm_min=rtc_tm.tm_min+1;
        }
        //rtc_alarmtm.enabled=0;
        //rtc_alarmtm.time=rtc_tm;

        /* Set the alarm to 5 sec in the future */

        retval = ioctl(fd, RTC_ALM_SET, &rtc_tm);
        if (retval <0) {
                printf("ioctl RTC_ALM_SET  faild!!!\n");
                return ;
        }

        /* Read the current alarm settings */

        retval = ioctl(fd, RTC_ALM_READ, &rtc_tm);
        if (retval <0) {
                printf("ioctl  RTC_ALM_READ faild!!!\n");
                return ;
        }

        /* Enable alarm interrupts */

        retval = ioctl(fd, RTC_AIE_ON, 0);
        if (retval <0) {
                printf("ioctl  RTC_AIE_ON faild!!!\n");
                return ;
        }

        fprintf(stderr, "System will wakeup after %d seconds ...\n\n",sec);

        // Enter Power Down
	system("echo mem > /sys/power/state");

        /* This blocks until the alarm ring causes an interrupt */
        retval = read(fd, &data, sizeof(unsigned long));
        if (retval >0) {
                func();
        } else {
                printf("!!!alarm faild!!!\n");
                return ;
        }
        /* Disable alarm interrupts */
        retval = ioctl(fd, RTC_AIE_OFF, 0);
        if (retval == -1) {
                printf("ioctl RTC_AIE_OFF faild!!!\n");
                return ;
        }

}

void enter_zone1(int fd)
{
	if ((_zone_state == STATE_ZONE3) || (_zone_state == STATE_ZONE4)) {
		ioctl(fd, SET_SYS_SPD_RESTORE, 0);
		_zone_state = 2;
	}

	if (ioctl(fd, SET_PMIC_VOLT, PMIC_VOLT_NORMAL)) {
		printf("Failed to restore PMIC voltage %d.%02d V!\n", PMIC_VOLT_NORMAL / 100, PMIC_VOLT_NORMAL % 100);
	}

	if (ioctl(fd, SET_CPU_FREQ_800M, 0))
		printf("Failed to set CPU clock as 800MHz!\n");

	if (ioctl(fd, SET_EPLL_RESTORE, 0))
		printf("Failed to restore EPLL setting!\n");

	/* turn-off cooling fan (PI.4) */
	system("echo 0 > /sys/class/gpio/gpio132/value");

	_zone_state = STATE_ZONE1;
}

void enter_zone2(int fd)
{
	int volt;

	if (ioctl(fd, SET_CPU_FREQ_500M, 0))
		printf("Failed to set CPU clock as 500MHz!\n");

	if (ioctl(fd, SET_EPLL_DIV_BY_4, 0))
		printf("Failed to set EPLL as divided by 4!\n");

	volt = ioctl(fd, GET_PMIC_VOLT, 0);
	if (volt < 0) {
		printf("Failed to get PMIC voltage!\n");
	} else {
		printf("PMIC voltage is %d.%02dV. Set to %d.%02dV.\n", volt / 100, volt % 100,
			PMIC_VOLT_LS / 100, PMIC_VOLT_LS % 100);
		if (ioctl(fd, SET_PMIC_VOLT, PMIC_VOLT_LS)) {
			printf("Failed to set PMIC as 1.10V!\n");
		}
	}

	/* turn-on cooling fan (PI.4) */
	system("echo 1 > /sys/class/gpio/gpio132/value");

	_zone_state = STATE_ZONE2;
}

void enter_zone3(int fd)
{
	if (ioctl(fd, SET_SYS_SPD_LOW, 0))
		printf("Failed to disable HMI clocks!\n");

	_zone_state = STATE_ZONE3;
}

void enter_zone4(int fd_rtc)
{
	_zone_state = STATE_ZONE4;

	printf("\n Enter Power Down Mode, will wakeup after %d seconds \n", ZONE4_PD_TIME);

	setup_alarm(fd_rtc, ZONE4_PD_TIME, alarm_func);

	// Enter Power Down
	// system("echo mem > /sys/power/state");
}


/**
*@breif 	main()
*/
int main(int argc, char **argv)
{
	int fd_misctrl, fd_temp, fd_rtc;
	char temp[16];
	int rev;
	unsigned int i = 0;
	int j_temp = 60;  /* Junction temperature */
	
	_zone_state = STATE_ZONE1;

	fd_temp = open("/sys/class/thermal/thermal_zone0/temp", O_RDONLY); // O_RDONLY // O_WRONLY
	if (fd_temp < 0) {
		printf("Failed to open /sys/class/thermal/thermal_zone0/temp ! errno = %d\n", fd_temp);
		return -ENODEV;
	}

	fd_misctrl = open ("/dev/ma35_misctrl", O_RDWR);
	if (fd_misctrl < 0) {
		printf("Failed to open /dev/ma35_misctrl ! errno = %d\n", fd_misctrl);
		return -ENODEV;
	}

	fd_rtc = open("/dev/rtc0", O_RDONLY);
	if (fd_rtc < 0) {
		printf("Failed to open /dev/rtc0 ! errno = %d\n", fd_rtc);
		return -ENODEV;
	}

	system("echo 132 > /sys/class/gpio/export");
	system("echo out > /sys/class/gpio/gpio132/direction");

	printf("\n");

	while(1)
	{
		memset(temp, 0, sizeof(temp));
		lseek(fd_temp, 0, 0);
		rev = read(fd_temp, temp, 6);

		j_temp = atoi(temp);

		printf("State %d, Temperature = %d \n", _zone_state, j_temp);

		switch (_zone_state) {
		case STATE_ZONE1:
			if (j_temp >= T_Z1_TO_Z2)
				enter_zone2(fd_misctrl);
			break;

		case STATE_ZONE2:
			if (j_temp >= T_Z2_TO_Z3) {
				enter_zone3(fd_misctrl);
			} else if (j_temp < T_DOWN_TO_Z1) {
				enter_zone1(fd_misctrl);
			}
			break;

		case STATE_ZONE3:
			if (j_temp >= T_Z3_TO_Z4) {
				enter_zone4(fd_rtc);
			} else if (j_temp < T_DOWN_TO_Z1) {
				enter_zone1(fd_misctrl);
			}
			break;

		case STATE_ZONE4:
			if (j_temp < T_DOWN_TO_Z1) {
				enter_zone1(fd_misctrl);
			} else {
				enter_zone4(fd_rtc);
			}
			break;
		}
		sleep(CHECK_INTERVAL);
	}
 	return 0;
}
