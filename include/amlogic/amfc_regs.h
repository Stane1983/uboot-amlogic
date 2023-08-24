#ifndef _AMFC_REGS_H_
#define _AMFC_REGS_H_

#define CLKCTRL_AMFC_CLK_CTRL                      ((0x0065  << 2) + 0xfe000000)

//=======================================================================
// AMFC
// -----------------------------------------------
// REG_BASE:  REGISTER_BASE_ADDR = 0xfe024000
// -----------------------------------------------
#define AMFC_GL_VERSION                            ((0x0000  << 2) + 0xfe024000)
//Bit 31:16        reserved
//Bit 15:8         ro_major_version                   //unsigned , RO,  a 8 bit value to show VLSI major version
//Bit  7:0         ro_minor_version                   //unsigned , RO,  a 8 bit value to show VLSI minor version
#define AMFC_GL_MISC                               ((0x0001  << 2) + 0xfe024000)
//Bit 31:1         reserved
//Bit  0           reg_cmds_split_mode               //unsigned , RW, default = 1 0: both cmd0 and cmd1 can work as enc,dec,or enc/dec. 1: cmd0 only for enc and cmd1 for dec
#define AMFC_GL_CMD0_DESC_BASE0_ADDR               ((0x0002  << 2) + 0xfe024000)
//Bit 31: 0        reg_cmd0_desc_base0_addr           // unsigned ,RW, default = 0  base address of descriptor list; Physical address = DESCRIPTOR BASE ADDDR<<6
#define AMFC_GL_CMD0_CURR_DESC_ADDR                ((0x0003  << 2) + 0xfe024000)
//Bit 31: 0        ro_cmd0_cur_desc_addr             // unsigned ,RO, default = 0  address of current processing descriptor
#define AMFC_GL_CMD0_CONTROL                       ((0x0004  << 2) + 0xfe024000)
//Bit 31           reg_cmd0_sw_rst                   // unsigned ,    RW, default = 0  reset AMFC state machine
//Bit 30: 5        reserved
//Bit 4            reg_cmd0_global_irq_en            // unsigned ,    RW, default = 0  1 to enable interrupt
//Bit 3: 2         reg_cmd0_64bit_os_mode            // unsigned ,    RW, default = 2  0: 32bit os mode,src/dst buffer address in descriptor list is [33:2]; 1: 64bit os mode, [34:3], 2/3: [37:6]
//Bit 1            reg_cmd0_sw_terminate              // unsigned ,    RW, default = 0  1: terminate current processing
//Bit 0            reg_cmd0_sw_start                 // unsigned ,    RW, default = 0  1: trigger hardware process descriptor list
#define AMFC_GL_CMD0_CONFIG                        ((0x0005  << 2) + 0xfe024000)
//Bit 31: 0        reg_cmd0_config                   // unsigned,     RW, default = 0
#define AMFC_GL_CMD0_STATUS                        ((0x0006  << 2) + 0xfe024000)
//Bit 31: 26       reserved
//Bit 25           ro_cmd0_erro_irq                  // unsigned ,     RO, default = 0  1: has irq signal
//Bit 24           ro_cmd0_done_irq                  // unsigned ,     RO, default = 0  1: has irq signal
//Bit 23: 16       reserved
//Bit 15: 8        ro_cmd0_err_code                  // unsigned ,     RO, default = 0
//Bit 7 : 0        ro_cmd0_status                    // unsigned ,     RO, default = 0  0: idle  1: busy  2: pending
#define AMFC_GL_CMD0_FEATURE                       ((0x0007  << 2) + 0xfe024000)
//Bit 31: 5        reserved
//Bit 4            ro_cmd0_feat_zlib                 // unsigned ,    RW, default = 0
//Bit 3            ro_cmd0_feat_deflate              // unsigned ,    RW, default = 0
//Bit 2            ro_cmd0_feat_gzip                 // unsigned ,    RW, default = 0
//Bit 1            ro_cmd0_feat_lz4                  // unsigned ,    RW, default = 0
//Bit 0            ro_cmd0_feat_zstd                 // unsigned ,    RW, default = 1 , if the whole module works as dma, write 'hf in this reg
#define AMFC_GL_CMD0_IRQCLR                        ((0x0008  << 2) + 0xfe024000)
//Bit 31: 2        reserved
//Bit 1            reg_cmd0_erro_irq_clr             // unsigned,     RW, default = 0  clear irq state
//Bit 0            reg_cmd0_done_irq_clr             // unsigned,     RW, default = 0  clear irq state
//--------------------------
#define AMFC_GL_CMD1_DESC_BASE0_ADDR               ((0x0012  << 2) + 0xfe024000)
//Bit 31: 0        reg_cmd1_desc_base0_addr           // unsigned ,RW, default = 0  base address of descriptor list; Physical address = DESCRIPTOR BASE ADDDR<<6
#define AMFC_GL_CMD1_CURR_DESC_ADDR                ((0x0013  << 2) + 0xfe024000)
//Bit 31: 0        ro_cmd1_cur_desc_addr             // unsigned ,RO, default = 0  address of current processing descriptor
#define AMFC_GL_CMD1_CONTROL                       ((0x0014  << 2) + 0xfe024000)
//Bit 31           reg_cmd1_sw_rst                   // unsigned ,    RW, default = 0  reset AMFC state machine
//Bit 30: 5        reserved
//Bit 4            reg_cmd1_global_irq_en            // unsigned ,    RW, default = 0 1 to enable interrupt
//Bit 3 : 2        reg_cmd1_64bit_os_mode            // unsigned ,    RW, default = 2  0: 32bit os mode,src/dst buffer address in descriptor list is [33:2]; 1: 64bit os mode, [34:3], 2/3: [37:6]
//Bit 1            reg_cmd1_sw_terminate              // unsigned ,    RW, default = 0  1: terminate current processing
//Bit 0            reg_cmd1_sw_start                 // unsigned ,    RW, default = 0  1: trigger hardware process descriptor list
#define AMFC_GL_CMD1_CONFIG                        ((0x0015  << 2) + 0xfe024000)
//Bit 31: 0        reg_cmd1_config                   // unsigned,     RW, default = 0
#define AMFC_GL_CMD1_STATUS                        ((0x0016  << 2) + 0xfe024000)
//Bit 31: 26       reserved
//Bit 25           ro_cmd1_erro_irq                  // unsigned ,     RO, default = 0  1: has irq signal
//Bit 24           ro_cmd1_done_irq                  // unsigned ,     RO, default = 0  1: has irq signal
//Bit 23: 16       reserved
//Bit 15: 8        ro_cmd1_err_code                  // unsigned ,     RO, default = 0
//Bit 7 : 0        ro_cmd1_status                    // unsigned ,     RO, default = 0  0: idle  1: busy  2: pending
#define AMFC_GL_CMD1_FEATURE                       ((0x0017  << 2) + 0xfe024000)
//Bit 31: 5        reserved
//Bit 4            ro_cmd1_feat_zlib                 // unsigned ,    RW, default = 0
//Bit 3            ro_cmd1_feat_deflate              // unsigned ,    RW, default = 0
//Bit 2            ro_cmd1_feat_gzip                 // unsigned ,    RW, default = 0
//Bit 1            ro_cmd1_feat_lz4                  // unsigned ,    RW, default = 0
//Bit 0            ro_cmd1_feat_zstd                 // unsigned ,    RW, default = 1 , if the whole module works as dma, write 'hf in this reg
#define AMFC_GL_CMD1_IRQCLR                        ((0x0018  << 2) + 0xfe024000)
//Bit 31: 2        reserved
//Bit 1            reg_cmd1_erro_irq_clr             // unsigned,     RW, default = 0  clear irq state
//Bit 0            reg_cmd1_done_irq_clr             // unsigned,     RW, default = 0  clear irq state

#define AMFC_CMD0_TIME_MEASURE                     ((0x001c  << 2) + 0xfe024000)
#define AMFC_CMD1_TIME_MEASURE                     ((0x001d  << 2) + 0xfe024000)
#define AMFC_DECOMPR_STATUS4                       ((0x002e  << 2) + 0xfe024000)

//---------------------------------------------
//Gate clock control
#define AMFC_CMD_GATE_CLK_CTRL                     ((0x0020  << 2) + 0xfe024000)
//Bit 31: 0        reg_cmd_gclk_ctrl                 // unsigned ,     RW, default = 0  gated clock control
#define AMFC_ENC_GATE_CTRL_CTRL_0                  ((0x0021  << 2) + 0xfe024000)
//Bit 31: 0        reg_enc_gclk_ctrl_0               // unsigned ,     RW, default = 0  gated clock control
#define AMFC_ENC_GATE_CTRL_CTRL_1                  ((0x0022  << 2) + 0xfe024000)
//Bit 31: 0        reg_enc_gclk_ctrl_1               // unsigned ,     RW, default = 0  gated clock control
#define AMFC_DEC_GATE_CTRL_CTRL_0                  ((0x0023  << 2) + 0xfe024000)
//Bit 31: 0        reg_dec_gclk_ctrl_0               // unsigned ,     RW, default = 0  gated clock control
#define AMFC_DEC_GATE_CTRL_CTRL_1                  ((0x0024  << 2) + 0xfe024000)
//Bit 31: 0        reg_dec_gclk_ctrl_1               // unsigned ,     RW, default = 0  gated clock control
//CODEC
#define AMFC_CODEC_CTRL                            ((0x0028  << 2) + 0xfe024000)
//Bit 31:8         reserved
//Bit  7:6         reg_cmd1_dst_end_mode             // unsigned ,     RW, default = 2,  0: mif req end;  1: data write done;  2: mif write respond done
//Bit  5:4         reg_cmd0_dst_end_mode             // unsigned ,     RW, default = 2,  0: mif req end;  1: data write done;  2: mif write respond done
//Bit  3:2         reserved
//Bit  1           reg_decmpr_enable                 // unsigned ,     RW, default = 0, 1 to enable the decompr
//Bit  0           reg_cmpr_enable                   // unsigned ,     RW, default = 0, 1 to enable the compr
#define AMFC_COMPR_STATUS                          ((0x0029  << 2) + 0xfe024000)
//Bit 31:0         ro_cmpr_status                    // unsigned ,     RO, default = 0, compr status
#define AMFC_DECOMPR_STATUS_0                      ((0x002a  << 2) + 0xfe024000)
//Bit 31:0         ro_decmpr_status_0               // unsigned ,     RO, default = 0, decompr status
#define AMFC_DECOMPR_STATUS_1                      ((0x002b  << 2) + 0xfe024000)
//Bit 31:0         ro_decmpr_status_1               // unsigned ,     RO, default = 0, decompr status
#define AMFC_DECOMPR_STATUS_2                      ((0x002c  << 2) + 0xfe024000)
//Bit 31:0         ro_decmpr_status_2               // unsigned ,     RO, default = 0, decompr status
#define AMFC_DECOMPR_STATUS_3                      ((0x002d  << 2) + 0xfe024000)
//Bit 31:0         ro_decmpr_bsinfo                // unsigned ,     RO, default = 0, decompr status
//----------------------------------
#define AMFC_ZSTD_MODE_MISC                        ((0x0030  << 2) + 0xfe024000)
//Bit 31:17        reserved
//Bit 16           reg_dec_frame_mode             // unsigned , RW, default = 0, 0: 1 only decode one frame
//Bit 15:3         reserved
//Bit  2           reg_enc_lz77_srch_mode         // unsigned , RW, default = 0, 0: search window is not across the block boundary. 1: search window is across the block boundary.
//Bit  1           reg_enc_lz77_disable           // unsigned , RW, default = 0, 1 to disable LZ77 in enc
//Bit  0           reg_enc_fse_default_allowed    // unsigned , RW, default = 1, 1 to enable FSE default table for compr
#define AMFC_ZSTD_HASH_TBL_INIT                    ((0x0031  << 2) + 0xfe024000)
//Bit 31:6         reserved
//Bit 5:4          reg_enc_hash_tbl_init_mode       // unsigned , RW, default = 0, hash table initial mode. 1:initial the hash table for each block. 2:initial the hash table for each frame. 0/3: no init
//Bit 3:1          reserved
//Bit  0           reg_enc_hash_tbl_init            // unsigned , RW, default = 0, write 1 to initial the hash table
#define AMFC_ZSTD_LZ77_SRCH_WIN                    ((0x0032  << 2) + 0xfe024000)
//Bit 31:16        reserved
//Bit 15:0         reg_enc_lz77_srch_wsize        // unsigned , RW, default = 4096, max search win size for LZ77, it is only valid when  reg_enc_lz77_srch_mode=1
#define AMFC_ZSTD_DEC_ERR_MSK                      ((0x0033  << 2) + 0xfe024000)
//Bit 31:0         reg_dec_error_msk                // unsigned , RW, default =0, 1 to mask the dec error
//------------------------------------------
//MIF register
#define AMFC_WR_MIF_CTRL                           ((0x0040  << 2) + 0xfe024000)
//Bit  31:24   reserved
//Bit  23:16   reg_wrmif_canvas_id             // unsigned , RW, default = 0, axi canvas id num
//Bit  15:11   reserved
//Bit  10:8    reg_wrmif_burst_len             // unsigned , RW, default = 2, burst type: 0-single 1-bst2 2-bst4 3-bst8 4-bst16
//Bit  7:6     reserved
//Bit  5       reg_wrmif_swap_64bit            // unsigned , RW, default = 0, 64bits of 128bit swap enable
//Bit  4       reg_wrmif_little_endian         // unsigned , RW, default = 0, big endian enable
//Bit  3:1     reserved
//Bit  0       reg_wrmif_enable                // unsigned , RW, default = 1  1 to mif
#define AMFC_WR_MIF_STATUS                         ((0x0041  << 2) + 0xfe024000)
//Bit  31:0    ro_wrmif_status              // unsigned ,  RO, default = 0, wrmif status
#define AMFC_RD_MIF_CTRL                           ((0x0042  << 2) + 0xfe024000)
//Bit  31:24   reserved
//Bit  23:16   reg_rdmif_canvas_id           // unsigned , RW, default = 0, axi canvas id num
//Bit  15:11   reserved
//Bit  10:8    reg_rdmif_burst_len           // unsigned , RW, default = 2, burst type: 0-single 1-bst2 2-bst4 3-bst8 4-bst16
//Bit  7:6     reserved
//Bit  5       reg_rdmif_swap_64bit          // unsigned , RW, default = 0, 64bits of 128bit swap enable
//Bit  4       reg_rdmif_little_endian       // unsigned , RW, default = 0, big endian enable
//Bit  3:1     reserved
//Bit  0       reg_rdmif_enable              // unsigned , RW, default = 1  1 to mif
#define AMFC_RD_MIF_STATUS                         ((0x0043  << 2) + 0xfe024000)
//Bit  31:0    ro_rdmif_status               // unsigned , RO, default = 0, rdmif status
#define AMFC_MIF_QOS_UGT                           ((0x0044  << 2) + 0xfe024000)
//Bit  31:16   reserved
//Bit  15      reg_decmpr_arugt              // unsigned , RW, default =0,  decmpr mif read Urgent
//Bit  14      reg_decmpr_arqos              // unsigned , RW, default =0,  decmpr mif read  QOS
//Bit  13      reg_decmpr_awugt              // unsigned , RW, default =0,  decmpr mif write Urgent
//Bit  12      reg_decmpr_awqos              // unsigned , RW, default =0,  decmpr mif write QOS
//Bit  11      reg_cmpr_arugt                // unsigned , RW, default =0,  cmpr mif read Urgent
//Bit  10      reg_cmpr_arqos                // unsigned , RW, default =0,  cmpr mif read  QOS
//Bit   9      reg_cmpr_awugt                // unsigned , RW, default =0,  cmpr mif write Urgent
//Bit   8      reg_cmpr_awqos                // unsigned , RW, default =0,  cmpr mif write QOS
//Bit   7      reg_cmd1_arugt                // unsigned , RW, default =0,  cmd1 mif read Urgent
//Bit   6      reg_cmd1_arqos                // unsigned , RW, default =0,  cmd1 mif read  QOS
//Bit   5      reg_cmd1_awugt                // unsigned , RW, default =0,  cmd1 mif write Urgent
//Bit   4      reg_cmd1_awqos                // unsigned , RW, default =0,  cmd1 mif write QOS
//Bit   3      reg_cmd0_arugt                // unsigned , RW, default =0,  cmd0 mif read Urgent
//Bit   2      reg_cmd0_arqos                // unsigned , RW, default =0,  cmd0 mif read  QOS
//Bit   1      reg_cmd0_awugt                // unsigned , RW, default =0,  cmd0 mif write Urgent
//Bit   0      reg_cmd0_awqos                // unsigned , RW, default =0,  cmd0 mif write QOS
#endif
