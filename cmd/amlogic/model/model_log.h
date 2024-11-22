/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef MODEL_LOG_H
#define MODEL_LOG_H

#define MODEL_LOG_SILENT              (0)
#define MODEL_LOG_ERROR               (1)
#define MODEL_LOG_WARN                (2)
#define MODEL_LOG_INFO                (3)
#define MODEL_LOG_DEBUG               (4)
#define MODEL_LOG_ALL                 (5)

#define MODEL_LOG_DEFAULT             MODEL_LOG_INFO

void model_set_log_level(int log_level);
int model_get_log_level(void);

#if LOG_NDEBUG == 1
#define ALOGD(...)
#define ALOGE(...)
#define ALOGI(...)
#define ALOGW(...)

#else
#define __ini_log_print(prio, tag, fmt, args...) \
	do { \
		if ((prio) <= model_get_log_level()) { \
			if ((prio) == MODEL_LOG_DEBUG) { \
				printf("D/ %s:  ", tag); \
			} else if ((prio) == MODEL_LOG_ERROR) { \
				printf("E/ %s:  ", tag); \
			} else if ((prio) == MODEL_LOG_INFO) { \
				printf("I/ %s:  ", tag); \
			} else if ((prio) == MODEL_LOG_WARN) { \
				printf("W/ %s:  ", tag); \
			} else { \
				printf("V/ %s:  ", tag); \
			} \
			printf(fmt, ##args); \
		} \
	} while (0)

#define ALOGD(...) __ini_log_print(MODEL_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __ini_log_print(MODEL_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define ALOGI(...) __ini_log_print(MODEL_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGW(...) __ini_log_print(MODEL_LOG_WARN, LOG_TAG, __VA_ARGS__)

#endif

#endif
