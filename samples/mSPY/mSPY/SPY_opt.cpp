/* File Header
 *fsh**************************************************************
 ******************************************************************
 **
 **  FILE : SPY_opt.cpp
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
 DESCRIPTION    : OPTIONS 메세지를 Queue에서 읽어서 처리하는 함수 들
 REMARKS        :
 *end*************************************************************/

#include "SPY_def.h"
#include "SPY_xref.h"

#define MAX_DEQUEUE_COUNT_OPT   10      // QUEUE메세지 연속 처리 횟수 (CPU 부하 고려) - FIXIT: goto config

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSSW_CheckExtServerStatus
 * CLASS-NAME     : -
 * PARAMETER    IN: pCR - 수신한 SIP 메세지 구조체 포인터
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 수신한 OPTIONS 메세지로 상대편 상태를 ALIVE로 변경하는 함수
 * REMARKS        : Down 상태에서 OPTIONS를 받으면 fault를 flase로 변경
 **end*******************************************************/
void RSSW_CheckExtServerStatus(CR_DATA *pCR)
{
#ifdef IPV6_MODE
    if(pCR->bIPV6) { g_ExtList_V6.receive_options(&(pCR->peer6.sin6_addr), pCR->ot_type); }
    else
#endif
                   { g_ExtList.receive_options(pCR->info.strCscfIp, pCR->ot_type); }
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSSW_SendOption200
 * CLASS-NAME     : -
 * PARAMETER    IN: pCR - 수신한 SIP 메세지 구조체 포인터
 * RET. VALUE     : BOOL
 * DESCRIPTION    : CSCF로 OPTION에 대한 200 Message를 보내는 함수
 * REMARKS        : 수신된 UDP의 source 주소로 응답한다. (일반적으로 Via)
 *                :   - CNSR 장비가 VIA의 주소로 보내면 인식을 못함
 **end*******************************************************/
bool RSSW_SendOption200(CR_DATA *pCR)
{
    int     ret;
    int     nSendLen;
    char    strSendBuf[MAX_BUF_SIZE+1];

    bzero(strSendBuf, sizeof(strSendBuf));
    
    /*
     * Write Start Line(Status Line)
     */
    nSendLen = sprintf(strSendBuf, "SIP/2.0 200 OK");

    /*
     * 수신한 header 중 일부 특정 Header를 그대로 복사해서 전송
     */
    LIB_AppendHeader(strSendBuf, &nSendLen, pCR->strRcvBuf, "\r\nVia:");
    LIB_AppendHeader(strSendBuf, &nSendLen, pCR->strRcvBuf, "\r\nRoute:");
    LIB_AppendHeader(strSendBuf, &nSendLen, pCR->strRcvBuf, "\r\nFrom");
    LIB_AppendHeader(strSendBuf, &nSendLen, pCR->strRcvBuf, "\r\nTo:");
    LIB_AppendHeader(strSendBuf, &nSendLen, pCR->strRcvBuf, "\r\nCall-ID:");
    LIB_AppendHeader(strSendBuf, &nSendLen, pCR->strRcvBuf, "\r\nCSeq:");

	nSendLen += sprintf(&strSendBuf[nSendLen], "\r\nContent-Length: 0\r\n\r\n");
	strSendBuf[nSendLen] = '\0';

    if(pCR->ot_type == OT_TYPE_ORIG)
    {
#ifdef IPV6_MODE
        if(pCR->bIPV6)
        {
            ret = rawUDP_SendByNameNoCRC_V6(C_ORIG_STACK_IP_V6, C_ORIG_STACK_PORT, pCR->info.strCscfIp, pCR->info.nCscfPort, strSendBuf, nSendLen, C_NETWORK_QOS);
            if(ret > 0) { ret = 0;  }   // FIXME: V6 함수에서는 전송한 BYTE가 결과로 return된다.
            else        { ret = -1; }
        }
        else
#endif
        {
            ret = rawUDP_SendByNameNoCRC(C_ORIG_STACK_IP, C_ORIG_STACK_PORT, pCR->info.strCscfIp, pCR->info.nCscfPort, strSendBuf, nSendLen, C_NETWORK_QOS);
        }
    }
    else
    {

#ifdef IPV6_MODE
        if(pCR->bIPV6)
        {
            ret = rawUDP_SendByNameNoCRC_V6(C_TERM_STACK_IP_V6, C_TERM_STACK_PORT, pCR->info.strCscfIp, pCR->info.nCscfPort, strSendBuf, nSendLen, C_NETWORK_QOS);
            if(ret > 0) { ret = 0;  }   // FIXME: V6 함수에서는 전송한 BYTE가 결과로 return된다.
            else        { ret = -1; }
        }
        else
#endif
        {
            ret = rawUDP_SendByNameNoCRC(C_TERM_STACK_IP, C_TERM_STACK_PORT, pCR->info.strCscfIp, pCR->info.nCscfPort, strSendBuf, nSendLen, C_NETWORK_QOS);
        }
    }
    
    if(ret >= 0)       // - 인 경우에만 Error
    {
#ifdef IPV6_MODE
        if(pCR->bIPV6)
        {
            statistic_v6.ssw.n_response_code_out[200] ++;
            statistic_v6.ssw.n_response_code_out_sum[pCR->ot_type] ++;
        }
        else
#endif
        {
            statistic.ssw.n_response_code_out[200] ++;
            statistic.ssw.n_response_code_out_sum[pCR->ot_type] ++;
        }
    }
    else
    {
        Log.printf(LOG_ERR, "[RSSW] RSSW_SendOption200(%d) errno=%d[%s]!!!\n", ret, errno, strerror(errno));
    }
    
    // FIXIT: Trace...
    //	TraceMsg(nRespCode, strCscfIp, nCscfPort);
    
    if(C_LOG_OPTIONS)
        Log.color(COLOR_YELLOW, LOG_LV1, "[RSSW] >>>--->>> Send Message(%d) [200-OPTIONS] [%s] >>>--->>>\n%s\n", nSendLen, pCR->info.strCscfAddr, strSendBuf);
    
    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSSW_SendOptionsResponseToSCM
 * CLASS-NAME     : -
 * PARAMETER    IN: pCR - 수신한 SIP 메세지 구조체 포인터
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 수신한 OPTIONS에 대한 응답을 SCM으로 전송하는 함수
 * REMARKS        : SCM에서 TAS는 user_part(user@ip)로 구분하고, mPBX는 IP로 구분한다. 
 **end*******************************************************/
bool RSSW_SendOptionsResponseToSCM(CR_DATA *pCR)
{
    char    strTemp[256];
    char    strContact[32], strAV[64];
    char    *ptrAddr;

#ifndef TAS_MODE    // MPBX_MODE
    int     port;
    IP_TYPE ip_type;
#else               // TAS_MODE
    URI_TYPE    nUriType;
#endif
    // Get To header (Mandatory)
    if(LIB_GetStrFromAToB(pCR->strRcvBuf, "To:", ASCII_CRLF, strTemp, sizeof(strTemp)))
    {
        // Get From addr(form sip: to ;tag)
#ifndef TAS_MODE    // MPBX_MODE
        if(LIB_GetIpAddrInContactHeaderV6(strTemp, pCR->info.strTo, sizeof(pCR->info.strTo), &port, &ip_type) == false)     // 20160919
#else               // TAS_MODE
        if(LIB_GetUriFromHeader(strTemp, &nUriType, pCR->info.strTo, sizeof(pCR->info.strTo)) == false)
#endif
        {
            Log.printf(LOG_ERR, "RSSW_SendOptionsResponseToSCM() From: Header Error: there's no tel: or sip:\n");
            return(false);
        }
    }
    else
    {
        Log.printf(LOG_ERR, "RSSW_SendOptionsResponseToSCM() - Not Exist To: Header\n");
        return(false);
    }
    
    bzero(strContact, sizeof(strContact));
    bzero(strAV,      sizeof(strAV));
    
    // Get Contact header(Option)
    if(LIB_GetStrFromAToB(pCR->strRcvBuf, "Contact:", ASCII_CRLF, strTemp, sizeof(strTemp)))
    {
        // Get From addr(form sip: to ;tag)
#ifndef TAS_MODE    // MPBX_MODE
        if(LIB_GetIpAddrInContactHeaderV6(strTemp, strContact, sizeof(strContact), &port, &ip_type) == false)       // 20160919
#else               // TAS_MODE
        if(LIB_GetUriFromHeader(strTemp, &nUriType, strContact, sizeof(strContact)) == false)
#endif
        {
            Log.printf(LOG_ERR, "RSSW_SendOptionsResponseToSCM() Contact: Header Not Exist: there's no tel: or sip:\n");
        }

        
        if((ptrAddr = strstr(strTemp, "audio")) != NULL)
        {
            if((ptrAddr = strstr(strTemp, "video")) != NULL) { sprintf(strAV, "audio,video"); }
            else                                             { sprintf(strAV, "audio"); }
        }
        else
        {
            if((ptrAddr = strstr(strTemp, "video")) != NULL) { sprintf(strAV, "video"); }
        }
    }
    
    SendOptionResponseRP_SCM(pCR->nSipMsg_ID, strContact, pCR->info.strTo, strAV);

    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSSW_RecvOptions
 * CLASS-NAME     : -
 * PARAMETER    IN: pCR - 수신한 SIP 메세지 구조체 포인터
 * RET. VALUE     : -
 * DESCRIPTION    : 수신한 OPTIONS 메세지를 처리하는 함수
 * REMARKS        :
 **end*******************************************************/
bool RSSW_RecvOptions(CR_DATA *pCR)
{
    // FIXIT: SIP Parsing Here
    // if(error) send 400 Bad Request;
    
    if(pCR->nSipMsgType == SIP_TYPE_REQUEST)    // OPTION 수신
    {
        if(C_AS_BLOCK == false) { RSSW_SendOption200(pCR); }  // BLOCK 이면 OPTIONS에 대한 응답을 하지 않는다.
    
        RSSW_CheckExtServerStatus(pCR);           // 외부 연동 서버 상태 Check
    }
    else                                        // OPTION에 대한 응답 수신
    {
        RSSW_SendOptionsResponseToSCM(pCR);
    }
    // FIXIT:
    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSSW_DequeueThread_OPT
 * CLASS-NAME     : -
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : OPTIONS 메세지를 처리하는 Thread
 * REMARKS        :
 **end*******************************************************/
void *RSSW_DequeueThread_OPT(void *arg)
{
    CR_DATA *pCR = NULL;
    int     count = 0;

    while(true)
    {
        // FIXIT: strRcvBuf가 아니라 CR_DATA 구조체로 받아야 함....
        if(Q_Opt.dequeue((void **)&pCR) == false)        // queue empty
        {
            msleep(50);         // 50 msec
            count = 0;          // 연속처리 count RESET
            continue;
        }

        RSSW_RecvOptions(pCR);
        g_CR.free(pCR);
        
        // 10개이상 연속처리한 경우  sleep 후 retry ...
        if(++count < MAX_DEQUEUE_COUNT_OPT) { continue; }   // 메세지가 많을 경우 연속 10개 까지만 처리하고  sleep 하기 위한 루틴
        msleep(50);             // 50 msec;
        count = 0;              // 연속처리 count RESET
        continue;
    }
}

