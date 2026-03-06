/* File Header
 *fsh**************************************************************
 ******************************************************************
 **
 **  FILE : SPY_stat.cpp
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
 DESCRIPTION    : 통계 관련 functions
 REMARKS        :
 *end*************************************************************/

#include "SPY_def.h"
#include "SPY_xref.h"


#pragma mark -
#pragma mark [MMC] DIS/SET Statistic (통계)

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : GetPsInfo_VmRSS
 * CLASS-NAME     :
 * PARAMETER      : -
 * RET. VALUE     : N(int)
 * DESCRIPTION    : Process의 VmRSS Size를 구하는 함수
 * REMARKS        :
 **end*******************************************************/
int GetPsInfo_VmRSS(void)
{
	char    filename[128];
    FILE    *fp;
    char    buf[128];
    int     size = 0;
    
	snprintf(filename, sizeof(filename), "/proc/%d/status", getpid());     // SPY staus filename
    
    fp = fopen("/proc/10056/status", "r");
    if(fp != NULL)
    {
        while(fgets(buf, 127, fp))
        {
            if(strncmp(buf, "VmRSS:", strlen("VmRSS:")) == 0)
            {
                sscanf(buf, "VmRSS: %d kB", &size);
                break;
            }
        }
        fclose(fp);
        return(size);
    }
    return(0);
}

#ifdef IPV6_MODE
/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MMC_DisStatSpy_V6
 * CLASS-NAME     :
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : Sending MMC Statistics-Message
 * REMARKS        : MMC 메세지
 **end*******************************************************/
void MMC_DisStatSpy_V6(MMCPDU *rMMC)
{
    DIS_STAT_SPY	send;
    
    if(g_nHA_State != HA_Active) { return;}
    
    Log.printf(LOG_INF, "[MMC] MMC_DisStatSpy_V6() Command Request\n");
    
    bzero(&send, sizeof(send));
    
    set_mmc_head(&send.mmc_head, &rMMC->Head, sizeof(send), false);
    
    // FIXIT: 항목이 많이 빠져있는데 MMC를 변경하고 전체 Method를 다 프린트하게 변경하는것을 검토....
    send.INVITE_Counts      = statistic.ssw.n_request[METHOD_INVITE];
    send.INVITE_OutCounts   = statistic.sam.n_request[METHOD_INVITE];
    
    send.ACK_Counts         = statistic.ssw.n_request[METHOD_ACK];
    send.ACK_OutCounts      = statistic.sam.n_request[METHOD_ACK];
    
    send.BYE_Counts         = statistic.ssw.n_request[METHOD_BYE];
    send.BYE_OutCounts      = statistic.sam.n_request[METHOD_BYE];
    
    send.CANCEL_Counts      = statistic.ssw.n_request[METHOD_CANCEL];
    send.CANCEL_OutCounts   = statistic.sam.n_request[METHOD_CANCEL];
    
    send.INFO_Counts        = statistic.ssw.n_request[METHOD_INFO];
    send.INFO_OutCounts     = statistic.sam.n_request[METHOD_INFO];
    
    send.MESSAGE_Counts     = statistic.ssw.n_request[METHOD_MESSAGE];
    send.MESSAGE_OutCounts  = statistic.sam.n_request[METHOD_MESSAGE];
    
    send.REGISTER_Counts    = statistic.ssw.n_request[METHOD_REGISTER];
    send.REGISTER_OutCounts = statistic.sam.n_request[METHOD_REGISTER];
    
    send.OPTIONS_Counts     = statistic.ssw.n_request[METHOD_OPTIONS];
    send.OPTIONS_OutCounts  = statistic.sam.n_request[METHOD_OPTIONS];
    
    send.R100_Counts        = statistic.ssw.n_response_code[100];
    send.R100_OutCounts     = statistic.ssw.n_response_code_out[100] + statistic.sam.n_response_code[100];
    
    send.R180_Counts        = statistic.ssw.n_response_code[180];
    send.R180_OutCounts     = statistic.ssw.n_response_code_out[180] + statistic.sam.n_response_code[180];
    
    send.R200_Counts        = statistic.ssw.n_response_code[200];
    send.R200_OutCounts     = statistic.ssw.n_response_code_out[200] + statistic.sam.n_response_code[200];
    
    
    send.InMsgCounts_O      = statistic.ssw.n_in_sum[OT_TYPE_ORIG];
    send.InMsgCounts_T      = statistic.ssw.n_in_sum[OT_TYPE_TERM];
    
    send.OutMsgCounts_O     = statistic.sam.n_out_sum[OT_TYPE_ORIG] + statistic.ssw.n_response_code_out_sum[OT_TYPE_ORIG];
    send.OutMsgCounts_T     = statistic.sam.n_out_sum[OT_TYPE_ORIG] + statistic.ssw.n_response_code_out_sum[OT_TYPE_TERM];
    
    send.HashFailCounts     = statistic.sam.n_hash_fail;  // g_SAMServer->GetHashFailCount(false);
    
    /* v6 추가 */
    send.INVITE_Counts      += statistic_v6.ssw.n_request[METHOD_INVITE];
    send.INVITE_OutCounts   += statistic_v6.sam.n_request[METHOD_INVITE];
    
    send.ACK_Counts         += statistic_v6.ssw.n_request[METHOD_ACK];
    send.ACK_OutCounts      += statistic_v6.sam.n_request[METHOD_ACK];
    
    send.BYE_Counts         += statistic_v6.ssw.n_request[METHOD_BYE];
    send.BYE_OutCounts      += statistic_v6.sam.n_request[METHOD_BYE];
    
    send.CANCEL_Counts      += statistic_v6.ssw.n_request[METHOD_CANCEL];
    send.CANCEL_OutCounts   += statistic_v6.sam.n_request[METHOD_CANCEL];
    
    send.INFO_Counts        += statistic_v6.ssw.n_request[METHOD_INFO];
    send.INFO_OutCounts     += statistic_v6.sam.n_request[METHOD_INFO];
    
    send.MESSAGE_Counts     += statistic_v6.ssw.n_request[METHOD_MESSAGE];
    send.MESSAGE_OutCounts  += statistic_v6.sam.n_request[METHOD_MESSAGE];
    
    send.REGISTER_Counts    += statistic_v6.ssw.n_request[METHOD_REGISTER];
    send.REGISTER_OutCounts += statistic_v6.sam.n_request[METHOD_REGISTER];
    
    send.OPTIONS_Counts     += statistic_v6.ssw.n_request[METHOD_OPTIONS];
    send.OPTIONS_OutCounts  += statistic_v6.sam.n_request[METHOD_OPTIONS];
    
    send.R100_Counts        += statistic_v6.ssw.n_response_code[100];
    send.R100_OutCounts     += (statistic_v6.ssw.n_response_code_out[100] + statistic_v6.sam.n_response_code[100]);
    
    send.R180_Counts        += statistic_v6.ssw.n_response_code[180];
    send.R180_OutCounts     += (statistic_v6.ssw.n_response_code_out[180] + statistic_v6.sam.n_response_code[180]);
    
    send.R200_Counts        += statistic_v6.ssw.n_response_code[200];
    send.R200_OutCounts     += (statistic_v6.ssw.n_response_code_out[200] + statistic_v6.sam.n_response_code[200]);
    
    
    send.InMsgCounts_O      += statistic_v6.ssw.n_in_sum[OT_TYPE_ORIG];
    send.InMsgCounts_T      += statistic_v6.ssw.n_in_sum[OT_TYPE_TERM];
    
    send.OutMsgCounts_O     += statistic_v6.sam.n_out_sum[OT_TYPE_ORIG] + statistic_v6.ssw.n_response_code_out_sum[OT_TYPE_ORIG];
    send.OutMsgCounts_T     += statistic_v6.sam.n_out_sum[OT_TYPE_ORIG] + statistic_v6.ssw.n_response_code_out_sum[OT_TYPE_TERM];

    send.HashFailCounts     += statistic_v6.sam.n_hash_fail;  // g_SAMServer->GetHashFailCount(false);
    
    // IPv4/IPv6 common
    send.TotalMsgCounts     = send.InMsgCounts_O  + send.InMsgCounts_T;
    send.TotalMsgOutCounts  = send.OutMsgCounts_O + send.OutMsgCounts_T;
    
    send.SessionCounts      = g_Hash_O.size() + g_Hash_T.size();
    send.SCMFailCounts      = statistic.spy.scm_fail_count;
    send.VmRSS              = GetPsInfo_VmRSS();
    //	Log.printf(LOG_INF, "Session = %d, VmRSS = %d\n", g_HashTable->GetSize(), GetVmSize());
    
    msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MMC_ClrStatSpy_V6
 * CLASS-NAME     : -
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : 통계 정보를 초기화하는 함수
 * REMARKS        : MMC 메세지
 **end*******************************************************/
void MMC_ClrStatSpy_V6(MMCPDU *rMMC)
{
    Log.printf(LOG_INF, "[MMC] MMC_ClrStatSpy_V6() Command Request\n");
    
    bzero(&statistic, sizeof(statistic));       // RESET STATISTIC DATA
    bzero(&statistic_v6, sizeof(statistic_v6));

    MMC_DisStatSpy_V6(rMMC);       // FIXIT:.... 보내봐야 0일껀데...
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MMC_ClrCallSpy_V6
 * CLASS-NAME     : -
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : CALL 정보를 초기화하는 함수
 * REMARKS        : MMC 메세지
 **end*******************************************************/
void MMC_ClrCallSpy_V6(MMCPDU *rMMC)
{
    Log.printf(LOG_INF, "[MMC] MMC_ClrCallSpy_V6() Command Request\n");
    
    g_Hash_O.clear();
    g_Hash_T.clear();
    
    MMC_DisStatSpy_V6(rMMC);
}
#endif  // IPV6_MODE

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MMC_DisStatSpy
 * CLASS-NAME     :
 * PARAMETER    IN: rMMC - 수신한 MMC Packet
 * RET. VALUE     : -
 * DESCRIPTION    : Sending MMC Statistics-Message
 * REMARKS        : MMC 메세지
 **end*******************************************************/
void MMC_DisStatSpy(MMCPDU *rMMC)
{
	DIS_STAT_SPY	send;
    
    if(g_nHA_State != HA_Active) { return;}
    
    Log.printf(LOG_INF, "[MMC] MMC_DisStatSpy() Command Request\n");
    
	bzero(&send, sizeof(send));
    
	set_mmc_head(&send.mmc_head, &rMMC->Head, sizeof(send), false);
    
    // FIXIT: 항목이 많이 빠져있는데 MMC를 변경하고 전체 Method를 다 프린트하게 변경하는것을 검토....
	send.INVITE_Counts      = statistic.ssw.n_request[METHOD_INVITE];
	send.INVITE_OutCounts   = statistic.sam.n_request[METHOD_INVITE];
    
    send.ACK_Counts         = statistic.ssw.n_request[METHOD_ACK];
	send.ACK_OutCounts      = statistic.sam.n_request[METHOD_ACK];
    
    send.BYE_Counts         = statistic.ssw.n_request[METHOD_BYE];
	send.BYE_OutCounts      = statistic.sam.n_request[METHOD_BYE];
    
    send.CANCEL_Counts      = statistic.ssw.n_request[METHOD_CANCEL];
	send.CANCEL_OutCounts   = statistic.sam.n_request[METHOD_CANCEL];
    
    send.INFO_Counts        = statistic.ssw.n_request[METHOD_INFO];
	send.INFO_OutCounts     = statistic.sam.n_request[METHOD_INFO];
    
    send.MESSAGE_Counts     = statistic.ssw.n_request[METHOD_MESSAGE];
	send.MESSAGE_OutCounts  = statistic.sam.n_request[METHOD_MESSAGE];
    
    send.REGISTER_Counts    = statistic.ssw.n_request[METHOD_REGISTER];
	send.REGISTER_OutCounts = statistic.sam.n_request[METHOD_REGISTER];
    
    send.OPTIONS_Counts     = statistic.ssw.n_request[METHOD_OPTIONS];
	send.OPTIONS_OutCounts  = statistic.sam.n_request[METHOD_OPTIONS];
    
	send.R100_Counts        = statistic.ssw.n_response_code[100];
	send.R100_OutCounts     = statistic.ssw.n_response_code_out[100] + statistic.sam.n_response_code[100];
    
    send.R180_Counts        = statistic.ssw.n_response_code[180];
	send.R180_OutCounts     = statistic.ssw.n_response_code_out[180] + statistic.sam.n_response_code[180];
    
    send.R200_Counts        = statistic.ssw.n_response_code[200];
	send.R200_OutCounts     = statistic.ssw.n_response_code_out[200] + statistic.sam.n_response_code[200];
    
    
	send.InMsgCounts_O      = statistic.ssw.n_in_sum[OT_TYPE_ORIG];
    send.InMsgCounts_T      = statistic.ssw.n_in_sum[OT_TYPE_TERM];
    
	send.OutMsgCounts_O     = statistic.sam.n_out_sum[OT_TYPE_ORIG] + statistic.ssw.n_response_code_out_sum[OT_TYPE_ORIG];
	send.OutMsgCounts_T     = statistic.sam.n_out_sum[OT_TYPE_ORIG] + statistic.ssw.n_response_code_out_sum[OT_TYPE_TERM];
    
	send.TotalMsgCounts     = send.InMsgCounts_O  + send.InMsgCounts_T;
	send.TotalMsgOutCounts  = send.OutMsgCounts_O + send.OutMsgCounts_T;
    
	send.SessionCounts      = g_Hash_O.size() + g_Hash_T.size();
	send.SCMFailCounts      = statistic.spy.scm_fail_count;
	send.HashFailCounts     = statistic.sam.n_hash_fail;  // g_SAMServer->GetHashFailCount(false);
	send.VmRSS              = GetPsInfo_VmRSS();
    //	Log.printf(LOG_INF, "Session = %d, VmRSS = %d\n", g_HashTable->GetSize(), GetVmSize());
    
	msendsig(sizeof(send), MGM, MSG_ID_MMC, (uint8_t *)&send);
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
void MMC_ClrStatSpy(MMCPDU *rMMC)
{
    Log.printf(LOG_INF, "[MMC] MMC_ClrStatSpy() Command Request\n");
    
    bzero(&statistic, sizeof(statistic));       // RESET STATISTIC DATA
    
    MMC_DisStatSpy(rMMC);       // FIXIT:.... 보내봐야 0일껀데...
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
void MMC_ClrCallSpy(MMCPDU *rMMC)
{
    Log.printf(LOG_INF, "[MMC] MMC_ClrCallSpy() Command Request\n");
    
    g_Hash_O.clear();
    g_Hash_T.clear();
    
    MMC_DisStatSpy(rMMC);
}

#pragma mark -
#pragma mark [SGM] 통계 데이터 전송 요청 처리 함수

#ifdef INCLUDE_REGI

#pragma mark -
#pragma mark regiCPS 처리

#include "SPY_scm.h"

pthread_mutex_t     m_regiMutex;

void InitRegiCPScnt(void)   {   pthread_mutex_init(&m_regiMutex, NULL); }
//void CloseRegiCPScnt(void)  {   pthread_mutex_destroy(&m_regiMutex);    }
//void LockRegiCPScnt(void)   {   pthread_mutex_lock(&m_regiMutex);   }
//void UnlockRegiCPScnt(void) {   pthread_mutex_unlock(&m_regiMutex);   }

int     g_nRegiCount = 0;


#define SRM_USAGE_INFO	0x4001

typedef struct {
    int srm_idx;    // SPY id : 9
    unsigned int ch_total;
    unsigned int ch_usage;
    unsigned int count;
    struct _svcInfo {
        char svckey[16];
        unsigned long int incomming;
        unsigned long int success;
    } svcItem[];
} SrmUsageInfo;

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : STAT_ADD_REGISTER_IN
 * CLASS-NAME     : -
 * PARAMETER    IN: pCR - Call Register 구조체 포인터
 * RET. VALUE     : -
 * DESCRIPTION    : REGISTER 수신 통계를 업데이트하는 함수
 * REMARKS        :
 **end*******************************************************/
void STAT_ADD_REGISTER_IN(CR_DATA *pCR)
{
    pthread_mutex_lock(&m_regiMutex);
    {
        if(pCR->bIsDeregi) { g_RegiSts.in_deregi ++; }
        else               { g_RegiSts.in_regi   ++; }
        
        // REGI CPS 정보 UPDATE
        g_nRegiCount++;
        if (g_nRegiCount >= 100000000) { g_nRegiCount = 0; }    // 20151021 bible - SRM에서 100000000까지 누적하고 있음
    }
    pthread_mutex_unlock(&m_regiMutex);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : STAT_ADD_REGISTER_FAIL
 * CLASS-NAME     : -
 * PARAMETER    IN: pCR - Call Register 구조체 포인터
 * RET. VALUE     : -
 * DESCRIPTION    : REGISTER 실패 통계를 업데이트하는 함수
 * REMARKS        :
 **end*******************************************************/
void STAT_ADD_REGISTER_FAIL(CR_DATA *pCR)
{
    pthread_mutex_lock(&m_regiMutex);
    {
        if(pCR->bIsDeregi) { g_RegiSts.in_deregi_fail ++; }
        else               { g_RegiSts.in_regi_fail   ++; }
    }
    pthread_mutex_unlock(&m_regiMutex);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : STAT_ADD_REGISTER_FAIL_REGI
 * CLASS-NAME     : -
 * PARAMETER    IN: pCR - Call Register 구조체 포인터
 * RET. VALUE     : -
 * DESCRIPTION    : REGISTER 실패 통계를 업데이트하는 함수
 * REMARKS        : REGI case
 **end*******************************************************/
void STAT_ADD_REGISTER_FAIL_REGI(CR_DATA *pCR)
{
    pthread_mutex_lock(&m_regiMutex);
    {
        g_RegiSts.in_regi_fail ++;
    }
    pthread_mutex_unlock(&m_regiMutex);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : STAT_ADD_REGISTER_FAIL_DEREGI
 * CLASS-NAME     : -
 * PARAMETER    IN: pCR - Call Register 구조체 포인터
 * RET. VALUE     : -
 * DESCRIPTION    : REGISTER 실패 통계를 업데이트하는 함수
 * REMARKS        : DEREGI case
 **end*******************************************************/
void STAT_ADD_REGISTER_FAIL_DEREGI(CR_DATA *pCR)
{
    pthread_mutex_lock(&m_regiMutex);
    {
        g_RegiSts.in_deregi_fail ++;
    }
    pthread_mutex_unlock(&m_regiMutex);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : STAT_ADD_REGISTER_SUCCESS
 * CLASS-NAME     : -
 * PARAMETER    IN: pCR - Call Register 구조체 포인터
 * RET. VALUE     : -
 * DESCRIPTION    : REGISTER 성공 통계를 업데이트하는 함수
 * REMARKS        :
 **end*******************************************************/
void STAT_ADD_REGISTER_SUCCESS(CR_DATA *pCR)
{
    pthread_mutex_lock(&m_regiMutex);
    {
        if(pCR->bIsDeregi) { g_RegiSts.in_deregi_succ ++; }
        else               { g_RegiSts.in_regi_succ   ++; }
    }
    pthread_mutex_unlock(&m_regiMutex);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : STAT_ADD_REGISTER_SUCCESS_REGI
 * CLASS-NAME     : -
 * PARAMETER    IN: pCR - Call Register 구조체 포인터
 * RET. VALUE     : -
 * DESCRIPTION    : REGISTER 실패 통계를 업데이트하는 함수
 * REMARKS        : REGI case
 **end*******************************************************/
void STAT_ADD_REGISTER_SUCCESS_REGI(CR_DATA *pCR)
{
    pthread_mutex_lock(&m_regiMutex);
    {
        g_RegiSts.in_regi_succ ++;
    }
    pthread_mutex_unlock(&m_regiMutex);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : STAT_ADD_REGISTER_SUCCESS_DEREGI
 * CLASS-NAME     : -
 * PARAMETER    IN: pCR - Call Register 구조체 포인터
 * RET. VALUE     : -
 * DESCRIPTION    : REGISTER 실패 통계를 업데이트하는 함수
 * REMARKS        : DEREGI case
 **end*******************************************************/
void STAT_ADD_REGISTER_SUCCESS_DEREGI(CR_DATA *pCR)
{
    pthread_mutex_lock(&m_regiMutex);
    {
        g_RegiSts.in_deregi_succ ++;
    }
    pthread_mutex_unlock(&m_regiMutex);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SPY_RegisterCPSThread
 * CLASS-NAME     : -
 * PARAMETER    IN: -
 * RET. VALUE     : -
 * DESCRIPTION    : 5초 주기로 REGISTER 인입 Count 전송 (단, mmtel.regi로 전송)
 * REMARKS        : add 20151020 by bible
 **end*******************************************************/
void *SPY_RegisterCPSThread(void *arg)
{
    // 20160309 RegCPS send flag
    int			nSendFlag = 0;
    SCM_HEAD 	*head;
    SrmUsageInfo *body;
    size_t 		length;
    char		msg[2048];
    
    while(true)
    {
        // 20151023 bible - Active SPY만 regi CPS처리하도록...
        if(g_nHA_State != HA_Active)
        {
            pthread_mutex_lock(&m_regiMutex);
            {
                g_nRegiCount = 0;
            }
            pthread_mutex_unlock(&m_regiMutex);
            sleep(1);
            continue;
        }
        
        if (time(NULL)%5 == 0)
        {
            // 20160309 RegCPS send flag
            if (nSendFlag)
            {
                usleep(100000);
                continue;
            }
            
//            LockRegiCPScnt();
            
            memset(msg, 0x00, sizeof(msg));
            head = (SCM_HEAD *)msg;
            
            head->id = SRM_USAGE_INFO;
            head->response = 0;
            head->reason = 0;
            
            body = (SrmUsageInfo *)(msg + sizeof(SCM_HEAD));
            body->srm_idx  = 9;
            body->ch_total = 0;
            body->ch_usage = 0;
            body->count    = 1;
            sprintf(body->svcItem[0].svckey, "mmtel.regi");
            
            pthread_mutex_lock(&m_regiMutex);
            {
                body->svcItem[0].incomming = g_nRegiCount;
                body->svcItem[0].success   = g_nRegiCount;
            }
            pthread_mutex_unlock(&m_regiMutex);
            
            length = sizeof(SCM_HEAD) + sizeof(SrmUsageInfo) + (sizeof(SrmUsageInfo::_svcInfo) * 1);    // FIXME: 왜 구조체를 이렇게 잡았나??
            
            // 20151021 bible - SRM에서 100000000까지 누적하고 있음, 초기화 주석 처리
            //g_nRegiCount = 0;
//            UnlockRegiCPScnt();
            
            msendsig(length, SCM, MSG_ID_SCM, (uint8_t *)msg);
            //            logprintf(DEBUG_INFO, "\033[0;40;32m @@@@@@@@@@@@@@ SPY_RegisterCPSThread() send Count:%d \n\033[0m\n", body->svcItem[0].incomming);
            // 20160309 RegCPS send flag
            nSendFlag = 1;
        }
        else
        {
            // 20160309 RegCPS send flag
            nSendFlag = 0;
        }
        
        // 20160309 RegCPS send flag
        //sleep(1);
        usleep(100000);
    }
}
// 20151020 END add

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : SEND_RegiStatInfoToSGM
 * CLASS-NAME     :
 * PARAMETER    IN: rXBUS -  XBUS에서 수신한 PDU pointer
 * RET. VALUE     : -
 * DESCRIPTION    : Send REGISTER Statistics Info to SGM
 * REMARKS        :
 **end*******************************************************/
void SEND_RegiStatInfoToSGM(XBUS_MSG *rXBUS)
{
    STA_HDR     *recv = (STA_HDR *)rXBUS->Data;
    SPY_REGI_STS    send;
    
    Log.printf(LOG_LV2, "REGI. Statistics Send Request...(%d)\n", recv->command);
    
    if(recv->command == CMD_STA_RESET_ONLY)
    {
        pthread_mutex_lock(&m_regiMutex);
        {
            bzero(&g_RegiSts, sizeof(SPY_REGI_STS));
        }
        pthread_mutex_unlock(&m_regiMutex);
        return;
    }
    
//    bzero(&send, sizeof(send));
    pthread_mutex_lock(&m_regiMutex);
    {
        memcpy(&send, &g_RegiSts, sizeof(SPY_REGI_STS));
        if(recv->command == CMD_STA_SEND_AND_RESET) { bzero(&g_RegiSts, sizeof(SPY_REGI_STS)); }
    }
    pthread_mutex_unlock(&m_regiMutex);
    
    send.header.len     = sizeof(send);
    send.header.command = CMD_STA_RESULT;
    send.header.reserv  = 0x01;                 // 150826 add by SMCHO REGI 통계와 기존 통계를 구분하기 위해 사용
    
    /* 시간 단위로 끊어서 보면 IN 보다 SUCC가 많을 수 있다. (IN은 Queue에 넣을때, SUCC/FAIL은 Queue에서 빼서 처리할 때 증가
    if(send.in_deregi_succ > send.in_deregi) { send.in_deregi_succ = send.in_deregi; }
    if(send.in_regi_succ   > send.in_regi)   { send.in_regi_succ   = send.in_regi; }
    
    send.in_deregi_fail = send.in_deregi - send.in_deregi_succ;
    send.in_regi_fail   = send.in_regi   - send.in_regi_succ;
    */
    
    msendsig(sizeof(send), SGM, MSG_ID_STA, (uint8_t *)&send);
    //logprintf(DEBUG_ERROR, "[REGI] send regi stat....[%d] - REG(%d, %d), DEREG(%d, %d)\n", ret, send.in_regi, send.in_regi_succ, send.in_deregi, send.in_deregi_succ);
}
#endif

#ifdef IPV6_MODE
/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : SEND_StatisticInfoToSGM_V6
 * CLASS-NAME     :
 * PARAMETER    IN: rXBUS -  XBUS에서 수신한 PDU pointer
 * RET. VALUE     : -
 * DESCRIPTION    : Send IPv6 Statistics Info to SGM
 * REMARKS        :
 **end*******************************************************/
void SEND_StatisticInfoToSGM_V6(XBUS_MSG *rXBUS)
{
    STA_HDR     *recv = (STA_HDR *)rXBUS->Data;
    SPY_STS		send;
    
    Log.printf(LOG_LV2, "IPv6 SIP Statistics Send Request...(%d)\n", recv->command);
    
    if(recv->command == CMD_STA_RESET_ONLY) { bzero(&statistic_v6, sizeof(statistic_v6)); return; }
    
    bzero(&send, sizeof(send));
    
    send.header.len     = sizeof(send);
    send.header.command = CMD_STA_RESULT;
    send.header.reserv  = 0x02;                 // IPv6
    
    for(int i = METHOD_INVITE; i <= METHOD_REFER; i ++)
    {
        send.in_request[i]  = statistic_v6.ssw.n_request[i];
        send.out_request[i] = statistic_v6.sam.n_request[i];
    }
    
    for(int i = 0; i < MAX_STA_RESPONSE_CODE; i ++)
    {
        send.in_response_code[i]  = statistic_v6.ssw.n_response_code[SPY_STS_RESP_CODE[i]];
        send.out_response_code[i] = statistic_v6.ssw.n_response_code_out[SPY_STS_RESP_CODE[i]]
                                  + statistic_v6.sam.n_response_code[SPY_STS_RESP_CODE[i]];
    }
    
    send.in_msg_o   = statistic_v6.ssw.n_in_sum[OT_TYPE_ORIG];
    send.in_msg_t   = statistic_v6.ssw.n_in_sum[OT_TYPE_TERM];
    
    send.out_msg_o  = statistic_v6.sam.n_out_sum[OT_TYPE_ORIG] + statistic_v6.ssw.n_response_code_out_sum[OT_TYPE_ORIG];
    send.out_msg_t  = statistic_v6.sam.n_out_sum[OT_TYPE_ORIG] + statistic_v6.ssw.n_response_code_out_sum[OT_TYPE_TERM];
    
    send.in_total_msg  = send.in_msg_o  + send.in_msg_t;
    send.out_total_msg = send.out_msg_o + send.out_msg_t;
    
    send.scm_error = statistic_v6.spy.scm_fail_count;
    //	send.send_fail = g_SAMServer->GetHashFailCount();
    
    msendsig(sizeof(send), SGM, MSG_ID_STA, (uint8_t *)&send);
    
//    if(recv->command == CMD_STA_SEND_AND_RESET) { bzero(&statistic, sizeof(statistic)); }      // RESET STATISTIC DATA
    if(recv->command == CMD_STA_SEND_AND_RESET) { bzero(&statistic_v6, sizeof(statistic_v6)); }      // RESET STATISTIC DATA - change 170125
    
}
#endif  // IPV6_MODE

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : SEND_StatisticInfoToSGM
 * CLASS-NAME     :
 * PARAMETER    IN: rXBUS -  XBUS에서 수신한 PDU pointer
 * RET. VALUE     : -
 * DESCRIPTION    : Send Statistics Info to SGM
 * REMARKS        :
 **end*******************************************************/
void SEND_StatisticInfoToSGM(XBUS_MSG *rXBUS)
{
    STA_HDR     *recv = (STA_HDR *)rXBUS->Data;
	SPY_STS		send;
    
    Log.printf(LOG_LV2, "SIP Statistics Send Request...(%d)\n", recv->command);
    
	if(recv->command == CMD_STA_RESET_ONLY) { bzero(&statistic, sizeof(statistic)); return; }
    
	bzero(&send, sizeof(send));
    
	send.header.len     = sizeof(send);
	send.header.command = CMD_STA_RESULT;
    send.header.reserv  = 0x00;                 // IPv4
    
    for(int i = METHOD_INVITE; i <= METHOD_REFER; i ++)
    {
        send.in_request[i]  = statistic.ssw.n_request[i];
        send.out_request[i] = statistic.sam.n_request[i];
    }
    
    for(int i = 0; i < MAX_STA_RESPONSE_CODE; i ++)
    {
        send.in_response_code[i]  = statistic.ssw.n_response_code[SPY_STS_RESP_CODE[i]];
        send.out_response_code[i] = statistic.ssw.n_response_code_out[SPY_STS_RESP_CODE[i]]
                                  + statistic.sam.n_response_code[SPY_STS_RESP_CODE[i]];
    }

	send.in_msg_o   = statistic.ssw.n_in_sum[OT_TYPE_ORIG];
	send.in_msg_t   = statistic.ssw.n_in_sum[OT_TYPE_TERM];
    
	send.out_msg_o  = statistic.sam.n_out_sum[OT_TYPE_ORIG] + statistic.ssw.n_response_code_out_sum[OT_TYPE_ORIG];
	send.out_msg_t  = statistic.sam.n_out_sum[OT_TYPE_ORIG] + statistic.ssw.n_response_code_out_sum[OT_TYPE_TERM];
    
	send.in_total_msg  = send.in_msg_o  + send.in_msg_t;
	send.out_total_msg = send.out_msg_o + send.out_msg_t;
    
	send.scm_error = statistic.spy.scm_fail_count;
    //	send.send_fail = g_SAMServer->GetHashFailCount();
    
	msendsig(sizeof(send), SGM, MSG_ID_STA, (uint8_t *)&send);
    
    if(recv->command == CMD_STA_SEND_AND_RESET) { bzero(&statistic, sizeof(statistic)); }      // RESET STATISTIC DATA

}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MBUS_MsgFromSGM
 * CLASS-NAME     :
 * PARAMETER    IN: len     - 수신한 메세지 길이
 *              IN: msg_ptr - 수신한 메세지 포인터
 * RET. VALUE     : -
 * DESCRIPTION    : SCM에서 수신받은 XBUS 메세지를 처리하는 함수
 * REMARKS        :
 **end*******************************************************/
int MBUS_MsgFromSGM(int len, XBUS_MSG *xbus_msg)
{
//    STS_MSG *msg_STS_SPY = (STS_MSG *)xbus_msg->Data;
    
    if (g_nHA_State != HA_Active) { return(0); }
    
    SEND_StatisticInfoToSGM(xbus_msg);
#ifdef IPV6_MODE
    SEND_StatisticInfoToSGM_V6(xbus_msg);
#endif
    
#ifdef INCLUDE_REGI
    SEND_RegiStatInfoToSGM(xbus_msg);
#endif
    
    return(0);
}



