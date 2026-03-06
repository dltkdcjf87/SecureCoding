//
//  SPY_xref.h
//  SPY
//
//  Created by smcho on 11. 11. 30..
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#ifndef __SPY_XREF_H__
#define __SPY_XREF_H__

#include "SPY_def.h"
#include "SPY_chmap.h"
#include "SPY_blist_v6.h"

//#define MAX_TRACE_LIST  10

// 20241205 kdh add m4.4.1-v2.1.2 MULTI_Q_MODE
typedef struct SamQthreadInfo
{
    int bd;
    int queueNo;
} SAMQ_TH_INFO;

/*
 * Extern Global Data
 */
extern  uint8_t MY_AS_ID;                // MY AS_INDEX (0 ~ 7)
extern  uint8_t MY_SIDE;
extern  uint8_t MY_BLK_ID;
extern  uint8_t PEER_BLK_ID;


// 20241226 kdh add for test
extern std::atomic<int> global_test_spy_send_to_sam_bye_count;

//extern  uint8_t nVersion[];
//extern  char    strUpdate_Date[16];
//extern  char    strUpdate_Owner[16];
//extern  char    strUpdate_Desc[128]; 

extern  SPY_CR      g_CR;               // SPY Call Register Class
extern  SPY_CFG     spy_cfg;            // SPY Config Data
extern  NSM_CFG      cfg;                // LIB SM CFG clas
extern  NSM_LOG     Log;                // LIB SM LOG class
extern  STA_INFO    statistic;          // SPY statistic Data
#ifdef IPV6_MODE
extern  STA_INFO        statistic_v6;       // IPv6
extern  BLIST_TBL_V6    g_BlackList_V6;     // IPv6
extern  EXTLIST_TBL_V6  g_ExtList_V6;       // IPv6용
#endif
extern  char    g_nHA_State;

extern  SCM_CH_MAP  g_ScmMap;           // SCM 채널할당시 등록하여 INVITE재전송시 중복할당 방지 및 SCM에서 응답이 없을 경우 할당된 CR을 해제하는 용도로 사용
extern  NSM_PTR_Q   Q_Reg;                  // SSW(CSCF)에서 수신한 REGISTER 메세지를 저장하는 Queue
extern  NSM_PTR_Q   Q_Opt;                  // SSW(CSCF)에서 수신한 OPTIONS 메세지를 저장하는 Queue
extern  NSM_PTR_Q   Q_Ssw_ORIG;             // SSW(CSCF)에서 수신한 SIP 메세지를 저장하는 Queue (ORIG STACK)
extern  NSM_PTR_Q   Q_Ssw_TERM;             // SSW(CSCF)에서 수신한 SIP 메세지를 저장하는 Queue (TERM STACK)
#ifdef MULTI_Q_MODE // KDH m4.4.1-v2.1.2
extern  NSM_PTR_Q   Q_Sam[MAX_SES_BOARD][MAX_SES_THREAD];
#else
extern  NSM_PTR_Q   Q_Sam[MAX_SES_BOARD];   // SAM에서 수신한 SIP 메세지를 저장하는 Queue
#endif // MULTI_Q_MODE

//v4.4.2-1.1.0 MYUNG 20230906
#ifdef FAULT_LOG_MODE
extern  FLCM_MANAGER    flcm_manager;
#endif

extern NSM_PTR_Q		Q_Sync_recv; // 20241202 kdh add

extern  SPY_HASH_MAP    g_Hash_O;       // ORIG STACK Call 정보를 포함하는 Hash Table (CSCF 발신 AS 착신 호)
extern  SPY_HASH_MAP    g_Hash_T;       // TERM STACK Call 정보를 포함하는 Hash Table (AS 발신 CSCF 착신 호)

extern  SVCKEY_TBL      g_SvcKey;       // service_key.cfg에 등록된 Service Key 관련 Class
extern  EXTLIST_TBL     g_ExtList;      // 등록된 EXT. Server list를 관리하는 MAP(extlist.cfg)

extern  ERRSVR_TBL      g_ErrSvrList;   // 문제가 있는 외부 연동 서버를 관리하는 Class
extern  TRACE_TBL       g_Trace;        // Call Trace를 관리하는 Class
extern  BLIST_TBL       g_BlackList;    // add by SMCHO 20140918

#ifdef INCLUDE_REGI
extern  SPY_REGI_STS    g_RegiSts;
#endif

extern  bool    g_bReqQ_Alarm;          // add 20170303
//extern  bool    g_bAsBlock;             // AS BLOCK 여부
extern  int     g_ReportTime;
extern  char    g_strCfgFile[MAX_FNAME_LEN];
//extern  char    g_strAsBlkFile[MAX_FNAME_LEN];

extern  char    STR_SIP_METHOD[13][10];
extern  char    STR_SIP_RESPONSE[];

extern  bool    g_DBMAlive[2];

extern  int     g_nSamServerResetCount;
extern  int     g_nSswServerResetCount[2];

/*
 * Extern Global Functions
 */

// SPY_alm.cpp
extern  int     SEND_AsBlockAlarm(void);
extern  int     SEND_AlarmMessage(int cmd_no, int flag, int alarm_id, const char *extend, const char *comment);
#ifdef OAMSV5_MODE
extern	int		SEND_AlarmV2(int type, int cmd_no, int xbusId, int alarmId, int indexId, int alarmFlag, int commentCnt, const char comments[][64]);
#endif
extern  int     SEND_FaultMessage(int cmd_no, int flag, int alarm_id, char extend, const char *comment);
extern  int     SEND_StatusMessage(uint16_t cmd_no, const char *comment);
extern  void    SEND_Reg_Q_Alarm(char alarm_onoff);
extern  void    CreateAlarmSendTherad(void);

// SPY_cfg.cpp
extern  bool    SPY_ReadConfigFile(void);
extern  bool    SPY_ReadAsBlockFile(void);
extern  bool    SPY_SetAndWriteAsBlockStatus(bool bBlock);

//SPY_ha.cpp
extern  bool    HASH_InsertAndSync(CR_DATA *pCR, time_t t_now);
extern void     HASH_SendHaUpdateRQ(CR_DATA *pCR, bool bIsReceiveACK);
extern bool     HASH_InsertTableRQ(XBUS_MSG *rXBUS); // 20241202 kdh chg
extern bool     HASH_InsertTable(XBUS_MSG *rXBUS); // 20241202 kdh chg
extern void* HASH_InsertTableThread(void* arg); // 20241202 kdh add
extern void     HASH_SyncAllTableRQ(void);
extern void     HASH_HaUpdateRQ(XBUS_MSG *rXBUS);

// SPY_lib.cpp
extern  void    LIB_change_space_to_null(char *str, int len);
extern  void    LIB_delete_special_character(char *str, int value);
extern  void    LIB_delete_special_character(char *str, int value, int value2);
extern  void    LIB_DeleteStartSpace(char *str);

extern  bool    LIB_GetIpAddrInContactHeader(const char *strHeader, char *strIpAddr, int MAX_LEN, int *nPort);
extern  bool    LIB_GetUriAndUserAndHostFromHeader(const char *strHeader, URI_TYPE *nUriType, char *strAddr, int MAX_LEN, char *strUser, int MAX_USER_LEN, char *strHost, int MAX_HOST_LEN);
extern  bool    LIB_GetUserAndHostFromHeader(const char *strHeader, char *strUser, int MAX_USER_LEN, char *strHost, int MAX_HOST_LEN);
extern  bool    LIB_GetUriFromHeader(const char *strHeader, URI_TYPE *nUriType, char *strAddr, int MAX_LEN);
extern  bool    LIB_GetUriFromHeader2(const char *strHeader, char *strURI, int MAX_LEN);        // include "sip:" or "tel:"
extern  bool    LIB_GetUserFromHeader(const char *strHeader, char *strUser, int MAX_USER_LEN);
extern  bool    LIB_GetHostFromHeader(const char *strHeader, char *strHost, int MAX_HOST_LEN);

extern  int     LIB_GetAddrFromFromToHeader(const char *strHeader, char *strAddr, int MAX_LEN);
extern  bool    LIB_GetAddrFromURI(const char *strURI, char *strAddr, int MAX_LEN, char *strUser, int MAX_USER_LEN, char *strHost);

extern  int     LIB_GetAddrFromViaHeader(const char *strBuf, char *ip, int MAX_LEN, int *port);
extern  bool    LIB_GetAndDelHeader(char *strBuf, const char *strHeaderName, char *strValue, int MAX_VALUE_LEN);
extern  bool    LIB_DelSipHeaderNameAndValue(char *strBuf, const char *strHeaderName);
extern  int     LIB_GetStrFromAToB(const char *strBuf, const char *strA, const char *strB, char *strResult, int MAX_LEN);
extern  bool    LIB_GetStrUntilCRLF(const char *strBuf, const char *strHeader, char *strOut, int MAX_LEN);
extern  bool    LIB_GetIntUntilCRLF(const char *strBuf, const char *strHeader, int *intOut);
extern  int     LIB_GetHeaderCount(const char *strBuf, const char *strHeader);
extern  int     LIB_GetSipRequestMethod(char *strRcvBuf);
extern  int     LIB_GetSipResponseCode(char *strRcvBuf);
extern  bool    LIB_GetSipTypeAndMethod(char *strRcvBuf, int *type, int *method);


extern  int     LIB_GetSipHeaderValue(const char *strSipMsg, const char *strHeader, char *strValue, int MAX_LEN);

//extern  bool    LIB_AddSpyViaHeader(CR_DATA *pCR, char *strSendBuf);
extern  bool    LIB_DelSpyViaHeader(CR_DATA *pCR, char *strSendBuf);
extern  bool    LIB_AppendHeader(char *strSendBuf, int *nSendLen, const char *strRcvBuf, const char *strHeader);
extern  bool    LIB_DeleteHeader(char *strSendBuf, int *nSendLen, const char *strHeader);

// SPY_main.cpp
extern  void    GetParams(char* pValue, char* IP, char* sBlock, char* sMonitor, int* nCheckTime, char *nTryCount);
extern  void    shut_down(int reason);


// SPY_mmc.cpp
extern  void    set_mmc_head(MMCHD *send, MMCHD *recv, int len, bool bContinue);
extern  int     MBUS_MsgFromMGM(int len, XBUS_MSG *xbus_msg);
// SPY_mmc2.cpp
extern  void    MMC_set_header(MMCHD *send, MMCHD *recv, int len, bool bContinue);
extern  void    MBUS_MsgFromMGM2(int len, XBUS_MSG *xbus_msg);      // mPBX에서는  MBUS_MsgFromMGM()를 사용하지 않고 MBUS_MsgFromMGM2()를 사용

// SPY_opt.cpp
extern  void    *RSSW_DequeueThread_OPT(void *arg);   // OPTIONS 메세지만 처리하는  Thread


// SPY_rssw.cpp
extern  bool    RSSW_main(void);
extern  bool    RSSW_MakeSendBufferAndSendToSAM(CR_DATA *pCR);
extern  bool    RSSW_SendResponse(CR_DATA *pCR, int nRespCode, const char *strReason);
#ifdef SB_MODE
extern  void    RSAM_Add_1stRecordRouteTo200INVITE(CR_DATA *pCR, char *strSendBuf);
extern void RSAM_Delete_FirstRoute(CR_DATA *pCR, char *strSendBuf);
#endif
// SPY_rsam.cpp
extern  bool    RSAM_main(void);

// SPY_regi.cpp
extern  void    *RSSW_DequeueThread_REG(void *arg);   // REGISTER 메세지만 처리하는  Thread
extern  void    Send_RegisterResponse(char *strSrcIpAddr, int nSrcPort, char *strRcvBuf, int nReason, const char* strReason);   // 140108 add by SMCHO (REG-Q)
extern  bool    REGI_IsDeREGISTER(CR_DATA *pCR);

#ifdef SOIP_REGI
extern	void 	Send3rdREGISTER(CR_DATA *pCR);
extern	void 	Send3rdREGISTER_TAS(CR_DATA *pCR);
#endif

// SPY_scm.cpp
extern  int     SendChDeallocRQ_SCM(int nBoard, int nChannel);
extern  bool    SEND_ChAllocRQToSCM(CR_DATA *pCR);
extern  int     MBUS_MsgFromSCM(int len, XBUS_MSG *rXBUS);
extern  int     SendOptionResponseRP_SCM(uint16_t nRespCode, char *strContact, char *strTo, char *strAV);

// SPY_stat.cpp
extern  void    MMC_DisStatSpy(MMCPDU *rMMC);
extern  void    MMC_ClrStatSpy(MMCPDU *rMMC);
extern  void    MMC_ClrCallSpy(MMCPDU *rMMC);
#ifdef IPV6_MODE
extern  void    MMC_DisStatSpy_V6(MMCPDU *rMMC);
extern  void    MMC_ClrStatSpy_V6(MMCPDU *rMMC);
extern  void    MMC_ClrCallSpy_V6(MMCPDU *rMMC);
#endif

extern  void    STAT_ADD_REGISTER_IN(CR_DATA *pCR);
extern  void    STAT_ADD_REGISTER_SUCCESS(CR_DATA *pCR);
extern  void    STAT_ADD_REGISTER_SUCCESS_REGI(CR_DATA *pCR);
extern  void    STAT_ADD_REGISTER_SUCCESS_DEREGI(CR_DATA *pCR);
extern  void    STAT_ADD_REGISTER_FAIL(CR_DATA *pCR);
extern  void    STAT_ADD_REGISTER_FAIL_REGI(CR_DATA *pCR);
extern  void    STAT_ADD_REGISTER_FAIL_DEREGI(CR_DATA *pCR);
//extern  void    STAT_send_stat_info(uint8_t nCommand);
extern  int     MBUS_MsgFromSGM(int len, XBUS_MSG *xbus_msg);


// SPY_send.cpp
extern  void    SEND_LogLevelToEMS(void);
extern  void    SEND_AsBlockStatusToEMS(void);


// SPY_xbus.cpp
extern  int     SUBS_callback(int len, uint8_t *msg_ptr);
extern  int     MBUS_callback(int len, uint8_t *msg_ptr);
extern  bool    SPY_init_xbus(void);


#endif // __SPY_XREF_H__


