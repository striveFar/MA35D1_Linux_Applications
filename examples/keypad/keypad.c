/* Keybad Driver Test/Example Program
 *
 * Compile with:
 *  gcc -s -Wall -Wstrict-prototypes keybad.c -o keybad_demo
 *
 *
 * Note :
 */

#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <linux/input.h>


#define DEVINPUT "/dev/input/event0"
#define NUM 1
int main(void)
{
	int fb = -1;
	ssize_t rb;

	struct input_event ev[NUM];
	int yalv;

	if ((fb = open(DEVINPUT, O_RDONLY | O_NDELAY)) < 0) {
		printf("\n cannot open %s\n", DEVINPUT);
		return 0;
    }

    printf("\n KPI test: \n");

	 while (1) {
	 	if((rb = read(fb, ev, sizeof(struct input_event) * NUM))>0) {
	 		for (yalv = 0; yalv < (int) (rb / sizeof(struct input_event));yalv++) {
	 			switch (ev[yalv].type) {
		    		case EV_SYN:
						printf("EV_TYPE = EV_SYN \n");
					break;
		    		case EV_KEY:
			    		printf("EV_KEY : code %d value %d \n", ev[yalv].code,ev[yalv].value);
					break;
		    		case EV_LED:
						printf("EV_TYPE = EV_LED\n");
					break;
		    		case EV_REP:
						printf("EV_TYPE = EV_REP\n");
					break;
					case EV_MSC:
						printf("EV_MSC: code %d value %d \n", ev[yalv].code,ev[yalv].value);
					break;
		    		default:
						printf("EV_TYPE = %x\n", ev[yalv].type);
					break;
		    	}
	 		}
	 	}
	}
	
	printf("\n KPI test: end \n");

	return 0;
}
