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
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <asm/ioctl.h>
#include <sys/mman.h>
#include "nua3500-ebi.h"

#define __CH		(0)//0~2

int main(int argc, char **argv)
{
	int fd,i,j;
	unsigned char *pEbiBuffer;
	unsigned long uEbiSize;
	unsigned int patterns[4]= {0x00000000,0x55555555,0xAAAAAAAA,0xFFFFFFFF};
	struct nua3500_set_ebi ebi;
	char dev[3][14] = {"/dev/ebi0",
			"/dev/ebi1",
			"/dev/ebi2"
			};


	i = __CH;
	fd = open(&dev[i][0], O_RDWR);
	if(fd < 0)
		printf("Open ebi%d error !!!\n", i);
	
	ebi.bank = i;
	ebi.base = 0x68000000 + (i * 0x100000);
	printf("EBI Bank %d Addr: 0x%08x\n", ebi.bank, ebi.base);

	ebi.busmode = EBI_OPMODE_ADSEPARATE;
	ebi.CSActiveLevel = EBI_CS_ACTIVE_LOW;
	ebi.width = EBI_BUSWIDTH_16BIT;

	ioctl(fd, EBI_IOC_SET, &ebi);
	ioctl(fd, EBI_IOC_SPEED, &ebi);

	uEbiSize = 0x100000;
	printf("EbiSize: 0x%08x\n", uEbiSize);
	pEbiBuffer = mmap(NULL, uEbiSize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (pEbiBuffer == MAP_FAILED) {
		printf("mmap() failed\n");
		exit(0);
	}

	printf("Start to Word Read/Write test\n");
	for(i=0; i<uEbiSize; i+=4) {
		unsigned int tmp;
		for(j=0; j<4; j++) {
			*(unsigned int *)(pEbiBuffer+i)=patterns[j];
			tmp=*(unsigned int *)(pEbiBuffer+i);
			if(tmp!=patterns[j]) {
				printf("Read/Write test failed, patterns=0x%08x, data[%d]=0x%08x\n",patterns[j],i,*(unsigned int*)(pEbiBuffer+i));
				return 0;
			}
		}
	}

	printf("Compare data passed\n");
	close(fd);

	return 0;
}
