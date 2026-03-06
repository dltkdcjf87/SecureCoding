/* File Header
 *fsh**************************************************************
 ******************************************************************
 **
 **  FILE : SPY_scm.cpp
 **
 ******************************************************************
 ******************************************************************
 BLOCK          : SPY
 SUBSYSTEM      : CMS
 SOR-NAME       :
 VERSION        : V4.X
 DATE           : 2014/03/
 AUTHOR         : SEUNG-MO, CHO
 HISTORY        :
 PROCESS(TASK)  :
 PROCEDURES     :
 DESCRIPTION    : SCM모듈과 연동하는 메세지를 처리하는 함수 들
 REMARKS        :
 *end*************************************************************/

#include "SPY_def.h"
#include "SPY_xref.h"
#include "SPY_scm.h"

#pragma mark -
#pragma mark SCM이 보낸 OPTION에 대한 Response를 SCM에 보내는 함수

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SendOptionResponseRP_SCM
 * CLASS-NAME     :
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : SCM에 Channel 해제 요청을 보내는 함수
 * REMARKS        :
 **end*******************************************************/
int SendOptionResponseRP_SCM(uint16_t nRespCode, char *strContact, char *strTo, char *strAV)
{
    S_SCM_OPTION_REPORT  send;
    
    bzero(&send, sizeof(send));

    send.header.id = CMS_SCM_OPTION_REPORT;
    send.code      = nRespCode;
    
    strncpy((char *)send.contact,     strContact, 31);
    strncpy((char *)send.to,          strTo,      63);
    strncpy((char *)send.audio_video, strAV,      63);
    
    if(C_LOG_OPTIONS) { Log.printf(LOG_LV2, "SEND OPTION-200 to SCM [%s][%s][%s]\n",  strContact, strTo, strAV); }
    
    return(msendsig(sizeof(send), SCM, MSG_ID_SCM, (uint8_t *)&send));
}

#pragma mark -
#pragma mark SCM에 Channel 해제 요청을 보내는 함수

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SendChDeallocRQ_SCM
 * CLASS-NAME     :
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : SCM에 Channel 해제 요청을 보내는 함수
 * REMARKS        :
 **end*******************************************************/
int SendChDeallocRQ_SCM(int nBoard, int nChannel)
{
	S_SCM_CH_FREE_RQ  send;
    
    bzero(&send, sizeof(send));

    send.header.id  = CMD_SCM_CH_FREE_RQ;
    send.board      = nBoard;
    send.ch         = nChannel;

    return(msendsig(sizeof(send), SCM, MSG_ID_SCM, (uint8_t *)&send));
}


#pragma mark -
#pragma mark SCM 채널할당 요청 T/O

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : TOUT_SendChAllocRQ
 * CLASS-NAME     : -
 * PARAMETER    IN: pCR  - Call Register 구조체 포인터
 *              IN: arg2 - 사용안함
 * RET. VALUE     : -
 * DESCRIPTION    : SCM에서 CHALLOCRQ에 대한 응답이 오지 않고 T/O 나서 처리하는 함수
 * REMARKS        : TO_TIME_SCM_ALLOC(10초) 동안 응답이 안오는 경우
 **end*******************************************************/
int TOUT_SendChAllocRQ(size_t nRcvSip, size_t arg2)
{
    CR_DATA *pCR = (CR_DATA *)nRcvSip;
   
    if(pCR->timer_id == 0)
    {
        // timer_id 가 RESET되었다면 SCM에서 MSG를 수신해서 처리한 것으로 간주하고 return 처리...
        // T/O과 SCM에 응답에 절묘하게 맞아 떨어지면 가능성이 있지만... 10초 후에 SCM에서 응답이 오고 이게 타이머랑 맞아 떨어진다..
        // 확률적으로 거의 발생할 가능성이 없음
        
        Log.printf(LOG_ERR, "[TO] TO_SCM_SendChAllocRQ() timeout(%p) but timer_id == 0 ???\n", pCR);
        return(0);
    }
#ifdef IPV6_MODE
    if(pCR->bIPV6) { statistic_v6.spy.scm_fail_count ++; }
    else
#endif
    {
        statistic.spy.scm_fail_count ++;        // 통계
    }
    Log.printf(LOG_ERR, "[TO] TO_SCM_SendChAllocRQ() timeout(%p)\n", pCR);
    RSSW_SendResponse(pCR, 500, "SIP;cause=500;text=\"Internal Timeout\"");
    g_CR.free(pCR);      // free - UDP 수신시 alloac한 메모리
    
    return(0);
}




#pragma mark -
#pragma mark SCM 채널할당 요청에 대한 응답 처리


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SCM_ChAllocFailProc
 * CLASS-NAME     : -
 * PARAMETER    IN: rPDU - SCM에서 보낸 응답 메세지 pointer
 * RET. VALUE     : BOOL
 * DESCRIPTION    : SCM에서 채널 할당 요청시 동작시킨 타이머를 종료 시키는 함수
 * REMARKS        :
 **end*******************************************************/
void SCM_ChAllocFailProc(R_SCM_CH_ALLOC_RP *rPDU)
{
    char    strReason[64];
    int     response_code = 503;    // default response code;
    
#ifdef IPV6_MODE
    if(rPDU->pCR->bIPV6) { statistic_v6.spy.scm_fail_count ++; }
    else
#endif
    {
        statistic.spy.scm_fail_count ++;        // 통계
    }

    switch(rPDU->header.reason)
    {
        case SERVICE_NUM_EXCEED:                    snprintf(strReason, sizeof(strReason), "Q.850;cause=34;text=\"No circuit/channel available\""); break;
        case UNKNOWN_SERVICE:                       snprintf(strReason, sizeof(strReason), "SIP;cause=503;text=\"Unknown Service Key\"");           break;
        case BLOCKED_TAS_CSFB: response_code = 480; snprintf(strReason, sizeof(strReason), "SIP;cause=415;text=\"Unsupported Media Type\"");        break;
        default:                                    snprintf(strReason, sizeof(strReason), "SIP;cause=500;text=\"Service not loaded\"");            break;
            
        case SERVICE_BLOCKED:   // change by SMCHO 130529 for TAS
            if((C_TAS_BLOCK_480_FLAG == true) && (strcasecmp(rPDU->serviceKey, "mmtel") == 0))
            {
                response_code = 480;
                snprintf(strReason, sizeof(strReason), "SIP;cause=415;text=\"Unsupported Media Type\"");
            }
            else
            {
                snprintf(strReason, sizeof(strReason), "SIP;cause=500;text=\"Service not loaded\"");
            }
            break;
            
        case OVERLOAD_DENY:     // mPBX 요구사항(호폐기 방식 설정으로 제어 가능해야 함)
            if(C_OVERLOAD_RESP_CODE == 0) { return; }       // 0이면 무응답
            response_code = C_OVERLOAD_RESP_CODE;
            snprintf(strReason, sizeof(strReason), "Q.850;cause=34;text=\"No circuit/channel available\"");
            break;

    }

    RSSW_SendResponse(rPDU->pCR, response_code, strReason);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RECV_ChAllocRPFromSCM
 * CLASS-NAME     : -
 * PARAMETER    IN: rPDU - SCM에서 보낸 응답 메세지 pointer
 * RET. VALUE     : BOOL
 * DESCRIPTION    : SCM에서 수신한 ChAllocRP를 처리하는 함수
 * REMARKS        :
 **end*******************************************************/
bool RECV_ChAllocRPFromSCM(R_SCM_CH_ALLOC_RP *rPDU)
{
    time_t  t_now  = time(NULL);
    CR_DATA *pCR = rPDU->pCR;

    if(rPDU->side != MY_SIDE) { return(true);  }    // 응답은 A/B모두 수신하기 때문에 채널 할당은 요청 보낸 SIDE에서 처리 함    
    
    // FIXIT - ChAllocRQ를 보낸 시간을 메시지에 포함하고 SCM으로 부터 리턴 받게 수정하고 일정 시간이 지나서 수신된 메시지는 처리하지 않고 삭제하거나 티이머를 종료시키고 삭제하는 방법 검토....
    //         꽤 시간이 지나서 응답이 온다면 응답이 와도 에러로 처리하는게 맞는 방법임 (시간은 가능한 config로)
    Log.printf(LOG_LV2, "[SCM_ChAllocRP][RESULT=%s] - Side=%d, Svc=%s, Dp=%s,  Board=%d, Ch=%d, pCR=%p, timer_id=%ld, Reason=%d, Call-Id=[%s]\n", 
						(rPDU->header.response == SCM_CONFIRM) ? "OK": "FAIL", 
						rPDU->side,  
						rPDU->serviceKey, 
						rPDU->dp, 
						rPDU->board, 
						rPDU->ch, 
						rPDU->pCR, 
						rPDU->timer_id, 
						rPDU->header.reason, 
						pCR->info.strCall_ID);


	if(pCR != NULL)
	    g_ScmMap.erase(pCR);    // SCM에서 응답이 왔기 때문에 MAP에서 삭제

    if(rPDU->header.response != SCM_CONFIRM)    // 채할당 실패
    {
        /*
        if(rPDU->header.reason == SERVICE_NUM_EXCEED)
        {
            Log.printf(LOG_ERR, "[SCM_ChAllocRP] SCM_CONFIRM reason=SVC_NO_EXD pCR=%zu, tiemr_id = %ld\n", pCR, rPDU->timer_id);
            return(false);
        }
        */
        Log.printf(LOG_LV3, "[SCM_ChAllocRP] Not Confirm Reason=%d pCR=%zu, tiemr_id = %ld\n", rPDU->header.reason, pCR, rPDU->userId);

        SCM_ChAllocFailProc(rPDU);
        g_CR.free(pCR);
        return(false);
    }
    
#ifdef IPV6_MODE
    if(pCR->bIPV6) { statistic_v6.spy.scm_resp_count ++; }
    else
#endif
    {
        statistic.spy.scm_resp_count ++;            // 통계
    }

    pCR->bd  = rPDU->board;
    pCR->ch  = rPDU->ch;
//    pCR->info.nTrace = g_Trace.exist(pCR->info.strFrom, pCR->info.strTo, pCR->info.strServiceKey, pCR->bd, pCR->ch);  // Call Trace 정보
// HASH_InsertAndSync 함수에서 Trace 확인
    switch(rPDU->header.reason)
    {
#ifdef TAS_MODE
        case BLOCKED_TAS_CSRN:   pCR->info.bCSRN = true;      // break; // TAS CSRN case - flag set & HASH_InsertAndSync()
#endif
        case ALLOCATION_SUCCESS:

#ifdef  SOIP_REGI
        	if(rPDU->pCR->nSipMsg_ID == METHOD_REGISTER)
        	{
				if(strcmp(pCR->info.strServiceKey, "V_UMC") == 0)
				{
					Send3rdREGISTER_TAS(pCR);
				}
				else if(strcmp(pCR->info.strServiceKey, "V_MTRS") == 0)
				{
	            	Send3rdREGISTER(pCR);
				}
				else
				{
					Log.printf(LOG_ERR, "RECV_ChAllocRPFromSCM() Not def ServiceKey!!\n");
				}
     
            	g_CR.free(pCR);
            	return(true);
        	}
#endif

            if(HASH_InsertAndSync(pCR, t_now) == false)
            {

                RSSW_SendResponse(pCR, 500, "SIP;cause=500;text=\"Internal Hash Error\"");
                g_CR.free(pCR);
                return(false);
            }

            break;
        default:
            Log.printf(LOG_ERR, "RECV_ChAllocRPFromSCM() - REALLOCATION \n"); break;   // 현재 사용안함(Hash 저장없이 Send)
    }

#ifndef SAM_SEND_100
    RSSW_SendResponse(pCR, 100, NULL);     // SCM MAP을 사용하는 경우 initial message에 대한 100을 Ch Alloc이 되면 보낸다.
#endif
    
    RSSW_MakeSendBufferAndSendToSAM(pCR);   // 여기서 UDP 수신시 alloac한 메모리 free 됨
    g_CR.free(pCR);                         // free - UDP 수신시 alloac한 메모리
    
    return(true);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RECV_ChFreeRPFromSCM
 * CLASS-NAME     : -
 * PARAMETER    IN: rPDU - SCM에서 보낸 채널 해제 메세지 pointer
 * RET. VALUE     : BOOL
 * DESCRIPTION    : SCM에서 수신한 ChAllocRP를 처리하는 함수
 * REMARKS        :
 **end*******************************************************/
bool RECV_ChFreeRPFromSCM(R_SCM_CH_FREE_RP *rPDU)
{
    if(g_Hash_O.delete_hash(rPDU->board, rPDU->ch)) { Log.printf(LOG_LV1, "[O] SCM Free(%d,%d) - OK  \n", rPDU->board, rPDU->ch); }
    else                                            { Log.printf(LOG_LV1, "[O] SCM Free(%d,%d) - FAIL\n", rPDU->board, rPDU->ch); }
    
    if(g_Hash_T.delete_hash(rPDU->board, rPDU->ch)) { Log.printf(LOG_LV1, "[T] SCM Free(%d,%d) - OK  \n", rPDU->board, rPDU->ch); }
    else                                            { Log.printf(LOG_LV1, "[T] SCM Free(%d,%d) - FAIL\n", rPDU->board, rPDU->ch); }

    return(true);
}

#pragma mark -
#pragma mark SCM에 채널할당 요청을 전송하는 함수

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SEND_ChAllocRQToSCM
 * CLASS-NAME     : -
 * PARAMETER    IN: pCR - Call Register 구조체 포인터
 * RET. VALUE     : BOOL
 * DESCRIPTION    : SCM에 Channel Allocation Request를 보내는 함수
 * REMARKS        :
 **end*******************************************************/
bool SEND_ChAllocRQToSCM(CR_DATA *pCR)
{
    S_SCM_CH_ALLOC_RQ   send;
    int     ret;
    
    bzero(&send, sizeof(send));
    
    strncpy((char *)send.serviceKey, pCR->info.strServiceKey, 63);
#if 0   // 20160908 - strUser_ID 사용안하게 변경
    strncpy((char *)send.userId,     pCR->info.strUser_ID,    63);
#else
    strcpy((char *)send.userId,     " ");
#endif
    strncpy((char *)send.dp,         pCR->info.strDP,          4);
    
    send.header.id  = CMS_SCM_CH_ALLOC_RQ;
    send.side       = MY_SIDE;
    send.req_time   = time(NULL);       // 채널 할당 요청을 보내는 현재 시간(수신시 시간을 비교하기 위해)
    send.pCR        = pCR;
    send.timer_id = 0;

    if(send.timer_id < 0)
    {
        Log.printf(LOG_ERR, "[SCM] SCM_SendChAllocRQ() timer error(%d)\n", send.timer_id);
        return(false);
    }
    
    pCR->timer_id = send.timer_id;      // 예외 처리를 위해 timer_id 저장
    
    if((ret = msendsig(sizeof(send), SCM, MSG_ID_SCM, (uint8_t *)&send)) != 0)
    {
#if 0       // 20160908 - strUser_ID 사용안하게 변경
        Log.printf(LOG_ERR, "[SCM] SCM_SendChAllocRQ() FAIL(%d) UserID = %s, ServiceKey = %s, DP = %s\n", ret, pCR->info.strUser_ID, pCR->info.strServiceKey, pCR->info.strDP);
#else
        Log.printf(LOG_ERR, "[SCM] SCM_SendChAllocRQ() FAIL(%d) From = %s, To = %s, Call-ID = %s, ServiceKey = %s, DP = %s\n", ret, pCR->info.strFrom, pCR->info.strTo, pCR->info.strCall_ID, pCR->info.strServiceKey, pCR->info.strDP);
#endif
        return(false);
    }
    
    Log.printf(LOG_LV1, "[SCM] SEND SCM_SendChAllocRQ() Call-ID = %s, ServiceKey = %s, DP = %s\n", pCR->info.strCall_ID, pCR->info.strServiceKey, pCR->info.strDP);
    return(true);
}

#pragma mark -
#pragma mark XBUS(M) callback - SCM 할당/해제에 대한 응답 수신

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MBUS_MsgFromSCM
 * CLASS-NAME     :
 * PARAMETER    IN: len     - 수신한 메세지 길이
 *              IN: msg_ptr - 수신한 메세지 포인터
 * RET. VALUE     : -
 * DESCRIPTION    : SCM에서 수신받은 XBUS 메세지를 처리하는 함수
 * REMARKS        :
 **end*******************************************************/
int MBUS_MsgFromSCM(int len, XBUS_MSG *rXBUS)
{
    R_SCM_CH_ALLOC_RP 	*rPDU = (R_SCM_CH_ALLOC_RP *)rXBUS->Data;
    
    switch(rPDU->header.id)
    {
        case CMS_SCM_CH_ALLOC_RP: RECV_ChAllocRPFromSCM((R_SCM_CH_ALLOC_RP *)rXBUS->Data); break;
        case CMD_SCM_CH_FREE_RP:  RECV_ChFreeRPFromSCM ((R_SCM_CH_FREE_RP  *)rXBUS->Data); break;
        default: Log.printf(LOG_ERR, "MBUS_MsgFromSCM() Undef Response[%d]\n",rPDU->header.id); break;
    }
    return(0);
}



