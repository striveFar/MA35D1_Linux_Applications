/*
 * V4L2 Codec decoding example application
 * Kamil Debski <k.debski@samsung.com>
 *
 * Argument parser
 *
 * Copyright 2012 Samsung Electronics Co., Ltd.
 * Copyright (c) 2015 Linaro Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/videodev2.h>

#include "common.h"
#include "parser.h"

static char _o_foramt[16] = "argb888";

void print_usage(char *name)
{
	printf("Usage:\n");
	printf("\t%s\n", name);
	printf("\t-d <device>  - Frame buffer device (e.g. /dev/fb0)\n");
	printf("\t-i <file> - Input file name\n");
	printf("\t-v <device> - video decoder device (e.g. /dev/video0)\n");
	printf("\t-x output image x axis position\n");
	printf("\t-y output image y axis position\n");
	printf("\t-w output image width\n");
	printf("\t-h output image height\n");
	printf("\t-f output format (argb888/rgb565/nv12)\n");
	printf("\t-p 1/0: enable/disable VC8000 PP\n");
	printf("\n");
}

void init_to_defaults(struct instance *i)
{
	memset(i, 0, sizeof(*i));
	i->config_only = 0;
	i->pp_enable = 1;
	i->parser.codec = V4L2_PIX_FMT_H264;
	i->parser.func = parse_h264_stream;
	i->x_pos = 0;
	i->y_pos = 0;
	i->width = 1024;
	i->height = 600;
	i->out_format = _o_foramt;
}

int parse_args(struct instance *i, int argc, char **argv)
{
	int c;

	init_to_defaults(i);

	while ((c = getopt(argc, argv, "x:y:w:h:d:i:v:f:p:")) != -1) {
		switch (c) {
		case 'd':
			i->fb.name = optarg;
			break;
		case 'i':
			i->in.name = optarg;
			break;
		case 'v':
			i->video.name = optarg;
			break;
		case 'x':
			i->x_pos = atoi(optarg);
			break;
		case 'y':
			i->y_pos = atoi(optarg);
			break;
		case 'w':
			i->width = atoi(optarg);
			break;
		case 'h':
			i->height = atoi(optarg);
			break;
		case 'f':
			i->out_format = optarg;
			break;
		case 'p':
			i->pp_enable = atoi(optarg) ? 1 : 0;
			break;
		default:
			err("Bad argument!!\n");
			return -1;
		}
	}
	
	if (i->pp_enable == 0) {
		i->config_only = 1;
		return 0;
	}

	if (!i->fb.name) {
		err("Frame buffer device name argument is required: -d\n");
		return -1;
	}
	if (!i->video.name) {
		err("VC8000 device name argument is required: -v\n");
		return -1;
	}
	if (!i->in.name) {
		i->config_only = 1;
	}
	return 0;
}

