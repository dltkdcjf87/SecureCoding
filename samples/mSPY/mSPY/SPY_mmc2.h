//
//  SPY_mmc2.h
//  nsmSPY
//
//  Created by SMCHO on 2016. 5. 14..
//  Copyright  2016년 SMCHO. All rights reserved.
//

#ifndef __SPY_MMC2_H__
#define __SPY_MMC2_H__

#include "MMC_head.h"

#define MMC2_RESULT_OK      0
#define MMC2_RESULT_FAIL    1
typedef enum
{
    MMC_DIS_CFG_SPY = 4500,
    MMC_SET_LOG_SPY,
    MMC_SET_QOS_SPY,
    
    MMC_DIS_STAT_SPY,
    MMC_CLR_STAT_SPY,
    
    MMC_DIS_BLOCK_SPY = 4510,
    MMC_SET_BLOCK_SPY,
    
    MMC_ADD_TRACE_SPY = 4520,
    MMC_DEL_TRACE_SPY,
    MMC_DIS_TRACE_SPY,

    MMC_SET_BLLIST_SPY = 4530,
    MMC_UPD_BLIST_SPY,
    MMC_UPD_BLIST_SPY_6,
    MMC_UPD_EXTLIST_SPY,
    MMC_UPD_EXTLIST_SPY_6,
    
    MMC_DIS_OVER_RESP = 4540,
    MMC_SET_OVER_RESP,
    MMC_DIS_SCM_MAP,
    MMC_SET_SCM_MAP,
    MMC_CLR_SCM_MAP,
    MMC_DIS_SCM_AUDIT,
    MMC_SET_SCM_AUDIT,
    
    MMC_CLR_CALL_SPY = 4590,
    MMC_DIS_CALL_SPY,
    MMC_SAVE_CFG_SPY,
    MMC_DIS_REPORT_SPY,
    MMC_CHG_REPORT_SPY,
    
    MMC_INIT_ALL_SPY = 4999

} SPY_MMC2;

typedef struct
{
    MMCHD   mmc_head;
    uint8_t side;           // default
} DIS_CFG_SPY_R;

typedef struct
{
    MMCHD   mmc_head;
    char	result;         // 사용하지 않더라도 MMC 결과에 항상포함
    char    reason;         // 사용하지 않더라도 MMC 결과에 항상포함
    
    uint8_t nLogLevel;
    bool    b_log_options;  // C_LOG_OPTION
    bool    b_log_register; // C_LOG_REGISTER
    uint8_t nQoS;
    char    dummy[2];
    
    char    strOrigStackAddr[32];
    char    strTermStackAddr[32];
    char    strOrigStackAddr_V6[64];
    char    strTermStackAddr_V6[64];
} DIS_CFG_SPY_S;    // M04500

typedef struct
{
    MMCHD       mmc_head;
    uint8_t     side;       // default
    
    uint8_t     nLogLevel;
    bool        b_log_options;  // C_LOG_OPTION
    bool        b_log_register; // C_LOG_REGISTER
} SET_LOG_SPY_R;

typedef struct
{
    MMCHD   mmc_head;
    char	result;         // 사용하지 않더라도 MMC 결과에 항상포함
    char    reason;         // 사용하지 않더라도 MMC 결과에 항상포함
    
    uint8_t nLogLevel;
    bool    b_log_options;  // C_LOG_OPTION
    bool    b_log_register; // C_LOG_REGISTER
} SET_LOG_SPY_S;

typedef struct
{
    MMCHD       mmc_head;
    uint8_t     side;       // default
    
    uint8_t     nQoS;
} SET_QOS_SPY_R;

typedef struct
{
    MMCHD   mmc_head;
    char	result;         // 사용하지 않더라도 MMC 결과에 항상포함
    char    reason;         // 사용하지 않더라도 MMC 결과에 항상포함
    
    uint8_t nQoS;
} SET_QOS_SPY_S;

typedef struct
{
    MMCHD   mmc_head;
    uint8_t side;           // default
} DIS_BLOCK_SPY_R;

typedef struct
{
    MMCHD	mmc_head;
    char	result;         // 사용하지 않더라도 MMC 결과에 항상포함
    char    reason;         // 사용하지 않더라도 MMC 결과에 항상포함
    
    uint8_t	block_state;
} DIS_BLOCK_SPY_S;


typedef struct
{
    MMCHD       mmc_head;
    uint8_t     side;       // default
    
    uint8_t     block_state;    // 0 - unblock, 1 - block
} SET_BLOCK_SPY_R;

typedef struct
{
    MMCHD	mmc_head;
    char	result;         // 사용하지 않더라도 MMC 결과에 항상포함
    char    reason;         // 사용하지 않더라도 MMC 결과에 항상포함
    
    uint8_t	block_state;
} SET_BLOCK_SPY_S;

typedef struct
{
    MMCHD       mmc_head;
    uint8_t     side;       // default
    
    uint8_t     blist_state;    // 0 - unuse, 1 - use
} SET_BLIST_SPY_R;

typedef struct
{
    MMCHD	mmc_head;
    char	result;         // 사용하지 않더라도 MMC 결과에 항상포함
    char    reason;         // 사용하지 않더라도 MMC 결과에 항상포함
    
    uint8_t blist_state;
} SET_BLIST_SPY_S;



typedef struct
{
    MMCHD	mmc_head;
    uint8_t side;           // default
    bool    bIsV6;          // V4(0)/V6(1) 여부 - 추후구현
} UPD_BLIST_SPY_R;

typedef struct
{
    MMCHD	mmc_head;
    char	result;         // 사용하지 않더라도 MMC 결과에 항상포함
    char    reason;         // 사용하지 않더라도 MMC 결과에 항상포함
    bool    bIsV6;          // V4(0)/V6(1) 여부 - 추후구현
    uint8_t n_blist;      // 등록된 EXT LIST 갯수
} UPD_BLIST_SPY_S;


typedef struct
{
    MMCHD	mmc_head;
    uint8_t side;           // default
    bool    bIsV6;          // V4(0)/V6(1) 여부 - 추후구현
} UPD_EXTLIST_SPY_R;

typedef struct
{
    MMCHD	mmc_head;
    char	result;         // 사용하지 않더라도 MMC 결과에 항상포함
    char    reason;         // 사용하지 않더라도 MMC 결과에 항상포함
    bool    bIsV6;          // V4(0)/V6(1) 여부 - 추후구현
    uint8_t n_extlist;      // 등록된 EXT LIST 갯수
} UPD_EXTLIST_SPY_S;

typedef struct
{
    MMCHD       mmc_head;
    uint8_t     side;       // default
    uint8_t     dummy;
    uint16_t    resp_code;
} SET_OVER_RESP_R;

typedef struct
{
    MMCHD       mmc_head;
    char        result;         // 사용하지 않더라도 MMC 결과에 항상포함
    char        reason;         // 사용하지 않더라도 MMC 결과에 항상포함
    
    uint16_t    resp_code;
} SET_OVER_RESP_S;

typedef struct
{
    MMCHD       mmc_head;
    uint8_t     side;       // default
} DIS_OVER_RESP_R;

typedef struct
{
    MMCHD       mmc_head;
    char        result;         // 사용하지 않더라도 MMC 결과에 항상포함
    char        reason;         // 사용하지 않더라도 MMC 결과에 항상포함
    
    uint16_t    resp_code;
} DIS_OVER_RESP_S;

typedef struct
{
    MMCHD       mmc_head;
    uint8_t     side;       // default
} DIS_SCM_MAP_R;

typedef struct
{
    MMCHD       mmc_head;
    char        result;         // 사용하지 않더라도 MMC 결과에 항상포함
    char        reason;         // 사용하지 않더라도 MMC 결과에 항상포함
    
    uint8_t     use_scm_map;
    char        dummy[3];
    
    int         n_cr_map;
    int         n_call_id_map;
} DIS_SCM_MAP_S;

typedef struct
{
    MMCHD       mmc_head;
    uint8_t     side;       // default
    
    uint8_t     use_scm_map;
} SET_SCM_MAP_R;

typedef struct
{
    MMCHD       mmc_head;
    char        result;         // 사용하지 않더라도 MMC 결과에 항상포함
    char        reason;         // 사용하지 않더라도 MMC 결과에 항상포함
    
    uint8_t     use_scm_map;
} SET_SCM_MAP_S;

typedef struct
{
    MMCHD       mmc_head;
    uint8_t     side;       // default
} CLR_SCM_MAP_R;

typedef struct
{
    MMCHD       mmc_head;
    char        result;         // 사용하지 않더라도 MMC 결과에 항상포함
    char        reason;         // 사용하지 않더라도 MMC 결과에 항상포함
    char        dummy[2];
    
    int         n_before;
    int         n_after;
} CLR_SCM_MAP_S;


typedef struct
{
    MMCHD       mmc_head;
    uint8_t     side;       // default
} DIS_SCM_AUDIT_R;

typedef struct
{
    MMCHD       mmc_head;
    char        result;         // 사용하지 않더라도 MMC 결과에 항상포함
    char        reason;         // 사용하지 않더라도 MMC 결과에 항상포함
    char        dummy[2];
    
    uint8_t     n_scm_audit_time;   // 0 ~ 5
} DIS_SCM_AUDIT_S;

typedef struct
{
    MMCHD       mmc_head;
    uint8_t     side;       // default
    
    uint8_t     n_scm_audit_time;
} SET_SCM_AUDIT_R;

typedef struct
{
    MMCHD       mmc_head;
    char        result;         // 사용하지 않더라도 MMC 결과에 항상포함
    char        reason;         // 사용하지 않더라도 MMC 결과에 항상포함
    char        dummy[2];
    
    uint8_t     n_scm_audit_time;   // 0 ~ 5
} SET_SCM_AUDIT_S;

typedef struct
{
    MMCHD       mmc_head;
    uint8_t     side;       // default
} SAVE_CFG_SPY_R;

typedef struct
{
    MMCHD       mmc_head;
    char        result;         // 사용하지 않더라도 MMC 결과에 항상포함
    char        reason;         // 사용하지 않더라도 MMC 결과에 항상포함
} SAVE_CFG_SPY_S;

typedef struct
{
    MMCHD       mmc_head;
    uint8_t     side;       // default
} CLR_STAT_SPY_R;

typedef struct
{
    MMCHD       mmc_head;
    char        result;         // 사용하지 않더라도 MMC 결과에 항상포함
    char        reason;         // 사용하지 않더라도 MMC 결과에 항상포함
} CLR_STAT_SPY_S;

typedef struct
{
    MMCHD       mmc_head;
    uint8_t     side;       // default
} CLR_CALL_SPY_R;

typedef struct
{
    MMCHD       mmc_head;
    char        result;         // 사용하지 않더라도 MMC 결과에 항상포함
    char        reason;         // 사용하지 않더라도 MMC 결과에 항상포함
    char        dummy[2];
    
    int         o_hash_size;
    int         t_hash_size;
} CLR_CALL_SPY_S;

typedef struct
{
    MMCHD       mmc_head;
    uint8_t     side;       // default
} DIS_CALL_SPY_R;

typedef struct
{
    MMCHD       mmc_head;
    uint8_t     side;       // default
    
    uint16_t    report_time;
} CHG_REPORT_SPY_R;

typedef struct
{
    MMCHD       mmc_head;
    char        result;         // 사용하지 않더라도 MMC 결과에 항상포함
    char        reason;         // 사용하지 않더라도 MMC 결과에 항상포함
    
    uint16_t    report_time_in_config;
    uint16_t    report_time;
} CHG_REPORT_SPY_S;

typedef struct
{
    MMCHD       mmc_head;
    uint8_t     side;       // default
} DIS_REPORT_SPY_R;

typedef struct
{
    MMCHD       mmc_head;
    char        result;         // 사용하지 않더라도 MMC 결과에 항상포함
    char        reason;         // 사용하지 않더라도 MMC 결과에 항상포함
    
    uint16_t    report_time_in_config;
    uint16_t    report_time;
} DIS_REPORT_SPY_S;

typedef struct
{
    MMCHD       mmc_head;
    uint8_t     side;       // default
} INIT_ALL_SPY_R;

typedef struct
{
    MMCHD       mmc_head;
    char        result;         // 사용하지 않더라도 MMC 결과에 항상포함
    char        reason;         // 사용하지 않더라도 MMC 결과에 항상포함
} INIT_ALL_SPY_S;

typedef struct
{
    MMCHD       mmc_head;
    char        result;         // 사용하지 않더라도 MMC 결과에 항상포함
    char        reason;         // 사용하지 않더라도 MMC 결과에 항상포함
    char        dummy[2];

    int         o_hash_size;
    int         t_hash_size;
} DIS_CALL_SPY_S;

typedef struct
{
    MMCHD       mmc_head;
    uint8_t     side;       // default
} DIS_STAT_SPY_R;

typedef struct
{
    MMCHD	mmc_head;
    char    result;         // 사용하지 않더라도 MMC 결과에 항상포함
    char    reason;         // 사용하지 않더라도 MMC 결과에 항상포함
    char    dummy[2];
    
    int     INVITE_Counts;
    int     R100_Counts;
    int     R180_Counts;
    int     R183_Counts;
    int     R200_Counts;
    int     ACK_Counts;
    int     CANCEL_Counts;
    int     BYE_Counts;
    int     INFO_Counts;
    
    int     PRACK_Counts;
    
    int     InMsgCounts_O;
    int     InMsgCounts_T;
    int     TotalMsgCounts;
    
    int     INVITE_OutCounts;
    int     R100_OutCounts;
    int     R180_OutCounts;
    int     R183_OutCounts;
    int     R200_OutCounts;
    int     ACK_OutCounts;
    int     CANCEL_OutCounts;
    int     BYE_OutCounts;
    int     INFO_OutCounts;
    
    int     PRACK_OutCounts;

    
    int     OutMsgCounts_O;
    int     OutMsgCounts_T;
    int     TotalMsgOutCounts;
    
    int     SessionCounts;
    int     SCMFailCounts;
    int     HashFailCounts;
    
} DIS_STAT_SPY_S;



typedef struct
{
    MMCHD       mmc_head;
    uint8_t     side;       // default
} DIS_TRACE_SPY_R;

typedef struct
{
    int     nIndex;
    char	strFrom[32];
    char	strTo[32];
//    char	strSvcKey[32];
    int     nMax;
    int     nCount;
} TRACE_ITEM;

typedef struct
{
    MMCHD	mmc_head;
    char    result;         // 사용하지 않더라도 MMC 결과에 항상포함
    char    reason;         // 사용하지 않더라도 MMC 결과에 항상포함
    char    dummy;
    uint8_t     n_item;
    TRACE_ITEM	item[10];
} DIS_TRACE_SPY_S;

typedef struct
{
    MMCHD       mmc_head;
    uint8_t     side;       // default
    
    //    uint16_t	bFlag;
//    uint16_t	msgType;
//    uint16_t	msgResponseCode;
//    uint16_t	msgDirection;
    uint8_t     dummy;
    uint16_t    nRepeatCount;           // Trace 반복횟수
    
    char        strFromAddr[32];
    char        strToAddr[32];
//    char        strSvcKey[32];
} ADD_TRACE_SPY_R;

typedef struct
{
    MMCHD	mmc_head;
    char    result;         // 사용하지 않더라도 MMC 결과에 항상포함
    char    reason;         // 사용하지 않더라도 MMC 결과에 항상포함
    
    uint8_t nIndex;
//    uint16_t	bFlag;
//    uint16_t	msgType;
//    uint16_t	msgResponseCode;
//    uint16_t	msgDirection;
    
//    int         msgCount;
//    char        strFromAddr[32];
//    char        strToAddr[32];
//    char        strSvcKey[32];
} ADD_TRACE_SPY_S;

typedef struct
{
    MMCHD       mmc_head;
    uint8_t     side;       // default
    
    uint8_t     nIndex;     // 삭제할 index (DIS_TRACE_SPY에 있음)
    
} DEL_TRACE_SPY_R;


typedef struct
{
    MMCHD	mmc_head;
    char    result;         // 사용하지 않더라도 MMC 결과에 항상포함
    char    reason;         // 사용하지 않더라도 MMC 결과에 항상포함

    uint8_t trace_size;
} DEL_TRACE_SPY_S;


#endif // __SPY_MMC2_H__

