// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <amlogic/rvc_interface.h>
#include <linux/ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <asm/arch/io.h>
#include <asm/byteorder.h>
#include <command.h>
#include <common.h>
#include <config.h>
#include <environment.h>
#include <ext4fs.h>
#include <fs.h>
#include <linux/ctype.h>
#include <malloc.h>
#include <partition_table.h>
#include <stdlib.h>
#include <string.h>
#include <u-boot/sha1.h>

/*basic transform char *function*/
double atodouble(const char *s)
{
	register const char *p = s;

	register long double value = 0.L;

	int sign = 0;

	long double factor;
	unsigned int expo;

	while (isspace(*p))
		p++;

	if (*p == '-' || *p == '+')
		sign = *p++;

	while ((unsigned int)(*p - '0') < 10u)
		value = value * 10 + (*p++ - '0');

	if (*p == '.') {
		factor = 1.;
		p++;
		while ((unsigned int)(*p - '0') < 10u)

		{
			factor *= 0.1;

			value += (*p++ - '0') * factor;
		}
	}

	if ((*p | 32) == 'e') {
		expo = 0;
		factor = 10.L;

		switch (*++p) {
		case '-':

			factor = 0.1;
			break;
		case '+':

			p++;

			break;

		case '0':

		case '1':

		case '2':

		case '3':

		case '4':

		case '5':

		case '6':

		case '7':

		case '8':

		case '9':

			break;

		default:

			value = 0.L;
			p = s;
			goto done;
		}

		while ((unsigned int)(*p - '0') < 10u)
			expo = 10 * expo + (*p++ - '0');

		while (1) {
			if (expo & 1)
				value *= factor;

			if ((expo >>= 1) == 0)
				break;

			factor *= factor;
		}
	}

done:
	return (sign == '-' ? -value : value);
}

int atoint(const char *nptr)
{
	int c; /* current char */
	int total; /* current total */
	int sign; /* if '-', then negative, otherwise positive */

	/* skip whitespace */
	while (isspace((int)(unsigned char)*nptr))
		++nptr;

	c = (int)(unsigned char)*nptr++;
	sign = c; /* save sign indication */
	if (c == '-' || c == '+')
		c = (int)(unsigned char)*nptr++; /* skip sign */

	total = 0;

	while (isdigit(c)) {
		total = 10 * total + (c - '0'); /* accumulate digit */
		c = (int)(unsigned char)*nptr++; /* get next char */
	}

	if (sign == '-')
		return -total;
	else
		return total; /* return result, negated if necessary */
}

bool atobool(const char *nptr)
{
	if (strcmp(nptr, "true") == 0)
		return true;
	else
		return false;
}

int64_t atoint64(const char *p)
{
	int64_t n;
	int c, neg = 0;
	unsigned char *up = (unsigned char *)p;
	bool isd = isdigit(c = *up);

	if (!isd) {
		while (isspace(c))
			c = *++up;

		if (c == '-') {
			neg++;
			c = *++up;
		} else if (c == '+') {
			c = *++up;
		}

		isd = isdigit(c);
		if (!isd)
			return (0);
	}

	for (n = '0' - c; isdigit(c = *++up);) {
		n *= 10; /* two steps to avoid unnecessary overflow */
		n += '0' - c; /* accum neg to avoid surprises at MAX */
	}

	return (neg ? n : -n);
}

/* Case insensitive string compare */
static int strcmpci(const char *a, const char *b)
{
	for (;;) {
		int d = tolower(*a) - tolower(*b);

		if (d != 0 || !*a)
			return d;
		a++, b++;
	}
}

/* Returns the next string in the split data */
static char *next(ini_t *ini, char *p)
{
	p += strlen(p);
	while (p < ini->end && *p == '\0')
		p++;
	return p;
}

static void trim_back(ini_t *ini, char *p)
{
	while (p >= ini->data && (*p == ' ' || *p == '\t' || *p == '\r'))
		*p-- = '\0';
}

static char *discard_line(ini_t *ini, char *p)
{
	while (p < ini->end && *p != '\n')
		*p++ = '\0';
	return p;
}

static char *unescape_quoted_value(ini_t *ini, char *p)
{
	char *q = p;

	p++;
	while (p < ini->end && *p != '"' && *p != '\r' && *p != '\n') {
		if (*p == '\\') {
			p++;
			switch (*p) {
			default:
				*q = *p;
				break;
			case 'r':
				*q = '\r';
				break;
			case 'n':
				*q = '\n';
				break;
			case 't':
				*q = '\t';
				break;
			case '\r':
			case '\n':
			case '\0':
				goto end;
			}
		} else {
			/* Handle normal char */
			*q = *p;
		}
		q++, p++;
	}
end:
	return q;
}

void split_data(ini_t *ini)
{
	char *value_start, *line_start;
	char *p = ini->data;

	while (p < ini->end) {
		switch (*p) {
		case '\r':
		case '\n':
		case '\t':
		case ' ':
			*p = '\0';
			break;
		case '\0':
			p++;
			break;

		case '[':
			p += strcspn(p, "]\n");
			*p = '\0';
			break;

		case ';':
			p = discard_line(ini, p);
			break;

		default:
			line_start = p;
			p += strcspn(p, "=\n");

			/* Is line missing a '='? */
			if (*p != '=') {
				p = discard_line(ini, line_start);
				break;
			}
			trim_back(ini, p - 1);

			/* Replace '=' and whitespace after it with '\0' */
			do {
				*p++ = '\0';
			} while (*p == ' ' || *p == '\r' || *p == '\t');

			/* Is a value after '=' missing? */
			if (*p == '\n' || *p == '\0') {
				p = discard_line(ini, line_start);
				break;
			}

			if (*p == '"') {
				/* Handle quoted string value */
				value_start = p;
				p = unescape_quoted_value(ini, p);

				/* Was the string empty? */
				if (p == value_start) {
					p = discard_line(ini, line_start);
					break;
				}

				/* Discard the rest of the line after the string value */
				p = discard_line(ini, p);
			} else {
				/* Handle normal value */
				p += strcspn(p, "\n");
				trim_back(ini, p - 1);
			}
			break;
		}
	}
}

/*get section and key value in ini*/
const char *ini_get(ini_t *ini, const char *section, const char *key)
{
	char *current_section = "";
	char *val;
	char *p = ini->data;

	if (*p == '\0')
		p = next(ini, p);

	while (p < ini->end) {
		if (*p == '[') {
			/* Handle section */
			current_section = p + 1;
		} else {
			/* Handle key */
			val = next(ini, p);
			if (!section || !strcmpci(section, current_section)) {
				if (!strcmpci(p, key))
					return val;
			}
			p = val;
		}

		p = next(ini, p);
	}

	printf("[%s] error %s--->%s did not match in file, set default to 0\n",
		__func__, section, key);
	return "0";
}

/*main function parse the ini buf to param*/
void ini_to_param(IRtosParam *p, ini_t *i_buf)
{
	if (!p || !i_buf) {
		printf("param or ini buf is null\n");
	} else {
		p->csi_com.w = atoint(ini_get(i_buf, "csi_com", "w"));
		p->csi_com.h = atoint(ini_get(i_buf, "csi_com", "h"));
		p->csi_com.bpp = atoint(ini_get(i_buf, "csi_com", "bpp"));
		p->csi_com.fmt = atoint(ini_get(i_buf, "csi_com", "fmt"));
		p->csi_com.buf_count = atoint(ini_get(i_buf, "csi_com", "buf_count"));

		p->ui_com.w = atoint(ini_get(i_buf, "ui_com", "w"));
		p->ui_com.h = atoint(ini_get(i_buf, "ui_com", "h"));
		p->ui_com.bpp = atoint(ini_get(i_buf, "ui_com", "bpp"));
		p->ui_com.fmt = atoint(ini_get(i_buf, "ui_com", "fmt"));
		p->ui_com.buf_count = atoint(ini_get(i_buf, "ui_com", "buf_count"));

		p->osd_com.w = atoint(ini_get(i_buf, "osd_com", "w"));
		p->osd_com.h = atoint(ini_get(i_buf, "osd_com", "h"));
		p->osd_com.bpp = atoint(ini_get(i_buf, "osd_com", "bpp"));
		p->osd_com.fmt = atoint(ini_get(i_buf, "osd_com", "fmt"));
		p->osd_com.buf_count = atoint(ini_get(i_buf, "osd_com", "buf_count"));

		p->ui_language = atoint(ini_get(i_buf, "ui_style", "ui_language"));
		p->ui_line_width = atoint(ini_get(i_buf, "ui_style", "ui_line_width"));
		p->ui_shadow_width = atoint(ini_get(i_buf, "ui_style", "ui_shadow_width"));
		p->ui_dynamic_line.red = atoint(ini_get(i_buf, "ui_style", "ui_dynamic_line_r"));
		p->ui_dynamic_line.green = atoint(ini_get(i_buf, "ui_style", "ui_dynamic_line_g"));
		p->ui_dynamic_line.blue = atoint(ini_get(i_buf, "ui_style", "ui_dynamic_line_b"));
		p->ui_dynamic_line.opt = atoint(ini_get(i_buf, "ui_style", "ui_dynamic_line_o"));

		p->dewarp_enable = atobool(ini_get(i_buf, "control", "dewarp_enable"));
		p->ui_scaler_enable = atobool(ini_get(i_buf, "control", "ui_scaler_enable"));
		p->gpio_simulate_enable = atobool(ini_get(i_buf, "control",
			"gpio_simulate_enable"));
		p->radar_simulate_enable = atobool(ini_get(i_buf, "control",
			"radar_simulate_enable"));
		p->dump_fps_enable = atobool(ini_get(i_buf, "control", "dump_fps_enable"));
		p->parking_gpio = atoint(ini_get(i_buf, "control", "parking_gpio"));
		p->osd_rotate_angle = atoint(ini_get(i_buf, "control", "osd_rotate_angle"));
		p->cam_num = atoint(ini_get(i_buf, "control", "cam_num"));
		p->cam_rotate = atoint(ini_get(i_buf, "control", "cam_rotate"));

		p->run_rtos_enable = atobool(ini_get(i_buf, "control", "run_rtos_enable"));
		p->autotest_enable = atobool(ini_get(i_buf, "control", "autotest_enable"));

		p->car.L = atoint(ini_get(i_buf, "car", "L"));
		p->car.D = atoint(ini_get(i_buf, "car", "D"));
		p->car.W = atoint(ini_get(i_buf, "car", "W"));
		p->car.Distance = atoint(ini_get(i_buf, "car", "Distance"));
		p->car.mScreenXBias = atoint(ini_get(i_buf, "car", "mScreenXBias"));
		p->car.mScreenYBias = atoint(ini_get(i_buf, "car", "mScreenYBias"));

		p->camera[0] = atoint64(ini_get(i_buf, "camera", "d9_0"));
		p->camera[1] = atoint64(ini_get(i_buf, "camera", "d9_1"));
		p->camera[2] = atoint64(ini_get(i_buf, "camera", "d9_2"));
		p->camera[3] = atoint64(ini_get(i_buf, "camera", "d9_3"));
		p->camera[4] = atoint64(ini_get(i_buf, "camera", "d9_4"));
		p->camera[5] = atoint64(ini_get(i_buf, "camera", "d9_5"));
		p->camera[6] = atoint64(ini_get(i_buf, "camera", "d9_6"));
		p->camera[7] = atoint64(ini_get(i_buf, "camera", "d9_7"));
		p->camera[8] = atoint64(ini_get(i_buf, "camera", "d9_8"));

		p->distortion[0] = atoint64(ini_get(i_buf, "distortion", "d5_0"));
		p->distortion[1] = atoint64(ini_get(i_buf, "distortion", "d5_1"));
		p->distortion[2] = atoint64(ini_get(i_buf, "distortion", "d5_2"));
		p->distortion[3] = atoint64(ini_get(i_buf, "distortion", "d5_3"));
		p->distortion[4] = atoint64(ini_get(i_buf, "distortion", "d5_4"));

		p->homoLVGL[0] = atoint64(ini_get(i_buf, "homoLVGL", "d9_0"));
		p->homoLVGL[1] = atoint64(ini_get(i_buf, "homoLVGL", "d9_1"));
		p->homoLVGL[2] = atoint64(ini_get(i_buf, "homoLVGL", "d9_2"));
		p->homoLVGL[3] = atoint64(ini_get(i_buf, "homoLVGL", "d9_3"));
		p->homoLVGL[4] = atoint64(ini_get(i_buf, "homoLVGL", "d9_4"));
		p->homoLVGL[5] = atoint64(ini_get(i_buf, "homoLVGL", "d9_5"));
		p->homoLVGL[6] = atoint64(ini_get(i_buf, "homoLVGL", "d9_6"));
		p->homoLVGL[7] = atoint64(ini_get(i_buf, "homoLVGL", "d9_7"));
		p->homoLVGL[8] = atoint64(ini_get(i_buf, "homoLVGL", "d9_8"));

		p->magic = atoint(ini_get(i_buf, "check", "magic"));
	}
}

void init_project_param(IRtosParam *p, enum ProjectConfig cfg)
{
	// init common setting
	p->csi_com.w = 1280;
	p->csi_com.h = 720;
	p->csi_com.buf_count = 3;
	p->csi_com.bpp = 4;
	p->csi_com.fmt = 21;

	p->ui_com.w = 1280;
	p->ui_com.h = 720;
	p->ui_com.buf_count = 3;
	p->ui_com.bpp = 4;
	p->ui_com.fmt = 5;

	p->osd_com.w = 1280;
	p->osd_com.h = 720;
	p->osd_com.buf_count = 2;
	p->osd_com.bpp = 4;
	p->osd_com.fmt = 3;

	p->homoLVGL[0] = 3000000000;
	p->homoLVGL[1] = -6222220000;
	p->homoLVGL[2] = 4480000000000;
	p->homoLVGL[3] = 0;
	p->homoLVGL[4] = -500000000;
	p->homoLVGL[5] = 2520000000000;
	p->homoLVGL[6] = 0;
	p->homoLVGL[7] = -9720000;
	p->homoLVGL[8] = 10000000000;

	p->ui_dynamic_line.blue = 0;
	p->ui_dynamic_line.green = 255;
	p->ui_dynamic_line.red = 255;
	p->ui_dynamic_line.opt = 255;

	p->car.L = 655;
	p->car.D = 384;
	p->car.W = 1298;
	p->car.Distance = 1380;
	p->car.mScreenXBias = 187;
	p->car.mScreenYBias = 30;

	p->run_rtos_enable = true;
	p->ui_scaler_enable = false;
	p->autotest_enable = false;
	p->gpio_simulate_enable = false;
	p->radar_simulate_enable = false;

	// init project setting
	switch (cfg) {
	case BE11_CAM_936:
		printf("init project config: be11_cam_936\n");
		p->dewarp_enable = false;
		p->dump_fps_enable = true;
		p->csi_com.fmt = 20;
		p->ui_language = CHINESE;
		p->ui_line_width = 40;
		p->ui_shadow_width = 0;
		p->osd_rotate_angle = 0;
		p->parking_gpio = 299;
		p->cam_num = 1;
		p->cam_rotate = 0;

		p->distortion[0] = 10000000000;
		p->distortion[1] = 10000000000;
		p->distortion[2] = 102000000000;
		p->distortion[3] = 0;
		p->distortion[4] = 0;
		break;
	case BE11_CAM_96722:
		printf("init project config: be11_cam_96722\n");
		p->dewarp_enable = true;
		p->dump_fps_enable = true;
		p->csi_com.fmt = 20;
		p->ui_language = CHINESE;
		p->ui_line_width = 40;
		p->ui_shadow_width = 0;
		p->osd_rotate_angle = 0;
		p->parking_gpio = 299;
		p->cam_num = 1;
		p->cam_rotate = 4;

		p->distortion[0] = 15000000000;
		p->distortion[1] = 15000000000;
		p->distortion[2] = 100000000000;
		p->distortion[3] = 0;
		p->distortion[4] = 0;
		break;
	default:
		printf("init project config: default\n");
		p->dewarp_enable = false;
		p->dump_fps_enable = true;

		p->ui_language = CHINESE;
		p->ui_line_width = 50;
		p->ui_shadow_width = 4;
		p->osd_rotate_angle = 3;
		p->parking_gpio = 299;
		p->cam_num = 0;
		p->cam_rotate = 0;

		p->distortion[0] = 10000000000;
		p->distortion[1] = 10000000000;
		p->distortion[2] = 102000000000;
		p->distortion[3] = 0;
		p->distortion[4] = 0;
		break;
	}
}

void print_param(IRtosParam *p)
{
	printf("[read_car_param] p->magic: %d\n", p->magic);
	printf("[read_car_param] -------------csi common---------------------\n");
	printf("[read_car_param] p->csi_com.w: %d\n", p->csi_com.w);
	printf("[read_car_param] p->csi_com.h: %d\n", p->csi_com.h);
	printf("[read_car_param] p->csi_com.bpp: %d\n", p->csi_com.bpp);
	printf("[read_car_param] p->csi_com.fmt: %d\n", p->csi_com.fmt);
	printf("[read_car_param] p->csi_com.buf_count: %d\n", p->csi_com.buf_count);

	printf("[read_car_param] ---------------ui_common-------------------\n");
	printf("[read_car_param] p->ui_com.w: %d\n", p->ui_com.w);
	printf("[read_car_param] p->ui_com.h: %d\n", p->ui_com.h);
	printf("[read_car_param] p->ui_com.bpp: %d\n", p->ui_com.bpp);
	printf("[read_car_param] p->ui_com.fmt: %d\n", p->ui_com.fmt);
	printf("[read_car_param] p->ui_com.buf_count: %d\n", p->ui_com.buf_count);

	printf("[read_car_param] ---------------osd_common-------------------\n");
	printf("[read_car_param] p->osd_com.w: %d\n", p->osd_com.w);
	printf("[read_car_param] p->osd_com.h: %d\n", p->osd_com.h);
	printf("[read_car_param] p->osd_com.bpp: %d\n", p->osd_com.bpp);
	printf("[read_car_param] p->osd_com.fmt: %d\n", p->osd_com.fmt);
	printf("[read_car_param] p->osd_com.buf_count: %d\n", p->osd_com.buf_count);

	printf("[read_car_param] ---------------ui_style-------------------\n");
	printf("[read_car_param] p->ui_language: %d\n", p->ui_language);
	printf("[read_car_param] p->ui_line_width: %d\n", p->ui_line_width);
	printf("[read_car_param] p->ui_shadow_width: %d\n", p->ui_shadow_width);
	printf("[read_car_param] p->ui_dynamic_line.red: %d\n",
		p->ui_dynamic_line.red);
	printf("[read_car_param] p->ui_dynamic_line.green: %d\n",
		p->ui_dynamic_line.green);
	printf("[read_car_param] p->ui_dynamic_line.blue: %d\n",
		p->ui_dynamic_line.blue);
	printf("[read_car_param] p->ui_dynamic_line.opt: %d\n",
		p->ui_dynamic_line.opt);

	printf("[read_car_param] ---------------control-------------------\n");
	printf("[read_car_param] p->dewarp_enable: %d\n", p->dewarp_enable);
	printf("[read_car_param] p->dump_fps_enable: %d\n", p->dump_fps_enable);
	printf("[read_car_param] p->ui_scaler_enable: %d\n", p->ui_scaler_enable);
	printf("[read_car_param] p->osd_rotate_angle: %d\n", p->osd_rotate_angle);
	printf("[read_car_param] p->gpio_simulate_enable: %d\n",
		p->gpio_simulate_enable);
	printf("[read_car_param] p->radar_simulate_enable: %d\n", p->radar_simulate_enable);
	printf("[read_car_param] p->parking_gpio: %d\n", p->parking_gpio);
	printf("[read_car_param] p->cam_num: %d\n", p->cam_num);
	printf("[read_car_param] p->cam_rotate: %d\n", p->cam_rotate);
	printf("[read_car_param] p->run_rtos_enable: %d\n", p->run_rtos_enable);
	printf("[read_car_param] p->autotest_enable: %d\n", p->autotest_enable);

	printf("[read_car_param] ---------------car-------------------\n");
	printf("[read_car_param] p->car.L: %d\n", p->car.L);
	printf("[read_car_param] p->car.D: %d\n", p->car.D);
	printf("[read_car_param] p->car.W: %d\n", p->car.W);
	printf("[read_car_param] p->car.Distance: %d\n", p->car.Distance);
	printf("[read_car_param] p->car.mScreenXBias: %d\n", p->car.mScreenXBias);
	printf("[read_car_param] p->car.mScreenYBias: %d\n", p->car.mScreenYBias);

	printf("[read_car_param] -------------camera---------------------\n");
	for (int i = 0; i < 9; i++)
		printf("[read_car_param] p->camera[%d]: %lld\n", i, p->camera[i]);

	printf("[read_car_param] --------------distortion--------------------\n");
	for (int i = 0; i < 5; i++)
		printf("[read_car_param] p->distortion[%d]: %lld\n", i, p->distortion[i]);

	printf("[read_car_param] --------------homoLVGL--------------------\n");
	for (int i = 0; i < 9; i++)
		printf("[read_car_param] p->homoLVGL[%d]: %lld\n", i, p->homoLVGL[i]);
}

int read_ini_file(ini_t *config, char *mmc_partition, char *filename)
{
	struct blk_desc *dev_desc = NULL;
	disk_partition_t info;
	int ret, part;
	loff_t actread, off = 0;
	loff_t file_size;

	part = blk_get_device_part_str("mmc", mmc_partition, &dev_desc, &info, 1);

	if (part >= 0) {
		ext4fs_set_blk_dev(dev_desc, &info);

		if (ext4fs_mount(info.size)) {
			printf("ext4fs_mount success\n");
			ext4fs_size(filename, &file_size);
		} else {
			printf("ext4fs_mount error\n");
			return -1;
		}

		if (file_size > 0) {
			printf("ext4fs_size get file size : %lld\n", file_size);
		} else {
			printf("ext4fs_size get file size failed!!!\n");
			return -1;
		}

		/*malloc memory*/
		char *buf = (char *)malloc(file_size + 1);

		if (buf) {
			memset(buf, 0, file_size + 1);

			ret = ext4fs_open(filename, &file_size);
			if (ret >= 0) {
				printf("File found %s\n", filename);
			} else {
				printf("File not found %s **\n", filename);
				free(buf);
				return -1;
			}

			ret = ext4fs_read(buf, off, file_size, &actread);
			if (ret >= 0) {
				printf("[%s] read %s size=%lld\n", __func__, filename, file_size);
				/*load ini form file buf*/
				config->data = (char *)buf;
				config->end = (char *)config->data + file_size;
				split_data(config);
				printf("[%s] done ...\n", __func__);
			} else {
				printf("ext4fs_read File error %s **\n", filename);
				free(buf);
				return -1;
			}
		} else {
			printf("buf malloc failed!!!\n");
			return -1;
		}
	} else {
		printf("blk_get_device_part_str error **\n");
		return -1;
	}

	return ret;
}
