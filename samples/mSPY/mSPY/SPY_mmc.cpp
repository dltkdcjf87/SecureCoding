/* File Header
 *fsh**************************************************************
 ******************************************************************
 **
 **  FILE : SPY_mmc.cpp
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
 DESCRIPTION    : MMC 관련 functions
 REMARKS        :
 *end*************************************************************/

#include "SPY_def.h"
#include "SPY_xref.h"


#pragma mark -
#pragma mark [MMC] common

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : set_mmc_head
 * CLASS-NAME     :
 * PARAMETER   OUT: send - 전송할 MMC Header pointer
 *              IN: recv - 수신한 MMC Header pointer
 *              IN: len  - 전송할 MMC Data Size
 *              IN: bContinue - MMC Type 정보이나 사용 안함
 * RET. VALUE     : -
 * DESCRIPTION    : 전송할 MMC Header setting
 * REMARKS        : 수신된 Header에서 copy
 **end*******************************************************/
void set_mmc_head(MMCHD *send, MMCHD *recv, int len, bool bContinue)
{
	send->Len    = len;
	send->MsgID  = recv->MsgID;
	send->CmdNo  = recv->CmdNo;
	send->From   = MY_BLK_ID;             // SPY_A or SPY_B;
	send->To     = recv->From;
	send->Type   = MMC_TYPE_BYPASS;
    //	send->Type  = bContinue ? MMC_TYPE_BYPASS : MMC_TYPE_END;
	send->JobNo  = recv->JobNo;
	send->arg1   = recv->arg1;
	send->arg2   = recv->arg2;
	send->Time   = (uint32_t)time(NULL);
	send->SockFD = recv->SockFD;
}

#pragma mark -
#pragma mark [MMC] RELOAD hosts.cfg

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : MMC_ReloadCfgSpy
 * CLASS-NAME     : -
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : Sending MMC Host Allow/Deny List Message
 * REMARKS        : MMC 메세지 - FIXIT
 **end*******************************************************/
void MMC_ReloadCfgSpy(MMCPDU *msg_MMC)
{
#if 0
// 예전 HOSTTBL 관련 MMC로 관련 소스 완전 삭제
#endif
}


#pragma mark -
#pragma mark [MMC] SET/DIS TRACE


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MMC_TraceOnSpy
 * CLASS-NAME     :
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : SIP Trace Start 요청 메세지를 처리하는 함수
 * REMARKS        : from MMC
 **end*******************************************************/
void MMC_TraceOnSpy(MMCPDU *rMMC)
{
    R_TRACE_ON_SPY   *rcv = (R_TRACE_ON_SPY *)rMMC;
	S_TRACE_ON_SPY	send;
    TRACE_LIST      trace;
    int         nIndex = -1;
    bool        result;
    
    bzero(&trace, sizeof(trace));
    
    if (rcv->strFromAddr[0] == '\0') { strcpy(trace.strFrom, "-"); }
    else                             { strncpy(trace.strFrom, rcv->strFromAddr, sizeof(trace.strFrom)-1); }
    
    if (rcv->strToAddr[0] == '\0') { strcpy(trace.strTo, "-"); }
    else                           { strncpy(trace.strTo, rcv->strToAddr, sizeof(trace.strTo)-1); }
#if 0
    if (rcv->strSvcKey[0] == '\0') { strcpy(trace.strSvcKey, "-"); }
    else                           { strncpy(trace.strSvcKey, rcv->strSvcKey, sizeof(trace.strSvcKey)-1); }
#else
    strcpy(trace.strSvcKey, "-");       // unuse ServiveKey field
#endif
    
    trace.nMaxCount = rcv->msgCount;
    result = g_Trace.insert(&trace, &nIndex);

    
    
	bzero(&send, sizeof(send));
	set_mmc_head(&send.mmc_head, (MMCHD*)&rcv->mmc_head, sizeof(send), false);
    
    if(result)
    {
        send.bFlag             = true;
        send.msgType           = 0;     // 0: TRACE-ON
        send.msgDirection      = 2;     // 0:RCV, 1:SEND, 2:NONE
        
        strncpy(send.strFromAddr, rcv->strFromAddr, sizeof(send.strFromAddr)-1);
        strncpy(send.strToAddr,   rcv->strToAddr,   sizeof(send.strToAddr)-1);
        strncpy(send.strSvcKey,   rcv->strSvcKey,   sizeof(send.strSvcKey)-1);
    }
    else
    {
     	send.bFlag          = false;
        send.msgType		= 17;       // TRACE-FAIL
        send.msgDirection	= 0;
    }
    
	msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
    
    Log.printf(LOG_LV2, "MMC Trace-on Command Request (Index=%d)... List Count=%d, From=%s, To=%s, SvcKey=%s\n", nIndex, g_Trace.size(), trace.strFrom, trace.strTo, trace.strSvcKey);
}


/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : MMC_TraceListSpy
 * CLASS-NAME     :
 * PARAMETER      : IN
 * RET. VALUE     : -
 * DESCRIPTION    : Sending MMC TRACE List Message
 * REMARKS        : MMC 메세지
 **end*******************************************************/
void MMC_TraceListSpy(MMCPDU *rMMC)
{
	DIS_TRACE_SPY	send;
    
	bzero(&send, sizeof(send));
	set_mmc_head(&send.mmc_head, &rMMC->Head, sizeof(send), false);
    
	for(int i = 0; i < MAX_TRACE_LIST; i++)
	{
		send.item[i].nIndex	= i;
        g_Trace.get_trace(i, send.item[i].strFrom, send.item[i].strTo, send.item[i].strSvcKey, &(send.item[i].nMax), &(send.item[i].nCount));
	}
    
	msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
}


/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : MMC_TraceOffSpy
 * CLASS-NAME     :
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : Trae 중지 요청 처리 함수
 * REMARKS        : MMC 메세지
 **end*******************************************************/
void MMC_TraceOffSpy(MMCPDU *rMMC)
{
    R_TRACE_ON_SPY   *rcv = (R_TRACE_ON_SPY *)rMMC;
    
    g_Trace.erase(rcv->msgCount);
    
    MMC_TraceListSpy(rMMC);
    
    Log.printf(LOG_LV2, "MMC Trace-off Command Request\n");
}


#pragma mark -
#pragma mark [MMC] DIS/SET LOG

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MMC_DisCfgSpy
 * CLASS-NAME     : -
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : Config 정보를 출력하는 함수
 * REMARKS        : MMC 메세지 - FIXIT V6 정보 추가
 **end*******************************************************/
void MMC_DisCfgSpy(MMCPDU *rMMC)
{
	R_DIS_CFG_SPY	send;
    
	bzero(&send, sizeof(send));
    
	set_mmc_head(&send.mmc_head, &rMMC->Head, sizeof(send), false);
    
	send.LogFile          = true;
	send.DebugLevel       = Log.get_level();
	send.Qos              = C_NETWORK_QOS;
	send.bFlag            = C_OUTBOUND_PROXY;
    snprintf(send.strOrigStack, sizeof(send.strOrigStack), "%s:%d", C_ORIG_STACK_IP, C_ORIG_STACK_PORT);
	snprintf(send.strTermStack, sizeof(send.strTermStack), "%s:%d", C_TERM_STACK_IP, C_TERM_STACK_PORT);
	strncpy(send.strData, C_OUTBOUND_IP, sizeof(send.strData)-1);
    
	msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
    
    Log.printf(LOG_INF, "[MMC] MMC_DisCfgSpy() Current Log Level=%d, QoS=%d\n", C_LOG_LEVEL, C_NETWORK_QOS);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MMC_SetCfgSpy
 * CLASS-NAME     : -
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : Config 정보를 수정하는 함수
 * REMARKS        : MMC 메세지
 **end*******************************************************/
void MMC_SetCfgSpy(MMCPDU *rMMC)
{
    R_SET_CONFIG_SPY *rPDU = (R_SET_CONFIG_SPY *)rMMC;

    if(rPDU->LogFile == false) { C_LOG_LEVEL = LOG_ERR; }
    else                       { C_LOG_LEVEL = rPDU->DebugLevel; }
    Log.set_level(C_LOG_LEVEL);

    C_NETWORK_QOS = rPDU->Qos;
    
    SEND_LogLevelToEMS();

    MMC_DisCfgSpy(rMMC);
}


#pragma mark -
#pragma mark [MMC] DIS/SET BLOCK/UNBLOCK

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SPY_SetAndWriteAsBlockStatus
 * CLASS-NAME     : -
 * PARAMETER    IN: bBlock - Block 여부
 * RET. VALUE     : BOOL
 * DESCRIPTION    : AS Block 정보를 변수와 파일에 UPDATE하는 함수
 * REMARKS        :
 **end*******************************************************/
bool SPY_SetAndWriteAsBlockStatus(bool bBlock)
{
    C_AS_BLOCK = bBlock;        //  set to global variable
    
    cfg.SetBool("MODULE", "AS_BLOCK", C_AS_BLOCK);
    
    if(cfg.SaveFile() == true) { Log.printf(LOG_INF, "SPY_SetAndWriteAsBlockStatus(%d) SAVE OK !!\n", bBlock); }
    else                       { Log.printf(LOG_ERR, "SPY_SetAndWriteAsBlockStatus(%d) SAVE FAIL !!\n", bBlock); }
    
    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MMC_DisBlockSpy
 * CLASS-NAME     :
 * PARAMETER      : IN
 * RET. VALUE     : -
 * DESCRIPTION    : Sending MMC Block-Message
 * REMARKS        :
 **end*******************************************************/
void MMC_DisBlockSpy(MMCPDU *msg_MMC)
{
	S_DIS_BLOCK_SPY     send;
    
	bzero(&send, sizeof(send));
    
	set_mmc_head(&send.mmc_head, &msg_MMC->Head, sizeof(send), false);
	send.result[0] = C_AS_BLOCK;
    
	msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
    
    Log.printf(LOG_LV2, "[MMC] Current g_bAsBlock=%s\n", C_AS_BLOCK ? "BLOCK" : "UNBLOCK");
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MMC_SetBlockSpy
 * CLASS-NAME     : -
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : Config 정보를 수정하는 함수
 * REMARKS        : MMC 메세지
 **end*******************************************************/
void MMC_SetBlockSpy(MMCPDU *rMMC)
{
    R_SET_BLOCK_SPY *rPDU = (R_SET_BLOCK_SPY *)rMMC;
    
    C_AS_BLOCK = rPDU->block;
    
    MMC_DisBlockSpy(rMMC);
    
    SPY_SetAndWriteAsBlockStatus(C_AS_BLOCK);
    
    if(g_nHA_State == HA_Active)
    {
        SEND_AsBlockStatusToEMS();
        SEND_AsBlockAlarm();
    }
}


#pragma mark -
#pragma mark [MMC] BLACK LIST ACTIVATE/UPDATE

#ifdef IPV6_MODE
/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : MMC_BlackListUpdateRQ
 * CLASS-NAME     :
 * PARAMETER    IN: rmmc - xbus에서 수신한 MMC PDU
 * RET. VALUE     : -
 * DESCRIPTION    : Black List Update Request
 * REMARKS        : MMC에서 요청
 **end*******************************************************/
int MMC_BlackListUpdateRQ_V6(MMCPDU *rmmc)
{
    UPD_BLACK_LIST_RP   send;
    
    Log.printf(LOG_INF, "[MMC] Bkack List Update Request IPv6\n");
    
    bzero(&send, sizeof(send));
    set_mmc_head(&send.mmc_head, &rmmc->Head, sizeof(send), false);
    
    switch(g_BlackList_V6.read_again())
    {
        case BLIST_OK:
            if(g_BlackList_V6.size() == 0)
            {
                Log.printf(LOG_ERR, "Black List Update Success but No IPv6 List\n");
                send.result = false;
                send.reason = BLIST_IP_NOT_EXIST;
            }
            else
            {
                send.result  = true;
            }
            break;
        case BLIST_FILEOPEN_ERR:
            send.result = false;
            send.reason  = BLIST_FILEOPEN_ERR;
            break;
        case BLIST_FILENAME_ERR:
            send.result = false;
            send.reason  = BLIST_FILENAME_ERR;
            break;
        default:
            send.result = false;
            send.reason  = BLIST_UNDEFINED_ERR;
    }
    
    msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
    return(0);
}
#endif      // IPV6_MODE

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : MMC_BlackListUpdateRQ
 * CLASS-NAME     :
 * PARAMETER      : IN
 * RET. VALUE     : -
 * DESCRIPTION    : Black List Update Request
 * REMARKS        : MMC에서 요청
 *                : Add 20140918 by SMCHO
 **end*******************************************************/
int MMC_BlackListUpdateRQ(MMCPDU *rmmc)
{
    UPD_BLACK_LIST_RP   send;
    
    Log.printf(LOG_INF, "[MMC] Bkack List Update Request \n");
    
    bzero(&send, sizeof(send));
    set_mmc_head(&send.mmc_head, &rmmc->Head, sizeof(send), false);
    
    switch(g_BlackList.read_again())
    {
        case BLIST_OK:
            if(g_BlackList.size() == 0)
            {
                Log.printf(LOG_ERR, "Black List Update Success but No IP List\n");
                send.result = false;
                send.reason = BLIST_IP_NOT_EXIST;
            }
            else
            {
                send.result  = true;
            }
            break;
        case BLIST_FILEOPEN_ERR:
            send.result = false;
            send.reason  = BLIST_FILEOPEN_ERR;
            break;
        case BLIST_FILENAME_ERR:
            send.result = false;
            send.reason  = BLIST_FILENAME_ERR;
            break;
        default:
            send.result = false;
            send.reason  = BLIST_UNDEFINED_ERR;
    }
    
    msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
    return(0);
}

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : MMC_BlackListActivateRQ
 * CLASS-NAME     :
 * PARAMETER      : IN
 * RET. VALUE     : -
 * DESCRIPTION    : Black List Activate Request
 * REMARKS        : MMC에서 요청
 *                : Add 20140918 by SMCHO
 **end*******************************************************/
int MMC_BlackListActivateRQ(MMCPDU *rmmc)
{
    ACT_BLACK_LIST_RQ   *rcv = (ACT_BLACK_LIST_RQ *)rmmc;
    ACT_BLACK_LIST_RP   send;
    
    Log.printf(LOG_INF, "[MMC] Bkack List Activate Request \n");
    
    bzero(&send, sizeof(send));
    set_mmc_head(&send.mmc_head, &rmmc->Head, sizeof(send), false);
    
    if(rcv->act) { C_BLACK_LIST_USE = true;  }
    else         { C_BLACK_LIST_USE = false; }
    
    send.result  = true;
    send.state   = C_BLACK_LIST_USE;    // V4/V6 공통
    
    msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
    return(0);
}

#ifdef IPV6_MODE
/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : MMC_ExtListUpdateRQ_V6
 * CLASS-NAME     :
 * PARAMETER      : IN
 * RET. VALUE     : -
 * DESCRIPTION    : SSWList & EXTList Update Request
 * REMARKS        : MMC에서 요청
 *                : Add 20140924 by SMCHO
 **end*******************************************************/
int MMC_ExtListUpdateRQ_V6(MMCPDU *rmmc)
{
    //	UPD_EXT_LIST_RQ   *rcv = (UPD_EXT_LIST_RQ *)rmmc;
    UPD_EXT_LIST_RP   send;
    
    Log.printf(LOG_LV1, "[MMC] IPv6 SSWList & EXTList File Update Request\n");
    
    bzero(&send, sizeof(send));
    set_mmc_head(&send.mmc_head, &rmmc->Head, sizeof(send), false);
    
    send.result = g_ExtList_V6.reread();
    
    Log.printf(LOG_INF, "MMC_ExtListUpdateRQ_V6() %s Current Ext List count=%d\n", (send.result== true) ? "OK": "Fail", g_ExtList_V6.size());
    
    msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
    return(0);
}
#endif

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : MMC_ExtListUpdateRQ
 * CLASS-NAME     :
 * PARAMETER      : IN
 * RET. VALUE     : -
 * DESCRIPTION    : SSWList & EXTList Update Request
 * REMARKS        : MMC에서 요청
 *                : Add 20140924 by SMCHO
 **end*******************************************************/
int MMC_ExtListUpdateRQ(MMCPDU *rmmc)
{
    //	UPD_EXT_LIST_RQ   *rcv = (UPD_EXT_LIST_RQ *)rmmc;
    UPD_EXT_LIST_RP   send;
    
    Log.printf(LOG_INF, "[MMC] SSWList & EXTList File Update Request\n");
    
    bzero(&send, sizeof(send));
    set_mmc_head(&send.mmc_head, &rmmc->Head, sizeof(send), false);
    
    send.result = g_ExtList.reread();

    Log.printf(LOG_INF, "MMC_ExtListUpdateRQ() %s Current Ext List count=%d\n", (send.result== true) ? "OK": "Fail", g_ExtList.size());
    
    msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
    return(0);
}


#pragma mark -
#pragma mark [MMC] ETC

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MMC_SetProxySpy
 * CLASS-NAME     : -
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : PROXY 정보를 설정하는 함수
 * REMARKS        : MMC 메세지 - FIXIT: 사용안해도 됨....
 **end*******************************************************/
void MMC_SetProxySpy(MMCPDU *rMMC)
{
    R_SET_PROXY_SPY  *rPDU = (R_SET_PROXY_SPY *)rMMC;
    
    Log.printf(LOG_LV2, "MMC Set Outbound-Proxy Command Request... Use=%d, IP=%s\n", rPDU->bFlag, rPDU->strData);
    
    C_OUTBOUND_PROXY = rPDU->bFlag;
    strcpy(C_OUTBOUND_IP, rPDU->strData);
    
    MMC_DisCfgSpy(rMMC);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MMC_SaveCfgSPY
 * CLASS-NAME     : -
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : Config 변수를 Config File로  Write하는 함수
 * REMARKS        : MMC 메세지
 **end*******************************************************/
void MMC_SaveCfgSPY(MMCPDU *rMMC)
{
    S_SAVE_CONFIG send;
    
    bzero(&send, sizeof(send));
    
    set_mmc_head(&send.mmc_head, &rMMC->Head, sizeof(send), false);
    
    if((send.result = cfg.SaveFile()) == true) { Log.printf(LOG_INF, "[MMC] %s() cfg.SaveFile() SAVE OK !!\n",   __FUNCTION__); }
    else                                       { Log.printf(LOG_ERR, "[MMC] %s() cfg.SaveFile() SAVE FAIL !!\n", __FUNCTION__); }
    
    msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MMC_DisOverResp
 * CLASS-NAME     : -
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : Overload 시 호를 제한할 때 사용하는 Response code (0이면 무응답)를 출력하는 함수
 * REMARKS        : MMC 메세지
 **end*******************************************************/
void MMC_DisOverResp(MMCPDU *rMMC)
{
    S_DIS_OVER_RESP	send;
    
    bzero(&send, sizeof(send));
    
    set_mmc_head(&send.mmc_head, &rMMC->Head, sizeof(send), false);
    
    send.resp_code = C_OVERLOAD_RESP_CODE;

    msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
    
    Log.printf(LOG_INF, "[MMC] %s() New OVERLOAD_RESP_CODE=%d\n", __FUNCTION__, C_OVERLOAD_RESP_CODE);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MMC_SetOverResp
 * CLASS-NAME     : -
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : Overload 시 Response code를 설정하는 함수
 * REMARKS        : MMC 메세지
 **end*******************************************************/
void MMC_SetOverResp(MMCPDU *rMMC)
{
    R_SET_OVER_RESP *rPDU = (R_SET_OVER_RESP *)rMMC;

    Log.printf(LOG_INF, "[MMC] %s() Old OVERLOAD_RESP_CODE=%d\n", __FUNCTION__, C_OVERLOAD_RESP_CODE);
    
    C_OVERLOAD_RESP_CODE = rPDU->resp_code;     // set
    
    cfg.SetInt("ETC", "OVERLOAD_RESP_CODE", C_OVERLOAD_RESP_CODE);

//    if(cfg.SaveFile()) { Log.printf(LOG_INF, "%s() cfg.SaveFile() SAVE OK !!\n",   __FUNCTION__); }
//    else               { Log.printf(LOG_ERR, "%s() cfg.SaveFile() SAVE FAIL !!\n", __FUNCTION__); }
    
    MMC_DisOverResp(rMMC);
}

#pragma mark -
#pragma mark Processing Received XBUS Message from MGM

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MBUS_MsgFromMGM
 * CLASS-NAME     :
 * PARAMETER    IN: len     - 수신한 메세지 길이
 *              IN: msg_ptr - 수신한 메세지 포인터
 * RET. VALUE     : -
 * DESCRIPTION    : MGM(MMC)에서 수신받은 XBUS 메세지를 처리하는 함수
 * REMARKS        : MMC 메세지
 **end*******************************************************/
int MBUS_MsgFromMGM(int len, XBUS_MSG *rXBUS)
{
    MMCPDU *rMMC  = (MMCPDU *)rXBUS->Data;

    switch(rMMC->Head.CmdNo)
    {
        case CNM_DisCfgSPY:     MMC_DisCfgSpy(rMMC);    return(0);
        case CNM_SetCfgSPY:     MMC_SetCfgSpy(rMMC);    return(0);
            
        case CNM_DisBlockSPY:   MMC_DisBlockSpy(rMMC);  return(0);
        case CNM_SetBlockSPY:   MMC_SetBlockSpy(rMMC);  return(0);

        case CNM_DisStatSPY:
#ifdef IPV6_MODE
            MMC_DisStatSpy_V6(rMMC);
#else
            MMC_DisStatSpy(rMMC);
#endif
            return(0);
        case CNM_ClrStatSPY:
#ifdef IPV6_MODE
            MMC_ClrStatSpy_V6(rMMC);
#else
            MMC_ClrStatSpy(rMMC);
#endif
            return(0);
        case CNM_ClrCallSPY:
#ifdef IPV6_MODE
            MMC_ClrCallSpy_V6(rMMC);
#else
            MMC_ClrCallSpy(rMMC);
#endif
            return(0);
            
        case CNM_ReloadCfgSPY:  MMC_ReloadCfgSpy(rMMC); return(0);

        case CNM_TraceOnSPY:    MMC_TraceOnSpy(rMMC);   return(0);
        case CNM_TraceOffSPY:   MMC_TraceOffSpy(rMMC);  return(0);
        case CNM_TraceListSPY:  MMC_TraceListSpy(rMMC); return(0);
            
        case CNM_BlackListUpdate:   MMC_BlackListUpdateRQ(rMMC);     return(true);   // 20140918 add by SMCHO
        case CNM_BlackListAct:      MMC_BlackListActivateRQ(rMMC);   return(true);   // 20140918 add by SMCHO
        case CNM_ExtListUpdate:     MMC_ExtListUpdateRQ(rMMC);       return(true);   // 20140924 add by SMCHO
            
#ifdef IPV6_MODE
        case CNM_BlackListUpdate_V6: MMC_BlackListUpdateRQ_V6(rMMC);     return(true);
        case CNM_ExtListUpdate_V6:   MMC_ExtListUpdateRQ_V6(rMMC);       return(true);
#endif
        case CNM_SetProxySPY:       MMC_SetProxySpy(rMMC);  return(0);
            
        case CNS_SaveCfgSPY:        MMC_SaveCfgSPY (rMMC);  return(0);          // 20160513 add for mPBX BMT
        case CNS_DisOverResp:       MMC_DisOverResp(rMMC);  return(0);          // 20160513 add for mPBX BMT
        case CNS_SetOverResp:       MMC_SetOverResp(rMMC);  return(0);          // 20160513 add for mPBX BMT

        default: Log.printf(LOG_ERR, "[MMC] Undefiend MMC[%d]\n", rMMC->Head.CmdNo); return(0);
    }
    
    return(0);
}
