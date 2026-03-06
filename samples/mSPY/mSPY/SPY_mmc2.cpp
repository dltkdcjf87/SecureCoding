/* File Header
 *fsh**************************************************************
 ******************************************************************
 **
 **  FILE : SPY_mmc2.cpp
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
 REMARKS        : mPBX BMT용으로 새로 만듬
 *end*************************************************************/

#include "SPY_def.h"
#include "SPY_xref.h"


#pragma mark -
#pragma mark [MMC] common

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MMC_set_header
 * CLASS-NAME     :
 * PARAMETER   OUT: send - 전송할 MMC Header pointer
 *              IN: recv - 수신한 MMC Header pointer
 *              IN: len  - 전송할 MMC Data Size
 *              IN: bContinue - MMC Type 정보이나 사용 안함
 * RET. VALUE     : -
 * DESCRIPTION    : 전송할 MMC Header setting
 * REMARKS        : 수신된 Header에서 copy
 **end*******************************************************/
void MMC_set_header(MMCHD *send, MMCHD *recv, int len, bool bContinue)
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
#pragma mark [MMC] LOG & CFG

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MMC2_DisCfgSpy
 * CLASS-NAME     : -
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : Config 정보를 조회하는 MMC
 * REMARKS        : DIS-CFG-SPY:ALL
 **end*******************************************************/
void MMC2_DisCfgSpy(DIS_CFG_SPY_R *rMMC)
{
    DIS_CFG_SPY_S	send;
    
    bzero(&send, sizeof(send));
    
    MMC_set_header(&send.mmc_head, &rMMC->mmc_head, sizeof(send), false);
    
    send.nLogLevel      = Log.get_level();
    if(send.nLogLevel == LOG_OFF) { send.nLogLevel = 0; }
    send.b_log_options  = C_LOG_OPTIONS;
    send.b_log_register = C_LOG_REGISTER;
    send.nQoS           = C_NETWORK_QOS;
    snprintf(send.strOrigStackAddr,    sizeof(send.strOrigStackAddr),    "%s", C_ORIG_STACK_IP_PORT);
    snprintf(send.strTermStackAddr,    sizeof(send.strTermStackAddr),    "%s", C_TERM_STACK_IP_PORT);
#ifdef IPV6_MODE
    snprintf(send.strOrigStackAddr_V6, sizeof(send.strOrigStackAddr_V6), "%s", C_ORIG_STACK_IP_PORT_V6);
    snprintf(send.strTermStackAddr_V6, sizeof(send.strTermStackAddr_V6), "%s", C_TERM_STACK_IP_PORT_V6);
#endif
    msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
    
    Log.printf(LOG_INF, "[MMC] %s() Current Log Level=%d, QoS=%d\n", __FUNCTION__, C_LOG_LEVEL, C_NETWORK_QOS);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MMC2_SetLogSpy
 * CLASS-NAME     : -
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : Log Level을 설정하는 MMC
 * REMARKS        : 04501::-:SET-LOG-SPY:ALL
 **end*******************************************************/
void MMC2_SetLogSpy(SET_LOG_SPY_R *rMMC)
{
    SET_LOG_SPY_S send;
    
    bzero(&send, sizeof(send));
    
    MMC_set_header(&send.mmc_head, &rMMC->mmc_head, sizeof(send), false);

    if((LOG_LV1 <= rMMC->nLogLevel) && (rMMC->nLogLevel <= LOG_OFF))
    {
        C_LOG_LEVEL = rMMC->nLogLevel;
        
        if(C_LOG_LEVEL == LOG_LV1) { Log.set_debug(); }  // 160823 - Log Level 1 은 별도 함수로 설정하게 변경 (운용자 실수 방지)
        else                       { Log.set_level(C_LOG_LEVEL); }
        
        C_LOG_OPTIONS  = rMMC->b_log_options;
        C_LOG_REGISTER = rMMC->b_log_register;
        send.result    = MMC2_RESULT_OK;
        cfg.SetInt ("DEBUG", "LOG_LEVEL",    C_LOG_LEVEL);
        cfg.SetBool("DEBUG", "LOG_OPTIONS",  C_LOG_OPTIONS);
        cfg.SetBool("DEBUG", "LOG_REGISTER", C_LOG_REGISTER);
    }
    else
    {
        send.result = MMC2_RESULT_FAIL;
        send.reason = MMC_PARAMETER_ERROR;
    }

    send.nLogLevel      = Log.get_level();
    if(send.nLogLevel == LOG_OFF) { send.nLogLevel = 0; }
    send.b_log_options  = C_LOG_OPTIONS;
    send.b_log_register = C_LOG_REGISTER;
    
    msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
    
    Log.printf(LOG_INF, "[MMC] %s() result=%s input(LOG_LEVEL=%d, LOG_OPTIONS=%d, LOG_REGISTER=%d)\n", __FUNCTION__, (send.result== MMC2_RESULT_OK) ? "OK" : "NOK",rMMC->nLogLevel, rMMC->b_log_options, rMMC->b_log_register);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MMC2_SetQosSpy
 * CLASS-NAME     : -
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : QoS 값을 설정하는 MMC
 * REMARKS        : MMC 메세지
 **end*******************************************************/
void MMC2_SetQosSpy(SET_QOS_SPY_R *rMMC)
{
    SET_QOS_SPY_S send;
    
    bzero(&send, sizeof(send));
    
    MMC_set_header(&send.mmc_head, &rMMC->mmc_head, sizeof(send), false);
    
    C_NETWORK_QOS = rMMC->nQoS;
    
    send.result   = MMC2_RESULT_OK;
    send.nQoS     = C_NETWORK_QOS;
    
    msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
    
    Log.printf(LOG_INF, "[MMC] %s() Current QoS=%d\n", __FUNCTION__, C_NETWORK_QOS);
}


#pragma mark -
#pragma mark [MMC] BLOCK/UNBLOCK

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MMC_DisBlockSpy
 * CLASS-NAME     :
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : SPY Block/Upblock 상태를 조회하는 MMC
 * REMARKS        : MMC 메세지
 **end*******************************************************/
void MMC2_DisBlockSpy(DIS_BLOCK_SPY_R *rMMC)
{
    DIS_BLOCK_SPY_S     send;
    
    bzero(&send, sizeof(send));
    
    MMC_set_header(&send.mmc_head, &rMMC->mmc_head, sizeof(send), false);
    
    send.block_state = C_AS_BLOCK;
    
    msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
    
    Log.printf(LOG_INF, "[MMC] %s() Current SPY Block=%s\n", __FUNCTION__, C_AS_BLOCK ? "BLOCK" : "UNBLOCK");
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MMC2_SetBlockSpy
 * CLASS-NAME     : -
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : SPY Block/Upblock 상태를 설정하는 MMC
 * REMARKS        : MMC 메세지
 **end*******************************************************/
void MMC2_SetBlockSpy(SET_BLOCK_SPY_R *rMMC)
{
    SET_BLOCK_SPY_S     send;
    
    if(C_AS_BLOCK != rMMC->block_state)
    {
        C_AS_BLOCK = rMMC->block_state;
        
        SPY_SetAndWriteAsBlockStatus(C_AS_BLOCK);
        
        if(g_nHA_State == HA_Active)
        {
            SEND_AsBlockStatusToEMS();
            SEND_AsBlockAlarm();
        }
    }
    
    bzero(&send, sizeof(send));
    
    MMC_set_header(&send.mmc_head, &rMMC->mmc_head, sizeof(send), false);
    
    send.block_state = C_AS_BLOCK;
    
    msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
    
    Log.printf(LOG_INF, "[MMC] %s() Current SPY Block=%s\n", __FUNCTION__, C_AS_BLOCK ? "BLOCK" : "UNBLOCK");
}

#pragma mark -
#pragma mark [MMC] BLACK/EXTERNAL_SERVER LIST ACTIVATE/UPDATE

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : MMC2_SetBlistSpy
 * CLASS-NAME     :
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : Black List를 사용할지 여부를 설정하는 MMC
 * REMARKS        : MMC 메세지
 **end*******************************************************/
void MMC2_SetBlistSpy(SET_BLIST_SPY_R *rMMC)
{
    SET_BLIST_SPY_S     send;

    if(rMMC->blist_state) { C_BLACK_LIST_USE = true;  }
    else                  { C_BLACK_LIST_USE = false; }
    
    bzero(&send, sizeof(send));

    MMC_set_header(&send.mmc_head, &rMMC->mmc_head, sizeof(send), false);

    send.blist_state   = C_BLACK_LIST_USE;    // V4/V6 공통
    
    msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
    
    Log.printf(LOG_INF, "[MMC] %s() Current BLACK_LIST_USE=%s\n", __FUNCTION__, (C_BLACK_LIST_USE==true) ? "true" : "false");
}


/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : MMC2_UpdBlistSpy
 * CLASS-NAME     :
 * PARAMETER    IN: rMMC  - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : Black List를 업데이트 하는 MMC
 * REMARKS        : MMC 메세지
 **end*******************************************************/
void MMC2_UpdBlistSpy(UPD_BLIST_SPY_R *rMMC)
{
    UPD_BLIST_SPY_S   send;
    int     result;
    
    bzero(&send, sizeof(send));
    MMC_set_header(&send.mmc_head, &rMMC->mmc_head, sizeof(send), false);
    
    result = g_BlackList.read_again();
    
    switch(result)
    {
        case BLIST_OK:
            send.n_blist = g_BlackList.size();
            
            if(send.n_blist == 0)
            {
                Log.printf(LOG_ERR, "Black List Update Success but No IP List\n");
                send.result = MMC2_RESULT_FAIL;
                send.reason = BLIST_IP_NOT_EXIST;
            }
            else
            {
                send.result = MMC2_RESULT_OK;
            }
            break;
            
        case BLIST_FILEOPEN_ERR:
            send.result = MMC2_RESULT_FAIL;
            send.reason = BLIST_FILEOPEN_ERR;
            break;
            
        case BLIST_FILENAME_ERR:
            send.result = MMC2_RESULT_FAIL;
            send.reason = BLIST_FILENAME_ERR;
            break;
            
        default:
            send.result = MMC2_RESULT_FAIL;
            send.reason = BLIST_UNDEFINED_ERR;
            break;
    }
    
    msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);

    Log.printf(LOG_INF, "[MMC] %s() Result=%s(%d)\n", __FUNCTION__, (send.result==MMC2_RESULT_OK) ? "OK" : "FAIL", send.reason);
}

#ifdef IPV6_MODE
/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : MMC2_UpdBlistSpy_V6
 * CLASS-NAME     :
 * PARAMETER    IN: rMMC  - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : IPv6 Black List를 업데이트 하는 MMC
 * REMARKS        : MMC 메세지
 **end*******************************************************/
void MMC2_UpdBlistSpy_V6(UPD_BLIST_SPY_R *rMMC)
{
    UPD_BLIST_SPY_S   send;
    int     result;
    
    bzero(&send, sizeof(send));
    MMC_set_header(&send.mmc_head, &rMMC->mmc_head, sizeof(send), false);
    
    result = g_BlackList_V6.read_again();
    
    switch(result)
    {
        case BLIST_OK:
            send.n_blist = g_BlackList_V6.size();
            
            if(send.n_blist == 0)
            {
                Log.printf(LOG_ERR, "IPv6 Black List Update Success but No IP List\n");
                send.result = MMC2_RESULT_FAIL;
                send.reason = BLIST_IP_NOT_EXIST;
            }
            else
            {
                send.result = MMC2_RESULT_OK;
            }
            break;
            
        case BLIST_FILEOPEN_ERR:
            send.result = MMC2_RESULT_FAIL;
            send.reason = BLIST_FILEOPEN_ERR;
            break;
            
        case BLIST_FILENAME_ERR:
            send.result = MMC2_RESULT_FAIL;
            send.reason = BLIST_FILENAME_ERR;
            break;
            
        default:
            send.result = MMC2_RESULT_FAIL;
            send.reason = BLIST_UNDEFINED_ERR;
            break;
    }
    
    msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
    
    Log.printf(LOG_INF, "[MMC] %s()-IPv6 Result=%s(%d)\n", __FUNCTION__, (send.result==MMC2_RESULT_OK) ? "OK" : "FAIL", send.reason);
}
#endif

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : MMC2_UpdExtlistSpy
 * CLASS-NAME     :
 * PARAMETER    IN: rMMC  - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : Ext List를 업데이트 하는 MMC
 * REMARKS        : MMC 메세지
 **end*******************************************************/
void MMC2_UpdExtlistSpy(UPD_EXTLIST_SPY_R *rMMC)
{
    UPD_EXTLIST_SPY_S   send;
    bool    result;
    
    bzero(&send, sizeof(send));
    MMC_set_header(&send.mmc_head, &rMMC->mmc_head, sizeof(send), false);
    
    result = g_ExtList.reread();

    if(result) { send.result = MMC2_RESULT_OK; }        // MMC result와 bool 의 실제 값은 반대
    else       { send.result = MMC2_RESULT_FAIL; }
    
    send.n_extlist = g_ExtList.size();
    
    msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);

    Log.printf(LOG_INF, "[MMC] %s()-IPv4 Result=%s Current ExtList size=%d\n", __FUNCTION__, (result) ? "OK" : "FAIL", send.n_extlist);
}

#ifdef IPV6_MODE
/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : MMC2_UpdExtlistSpy_V6
 * CLASS-NAME     :
 * PARAMETER    IN: rMMC  - 수신한 MMC Packet
 *              IN: bIsV6 - V6에 대한 요청이면 true
 * RET. VALUE     : -
 * DESCRIPTION    : Ext List를 업데이트 하는 MMC
 * REMARKS        : MMC 메세지
 **end*******************************************************/
void MMC2_UpdExtlistSpy_V6(UPD_EXTLIST_SPY_R *rMMC)
{
    UPD_EXTLIST_SPY_S   send;
    bool    result;
    
    bzero(&send, sizeof(send));
    MMC_set_header(&send.mmc_head, &rMMC->mmc_head, sizeof(send), false);
    
    result = g_ExtList_V6.reread();

    if(result) { send.result = MMC2_RESULT_OK;   }      // MMC result와 bool 의 실제 값은 반대
    else       { send.result = MMC2_RESULT_FAIL; }
    
    send.n_extlist = g_ExtList_V6.size();
    
    msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
    
    Log.printf(LOG_INF, "[MMC] %s()-IPv6 Result=%s Current ExtList size=%d\n", __FUNCTION__, (result) ? "OK" : "FAIL", send.n_extlist);
}
#endif


#pragma mark -
#pragma mark [MMC] OVERLOAD RRESONSE CDODE

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MMC2_DisOverResp
 * CLASS-NAME     : -
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : Overload 시 호를 제한할 때 사용하는 Response code (0이면 무응답)를 조회하는 MMC
 * REMARKS        : MMC 메세지
 **end*******************************************************/
void MMC2_DisOverResp(DIS_OVER_RESP_R *rMMC)
{
    DIS_OVER_RESP_S     send;
    
    bzero(&send, sizeof(send));
    MMC_set_header(&send.mmc_head, &rMMC->mmc_head, sizeof(send), false);
    
    send.resp_code = C_OVERLOAD_RESP_CODE;
    
    msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
    
    Log.printf(LOG_INF, "[MMC] %s() Current OVERLOAD_RESP_CODE=%d\n", __FUNCTION__, C_OVERLOAD_RESP_CODE);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MMC2_SetOverResp
 * CLASS-NAME     : -
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : Overload 시 전송할 Response code를 설정하는 MMC
 * REMARKS        : MMC 메세지
 **end*******************************************************/
void MMC2_SetOverResp(SET_OVER_RESP_R *rMMC)
{
    SET_OVER_RESP_S     send;
    uint16_t    old_code;
    
    bzero(&send, sizeof(send));
    MMC_set_header(&send.mmc_head, &rMMC->mmc_head, sizeof(send), false);
    
    if(rMMC->resp_code > 700)
    {
        send.result = MMC2_RESULT_FAIL;
        send.reason = MMC_PARAMETER_ERROR;

        Log.printf(LOG_ERR, "[MMC] %s() Fail request resp_code(%d) >= 700\n", __FUNCTION__, rMMC->resp_code);
    }
    else
    {
        old_code = C_OVERLOAD_RESP_CODE;
        C_OVERLOAD_RESP_CODE = rMMC->resp_code;     // set
        
        cfg.SetInt("ETC", "OVERLOAD_RESP_CODE", C_OVERLOAD_RESP_CODE);
        
        //    if(cfg.SaveFile()) { Log.printf(LOG_INF, "%s() cfg.SaveFile() SAVE OK !!\n",   __FUNCTION__); }
        //    else               { Log.printf(LOG_ERR, "%s() cfg.SaveFile() SAVE FAIL !!\n", __FUNCTION__); }
        
        send.result    = MMC2_RESULT_OK;
        send.resp_code = C_OVERLOAD_RESP_CODE;
        
        Log.printf(LOG_INF, "[MMC] %s() OVERLOAD_RESP_CODE change [%d->%d]\n", __FUNCTION__, old_code, C_OVERLOAD_RESP_CODE);
    }
    
    msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
}


#pragma mark -
#pragma mark [MMC] SCM CH MAP (add for mPBX BMT)

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MMC2_DisScmMap
 * CLASS-NAME     : -
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : SCM CH MAP 상태를 조회하는 MMC
 * REMARKS        : MMC 메세지
 **end*******************************************************/
void MMC2_DisScmMap(DIS_SCM_MAP_R *rMMC)
{
    DIS_SCM_MAP_S     send;
    
    bzero(&send, sizeof(send));
    MMC_set_header(&send.mmc_head, &rMMC->mmc_head, sizeof(send), false);
    
    send.result      = MMC2_RESULT_OK;
    send.use_scm_map = 1;                   // always use
    
    send.n_cr_map      = (int)g_ScmMap.m_map_pCR.size();
    send.n_call_id_map = (int)g_ScmMap.m_map_CallID.size();
    
    msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
    
    Log.printf(LOG_INF, "[MMC] %s() Current size=(%d, %d)\n", __FUNCTION__, send.n_cr_map, send.n_call_id_map);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MMC2_SetScmMap
 * CLASS-NAME     : -
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : SCM CH MAP 관련 C_USE_SCM_MAP 변수를 변경하는 MMC
 * REMARKS        : MMC 메세지 (20161027 C_USE_SCM_MAP 사용안하게 변경) - MMC는 아직 남겨 둠
 **end*******************************************************/
void MMC2_SetScmMap(SET_SCM_MAP_R *rMMC)
{
    SET_SCM_MAP_S     send;
//    bool            old;
    
//    old = C_USE_SCM_MAP;
//    C_USE_SCM_MAP = rMMC->use_scm_map;
    
    bzero(&send, sizeof(send));
    MMC_set_header(&send.mmc_head, &rMMC->mmc_head, sizeof(send), false);
    
    send.result      = MMC2_RESULT_OK;
    send.use_scm_map = 1;
    
    msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
    
    Log.printf(LOG_INF, "[MMC] MMC2_SetScmMap() C_USE_SCM_MAP 1->1 (unused MMC)\n");
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MMC2_ClrScmMap
 * CLASS-NAME     : -
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : SCM CH MAP 관련 MAP을 clear 하는 MMC
 * REMARKS        : MMC 메세지
 **end*******************************************************/
void MMC2_ClrScmMap(CLR_SCM_MAP_R *rMMC)
{
    CLR_SCM_MAP_S   send;
    
    bzero(&send, sizeof(send));
    MMC_set_header(&send.mmc_head, &rMMC->mmc_head, sizeof(send), false);
    
    send.n_before = (int)(g_ScmMap.m_map_CallID.size() + g_ScmMap.m_map_pCR.size());
    g_ScmMap.clear();
    send.n_after = (int)(g_ScmMap.m_map_CallID.size() + g_ScmMap.m_map_pCR.size());

    send.result      = MMC2_RESULT_OK;
    
    msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
    
    Log.printf(LOG_INF, "[MMC] %s() MAP size %d->%d\n", __FUNCTION__, send.n_before, send.n_after);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MMC2_DisScmAudit
 * CLASS-NAME     : -
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : C_SCM_MAP_AUDIT_TIME 변수 값을 조회하는 MMC
 * REMARKS        : MMC 메세지
 **end*******************************************************/
void MMC2_DisScmAudit(DIS_SCM_AUDIT_R *rMMC)
{
    DIS_SCM_AUDIT_S     send;
    
    bzero(&send, sizeof(send));
    MMC_set_header(&send.mmc_head, &rMMC->mmc_head, sizeof(send), false);
    
    send.result           = MMC2_RESULT_OK;
    send.n_scm_audit_time = C_SCM_MAP_AUDIT_TIME;
    
    msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
    
    Log.printf(LOG_INF, "[MMC] %s() Current C_SCM_MAP_AUDIT_TIME=%d, \n", __FUNCTION__, C_SCM_MAP_AUDIT_TIME);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MMC2_SetScmAudit
 * CLASS-NAME     : -
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : C_SCM_MAP_AUDIT_TIME 변수 값을 변경하는 MMC
 * REMARKS        : MMC 메세지
 **end*******************************************************/
void MMC2_SetScmAudit(SET_SCM_AUDIT_R *rMMC)
{
    SET_SCM_AUDIT_S     send;
    int                 old;
    
    old = C_SCM_MAP_AUDIT_TIME;
    C_SCM_MAP_AUDIT_TIME = rMMC->n_scm_audit_time;
    
    bzero(&send, sizeof(send));
    MMC_set_header(&send.mmc_head, &rMMC->mmc_head, sizeof(send), false);
    
    send.result           = MMC2_RESULT_OK;
    send.n_scm_audit_time = C_SCM_MAP_AUDIT_TIME;
    
    msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
    
    Log.printf(LOG_INF, "[MMC] %s() C_SCM_MAP_AUDIT_TIME %d->%d\n", __FUNCTION__, old, C_SCM_MAP_AUDIT_TIME);
}

#pragma mark -
#pragma mark [MMC] STAT. (statistic)

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MMC_DisStatSpy
 * CLASS-NAME     :
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : Sending MMC Statistics-Message
 * REMARKS        : MMC 메세지
 **end*******************************************************/
void MMC2_DisStatSpy(DIS_STAT_SPY_R *rMMC)
{
    DIS_STAT_SPY_S	send;
    
    bzero(&send, sizeof(send));
    
    MMC_set_header(&send.mmc_head, &rMMC->mmc_head, sizeof(send), false);
    
    // FIXIT: 항목이 많이 빠져있는데 MMC를 변경하고 전체 Method를 다 프린트하게 변경하는것을 검토....
    send.INVITE_Counts      = statistic.ssw.n_request[METHOD_INVITE];
    send.INVITE_OutCounts   = statistic.sam.n_request[METHOD_INVITE];
    
    send.R100_Counts        = statistic.ssw.n_response_code[100];
    send.R100_OutCounts     = statistic.ssw.n_response_code_out[100] + statistic.sam.n_response_code[100];
    
    send.R180_Counts        = statistic.ssw.n_response_code[180];
    send.R180_OutCounts     = statistic.ssw.n_response_code_out[180] + statistic.sam.n_response_code[180];
    
    send.R183_Counts        = statistic.ssw.n_response_code[183];
    send.R183_OutCounts     = statistic.ssw.n_response_code_out[183] + statistic.sam.n_response_code[183];
    
    send.R200_Counts        = statistic.ssw.n_response_code[200];
    send.R200_OutCounts     = statistic.ssw.n_response_code_out[200] + statistic.sam.n_response_code[200];
    
    send.ACK_Counts         = statistic.ssw.n_request[METHOD_ACK];
    send.ACK_OutCounts      = statistic.sam.n_request[METHOD_ACK];
    
    send.BYE_Counts         = statistic.ssw.n_request[METHOD_BYE];
    send.BYE_OutCounts      = statistic.sam.n_request[METHOD_BYE];
    
    send.CANCEL_Counts      = statistic.ssw.n_request[METHOD_CANCEL];
    send.CANCEL_OutCounts   = statistic.sam.n_request[METHOD_CANCEL];
    
    send.INFO_Counts        = statistic.ssw.n_request[METHOD_INFO];
    send.INFO_OutCounts     = statistic.sam.n_request[METHOD_INFO];
    
    send.PRACK_Counts       = statistic.ssw.n_request[METHOD_PRACK];
    send.PRACK_OutCounts    = statistic.sam.n_request[METHOD_PRACK];
    
    send.InMsgCounts_O      = statistic.ssw.n_in_sum[OT_TYPE_ORIG];
    send.InMsgCounts_T      = statistic.ssw.n_in_sum[OT_TYPE_TERM];
    
    send.OutMsgCounts_O     = statistic.sam.n_out_sum[OT_TYPE_ORIG] + statistic.ssw.n_response_code_out_sum[OT_TYPE_ORIG];
    send.OutMsgCounts_T     = statistic.sam.n_out_sum[OT_TYPE_ORIG] + statistic.ssw.n_response_code_out_sum[OT_TYPE_TERM];
    
    send.HashFailCounts     = statistic.sam.n_hash_fail;
    
    /* v6 추가 */
#ifdef IPV6_MODE
    send.INVITE_Counts      += statistic_v6.ssw.n_request[METHOD_INVITE];
    send.INVITE_OutCounts   += statistic_v6.sam.n_request[METHOD_INVITE];
    
    send.R100_Counts        += statistic_v6.ssw.n_response_code[100];
    send.R100_OutCounts     += (statistic_v6.ssw.n_response_code_out[100] + statistic_v6.sam.n_response_code[100]);
    
    send.R180_Counts        += statistic_v6.ssw.n_response_code[180];
    send.R180_OutCounts     += (statistic_v6.ssw.n_response_code_out[180] + statistic_v6.sam.n_response_code[180]);
    
    send.R183_Counts        += statistic_v6.ssw.n_response_code[183];
    send.R183_OutCounts     += (statistic_v6.ssw.n_response_code_out[183] + statistic_v6.sam.n_response_code[183]);
    
    send.R200_Counts        += statistic_v6.ssw.n_response_code[200];
    send.R200_OutCounts     += (statistic_v6.ssw.n_response_code_out[200] + statistic_v6.sam.n_response_code[200]);
    
    send.ACK_Counts         += statistic_v6.ssw.n_request[METHOD_ACK];
    send.ACK_OutCounts      += statistic_v6.sam.n_request[METHOD_ACK];
    
    send.BYE_Counts         += statistic_v6.ssw.n_request[METHOD_BYE];
    send.BYE_OutCounts      += statistic_v6.sam.n_request[METHOD_BYE];
    
    send.CANCEL_Counts      += statistic_v6.ssw.n_request[METHOD_CANCEL];
    send.CANCEL_OutCounts   += statistic_v6.sam.n_request[METHOD_CANCEL];
    
    send.INFO_Counts        += statistic_v6.ssw.n_request[METHOD_INFO];
    send.INFO_OutCounts     += statistic_v6.sam.n_request[METHOD_INFO];
    
    send.PRACK_Counts       += statistic_v6.ssw.n_request[METHOD_PRACK];
    send.PRACK_OutCounts    += statistic_v6.sam.n_request[METHOD_PRACK];

    
    send.InMsgCounts_O      += statistic_v6.ssw.n_in_sum[OT_TYPE_ORIG];
    send.InMsgCounts_T      += statistic_v6.ssw.n_in_sum[OT_TYPE_TERM];
    
    send.OutMsgCounts_O     += statistic_v6.sam.n_out_sum[OT_TYPE_ORIG] + statistic_v6.ssw.n_response_code_out_sum[OT_TYPE_ORIG];
    send.OutMsgCounts_T     += statistic_v6.sam.n_out_sum[OT_TYPE_ORIG] + statistic_v6.ssw.n_response_code_out_sum[OT_TYPE_TERM];
    
    send.HashFailCounts     += statistic_v6.sam.n_hash_fail;  // g_SAMServer->GetHashFailCount(false);
#endif
    
    // IPv4/IPv6 common
    send.TotalMsgCounts     = send.InMsgCounts_O  + send.InMsgCounts_T;
    send.TotalMsgOutCounts  = send.OutMsgCounts_O + send.OutMsgCounts_T;
    
    send.SessionCounts      = g_Hash_O.size() + g_Hash_T.size();
    send.SCMFailCounts      = statistic.spy.scm_fail_count;

    msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
    
    Log.printf(LOG_INF, "[MMC] %s() End\n", __FUNCTION__);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MMC_ClrStatSpy
 * CLASS-NAME     : -
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : 통계 정보를 초기화하는 함수
 * REMARKS        : MMC 메세지
 **end*******************************************************/
void MMC2_ClrStatSpy(CLR_STAT_SPY_R *rMMC)
{
    CLR_STAT_SPY_S send;
    
    bzero(&statistic,    sizeof(statistic));       // RESET STATISTIC DATA
#ifdef IPV6_MODE
    bzero(&statistic_v6, sizeof(statistic_v6));
#endif
    bzero(&send, sizeof(send));
    MMC_set_header(&send.mmc_head, &rMMC->mmc_head, sizeof(send), false);
    
    send.result = MMC2_RESULT_OK;
    
    msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
    
    Log.printf(LOG_INF, "[MMC] %s() OK \n", __FUNCTION__);
}


#pragma mark -
#pragma mark [MMC] TRACE

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : MMC2_DisTraceSpy
 * CLASS-NAME     :
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : SPY에 등록된 Trace List를 조회하는 함수
 * REMARKS        : MMC 메세지
 **end*******************************************************/
void MMC2_DisTraceSpy(DIS_TRACE_SPY_R *rMMC)
{
    DIS_TRACE_SPY_S	send;
    int     i;
    char    dummy[64];
    
    bzero(&send, sizeof(send));
    MMC_set_header(&send.mmc_head, &rMMC->mmc_head, sizeof(send), false);
    
    for(i = 0; i < MAX_TRACE_LIST; i++)
    {
        send.item[i].nIndex	= i;
        g_Trace.get_trace(i, send.item[i].strFrom, send.item[i].strTo, dummy, &(send.item[i].nMax), &(send.item[i].nCount));
    }
    
    send.n_item = g_Trace.size();
    
    msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
    
    Log.printf(LOG_INF, "[MMC] %s() Trace Count=%d Size=%d\n", __FUNCTION__, i, g_Trace.size());

}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MMC2_AddTraceSpy
 * CLASS-NAME     :
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : SIP Trace Start 요청 메세지를 처리하는 함수
 * REMARKS        : MMC 메세지
 **end*******************************************************/
void MMC2_AddTraceSpy(ADD_TRACE_SPY_R *rMMC)
{
    ADD_TRACE_SPY_S send;
    TRACE_LIST      trace;
    int         nIndex;
//    bool        result;
    
    bzero(&trace, sizeof(trace));
    
    if (rMMC->strFromAddr[0] == '\0') { strcpy(trace.strFrom, "-"); }
    else                              { strncpy(trace.strFrom, rMMC->strFromAddr, sizeof(trace.strFrom)-1); }
    
    if (rMMC->strToAddr[0] == '\0') { strcpy(trace.strTo, "-"); }
    else                            { strncpy(trace.strTo, rMMC->strToAddr, sizeof(trace.strTo)-1); }
    
    strcpy(trace.strSvcKey, "-");       // mPBX unuse ServiveKey field
    trace.nMaxCount = rMMC->nRepeatCount;
    
    bzero(&send, sizeof(send));
    MMC_set_header(&send.mmc_head, &rMMC->mmc_head, sizeof(send), false);

    
    if(g_Trace.insert(&trace, &nIndex) == true)
    {
        send.result = MMC2_RESULT_OK;
        send.nIndex = nIndex;
    }
    else
    {
        send.result = MMC2_RESULT_FAIL;
        send.reason = nIndex;               // insert() false시 index에 error reason이 있음
    }
    
    msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
    
    Log.printf(LOG_INF, "[MMC] MMC2_AddTraceSpy() result=%s (Index=%d)... List size=%d, From=%s, To=%s\n", (send.result==MMC2_RESULT_OK) ? "OK": "NOK", nIndex, g_Trace.size(), trace.strFrom, trace.strTo);
}



/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : MMC2_DelTraceSpy
 * CLASS-NAME     :
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : Trae 중지 요청 MMC
 * REMARKS        : MMC 메세지
 **end*******************************************************/
void MMC2_DelTraceSpy(DEL_TRACE_SPY_R *rMMC)
{
    DEL_TRACE_SPY_S   send;
    
    bzero(&send, sizeof(send));
    MMC_set_header(&send.mmc_head, &rMMC->mmc_head, sizeof(send), false);

    if(g_Trace.erase(rMMC->nIndex) == true)
    {
        send.result = MMC2_RESULT_OK;
    }
    else
    {
        send.result = MMC2_RESULT_FAIL;
        send.reason = MMC_PARAMETER_ERROR;               // insert() false시 index에 error reason이 있음
    }
    
    if(send.result == false) { send.reason = MMC_PARAMETER_ERROR; }
    
    send.trace_size = g_Trace.size();
    
    msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
    
    Log.printf(LOG_INF, "[MMC] MMC2_DelTraceSpy() index=%d result=%s, current trace size=%d\n", rMMC->nIndex, (send.result==MMC2_RESULT_OK) ? "OK" : "FAIL", send.trace_size);
}


#pragma mark -
#pragma mark [MMC] ETC

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MMC2_SaveCfgSpy
 * CLASS-NAME     : -
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : Config 변수를 Config File로  Write하는 함수
 * REMARKS        : MMC 메세지
 **end*******************************************************/
void MMC2_SaveCfgSpy(SAVE_CFG_SPY_R *rMMC)
{
    SAVE_CFG_SPY_S send;
    
    bzero(&send, sizeof(send));
    set_mmc_head(&send.mmc_head, &rMMC->mmc_head, sizeof(send), false);
    
    if(cfg.SaveFile() == true) { send.result = MMC2_RESULT_OK; }
    else                       { send.result = MMC2_RESULT_FAIL; }

    msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
    
    Log.printf(LOG_INF, "[MMC] %s() cfg.SaveFile() Result=%s \n", __FUNCTION__, (send.result==MMC2_RESULT_OK) ? "OK" : "FAIL");
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MMC_ClrCallSpy
 * CLASS-NAME     : -
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : CALL 정보를 초기화하는 함수
 * REMARKS        : MMC 메세지
 **end*******************************************************/
void MMC2_ClrCallSpy(CLR_CALL_SPY_R *rMMC)
{
    CLR_CALL_SPY_S send;
    
    g_Hash_O.clear();
    g_Hash_T.clear();
    
    bzero(&send, sizeof(send));
    set_mmc_head(&send.mmc_head, &rMMC->mmc_head, sizeof(send), false);
    
    send.result      = MMC2_RESULT_OK;
    send.o_hash_size = g_Hash_O.size();
    send.t_hash_size = g_Hash_T.size();
    
    msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
    
    Log.printf(LOG_INF, "[MMC] %s() O_HASH-%d, T_HASH=%d OK\n", __FUNCTION__, send.o_hash_size, send.t_hash_size);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MMC_DisCallSpy
 * CLASS-NAME     : -
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : Hash에 저장된 Call 수를 조회하는 함수
 * REMARKS        : MMC 메세지
 **end*******************************************************/
void MMC2_DisCallSpy(DIS_CALL_SPY_R *rMMC)
{
    DIS_CALL_SPY_S send;
    
    bzero(&send, sizeof(send));
    set_mmc_head(&send.mmc_head, &rMMC->mmc_head, sizeof(send), false);
    
    send.result      = MMC2_RESULT_OK;
    send.o_hash_size = g_Hash_O.size();
    send.t_hash_size = g_Hash_T.size();
    
    msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
    
    Log.printf(LOG_INF, "[MMC] %s() O_HASH-%d, T_HASH=%dOK \n", __FUNCTION__, send.o_hash_size, send.t_hash_size);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MMC2_DisReportSPY
 * CLASS-NAME     : -
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : Log에 출력되는 Report 주기를 조회 하는 함수
 * REMARKS        : MMC 메세지
 **end*******************************************************/
void MMC2_DisReportSPY(DIS_REPORT_SPY_R *rMMC)
{
    DIS_REPORT_SPY_S send;
    
    bzero(&send, sizeof(send));
    set_mmc_head(&send.mmc_head, &rMMC->mmc_head, sizeof(send), false);
    
    send.result                = MMC2_RESULT_OK;
    send.report_time_in_config = C_REPORT_TIME;
    send.report_time           =g_ReportTime * TIME_CHECK_SSW_EXT_STATUS_CHECK;
    
    msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
    
    Log.printf(LOG_INF, "[MMC] %s() SPY REPORT TIME IN CONFIG=%d, SPY REPORT TIME=%d \n", __FUNCTION__, send.report_time_in_config, send.report_time);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MMC2_ChgReportSPY
 * CLASS-NAME     : -
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : Log에 출력되는 Report 주기를 변경하는 함수
 * REMARKS        : MMC 메세지
 **end*******************************************************/
void MMC2_ChgReportSPY(CHG_REPORT_SPY_R *rMMC)
{
    CHG_REPORT_SPY_S send;
    
    C_REPORT_TIME = rMMC->report_time;
    g_ReportTime = (int)(C_REPORT_TIME / TIME_CHECK_SSW_EXT_STATUS_CHECK);
    
    bzero(&send, sizeof(send));
    set_mmc_head(&send.mmc_head, &rMMC->mmc_head, sizeof(send), false);
    
    send.result                = MMC2_RESULT_OK;
    send.report_time_in_config = C_REPORT_TIME;
    send.report_time           =g_ReportTime * TIME_CHECK_SSW_EXT_STATUS_CHECK;
    
    msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
    
    Log.printf(LOG_INF, "[MMC] %s() SPY REPORT TIME IN CONFIG=%d, SPY REPORT TIME=%d \n", __FUNCTION__, send.report_time_in_config, send.report_time);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MMC_DisCallSpy
 * CLASS-NAME     : -
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : SPY에서 사용하는 각종 데이터를 초기화 하는 함수
 * REMARKS        : MMC 메세지
 **end*******************************************************/
void MMC2_InitAllSpy(INIT_ALL_SPY_R *rMMC)
{
    INIT_ALL_SPY_S send;
    
    g_ScmMap.clear();
    g_Hash_O.clear();
    g_Hash_T.clear();
    
    bzero(&send, sizeof(send));
    set_mmc_head(&send.mmc_head, &rMMC->mmc_head, sizeof(send), false);
    
    send.result                = MMC2_RESULT_OK;
    
    msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
    
    Log.printf(LOG_INF, "[MMC] %s() SPY INIT SCM_MAP, HASH_O, HASH_T\n", __FUNCTION__);
}

#pragma mark -
#pragma mark Processing Received XBUS Message from MGM

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MBUS_MsgFromMGM2
 * CLASS-NAME     :
 * PARAMETER    IN: len     - 수신한 메세지 길이
 *              IN: msg_ptr - 수신한 메세지 포인터
 * RET. VALUE     : -
 * DESCRIPTION    : MGM(MMC)에서 수신받은 XBUS 메세지를 처리하는 함수
 * REMARKS        : MMC 메세지
 **end*******************************************************/
void MBUS_MsgFromMGM2(int len, XBUS_MSG *rXBUS)
{
    MMCPDU *rMMC  = (MMCPDU *)rXBUS->Data;
    
    // 이전 MMC는 Side 비교를 안하는 것도 있어서  SIDE 비교전에 예전 MMC를 호출할 것
    // OLD MMC (이전 버전 호환을 위해 삭제 안함) - 가급적 이전 MMC는 사용하지 않는것이 좋겠지만 이전 TAS와 대용량 TAS를 혼용할 경우를 위해 남겨둠
    if(rMMC->Head.CmdNo < MMC_DIS_CFG_SPY) { MBUS_MsgFromMGM(len, rXBUS); return; }
    
    // 이하 새로 변경된 MMC
    if((rMMC->Body[0] != MY_SIDE) && (rMMC->Body[0] != 2))
    {
#ifdef DEBUG_MODE
        Log.printf(LOG_LV1, "[MMC] %s() Cmd=[%d] Received, but not my side(req. side=%d)\n", __FUNCTION__, rMMC->Head.CmdNo, rMMC->Body[0] );
#endif
        return;
    }
    
#ifdef DEBUG_MODE
    Log.printf(LOG_LV1, "[MMC] MMC Cmd=[%d] Received\n", rMMC->Head.CmdNo);
#endif

    switch(rMMC->Head.CmdNo)
    {
        case MMC_DIS_CFG_SPY:     MMC2_DisCfgSpy((DIS_CFG_SPY_R *)rMMC);    return;
        case MMC_SET_LOG_SPY:     MMC2_SetLogSpy((SET_LOG_SPY_R *)rMMC);    return;
        case MMC_SET_QOS_SPY:     MMC2_SetQosSpy((SET_QOS_SPY_R *)rMMC);    return;
            
        case MMC_DIS_BLOCK_SPY:   MMC2_DisBlockSpy((DIS_BLOCK_SPY_R *)rMMC);  return;
        case MMC_SET_BLOCK_SPY:   MMC2_SetBlockSpy((SET_BLOCK_SPY_R *)rMMC);  return;

        case MMC_SET_BLLIST_SPY:    MMC2_SetBlistSpy     ((SET_BLIST_SPY_R   *)rMMC); return;
        case MMC_UPD_BLIST_SPY:     MMC2_UpdBlistSpy     ((UPD_BLIST_SPY_R   *)rMMC); return;
#ifdef IPV6_MODE
        case MMC_UPD_BLIST_SPY_6:   MMC2_UpdBlistSpy_V6  ((UPD_BLIST_SPY_R   *)rMMC); return;
#endif
        case MMC_UPD_EXTLIST_SPY:   MMC2_UpdExtlistSpy   ((UPD_EXTLIST_SPY_R *)rMMC); return;
#ifdef IPV6_MODE
        case MMC_UPD_EXTLIST_SPY_6: MMC2_UpdExtlistSpy_V6((UPD_EXTLIST_SPY_R *)rMMC); return;
#endif   
        case MMC_DIS_OVER_RESP:     MMC2_DisOverResp((DIS_OVER_RESP_R *)rMMC); return;
        case MMC_SET_OVER_RESP:     MMC2_SetOverResp((SET_OVER_RESP_R *)rMMC); return;
            
        case MMC_DIS_SCM_MAP:       MMC2_DisScmMap((DIS_SCM_MAP_R *)rMMC); return;          // add 20160615
        case MMC_SET_SCM_MAP:       MMC2_SetScmMap((SET_SCM_MAP_R *)rMMC); return;          // add 20160615
        case MMC_CLR_SCM_MAP:       MMC2_ClrScmMap((CLR_SCM_MAP_R *)rMMC); return;          // add 20160615
        case MMC_DIS_SCM_AUDIT:     MMC2_DisScmAudit((DIS_SCM_AUDIT_R *)rMMC); return;      // add 20160619
        case MMC_SET_SCM_AUDIT:     MMC2_SetScmAudit((SET_SCM_AUDIT_R *)rMMC); return;      // add 20160619

        case MMC_SAVE_CFG_SPY:      MMC2_SaveCfgSpy((SAVE_CFG_SPY_R *)rMMC);  return;

        case MMC_DIS_STAT_SPY:      MMC2_DisStatSpy((DIS_STAT_SPY_R *)rMMC);  return;
        case MMC_CLR_STAT_SPY:      MMC2_ClrStatSpy((CLR_STAT_SPY_R *)rMMC);  return;
            
        case MMC_ADD_TRACE_SPY:     MMC2_AddTraceSpy((ADD_TRACE_SPY_R *)rMMC);  return;
        case MMC_DEL_TRACE_SPY:     MMC2_DelTraceSpy((DEL_TRACE_SPY_R *)rMMC);  return;
        case MMC_DIS_TRACE_SPY:     MMC2_DisTraceSpy((DIS_TRACE_SPY_R *)rMMC);  return;
            
        case MMC_CLR_CALL_SPY:      MMC2_ClrCallSpy((CLR_CALL_SPY_R *)rMMC);  return;
        case MMC_DIS_CALL_SPY:      MMC2_DisCallSpy((DIS_CALL_SPY_R *)rMMC);  return;
            
        case MMC_DIS_REPORT_SPY:    MMC2_DisReportSPY((DIS_REPORT_SPY_R *)rMMC);  return;
        case MMC_CHG_REPORT_SPY:    MMC2_ChgReportSPY((CHG_REPORT_SPY_R *)rMMC);  return;
            
        case MMC_INIT_ALL_SPY:      MMC2_InitAllSpy((INIT_ALL_SPY_R *)rMMC);  return;
            
       default:
            Log.printf(LOG_ERR, "[MMC] %s() Undefiend MMC[%d]\n", __FUNCTION__, rMMC->Head.CmdNo);
            return;
    }
}
