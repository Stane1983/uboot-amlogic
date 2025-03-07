#include <common.h>
#include <command.h>
#include <cpu_func.h>
#include <amlogic/jpeg.h>
#include <amlogic/fb.h>

static long splash_rd_offset;
static pjpeg_image_info_t pjpeg;
static uint32_t *fb_addr;
static unsigned long fb_width;


typedef struct splash_t
{
	u32 addr;
	u32 length;
}jpeg_splash_t;

typedef unsigned char (*pjpeg_need_bytes_callback_t)(unsigned char* pBuf, unsigned char buf_size, unsigned char *pBytes_actually_read, void *pCallback_data);

unsigned char pjpeg_need_bytes_callback(unsigned char* pBuf, unsigned char buf_size, unsigned char *pBytes_actually_read, void *pCallback_data)
{
    uint n;
    jpeg_splash_t *splash = (jpeg_splash_t *)pCallback_data;

    if (splash_rd_offset >= splash->length) {
        splash_rd_offset = 0;
        return PJPG_STREAM_READ_ERROR;
    }
    n = min((unsigned int)(splash->length - splash_rd_offset), (unsigned int)buf_size);
    memcpy(pBuf, (void *)(splash->addr + splash_rd_offset), n);
    splash_rd_offset += n;
    *pBytes_actually_read = n;

    return 0;
}

int pjpeg_decode(pjpeg_image_info_t *image_info, uint32_t *pImage)
{
    int mcu_x = 0;
    int mcu_y = 0;
    uint8_t status = 0;

#ifdef PJPEG_DECODE_DBG
    printf("pjpeg_decode mcu:%dx%d size:%dx%d\n",
        image_info->m_MCUSPerRow, image_info->m_MCUSPerCol,
        image_info->m_MCUWidth, image_info->m_MCUHeight);
#endif

    for ( ; ; ) {
        int x, y;
        uint32_t *pDst_row;

        if (mcu_y >= image_info->m_MCUSPerCol)
            break;
        status = pjpeg_decode_mcu();
        if (status) {
            if (status != PJPG_NO_MORE_BLOCKS) {
                printf("pjpeg_decode_mcu() failed with status %u\n", status);
                return status;
            }
            break;
        }
        // Copy MCU's pixel blocks into the destination bitmap.
        pDst_row = pImage + mcu_y * image_info->m_MCUHeight * fb_width + mcu_x * image_info->m_MCUWidth;
        for (y = 0; y < image_info->m_MCUHeight; y += 8) {
            const int by_limit = min(8, image_info->m_height - (mcu_y * image_info->m_MCUHeight + y));

            for (x = 0; x < image_info->m_MCUWidth; x += 8) {
                // Compute source byte offset of the block in the decoder's MCU buffer.
                const uint src_ofs = (x * 8U) + (y * 16U);
                uint8_t *pSrcR = image_info->m_pMCUBufR + src_ofs;
                uint8_t *pSrcG = image_info->m_pMCUBufG + src_ofs;
                uint8_t *pSrcB = image_info->m_pMCUBufB + src_ofs;
                const int bx_limit = min(8, image_info->m_width - (mcu_x * image_info->m_MCUWidth + x));
                int bx, by;
                uint32_t *pDst_block = pDst_row + x;

                for (by = 0; by < by_limit; by++) {
                    uint32_t *pDst = pDst_block;

                    for (bx = 0; bx < bx_limit; bx++) {
                        *pDst = (0xff << 24) | (*pSrcR << 16) | (*pSrcG << 8) | (*pSrcB);
                        pSrcR++;
                        pSrcG++;
                        pSrcB++;
                        pDst++;
                    }
                    pSrcR += (8 - bx_limit);
                    pSrcG += (8 - bx_limit);
                    pSrcB += (8 - bx_limit);
                    pDst_block += image_info->m_width;
                }
            }
            pDst_row += image_info->m_width * 8;
        }
        mcu_x++;
        if (mcu_x == image_info->m_MCUSPerRow) {
            mcu_x = 0;
            mcu_y++;
        }
    }
    flush_cache((unsigned long)pImage,
    image_info->m_width * image_info->m_height * 4);
#ifdef PJPEG_DECODE_DBG
    printf("Decode completed %dx%d to %p.\n",
        image_info->m_width, image_info->m_height, pImage);
#endif
    return status;
}

jpeg_splash_t get_splash_image (void)
{
	jpeg_splash_t splash_t;

	splash_t.addr = 0;
	splash_t.length = 0;
	/*need to ask wu yihui how to load jpeg logo,get jpeg addr and length*/
	return splash_t;
}

static unsigned long env_strtoul(const char *name, int base)
{
	unsigned long ret = 0;
	char *str = NULL;

	str = env_get(name);
	if (str) {
		ret = simple_strtoul(str, NULL, base);
	}
	if (base == 16)
		printf("%s: 0x%lx\n", name, ret);
	else if (base == 10)
		printf("%s: %ld\n", name, ret);

	return ret;
}

static int jpeg_display(cmd_tbl_t *cmdtp, int flag, int argc,
		       char *const argv[])
{
	int ret;
	jpeg_splash_t splash;

	if (argc != 3) {
		printf("jpeg display parameter num error\n");
		return -1;
	}

	splash.addr = simple_strtoul(argv[1], NULL, 16);
	splash.length = simple_strtoul(argv[1], NULL, 16);
    fb_addr = (uint32_t *)get_fb_addr();
    fb_width = env_strtoul("fb_width", 10);
    if (!fb_addr || !fb_width) {
        printf("get fb error\n");
        return -1;
    }
	splash_rd_offset = 0;
	ret = pjpeg_decode_init(&pjpeg, pjpeg_need_bytes_callback, (void *)&splash);
	if (ret) {
	    printf("pjpeg_decode_init() failed with status %d.\n", ret);
	    if (ret == PJPG_UNSUPPORTED_MODE)
	        printf("Progressive JPEG files are not supported.\n");
	    return -1;
	}
	ret = pjpeg_decode(&pjpeg, (uint32_t *)fb_addr);
	if (ret) {
		printf("decode JPG image failed with status %d.\n", ret);
		return -1;
	}
	return 0;
}

static cmd_tbl_t cmd_jpeg_sub[] = {
	U_BOOT_CMD_MKENT(display, 3, 0, jpeg_display, "", ""),
};

static int do_jpeg(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	cmd_tbl_t *c;
	argc--;
	argv++;

	c = find_cmd_tbl(argv[0], &cmd_jpeg_sub[0], ARRAY_SIZE(cmd_jpeg_sub));

	if (c)
		return  c->cmd(cmdtp, flag, argc, argv);
	else
		return CMD_RET_USAGE;
}

U_BOOT_CMD(
	jpeg,	5,	0,	do_jpeg,
	"jpeg sub-system",
	"jpeg display           - jpeg decode\n"
);

