/****************************************************************************
 *                                                                          *
 * Copyright (c) 2014 Nuvoton Technology Corp. All rights reserved.         *
 *                                                                          *
 ****************************************************************************/
 
/****************************************************************************
 * 
 * FILENAME
 *     thermal_demo.c
 *
 * VERSION
 *     1.0
 *
 * DESCRIPTION
 *     This is the test program used to test the CPU cooling on MA35D1 EV board
 *
 * DATA STRUCTURES
 *     None
 *
 * FUNCTIONS
 *     None
 *
 * HISTORY
 *
 *
 * REMARK
 *     None
 ****************************************************************************/
#include     <stdio.h>
#include     <stdlib.h>
#include     <unistd.h> 
#include     <sys/types.h>
#include     <sys/stat.h>
#include     <fcntl.h>
#include     <termios.h> 
#include     <errno.h>
#include     <string.h>
#include     <signal.h>
#include     <pthread.h>
#include     <math.h>
#include     <sys/ioctl.h>
#include     <linux/rtc.h>

#define CPU_FREQUENCY_700M 0x1001  // CPU  700MHz
#define CPU_FREQUENCY_500M 0x1002  // CPU  500MHz
#define CPU_FREQUENCY_1000M 0x1003 // CPU 1000MHz

#define RTC_TICK_ON      _IO('p', 0x03)  /* Update int. enable on        */
#define RTC_TICK_OFF     _IO('p', 0x04)  /* ... off                      */
#define RTC_TICK_SET     _IO('p', 0x05)  /* Periodic int. enable on      */
#define RTC_TICK_READ    _IO('p', 0x06)  /* ... off                      */

#define HR24 1

void alarm_func()
{
	printf("\n\nSystem wake up! \n");
}


/*
*setup_alarm():set up the sec,func
*/
void setup_alarm(int fd,int sec,void(*func)())
{

        int i,retval;
        unsigned long tmp, data;
        struct rtc_time rtc_tm;
        int scale_type = HR24;
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
        fflush(stderr);

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


/**
*@breif 	main()
*/
int main(int argc, char **argv)
{
	int fd_cooling, fd_temp, fd_rtc;
	char temp[10];
	int rev;
	unsigned int mode = COOLING_MODE_3;
	unsigned int i = 0;

	char temp_1 = 0;

	fd_temp = open("/sys/class/thermal/thermal_zone0/temp", O_RDONLY); // O_RDONLY // O_WRONLY

	fd_cooling = open ("/dev/misctrl_tsen", O_RDWR);

	fd_rtc = open("/dev/rtc0", O_RDONLY);

	printf("\n");

	while(1)
	{
		temp[0] = 0;
		temp[1] = 0;
		rev = 0;

		lseek(fd_temp, 0, 0);
		rev = read(fd_temp, &temp[0], 2);

		temp_1 = ((temp[0]-0x30)*10) + (temp[1]-0x30);

		printf("\n temperature = %d \n", temp_1);

		sleep(1);

		if((temp_1 > 10) && (mode == CPU_FREQUENCY_1000M))
		{
			mode = CPU_FREQUENCY_700M;
			ioctl(fd_cooling, CPU_FREQUENCY_700M, 0);			
		}
		else if((temp_1 > 20) && (mode == CPU_FREQUENCY_700M))
		{
			mode = CPU_FREQUENCY_500M;
			ioctl(fd_cooling, CPU_FREQUENCY_500M, 0);
		}
		else if((temp_1 > 30) && (mode == CPU_FREQUENCY_500M))
		{
			printf("\n Enter Power Down Mode \n");

			setup_alarm(fd_rtc, 3, alarm_func);

			// Enter Power Down
			system("echo mem > /sys/power/state");

			// CPU set 1000MHz
			mode = CPU_FREQUENCY_1000M;
			ioctl(fd_cooling, CPU_FREQUENCY_1000M, 0);
		}

		sleep(2);
	}

 	return 0;
}
