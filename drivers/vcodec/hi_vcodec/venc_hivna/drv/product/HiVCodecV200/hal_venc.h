
#ifndef __HAL_VENC_H__
#define __HAL_VENC_H__

#include "hi_type.h"
#include "soc_venc_reg_interface.h"
#include "drv_venc_ioctl.h"

#define VENC_COMPATIBLE       "hisi,kirin970-venc"
#define VENC_ES_SUPPORT
#define VENC_CS_SUPPORT
#define VENC_FPGAFLAG_ES      "fpga_flag_es"
#define VENC_FPGAFLAG_CS      "fpga_flag_cs"
#define VENC_PCTRL_PERI_ES    0xE8A090A4
#define VENC_PCTRL_PERI_CS    0xE8A090BC
#define VENC_EXISTBIT_ES      0x4
#define VENC_EXISTBIT_CS      0x40000

HI_VOID venc_hal_clr_all_int(S_HEVC_AVC_REGS_TYPE *vedu_reg);
HI_VOID venc_hal_disable_all_int(S_HEVC_AVC_REGS_TYPE *vedu_reg);
HI_VOID venc_hal_start_encode(S_HEVC_AVC_REGS_TYPE *vedu_reg);
HI_VOID venc_hal_get_reg_venc(struct stream_info *stream_info, HI_U32 *reg_base);
HI_VOID vedu_hal_cfg_reg_intraset(struct encode_info *channel_cfg, HI_U32 *reg_base);
HI_VOID vedu_hal_cfg_reg_lambda_set(struct encode_info *channel_cfg, HI_U32 *reg_base);
HI_VOID vedu_hal_cfg_reg_qpg_map_set(struct encode_info *channel_cfg, HI_U32 *reg_base);
HI_VOID vedu_hal_cfg_reg_addr_set(struct encode_info *channel_cfg, HI_U32 *reg_base);
HI_VOID vedu_hal_cfg_reg_slc_head_set(struct encode_info *channel_cfg, HI_U32 *reg_base);
HI_VOID vedu_hal_cfg_reg_smmu_set(struct encode_info *channel_cfg, HI_U32 *reg_base);
HI_VOID vedu_hal_cfg_reg_premmu_set(struct encode_info *channel_cfg, HI_U32 *reg_base);
HI_VOID vedu_hal_cfg_reg_simple(struct encode_info *channel_cfg, HI_U32 *reg_base, HI_BOOL *first_cfg_flag);
HI_VOID vedu_hal_cfg_reg(struct encode_info *regcfginfo, HI_U32 *reg_base, HI_BOOL *first_cfg_flag);
HI_VOID venc_hal_set_smmu_addr(S_HEVC_AVC_REGS_TYPE *vedu_reg);


#endif				//__HAL_VENC_H__

