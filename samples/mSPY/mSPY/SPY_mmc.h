#ifndef __SPY_MMC_H__
#define __SPY_MMC_H__

#include "MMC_head.h"


#define CNM_DisCfgSPY       4000
#define CNM_DisStatSPY      4001
#define CNM_SetCfgSPY       4002
#define CNM_ReloadCfgSPY    4003
#define CNM_SetBlockSPY     4004
#define CNM_DisBlockSPY     4005

#define CNM_ClrStatSPY      4100
#define CNM_ClrCallSPY      4101

#define CNM_TraceOnSPY      4200
#define CNM_TraceOffSPY     4201
#define CNM_TraceListSPY    4202

#define CNM_BlackListUpdate 4210
#define CNM_BlackListAct    4211
#define CNM_ExtListUpdate   4212    // 기존 SSW, EXT로 나누어져 있던것을 EXT로 통합

#ifdef IPV6_MODE
#   define CNM_BlackListUpdate_V6   4215
#   define CNM_ExtListUpdate_V6     4216    // 기존 SSW, EXT로 나누어져 있던것을 EXT로 통합
#endif
#define CNM_SetProxySPY     4300    // FIXIT: use ??

#define CNS_SaveCfgSPY      4301        // add 20160513 for mPBX BMT
#define CNS_DisOverResp     4302        // add 20160513 for mPBX BMT
#define CNS_SetOverResp     4303        // add 20160513 for mPBX BMT

typedef struct
{
	MMCHD       mmc_head;
	uint8_t     LogFile;
	uint8_t     DebugLevel;
	uint8_t     Qos;
	int         InThreadCounts;
	int         OutThreadCounts;
	char        strOrigStack[32];
	char        strTermStack[32];
	uint8_t     bFlag;
	char        strData[32];
} R_DIS_CFG_SPY;

typedef struct reload_config_spy
{
	MMCHD       mmc_head;
	char        strAllow[16][32];
	char        strDeny[16][32];
} S_RELOAD_CFG_SPY;

typedef struct
{
	MMCHD       mmc_head;
	uint8_t     block;
} R_SET_BLOCK_SPY;

typedef struct
{
	MMCHD	mmc_head;
	char	result[4];
} S_DIS_BLOCK_SPY;

typedef struct
{
    MMCHD   mmc_head;
} R_SAVE_CONFIG;

typedef struct
{
    MMCHD   mmc_head;
    char	result;
    char    reason;
} S_SAVE_CONFIG;

typedef struct
{
    MMCHD       mmc_head;
    uint16_t    resp_code;
} R_SET_OVER_RESP;

typedef struct
{
    MMCHD       mmc_head;
    uint16_t    resp_code;
} S_DIS_OVER_RESP;

typedef struct
{
	MMCHD       mmc_head;
	uint8_t     LogFile;
	uint8_t     DebugLevel;
	uint8_t     Qos;
} R_SET_CONFIG_SPY;

typedef struct dis_stat_spy
{
	MMCHD	mmc_head;
	int	INVITE_Counts;
	int	R100_Counts;
	int	R180_Counts;
	int	R200_Counts;
	int	ACK_Counts;
	int	CANCEL_Counts;
	int	BYE_Counts;
	int	INFO_Counts;
	int	MESSAGE_Counts;
	int	REGISTER_Counts;
	int	OPTIONS_Counts;
	int	InMsgCounts_O;
	int	InMsgCounts_T;
	int	TotalMsgCounts;
	int	INVITE_OutCounts;
	int	R100_OutCounts;
	int	R180_OutCounts;
	int	R200_OutCounts;
	int	ACK_OutCounts;
	int	CANCEL_OutCounts;
	int	BYE_OutCounts;
	int	INFO_OutCounts;
	int	MESSAGE_OutCounts;
	int	REGISTER_OutCounts;
	int	OPTIONS_OutCounts;
	int	OutMsgCounts_O;
	int	OutMsgCounts_T;
	int	TotalMsgOutCounts;
	int	SessionCounts;
	int	SCMFailCounts;
	int	HashFailCounts;
	int	VmRSS;
} DIS_STAT_SPY;

typedef struct
{
	MMCHD	mmc_head;
	uint16_t	bFlag;
	uint16_t	msgType;
	uint16_t	msgResponseCode;
	uint16_t	msgDirection;
	int     msgCount;
	char	strFromAddr[32];
	char	strToAddr[32];
	char	strSvcKey[32];
} R_TRACE_ON_SPY;

#define S_TRACE_ON_SPY  R_TRACE_ON_SPY  // 동일한 구조체

typedef struct dis_trace_item
{
	uint8_t	nIndex;
	char	strFrom[32];
	char	strTo[32];
	char	strSvcKey[32];
	int	nMax;
	int	nCount;
} DIS_TRACE_ITEM;

typedef struct dis_trace_spy
{
	MMCHD	mmc_head;
	DIS_TRACE_ITEM	item[10];	
} DIS_TRACE_SPY;

typedef struct
{
	MMCHD       mmc_head;
	uint8_t     bFlag;
	char        strData[32];
} R_SET_PROXY_SPY;


typedef struct
{
    MMCHD	mmc_head;
    char	dummy;
} UPD_BLACK_LIST_RQ;

typedef struct
{
    MMCHD	mmc_head;
    char	result;
    char    reason;
} UPD_BLACK_LIST_RP;

typedef struct
{
    MMCHD	mmc_head;
    char	act;
} ACT_BLACK_LIST_RQ;

typedef struct
{
    MMCHD	mmc_head;
    char	result;
    char	state;
} ACT_BLACK_LIST_RP;

typedef struct
{
    MMCHD	mmc_head;
    char	result;
    char    reason;
} UPD_EXT_LIST_RP;

#endif // __SPY_MMC_H__

