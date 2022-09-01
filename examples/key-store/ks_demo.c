// SPDX-License-Identifier: GPL-2.0
/*
 * ks_demo.c
 *
 * The sample program for MA35D1 Key Store access.
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

#include "ma35d1_ks.h"


#define KS_KEY_MAX    (KS_MAX_KEY_SIZE/32)
#define KEY_SEED      0x1A39175D


struct ks_keys {
	unsigned int   key[KS_KEY_MAX];
	int            size;
	int            number;
	unsigned int   meta;
};

struct ks_keys  _my_keys[KS_SRAM_KEY_CNT];

int   key_size_list[] = { 128, 128, 128, 256, 256, 128, 163, 192, 224,
			  233, 255, 256, 283, 384, 409, 512, 521, 571,
			  1024, 2048, 4096 };
int   key_size_meta[] = { KS_META_128, KS_META_128, KS_META_128, KS_META_256,
			  KS_META_256, KS_META_128, KS_META_163, KS_META_192,
			  KS_META_224, KS_META_233, KS_META_255, KS_META_256,
			  KS_META_283, KS_META_384, KS_META_409, KS_META_512,
			  KS_META_521, KS_META_571, KS_META_1024, KS_META_2048,
			  KS_META_4096 };

int  fd;

static void test_vector_init(void)
{
	int   i, j, k, klist_size;
	unsigned int  k32 = KEY_SEED;

	printf("Initialize test vectors...");

	klist_size = sizeof(key_size_list)/4;
	k = 0;
	for (i = 0; i < KS_SRAM_KEY_CNT; i++) {
		// printf("KEY%d: ", i);
		for (j = 0; j < 128; j++) {
			_my_keys[i].key[j] = k32;
			k32 += KEY_SEED;
			// printf("%08x ", _my_keys[i].key[j]);
		}
		//printf("\n");
		_my_keys[i].number = -1;    /* not used */
		_my_keys[i].size = key_size_list[k];
		_my_keys[i].meta = key_size_meta[k];
		k = (k+1) % klist_size;
	}
	printf("Done.\n");
}

static int ks_key_compare(unsigned int *au32KeyExpect,
			  unsigned int *au32KeyData, int key_size)
{
	int  i, j, bits;

	for (i = 0; i < key_size/32; i++) {
		if (au32KeyExpect[i] != au32KeyData[i]) {
			printf("\nKey compare mismatch at word %d\n", i);
			goto mismatch;
		}
	}
	if (key_size % 32) {
		i = (key_size/32);
		bits = key_size % 32;
		for (j = 0; j < bits; j++) {
			if ((au32KeyExpect[i] & (1 << j)) !=
				(au32KeyData[i] & (1 << j))) {
				printf("\nKey compare mismatch at bit %d\n",
					i * 32 + j);
				goto mismatch;
			}
		}
	}
	return 0;

mismatch:
	printf("Key Expect: ");
	for (i = 0; i < (key_size+31)/32; i++)
		printf("%08x ", au32KeyExpect[i]);
	printf("\n");

	printf("Key Data: ");
	for (i = 0; i < (key_size+31)/32; i++)
		printf("%08x ", au32KeyData[i]);
	return -1;
}

static int  test_vector_verify_sram_keys(void)
{
	struct ks_read_args   r_args;
	int    i, ret = 0;

	printf("[Verify KS_SRAM keys]\n");
	for (i = 0; i < KS_SRAM_KEY_CNT; i++) {
		printf("<%d> Read SRAM key %d size %d....\n", i,
				_my_keys[i].number, _my_keys[i].size);
		r_args.type = KS_SRAM;
		r_args.key_idx = _my_keys[i].number;
		r_args.word_cnt = (_my_keys[i].size+31)/32;
		ret = ioctl(fd, KS_IOCTL_READ, &r_args);
		if (ret != 0) {
			printf("[FAIL]\nRead SRAM key fail! %d\n", ret);
			return -1;
		}

		if (ks_key_compare(_my_keys[i].key, r_args.key,
					_my_keys[i].size) != 0)
			return -1;

		printf("[PASS]\n");
	}

	return 0;
}

static int  test_vector_write_all_keys(unsigned int key_meta)
{
	struct ks_read_args	r_args;
	struct ks_write_args	w_args;
	struct ks_kidx_args	k_args;
	int		i32KeyIdx;
	int		i, j, ret, err;

	printf("+-----------------------------------------+\n");
	printf("|  Write all keys to KS SRAM              |\n");
	printf("+-----------------------------------------+\n");

	ret = ioctl(fd, KS_IOCTL_ERASE_ALL, 0);
	if (ret != 0) {
		printf("KS_IOCTL_ERASE_ALL failed: %d!\n", ret);
		return -1;
	}

	printf("[Write keys to KS_SRAM]\n");
	for (i = 0; i < KS_SRAM_KEY_CNT; i++) {
		_my_keys[i].meta |= key_meta;
		printf("Write key size %d, meta=0x%x....", _my_keys[i].size,
							_my_keys[i].meta);
		w_args.meta_data = _my_keys[i].meta;
		memcpy(w_args.key, _my_keys[i].key, KS_MAX_KEY_SIZE/8);
		ret = ioctl(fd, KS_IOCTL_WRITE_SRAM, &w_args);
		if (ret < 0) {
			printf("KS_Write failed! %d\n", ret);
			goto lexit;
		} else {
			_my_keys[i].number = ret;
			printf("[PASS] number=%d\n", _my_keys[i].number);
		}

		ret = ioctl(fd, KS_IOCTL_GET_REMAIN, 0);
		printf("SRAM key remains: %d\n", ret);

	}
	err = test_vector_verify_sram_keys();

lexit:
	if (err == 0)
		printf("\nTest PASSED!\n");
	else
		printf("\nTest FAILED!\n");
	return err;
}

static int dump_otp()
{
	struct ks_read_args  r_args;
	int		i, j, ret;

	printf("+-----------------------------------------+\n");
	printf("|  DUMP OTP                               |\n");
	printf("+-----------------------------------------+\n");

	/*
	 *  Power-on Setting: 0x100~0x103
	 */
	r_args.key_idx = 0x100;
	r_args.word_cnt = 1;
	ret = ioctl(fd, KS_IOCTL_READ_OTP, &r_args);
	if (ret != 0)
		goto err_out;
	printf("Power-on Setting = 0x%x\n", r_args.key[0]);

	/*
	 *  DPM Setting: 0x104~0x107
	 */
	r_args.key_idx = 0x104;
	r_args.word_cnt = 1;
	ret = ioctl(fd, KS_IOCTL_READ_OTP, &r_args);
	if (ret != 0)
		goto err_out;
	printf("DPM Setting = 0x%x\n", r_args.key[0]);

	/*
	 *  PLM Setting: 0x108~0x10B
	 */
	r_args.key_idx = 0x108;
	r_args.word_cnt = 1;
	ret = ioctl(fd, KS_IOCTL_READ_OTP, &r_args);
	if (ret != 0)
		goto err_out;
	printf("PLM Setting = 0x%x\n", r_args.key[0]);

	/*
	 *  MAC0 Address: 0x10C~0x113
	 */
	r_args.key_idx = 0x10C;
	r_args.word_cnt = 2;
	ret = ioctl(fd, KS_IOCTL_READ_OTP, &r_args);
	if (ret != 0)
		goto err_out;
	printf("MAC0 address = 0x%08x, 0x%08x\n", r_args.key[0], r_args.key[1]);

	/*
	 *  MAC1 Address: 0x114~0x11B
	 */
	r_args.key_idx = 0x114;
	r_args.word_cnt = 2;
	ret = ioctl(fd, KS_IOCTL_READ_OTP, &r_args);
	if (ret != 0)
		goto err_out;
	printf("MAC1 address = 0x%08x, 0x%08x\n", r_args.key[0], r_args.key[1]);

	/*
	 *  Secure Region: 0x120~0x177
	 */
	r_args.key_idx = 0x120;
	r_args.word_cnt = 22;
	memset(r_args.key, 0, 88);
	ret = ioctl(fd, KS_IOCTL_READ_OTP, &r_args);
	if (ret != 0)
		goto err_out;
	printf("Secure Region = ");
	for (i = 0; i < 22; i++)
		printf("%08x ", r_args.key[i]);
	printf("\n");

	/*
	 *  Non-secure Region: 0x178~0x1CF
	 */
	r_args.key_idx = 0x178;
	r_args.word_cnt = 22;
	memset(r_args.key, 0, 88);
	ret = ioctl(fd, KS_IOCTL_READ_OTP, &r_args);
	if (ret != 0)
		goto err_out;
	printf("Non-secure Region = ");
	for (i = 0; i < 22; i++)
		printf("%08x ", r_args.key[i]);
	printf("\n");

	printf("+-----------------------------------------+\n");
	printf("|  DUMP Key Store OTP Keys                |\n");
	printf("+-----------------------------------------+\n");

	/*
	 *  Hardware Unique Key 0 (HUK): Key Store OTP Key 0
	 */
	for (i = 0; i < 3; i++) {
		r_args.type = KS_OTP;
		r_args.key_idx = i;
		r_args.word_cnt = 4;
		ret = ioctl(fd, KS_IOCTL_READ, &r_args);
		if (ret != 0)
			goto err_out2;
		printf("HUK%d = ", i);
		for (j = 0; j < 4; j++)
			printf("%08x ", r_args.key[j]);
		printf("\n");
	}

	for (i = 3; i < 9; i++) {
		r_args.type = KS_OTP;
		r_args.key_idx = i;
		r_args.word_cnt = 8;
		ret = ioctl(fd, KS_IOCTL_READ, &r_args);
		if (ret != 0)
			goto err_out2;
		printf("Key Store OTP Key %d = ", i);
		for (j = 0; j < 8; j++)
			printf("%08x ", r_args.key[j]);
		printf("\n");
	}
	return 0;

err_out:
	printf("Read OTP address 0x%x failed, ret=0x%x\n", r_args.key_idx, ret);
	return ret;

err_out2:
	printf("Read Key Store OTP Key %d failed, ret=0x%x\n", r_args.key_idx, ret);
	return ret;
}


int main(void)
{
	fd = open("/dev/ksdev", O_RDWR);
	if (fd < 0) {
		printf("open ks_dev faild!!!\n");
		return -1;
	}
	test_vector_init();
	test_vector_write_all_keys(KS_META_CPU | KS_META_READABLE);
	dump_otp();
	close(fd);
	return 0;
} /* end main */

