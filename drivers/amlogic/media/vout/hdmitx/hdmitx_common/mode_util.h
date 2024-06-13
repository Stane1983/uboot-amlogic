/*
 * Copyright (c) 2022 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef MESON_MODE_POLICY_UTIL_H
#define MESON_MODE_POLICY_UTIL_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

/*
 * Macro define
 */
#define MESON_MODE_LEN        64
#define MESON_DV_MODE_LEN     265
#define MESON_MAX_STR_LEN     4096

#define DEBUG

#ifdef DEBUG

#ifndef __UBOOT__
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <poll.h>
#include <time.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LINUX_COMPILE
/*
 * Android
 */
#include <log/log.h>

#ifndef LOG_TAG
#define LOG_TAG "MesonHwc"
#endif

#define SYS_LOGV(fmt,...)     ALOGV(fmt, ##__VA_ARGS__)
#define SYS_LOGD(fmt,...)     ALOGD(fmt, ##__VA_ARGS__)
#define SYS_LOGI(fmt,...)     ALOGI("[%s, %s, %d] " fmt, strrchr(__FILE__, '/'), __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define SYS_LOGW(fmt,...)     ALOGW(fmt, ##__VA_ARGS__)
#define SYS_LOGE(fmt,...)     ALOGE(fmt, ##__VA_ARGS__)
#define SYS_ASSERT(condition,fmt,...) \
        if (!(condition)) { \
            ALOGE(fmt, ##__VA_ARGS__); \
            abort(); \
        }

#else
/*
 * Linux
 */

#ifndef LOG_TAG
#define LOG_TAG "Westeros"
#endif

#define SYS_LOGV(fmt,...)       fprintf(stderr, fmt "\n", ##__VA_ARGS__)
#define SYS_LOGD(fmt,...)       fprintf(stderr, fmt "\n", ##__VA_ARGS__)
#define SYS_LOGI(fmt,...)       fprintf(stderr, "[%s, %s, %d] " fmt "\n", strrchr(__FILE__, '/'), __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define SYS_LOGW(fmt,...)       fprintf(stderr, fmt "\n", ##__VA_ARGS__)
#define SYS_LOGE(fmt,...)       fprintf(stderr, fmt "\n", ##__VA_ARGS__)
#define SYS_ASSERT(condition,fmt,...) \
        if (!(condition)) { \
            fprintf(stderr, fmt, ##__VA_ARGS__); \
            abort(); \
        }

#ifndef __unused
#define __unused __attribute__((__unused__))
#endif

#include <linux/amlogic/drm/meson_drm.h>
#endif

int32_t meson_mode_write_sys(const char *path, const char *val);
int32_t meson_mode_read_sys(const char *path, char *val, bool original, int valSize);
bool meson_write_valid_mode_sys(const char *path, const char *outputmode);

#ifdef __cplusplus
}
#endif

#else
/*
 * Uboot
 */
#ifdef CONFIG_AML_HDMITX20
#include <amlogic/media/vout/hdmitx/hdmitx.h>
#else
#include <amlogic/media/vout/hdmitx21/hdmitx.h>
#endif
#include <linux/kernel.h>

#define SYS_LOGV(fmt,...)       printf(fmt, ##__VA_ARGS__)
#define SYS_LOGD(fmt,...)       printf(fmt, ##__VA_ARGS__)
#define SYS_LOGI(fmt,...)       printf("[%s, %s, %d] " fmt, strrchr(__FILE__, '/'), __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define SYS_LOGW(fmt,...)       printf(fmt, ##__VA_ARGS__)
#define SYS_LOGE(fmt,...)       printf(fmt, ##__VA_ARGS__)

#define __unused __always_unused

#endif

#else
#define SYS_LOGV(fmt,...)       ((void)0)
#define SYS_LOGD(fmt,...)       ((void)0)
#define SYS_LOGI(fmt,...)       ((void)0)
#define SYS_LOGW(fmt,...)       ((void)0)

#endif //DEBUG

#endif // MESON_MODE_POLICY_UTIL_H
