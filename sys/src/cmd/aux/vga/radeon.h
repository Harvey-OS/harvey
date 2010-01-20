/*
 * Copyright 2000 ATI Technologies Inc., Markham, Ontario, and
 *                VA Linux Systems Inc., Fremont, California.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation on the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT.  IN NO EVENT SHALL ATI, VA LINUX SYSTEMS AND/OR
 * THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *   Kevin E. Martin <martin@xfree86.org>
 *   Rickard E. Faith <faith@valinux.com>
 *   Alan Hourihane <alanh@fairlite.demon.co.uk>
 *
 * References:
 *
 * !!!! FIXME !!!!
 *   RAGE 128 VR/ RAGE 128 GL Register Reference Manual (Technical
 *   Reference Manual P/N RRG-G04100-C Rev. 0.04), ATI Technologies: April
 *   1999.
 *
 * !!!! FIXME !!!!
 *   RAGE 128 Software Development Manual (Technical Reference Manual P/N
 *   SDK-G04000 Rev. 0.01), ATI Technologies: June 1999.
 *
 * !!!! FIXME !!!!  NOTE: THIS FILE HAS BEEN CONVERTED FROM r128_reg.h
 * AND CONTAINS REGISTERS AND REGISTER DEFINITIONS THAT ARE NOT CORRECT
 * ON THE RADEON.  A FULL AUDIT OF THIS CODE IS NEEDED!
 */

#define BIOS8(r, o)  ((uchar) (r->bios[o]))
#define BIOS16(r, o) ((ushort)(r->bios[o] | (r->bios[(o) + 1] << 8)))
#define BIOS32(r, o) ((ulong) (r->bios[o] | (r->bios[(o) + 1] << 8) | \
	(r->bios[(o) + 2] << 16) | (r->bios[(o) + 3] << 24)))

enum {
	BIOS_START	= 0x48,
};

/* Registers for 2D/Video/Overlay */
#define ADAPTER_ID                   0x0f2c /* PCI */
#define AGP_BASE                     0x0170
#define AGP_CNTL                     0x0174
#       define AGP_APER_SIZE_256MB   (0x00 << 0)
#       define AGP_APER_SIZE_128MB   (0x20 << 0)
#       define AGP_APER_SIZE_64MB    (0x30 << 0)
#       define AGP_APER_SIZE_32MB    (0x38 << 0)
#       define AGP_APER_SIZE_16MB    (0x3c << 0)
#       define AGP_APER_SIZE_8MB     (0x3e << 0)
#       define AGP_APER_SIZE_4MB     (0x3f << 0)
#       define AGP_APER_SIZE_MASK    (0x3f << 0)
#define AGP_COMMAND                  0x0f60 /* PCI */
#define AGP_PLL_CNTL                 0x000b /* PLL */
#define AGP_STATUS                   0x0f5c /* PCI */
#       define AGP_1X_MODE           0x01
#       define AGP_2X_MODE           0x02
#       define AGP_4X_MODE           0x04
#       define AGP_FW_MODE           0x10
#       define AGP_MODE_MASK         0x17
#define ATTRDR                       0x03c1 /* VGA */
#define ATTRDW                       0x03c0 /* VGA */
#define ATTRX                        0x03c0 /* VGA */
#define AUX_SC_CNTL                  0x1660
#       define AUX1_SC_EN            (1 << 0)
#       define AUX1_SC_MODE_OR       (0 << 1)
#       define AUX1_SC_MODE_NAND     (1 << 1)
#       define AUX2_SC_EN            (1 << 2)
#       define AUX2_SC_MODE_OR       (0 << 3)
#       define AUX2_SC_MODE_NAND     (1 << 3)
#       define AUX3_SC_EN            (1 << 4)
#       define AUX3_SC_MODE_OR       (0 << 5)
#       define AUX3_SC_MODE_NAND     (1 << 5)
#define AUX1_SC_BOTTOM               0x1670
#define AUX1_SC_LEFT                 0x1664
#define AUX1_SC_RIGHT                0x1668
#define AUX1_SC_TOP                  0x166c
#define AUX2_SC_BOTTOM               0x1680
#define AUX2_SC_LEFT                 0x1674
#define AUX2_SC_RIGHT                0x1678
#define AUX2_SC_TOP                  0x167c
#define AUX3_SC_BOTTOM               0x1690
#define AUX3_SC_LEFT                 0x1684
#define AUX3_SC_RIGHT                0x1688
#define AUX3_SC_TOP                  0x168c
#define AUX_WINDOW_HORZ_CNTL         0x02d8
#define AUX_WINDOW_VERT_CNTL         0x02dc

#define BASE_CODE                    0x0f0b
#define BIOS_0_SCRATCH               0x0010
#define BIOS_1_SCRATCH               0x0014
#define BIOS_2_SCRATCH               0x0018
#define BIOS_3_SCRATCH               0x001c
#define BIOS_4_SCRATCH               0x0020
#define BIOS_5_SCRATCH               0x0024
#define BIOS_6_SCRATCH               0x0028
#define BIOS_7_SCRATCH               0x002c
#define BIOS_ROM                     0x0f30 /* PCI */
#define BIST                         0x0f0f /* PCI */
#define BRUSH_DATA0                  0x1480
#define BRUSH_DATA1                  0x1484
#define BRUSH_DATA10                 0x14a8
#define BRUSH_DATA11                 0x14ac
#define BRUSH_DATA12                 0x14b0
#define BRUSH_DATA13                 0x14b4
#define BRUSH_DATA14                 0x14b8
#define BRUSH_DATA15                 0x14bc
#define BRUSH_DATA16                 0x14c0
#define BRUSH_DATA17                 0x14c4
#define BRUSH_DATA18                 0x14c8
#define BRUSH_DATA19                 0x14cc
#define BRUSH_DATA2                  0x1488
#define BRUSH_DATA20                 0x14d0
#define BRUSH_DATA21                 0x14d4
#define BRUSH_DATA22                 0x14d8
#define BRUSH_DATA23                 0x14dc
#define BRUSH_DATA24                 0x14e0
#define BRUSH_DATA25                 0x14e4
#define BRUSH_DATA26                 0x14e8
#define BRUSH_DATA27                 0x14ec
#define BRUSH_DATA28                 0x14f0
#define BRUSH_DATA29                 0x14f4
#define BRUSH_DATA3                  0x148c
#define BRUSH_DATA30                 0x14f8
#define BRUSH_DATA31                 0x14fc
#define BRUSH_DATA32                 0x1500
#define BRUSH_DATA33                 0x1504
#define BRUSH_DATA34                 0x1508
#define BRUSH_DATA35                 0x150c
#define BRUSH_DATA36                 0x1510
#define BRUSH_DATA37                 0x1514
#define BRUSH_DATA38                 0x1518
#define BRUSH_DATA39                 0x151c
#define BRUSH_DATA4                  0x1490
#define BRUSH_DATA40                 0x1520
#define BRUSH_DATA41                 0x1524
#define BRUSH_DATA42                 0x1528
#define BRUSH_DATA43                 0x152c
#define BRUSH_DATA44                 0x1530
#define BRUSH_DATA45                 0x1534
#define BRUSH_DATA46                 0x1538
#define BRUSH_DATA47                 0x153c
#define BRUSH_DATA48                 0x1540
#define BRUSH_DATA49                 0x1544
#define BRUSH_DATA5                  0x1494
#define BRUSH_DATA50                 0x1548
#define BRUSH_DATA51                 0x154c
#define BRUSH_DATA52                 0x1550
#define BRUSH_DATA53                 0x1554
#define BRUSH_DATA54                 0x1558
#define BRUSH_DATA55                 0x155c
#define BRUSH_DATA56                 0x1560
#define BRUSH_DATA57                 0x1564
#define BRUSH_DATA58                 0x1568
#define BRUSH_DATA59                 0x156c
#define BRUSH_DATA6                  0x1498
#define BRUSH_DATA60                 0x1570
#define BRUSH_DATA61                 0x1574
#define BRUSH_DATA62                 0x1578
#define BRUSH_DATA63                 0x157c
#define BRUSH_DATA7                  0x149c
#define BRUSH_DATA8                  0x14a0
#define BRUSH_DATA9                  0x14a4
#define BRUSH_SCALE                  0x1470
#define BRUSH_Y_X                    0x1474
#define BUS_CNTL                     0x0030
#       define BUS_MASTER_DIS         (1 << 6)
#       define BUS_RD_DISCARD_EN      (1 << 24)
#       define BUS_RD_ABORT_EN        (1 << 25)
#       define BUS_MSTR_DISCONNECT_EN (1 << 28)
#       define BUS_WRT_BURST          (1 << 29)
#       define BUS_READ_BURST         (1 << 30)
#define BUS_CNTL1                    0x0034
#       define BUS_WAIT_ON_LOCK_EN    (1 << 4)

#define CACHE_CNTL                   0x1724
#define CACHE_LINE                   0x0f0c /* PCI */
#define CAP0_TRIG_CNTL               0x0950 /* ? */
#define CAP1_TRIG_CNTL               0x09c0 /* ? */
#define CAPABILITIES_ID              0x0f50 /* PCI */
#define CAPABILITIES_PTR             0x0f34 /* PCI */
#define CLK_PIN_CNTL                 0x0001 /* PLL */
#define CLOCK_CNTL_DATA              0x000c
#define CLOCK_CNTL_INDEX             0x0008
#       define PLL_WR_EN             (1 << 7)
#       define PLL_DIV_SEL           (3 << 8)
#       define PLL2_DIV_SEL_MASK     ~(3 << 8)
#define CLR_CMP_CLR_3D               0x1a24
#define CLR_CMP_CLR_DST              0x15c8
#define CLR_CMP_CLR_SRC              0x15c4
#define CLR_CMP_CNTL                 0x15c0
#       define SRC_CMP_EQ_COLOR      (4 <<  0)
#       define SRC_CMP_NEQ_COLOR     (5 <<  0)
#       define CLR_CMP_SRC_SOURCE    (1 << 24)
#define CLR_CMP_MASK                 0x15cc
#       define CLR_CMP_MSK           0xffffffff
#define CLR_CMP_MASK_3D              0x1A28
#define COMMAND                      0x0f04 /* PCI */
#define COMPOSITE_SHADOW_ID          0x1a0c
#define CONFIG_APER_0_BASE           0x0100
#define CONFIG_APER_1_BASE           0x0104
#define CONFIG_APER_SIZE             0x0108
#define CONFIG_BONDS                 0x00e8
#define CONFIG_CNTL                  0x00e0
#       define CFG_ATI_REV_A11       (0   << 16)
#       define CFG_ATI_REV_A12       (1   << 16)
#       define CFG_ATI_REV_A13       (2   << 16)
#       define CFG_ATI_REV_ID_MASK   (0xf << 16)
#define CONFIG_MEMSIZE               0x00f8
#define CONFIG_MEMSIZE_EMBEDDED      0x0114
#define CONFIG_REG_1_BASE            0x010c
#define CONFIG_REG_APER_SIZE         0x0110
#define CONFIG_XSTRAP                0x00e4
#define CONSTANT_COLOR_C             0x1d34
#       define CONSTANT_COLOR_MASK   0x00ffffff
#       define CONSTANT_COLOR_ONE    0x00ffffff
#       define CONSTANT_COLOR_ZERO   0x00000000
#define CRC_CMDFIFO_ADDR             0x0740
#define CRC_CMDFIFO_DOUT             0x0744
#define CRTC_CRNT_FRAME              0x0214
#define CRTC_EXT_CNTL                0x0054
#       define CRTC_VGA_XOVERSCAN    (1 <<  0)
#       define VGA_ATI_LINEAR        (1 <<  3)
#       define XCRT_CNT_EN           (1 <<  6)
#       define CRTC_HSYNC_DIS        (1 <<  8)
#       define CRTC_VSYNC_DIS        (1 <<  9)
#       define CRTC_DISPLAY_DIS      (1 << 10)
#       define CRTC_SYNC_TRISTAT     (1 << 11)
#       define CRTC_CRT_ON           (1 << 15)
#define CRTC_EXT_CNTL_DPMS_BYTE      0x0055
#       define CRTC_HSYNC_DIS_BYTE   (1 <<  0)
#       define CRTC_VSYNC_DIS_BYTE   (1 <<  1)
#       define CRTC_DISPLAY_DIS_BYTE (1 <<  2)
#define CRTC_GEN_CNTL                0x0050
#       define CRTC_DBL_SCAN_EN      (1 <<  0)
#       define CRTC_INTERLACE_EN     (1 <<  1)
#       define CRTC_CSYNC_EN         (1 <<  4)
#       define CRTC_CUR_EN           (1 << 16)
#       define CRTC_CUR_MODE_MASK    (7 << 17)
#       define CRTC_ICON_EN          (1 << 20)
#       define CRTC_EXT_DISP_EN      (1 << 24)
#       define CRTC_EN               (1 << 25)
#       define CRTC_DISP_REQ_EN_B    (1 << 26)
#define CRTC2_GEN_CNTL               0x03f8
#       define CRTC2_DBL_SCAN_EN     (1 <<  0)
#       define CRTC2_INTERLACE_EN    (1 <<  1)
#       define CRTC2_SYNC_TRISTAT    (1 <<  4)
#       define CRTC2_HSYNC_TRISTAT   (1 <<  5)
#       define CRTC2_VSYNC_TRISTAT   (1 <<  6)
#       define CRTC2_CRT2_ON         (1 <<  7)
#       define CRTC2_ICON_EN         (1 << 15)
#       define CRTC2_CUR_EN          (1 << 16)
#       define CRTC2_CUR_MODE_MASK   (7 << 20)
#       define CRTC2_DISP_DIS        (1 << 23)
#       define CRTC2_EN              (1 << 25)
#       define CRTC2_DISP_REQ_EN_B   (1 << 26)
#       define CRTC2_CSYNC_EN        (1 << 27)
#       define CRTC2_HSYNC_DIS       (1 << 28)
#       define CRTC2_VSYNC_DIS       (1 << 29)
#define CRTC_GUI_TRIG_VLINE          0x0218
#define CRTC_H_SYNC_STRT_WID         0x0204
#       define CRTC_H_SYNC_STRT_PIX        (0x07  <<  0)
#       define CRTC_H_SYNC_STRT_CHAR       (0x3ff <<  3)
#       define CRTC_H_SYNC_STRT_CHAR_SHIFT 3
#       define CRTC_H_SYNC_WID             (0x3f  << 16)
#       define CRTC_H_SYNC_WID_SHIFT       16
#       define CRTC_H_SYNC_POL             (1     << 23)
#define CRTC2_H_SYNC_STRT_WID        0x0304
#       define CRTC2_H_SYNC_STRT_PIX        (0x07  <<  0)
#       define CRTC2_H_SYNC_STRT_CHAR       (0x3ff <<  3)
#       define CRTC2_H_SYNC_STRT_CHAR_SHIFT 3
#       define CRTC2_H_SYNC_WID             (0x3f  << 16)
#       define CRTC2_H_SYNC_WID_SHIFT       16
#       define CRTC2_H_SYNC_POL             (1     << 23)
#define CRTC_H_TOTAL_DISP            0x0200
#       define CRTC_H_TOTAL          (0x03ff << 0)
#       define CRTC_H_TOTAL_SHIFT    0
#       define CRTC_H_DISP           (0x01ff << 16)
#       define CRTC_H_DISP_SHIFT     16
#define CRTC2_H_TOTAL_DISP           0x0300
#       define CRTC2_H_TOTAL         (0x03ff << 0)
#       define CRTC2_H_TOTAL_SHIFT   0
#       define CRTC2_H_DISP          (0x01ff << 16)
#       define CRTC2_H_DISP_SHIFT    16
#define CRTC_OFFSET                  0x0224
#define CRTC2_OFFSET                 0x0324
#define CRTC_OFFSET_CNTL             0x0228
#       define CRTC_TILE_EN          (1 << 15)
#define CRTC2_OFFSET_CNTL            0x0328
#       define CRTC2_TILE_EN         (1 << 15)
#define CRTC_PITCH                   0x022c
#define CRTC2_PITCH                  0x032c
#define CRTC_STATUS                  0x005c
#       define CRTC_VBLANK_SAVE      (1 <<  1)
#       define CRTC_VBLANK_SAVE_CLEAR  (1 <<  1)
#define CRTC2_STATUS                  0x03fc
#       define CRTC2_VBLANK_SAVE      (1 <<  1)
#       define CRTC2_VBLANK_SAVE_CLEAR  (1 <<  1)
#define CRTC_V_SYNC_STRT_WID         0x020c
#       define CRTC_V_SYNC_STRT        (0x7ff <<  0)
#       define CRTC_V_SYNC_STRT_SHIFT  0
#       define CRTC_V_SYNC_WID         (0x1f  << 16)
#       define CRTC_V_SYNC_WID_SHIFT   16
#       define CRTC_V_SYNC_POL         (1     << 23)
#define CRTC2_V_SYNC_STRT_WID        0x030c
#       define CRTC2_V_SYNC_STRT       (0x7ff <<  0)
#       define CRTC2_V_SYNC_STRT_SHIFT 0
#       define CRTC2_V_SYNC_WID        (0x1f  << 16)
#       define CRTC2_V_SYNC_WID_SHIFT  16
#       define CRTC2_V_SYNC_POL        (1     << 23)
#define CRTC_V_TOTAL_DISP            0x0208
#       define CRTC_V_TOTAL          (0x07ff << 0)
#       define CRTC_V_TOTAL_SHIFT    0
#       define CRTC_V_DISP           (0x07ff << 16)
#       define CRTC_V_DISP_SHIFT     16
#define CRTC2_V_TOTAL_DISP           0x0308
#       define CRTC2_V_TOTAL         (0x07ff << 0)
#       define CRTC2_V_TOTAL_SHIFT   0
#       define CRTC2_V_DISP          (0x07ff << 16)
#       define CRTC2_V_DISP_SHIFT    16
#define CRTC_VLINE_CRNT_VLINE        0x0210
#       define CRTC_CRNT_VLINE_MASK  (0x7ff << 16)
#define CRTC2_CRNT_FRAME             0x0314
#define CRTC2_GUI_TRIG_VLINE         0x0318
#define CRTC2_VLINE_CRNT_VLINE       0x0310
#define CRTC8_DATA                   0x03d5 /* VGA, 0x3b5 */
#define CRTC8_IDX                    0x03d4 /* VGA, 0x3b4 */
#define CUR_CLR0                     0x026c
#define CUR_CLR1                     0x0270
#define CUR_HORZ_VERT_OFF            0x0268
#define CUR_HORZ_VERT_POSN           0x0264
#define CUR_OFFSET                   0x0260
#       define CUR_LOCK              (1 << 31)
#define CUR2_CLR0                    0x036c
#define CUR2_CLR1                    0x0370
#define CUR2_HORZ_VERT_OFF           0x0368
#define CUR2_HORZ_VERT_POSN          0x0364
#define CUR2_OFFSET                  0x0360
#       define CUR2_LOCK             (1 << 31)

#define DAC_CNTL                     0x0058
#       define DAC_RANGE_CNTL        (3 <<  0)
#       define DAC_BLANKING          (1 <<  2)
#       define DAC_8BIT_EN           (1 <<  8)
#       define DAC_VGA_ADR_EN        (1 << 13)
#       define DAC_PDWN              (1 << 15)
#       define DAC_MASK_ALL          (0xff << 24)
#define DAC_CNTL2                    0x007c
#       define DAC2_DAC_CLK_SEL      (1 <<  0)
#       define DAC2_DAC2_CLK_SEL     (1 <<  1)
#       define DAC2_PALETTE_ACC_CTL  (1 <<  5)
#define TV_DAC_CNTL                  0x088c
#       define TV_DAC_STD_MASK       0x0300
#       define TV_DAC_RDACPD         (1 <<  24)
#       define TV_DAC_GDACPD         (1 <<  25)
#       define TV_DAC_BDACPD         (1 <<  26)
#define DISP_HW_DEBUG                0x0d14
#       define CRT2_DISP1_SEL        (1 <<  5)
#define DISP_OUTPUT_CNTL             0x0d64
#       define DISP_DAC_SOURCE_MASK  0x03
#       define DISP_DAC_SOURCE_CRTC2 0x01
#define DAC_CRC_SIG                  0x02cc
#define DAC_DATA                     0x03c9 /* VGA */
#define DAC_MASK                     0x03c6 /* VGA */
#define DAC_R_INDEX                  0x03c7 /* VGA */
#define DAC_W_INDEX                  0x03c8 /* VGA */
#define DDA_CONFIG                   0x02e0
#define DDA_ON_OFF                   0x02e4
#define DEFAULT_OFFSET               0x16e0
#define DEFAULT_PITCH                0x16e4
#define DEFAULT_SC_BOTTOM_RIGHT      0x16e8
#       define DEFAULT_SC_RIGHT_MAX  (0x1fff <<  0)
#       define DEFAULT_SC_BOTTOM_MAX (0x1fff << 16)
#define DESTINATION_3D_CLR_CMP_VAL   0x1820
#define DESTINATION_3D_CLR_CMP_MSK   0x1824
#define DEVICE_ID                    0x0f02 /* PCI */
#define DISP_MISC_CNTL               0x0d00
#       define SOFT_RESET_GRPH_PP    (1 << 0)
#define DP_BRUSH_BKGD_CLR            0x1478
#define DP_BRUSH_FRGD_CLR            0x147c
#define DP_CNTL                      0x16c0
#       define DST_X_LEFT_TO_RIGHT   (1 <<  0)
#       define DST_Y_TOP_TO_BOTTOM   (1 <<  1)
#define DP_CNTL_XDIR_YDIR_YMAJOR     0x16d0
#       define DST_Y_MAJOR             (1 <<  2)
#       define DST_Y_DIR_TOP_TO_BOTTOM (1 << 15)
#       define DST_X_DIR_LEFT_TO_RIGHT (1 << 31)
#define DP_DATATYPE                  0x16c4
#       define HOST_BIG_ENDIAN_EN    (1 << 29)
#define DP_GUI_MASTER_CNTL           0x146c
#       define GMC_SRC_PITCH_OFFSET_CNTL   (1    <<  0)
#       define GMC_DST_PITCH_OFFSET_CNTL   (1    <<  1)
#       define GMC_SRC_CLIPPING            (1    <<  2)
#       define GMC_DST_CLIPPING            (1    <<  3)
#       define GMC_BRUSH_DATATYPE_MASK     (0x0f <<  4)
#       define GMC_BRUSH_8X8_MONO_FG_BG    (0    <<  4)
#       define GMC_BRUSH_8X8_MONO_FG_LA    (1    <<  4)
#       define GMC_BRUSH_1X8_MONO_FG_BG    (4    <<  4)
#       define GMC_BRUSH_1X8_MONO_FG_LA    (5    <<  4)
#       define GMC_BRUSH_32x1_MONO_FG_BG   (6    <<  4)
#       define GMC_BRUSH_32x1_MONO_FG_LA   (7    <<  4)
#       define GMC_BRUSH_32x32_MONO_FG_BG  (8    <<  4)
#       define GMC_BRUSH_32x32_MONO_FG_LA  (9    <<  4)
#       define GMC_BRUSH_8x8_COLOR         (10   <<  4)
#       define GMC_BRUSH_1X8_COLOR         (12   <<  4)
#       define GMC_BRUSH_SOLID_COLOR       (13   <<  4)
#       define GMC_BRUSH_NONE              (15   <<  4)
#       define GMC_DST_8BPP_CI             (2    <<  8)
#       define GMC_DST_15BPP               (3    <<  8)
#       define GMC_DST_16BPP               (4    <<  8)
#       define GMC_DST_24BPP               (5    <<  8)
#       define GMC_DST_32BPP               (6    <<  8)
#       define GMC_DST_8BPP_RGB            (7    <<  8)
#       define GMC_DST_Y8                  (8    <<  8)
#       define GMC_DST_RGB8                (9    <<  8)
#       define GMC_DST_VYUY                (11   <<  8)
#       define GMC_DST_YVYU                (12   <<  8)
#       define GMC_DST_AYUV444             (14   <<  8)
#       define GMC_DST_ARGB4444            (15   <<  8)
#       define GMC_DST_DATATYPE_MASK       (0x0f <<  8)
#       define GMC_DST_DATATYPE_SHIFT      8
#       define GMC_SRC_DATATYPE_MASK       (3    << 12)
#       define GMC_SRC_DATATYPE_MONO_FG_BG (0    << 12)
#       define GMC_SRC_DATATYPE_MONO_FG_LA (1    << 12)
#       define GMC_SRC_DATATYPE_COLOR      (3    << 12)
#       define GMC_BYTE_PIX_ORDER          (1    << 14)
#       define GMC_BYTE_MSB_TO_LSB         (0    << 14)
#       define GMC_BYTE_LSB_TO_MSB         (1    << 14)
#       define GMC_CONVERSION_TEMP         (1    << 15)
#       define GMC_CONVERSION_TEMP_6500    (0    << 15)
#       define GMC_CONVERSION_TEMP_9300    (1    << 15)
#       define GMC_ROP3_MASK               (0xff << 16)
#       define DP_SRC_SOURCE_MASK          (7    << 24)
#       define DP_SRC_SOURCE_MEMORY        (2    << 24)
#       define DP_SRC_SOURCE_HOST_DATA     (3    << 24)
#       define GMC_3D_FCN_EN               (1    << 27)
#       define GMC_CLR_CMP_CNTL_DIS        (1    << 28)
#       define GMC_AUX_CLIP_DIS            (1    << 29)
#       define GMC_WR_MSK_DIS              (1    << 30)
#       define GMC_LD_BRUSH_Y_X            (1    << 31)
#       define ROP3_ZERO             0x00000000
#       define ROP3_DSa              0x00880000
#       define ROP3_SDna             0x00440000
#       define ROP3_S                0x00cc0000
#       define ROP3_DSna             0x00220000
#       define ROP3_D                0x00aa0000
#       define ROP3_DSx              0x00660000
#       define ROP3_DSo              0x00ee0000
#       define ROP3_DSon             0x00110000
#       define ROP3_DSxn             0x00990000
#       define ROP3_Dn               0x00550000
#       define ROP3_SDno             0x00dd0000
#       define ROP3_Sn               0x00330000
#       define ROP3_DSno             0x00bb0000
#       define ROP3_DSan             0x00770000
#       define ROP3_ONE              0x00ff0000
#       define ROP3_DPa              0x00a00000
#       define ROP3_PDna             0x00500000
#       define ROP3_P                0x00f00000
#       define ROP3_DPna             0x000a0000
#       define ROP3_DPo              0x00fa0000
#       define ROP3_DPon             0x00050000
#       define ROP3_PDxn             0x00a50000
#       define ROP3_DPx              0x005a0000
#       define ROP3_PDno             0x00f50000
#       define ROP3_Pn               0x000f0000
#       define ROP3_DPno             0x00af0000
#       define ROP3_DPan             0x005f0000
#define DP_GUI_MASTER_CNTL_C         0x1c84
#define DP_MIX                       0x16c8
#define DP_SRC_BKGD_CLR              0x15dc
#define DP_SRC_FRGD_CLR              0x15d8
#define DP_WRITE_MASK                0x16cc
#define DST_BRES_DEC                 0x1630
#define DST_BRES_ERR                 0x1628
#define DST_BRES_INC                 0x162c
#define DST_BRES_LNTH                0x1634
#define DST_BRES_LNTH_SUB            0x1638
#define DST_HEIGHT                   0x1410
#define DST_HEIGHT_WIDTH             0x143c
#define DST_HEIGHT_WIDTH_8           0x158c
#define DST_HEIGHT_WIDTH_BW          0x15b4
#define DST_HEIGHT_Y                 0x15a0
#define DST_LINE_START               0x1600
#define DST_LINE_END                 0x1604
#define DST_LINE_PATCOUNT            0x1608
#       define BRES_CNTL_SHIFT       8
#define DST_OFFSET                   0x1404
#define DST_PITCH                    0x1408
#define DST_PITCH_OFFSET             0x142c
#define DST_PITCH_OFFSET_C           0x1c80
#       define PITCH_SHIFT           21
#       define DST_TILE_LINEAR       (0 << 30)
#       define DST_TILE_MACRO        (1 << 30)
#       define DST_TILE_MICRO        (2 << 30)
#       define DST_TILE_BOTH         (3 << 30)
#define DST_WIDTH                    0x140c
#define DST_WIDTH_HEIGHT             0x1598
#define DST_WIDTH_X                  0x1588
#define DST_WIDTH_X_INCY             0x159c
#define DST_X                        0x141c
#define DST_X_SUB                    0x15a4
#define DST_X_Y                      0x1594
#define DST_Y                        0x1420
#define DST_Y_SUB                    0x15a8
#define DST_Y_X                      0x1438

#define FCP_CNTL                     0x0910
#      define FCP0_SRC_PCICLK             0
#      define FCP0_SRC_PCLK               1
#      define FCP0_SRC_PCLKb              2
#      define FCP0_SRC_HREF               3
#      define FCP0_SRC_GND                4
#      define FCP0_SRC_HREFb              5
#define FLUSH_1                      0x1704
#define FLUSH_2                      0x1708
#define FLUSH_3                      0x170c
#define FLUSH_4                      0x1710
#define FLUSH_5                      0x1714
#define FLUSH_6                      0x1718
#define FLUSH_7                      0x171c
#define FOG_3D_TABLE_START           0x1810
#define FOG_3D_TABLE_END             0x1814
#define FOG_3D_TABLE_DENSITY         0x181c
#define FOG_TABLE_INDEX              0x1a14
#define FOG_TABLE_DATA               0x1a18
#define FP_CRTC_H_TOTAL_DISP         0x0250
#define FP_CRTC_V_TOTAL_DISP         0x0254
#define FP_CRTC2_H_TOTAL_DISP        0x0350
#define FP_CRTC2_V_TOTAL_DISP        0x0354
#       define FP_CRTC_H_TOTAL_MASK      0x000003ff
#       define FP_CRTC_H_DISP_MASK       0x01ff0000
#       define FP_CRTC_V_TOTAL_MASK      0x00000fff
#       define FP_CRTC_V_DISP_MASK       0x0fff0000
#       define FP_H_SYNC_STRT_CHAR_MASK  0x00001ff8
#       define FP_H_SYNC_WID_MASK        0x003f0000
#       define FP_V_SYNC_STRT_MASK       0x00000fff
#       define FP_V_SYNC_WID_MASK        0x001f0000
#       define FP_CRTC_H_TOTAL_SHIFT     0x00000000
#       define FP_CRTC_H_DISP_SHIFT      0x00000010
#       define FP_CRTC_V_TOTAL_SHIFT     0x00000000
#       define FP_CRTC_V_DISP_SHIFT      0x00000010
#       define FP_H_SYNC_STRT_CHAR_SHIFT 0x00000003
#       define FP_H_SYNC_WID_SHIFT       0x00000010
#       define FP_V_SYNC_STRT_SHIFT      0x00000000
#       define FP_V_SYNC_WID_SHIFT       0x00000010
#define FP_GEN_CNTL                  0x0284
#       define FP_FPON                  (1 <<  0)
#       define FP_TMDS_EN               (1 <<  2)
#       define FP_PANEL_FORMAT          (1 <<  3)
#       define FP_EN_TMDS               (1 <<  7)
#       define FP_DETECT_SENSE          (1 <<  8)
#       define FP_SEL_CRTC2             (1 << 13)
#       define FP_CRTC_DONT_SHADOW_HPAR (1 << 15)
#       define FP_CRTC_DONT_SHADOW_VPAR (1 << 16)
#       define FP_CRTC_DONT_SHADOW_HEND (1 << 17)
#       define FP_CRTC_USE_SHADOW_VEND  (1 << 18)
#       define FP_RMX_HVSYNC_CONTROL_EN (1 << 20)
#       define FP_DFP_SYNC_SEL          (1 << 21)
#       define FP_CRTC_LOCK_8DOT        (1 << 22)
#       define FP_CRT_SYNC_SEL          (1 << 23)
#       define FP_USE_SHADOW_EN         (1 << 24)
#       define FP_CRT_SYNC_ALT          (1 << 26)
#define FP2_GEN_CNTL                 0x0288
#       define FP2_BLANK_EN             (1 <<  1)
#       define FP2_ON                   (1 <<  2)
#       define FP2_PANEL_FORMAT         (1 <<  3)
#       define FP2_SEL_CRTC2            (1 << 13)
#       define FP2_FP_POL               (1 << 16)
#       define FP2_LP_POL               (1 << 17)
#       define FP2_SCK_POL              (1 << 18)
#       define FP2_LCD_CNTL_MASK        (7 << 19)
#       define FP2_PAD_FLOP_EN          (1 << 22)
#       define FP2_CRC_EN               (1 << 23)
#       define FP2_CRC_READ_EN          (1 << 24)
#define FP_H_SYNC_STRT_WID           0x02c4
#define FP_H2_SYNC_STRT_WID          0x03c4
#define FP_HORZ_STRETCH              0x028c
#define FP_HORZ2_STRETCH             0x038c
#       define HORZ_STRETCH_RATIO_MASK 0xffff
#       define HORZ_STRETCH_RATIO_MAX  4096
#       define HORZ_PANEL_SIZE         (0x1ff   << 16)
#       define HORZ_PANEL_SHIFT        16
#       define HORZ_STRETCH_PIXREP     (0      << 25)
#       define HORZ_STRETCH_BLEND      (1      << 26)
#       define HORZ_STRETCH_ENABLE     (1      << 25)
#       define HORZ_AUTO_RATIO         (1      << 27)
#       define HORZ_FP_LOOP_STRETCH    (0x7    << 28)
#       define HORZ_AUTO_RATIO_INC     (1      << 31)
#define FP_V_SYNC_STRT_WID           0x02c8
#define FP_VERT_STRETCH              0x0290
#define FP_V2_SYNC_STRT_WID          0x03c8
#define FP_VERT2_STRETCH             0x0390
#       define VERT_PANEL_SIZE          (0xfff << 12)
#       define VERT_PANEL_SHIFT         12
#       define VERT_STRETCH_RATIO_MASK  0xfff
#       define VERT_STRETCH_RATIO_SHIFT 0
#       define VERT_STRETCH_RATIO_MAX   4096
#       define VERT_STRETCH_ENABLE      (1     << 25)
#       define VERT_STRETCH_LINEREP     (0     << 26)
#       define VERT_STRETCH_BLEND       (1     << 26)
#       define VERT_AUTO_RATIO_EN       (1     << 27)
#       define VERT_STRETCH_RESERVED    0xf1000000

#define GEN_INT_CNTL                 0x0040
#define GEN_INT_STATUS               0x0044
#       define VSYNC_INT_AK          (1 <<  2)
#       define VSYNC_INT             (1 <<  2)
#       define VSYNC2_INT_AK         (1 <<  6)
#       define VSYNC2_INT            (1 <<  6)
#define GENENB                       0x03c3 /* VGA */
#define GENFC_RD                     0x03ca /* VGA */
#define GENFC_WT                     0x03da /* VGA, 0x03ba */
#define GENMO_RD                     0x03cc /* VGA */
#define GENMO_WT                     0x03c2 /* VGA */
#define GENS0                        0x03c2 /* VGA */
#define GENS1                        0x03da /* VGA, 0x03ba */
#define GPIO_MONID                   0x0068 /* DDC interface via I2C */
#define GPIO_MONIDB                  0x006c
#define GPIO_CRT2_DDC                0x006c
#define GPIO_DVI_DDC                 0x0064
#define GPIO_VGA_DDC                 0x0060
#       define GPIO_A_0              (1 <<  0)
#       define GPIO_A_1              (1 <<  1)
#       define GPIO_Y_0              (1 <<  8)
#       define GPIO_Y_1              (1 <<  9)
#       define GPIO_Y_SHIFT_0        8
#       define GPIO_Y_SHIFT_1        9
#       define GPIO_EN_0             (1 << 16)
#       define GPIO_EN_1             (1 << 17)
#       define GPIO_MASK_0           (1 << 24) /*??*/
#       define GPIO_MASK_1           (1 << 25) /*??*/
#define GRPH8_DATA                   0x03cf /* VGA */
#define GRPH8_IDX                    0x03ce /* VGA */
#define GUI_SCRATCH_REG0             0x15e0
#define GUI_SCRATCH_REG1             0x15e4
#define GUI_SCRATCH_REG2             0x15e8
#define GUI_SCRATCH_REG3             0x15ec
#define GUI_SCRATCH_REG4             0x15f0
#define GUI_SCRATCH_REG5             0x15f4

#define HEADER                       0x0f0e /* PCI */
#define HOST_DATA0                   0x17c0
#define HOST_DATA1                   0x17c4
#define HOST_DATA2                   0x17c8
#define HOST_DATA3                   0x17cc
#define HOST_DATA4                   0x17d0
#define HOST_DATA5                   0x17d4
#define HOST_DATA6                   0x17d8
#define HOST_DATA7                   0x17dc
#define HOST_DATA_LAST               0x17e0
#define HOST_PATH_CNTL               0x0130
#       define HDP_SOFT_RESET        (1 << 26)
#define HTOTAL_CNTL                  0x0009 /* PLL */
#define HTOTAL2_CNTL                 0x002e /* PLL */

#define I2C_CNTL_1                   0x0094 /* ? */
#define DVI_I2C_CNTL_1               0x02e4 /* ? */
#define INTERRUPT_LINE               0x0f3c /* PCI */
#define INTERRUPT_PIN                0x0f3d /* PCI */
#define IO_BASE                      0x0f14 /* PCI */

#define LATENCY                      0x0f0d /* PCI */
#define LEAD_BRES_DEC                0x1608
#define LEAD_BRES_LNTH               0x161c
#define LEAD_BRES_LNTH_SUB           0x1624
#define LVDS_GEN_CNTL                0x02d0
#       define LVDS_ON               (1   <<  0)
#       define LVDS_DISPLAY_DIS      (1   <<  1)
#       define LVDS_PANEL_TYPE       (1   <<  2)
#       define LVDS_PANEL_FORMAT     (1   <<  3)
#       define LVDS_EN               (1   <<  7)
#       define LVDS_DIGON            (1   << 18)
#       define LVDS_BLON             (1   << 19)
#       define LVDS_SEL_CRTC2        (1   << 23)
#define LVDS_PLL_CNTL                0x02d4
#       define HSYNC_DELAY_SHIFT     28
#       define HSYNC_DELAY_MASK      (0xf << 28)

#define MAX_LATENCY                  0x0f3f /* PCI */
#define MC_AGP_LOCATION              0x014c
#define MC_FB_LOCATION               0x0148
#define MCLK_CNTL                    0x0012 /* PLL */
#       define FORCEON_MCLKA         (1 << 16)
#       define FORCEON_MCLKB         (1 << 17)
#       define FORCEON_YCLKA         (1 << 18)
#       define FORCEON_YCLKB         (1 << 19)
#       define FORCEON_MC            (1 << 20)
#       define FORCEON_AIC           (1 << 21)
#define MDGPIO_A_REG                 0x01ac
#define MDGPIO_EN_REG                0x01b0
#define MDGPIO_MASK                  0x0198
#define MDGPIO_Y_REG                 0x01b4
#define MEM_ADDR_CONFIG              0x0148
#define MEM_BASE                     0x0f10 /* PCI */
#define MEM_CNTL                     0x0140
#define MEM_INIT_LAT_TIMER           0x0154
#define MEM_INTF_CNTL                0x014c
#define MEM_SDRAM_MODE_REG           0x0158
#define MEM_STR_CNTL                 0x0150
#define MEM_VGA_RP_SEL               0x003c
#define MEM_VGA_WP_SEL               0x0038
#define MIN_GRANT                    0x0f3e /* PCI */
#define MM_DATA                      0x0004
#define MM_INDEX                     0x0000
#define MPLL_CNTL                    0x000e /* PLL */
#define MPP_TB_CONFIG                0x01c0 /* ? */
#define MPP_GP_CONFIG                0x01c8 /* ? */


#define N_VIF_COUNT                  0x0248

#define OV0_AUTO_FLIP_CNTL           0x0470
#define OV0_COLOUR_CNTL              0x04E0
#define OV0_DEINTERLACE_PATTERN      0x0474
#define OV0_TEST                     0x04F8
#define OV0_EXCLUSIVE_HORZ           0x0408
#       define  EXCL_HORZ_START_MASK        0x000000ff
#       define  EXCL_HORZ_END_MASK          0x0000ff00
#       define  EXCL_HORZ_BACK_PORCH_MASK   0x00ff0000
#       define  EXCL_HORZ_EXCLUSIVE_EN      0x80000000
#define OV0_EXCLUSIVE_VERT           0x040C
#       define  EXCL_VERT_START_MASK        0x000003ff
#       define  EXCL_VERT_END_MASK          0x03ff0000
#define OV0_FILTER_CNTL              0x04A0
#define OV0_FOUR_TAP_COEF_0          0x04B0
#define OV0_FOUR_TAP_COEF_1          0x04B4
#define OV0_FOUR_TAP_COEF_2          0x04B8
#define OV0_FOUR_TAP_COEF_3          0x04BC
#define OV0_FOUR_TAP_COEF_4          0x04C0
#define OV0_GAMMA_000_00F            0x0d40
#define OV0_GAMMA_010_01F            0x0d44
#define OV0_GAMMA_020_03F            0x0d48
#define OV0_GAMMA_040_07F            0x0d4c
#define OV0_GAMMA_080_0BF            0x0e00
#define OV0_GAMMA_0C0_0FF            0x0e04
#define OV0_GAMMA_100_13F            0x0e08
#define OV0_GAMMA_140_17F            0x0e0c
#define OV0_GAMMA_180_1BF            0x0e10
#define OV0_GAMMA_1C0_1FF            0x0e14
#define OV0_GAMMA_200_23F            0x0e18
#define OV0_GAMMA_240_27F            0x0e1c
#define OV0_GAMMA_280_2BF            0x0e20
#define OV0_GAMMA_2C0_2FF            0x0e24
#define OV0_GAMMA_300_33F            0x0e28
#define OV0_GAMMA_340_37F            0x0e2c
#define OV0_GAMMA_380_3BF            0x0d50
#define OV0_GAMMA_3C0_3FF            0x0d54
#define OV0_GRAPHICS_KEY_CLR_LOW     0x04EC
#define OV0_GRAPHICS_KEY_CLR_HIGH    0x04F0
#define OV0_H_INC                    0x0480
#define OV0_KEY_CNTL                 0x04F4
#       define  VIDEO_KEY_FN_MASK    0x00000003L
#       define  VIDEO_KEY_FN_FALSE   0x00000000L
#       define  VIDEO_KEY_FN_TRUE    0x00000001L
#       define  VIDEO_KEY_FN_EQ      0x00000002L
#       define  VIDEO_KEY_FN_NE      0x00000003L
#       define  GRAPHIC_KEY_FN_MASK  0x00000030L
#       define  GRAPHIC_KEY_FN_FALSE 0x00000000L
#       define  GRAPHIC_KEY_FN_TRUE  0x00000010L
#       define  GRAPHIC_KEY_FN_EQ    0x00000020L
#       define  GRAPHIC_KEY_FN_NE    0x00000030L
#       define  CMP_MIX_MASK         0x00000100L
#       define  CMP_MIX_OR           0x00000000L
#       define  CMP_MIX_AND          0x00000100L
#define OV0_LIN_TRANS_A              0x0d20
#define OV0_LIN_TRANS_B              0x0d24
#define OV0_LIN_TRANS_C              0x0d28
#define OV0_LIN_TRANS_D              0x0d2c
#define OV0_LIN_TRANS_E              0x0d30
#define OV0_LIN_TRANS_F              0x0d34
#define OV0_P1_BLANK_LINES_AT_TOP    0x0430
#       define  P1_BLNK_LN_AT_TOP_M1_MASK   0x00000fffL
#       define  P1_ACTIVE_LINES_M1          0x0fff0000L
#define OV0_P1_H_ACCUM_INIT          0x0488
#define OV0_P1_V_ACCUM_INIT          0x0428
#       define  OV0_P1_MAX_LN_IN_PER_LN_OUT 0x00000003L
#       define  OV0_P1_V_ACCUM_INIT_MASK    0x01ff8000L
#define OV0_P1_X_START_END           0x0494
#define OV0_P2_X_START_END           0x0498
#define OV0_P23_BLANK_LINES_AT_TOP   0x0434
#       define  P23_BLNK_LN_AT_TOP_M1_MASK  0x000007ffL
#       define  P23_ACTIVE_LINES_M1         0x07ff0000L
#define OV0_P23_H_ACCUM_INIT         0x048C
#define OV0_P23_V_ACCUM_INIT         0x042C
#define OV0_P3_X_START_END           0x049C
#define OV0_REG_LOAD_CNTL            0x0410
#       define  REG_LD_CTL_LOCK                 0x00000001L
#       define  REG_LD_CTL_VBLANK_DURING_LOCK   0x00000002L
#       define  REG_LD_CTL_STALL_GUI_UNTIL_FLIP 0x00000004L
#       define  REG_LD_CTL_LOCK_READBACK        0x00000008L
#define OV0_SCALE_CNTL               0x0420
#       define  SCALER_HORZ_PICK_NEAREST    0x00000004L
#       define  SCALER_VERT_PICK_NEAREST    0x00000008L
#       define  SCALER_SIGNED_UV            0x00000010L
#       define  SCALER_GAMMA_SEL_MASK       0x00000060L
#       define  SCALER_GAMMA_SEL_BRIGHT     0x00000000L
#       define  SCALER_GAMMA_SEL_G22        0x00000020L
#       define  SCALER_GAMMA_SEL_G18        0x00000040L
#       define  SCALER_GAMMA_SEL_G14        0x00000060L
#       define  SCALER_COMCORE_SHIFT_UP_ONE 0x00000080L
#       define  SCALER_SURFAC_FORMAT        0x00000f00L
#       define  SCALER_SOURCE_15BPP         0x00000300L
#       define  SCALER_SOURCE_16BPP         0x00000400L
#       define  SCALER_SOURCE_32BPP         0x00000600L
#       define  SCALER_SOURCE_YUV9          0x00000900L
#       define  SCALER_SOURCE_YUV12         0x00000A00L
#       define  SCALER_SOURCE_VYUY422       0x00000B00L
#       define  SCALER_SOURCE_YVYU422       0x00000C00L
#       define  SCALER_ADAPTIVE_DEINT       0x00001000L
#       define  SCALER_TEMPORAL_DEINT       0x00002000L
#       define  SCALER_SMART_SWITCH         0x00008000L
#       define  SCALER_BURST_PER_PLANE      0x007F0000L
#       define  SCALER_DOUBLE_BUFFER        0x01000000L
#       define  SCALER_DIS_LIMIT            0x08000000L
#       define  SCALER_INT_EMU              0x20000000L
#       define  SCALER_ENABLE               0x40000000L
#       define  SCALER_SOFT_RESET           0x80000000L
#define OV0_STEP_BY                  0x0484
#define OV0_V_INC                    0x0424
#define OV0_VID_BUF_PITCH0_VALUE     0x0460
#define OV0_VID_BUF_PITCH1_VALUE     0x0464
#define OV0_VID_BUF0_BASE_ADRS       0x0440
#       define  VIF_BUF0_PITCH_SEL          0x00000001L
#       define  VIF_BUF0_TILE_ADRS          0x00000002L
#       define  VIF_BUF0_BASE_ADRS_MASK     0x03fffff0L
#       define  VIF_BUF0_1ST_LINE_LSBS_MASK 0x48000000L
#define OV0_VID_BUF1_BASE_ADRS       0x0444
#       define  VIF_BUF1_PITCH_SEL          0x00000001L
#       define  VIF_BUF1_TILE_ADRS          0x00000002L
#       define  VIF_BUF1_BASE_ADRS_MASK     0x03fffff0L
#       define  VIF_BUF1_1ST_LINE_LSBS_MASK 0x48000000L
#define OV0_VID_BUF2_BASE_ADRS       0x0448
#       define  VIF_BUF2_PITCH_SEL          0x00000001L
#       define  VIF_BUF2_TILE_ADRS          0x00000002L
#       define  VIF_BUF2_BASE_ADRS_MASK     0x03fffff0L
#       define  VIF_BUF2_1ST_LINE_LSBS_MASK 0x48000000L
#define OV0_VID_BUF3_BASE_ADRS       0x044C
#define OV0_VID_BUF4_BASE_ADRS       0x0450
#define OV0_VID_BUF5_BASE_ADRS       0x0454
#define OV0_VIDEO_KEY_CLR_HIGH       0x04E8
#define OV0_VIDEO_KEY_CLR_LOW        0x04E4
#define OV0_Y_X_START                0x0400
#define OV0_Y_X_END                  0x0404
#define OV1_Y_X_START                0x0600
#define OV1_Y_X_END                  0x0604
#define OVR_CLR                      0x0230
#define OVR_WID_LEFT_RIGHT           0x0234
#define OVR_WID_TOP_BOTTOM           0x0238

#define P2PLL_CNTL                   0x002a /* P2PLL */
#       define P2PLL_RESET                (1 <<  0)
#       define P2PLL_SLEEP                (1 <<  1)
#       define P2PLL_ATOMIC_UPDATE_EN     (1 << 16)
#       define P2PLL_VGA_ATOMIC_UPDATE_EN (1 << 17)
#       define P2PLL_ATOMIC_UPDATE_VSYNC  (1 << 18)
#define P2PLL_DIV_0                  0x002c
#       define P2PLL_FB0_DIV_MASK    0x07ff
#       define P2PLL_POST0_DIV_MASK  0x00070000
#define P2PLL_REF_DIV                0x002B /* PLL */
#       define P2PLL_REF_DIV_MASK    0x03ff
#       define P2PLL_ATOMIC_UPDATE_R (1 << 15) /* same as _W */
#       define P2PLL_ATOMIC_UPDATE_W (1 << 15) /* same as _R */
#       define R300_PPLL_REF_DIV_ACC_MASK   (0x3ff < 18)
#       define R300_PPLL_REF_DIV_ACC_SHIFT  18
#define PALETTE_DATA                 0x00b4
#define PALETTE_30_DATA              0x00b8
#define PALETTE_INDEX                0x00b0
#define PCI_GART_PAGE                0x017c
#define PIXCLKS_CNTL                 0x002d
#       define PIX2CLK_SRC_SEL_MASK     0x03
#       define PIX2CLK_SRC_SEL_CPUCLK   0x00
#       define PIX2CLK_SRC_SEL_PSCANCLK 0x01
#       define PIX2CLK_SRC_SEL_BYTECLK  0x02
#       define PIX2CLK_SRC_SEL_P2PLLCLK 0x03
#define PLANE_3D_MASK_C              0x1d44
#define PLL_TEST_CNTL                0x0013 /* PLL */
#define PMI_CAP_ID                   0x0f5c /* PCI */
#define PMI_DATA                     0x0f63 /* PCI */
#define PMI_NXT_CAP_PTR              0x0f5d /* PCI */
#define PMI_PMC_REG                  0x0f5e /* PCI */
#define PMI_PMCSR_REG                0x0f60 /* PCI */
#define PMI_REGISTER                 0x0f5c /* PCI */
#define PPLL_CNTL                    0x0002 /* PLL */
#       define PPLL_RESET                (1 <<  0)
#       define PPLL_SLEEP                (1 <<  1)
#       define PPLL_ATOMIC_UPDATE_EN     (1 << 16)
#       define PPLL_VGA_ATOMIC_UPDATE_EN (1 << 17)
#       define PPLL_ATOMIC_UPDATE_VSYNC  (1 << 18)
#define PPLL_DIV_0                   0x0004 /* PLL */
#define PPLL_DIV_1                   0x0005 /* PLL */
#define PPLL_DIV_2                   0x0006 /* PLL */
#define PPLL_DIV_3                   0x0007 /* PLL */
#       define PPLL_FB3_DIV_MASK     0x07ff
#       define PPLL_POST3_DIV_MASK   0x00070000
#define PPLL_REF_DIV                 0x0003 /* PLL */
#       define PPLL_REF_DIV_MASK     0x03ff
#       define PPLL_ATOMIC_UPDATE_R  (1 << 15) /* same as _W */
#       define PPLL_ATOMIC_UPDATE_W  (1 << 15) /* same as _R */
#define PWR_MNGMT_CNTL_STATUS        0x0f60 /* PCI */

#define RBBM_GUICNTL                 0x172c
#       define HOST_DATA_SWAP_NONE   (0 << 0)
#       define HOST_DATA_SWAP_16BIT  (1 << 0)
#       define HOST_DATA_SWAP_32BIT  (2 << 0)
#       define HOST_DATA_SWAP_HDW    (3 << 0)
#define RBBM_SOFT_RESET              0x00f0
#       define SOFT_RESET_CP         (1 <<  0)
#       define SOFT_RESET_HI         (1 <<  1)
#       define SOFT_RESET_SE         (1 <<  2)
#       define SOFT_RESET_RE         (1 <<  3)
#       define SOFT_RESET_PP         (1 <<  4)
#       define SOFT_RESET_E2         (1 <<  5)
#       define SOFT_RESET_RB         (1 <<  6)
#       define SOFT_RESET_HDP        (1 <<  7)
#define RBBM_STATUS                  0x0e40
#       define RBBM_FIFOCNT_MASK     0x007f
#       define RBBM_ACTIVE           (1 << 31)
#define RB2D_DSTCACHE_CTLSTAT        0x342c
#       define RB2D_DC_FLUSH         (3 << 0)
#       define RB2D_DC_FREE          (3 << 2)
#       define RB2D_DC_FLUSH_ALL     0xf
#       define RB2D_DC_BUSY          (1 << 31)
#define RB2D_DSTCACHE_MODE           0x3428
#define REG_BASE                     0x0f18 /* PCI */
#define REGPROG_INF                  0x0f09 /* PCI */
#define REVISION_ID                  0x0f08 /* PCI */

#define SC_BOTTOM                    0x164c
#define SC_BOTTOM_RIGHT              0x16f0
#define SC_BOTTOM_RIGHT_C            0x1c8c
#define SC_LEFT                      0x1640
#define SC_RIGHT                     0x1644
#define SC_TOP                       0x1648
#define SC_TOP_LEFT                  0x16ec
#define SC_TOP_LEFT_C                0x1c88
#       define SC_SIGN_MASK_LO       0x8000
#       define SC_SIGN_MASK_HI       0x80000000
#define SCLK_CNTL                    0x000d /* PLL */
#       define DYN_STOP_LAT_MASK     0x00007ff8
#       define CP_MAX_DYN_STOP_LAT   0x0008
#       define SCLK_FORCEON_MASK     0xffff8000
#define SCLK_MORE_CNTL               0x0035 /* PLL */
#       define SCLK_MORE_FORCEON     0x0700
#define SDRAM_MODE_REG               0x0158
#define SEQ8_DATA                    0x03c5 /* VGA */
#define SEQ8_IDX                     0x03c4 /* VGA */
#define SNAPSHOT_F_COUNT             0x0244
#define SNAPSHOT_VH_COUNTS           0x0240
#define SNAPSHOT_VIF_COUNT           0x024c
#define SRC_OFFSET                   0x15ac
#define SRC_PITCH                    0x15b0
#define SRC_PITCH_OFFSET             0x1428
#define SRC_SC_BOTTOM                0x165c
#define SRC_SC_BOTTOM_RIGHT          0x16f4
#define SRC_SC_RIGHT                 0x1654
#define SRC_X                        0x1414
#define SRC_X_Y                      0x1590
#define SRC_Y                        0x1418
#define SRC_Y_X                      0x1434
#define STATUS                       0x0f06 /* PCI */
#define SUBPIC_CNTL                  0x0540 /* ? */
#define SUB_CLASS                    0x0f0a /* PCI */
#define SURFACE_CNTL                 0x0b00
#       define SURF_TRANSLATION_DIS  (1 << 8)
#       define NONSURF_AP0_SWP_16BPP (1 << 20)
#       define NONSURF_AP0_SWP_32BPP (1 << 21)
#define SURFACE0_INFO                0x0b0c
#define SURFACE0_LOWER_BOUND         0x0b04
#define SURFACE0_UPPER_BOUND         0x0b08
#define SURFACE1_INFO                0x0b1c
#define SURFACE1_LOWER_BOUND         0x0b14
#define SURFACE1_UPPER_BOUND         0x0b18
#define SURFACE2_INFO                0x0b2c
#define SURFACE2_LOWER_BOUND         0x0b24
#define SURFACE2_UPPER_BOUND         0x0b28
#define SURFACE3_INFO                0x0b3c
#define SURFACE3_LOWER_BOUND         0x0b34
#define SURFACE3_UPPER_BOUND         0x0b38
#define SURFACE4_INFO                0x0b4c
#define SURFACE4_LOWER_BOUND         0x0b44
#define SURFACE4_UPPER_BOUND         0x0b48
#define SURFACE5_INFO                0x0b5c
#define SURFACE5_LOWER_BOUND         0x0b54
#define SURFACE5_UPPER_BOUND         0x0b58
#define SURFACE6_INFO                0x0b6c
#define SURFACE6_LOWER_BOUND         0x0b64
#define SURFACE6_UPPER_BOUND         0x0b68
#define SURFACE7_INFO                0x0b7c
#define SURFACE7_LOWER_BOUND         0x0b74
#define SURFACE7_UPPER_BOUND         0x0b78
#define SW_SEMAPHORE                 0x013c

#define TEST_DEBUG_CNTL              0x0120
#define TEST_DEBUG_MUX               0x0124
#define TEST_DEBUG_OUT               0x012c
#define TMDS_PLL_CNTL                0x02a8
#define TRAIL_BRES_DEC               0x1614
#define TRAIL_BRES_ERR               0x160c
#define TRAIL_BRES_INC               0x1610
#define TRAIL_X                      0x1618
#define TRAIL_X_SUB                  0x1620

#define VCLK_ECP_CNTL                0x0008 /* PLL */
#       define VCLK_SRC_SEL_MASK     0x03
#       define VCLK_SRC_SEL_CPUCLK   0x00
#       define VCLK_SRC_SEL_PSCANCLK 0x01
#       define VCLK_SRC_SEL_BYTECLK  0x02
#       define VCLK_SRC_SEL_PPLLCLK  0x03
#define VENDOR_ID                    0x0f00 /* PCI */
#define VGA_DDA_CONFIG               0x02e8
#define VGA_DDA_ON_OFF               0x02ec
#define VID_BUFFER_CONTROL           0x0900
#define VIDEOMUX_CNTL                0x0190
#define VIPH_CONTROL                 0x0c40 /* ? */

#define WAIT_UNTIL                   0x1720
#       define WAIT_CRTC_PFLIP       (1 << 0)
#       define WAIT_2D_IDLECLEAN     (1 << 16)
#       define WAIT_3D_IDLECLEAN     (1 << 17)
#       define WAIT_HOST_IDLECLEAN   (1 << 18)

#define X_MPLL_REF_FB_DIV            0x000a /* PLL */
#define XCLK_CNTL                    0x000d /* PLL */
#define XDLL_CNTL                    0x000c /* PLL */
#define XPLL_CNTL                    0x000b /* PLL */



				/* Registers for 3D/TCL */
#define PP_BORDER_COLOR_0            0x1d40
#define PP_BORDER_COLOR_1            0x1d44
#define PP_BORDER_COLOR_2            0x1d48
#define PP_CNTL                      0x1c38
#       define STIPPLE_ENABLE        (1 <<  0)
#       define SCISSOR_ENABLE        (1 <<  1)
#       define PATTERN_ENABLE        (1 <<  2)
#       define SHADOW_ENABLE         (1 <<  3)
#       define TEX_ENABLE_MASK       (0xf << 4)
#       define TEX_0_ENABLE          (1 <<  4)
#       define TEX_1_ENABLE          (1 <<  5)
#       define TEX_2_ENABLE          (1 <<  6)
#       define TEX_3_ENABLE          (1 <<  7)
#       define TEX_BLEND_ENABLE_MASK (0xf << 12)
#       define TEX_BLEND_0_ENABLE    (1 << 12)
#       define TEX_BLEND_1_ENABLE    (1 << 13)
#       define TEX_BLEND_2_ENABLE    (1 << 14)
#       define TEX_BLEND_3_ENABLE    (1 << 15)
#       define PLANAR_YUV_ENABLE     (1 << 20)
#       define SPECULAR_ENABLE       (1 << 21)
#       define FOG_ENABLE            (1 << 22)
#       define ALPHA_TEST_ENABLE     (1 << 23)
#       define ANTI_ALIAS_NONE       (0 << 24)
#       define ANTI_ALIAS_LINE       (1 << 24)
#       define ANTI_ALIAS_POLY       (2 << 24)
#       define ANTI_ALIAS_LINE_POLY  (3 << 24)
#       define BUMP_MAP_ENABLE       (1 << 26)
#       define BUMPED_MAP_T0         (0 << 27)
#       define BUMPED_MAP_T1         (1 << 27)
#       define BUMPED_MAP_T2         (2 << 27)
#       define TEX_3D_ENABLE_0       (1 << 29)
#       define TEX_3D_ENABLE_1       (1 << 30)
#       define MC_ENABLE             (1 << 31)
#define PP_FOG_COLOR                 0x1c18
#       define FOG_COLOR_MASK        0x00ffffff
#       define FOG_VERTEX            (0 << 24)
#       define FOG_TABLE             (1 << 24)
#       define FOG_USE_DEPTH         (0 << 25)
#       define FOG_USE_DIFFUSE_ALPHA (2 << 25)
#       define FOG_USE_SPEC_ALPHA    (3 << 25)
#define PP_LUM_MATRIX                0x1d00
#define PP_MISC                      0x1c14
#       define REF_ALPHA_MASK        0x000000ff
#       define ALPHA_TEST_FAIL       (0 << 8)
#       define ALPHA_TEST_LESS       (1 << 8)
#       define ALPHA_TEST_LEQUAL     (2 << 8)
#       define ALPHA_TEST_EQUAL      (3 << 8)
#       define ALPHA_TEST_GEQUAL     (4 << 8)
#       define ALPHA_TEST_GREATER    (5 << 8)
#       define ALPHA_TEST_NEQUAL     (6 << 8)
#       define ALPHA_TEST_PASS       (7 << 8)
#       define ALPHA_TEST_OP_MASK    (7 << 8)
#       define CHROMA_FUNC_FAIL      (0 << 16)
#       define CHROMA_FUNC_PASS      (1 << 16)
#       define CHROMA_FUNC_NEQUAL    (2 << 16)
#       define CHROMA_FUNC_EQUAL     (3 << 16)
#       define CHROMA_KEY_NEAREST    (0 << 18)
#       define CHROMA_KEY_ZERO       (1 << 18)
#       define SHADOW_ID_AUTO_INC    (1 << 20)
#       define SHADOW_FUNC_EQUAL     (0 << 21)
#       define SHADOW_FUNC_NEQUAL    (1 << 21)
#       define SHADOW_PASS_1         (0 << 22)
#       define SHADOW_PASS_2         (1 << 22)
#       define RIGHT_HAND_CUBE_D3D   (0 << 24)
#       define RIGHT_HAND_CUBE_OGL   (1 << 24)
#define PP_ROT_MATRIX_0              0x1d58
#define PP_ROT_MATRIX_1              0x1d5c
#define PP_TXFILTER_0                0x1c54
#define PP_TXFILTER_1                0x1c6c
#define PP_TXFILTER_2                0x1c84
#       define MAG_FILTER_NEAREST                   (0  <<  0)
#       define MAG_FILTER_LINEAR                    (1  <<  0)
#       define MAG_FILTER_MASK                      (1  <<  0)
#       define MIN_FILTER_NEAREST                   (0  <<  1)
#       define MIN_FILTER_LINEAR                    (1  <<  1)
#       define MIN_FILTER_NEAREST_MIP_NEAREST       (2  <<  1)
#       define MIN_FILTER_NEAREST_MIP_LINEAR        (3  <<  1)
#       define MIN_FILTER_LINEAR_MIP_NEAREST        (6  <<  1)
#       define MIN_FILTER_LINEAR_MIP_LINEAR         (7  <<  1)
#       define MIN_FILTER_ANISO_NEAREST             (8  <<  1)
#       define MIN_FILTER_ANISO_LINEAR              (9  <<  1)
#       define MIN_FILTER_ANISO_NEAREST_MIP_NEAREST (10 <<  1)
#       define MIN_FILTER_ANISO_NEAREST_MIP_LINEAR  (11 <<  1)
#       define MIN_FILTER_MASK                      (15 <<  1)
#       define MAX_ANISO_1_TO_1                     (0  <<  5)
#       define MAX_ANISO_2_TO_1                     (1  <<  5)
#       define MAX_ANISO_4_TO_1                     (2  <<  5)
#       define MAX_ANISO_8_TO_1                     (3  <<  5)
#       define MAX_ANISO_16_TO_1                    (4  <<  5)
#       define MAX_ANISO_MASK                       (7  <<  5)
#       define LOD_BIAS_MASK                        (0xff <<  8)
#       define LOD_BIAS_SHIFT                       8
#       define MAX_MIP_LEVEL_MASK                   (0x0f << 16)
#       define MAX_MIP_LEVEL_SHIFT                  16
#       define WRAPEN_S                             (1  << 22)
#       define CLAMP_S_WRAP                         (0  << 23)
#       define CLAMP_S_MIRROR                       (1  << 23)
#       define CLAMP_S_CLAMP_LAST                   (2  << 23)
#       define CLAMP_S_MIRROR_CLAMP_LAST            (3  << 23)
#       define CLAMP_S_CLAMP_BORDER                 (4  << 23)
#       define CLAMP_S_MIRROR_CLAMP_BORDER          (5  << 23)
#       define CLAMP_S_MASK                         (7  << 23)
#       define WRAPEN_T                             (1  << 26)
#       define CLAMP_T_WRAP                         (0  << 27)
#       define CLAMP_T_MIRROR                       (1  << 27)
#       define CLAMP_T_CLAMP_LAST                   (2  << 27)
#       define CLAMP_T_MIRROR_CLAMP_LAST            (3  << 27)
#       define CLAMP_T_CLAMP_BORDER                 (4  << 27)
#       define CLAMP_T_MIRROR_CLAMP_BORDER          (5  << 27)
#       define CLAMP_T_MASK                         (7  << 27)
#       define BORDER_MODE_OGL                      (0  << 31)
#       define BORDER_MODE_D3D                      (1  << 31)
#define PP_TXFORMAT_0                0x1c58
#define PP_TXFORMAT_1                0x1c70
#define PP_TXFORMAT_2                0x1c88
#       define TXFORMAT_I8                 (0  <<  0)
#       define TXFORMAT_AI88               (1  <<  0)
#       define TXFORMAT_RGB332             (2  <<  0)
#       define TXFORMAT_ARGB1555           (3  <<  0)
#       define TXFORMAT_RGB565             (4  <<  0)
#       define TXFORMAT_ARGB4444           (5  <<  0)
#       define TXFORMAT_ARGB8888           (6  <<  0)
#       define TXFORMAT_RGBA8888           (7  <<  0)
#       define TXFORMAT_Y8                 (8  <<  0)
#       define TXFORMAT_FORMAT_MASK        (31 <<  0)
#       define TXFORMAT_FORMAT_SHIFT       0
#       define TXFORMAT_APPLE_YUV_MODE     (1  <<  5)
#       define TXFORMAT_ALPHA_IN_MAP       (1  <<  6)
#       define TXFORMAT_NON_POWER2         (1  <<  7)
#       define TXFORMAT_WIDTH_MASK         (15 <<  8)
#       define TXFORMAT_WIDTH_SHIFT        8
#       define TXFORMAT_HEIGHT_MASK        (15 << 12)
#       define TXFORMAT_HEIGHT_SHIFT       12
#       define TXFORMAT_F5_WIDTH_MASK      (15 << 16)
#       define TXFORMAT_F5_WIDTH_SHIFT     16
#       define TXFORMAT_F5_HEIGHT_MASK     (15 << 20)
#       define TXFORMAT_F5_HEIGHT_SHIFT    20
#       define TXFORMAT_ST_ROUTE_STQ0      (0  << 24)
#       define TXFORMAT_ST_ROUTE_MASK      (3  << 24)
#       define TXFORMAT_ST_ROUTE_STQ1      (1  << 24)
#       define TXFORMAT_ST_ROUTE_STQ2      (2  << 24)
#       define TXFORMAT_ENDIAN_NO_SWAP     (0  << 26)
#       define TXFORMAT_ENDIAN_16BPP_SWAP  (1  << 26)
#       define TXFORMAT_ENDIAN_32BPP_SWAP  (2  << 26)
#       define TXFORMAT_ENDIAN_HALFDW_SWAP (3  << 26)
#       define TXFORMAT_ALPHA_MASK_ENABLE  (1  << 28)
#       define TXFORMAT_CHROMA_KEY_ENABLE  (1  << 29)
#       define TXFORMAT_CUBIC_MAP_ENABLE   (1  << 30)
#       define TXFORMAT_PERSPECTIVE_ENABLE (1  << 31)
#define PP_CUBIC_FACES_0             0x1d24
#define PP_CUBIC_FACES_1             0x1d28
#define PP_CUBIC_FACES_2             0x1d2c
#       define FACE_WIDTH_1_SHIFT          0
#       define FACE_HEIGHT_1_SHIFT         4
#       define FACE_WIDTH_1_MASK           (0xf << 0)
#       define FACE_HEIGHT_1_MASK          (0xf << 4)
#       define FACE_WIDTH_2_SHIFT          8
#       define FACE_HEIGHT_2_SHIFT         12
#       define FACE_WIDTH_2_MASK           (0xf << 8)
#       define FACE_HEIGHT_2_MASK          (0xf << 12)
#       define FACE_WIDTH_3_SHIFT          16
#       define FACE_HEIGHT_3_SHIFT         20
#       define FACE_WIDTH_3_MASK           (0xf << 16)
#       define FACE_HEIGHT_3_MASK          (0xf << 20)
#       define FACE_WIDTH_4_SHIFT          24
#       define FACE_HEIGHT_4_SHIFT         28
#       define FACE_WIDTH_4_MASK           (0xf << 24)
#       define FACE_HEIGHT_4_MASK          (0xf << 28)

#define PP_TXOFFSET_0                0x1c5c
#define PP_TXOFFSET_1                0x1c74
#define PP_TXOFFSET_2                0x1c8c
#       define TXO_ENDIAN_NO_SWAP     (0 << 0)
#       define TXO_ENDIAN_BYTE_SWAP   (1 << 0)
#       define TXO_ENDIAN_WORD_SWAP   (2 << 0)
#       define TXO_ENDIAN_HALFDW_SWAP (3 << 0)
#       define TXO_MACRO_LINEAR       (0 << 2)
#       define TXO_MACRO_TILE         (1 << 2)
#       define TXO_MICRO_LINEAR       (0 << 3)
#       define TXO_MICRO_TILE_X2      (1 << 3)
#       define TXO_MICRO_TILE_OPT     (2 << 3)
#       define TXO_OFFSET_MASK        0xffffffe0
#       define TXO_OFFSET_SHIFT       5

#define PP_CUBIC_OFFSET_T0_0         0x1dd0  /* bits [31:5] */
#define PP_CUBIC_OFFSET_T0_1         0x1dd4
#define PP_CUBIC_OFFSET_T0_2         0x1dd8
#define PP_CUBIC_OFFSET_T0_3         0x1ddc
#define PP_CUBIC_OFFSET_T0_4         0x1de0
#define PP_CUBIC_OFFSET_T1_0         0x1e00
#define PP_CUBIC_OFFSET_T1_1         0x1e04
#define PP_CUBIC_OFFSET_T1_2         0x1e08
#define PP_CUBIC_OFFSET_T1_3         0x1e0c
#define PP_CUBIC_OFFSET_T1_4         0x1e10
#define PP_CUBIC_OFFSET_T2_0         0x1e14
#define PP_CUBIC_OFFSET_T2_1         0x1e18
#define PP_CUBIC_OFFSET_T2_2         0x1e1c
#define PP_CUBIC_OFFSET_T2_3         0x1e20
#define PP_CUBIC_OFFSET_T2_4         0x1e24

#define PP_TEX_SIZE_0                0x1d04  /* NPOT */
#define PP_TEX_SIZE_1                0x1d0c
#define PP_TEX_SIZE_2                0x1d14
#       define TEX_USIZE_MASK        (0x7ff << 0)
#       define TEX_USIZE_SHIFT       0
#       define TEX_VSIZE_MASK        (0x7ff << 16)
#       define TEX_VSIZE_SHIFT       16
#       define SIGNED_RGB_MASK       (1 << 30)
#       define SIGNED_RGB_SHIFT      30
#       define SIGNED_ALPHA_MASK     (1 << 31)
#       define SIGNED_ALPHA_SHIFT    31

#define PP_TXCBLEND_0                0x1c60
#define PP_TXCBLEND_1                0x1c78
#define PP_TXCBLEND_2                0x1c90
#       define COLOR_ARG_A_SHIFT          0
#       define COLOR_ARG_A_MASK           (0x1f << 0)
#       define COLOR_ARG_A_ZERO           (0    << 0)
#       define COLOR_ARG_A_CURRENT_COLOR  (2    << 0)
#       define COLOR_ARG_A_CURRENT_ALPHA  (3    << 0)
#       define COLOR_ARG_A_DIFFUSE_COLOR  (4    << 0)
#       define COLOR_ARG_A_DIFFUSE_ALPHA  (5    << 0)
#       define COLOR_ARG_A_SPECULAR_COLOR (6    << 0)
#       define COLOR_ARG_A_SPECULAR_ALPHA (7    << 0)
#       define COLOR_ARG_A_TFACTOR_COLOR  (8    << 0)
#       define COLOR_ARG_A_TFACTOR_ALPHA  (9    << 0)
#       define COLOR_ARG_A_T0_COLOR       (10   << 0)
#       define COLOR_ARG_A_T0_ALPHA       (11   << 0)
#       define COLOR_ARG_A_T1_COLOR       (12   << 0)
#       define COLOR_ARG_A_T1_ALPHA       (13   << 0)
#       define COLOR_ARG_A_T2_COLOR       (14   << 0)
#       define COLOR_ARG_A_T2_ALPHA       (15   << 0)
#       define COLOR_ARG_A_T3_COLOR       (16   << 0)
#       define COLOR_ARG_A_T3_ALPHA       (17   << 0)
#       define COLOR_ARG_B_SHIFT          5
#       define COLOR_ARG_B_MASK           (0x1f << 5)
#       define COLOR_ARG_B_ZERO           (0    << 5)
#       define COLOR_ARG_B_CURRENT_COLOR  (2    << 5)
#       define COLOR_ARG_B_CURRENT_ALPHA  (3    << 5)
#       define COLOR_ARG_B_DIFFUSE_COLOR  (4    << 5)
#       define COLOR_ARG_B_DIFFUSE_ALPHA  (5    << 5)
#       define COLOR_ARG_B_SPECULAR_COLOR (6    << 5)
#       define COLOR_ARG_B_SPECULAR_ALPHA (7    << 5)
#       define COLOR_ARG_B_TFACTOR_COLOR  (8    << 5)
#       define COLOR_ARG_B_TFACTOR_ALPHA  (9    << 5)
#       define COLOR_ARG_B_T0_COLOR       (10   << 5)
#       define COLOR_ARG_B_T0_ALPHA       (11   << 5)
#       define COLOR_ARG_B_T1_COLOR       (12   << 5)
#       define COLOR_ARG_B_T1_ALPHA       (13   << 5)
#       define COLOR_ARG_B_T2_COLOR       (14   << 5)
#       define COLOR_ARG_B_T2_ALPHA       (15   << 5)
#       define COLOR_ARG_B_T3_COLOR       (16   << 5)
#       define COLOR_ARG_B_T3_ALPHA       (17   << 5)
#       define COLOR_ARG_C_SHIFT          10
#       define COLOR_ARG_C_MASK           (0x1f << 10)
#       define COLOR_ARG_C_ZERO           (0    << 10)
#       define COLOR_ARG_C_CURRENT_COLOR  (2    << 10)
#       define COLOR_ARG_C_CURRENT_ALPHA  (3    << 10)
#       define COLOR_ARG_C_DIFFUSE_COLOR  (4    << 10)
#       define COLOR_ARG_C_DIFFUSE_ALPHA  (5    << 10)
#       define COLOR_ARG_C_SPECULAR_COLOR (6    << 10)
#       define COLOR_ARG_C_SPECULAR_ALPHA (7    << 10)
#       define COLOR_ARG_C_TFACTOR_COLOR  (8    << 10)
#       define COLOR_ARG_C_TFACTOR_ALPHA  (9    << 10)
#       define COLOR_ARG_C_T0_COLOR       (10   << 10)
#       define COLOR_ARG_C_T0_ALPHA       (11   << 10)
#       define COLOR_ARG_C_T1_COLOR       (12   << 10)
#       define COLOR_ARG_C_T1_ALPHA       (13   << 10)
#       define COLOR_ARG_C_T2_COLOR       (14   << 10)
#       define COLOR_ARG_C_T2_ALPHA       (15   << 10)
#       define COLOR_ARG_C_T3_COLOR       (16   << 10)
#       define COLOR_ARG_C_T3_ALPHA       (17   << 10)
#       define COMP_ARG_A                 (1 << 15)
#       define COMP_ARG_A_SHIFT           15
#       define COMP_ARG_B                 (1 << 16)
#       define COMP_ARG_B_SHIFT           16
#       define COMP_ARG_C                 (1 << 17)
#       define COMP_ARG_C_SHIFT           17
#       define BLEND_CTL_MASK             (7 << 18)
#       define BLEND_CTL_ADD              (0 << 18)
#       define BLEND_CTL_SUBTRACT         (1 << 18)
#       define BLEND_CTL_ADDSIGNED        (2 << 18)
#       define BLEND_CTL_BLEND            (3 << 18)
#       define BLEND_CTL_DOT3             (4 << 18)
#       define SCALE_SHIFT                21
#       define SCALE_MASK                 (3 << 21)
#       define SCALE_1X                   (0 << 21)
#       define SCALE_2X                   (1 << 21)
#       define SCALE_4X                   (2 << 21)
#       define CLAMP_TX                   (1 << 23)
#       define T0_EQ_TCUR                 (1 << 24)
#       define T1_EQ_TCUR                 (1 << 25)
#       define T2_EQ_TCUR                 (1 << 26)
#       define T3_EQ_TCUR                 (1 << 27)
#       define COLOR_ARG_MASK             0x1f
#       define COMP_ARG_SHIFT             15
#define PP_TXABLEND_0                0x1c64
#define PP_TXABLEND_1                0x1c7c
#define PP_TXABLEND_2                0x1c94
#       define ALPHA_ARG_A_SHIFT          0
#       define ALPHA_ARG_A_MASK           (0xf << 0)
#       define ALPHA_ARG_A_ZERO           (0   << 0)
#       define ALPHA_ARG_A_CURRENT_ALPHA  (1   << 0)
#       define ALPHA_ARG_A_DIFFUSE_ALPHA  (2   << 0)
#       define ALPHA_ARG_A_SPECULAR_ALPHA (3   << 0)
#       define ALPHA_ARG_A_TFACTOR_ALPHA  (4   << 0)
#       define ALPHA_ARG_A_T0_ALPHA       (5   << 0)
#       define ALPHA_ARG_A_T1_ALPHA       (6   << 0)
#       define ALPHA_ARG_A_T2_ALPHA       (7   << 0)
#       define ALPHA_ARG_A_T3_ALPHA       (8   << 0)
#       define ALPHA_ARG_B_SHIFT          4
#       define ALPHA_ARG_B_MASK           (0xf << 4)
#       define ALPHA_ARG_B_ZERO           (0   << 4)
#       define ALPHA_ARG_B_CURRENT_ALPHA  (1   << 4)
#       define ALPHA_ARG_B_DIFFUSE_ALPHA  (2   << 4)
#       define ALPHA_ARG_B_SPECULAR_ALPHA (3   << 4)
#       define ALPHA_ARG_B_TFACTOR_ALPHA  (4   << 4)
#       define ALPHA_ARG_B_T0_ALPHA       (5   << 4)
#       define ALPHA_ARG_B_T1_ALPHA       (6   << 4)
#       define ALPHA_ARG_B_T2_ALPHA       (7   << 4)
#       define ALPHA_ARG_B_T3_ALPHA       (8   << 4)
#       define ALPHA_ARG_C_SHIFT          8
#       define ALPHA_ARG_C_MASK           (0xf << 8)
#       define ALPHA_ARG_C_ZERO           (0   << 8)
#       define ALPHA_ARG_C_CURRENT_ALPHA  (1   << 8)
#       define ALPHA_ARG_C_DIFFUSE_ALPHA  (2   << 8)
#       define ALPHA_ARG_C_SPECULAR_ALPHA (3   << 8)
#       define ALPHA_ARG_C_TFACTOR_ALPHA  (4   << 8)
#       define ALPHA_ARG_C_T0_ALPHA       (5   << 8)
#       define ALPHA_ARG_C_T1_ALPHA       (6   << 8)
#       define ALPHA_ARG_C_T2_ALPHA       (7   << 8)
#       define ALPHA_ARG_C_T3_ALPHA       (8   << 8)
#       define DOT_ALPHA_DONT_REPLICATE   (1   << 9)
#       define ALPHA_ARG_MASK             0xf

#define PP_TFACTOR_0                 0x1c68
#define PP_TFACTOR_1                 0x1c80
#define PP_TFACTOR_2                 0x1c98

#define RB3D_BLENDCNTL               0x1c20
#       define COMB_FCN_MASK                    (3  << 12)
#       define COMB_FCN_ADD_CLAMP               (0  << 12)
#       define COMB_FCN_ADD_NOCLAMP             (1  << 12)
#       define COMB_FCN_SUB_CLAMP               (2  << 12)
#       define COMB_FCN_SUB_NOCLAMP             (3  << 12)
#       define SRC_BLEND_GL_ZERO                (32 << 16)
#       define SRC_BLEND_GL_ONE                 (33 << 16)
#       define SRC_BLEND_GL_SRC_COLOR           (34 << 16)
#       define SRC_BLEND_GL_ONE_MINUS_SRC_COLOR (35 << 16)
#       define SRC_BLEND_GL_DST_COLOR           (36 << 16)
#       define SRC_BLEND_GL_ONE_MINUS_DST_COLOR (37 << 16)
#       define SRC_BLEND_GL_SRC_ALPHA           (38 << 16)
#       define SRC_BLEND_GL_ONE_MINUS_SRC_ALPHA (39 << 16)
#       define SRC_BLEND_GL_DST_ALPHA           (40 << 16)
#       define SRC_BLEND_GL_ONE_MINUS_DST_ALPHA (41 << 16)
#       define SRC_BLEND_GL_SRC_ALPHA_SATURATE  (42 << 16)
#       define SRC_BLEND_MASK                   (63 << 16)
#       define DST_BLEND_GL_ZERO                (32 << 24)
#       define DST_BLEND_GL_ONE                 (33 << 24)
#       define DST_BLEND_GL_SRC_COLOR           (34 << 24)
#       define DST_BLEND_GL_ONE_MINUS_SRC_COLOR (35 << 24)
#       define DST_BLEND_GL_DST_COLOR           (36 << 24)
#       define DST_BLEND_GL_ONE_MINUS_DST_COLOR (37 << 24)
#       define DST_BLEND_GL_SRC_ALPHA           (38 << 24)
#       define DST_BLEND_GL_ONE_MINUS_SRC_ALPHA (39 << 24)
#       define DST_BLEND_GL_DST_ALPHA           (40 << 24)
#       define DST_BLEND_GL_ONE_MINUS_DST_ALPHA (41 << 24)
#       define DST_BLEND_MASK                   (63 << 24)
#define RB3D_CNTL                    0x1c3c
#       define ALPHA_BLEND_ENABLE       (1  <<  0)
#       define PLANE_MASK_ENABLE        (1  <<  1)
#       define DITHER_ENABLE            (1  <<  2)
#       define ROUND_ENABLE             (1  <<  3)
#       define SCALE_DITHER_ENABLE      (1  <<  4)
#       define DITHER_INIT              (1  <<  5)
#       define ROP_ENABLE               (1  <<  6)
#       define STENCIL_ENABLE           (1  <<  7)
#       define Z_ENABLE                 (1  <<  8)
#       define DEPTH_XZ_OFFEST_ENABLE   (1  <<  9)
#       define COLOR_FORMAT_ARGB1555    (3  << 10)
#       define COLOR_FORMAT_RGB565      (4  << 10)
#       define COLOR_FORMAT_ARGB8888    (6  << 10)
#       define COLOR_FORMAT_RGB332      (7  << 10)
#       define COLOR_FORMAT_Y8          (8  << 10)
#       define COLOR_FORMAT_RGB8        (9  << 10)
#       define COLOR_FORMAT_YUV422_VYUY (11 << 10)
#       define COLOR_FORMAT_YUV422_YVYU (12 << 10)
#       define COLOR_FORMAT_aYUV444     (14 << 10)
#       define COLOR_FORMAT_ARGB4444    (15 << 10)
#       define CLRCMP_FLIP_ENABLE       (1  << 14)
#define RB3D_COLOROFFSET             0x1c40
#       define COLOROFFSET_MASK      0xfffffff0
#define RB3D_COLORPITCH              0x1c48
#       define COLORPITCH_MASK         0x000001ff8
#       define COLOR_TILE_ENABLE       (1 << 16)
#       define COLOR_MICROTILE_ENABLE  (1 << 17)
#       define COLOR_ENDIAN_NO_SWAP    (0 << 18)
#       define COLOR_ENDIAN_WORD_SWAP  (1 << 18)
#       define COLOR_ENDIAN_DWORD_SWAP (2 << 18)
#define RB3D_DEPTHOFFSET             0x1c24
#define RB3D_DEPTHPITCH              0x1c28
#       define DEPTHPITCH_MASK         0x00001ff8
#       define DEPTH_ENDIAN_NO_SWAP    (0 << 18)
#       define DEPTH_ENDIAN_WORD_SWAP  (1 << 18)
#       define DEPTH_ENDIAN_DWORD_SWAP (2 << 18)
#define RB3D_PLANEMASK               0x1d84
#define RB3D_ROPCNTL                 0x1d80
#       define ROP_MASK              (15 << 8)
#       define ROP_CLEAR             (0  << 8)
#       define ROP_NOR               (1  << 8)
#       define ROP_AND_INVERTED      (2  << 8)
#       define ROP_COPY_INVERTED     (3  << 8)
#       define ROP_AND_REVERSE       (4  << 8)
#       define ROP_INVERT            (5  << 8)
#       define ROP_XOR               (6  << 8)
#       define ROP_NAND              (7  << 8)
#       define ROP_AND               (8  << 8)
#       define ROP_EQUIV             (9  << 8)
#       define ROP_NOOP              (10 << 8)
#       define ROP_OR_INVERTED       (11 << 8)
#       define ROP_COPY              (12 << 8)
#       define ROP_OR_REVERSE        (13 << 8)
#       define ROP_OR                (14 << 8)
#       define ROP_SET               (15 << 8)
#define RB3D_STENCILREFMASK          0x1d7c
#       define STENCIL_REF_SHIFT       0
#       define STENCIL_REF_MASK        (0xff << 0)
#       define STENCIL_MASK_SHIFT      16
#       define STENCIL_VALUE_MASK      (0xff << 16)
#       define STENCIL_WRITEMASK_SHIFT 24
#       define STENCIL_WRITE_MASK      (0xff << 24)
#define RB3D_ZSTENCILCNTL            0x1c2c
#       define DEPTH_FORMAT_MASK          (0xf << 0)
#       define DEPTH_FORMAT_16BIT_INT_Z   (0  <<  0)
#       define DEPTH_FORMAT_24BIT_INT_Z   (2  <<  0)
#       define DEPTH_FORMAT_24BIT_FLOAT_Z (3  <<  0)
#       define DEPTH_FORMAT_32BIT_INT_Z   (4  <<  0)
#       define DEPTH_FORMAT_32BIT_FLOAT_Z (5  <<  0)
#       define DEPTH_FORMAT_16BIT_FLOAT_W (7  <<  0)
#       define DEPTH_FORMAT_24BIT_FLOAT_W (9  <<  0)
#       define DEPTH_FORMAT_32BIT_FLOAT_W (11 <<  0)
#       define Z_TEST_NEVER               (0  <<  4)
#       define Z_TEST_LESS                (1  <<  4)
#       define Z_TEST_LEQUAL              (2  <<  4)
#       define Z_TEST_EQUAL               (3  <<  4)
#       define Z_TEST_GEQUAL              (4  <<  4)
#       define Z_TEST_GREATER             (5  <<  4)
#       define Z_TEST_NEQUAL              (6  <<  4)
#       define Z_TEST_ALWAYS              (7  <<  4)
#       define Z_TEST_MASK                (7  <<  4)
#       define STENCIL_TEST_NEVER         (0  << 12)
#       define STENCIL_TEST_LESS          (1  << 12)
#       define STENCIL_TEST_LEQUAL        (2  << 12)
#       define STENCIL_TEST_EQUAL         (3  << 12)
#       define STENCIL_TEST_GEQUAL        (4  << 12)
#       define STENCIL_TEST_GREATER       (5  << 12)
#       define STENCIL_TEST_NEQUAL        (6  << 12)
#       define STENCIL_TEST_ALWAYS        (7  << 12)
#       define STENCIL_TEST_MASK          (0x7 << 12)
#       define STENCIL_FAIL_KEEP          (0  << 16)
#       define STENCIL_FAIL_ZERO          (1  << 16)
#       define STENCIL_FAIL_REPLACE       (2  << 16)
#       define STENCIL_FAIL_INC           (3  << 16)
#       define STENCIL_FAIL_DEC           (4  << 16)
#       define STENCIL_FAIL_INVERT        (5  << 16)
#       define STENCIL_FAIL_MASK          (0x7 << 16)
#       define STENCIL_ZPASS_KEEP         (0  << 20)
#       define STENCIL_ZPASS_ZERO         (1  << 20)
#       define STENCIL_ZPASS_REPLACE      (2  << 20)
#       define STENCIL_ZPASS_INC          (3  << 20)
#       define STENCIL_ZPASS_DEC          (4  << 20)
#       define STENCIL_ZPASS_INVERT       (5  << 20)
#       define STENCIL_ZPASS_MASK         (0x7 << 20)
#       define STENCIL_ZFAIL_KEEP         (0  << 24)
#       define STENCIL_ZFAIL_ZERO         (1  << 24)
#       define STENCIL_ZFAIL_REPLACE      (2  << 24)
#       define STENCIL_ZFAIL_INC          (3  << 24)
#       define STENCIL_ZFAIL_DEC          (4  << 24)
#       define STENCIL_ZFAIL_INVERT       (5  << 24)
#       define STENCIL_ZFAIL_MASK         (0x7 << 24)
#       define Z_COMPRESSION_ENABLE       (1  << 28)
#       define FORCE_Z_DIRTY              (1  << 29)
#       define Z_WRITE_ENABLE             (1  << 30)
#define RE_LINE_PATTERN              0x1cd0
#       define LINE_PATTERN_MASK             0x0000ffff
#       define LINE_REPEAT_COUNT_SHIFT       16
#       define LINE_PATTERN_START_SHIFT      24
#       define LINE_PATTERN_LITTLE_BIT_ORDER (0 << 28)
#       define LINE_PATTERN_BIG_BIT_ORDER    (1 << 28)
#       define LINE_PATTERN_AUTO_RESET       (1 << 29)
#define RE_LINE_STATE                0x1cd4
#       define LINE_CURRENT_PTR_SHIFT   0
#       define LINE_CURRENT_COUNT_SHIFT 8
#define RE_MISC                      0x26c4
#       define STIPPLE_COORD_MASK       0x1f
#       define STIPPLE_X_OFFSET_SHIFT   0
#       define STIPPLE_X_OFFSET_MASK    (0x1f << 0)
#       define STIPPLE_Y_OFFSET_SHIFT   8
#       define STIPPLE_Y_OFFSET_MASK    (0x1f << 8)
#       define STIPPLE_LITTLE_BIT_ORDER (0 << 16)
#       define STIPPLE_BIG_BIT_ORDER    (1 << 16)
#define RE_SOLID_COLOR               0x1c1c
#define RE_TOP_LEFT                  0x26c0
#       define RE_LEFT_SHIFT         0
#       define RE_TOP_SHIFT          16
#define RE_WIDTH_HEIGHT              0x1c44
#       define RE_WIDTH_SHIFT        0
#       define RE_HEIGHT_SHIFT       16

#define SE_CNTL                      0x1c4c
#       define FFACE_CULL_CW          (0 <<  0)
#       define FFACE_CULL_CCW         (1 <<  0)
#       define FFACE_CULL_DIR_MASK    (1 <<  0)
#       define BFACE_CULL             (0 <<  1)
#       define BFACE_SOLID            (3 <<  1)
#       define FFACE_CULL             (0 <<  3)
#       define FFACE_SOLID            (3 <<  3)
#       define FFACE_CULL_MASK        (3 <<  3)
#       define BADVTX_CULL_DISABLE    (1 <<  5)
#       define FLAT_SHADE_VTX_0       (0 <<  6)
#       define FLAT_SHADE_VTX_1       (1 <<  6)
#       define FLAT_SHADE_VTX_2       (2 <<  6)
#       define FLAT_SHADE_VTX_LAST    (3 <<  6)
#       define DIFFUSE_SHADE_SOLID    (0 <<  8)
#       define DIFFUSE_SHADE_FLAT     (1 <<  8)
#       define DIFFUSE_SHADE_GOURAUD  (2 <<  8)
#       define DIFFUSE_SHADE_MASK     (3 <<  8)
#       define ALPHA_SHADE_SOLID      (0 << 10)
#       define ALPHA_SHADE_FLAT       (1 << 10)
#       define ALPHA_SHADE_GOURAUD    (2 << 10)
#       define ALPHA_SHADE_MASK       (3 << 10)
#       define SPECULAR_SHADE_SOLID   (0 << 12)
#       define SPECULAR_SHADE_FLAT    (1 << 12)
#       define SPECULAR_SHADE_GOURAUD (2 << 12)
#       define SPECULAR_SHADE_MASK    (3 << 12)
#       define FOG_SHADE_SOLID        (0 << 14)
#       define FOG_SHADE_FLAT         (1 << 14)
#       define FOG_SHADE_GOURAUD      (2 << 14)
#       define FOG_SHADE_MASK         (3 << 14)
#       define ZBIAS_ENABLE_POINT     (1 << 16)
#       define ZBIAS_ENABLE_LINE      (1 << 17)
#       define ZBIAS_ENABLE_TRI       (1 << 18)
#       define WIDELINE_ENABLE        (1 << 20)
#       define VPORT_XY_XFORM_ENABLE  (1 << 24)
#       define VPORT_Z_XFORM_ENABLE   (1 << 25)
#       define VTX_PIX_CENTER_D3D     (0 << 27)
#       define VTX_PIX_CENTER_OGL     (1 << 27)
#       define ROUND_MODE_TRUNC       (0 << 28)
#       define ROUND_MODE_ROUND       (1 << 28)
#       define ROUND_MODE_ROUND_EVEN  (2 << 28)
#       define ROUND_MODE_ROUND_ODD   (3 << 28)
#       define ROUND_PREC_16TH_PIX    (0 << 30)
#       define ROUND_PREC_8TH_PIX     (1 << 30)
#       define ROUND_PREC_4TH_PIX     (2 << 30)
#       define ROUND_PREC_HALF_PIX    (3 << 30)
#define SE_CNTL_STATUS               0x2140
#       define VC_NO_SWAP            (0 << 0)
#       define VC_16BIT_SWAP         (1 << 0)
#       define VC_32BIT_SWAP         (2 << 0)
#       define VC_HALF_DWORD_SWAP    (3 << 0)
#       define TCL_BYPASS            (1 << 8)
#define SE_COORD_FMT                 0x1c50
#       define VTX_XY_PRE_MULT_1_OVER_W0  (1 <<  0)
#       define VTX_Z_PRE_MULT_1_OVER_W0   (1 <<  1)
#       define VTX_ST0_NONPARAMETRIC      (1 <<  8)
#       define VTX_ST1_NONPARAMETRIC      (1 <<  9)
#       define VTX_ST2_NONPARAMETRIC      (1 << 10)
#       define VTX_ST3_NONPARAMETRIC      (1 << 11)
#       define VTX_W0_NORMALIZE           (1 << 12)
#       define VTX_W0_IS_NOT_1_OVER_W0    (1 << 16)
#       define VTX_ST0_PRE_MULT_1_OVER_W0 (1 << 17)
#       define VTX_ST1_PRE_MULT_1_OVER_W0 (1 << 19)
#       define VTX_ST2_PRE_MULT_1_OVER_W0 (1 << 21)
#       define VTX_ST3_PRE_MULT_1_OVER_W0 (1 << 23)
#       define TEX1_W_ROUTING_USE_W0      (0 << 26)
#       define TEX1_W_ROUTING_USE_Q1      (1 << 26)
#define SE_LINE_WIDTH                0x1db8
#define SE_TCL_LIGHT_MODEL_CTL       0x226c
#       define LIGHTING_ENABLE              (1 << 0)
#       define LIGHT_IN_MODELSPACE          (1 << 1)
#       define LOCAL_VIEWER                 (1 << 2)
#       define NORMALIZE_NORMALS            (1 << 3)
#       define RESCALE_NORMALS              (1 << 4)
#       define SPECULAR_LIGHTS              (1 << 5)
#       define DIFFUSE_SPECULAR_COMBINE     (1 << 6)
#       define LIGHT_ALPHA                  (1 << 7)
#       define LOCAL_LIGHT_VEC_GL           (1 << 8)
#       define LIGHT_NO_NORMAL_AMBIENT_ONLY (1 << 9)
#       define LM_SOURCE_STATE_PREMULT      0
#       define LM_SOURCE_STATE_MULT         1
#       define LM_SOURCE_VERTEX_DIFFUSE     2
#       define LM_SOURCE_VERTEX_SPECULAR    3
#       define EMISSIVE_SOURCE_SHIFT        16
#       define AMBIENT_SOURCE_SHIFT         18
#       define DIFFUSE_SOURCE_SHIFT         20
#       define SPECULAR_SOURCE_SHIFT        22
#define SE_TCL_MATERIAL_AMBIENT_RED     0x2220
#define SE_TCL_MATERIAL_AMBIENT_GREEN   0x2224
#define SE_TCL_MATERIAL_AMBIENT_BLUE    0x2228
#define SE_TCL_MATERIAL_AMBIENT_ALPHA   0x222c
#define SE_TCL_MATERIAL_DIFFUSE_RED     0x2230
#define SE_TCL_MATERIAL_DIFFUSE_GREEN   0x2234
#define SE_TCL_MATERIAL_DIFFUSE_BLUE    0x2238
#define SE_TCL_MATERIAL_DIFFUSE_ALPHA   0x223c
#define SE_TCL_MATERIAL_EMMISSIVE_RED   0x2210
#define SE_TCL_MATERIAL_EMMISSIVE_GREEN 0x2214
#define SE_TCL_MATERIAL_EMMISSIVE_BLUE  0x2218
#define SE_TCL_MATERIAL_EMMISSIVE_ALPHA 0x221c
#define SE_TCL_MATERIAL_SPECULAR_RED    0x2240
#define SE_TCL_MATERIAL_SPECULAR_GREEN  0x2244
#define SE_TCL_MATERIAL_SPECULAR_BLUE   0x2248
#define SE_TCL_MATERIAL_SPECULAR_ALPHA  0x224c
#define SE_TCL_MATRIX_SELECT_0       0x225c
#       define MODELVIEW_0_SHIFT        0
#       define MODELVIEW_1_SHIFT        4
#       define MODELVIEW_2_SHIFT        8
#       define MODELVIEW_3_SHIFT        12
#       define IT_MODELVIEW_0_SHIFT     16
#       define IT_MODELVIEW_1_SHIFT     20
#       define IT_MODELVIEW_2_SHIFT     24
#       define IT_MODELVIEW_3_SHIFT     28
#define SE_TCL_MATRIX_SELECT_1       0x2260
#       define MODELPROJECT_0_SHIFT     0
#       define MODELPROJECT_1_SHIFT     4
#       define MODELPROJECT_2_SHIFT     8
#       define MODELPROJECT_3_SHIFT     12
#       define TEXMAT_0_SHIFT           16
#       define TEXMAT_1_SHIFT           20
#       define TEXMAT_2_SHIFT           24
#       define TEXMAT_3_SHIFT           28


#define SE_TCL_OUTPUT_VTX_FMT        0x2254
#       define TCL_VTX_W0                 (1 <<  0)
#       define TCL_VTX_FP_DIFFUSE         (1 <<  1)
#       define TCL_VTX_FP_ALPHA           (1 <<  2)
#       define TCL_VTX_PK_DIFFUSE         (1 <<  3)
#       define TCL_VTX_FP_SPEC            (1 <<  4)
#       define TCL_VTX_FP_FOG             (1 <<  5)
#       define TCL_VTX_PK_SPEC            (1 <<  6)
#       define TCL_VTX_ST0                (1 <<  7)
#       define TCL_VTX_ST1                (1 <<  8)
#       define TCL_VTX_Q1                 (1 <<  9)
#       define TCL_VTX_ST2                (1 << 10)
#       define TCL_VTX_Q2                 (1 << 11)
#       define TCL_VTX_ST3                (1 << 12)
#       define TCL_VTX_Q3                 (1 << 13)
#       define TCL_VTX_Q0                 (1 << 14)
#       define TCL_VTX_WEIGHT_COUNT_SHIFT 15
#       define TCL_VTX_NORM0              (1 << 18)
#       define TCL_VTX_XY1                (1 << 27)
#       define TCL_VTX_Z1                 (1 << 28)
#       define TCL_VTX_W1                 (1 << 29)
#       define TCL_VTX_NORM1              (1 << 30)
#       define TCL_VTX_Z0                 (1 << 31)

#define SE_TCL_OUTPUT_VTX_SEL        0x2258
#       define TCL_COMPUTE_XYZW           (1 << 0)
#       define TCL_COMPUTE_DIFFUSE        (1 << 1)
#       define TCL_COMPUTE_SPECULAR       (1 << 2)
#       define TCL_FORCE_NAN_IF_COLOR_NAN (1 << 3)
#       define TCL_FORCE_INORDER_PROC     (1 << 4)
#       define TCL_TEX_INPUT_TEX_0        0
#       define TCL_TEX_INPUT_TEX_1        1
#       define TCL_TEX_INPUT_TEX_2        2
#       define TCL_TEX_INPUT_TEX_3        3
#       define TCL_TEX_COMPUTED_TEX_0     8
#       define TCL_TEX_COMPUTED_TEX_1     9
#       define TCL_TEX_COMPUTED_TEX_2     10
#       define TCL_TEX_COMPUTED_TEX_3     11
#       define TCL_TEX_0_OUTPUT_SHIFT     16
#       define TCL_TEX_1_OUTPUT_SHIFT     20
#       define TCL_TEX_2_OUTPUT_SHIFT     24
#       define TCL_TEX_3_OUTPUT_SHIFT     28

#define SE_TCL_PER_LIGHT_CTL_0       0x2270
#       define LIGHT_0_ENABLE               (1 <<  0)
#       define LIGHT_0_ENABLE_AMBIENT       (1 <<  1)
#       define LIGHT_0_ENABLE_SPECULAR      (1 <<  2)
#       define LIGHT_0_IS_LOCAL             (1 <<  3)
#       define LIGHT_0_IS_SPOT              (1 <<  4)
#       define LIGHT_0_DUAL_CONE            (1 <<  5)
#       define LIGHT_0_ENABLE_RANGE_ATTEN   (1 <<  6)
#       define LIGHT_0_CONSTANT_RANGE_ATTEN (1 <<  7)
#       define LIGHT_0_SHIFT                0
#       define LIGHT_1_ENABLE               (1 << 16)
#       define LIGHT_1_ENABLE_AMBIENT       (1 << 17)
#       define LIGHT_1_ENABLE_SPECULAR      (1 << 18)
#       define LIGHT_1_IS_LOCAL             (1 << 19)
#       define LIGHT_1_IS_SPOT              (1 << 20)
#       define LIGHT_1_DUAL_CONE            (1 << 21)
#       define LIGHT_1_ENABLE_RANGE_ATTEN   (1 << 22)
#       define LIGHT_1_CONSTANT_RANGE_ATTEN (1 << 23)
#       define LIGHT_1_SHIFT                16
#define SE_TCL_PER_LIGHT_CTL_1       0x2274
#       define LIGHT_2_SHIFT            0
#       define LIGHT_3_SHIFT            16
#define SE_TCL_PER_LIGHT_CTL_2       0x2278
#       define LIGHT_4_SHIFT            0
#       define LIGHT_5_SHIFT            16
#define SE_TCL_PER_LIGHT_CTL_3       0x227c
#       define LIGHT_6_SHIFT            0
#       define LIGHT_7_SHIFT            16

#define SE_TCL_SHININESS             0x2250

#define SE_TCL_TEXTURE_PROC_CTL      0x2268
#       define TEXGEN_TEXMAT_0_ENABLE      (1 << 0)
#       define TEXGEN_TEXMAT_1_ENABLE      (1 << 1)
#       define TEXGEN_TEXMAT_2_ENABLE      (1 << 2)
#       define TEXGEN_TEXMAT_3_ENABLE      (1 << 3)
#       define TEXMAT_0_ENABLE             (1 << 4)
#       define TEXMAT_1_ENABLE             (1 << 5)
#       define TEXMAT_2_ENABLE             (1 << 6)
#       define TEXMAT_3_ENABLE             (1 << 7)
#       define TEXGEN_INPUT_MASK           0xf
#       define TEXGEN_INPUT_TEXCOORD_0     0
#       define TEXGEN_INPUT_TEXCOORD_1     1
#       define TEXGEN_INPUT_TEXCOORD_2     2
#       define TEXGEN_INPUT_TEXCOORD_3     3
#       define TEXGEN_INPUT_OBJ            4
#       define TEXGEN_INPUT_EYE            5
#       define TEXGEN_INPUT_EYE_NORMAL     6
#       define TEXGEN_INPUT_EYE_REFLECT    7
#       define TEXGEN_INPUT_EYE_NORMALIZED 8
#       define TEXGEN_0_INPUT_SHIFT        16
#       define TEXGEN_1_INPUT_SHIFT        20
#       define TEXGEN_2_INPUT_SHIFT        24
#       define TEXGEN_3_INPUT_SHIFT        28

#define SE_TCL_UCP_VERT_BLEND_CTL    0x2264
#       define UCP_IN_CLIP_SPACE            (1 <<  0)
#       define UCP_IN_MODEL_SPACE           (1 <<  1)
#       define UCP_ENABLE_0                 (1 <<  2)
#       define UCP_ENABLE_1                 (1 <<  3)
#       define UCP_ENABLE_2                 (1 <<  4)
#       define UCP_ENABLE_3                 (1 <<  5)
#       define UCP_ENABLE_4                 (1 <<  6)
#       define UCP_ENABLE_5                 (1 <<  7)
#       define TCL_FOG_MASK                 (3 <<  8)
#       define TCL_FOG_DISABLE              (0 <<  8)
#       define TCL_FOG_EXP                  (1 <<  8)
#       define TCL_FOG_EXP2                 (2 <<  8)
#       define TCL_FOG_LINEAR               (3 <<  8)
#       define RNG_BASED_FOG                (1 << 10)
#       define LIGHT_TWOSIDE                (1 << 11)
#       define BLEND_OP_COUNT_MASK          (7 << 12)
#       define BLEND_OP_COUNT_SHIFT         12
#       define POSITION_BLEND_OP_ENABLE     (1 << 16)
#       define NORMAL_BLEND_OP_ENABLE       (1 << 17)
#       define VERTEX_BLEND_SRC_0_PRIMARY   (1 << 18)
#       define VERTEX_BLEND_SRC_0_SECONDARY (1 << 18)
#       define VERTEX_BLEND_SRC_1_PRIMARY   (1 << 19)
#       define VERTEX_BLEND_SRC_1_SECONDARY (1 << 19)
#       define VERTEX_BLEND_SRC_2_PRIMARY   (1 << 20)
#       define VERTEX_BLEND_SRC_2_SECONDARY (1 << 20)
#       define VERTEX_BLEND_SRC_3_PRIMARY   (1 << 21)
#       define VERTEX_BLEND_SRC_3_SECONDARY (1 << 21)
#       define VERTEX_BLEND_WGT_MINUS_ONE   (1 << 22)
#       define CULL_FRONT_IS_CW             (0 << 28)
#       define CULL_FRONT_IS_CCW            (1 << 28)
#       define CULL_FRONT                   (1 << 29)
#       define CULL_BACK                    (1 << 30)
#       define FORCE_W_TO_ONE               (1 << 31)

#define SE_VPORT_XSCALE              0x1d98
#define SE_VPORT_XOFFSET             0x1d9c
#define SE_VPORT_YSCALE              0x1da0
#define SE_VPORT_YOFFSET             0x1da4
#define SE_VPORT_ZSCALE              0x1da8
#define SE_VPORT_ZOFFSET             0x1dac
#define SE_ZBIAS_FACTOR              0x1db0
#define SE_ZBIAS_CONSTANT            0x1db4



				/* Registers for CP and Microcode Engine */
#define CP_ME_RAM_ADDR               0x07d4
#define CP_ME_RAM_RADDR              0x07d8
#define CP_ME_RAM_DATAH              0x07dc
#define CP_ME_RAM_DATAL              0x07e0

#define CP_RB_BASE                   0x0700
#define CP_RB_CNTL                   0x0704
#define CP_RB_RPTR_ADDR              0x070c
#define CP_RB_RPTR                   0x0710
#define CP_RB_WPTR                   0x0714

#define CP_IB_BASE                   0x0738
#define CP_IB_BUFSZ                  0x073c

#define CP_CSQ_CNTL                  0x0740
#       define CSQ_CNT_PRIMARY_MASK     (0xff << 0)
#       define CSQ_PRIDIS_INDDIS        (0    << 28)
#       define CSQ_PRIPIO_INDDIS        (1    << 28)
#       define CSQ_PRIBM_INDDIS         (2    << 28)
#       define CSQ_PRIPIO_INDBM         (3    << 28)
#       define CSQ_PRIBM_INDBM          (4    << 28)
#       define CSQ_PRIPIO_INDPIO        (15   << 28)
#define CP_CSQ_STAT                  0x07f8
#       define CSQ_RPTR_PRIMARY_MASK    (0xff <<  0)
#       define CSQ_WPTR_PRIMARY_MASK    (0xff <<  8)
#       define CSQ_RPTR_INDIRECT_MASK   (0xff << 16)
#       define CSQ_WPTR_INDIRECT_MASK   (0xff << 24)
#define CP_CSQ_ADDR                  0x07f0
#define CP_CSQ_DATA                  0x07f4
#define CP_CSQ_APER_PRIMARY          0x1000
#define CP_CSQ_APER_INDIRECT         0x1300

#define CP_RB_WPTR_DELAY             0x0718
#       define PRE_WRITE_TIMER_SHIFT    0
#       define PRE_WRITE_LIMIT_SHIFT    23

#define AIC_CNTL                     0x01d0
#       define PCIGART_TRANSLATE_EN     (1 << 0)


/* Constants */
#define AGP_TEX_OFFSET               0x02000000

#define LAST_FRAME_REG               GUI_SCRATCH_REG0
#define LAST_CLEAR_REG               GUI_SCRATCH_REG2


/* CP packet types */
#define CP_PACKET0                           0x00000000
#define CP_PACKET1                           0x40000000
#define CP_PACKET2                           0x80000000
#define CP_PACKET3                           0xC0000000
#       define CP_PACKET_MASK                0xC0000000
#       define CP_PACKET_COUNT_MASK          0x3fff0000
#       define CP_PACKET_MAX_DWORDS          (1 << 12)
#       define CP_PACKET0_REG_MASK           0x000007ff
#       define CP_PACKET1_REG0_MASK          0x000007ff
#       define CP_PACKET1_REG1_MASK          0x003ff800

#define CP_PACKET0_ONE_REG_WR                0x00008000

#define CP_PACKET3_NOP                       0xC0001000
#define CP_PACKET3_NEXT_CHAR                 0xC0001900
#define CP_PACKET3_PLY_NEXTSCAN              0xC0001D00
#define CP_PACKET3_SET_SCISSORS              0xC0001E00
#define CP_PACKET3_3D_RNDR_GEN_INDX_PRIM     0xC0002300
#define CP_PACKET3_LOAD_MICROCODE            0xC0002400
#define CP_PACKET3_WAIT_FOR_IDLE             0xC0002600
#define CP_PACKET3_3D_DRAW_VBUF              0xC0002800
#define CP_PACKET3_3D_DRAW_IMMD              0xC0002900
#define CP_PACKET3_3D_DRAW_INDX              0xC0002A00
#define CP_PACKET3_LOAD_PALETTE              0xC0002C00
#define CP_PACKET3_3D_LOAD_VBPNTR            0xC0002F00
#define CP_PACKET3_CNTL_PAINT                0xC0009100
#define CP_PACKET3_CNTL_BITBLT               0xC0009200
#define CP_PACKET3_CNTL_SMALLTEXT            0xC0009300
#define CP_PACKET3_CNTL_HOSTDATA_BLT         0xC0009400
#define CP_PACKET3_CNTL_POLYLINE             0xC0009500
#define CP_PACKET3_CNTL_POLYSCANLINES        0xC0009800
#define CP_PACKET3_CNTL_PAINT_MULTI          0xC0009A00
#define CP_PACKET3_CNTL_BITBLT_MULTI         0xC0009B00
#define CP_PACKET3_CNTL_TRANS_BITBLT         0xC0009C00

#define CP_VC_FRMT_XY                        0x00000000
#define CP_VC_FRMT_W0                        0x00000001
#define CP_VC_FRMT_FPCOLOR                   0x00000002
#define CP_VC_FRMT_FPALPHA                   0x00000004
#define CP_VC_FRMT_PKCOLOR                   0x00000008
#define CP_VC_FRMT_FPSPEC                    0x00000010
#define CP_VC_FRMT_FPFOG                     0x00000020
#define CP_VC_FRMT_PKSPEC                    0x00000040
#define CP_VC_FRMT_ST0                       0x00000080
#define CP_VC_FRMT_ST1                       0x00000100
#define CP_VC_FRMT_Q1                        0x00000200
#define CP_VC_FRMT_ST2                       0x00000400
#define CP_VC_FRMT_Q2                        0x00000800
#define CP_VC_FRMT_ST3                       0x00001000
#define CP_VC_FRMT_Q3                        0x00002000
#define CP_VC_FRMT_Q0                        0x00004000
#define CP_VC_FRMT_BLND_WEIGHT_CNT_MASK      0x00038000
#define CP_VC_FRMT_N0                        0x00040000
#define CP_VC_FRMT_XY1                       0x08000000
#define CP_VC_FRMT_Z1                        0x10000000
#define CP_VC_FRMT_W1                        0x20000000
#define CP_VC_FRMT_N1                        0x40000000
#define CP_VC_FRMT_Z                         0x80000000

#define CP_VC_CNTL_PRIM_TYPE_NONE            0x00000000
#define CP_VC_CNTL_PRIM_TYPE_POINT           0x00000001
#define CP_VC_CNTL_PRIM_TYPE_LINE            0x00000002
#define CP_VC_CNTL_PRIM_TYPE_LINE_STRIP      0x00000003
#define CP_VC_CNTL_PRIM_TYPE_TRI_LIST        0x00000004
#define CP_VC_CNTL_PRIM_TYPE_TRI_FAN         0x00000005
#define CP_VC_CNTL_PRIM_TYPE_TRI_STRIP       0x00000006
#define CP_VC_CNTL_PRIM_TYPE_TRI_TYPE_2      0x00000007
#define CP_VC_CNTL_PRIM_TYPE_RECT_LIST       0x00000008
#define CP_VC_CNTL_PRIM_TYPE_3VRT_POINT_LIST 0x00000009
#define CP_VC_CNTL_PRIM_TYPE_3VRT_LINE_LIST  0x0000000a
#define CP_VC_CNTL_PRIM_WALK_IND             0x00000010
#define CP_VC_CNTL_PRIM_WALK_LIST            0x00000020
#define CP_VC_CNTL_PRIM_WALK_RING            0x00000030
#define CP_VC_CNTL_COLOR_ORDER_BGRA          0x00000000
#define CP_VC_CNTL_COLOR_ORDER_RGBA          0x00000040
#define CP_VC_CNTL_MAOS_ENABLE               0x00000080
#define CP_VC_CNTL_VTX_FMT_NON_MODE   0x00000000
#define CP_VC_CNTL_VTX_FMT_MODE       0x00000100
#define CP_VC_CNTL_TCL_DISABLE               0x00000000
#define CP_VC_CNTL_TCL_ENABLE                0x00000200
#define CP_VC_CNTL_NUM_SHIFT                 16

#define VS_MATRIX_0_ADDR                   0
#define VS_MATRIX_1_ADDR                   4
#define VS_MATRIX_2_ADDR                   8
#define VS_MATRIX_3_ADDR                  12
#define VS_MATRIX_4_ADDR                  16
#define VS_MATRIX_5_ADDR                  20
#define VS_MATRIX_6_ADDR                  24
#define VS_MATRIX_7_ADDR                  28
#define VS_MATRIX_8_ADDR                  32
#define VS_MATRIX_9_ADDR                  36
#define VS_MATRIX_10_ADDR                 40
#define VS_MATRIX_11_ADDR                 44
#define VS_MATRIX_12_ADDR                 48
#define VS_MATRIX_13_ADDR                 52
#define VS_MATRIX_14_ADDR                 56
#define VS_MATRIX_15_ADDR                 60
#define VS_LIGHT_AMBIENT_ADDR             64
#define VS_LIGHT_DIFFUSE_ADDR             72
#define VS_LIGHT_SPECULAR_ADDR            80
#define VS_LIGHT_DIRPOS_ADDR              88
#define VS_LIGHT_HWVSPOT_ADDR             96
#define VS_LIGHT_ATTENUATION_ADDR        104
#define VS_MATRIX_EYE2CLIP_ADDR          112
#define VS_UCP_ADDR                      116
#define VS_GLOBAL_AMBIENT_ADDR           122
#define VS_FOG_PARAM_ADDR                123
#define VS_EYE_VECTOR_ADDR               124

#define SS_LIGHT_DCD_ADDR                  0
#define SS_LIGHT_SPOT_EXPONENT_ADDR        8
#define SS_LIGHT_SPOT_CUTOFF_ADDR         16
#define SS_LIGHT_SPECULAR_THRESH_ADDR     24
#define SS_LIGHT_RANGE_CUTOFF_ADDR        32
#define SS_VERT_GUARD_CLIP_ADJ_ADDR       48
#define SS_VERT_GUARD_DISCARD_ADJ_ADDR    49
#define SS_HORZ_GUARD_CLIP_ADJ_ADDR       50
#define SS_HORZ_GUARD_DISCARD_ADJ_ADDR    51
#define SS_SHININESS                      60


/* ATI Technologies Inc */
#define ATI_PCIVID		0x1002

struct pciids {
	ushort	did;
	int	type;
	char*	name;
};

enum {
	ATI_R300,
	ATI_RV250,
	ATI_R250,
	ATI_RV200,
	ATI_R200,
	ATI_RV100,
	ATI_R100,
	ATI_M6,
	ATI_M7,
	ATI_M9,
};

struct pciids radeon_pciids[] = {
	/* ATI_M6, LY */
	/* ATI_M6, LZ */

	0x4c57, ATI_M7,		"Radeon Mobility M7 LW [Radeon Mobility 7500]",
	0x4c58, ATI_M7,		"Radeon RV200 LX [Mobility FireGL 7800 M7]",

	0x4c64, ATI_M9,		"Radeon R250 Ld [Radeon Mobility 9000 M9]",
	0x4c65, ATI_M9,		"Radeon R250 Le [Radeon Mobility 9000 M9]",
	0x4c66, ATI_M9,		"Radeon R250 Lf [Radeon Mobility 9000 M9]",
	0x4c67, ATI_M9,		"Radeon R250 Lg [Radeon Mobility 9000 M9]",

	0x5159, ATI_RV100,	"Radeon RV100 QY [Radeon 7000/VE]",
	0x515a, ATI_RV100,	"Radeon RV100 QZ [Radeon 7000/VE]",

	0x5144, ATI_R100,	"Radeon R100 QD [Radeon 7200]",
	0x5145, ATI_R100,	"Radeon R100 QE",
	0x5146, ATI_R100,	"Radeon R100 QF",
	0x5147, ATI_R100,	"Radeon R100 QG",

	0x5148, ATI_R200,	"Radeon R200 QH [Radeon 8500]",
	0x5149, ATI_R200,	"Radeon R200 QI",
	0x514a, ATI_R200,	"Radeon R200 QJ",
	0x514b, ATI_R200,	"Radeon R200 QK",
	0x514c, ATI_R200,	"Radeon R200 QL [Radeon 8500 LE]",
	0x514d, ATI_R200,	"Radeon R200 QM [Radeon 9100]",
	0x514e, ATI_R200,	"Radeon R200 QN [Radeon 8500LE]",
	0x514f, ATI_R200,	"Radeon R200 QO [Radeon 8500LE]",
	0x5168, ATI_R200,	"Radeon R200 Qh",
	0x5169, ATI_R200,	"Radeon R200 Qi",
	0x516a, ATI_R200,	"Radeon R200 Qj",
	0x516b, ATI_R200,	"Radeon R200 Qk",
	0x516c, ATI_R200,	"Radeon R200 Ql",

	0x5157, ATI_RV200,	"Radeon RV200 QW [Radeon 7500]",
	0x5158, ATI_RV200,	"Radeon RV200 QX [Radeon 7500]",

	0x4964, ATI_RV250, 	"Radeon R250 Id [Radeon 9000]",
	0x4965, ATI_RV250, 	"Radeon R250 Ie [Radeon 9000]",
	0x4966, ATI_RV250, 	"Radeon R250 If [Radeon 9000]",
	0x4967, ATI_RV250,	"Radeon R250 Ig [Radeon 9000]",

	0x4144, ATI_R300, 	"Radeon R300 AD [Radeon 9500 Pro]",
	0x4145, ATI_R300, 	"Radeon R300 AE [Radeon 9500 Pro]",
	0x4146, ATI_R300, 	"Radeon R300 AF [Radeon 9500 Pro]",
	0x4147, ATI_R300, 	"Radeon R300 AG [FireGL Z1/X1]",
	0x4e44, ATI_R300,	"Radeon R300 ND [Radeon 9700]",
	0x4e45, ATI_R300,	"Radeon R300 NE [Radeon 9700]",
	0x4e46, ATI_R300,	"Radeon R300 NF [Radeon 9700]",
	0x4e47, ATI_R300,	"Radeon R300 NG [FireGL X1]",

	0x4e64, ATI_R300,	"Radeon R300 [Radeon 9700 Pro] (Secondary)",
	0x4e65, ATI_R300,	"Radeon R300 [Radeon 9700] (Secondary)",
	0x4e66, ATI_R300,	"Radeon R300 [Radeon 9700] (Secondary)",
	0x4e67, ATI_R300,	"Radeon R300 [FireGL X1] (Secondary)",

	0
};
