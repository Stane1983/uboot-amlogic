/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef RVC_INTERFACE_H
#define RVC_INTERFACE_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct ini_t {
	char *data;
	char *end;
} ini_t;
/*ini file use*/

typedef struct s_Common {
	int w;
	int h;
	int bpp;
	int fmt;
	int buf_count;
} Common;
/*Rtos param struct*/

typedef struct s_Color {
	uint8_t blue;
	uint8_t green;
	uint8_t red;
	uint8_t opt;
} Color;
/*rgba color define*/
typedef struct s_CarBody {
	int L;
	int D;
	int W;
	int Distance;
	int mScreenXBias;
	int mScreenYBias;
} CarBody;

enum CountryLanguage {
	CHINESE = 0,
	ENGLISH,
	ARABIC,
	GERMAN,
	SPANISH,
	HEBREW,
	RUSSIAN,
	TURKISH
};

enum ProjectConfig {
	BE11_CAM_936,
	BE11_CAM_96722
};

typedef struct s_IRtosParam {
	uint32_t magic;

	Common csi_com;
	Common osd_com;
	Common ui_com;

	bool run_rtos_enable;
	bool dewarp_enable;
	bool ui_scaler_enable;
	bool dump_fps_enable;
	bool autotest_enable;
	bool gpio_simulate_enable;
	bool radar_simulate_enable;

	int ui_language;
	int ui_line_width;
	int ui_shadow_width;
	int osd_rotate_angle;
	int parking_gpio;
	int cam_num;
	int cam_rotate;

	Color ui_dynamic_line;
	CarBody car;

	int64_t camera[9];
	int64_t distortion[5];
	int64_t homoLVGL[9];

	// rounds up to 2048-byte.
	uint8_t reserved[1725];
	uint32_t crc32;
} IRtosParam;
/*main struct for rtos param*/

/*debug to printf all IRtosParam*/
void print_param(IRtosParam *p);
void init_project_param(IRtosParam *p, enum ProjectConfig config);

/*ini file read and get*/
void ini_to_param(IRtosParam *p, ini_t *i_buf);
const char *ini_get(ini_t *ini, const char *section, const char *key);
void split_data(ini_t *ini);

/*type convert*/
bool atobool(const char *nptr);
int atoint(const char *nptr);
double atodouble(const char *s);
int64_t atoint64(const char *nptr);

int read_ini_file(ini_t *config, char *mmc_partition, char *filename);
#endif
