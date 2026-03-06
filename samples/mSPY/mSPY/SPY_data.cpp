/* File Header
 *fsh**************************************************************
 ******************************************************************
 **
 **  FILE : SPY_data.cpp
 **
 ******************************************************************
 ******************************************************************
 BLOCK          : SPY
 SUBSYSTEM      : CMS
 SOR-NAME       :
 VERSION        : V4.X
 DATE           :
 AUTHOR         : SEUNG-MO, CHO
 HISTORY        :
 PROCESS(TASK)  :
 PROCEDURES     :
 DESCRIPTION    : Global Data 선언
 REMARKS        :
 *end*************************************************************/

#include "SPY_def.h"

uint8_t MY_AS_ID;               // MY AS_INDEX (0 ~ 7)
uint8_t MY_SIDE;                // My SIDE Info. A(0)/B(1)
uint8_t MY_BLK_ID;              // My(SPY) XBUS_ID
uint8_t PEER_BLK_ID;            // 상대편 SPY XBUS_ID

//uint8_t nVersion[] = { '5', '0', '5','\0' };
//char    strUpdate_Date[16]  = "20170710";
//char    strUpdate_Owner[16] = "SMCHO";
//char    strUpdate_Desc[128] = "[CHG] Increase SIP Recv & Send Buffer 4096 -> 8192 (3.1.9)";

//20241226 kdh add for test
std::atomic<int> global_test_spy_send_to_sam_bye_count(0);

SPY_CR      g_CR;               // SPY Call Register Class
SPY_CFG     spy_cfg;            // SPY Config Data
NSM_CFG     cfg;                // LIB SM LOG CFG class
NSM_LOG     Log;                // LIB SM LOG LOG class
STA_INFO    statistic;          // SPY statistic Data
#ifdef IPV6_MODE
STA_INFO        statistic_v6;       // IPv6
BLIST_TBL_V6    g_BlackList_V6;     // IPv6
EXTLIST_TBL_V6  g_ExtList_V6;       // IPv6용
#endif
NSM_PTR_Q       Q_Reg;              // SSW(CSCF)에서 수신한 REGISTER 메세지를 저장하는 Queue
NSM_PTR_Q       Q_Opt;              // SSW(CSCF)에서 수신한 OPTIONS 메세지를 저장하는 Queue
NSM_PTR_Q       Q_Ssw_ORIG;         // SSW(CSCF)에서 수신한 SIP 메세지를 저장하는 Queue (ORIG STACK)
NSM_PTR_Q       Q_Ssw_TERM;         // SSW(CSCF)에서 수신한 SIP 메세지를 저장하는 Queue (TERM STACK)
#ifdef MULTI_Q_MODE // KDH m4.4.1-v2.1.2
NSM_PTR_Q       Q_Sam[MAX_SES_BOARD][MAX_SES_THREAD];
#else
NSM_PTR_Q       Q_Sam[MAX_SES_BOARD];              // SAM에서 수신한 SIP 메세지를 저장하는 Queue
#endif // MULTI_Q_MODE
SCM_CH_MAP      g_ScmMap;           // SCM 채널할당시 등록하여 INVITE재전송시 중복할당 방지 및 SCM에서 응답이 없을 경우 할당된 CR을 해제하는 용도로 사용

#ifdef SES_ACTSTD_MODE
NSM_PTR_Q		Q_Sync_recv; // 20241202 kdh add
#endif

#ifdef FAULT_LOG_MODE //v4.4.2-1.1.0 MYUNG 20230906
FLCM_MANAGER	flcm_manager;
#endif

SPY_HASH_MAP    g_Hash_O;       // ORIG STACK Call 정보를 포함하는 Hash Table (CSCF 발신 AS 착신 호)
SPY_HASH_MAP    g_Hash_T;       // TERM STACK Call 정보를 포함하는 Hash Table (AS 발신 CSCF 착신 호)


SVCKEY_TBL      g_SvcKey;       // service_key.cfg에 등록된 Service Key 관련 Class
EXTLIST_TBL     g_ExtList;      // 등록된 EXT. Server list를 관리하는 Class(extlist.cfg)

ERRSVR_TBL      g_ErrSvrList;   // 문제가 있는 외부 연동 서버를 관리하는 Class
TRACE_TBL       g_Trace;        // Call Trace를 관리하는 Class
BLIST_TBL       g_BlackList;    // add by SMCHO 20140918

#ifdef INCLUDE_REGI
SPY_REGI_STS    g_RegiSts;
#endif

bool    g_bReqQ_Alarm;          // add 20170303
//bool    g_bAsBlock;             // AS BLOCK 여부
char    g_nHA_State;
int     g_ReportTime = 20;        // default 15sec * 20 = 5 min
char    g_strCfgFile[MAX_FNAME_LEN];
//char    g_strAsBlkFile[MAX_FNAME_LEN];

bool    g_DBMAlive[2] = { 0, 0 };


char    STR_SIP_RESPONSE[] = "SIP/2.0 ";
char    STR_SIP_METHOD[13][10] =    /* enum SIP_METHOD와 순서가 반드시 동일해야 함 */
{
    "INVITE",       "ACK",          "CANCEL",       "BYE",      "MESSAGE",
    "REGISTER",     "OPTIONS",      "INFO",         "PRACK",    "UPDATE",
    "SUBSCRIBE",    "NOTIFY",       "REFER"
};

int     g_nSamServerResetCount;
int     g_nSswServerResetCount[2];


