#ifndef _AMFC_H_
#define _AMFC_H_


#define ALGORITHM_ZSTD			1
#define ALGORITHM_LZ4			2
#define ALGORITHM_LZ4HC			3
#define ALGORITHM_GZIP			4
#define ALGORITHM_ZLIB			5
#define ALGORITHM_DEFLAT		6

#define CMD_COMPRESS			1
#define CMD_DECOMPRESS			0

#define STATUS_IDLE			0
#define STATUS_BUSY			1
#define STATUS_MASK			1

#define IRQ_MASK			(0x03 << 24)
#define IRQ_DONE			(0x01 << 24)
#define IRQ_ERR				(0x02 << 24)

#ifndef PAGE_SIZE
#define PAGE_SIZE			4096
#endif
#define PAGE_MASK			(~(PAGE_SIZE - 1))
#define PAGE_SHIFT			12

#define ADDR_SHIFT			5

#define ZSTD_TAG			0xfd2fb528

/* error code */
#define AMFC_CMD0_ERR_SRC_SIZE0			0x01
#define AMFC_CMD0_ERR_DST_SIZE0			0x02
#define AMFC_CMD0_ERR_OWNER0			0x03
#define AMFC_CMD0_ERR_ALG			0x04

#define AMFC_CMD1_ERR_SRC_SIZE0			0x09
#define AMFC_CMD1_ERR_DST_SIZE0			0x0a
#define AMFC_CMD1_ERR_OWNER0			0x0b
#define AMFC_CMD1_ERR_ALG			0x0c

#define AMFC_ENC_SRC_PAGE_ERR			0x40
#define AMFC_ENC_DST_PAGE_ERR			0x41
#define AMFC_ENC_DST_SIZE_OVF			0x42

#define AMFC_DEC_SRC_PAGE_ERR			0x80
#define AMFC_DEC_DST_PAGE_ERR			0x81
#define AMFC_DEC_DST_SIZE_OVF			0x82

#define ZSTD_DERR_MAIN_UNDFLOW			0x87
#define ZSTD_DERR_TRI_TYPE_UNDFLOW		0x88
#define ZSTD_DERR_FSEH_LL_NCNT_UNDERFLOW	0x89
#define ZSTD_DERR_FSEH_OF_NCNT_UNDERFLOW	0x8a
#define ZSTD_DERR_FSEH_ML_NCNT_UNDERFLOW	0x8b
#define ZSTD_DERR_FSEH_HUFH_NCNT_UNDERFLOW	0x8c
#define ZSTD_DERR_FSEB_TRI_UNDFLOW		0x8d
#define ZSTD_DERR_HUFB0_UNDFLOW			0x8e
#define ZSTD_DERR_HUFB1_UNDFLOW			0x8f
#define ZSTD_DERR_HUFB2_UNDFLOW			0x90
#define ZSTD_DERR_HUFB3_UNDFLOW			0x91
#define ZSTD_DERR_FRMH_NOT_FOUND		0x92
#define ZSTD_DERR_FRMH_MAGIC_ID			0x93
#define ZSTD_DERR_FRMH_DICT_ID			0x94
#define ZSTD_DERR_BLKH_TYPE			0x95
#define ZSTD_DERR_FSEH_TLOG_HUFH		0x96
#define ZSTD_DERR_FSEH_TLOG_ML			0x97
#define ZSTD_DERR_FSEH_TLOG_OF			0x98
#define ZSTD_DERR_FSEH_TLOG_LL			0x99
#define ZSTD_DERR_FSEH_MAX_HUFH			0x9a
#define ZSTD_DERR_FSEH_MAX_ML			0x9b
#define ZSTD_DERR_FSEH_MAX_OF			0x9c
#define ZSTD_DERR_FSEH_MAX_LL			0x9d
#define ZSTD_DERR_FSEB_HUF_MAXCNT		0x9e
#define ZSTD_DERR_HUFB0_CODE			0x9f
#define ZSTD_DERR_HUFB1_CODE			0xa0
#define ZSTD_DERR_HUFB2_CODE			0xa1
#define ZSTD_DERR_HUFB3_CODE			0xa2
#define ZSTD_DERR_FSEB_OF_BIG			0xa3
#define ZSTD_DERR_FSEB_OF_OOB			0xa4

#define AMFC_ERROR_MASK				(0xff << 8)

struct amfc_cmd_list {
	unsigned int src_addr;
	unsigned int dst_addr;
	unsigned int link_addr;
	unsigned int src_size;
	unsigned int dst_size;
	union {
		unsigned int control;
		struct {
			unsigned src_hash    : 8;
			unsigned algorithm   : 4;
			unsigned rsved       : 4;
			unsigned dst_addr_h  : 4;
			unsigned src_addr_h  : 4;
			unsigned end         : 1;
			unsigned dst_scatter : 1;
			unsigned src_scatter : 1;
			unsigned link_mode   : 1;
			unsigned stream      : 1;
			unsigned hash        : 1;
			unsigned compress    : 1;
			unsigned owner       : 1;
		};
	};
	unsigned int status;
	unsigned int result_size;
};

int amfc_init(void);
int amfc_decompress(void *src, void *dst, ssize_t src_size,
		    ssize_t dst_size, int stream);

#endif
