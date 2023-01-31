#ifndef __IMX_USDHC__
#define __IMX_USDHC__

#include <inc/types.h>
#include <fs/sd_card.h>
#include <imx6/imx6ul.h>

typedef struct imx_usdhc_tag
{
    volatile uint32_t dma_sys_addr;         //<00h
    volatile uint32_t blk_att;              //<04h
    volatile uint32_t cmd_arg;              //<08h
    volatile uint32_t cmd_xfr_type;         //<0Ch
    volatile uint32_t cmd_rsp0;             //<10h
    volatile uint32_t cmd_rsp1;             //<14h
    volatile uint32_t cmd_rsp2;             //<18h
    volatile uint32_t cmd_rsp3;             //<1Ch
    volatile uint32_t data_buff_acc_port;   //<20H
    volatile uint32_t pres_state;           //<24h
    volatile uint32_t prot_ctrl;            //<28h
    volatile uint32_t sys_ctrl;             //<2Ch
    volatile uint32_t int_status;           //<30h
    volatile uint32_t int_status_en;        //<34h
    volatile uint32_t int_singal_en;        //<38h
    volatile uint32_t autocmd12_err_status; //<3Ch
    volatile uint32_t host_ctrl_cap;        //<40h
    volatile uint32_t wtmk_lvl;             //<44h
    volatile uint32_t mix_ctrl;             //<48h
    volatile uint32_t reserve1;             //<4Ch
    volatile uint32_t force_event;          //<50h
    volatile uint32_t adma_error_status;    //<54h
    volatile uint32_t adma_sys_addr;        //<58h
    volatile uint32_t reserve2;             //<5Ch
    volatile uint32_t dll_ctrl;             //<60h
    volatile uint32_t dll_status;           //<64h
    volatile uint32_t clk_tune_ctrl_status; //<68h
    volatile uint32_t reserve3;             //<6Ch
    volatile uint32_t reserve4[20];         //<70h-BFh
    volatile uint32_t vend_spec;            //<C0h
    volatile uint32_t mmc_boot;             //<C4h
    volatile uint32_t vend_spec2;           //<C8h
    volatile uint32_t tuning_ctrl;          //<CCh
} imx_usdhc_t;

// #define USDHC_BLKATT_BLKSIZE_SHIFT 0UL
// #define USDHC_BLKATT_BLKSIZE_MASK (0xFFFUL << USDHC_BLKATT_BLKSIZE_SHIFT)

#define USDHC_BLKATT_BLKCNT_SHIFT 16UL
#define USDHC_BLKATT_BLKCNT_MASK (0xFFFFUL << USDHC_BLKATT_BLKCNT_SHIFT)

#define USDHC_CMD_XFRTYPE_CMDINX_SHIFT 24UL
#define USDHC_CMD_XFRTYPE_CMDINX_MASK (0x3FUL << USDHC_CMD_XFRTYPE_CMDINX_SHIFT)

// #define USDHC_CMD_XFRTYPE_CMDTYPE_SHIFT 22UL
// #define USDHC_CMD_XFRTYPE_CMDTYPE_MASK (0x3UL << USDHC_CMD_XFRTYPE_CMDTYPE_SHIFT)

#define USDHC_CMD_XFRTYPE_DPSEL_SHIFT 21UL
#define USDHC_CMD_XFRTYPE_DPSEL_MASK (1UL << USDHC_CMD_XFRTYPE_DPSEL_SHIFT)

#define USDHC_CMD_XFRTYPE_CICEN_SHIFT 20UL
#define USDHC_CMD_XFRTYPE_CICEN_MASK (1UL << USDHC_CMD_XFRTYPE_CICEN_SHIFT)

#define USDHC_CMD_XFRTYPE_CCCEN_SHIFT 19UL
#define USDHC_CMD_XFRTYPE_CCCEN_MASK (1UL << USDHC_CMD_XFRTYPE_CCCEN_SHIFT)

#define USDHC_CMD_XFRTYPE_RSPTYPE_SHIFT 16UL
#define USDHC_CMD_XFRTYPE_RSPTYPE_MASK (0x3UL << USDHC_CMD_XFRTYPE_RSPTYPE_SHIFT)

// #define USDHC_PRES_STATE_DLSL_SHIFT 24UL
// #define USDHC_PRES_STATE_DLSL_MASK (0xFFUL << USDHC_PRES_STATE_DLSL_SHIFT)

// #define USDHC_PRES_STATE_CLSL_SHIFT 23UL
// #define USDHC_PRES_STATE_CLSL_MASK (1UL << USDHC_PRES_STATE_CLSL_SHIFT)

// #define USDHC_PRES_STATE_WPSPL_SHIFT 19UL
// #define USDHC_PRES_STATE_WPSPL_MASK (1UL << USDHC_PRES_STATE_WPSPL_SHIFT)

// #define USDHC_PRES_STATE_CDPL_SHIFT 18UL
// #define USDHC_PRES_STATE_CDPL_MASK (1UL << USDHC_PRES_STATE_CDPL_SHIFT)

// #define USDHC_PRES_STATE_CINST_SHIFT 16UL
// #define USDHC_PRES_STATE_CINST_MASK (1UL << USDHC_PRES_STATE_CINST_SHIFT)

// #define USDHC_PRES_STATE_TSCD_SHIFT 15UL
// #define USDHC_PRES_STATE_TSCD_MASK (1UL << USDHC_PRES_STATE_TSCD_SHIFT)

// #define USDHC_PRES_STATE_RTR_SHIFT 12UL
// #define USDHC_PRES_STATE_RTR_MASK (1UL << USDHC_PRES_STATE_RTR_SHIFT)

// #define USDHC_PRES_STATE_BREN_SHIFT 11UL
// #define USDHC_PRES_STATE_BREN_MASK (1UL << USDHC_PRES_STATE_BREN_SHIFT)

// #define USDHC_PRES_STATE_BWEN_SHIFT 10UL
// #define USDHC_PRES_STATE_BWEN_MASK (1UL << USDHC_PRES_STATE_BWEN_SHIFT)

// #define USDHC_PRES_STATE_RTA_SHIFT 9UL
// #define USDHC_PRES_STATE_RTA_MASK (1UL << USDHC_PRES_STATE_RTA_SHIFT)

// #define USDHC_PRES_STATE_WTA_SHIFT 8UL
// #define USDHC_PRES_STATE_WTA_MASK (1UL << USDHC_PRES_STATE_WTA_SHIFT)

// #define USDHC_PRES_STATE_SDOFF_SHIFT 7UL
// #define USDHC_PRES_STATE_SDOFF_MASK (1UL << USDHC_PRES_STATE_SDOFF_SHIFT)

// #define USDHC_PRES_STATE_PEROFF_SHIFT 6UL
// #define USDHC_PRES_STATE_PEROFF_MASK (1UL << USDHC_PRES_STATE_PEROFF_SHIFT)

// #define USDHC_PRES_STAET_HCKOFF_SHIFT 5UL
// #define USDHC_PRES_STATE_HCKOFF_MASK (1UL << USDHC_PRES_STAET_HCKOFF_SHIFT)

// #define USDHC_PRES_STATE_IPGOFF_SHIFT 4UL
// #define USDHC_PRES_STATE_IPGOFF_MASK (1UL << USDHC_PRES_STATE_IPGOFF_SHIFT)

// #define USDHC_PRES_STAET_SDSTB_SHIFT 3UL
// #define USDHC_PRES_STATE_SDSTB_MASK (1UL << USDHC_PRES_STAET_SDSTB_SHIFT)

// #define USDHC_PRES_STATE_DLA_SHIFT 2UL
// #define USDHC_PRES_STATE_DLA_MASK (1UL << USDHC_PRES_STATE_DLA_SHIFT)

// #define USDHC_PRES_STAET_CDIHB_SHIFT 1UL
// #define USDHC_PRES_STATE_CDIHB_MASK (1UL << USDHC_PRES_STAET_CDIHB_SHIFT)

// #define USDHC_PRES_STATE_CIHB_SHIFT 0UL
// #define USDHC_PRES_STATE_CIHB_MASK (1UL << USDHC_PRES_STATE_CIHB_SHIFT)

// #define USDHC_PROT_CTRL_NON_EXACTLK_RD_SHIFT 30UL
// #define USDHC_PROT_CTRL_NON_EXACTLK_RD_MASK (1UL << USDHC_PROT_CTRL_NON_EXACTLK_RD_SHIFT)

// #define USDHC_PROT_CTRL_BURST_LEN_EN_SHIFT 27UL
// #define USDHC_PROT_CTRL_BURST_LEN_EN_MASK (0x7UL << USDHC_PROT_CTRL_BURST_LEN_EN_SHIFT)

// #define USDHC_PROT_CTRL_WECRM_SHIFT 26UL
// #define USDHC_PROT_CTRL_WECRM_MASK (1UL << USDHC_PROT_CTRL_WECRM_SHIFT)

// #define USDHC_PROT_CTRL_WECINS_SHIFT 25UL
// #define USDHC_PROT_CTRL_WECINS_MASK (1UL << USDHC_PROT_CTRL_WECINS_SHIFT)

// #define USDHC_PROT_CTRL_WECINT_SHIFT 24UL
// #define USDHC_PROT_CTRL_WECINT_MASK (1UL << USDHC_PROT_CTRL_WECINT_SHIFT)

// #define USDHC_PROT_CTRL_RD_DONE_NO_8CLK_SHIFT 20UL
// #define USDHC_PROT_CTRL_RD_DONE_NO_8CLK_MASK (1UL << USDHC_PROT_CTRL_RD_DONE_NO_8CLK_SHIFT)

// #define USDHC_PROT_CTRL_IABG_SHIFT 19UL
// #define USDHC_PROT_CTRL_IABG_MASK (1UL << USDHC_PROT_CTRL_IABG_SHIFT)

// #define USDHC_PROT_CTRL_RWCTL_SHIFT 18UL
// #define USDHC_PROT_CTRL_RWCTL_MASK (1UL << USDHC_PROT_CTRL_RWCTL_SHIFT)

// #define USDHC_PROT_CTRL_CREQ_SHIFT 17UL
// #define USDHC_PROT_CTRL_CREQ_MASK (1UL << USDHC_PROT_CTRL_CREQ_SHIFT)

// #define USDHC_PROT_CTRL_SABGREQ_SHIFT 16UL
// #define USDHC_PROT_CTRL_SABGREQ_MASK (1UL << USDHC_PROT_CTRL_SABGREQ_SHIFT)

// #define USDHC_PROT_CTRL_DMASEL_SHIFT 8UL
// #define USDHC_PROT_CTRL_DMASEL_MASK (0x3UL << USDHC_PROT_CTRL_DMASEL_SHIFT)

// #define USDHC_PROT_CTRL_CDSS_SHIFT 7UL
// #define USDHC_PROT_CTRL_CDSS_MASK (1UL << USDHC_PROT_CTRL_CDSS_SHIFT)

// #define USDHC_PROT_CTRL_CDTL_SHIFT 6UL
// #define USDHC_PROT_CTRL_CDTL_MASK (1UL << USDHC_PROT_CTRL_CDTL_SHIFT)

// #define USDHC_PROT_CTRL_EMODE_SHIFT 4UL
// #define USDHC_PROT_CTRL_EMODE_MASK (0x3UL << USDHC_PROT_CTRL_EMODE_SHIFT)

// #define USDHC_PROT_CTRL_D3CD_SHIFT 3UL
// #define USDHC_PROT_CTRL_D3CD_MASK (1UL << USDHC_PROT_CTRL_D3CD_SHIFT)

// #define USDHC_PROT_CTRL_DTW_SHIFT 1UL
// #define USDHC_PROT_CTRL_DTW_MASK (0x3UL << USDHC_PROT_CTRL_DTW_SHIFT)

// #define USDHC_PROT_CTRL_LCTL_SHIFT 0UL
// #define USDHC_PROT_CTRL_LCTL_MASK (1UL << USDHC_PROT_CTRL_LCTL_SHIFT)

// #define USDHC_SYS_CTRL_RSTT_SHIFT 28UL
// #define USDHC_SYS_CTRL_RSTT_MASK (1UL << USDHC_SYS_CTRL_RSTT_SHIFT)

// #define USDHC_SYS_CTRL_INITA_SHIFT 27UL
// #define USDHC_SYS_CTRL_INITA_MASK (1UL << USDHC_SYS_CTRL_INITA_SHIFT)

// #define USDHC_SYS_CTRL_RSTD_SHIFT 26UL
// #define USDHC_SYS_CTRL_RSTD_MASK (1UL << USDHC_SYS_CTRL_RSTD_SHIFT)

// #define USDHC_SYS_CTRL_RSTC_SHIFT 25UL
// #define USDHC_SYS_CTRL_RSTC_MASK (1UL << USDHC_SYS_CTRL_RSTC_SHIFT)

// #define USHDC_SYS_CTRL_RSTA_SHIFT 24UL
// #define USDHC_SYS_CTRL_RSTA_MASK (1UL << USHDC_SYS_CTRL_RSTA_SHIFT)

// #define USDHC_SYS_CTRL_IPP_RST_N_SHIFT 23UL
// #define USDHC_SYS_CTRL_IPP_RST_N_MASK (1UL << USDHC_SYS_CTRL_IPP_RST_N_SHIFT)

// #define USDHC_SYS_CTRL_DTOCV_SHIFT 16UL
// #define USDHC_SYS_CTRL_DTOCV_MASK (0xFUL << USDHC_SYS_CTRL_DTOCV_SHIFT)

// #define USDHC_SYS_CTRL_SDCLKFS_SHIFT 8UL
// #define USDHC_SYS_CTRL_SDCLKFS_MASK (0xFFUL << USDHC_SYS_CTRL_SDCLKFS_SHIFT)

// #define USDHC_SYS_CTRL_DVS_SHIFT 4UL
// #define USDHC_SYS_CTRL_DVS_MASK (0xFUL << USDHC_SYS_CTRL_DVS_SHIFT)

// #define USDHC_INT_STATUS_DMAE_SHIFT 28UL
// #define USDHC_INT_STATUS_DMAE_MASK (1UL << USDHC_INT_STATUS_DMAE_SHIFT)

// #define USDHC_INT_STATUS_TNE_SHIFT 26UL
// #define USDHC_INT_STATUS_TNE_MASK (1UL << USDHC_INT_STATUS_TNE_SHIFT)

// #define USDHC_INT_STATUS_AC12E_SHIFT 24UL
// #define USDHC_INT_STATUS_AC12E_MASK (1UL << USDHC_INT_STATUS_AC12E_SHIFT)

// #define USDHC_INT_STATUS_DEBE_SHIFT 22UL
// #define USDHC_INT_STATUS_DEBE_MASK (1UL << USDHC_INT_STATUS_DEBE_SHIFT)

// #define USDHC_INT_STATUS_DCE_SHIFT 21UL
// #define USDHC_INT_STATUS_DCE_MASK (1UL << USDHC_INT_STATUS_DCE_SHIFT)

// #define USDHC_INT_STATUS_DTOE_SHIFT 20UL
// #define USHC_INT_STATUS_DTOE_MASK (1UL << USDHC_INT_STATUS_DTOE_SHIFT)

// #define USDHC_INT_STATUS_CIE_SHIFT 19UL
// #define USDHC_INT_STATUS_CIE_MASK (1UL << USDHC_INT_STATUS_CIE_SHIFT)

// #define USDHC_INT_STATUS_CEBE_SHIFT 18UL
// #define USDHC_INT_STATUS_CEBE_MASK (1UL << USDHC_INT_STATUS_CEBE_SHIFT)

// #define USDHC_INT_STATUS_CCE_SHIFT 17UL
// #define USDHC_INT_STATUS_CCE_MASK (1UL << USDHC_INT_STATUS_CCE_SHIFT)

// #define USDHC_INT_STATUS_CTOE_SHIFT 16UL
// #define USDHC_INT_STATUS_CTOE_MASK (1UL << USDHC_INT_STATUS_CTOE_SHIFT)

// #define USDHC_INT_STATUS_TP_SHIFT 14UL
// #define USDHC_INT_STATUS_TP_MASK (1UL << USDHC_INT_STATUS_TP_SHIFT)

// #define USDHC_INT_STATUS_RTE_SHIFT 12UL
// #define USDHC_INT_STATUS_RTE_MASK (1UL << USDHC_INT_STATUS_RTE_SHIFT)

// #define USDHC_INT_STATUS_CINT_SHIFT 8UL
// #define USDHC_INT_STATUS_CINT_MASK (1UL << USDHC_INT_STATUS_CINT_SHIFT)

// #define USDHC_INT_STATUS_CRM_SHIFT 7UL
// #define USDHC_INT_STATUS_CRM_MASK (1UL << USDHC_INT_STATUS_CRM_SHIFT)

// #define USDHC_INT_STATUS_CINS_SHIFT 6UL
// #define USDHC_INT_STATUS_CINS_MASK (1UL << USDHC_INT_STATUS_CINS_SHIFT)

// #define USDHC_INT_STATUS_BRR_SHIFT 5UL
// #define USDHC_INT_STATUS_BRR_MASK (1UL << USDHC_INT_STATUS_BRR_SHIFT)

// #define USDHC_INT_STATUS_BWR_SHIFT 4UL
// #define USDHC_INT_STATUS_BWR_MASK (1UL << USDHC_INT_STATUS_BWR_SHIFT)

// #define USDHC_INT_STATUS_DINT_SHIFT 3UL
// #define USDHC_INT_STATUS_DINT_MASK (1UL << USDHC_INT_STATUS_DINT_SHIFT)

// #define USDHC_INT_STATUS_DGE_SHIFT 2UL
// #define USDHC_INT_STATUS_DGE_MASK (1UL << USDHC_INT_STATUS_DGE_SHIFT)

// #define USDHC_INT_STATUS_TC_SHIFT 1UL
// #define USDHC_INT_STATUS_TC_MASK (1UL << USDHC_INT_STATUS_TC_SHIFT)

// #define USDHC_INT_STATUS_CC_SHIFT 0UL
// #define USDHC_INT_STATUS_CC_MASK (1UL << USDHC_INT_STATUS_CC_SHIFT)

// #define USDHC_INT_STATUS_EN_DMAESEN_SHIFT 28UL
// #define USDHC_INT_STATUS_EN_DMAESEN_MASK (1UL << USDHC_INT_STATUS_EN_DMAESEN_SHIFT)

// #define USDHC_INT_STATUS_EN_TNESEN_SHIFT 26UL
// #define USDHC_INT_STATUS_EN_TNESEN_MASK (1UL << USDHC_INT_STATUS_EN_TNESEN_SHIFT)

// #define USDHC_INT_STATUS_EN_AC12ESEN_SHIFT 24UL
// #define USDHC_INT_STATUS_EN_AC12ESEN_MASK (1UL << USDHC_INT_STATUS_EN_AC12ESEN_SHIFT)

// #define USDHC_INT_STATUS_EN_DEBESEN_SHIFT 22UL
// #define USDHC_INT_STATUS_EN_DEBESEN_MASK (1UL << USDHC_INT_STATUS_EN_DEBESEN_SHIFT)

// #define USDHC_INT_STATUS_EN_DCESEN_SHIFT 21UL
// #define USDHC_INT_STATUS_EN_DCESEN_MASK (1UL << USDHC_INT_STATUS_EN_DCESEN_SHIFT)

// #define USDHC_INT_STATUS_EN_DTOESEN_SHIFT 20UL
// #define USDHC_INT_STATUS_EN_DTOESEN_MASK (1UL << USDHC_INT_STATUS_EN_DTOESEN_SHIFT)

// #define USDHC_INT_STATUS_EN_CIESEN_SHIFT 19UL
// #define USDHC_INT_STATUS_EN_CIESEN_MASK (1UL << USDHC_INT_STATUS_EN_CIESEN_SHIFT)

// #define USDHC_INT_STATUS_EN_CEBESEN_SHIFT 18UL
// #define USDHC_INT_STATUS_EN_CEBESEN_MASK (1UL << USDHC_INT_STATUS_EN_CEBESEN_SHIFT)

// #define USDHC_INT_STATUS_EN_CCESEN_SHIFT 17UL
// #define USDHC_INT_STATUS_EN_CCESEN_MASK (1UL << USDHC_INT_STATUS_EN_CCESEN_SHIFT)

// #define USDHC_INT_STATUS_EN_CTOESEN_SHIFT 16UL
// #define USDHC_INT_STATUS_EN_CTOESEN_MASK (1UL << USDHC_INT_STATUS_EN_CTOESEN_SHIFT)

// #define USDHC_INT_STATUS_EN_TPSEN_SHIFT 14UL
// #define USDHC_INT_STATUS_EN_TPSEN_MASK (1UL << USDHC_INT_STATUS_EN_TPSEN_SHIFT)

// #define USDHC_INT_STATUS_EN_RTESEN_SHIFT 12UL
// #define USDHC_INT_STATUS_EN_RTESEN_MASK (1UL << USDHC_INT_STATUS_EN_RTESEN_SHIFT)

// #define USDHC_INT_STATUS_EN_CINTSEN_SHIFT 8UL
// #define USDHC_INT_STATUS_EN_CINTSEN_MASK (1UL << USDHC_INT_STATUS_EN_CINTSEN_SHIFT)

// #define USDHC_INT_STATUS_EN_CRMSEN_SHIFT 7UL
// #define USDHC_INT_STATUS_EN_CRMSEN_MASK (1UL << USDHC_INT_STATUS_EN_CRMSEN_SHIFT)

// #define USDHC_INT_STATUS_EN_CINSSEN_SHIFT 6UL
// #define USDHC_INT_STATUS_EN_CINSSEN_MASK (1UL << USDHC_INT_STATUS_EN_CINSSEN_SHIFT)

// #define USDHC_INT_STATUS_EN_BRRSEN_SHIFT 5UL
// #define USDHC_INT_STATUS_EN_BRRSEN_MASK (1UL << USDHC_INT_STATUS_EN_BRRSEN_MASK)

// #define USDHC_INT_STATUS_EN_BWRSEN_SHIFT 4UL
// #define USDHC_INT_STATUS_EN_BWRSEN_MASK (1UL << USDHC_INT_STATUS_EN_BWRSEN_SHIFT)

// #define USDHC_INT_STATUS_EN_DINTSEN_SHIFT 3UL
// #define USDHC_INT_STATUS_EN_DINTSEN_MASK (1UL << USDHC_INT_STATUS_EN_DINTSEN_SHIFT)

// #define USDHC_INT_STATUS_EN_BGESEN_SHIFT 2UL
// #define USDHC_INT_STATUS_EN_BGESEN_MASK (1U << USDHC_INT_STATUS_EN_BGESEN_SHIFT)

// #define USDHC_INT_STATUS_EN_TCSEN_SHIFT 1UL
// #define USDHC_INT_STATUS_EN_TCSEN_MASK (1UL << USDHC_INT_STATUS_EN_TCSEN_SHIFT)

// #define USDHC_INT_STATUS_EN_CCSEN_SHIFT 0UL
// #define USDHC_INT_STATUS_EN_CCSEN_MASK (1UL << USDHC_INT_STATUS_EN_CCSEN_SHIFT)

// #define USDHC_INT_SIGNAL_EN_DMAEIEN_SHIFT 28UL
// #define USDHC_INT_SIGNAL_EN_DMAEIEN_MASK (1UL << USDHC_INT_SIGNAL_EN_DMAEIEN_SHIFT)

// #define USDHC_INT_SIGNAL_EN_TNEIEN_SHIFT 26UL
// #define USDHC_INT_SIGNAL_EN_TNEIEN_MASK (1UL << USDHC_INT_SIGNAL_EN_TNEIEN_SHIFT)

// #define USDHC_INT_SIGNAL_EN_AC12EIEN_SHIFT 24UL
// #define USDHC_INT_SIGNAL_EN_AC12EIEN_MASK (1UL << USDHC_INT_SIGNAL_EN_AC12EIEN_SHIFT)

// #define USDHC_INT_SIGNAL_EN_DEBEIEN_SHIFT 22UL
// #define USDHC_INT_SIGNAL_EN_DEBEIEN_MASK (1UL << USDHC_INT_SIGNAL_EN_DEBEIEN_SHIFT)

// #define USDHC_INT_SIGNAL_EN_DCEIEN_SHIFT 21UL
// #define USDHC_INT_SIGNAL_EN_DCEIEN_MASK (1UL << USDHC_INT_SIGNAL_EN_DCEIEN_SHIFT)

// #define USDHC_INT_SIGNAL_EN_DTOEIEN_SHIFT 20UL
// #define USDHC_INT_SIGNAL_EN_DTOEIEN_MASK (1UL << USDHC_INT_SIGNAL_EN_DTOEIEN_SHIFT)

// #define USDHC_INT_SIGNAL_EN_CIEIEN_SHIFT 19UL
// #define USDHC_INT_SIGNAL_EN_CIEIEN_MASK (1UL << USDHC_INT_SIGNAL_EN_CIEIEN_SHIFT)

// #define USDHC_INT_SIGNAL_EN_CEBEIEN_SHIFT 18UL
// #define USDHC_INT_SIGNAL_EN_CEBEIEN_MASK (1UL << USDHC_INT_SIGNAL_EN_CEBEIEN_SHIFT)

// #define USDHC_INT_SIGNAL_EN_CCEIEN_SHIFT 17UL
// #define USDHC_INT_SIGNAL_EN_CCEIEN_MASK (1UL << USDHC_INT_SIGNAL_EN_CCEIEN_SHIFT)

// #define USDHC_INT_SIGNAL_EN_CTOEIEN_SHIFT 16UL
// #define USDHC_INT_SIGNAL_EN_CTOEIEN_MASK (1UL << USDHC_INT_SIGNAL_EN_CTOIEN_SHIFT)

// #define USDHC_INT_SIGNAL_EN_TPIEN_SHIFT 14UL
// #define USDHC_INT_SIGNAL_EN_TPIEN_MASK (1UL << USDHC_INT_SIGNAL_EN_TPIEN_SHIFT)

// #define USDHC_INT_SIGNAL_EN_RTEIEN_SHIFT 12UL
// #define USDHC_INT_SIGNAL_EN_RTEIEN_MASK (1UL << USDHC_INT_SIGNAL_EN_RTEIEN_SHIFT)

// #define USDHC_INT_SIGNAL_EN_CINTIEN_SHIFT 8UL
// #define USDHC_INT_SIGNAL_EN_CINTIEN_MASK (1UL << USDHC_INT_SIGNAL_EN_CINTIEN_SHIFT)

// #define USDHC_INT_SIGNAL_EN_CRMIEN_SHIFT 7UL
// #define USDHC_INT_SIGNAL_EN_CRMIEN_MASK (1UL << USDHC_INT_SIGNAL_EN_CRMIEN_SHIFT)

// #define USDHC_INT_SIGNAL_EN_CINSIEN_SHIFT 6UL
// #define USDHC_INT_SIGNAL_EN_CINSIEN_MASK (1UL << USDHC_INT_SIGNAL_EN_CINSIEN_SHIFT)

// #define USDHC_INT_SIGNAL_EN_BRRIEN_SHIFT 5UL
// #define USDHC_INT_SIGNAL_EN_BRRIEN_MASK (1UL << USDHC_INT_SIGNAL_EN_BRRIEN_SHIFT)

// #define USDHC_INT_SIGNAL_EN_BWRIEN_SHIFT 4UL
// #define USDHC_INT_SIGNAL_EN_BWRIEN_MASK (1UL << USDHC_INT_SIGNAL_EN_BWRIEN_SHIFT)

// #define USDHC_INT_SIGNAL_EN_DINTIEN_SHIFT 3UL
// #define USDHC_INT_SIGNAL_EN_DINTIEN_MASK (1UL << USDHC_INT_SIGNAL_EN_DINTIEN_SHIFT)

// #define USDHC_INT_SIGNAL_EN_BGEIEN_SHIFT 2UL
// #define USDHC_INT_SIGNAL_EN_BGEIEN_MASK (1UL << USDHC_INT_SIGNAL_EN_BGEIEN_SHIFT)

// #define USDHC_INT_SIGNAL_EN_TCIEN_SHIFT 1UL
// #define USDHC_INT_SIGNAL_EN_TCIEN_MASK (1UL << USDHC_INT_SIGNAL_EN_TCIEN_SHIFT)

// #define USDHC_INT_SIGNAL_EN_CCIEN_SHIFT 0UL
// #define USDHC_INT_SIGNAL_EN_CCIEN_MASK (1UL << USDHC_INT_SIGNAL_EN_CCIEN_SHIFT)

// #define USDHC_ACMD12_ERR_STATUS_SMP_CLK_SEL_SHIFT 23UL
// #define USDHC_ACMD12_ERR_STATUS_SMP_CLK_SEL_MASK (1UL << USDHC_ACMD12_ERR_STATUS_SMP_CLK_SEL_SHIFT)

// #define USDHC_ACMD12_ERR_STATUS_EXECUTE_TUNING_SHIFT 22UL
// #define USDHC_ACMD12_ERR_STATUS_EXECUTE_TUNING_MASK (1UL << USDHC_ACMD12_ERR_STATUS_EXECUTE_TUNING_SHIFT)

// #define USDHC_ACMD12_ERR_STATUS_CNIBAC12E_SHIFT 7UL
// #define USDHC_ACMD12_ERR_STATUS_CNIBAC12E_MASK (1UL << USDHC_ACMD12_ERR_STATUS_CNIBAC12E_SHIFT)

// #define USDHC_ACMD12_ERR_STATUS_AC12IE_SHIFT 4UL
// #define USDHC_ACMD12_ERR_STATUS_AC12IE_MASK (1UL << USDHC_ACMD12_ERR_STATUS_AC12IE_SHIFT)

// #define USDHC_ACMD12_ERR_STATUS_AC12CE_SHIFT 3UL
// #define USDHC_ACMD12_ERR_STATUS_AC12CE_MASK (1UL << USDHC_ACMD12_ERR_STATUS_AC12CE_SHIFT)

// #define USDHC_ACMD12_ERR_STATUS_AC12EBE_SHIFT 2UL
// #define USDHC_ACMD12_ERR_STATUS_AC12EBE_MASK (1UL << USDHC_ACMD12_ERR_STATUS_AC12EBE_SHIFT)

// #define USDHC_ACMD12_ERR_STATUS_AC12TOE_SHIFT 1UL
// #define USDHC_ACMD12_ERR_STATUS_AC12TOE_MASK (1UL << USDHC_ACMD12_ERR_STATUS_AC12TOE_SHIFT)

// #define USDHC_ACMD12_ERR_STATUS_AC12NE_SHIFT 0UL
// #define USDHC_ACMD12_ERR_STATUS_AC12NE_MASK (1UL << USDHC_ACMD12_ERR_STATUS_AC12NE_SHIFT)

// #define USDHC_HOST_CTRL_CAP_VS18_SHIFT 26UL
// #define USDHC_HOST_CTRL_CAP_VS18_MASK (1UL << USDHC_HOST_CTRL_CAP_VS18_SHIFT)

// #define USDHC_HOST_CTRL_CAP_VS30_SHIFT 25UL
// #define USDHC_HOST_CTRL_CAP_VS30_MASK (1UL << USDHC_HOST_CTRL_CAP_VS30_SHIFT)

// #define USDHC_HOST_CTRL_CAP_VS33_SHIFT 24UL
// #define USDHC_HOST_CTRL_CAP_VS33_MASK (1UL << USDHC_HOST_CTRL_CAP_VS33_SHIFT)

// #define USDHC_HOST_CTRL_CAP_SRS_SHIFT 23UL
// #define USDHC_HOST_CTRL_CAP_SRS_MASK (1UL << USDHC_HOST_CTRL_CAP_VS33_SHIFT)

// #define USDHC_HOST_CTRL_CAP_DMAS_SHIFT 22UL
// #define USDHC_HOST_CTRL_CAP_DMAS_MASK (1UL << USDHC_HOST_CTRL_CAP_DMAS_SHIFT)

// #define USDHC_HOST_CTRL_CAP_HSS_SHIFT 21UL
// #define USDHC_HOST_CTRL_CAP_HSS_MASK (1UL << USDHC_HOST_CTRL_CAP_HSS_SHIFT)

// #define USDHC_HOST_CTRL_CAP_ADMAS_SHIFT 20UL
// #define USDHC_HOST_CTRL_CAP_ADMAS_MASK (1UL << USDHC_HOST_CTRL_CAP_ADMAS_SHIFT)

// #define USDHC_HOST_CTRL_CAP_MBL_SHIFT 16UL
// #define USDHC_HOST_CTRL_CAP_MBL_MASK (0x7UL << USDHC_HOST_CTRL_CAP_MBL_SHIFT)

// #define USDHC_HOST_CTRL_CAP_RETUNING_MODE_SHIFT 14UL
// #define USDHC_HOST_CTRL_CAP_RETUNING_MODE_MASK (3UL << USDHC_HOST_CTRL_CAP_RETUNING_MODE_SHIFT)

// #define USDHC_HOST_CTRL_CAP_USE_TUNING_SDR50_SHIFT 14UL
// #define USDHC_HOST_CTRL_CAP_USE_TUNING_SDR50_MASK (3UL << USDHC_HOST_CTRL_CAP_USE_TUNING_SDR50_SHIFT)

// #define USDHC_HOST_CTRL_CAP_TIME_COUNT_RETUNING_SHIFT 8UL
// #define USDHC_HOST_CTRL_CAP_TIME_COUNT_RETUNING_MASK (0xFUL << USDHC_HOST_CTRL_CAP_TIME_COUNT_RETUNING_SHIFT)

// #define USDHC_HOST_CTRL_CAP_DDR50_SUPPORT_SHIFT 2UL
// #define USDHC_HOST_CTRL_CAP_DDR50_SUPPORT_MASK (1UL << USDHC_HOST_CTRL_CAP_DDR50_SUPPORT_SHIFT)

// #define USDHC_HOST_CTRL_CAP_SDR104_SUPPORT_SHIFT 1UL
// #define USDHC_HOST_CTRL_CAP_SDR104_SUPPORT_MASK (1UL << USDHC_HOST_CTRL_CAP_SDR104_SUPPORT_SHIFT)

// #define USDHC_HOST_CTRL_CAP_SDR50_SUPPORT_SHIFT 0UL
// #define USDHC_HOST_CTRL_CAP_SDR50_SUPPORT_MASK (1UL << USDHC_HOST_CTRL_CAP_SDR50_SUPPORT_SHIFT)

// #define USDHC_WTMK_LVL_WR_BRST_LEN_SHIFT 24UL
// #define USDHC_WTMK_LVL_WR_BRST_LEN_MASK (0x1FUL << USDHC_WTMK_LVL_WR_BRST_LEN_SHIFT)

// #define USDHC_WTMK_LVL_WR_WML_SHIFT 16UL
// #define USDHC_WTMK_LVL_WR_WML_MASK (0xFFUL << USDHC_WTMK_LVL_WR_WML_SHIFT)

// #define USDHC_WTMK_LVL_RD_BRST_LEN_SHIFT 8UL
// #define USDHC_WTMK_LVL_RD_BRST_LEN_MASK (0x1FUL << USDHC_WTMK_LVL_RD_BRST_LEN_SHIFT)

// #define USDHC_WTMK_LVL_RD_WML_SHIFT 0UL
// #define USDHC_WTMK_LVL_RD_WML_MASK (0xFFU << USDHC_WTMK_LVL_RD_WML_SHIFT)

// #define USDHC_MIX_CTRL_FBCLK_SEL_SHIFT 25UL
// #define USDHC_MIX_CTRL_FBCLK_SEL_MASK (1UL << USDHC_MIX_CTRL_FBCLK_SEL_SHIFT)

// #define USDHC_MIX_CTRL_AUTO_TUNE_EN_SHIFT 24UL
// #define USDHC_MIX_CTRL_AUTO_TUNE_EN_MASK (1UL << USDHC_MIX_CTRL_AUTO_TUNE_EN_SHIFT)

// #define USDHC_MIX_CTRL_SMP_CLK_SEL_SHIFT 23UL
// #define USDHC_MIX_CTRL_SMP_CLK_SEL_MASK (1UL << USDHC_MIX_CTRL_SMP_CLK_SEL_SHIFT)

// #define USDHC_MIX_CTRL_EXE_TUNE_SHIFT 22UL
// #define USDHC_MIX_CTRL_EXE_TUNE_MASK (1UL << USDHC_MIX_CTRL_EXE_TUNE_SHIFT)

// #define USDHC_MIX_CTRL_AC23EN_SHIFT 7UL
// #define USDHC_MIX_CTRL_AC23EN_MASK (1UL << USDHC_MIX_CTRL_AC23EN_SHIFT)

// #define USDHC_MIX_CTRL_NIBBLE_POS_SHIFT 6UL
// #define USDHC_MIX_CTRL_NIBBLE_POS_MASK (1UL << USDHC_MIX_CTRL_NIBBLE_POS_SHIFT)

// #define USDHC_MIX_CTRL_MSBSEL_SHIFT 5UL
// #define USDHC_MIX_CTRL_MSBSEL_MASK (1UL << USDHC_MIX_CTRL_MSBSEL_SHIFT)

// #define USDHC_MIX_CTRL_DTDSEL_SHIFT 4UL
// #define USDHC_MIX_CTRL_DTDSEL_MASK (1UL << USDHC_MIX_CTRL_DTDSEL_SHIFT)

#define USDHC_MIX_CTRL_DDREN_SHIFT 3UL
#define USDHC_MIX_CTRL_DDREN_MASK (1UL << USDHC_MIX_CTRL_DDREN_SHIFT)

// #define USDHC_MIX_CTRL_AC12EN_SHIFT 2UL
// #define USDHC_MIX_CTRL_AC12EN_MASK (1UL << USDHC_MIX_CTRL_AC12EN_SHIFT)

// #define USDHC_MIX_CTRL_BCEN_SHIFT 1UL
// #define USDHC_MIX_CTRL_BCEN_MASK (1UL << USDHC_MIX_CTRL_BCEN_SHIFT)

// #define USDHC_MIX_CTRL_DMAEN_SHIFT 0UL
// #define USDHC_MIX_CTRL_DMAEN_MASK (1UL << USDHC_MIX_CTRL_DMAEN_SHIFT)

// #define USDHC_FORCE_EVENT_FEVTCINT_SHIFT 31UL
// #define USDHC_FORCE_EVENT_FEVTCINT_MASK (1UL << USDHC_FORCE_EVENT_FEVTCINT_SHIFT)

// #define USDHC_FORCE_EVENT_FEVTDMAE_SHIFT 28UL
// #define USDHC_FORCE_EVENT_FEVTDMAE_MASK (1UL << USDHC_FORCE_EVENT_FEVTDMAE_SHIFT)

// #define USDHC_FORCE_EVENT_FEVTTNE_SHIFT 26UL
// #define USDHC_FORCE_EVENT_FEVTTNE_MASK (1UL << USDHC_FORCE_EVENT_FEVTTNE_SHIFT)

// #define USDHC_FORCE_EVENT_FEVTDEBE_SHIFT 22UL
// #define USDHC_FORCE_EVENT_FEVTDEBE_MASK (1UL << USDHC_FORCE_EVENT_FEVTDEBE_SHIFT)

// #define USDHC_FORCE_EVENT_FEVTAC12E_SHIFT 24UL
// #define USDHC_FORCE_EVENT_FEVTAC12E_MASK (1UL << USDHC_FORCE_EVENT_FEVTAC12E_SHIFT)

// #define USDHC_FORCE_EVENT_FEVTDCE_SHIFT 21UL
// #define USDHC_FORCE_EVENT_FEVTDCE_MASK (1UL << USDHC_FORCE_EVENT_FEVTDCE_SHIFT)

// #define USDHC_FORCE_EVENT_FEVTDTOE_SHIFT 20UL
// #define USDHC_FORCE_EVENT_FEVTDTOE_MASK (1UL << USDHC_FORCE_EVENT_FEVTDTOE_SHIFT)

// #define USDHC_FORCE_EVENT_FEVTCIE_SHIFT 19UL
// #define USDHC_FORCE_EVENT_FEVTCIE_MASK (1UL << USDHC_FORCE_EVENT_FEVTCIE_SHIFT)

// #define USDHC_FORCE_EVENT_FEVTCEBE_SHIFT 18UL
// #define USDHC_FORCE_EVENT_FEVTCEBE_MASK (1UL << USDHC_FORCE_EVENT_FEVTCEBE_SHIFT)

// #define USDHC_FORCE_EVENT_FEVTCCE_SHIFT 17UL
// #define USDHC_FORCE_EVENT_FEVTCCE_MASK (1UL << USDHC_FORCE_EVENT_FEVTCCE_SHIFT)

// #define USDHC_FORCE_EVENT_FEVTCTOE_SHIFT 16UL
// #define USDHC_FORCE_EVENT_FEVTCTOE_MASK (1UL << USDHC_FORCE_EVENT_FEVTCTOE_SHIFT)

// #define USDHC_FORCE_EVENT_FEVTAC12IE_SHIFT 4UL
// #define USDHC_FORCE_EVENT_FEVTAC12IE_MASK (1UL << USDHC_FORCE_EVENT_FEVTAC12IE_SHIFT)

// #define USDHC_FORCE_EVENT_FEVTCNIBAC12E_SHIFT 7UL
// #define USDHC_FORCE_EVENT_FEVTCNIBAC12E_MASK (1UL << USDHC_FORCE_EVENT_FEVTCNIBAC12E_SHIFT)

// #define USDHC_FORCE_EVENT_FEVTAC12EBE_SHIFT 3UL
// #define USDHC_FORCE_EVENT_FEVTAC12EBE_MASK (1UL << USDHC_FORCE_EVENT_FEVTAC12EBE_SHIFT)

// #define USDHC_FORCE_EVENT_FEVTAC12ECE_SHIFT 2UL
// #define USDHC_FORCE_EVENT_FEVTAC12ECE_MASK (1UL << USDHC_FORCE_EVENT_FEVTAC12ECE_SHIFT)

// #define USDHC_FORCE_EVENT_FEVTAC12TOE_SHIFT 1UL
// #define USDHC_FORCE_EVENT_FEVTAC12TOE_MASK (1UL << USDHC_FORCE_EVENT_FEVTAC12TOE_SHIFT)

// #define USDHC_FORCE_EVENT_FEVTAC12NE_SHIFT 0UL
// #define USDHC_FORCE_EVENT_FEVTAC12NE_MASK (1UL << USDHC_FORCE_EVENT_FEVTAC12NE_SHIFT)

// #define USDHC_ADMA_ERR_STATUS_ADMADCE_SHIFT 3UL
// #define USDHC_ADMA_ERR_STATUS_ADMADCE_MASK (1UL << USDHC_ADMA_ERR_STATUS_ADMADCE_SHIFT)

// #define USDHC_ADMA_ERR_STATUS_ADMALME_SHIFT 2UL
// #define USDHC_ADMA_ERR_STATUS_ADMALME_MASK (1UL << USDHC_ADMA_ERR_STATUS_ADMALME_SHIFT)

// #define USDHC_ADMA_ERR_STATUS_ADMAES_SHIFT 0UL
// #define USDHC_ADMA_ERR_STATUS_ADMAES_MASK (0x3UL << USDHC_ADMA_ERR_STATUS_ADMAES_SHIFT)

// #define USDHC_DLL_CTRL_DLL_CTRL_REF_UPDATE_INT_SHIFT 28UL
// #define USDHC_DLL_CTRL_DLL_CTRL_REF_UPDATE_INT_MASK (0xFUL << USDHC_DLL_CTRL_DLL_CTRL_REF_UPDATE_INT_SHIFT)

// #define USDHC_DLL_CTRL_DLL_CTRL_SLV_UPDATE_INT_SHIFT 20UL
// #define USDHC_DLL_CTRL_DLL_CTRL_SLV_UPDATE_INT_MASK (0xFFUL << USDHC_DLL_CTRL_DLL_CTRL_SLV_UPDATE_INT_SHIFT)

// #define USDHC_DLL_CTRL_DLL_CTRL_SLV_DLY_TARGET1_SHIFT 16UL
// #define USDHC_DLL_CTRL_DLL_CTRL_SLV_DLY_TARGET1_MASK (0x7UL << USDHC_DLL_CTRL_DLL_CTRL_SLV_DLY_TARGET1_SHIFT)

// #define USDHC_DLL_CTRL_DLL_CTRL_SLV_OVERRIDE_VAL_SHIFT 9UL
// #define USDHC_DLL_CTRL_DLL_CTRL_SLV_OVERRIDE_VAL_MASK (0x7FUL << USDHC_DLL_CTRL_DLL_CTRL_SLV_OVERRIDE_VAL_SHIFT)

// #define USDHC_DLL_CTRL_DLL_CTRL_SLV_OVERRIDE_SHIFT 8UL
// #define USDHC_DLL_CTRL_DLL_CTRL_SLV_OVERRIDE_MASK (1UL << USDHC_DLL_CTRL_DLL_CTRL_SLV_OVERRIDE_SHIFT)

// #define USDHC_DLL_CTRL_DLL_CTRL_GATE_UPDATE_SHIFT 7UL
// #define USDHC_DLL_CTRL_DLL_CTRL_GATE_UPDATE_MASK (1UL << USDHC_DLL_CTRL_DLL_CTRL_GATE_UPDATE_SHIFT)

// #define USDHC_DLL_CTRL_DLL_CTRL_SLV_DLY_TARGET0_SHIFT 3UL
// #define USDHC_DLL_CTRL_DLL_CTRL_SLV_DLY_TARGET0_MASK (0xFUL << USDHC_DLL_CTRL_DLL_CTRL_SLV_DLY_TARGET0_SHIFT)

// #define USDHC_DLL_CTRL_DLL_CTRL_SLV_FORCE_UPD_SHIFT 2UL
// #define USDHC_DLL_CTRL_DLL_CTRL_SLV_FORCE_UPD_MASK (1UL << USDHC_DLL_CTRL_DLL_CTRL_SLV_FORCE_UPD_SHIFT)

// #define USDHC_DLL_CTRL_DLL_CTRL_RESET_SHIFT 1UL
// #define USDHC_DLL_CTRL_DLL_CTRL_RESET_MASK (1UL << USDHC_DLL_CTRL_DLL_CTRL_RESET_SHIFT)

// #define USDHC_DLL_CTRL_DLL_CTRL_ENABLE_SHIFT 0UL
// #define USDHC_DLL_CTRL_DLL_CTRL_ENABLE_MASK (1UL << USDHC_DLL_CTRL_DLL_CTRL_ENABLE_SHIFT)

// #define USDHC_DLL_STATUS_REF_SEL_SHIFT 9UL
// #define USDHC_DLL_STATUS_REF_SEL_MASK (0x7FUL << USDHC_DLL_STATUS_REF_SEL_SHIFT)

// #define USDHC_DLL_STATUS_SLV_SEL_SHIFT 2UL
// #define USDHC_DLL_STATUS_SLV_SEL_MASK (0x7FUL << USDHC_DLL_STATUS_SLV_SEL_SHIFT)

// #define USDHC_DLL_STATUS_REF_LOCK_SHIFT 1UL
// #define USDHC_DLL_STATUS_REF_LOCK_MASK (1UL << USDHC_DLL_STATUS_REF_LOCK_SHIFT)

// #define USDHC_DLL_STATUS_SLV_LOCK_SHIFT 0UL
// #define USDHC_DLL_STATUS_SLV_LOCK_MASK (1UL << USDHC_DLL_STATUS_SLV_LOCK_SHIFT)

// #define USDHC_CLK_TUNE_CTRL_STATUS_PRE_ERR_SHIFT 31UL
// #define USDHC_CLK_TUNE_CTRL_STATUS_PRE_ERR_MASK (1UL << USDHC_CLK_TUNE_CTRL_STATUS_PRE_ERR_SHIFT)

// #define USDHC_CLK_TUNE_CTRL_STATUS_TAP_SEL_PRE_SHIFT 24UL
// #define USDHC_CLK_TUNE_CTRL_STATUS_TAP_SEL_PRE_MASK (0x7FUL << USDHC_CLK_TUNE_CTRL_STATUS_TAP_SEL_PRE_SHIFT)

// #define USDHC_CLK_TUNE_CTRL_STATUS_TAP_SEL_OUT_SHIFT 20UL
// #define USDHC_CLK_TUNE_CTRL_STATUS_TAP_SEL_OUT_MASK (0xFUL << USDHC_CLK_TUNE_CTRL_STATUS_TAP_SEL_OUT_SHIFT)

// #define USDHC_CLK_TUNE_CTRL_STATUS_TAP_SEL_POST_SHIFT 16UL
// #define USDHC_CLK_TUNE_CTRL_STATUS_TAP_SEL_POST_MASK (0xFUL << USDHC_CLK_TUNE_CTRL_STATUS_TAP_SEL_POST_SHIFT)

// #define USDHC_CLK_TUNE_CTRL_STATUS_NXT_ERR_SHIFT 15UL
// #define USDHC_CLK_TUNE_CTRL_STATUS_NXT_ERR_MASK (1UL << USDHC_CLK_TUNE_CTRL_STATUS_NXT_ERR_SHIFT)

// #define USDHC_CLK_TUNE_CTRL_STATUS_DLL_CELL_SET_PRE_SHIFT 8UL
// #define USDHC_CLK_TUNE_CTRL_STATUS_DLL_CELL_SET_PRE_MASK (0x7FUL << USDHC_CLK_TUNE_CTRL_STATUS_DLL_CELL_SET_PRE_SHIFT)

// #define USDHC_CLK_TUNE_CTRL_STATUS_DLL_CELL_OUT_SHIFT 4UL
// #define USDHC_CLK_TUNE_CTRL_STATUS_DLL_CELL_OUT_MASK (0xFUL << USDHC_CLK_TUNE_CTRL_STATUS_DLL_CELL_OUT_SHIFT)

// #define USDHC_CLK_TUNE_CTRL_STATUS_DLL_CELL_SET_POST_SHIFT 0UL
// #define USDHC_CLK_TUNE_CTRL_STATUS_DLL_CELL_SET_POST_MASK (0xFUL << USDHC_CLK_TUNE_CTRL_STATUS_DLL_CELL_SET_POST_SHIFT)

// #define USDHC_VEND_SPEC_CMD_BYTE_EN_SHFIT 31UL
// #define USDHC_VEND_SPEC_CMD_BYTE_EN_MASK (1UL << USDHC_VEND_SPEC_CMD_BYTE_EN_SHFIT)

// #define USDHC_VEND_SPEC_INST_ST_VAL_SHFIT 16UL
// #define USDHC_VEND_SPEC_INST_ST_VAL_MASK (0xFFUL << USDHC_VEND_SPEC_INST_ST_VAL_SHFIT)

// #define USDHC_VEND_SPEC_CRC_CHK_DIS_SHIFT 15UL
// #define USDHC_VEND_SPEC_CRC_CHK_DIS_MASK (1UL << USDHC_VEND_SPEC_CRC_CHK_DIS_SHIFT)

// #define USDHC_VEND_SPEC_CARC_CLK_SOFT_EN_SHIFT 14UL
// #define USDHC_VEND_SPEC_CARC_CLK_SOFT_EN_MASK (1UL << USDHC_VEND_SPEC_CARC_CLK_SOFT_EN_SHIFT)

// #define USDCH_VEND_SPEC_IPG_PERCLK_SOFT_EN_SHIFT 13UL
// #define USDCH_VEND_SPEC_IPG_PERCLK_SOFT_EN_MASK (1UL << USDCH_VEND_SPEC_IPG_PERCLK_SOFT_EN_SHIFT)

// #define USDCH_VEND_SPEC_HCLK_SOFT_EN_SHIFT 12UL
// #define USDCH_VEND_SPEC_HCLK_SOFT_EN_MASK (1UL << USDCH_VEND_SPEC_HCLK_SOFT_EN_SHIFT)

// #define USDCH_VEND_SPEC_IPG_CLK_SOFT_EN_SHIFT 11UL
// #define USDCH_VEND_SPEC_IPG_CLK_SOFT_EN_MASK (1UL << USDCH_VEND_SPEC_IPG_CLK_SOFT_EN_SHIFT)

// #define USDHC_VEND_SPEC_FRC_SDCLK_ON_SHIFT 8UL
// #define USDHC_VEND_SPEC_FRC_SDCLK_ON_MASK (1UL << USDHC_VEND_SPEC_FRC_SDCLK_ON_SHIFT)

// #define USDHC_VEND_SPEC_CLKONJ_IN_ABORT_SHIFT 7UL
// #define USDHC_VEND_SPEC_CLKONJ_IN_ABORT_MASK (1UL << USDHC_VEND_SPEC_CLKONJ_IN_ABORT_SHIFT)

// #define USDHC_VEND_SPEC_WP_POL_SHIFT 6UL
// #define USDHC_VEND_SPEC_WP_POL_MASK (1UL << USDHC_VEND_SPEC_WP_POL_SHIFT)

// #define USDHC_VEND_SPEC_CD_POL_SHIFT 5UL
// #define USDHC_VEND_SPEC_CD_POL_MASK (1UL << USDHC_VEND_SPEC_CD_POL_SHIFT)

// #define USDHC_VEND_SPEC_DATA3_CD_POL_SHIFT 4UL
// #define USDHC_VEND_SPEC_DATA3_CD_POL_MASK (1UL << USDHC_VEND_SPEC_DATA3_CD_POL_SHIFT)

// #define USDHC_VEND_SPEC_AC12_WR_CHKBUSY_EN_SHIFT 3UL
// #define USDHC_VEND_SPEC_AC12_WR_CHKBUSY_EN_MASK (1UL << USDHC_VEND_SPEC_AC12_WR_CHKBUSY_EN_SHIFT)

// #define USDHC_VEND_SPEC_CONFLICT_CHK_EN_SHIFT 2UL
// #define USDHC_VEND_SPEC_CONFLICT_CHK_EN_MASK (1UL << USDHC_VEND_SPEC_CONFLICT_CHK_EN_SHIFT)

// #define USDHC_VEND_SPEC_VSELECT_SHIFT 1UL
// #define USDHC_VEND_SPEC_VSELECT_MASK (1UL << USDHC_VEND_SPEC_VSELECT_SHIFT)

// #define USDHC_VEND_SPEC_EXT_DMA_EN_SHIFT 0UL
// #define USDHC_VEND_SPEC_EXT_DMA_EN_MASK (1UL << USDHC_VEND_SPEC_EXT_DMA_EN_SHIFT)

// #define USDHC_MCC_BOOT_BOOT_BLK_CNT_SHFIT 16UL
// #define USDHC_MCC_BOOT_BOOT_BLK_CNT_MASK (0xFFFFUL << USDHC_MCC_BOOT_BOOT_BLK_CNT_SHFIT)

// #define USDHC_MCC_BOOT_DISABLE_TIMEOUT_SHFIT 8UL
// #define USDHC_MCC_BOOT_DISABLE_TIMEOUT_MASK (1UL << USDHC_MCC_BOOT_DISABLE_TIMEOUT_SHFIT)

// #define USDHC_MCC_BOOT_AUTO_SABG_EN_SHIFT 7UL
// #define USDHC_MCC_BOOT_AUTO_SABG_EN_MASK (1UL << USDHC_MCC_BOOT_AUTO_SABG_EN_SHIFT)

// #define USDHC_MCC_BOOT_BOOT_EN_SHIFT 6UL
// #define USDHC_MCC_BOOT_BOOT_EN_MASK (1UL << USDHC_MCC_BOOT_BOOT_EN_SHIFT)

// #define USDHC_MMC_BOOT_BOOT_MODE_SHIFT 5UL
// #define USDHC_MMC_BOOT_BOOT_MODE_MASK (1UL << USDHC_MMC_BOOT_BOOT_MODE_SHIFT)

// #define USDHC_MMC_BOOT_BOOT_ACK_SHIFT 4UL
// #define USDHC_MMC_BOOT_BOOT_ACK_MASK (1UL << USDHC_MMC_BOOT_BOOT_ACK_SHIFT)

// #define USDHC_MMC_BOOT_DTOCV_ACK_SHIFT 0UL
// #define USDHC_MMC_BOOT_DTOCV_ACK_MASK (0xFUL << USDHC_MMC_BOOT_DTOCV_ACK_SHIFT)

// #define USDHC_VEND_SPEC2_ACMD23_ARGU2_EN_SHIFT 23UL
// #define USDHC_VEND_SPEC2_ACMD23_ARGU2_EN_MASK (1UL << USDHC_VEND_SPEC2_ACMD23_ARGU2_EN_SHIFT)

// #define USDHC_VEND_SPEC2_CARD_INT_AUTO_CLR_DIS_SHIFT 7UL
// #define USDHC_VEND_SPEC2_CARD_INT_AUTO_CLR_DIS_MASK (1UL << USDHC_VEND_SPEC2_CARD_INT_AUTO_CLR_DIS_SHIFT)

// #define USDHC_VEND_SPEC2_TUNING_CMD_EN_SHIFT 6UL
// #define USDHC_VEND_SPEC2_TUNING_CMD_EN_MASK (1UL << USDHC_VEND_SPEC2_TUNING_CMD_EN_SHIFT)

// #define USDHC_VEND_SPEC2_TUNING_1BIT_EN_SHIFT 5UL
// #define USDHC_VEND_SPEC2_TUNING_1BIT_EN_MASK (1UL << USDHC_VEND_SPEC2_TUNING_1BIT_EN_SHIFT)

// #define USDHC_VEND_SPEC2_TUNING_8BIT_EN_SHIFT 4UL
// #define USDHC_VEND_SPEC2_TUNING_8BIT_EN_MASK (1UL << USDHC_VEND_SPEC2_TUNING_8BIT_EN_SHIFT)

// #define USDHC_VEND_SPEC2_CARD_INT_D3_TEST_SHIFT 3UL
// #define USDHC_VEND_SPEC2_CARD_INT_D3_TEST_MASK (1UL << USDHC_VEND_SPEC2_CARD_INT_D3_TEST_SHIFT)

// #define USDHC_VEND_SPEC2_SDR104_NSDDIS_SHIFT 2UL
// #define USDHC_VEND_SPEC2_SDR104_NSDDIS_MASK (1UL << USDHC_VEND_SPEC2_SDR104_NSDDIS_SHIFT)

// #define USDHC_VEND_SPEC2_SDR104_OEDIS_SHIFT 1UL
// #define USDHC_VEND_SPEC2_SDR104_OEDIS_MASK (1UL << USDHC_VEND_SPEC2_SDR104_OEDIS_SHIFT)

// #define USDHC_VEND_SPEC2_SDR104_TIMINGDIS_SHIFT 0UL
// #define USDHC_VEND_SPEC2_SDR104_TIMINGDIS_MASK (1UL << USDHC_VEND_SPEC2_SDR104_TIMINGDIS_SHIFT)

// #define USDHC_TUNING_CTRL_STD_TUNING_EN_SHIFT 24UL
// #define USDHC_TUNING_CTRL_STD_TUNING_EN_MASK (1UL << USDHC_TUNING_CTRL_STD_TUNING_EN_SHIFT)

// #define USDHC_TUNING_CTRL_TUNING_WINDOW_SHIFT 20UL
// #define USDHC_TUNING_CTRL_TUNING_WINDOW_MASK (0x7UL << USDHC_TUNING_CTRL_TUNING_WINDOW_SHIFT)

// #define USDHC_TUNING_CTRL_TUNING_STEP_SHIFT 16UL
// #define USDHC_TUNING_CTRL_TUNING_STEP_MASK (0x7UL << USDHC_TUNING_CTRL_TUNING_STEP_SHIFT)

// #define USDHC_TUNING_CTRL_TUNING_COUNTER_SHIFT 8UL
// #define USDHC_TUNING_CTRL_TUNING_COUNTER_MASK (0xFFUL << USDHC_TUNING_CTRL_TUNING_COUNTER_SHIFT)

// #define USDHC_TUNING_CTRL_TUNING_START_TAP_SHIFT 0UL
// #define USDHC_TUNING_CTRL_TUNING_START_TAP_MASK (0xFFUL << USDHC_TUNING_CTRL_TUNING_START_TAP_SHIFT)

// #define USDHC_DEBUG
#ifdef USDHC_DEBUG
#define USDHC_TRACE cprintf
#else
#define USDHC_TRACE(fmt, args...)
#endif

#define USDHC_DWT_1BIT 0
#define USDHC_DWT_4BIT 1
#define USDHC_DWT_8BIT 2

#define USDHC_EMODE_BIG_ENDIAN 0
#define USDHC_EMODE_HALF_WORD_BIG_ENDIAN 1
#define USDHC_EMODE_LITTLE_ENDIAN 2

#define WRITE 0
#define READ 1

#define RESPONSE_NONE 0 
#define RESPONSE_136 1
#define RESPONSE_48 2
#define RESPONSE_48_CHECK_BUSY 3

#define SINGLE_BLK 0
#define MULTIPLE_BLK 1

typedef struct usdhc_cmd_tag{
    uint32_t cmd;
    uint32_t arg;
    uint8_t xfer_type;
    uint8_t resp_format;
    bool dat_pres;
    bool crc_chk;
    bool cmdidx_chk;
    bool blk_cnt_en_chk;
    uint8_t blk_type;
    bool dma_en;
    bool auto_cmd12_en;
    bool ddr_en;
} usdhc_cmd_t;

typedef struct {
    uint8_t att;
    uint8_t reserved;
    uint16_t len;
    uint32_t addr;
} __attribute__((packed)) adma_bd_t;

extern bool usdhc_init(void *);
extern void usdhc_set_data_width(void *, uint8_t);
extern void usdhc_set_endian_mode(void *, uint8_t);
extern void usdhc_set_clock(void *, uint8_t);
extern bool usdhc_send_command(void *,uint8_t, uint32_t);
extern void usdhc_get_response(void *, sd_resp_t *);
extern bool usdhc_read_block(void *, uint8_t *, uint32_t);
extern bool usdhc_write_block(void *, uint8_t *, uint32_t);

#endif
