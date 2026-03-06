
/* File Header
 *fsh**************************************************************
 ******************************************************************
 **
 **  FILE : SPY_rssw.cpp
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
 DESCRIPTION    : SSW(CSCF)에서 SIP 메세지를 수신하여 SAM로 전달하는 함수들
 REMARKS        :
 *end*************************************************************/

#include "SPY_def.h"
#include "SPY_xref.h"

//#pragma mark -
//#pragma mark CSCF(SSW) 라이브러리 성격 함수들
/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSSW_sockaddr_to_string
 * CLASS-NAME     : -
 * PARAMETER    IN: pCR       - Call Register 구조체 포인터
 * RET. VALUE     : -
 * DESCRIPTION    : Packet을 수신한 주소를 string 으로 변경하여 cscf 정보에 저장
 * REMARKS        :
 **end*******************************************************/
void RSSW_sockaddr_to_string(CR_DATA *pCR)
{
#ifdef IPV6_MODE
    IP_TYPE ip_type;
    
    ip_type = LIB_get_ip_type(pCR->peer6.sin6_addr, pCR->info.strCscfIp, sizeof(pCR->info.strCscfIp));
    if(ip_type == IP_TYPE_V6) { pCR->bIPV6 = true; }
    pCR->info.nCscfPort = ntohs(pCR->peer6.sin6_port);
#else
    inet_ntop(AF_INET, &pCR->peer.sin_addr, pCR->info.strCscfIp, sizeof(pCR->info.strCscfIp));
    pCR->info.nCscfPort = ntohs(pCR->peer.sin_port);
#endif
    
    snprintf(pCR->info.strCscfAddr, sizeof(pCR->info.strCscfAddr), "%s:%d", pCR->info.strCscfIp, pCR->info.nCscfPort);
}

//#pragma mark -
//#pragma mark CSCF(SSW) 등록/확인 함수들

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSSW_CheckRegisteredSSW
 * CLASS-NAME     : -
 * PARAMETER    IN: pCR - Call Register 구조체 포인터
 * RET. VALUE     : -1(UNREG)/0(REG-BLOCK)/1(REG-OK)
 * DESCRIPTION    : 입력 주소가 등록된 CSCF Address인지 검사하는 함수
 * REMARKS        :
 **end*******************************************************/
int RSSW_CheckRegisteredSSW(CR_DATA *pCR)
{
    bool    bBlock;
    
#ifdef IPV6_MODE
    if(pCR->bIPV6)
    {
        if(g_ExtList_V6.select(&(pCR->peer6.sin6_addr), &bBlock) == true)
        {
            if(bBlock == true) { return(0); }   // Registered but BLOCK
            else               { return(1); }   // Registered OK
        }
    }
    else
#endif
    {
        if(g_ExtList.select(pCR->info.strCscfIp, &bBlock) == true)
        {
            if(bBlock == true) { return(0); }   // Registered but BLOCK
            else               { return(1); }   // Registered OK
        }
    }
    return(-1);     // Unregistered

}


//#pragma mark -
//#pragma mark CSCF로 수신한 메세지에 대한 응답 메세지를 전송하는 함수 들

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSSW_SendResponse
 * CLASS-NAME     : -
 * PARAMETER    IN: pCR       - Call Register 구조체 포인터
 *              IN: nRespCode - CSCF로 보낼 응답 메세지 Code
 *              IN: strReason - Reason Header 내용
 * RET. VALUE     : BOOL
 * DESCRIPTION    : CSCF로 SIP Response Message를 보내는 함수
 * REMARKS        : SPY에서 바로 response를 보내는 경우는 에러 case(단, 100 제외)
 *                : First VIA의 주소로 응답한다.
 **end*******************************************************/
bool RSSW_SendResponse(CR_DATA *pCR, int nRespCode, const char *strReason=NULL)
{
    int     ret;
    char    strCscfIp[64];
    int     nCscfPort;
    int     nSendLen;
    char    strSendBuf[MAX_BUF_SIZE+1];
    
    /*
     * Via Header에서 응답할 CSCF Address를 가져옴
     */
#ifdef IPV6_MODE
    if(pCR->bIPV6)
    {
        if(LIB_GetAddrFromViaHeaderV6(pCR->strRcvBuf, strCscfIp, sizeof(strCscfIp), &nCscfPort) == false)
        {
            Log.printf(LOG_ERR, "RSSW_SendResponse() can't get Addr from Via Header\n");
            return(false);
        }
    }
    else
#endif
    {
        if(LIB_GetAddrFromViaHeader(pCR->strRcvBuf, strCscfIp, sizeof(strCscfIp), &nCscfPort) == false)
        {
            Log.printf(LOG_ERR, "RSSW_SendResponse() can't get Addr from Via Header\n");
            return(false);
        }
    }
    
	/*
     * Looping 방지 - REQUEST를 보내온 CSCF 주소가 SPY와 같은 경우 Looping 에러 처리 cf) ORIG/TERM stack IP는 동일함
     */
	if(nRespCode > 100)
    {
        if((strcmp(strCscfIp, C_ORIG_STACK_IP) == 0) || (strcmp(strCscfIp, "127.0.0.1") == 0) || (strcmp(strCscfIp, "0.0.0.0") == 0))
        {
            Log.printf(LOG_ERR, "RSSW_SendResponse() Looping Addr in Via=[%s]\n", strCscfIp);
            return(false);
        }
#ifdef IPV6_MODE
        else if((LIB_ipv6cmp(strCscfIp, C_IN6_SPY) == true) || (strcmp(strCscfIp, "::1") == 0) || (strcmp(strCscfIp, "::") == 0))
        {
            Log.printf(LOG_ERR, "RSSW_SendResponse() Looping Addr in Via=[%s]\n", strCscfIp);
            return(false);
        }
#endif
    }
    
    /*
     * Write Start Line(Status Line)
     */
	bzero(strSendBuf, sizeof(strSendBuf));
	switch (nRespCode)
	{
		case 100: nSendLen = sprintf(strSendBuf, "SIP/2.0 100 Trying");                             break;
		case 200: nSendLen = sprintf(strSendBuf, "SIP/2.0 200 OK");                                 break;
		case 300: nSendLen = sprintf(strSendBuf, "SIP/2.0 300 Multiple Choices");                   break;
		case 301: nSendLen = sprintf(strSendBuf, "SIP/2.0 301 Moved Permanently");                  break;
		case 302: nSendLen = sprintf(strSendBuf, "SIP/2.0 302 Moved Temporarily");                  break;
		case 305: nSendLen = sprintf(strSendBuf, "SIP/2.0 305 Use Proxy");                          break;
		case 400: nSendLen = sprintf(strSendBuf, "SIP/2.0 400 Bad Request");                        break;
		case 401: nSendLen = sprintf(strSendBuf, "SIP/2.0 401 Unauthorized");                       break;
		case 402: nSendLen = sprintf(strSendBuf, "SIP/2.0 402 Payment Required");                   break;
		case 403: nSendLen = sprintf(strSendBuf, "SIP/2.0 403 Forbidden");                          break;
		case 404: nSendLen = sprintf(strSendBuf, "SIP/2.0 404 Not Found");                          break;
		case 405: nSendLen = sprintf(strSendBuf, "SIP/2.0 405 Method Not Allowed");                 break;
		case 406: nSendLen = sprintf(strSendBuf, "SIP/2.0 406 Not Acceptable");                     break;
		case 407: nSendLen = sprintf(strSendBuf, "SIP/2.0 407 Proxy Authentication Required");      break;
		case 408: nSendLen = sprintf(strSendBuf, "SIP/2.0 408 Request Timeout");                    break;
		case 480: nSendLen = sprintf(strSendBuf, "SIP/2.0 480 Temporarily Unavailable");            break;
		case 481: nSendLen = sprintf(strSendBuf, "SIP/2.0 481 Call/Transaction Does Not Exist");    break;
		case 487: nSendLen = sprintf(strSendBuf, "SIP/2.0 487 Request Terminated");                 break;
		case 488: nSendLen = sprintf(strSendBuf, "SIP/2.0 488 Not Acceptable Here");                break;
		case 491: nSendLen = sprintf(strSendBuf, "SIP/2.0 491 Request Pending");                    break;
		case 500: nSendLen = sprintf(strSendBuf, "SIP/2.0 500 Server Internal Error");              break;
		case 501: nSendLen = sprintf(strSendBuf, "SIP/2.0 501 Not Implemented");                    break;
		case 503: nSendLen = sprintf(strSendBuf, "SIP/2.0 503 Service Unavailable");                break;
		case 504: nSendLen = sprintf(strSendBuf, "SIP/2.0 504 Server Timeout");                     break;
		case 600: nSendLen = sprintf(strSendBuf, "SIP/2.0 600 Busy Everywhere");                    break;
		case 603: nSendLen = sprintf(strSendBuf, "SIP/2.0 603 Decline");                            break;
		case 604: nSendLen = sprintf(strSendBuf, "SIP/2.0 604 Does Not Exist Anywhere");            break;
		case 606: nSendLen = sprintf(strSendBuf, "SIP/2.0 606 Not Acceptable");                     break;
		default:  nSendLen = sprintf(strSendBuf, "SIP/2.0 503 Service Unavailable");                break;
	}
    
    /*
     * 수신한 header 중 일부 특정 Header를 그대로 복사해서 전송
     */
    LIB_AppendHeader(strSendBuf, &nSendLen, pCR->strRcvBuf, "\r\nVia:");
    LIB_AppendHeader(strSendBuf, &nSendLen, pCR->strRcvBuf, "\r\nRoute:");
    LIB_AppendHeader(strSendBuf, &nSendLen, pCR->strRcvBuf, "\r\nFrom");
    LIB_AppendHeader(strSendBuf, &nSendLen, pCR->strRcvBuf, "\r\nTo:");
    LIB_AppendHeader(strSendBuf, &nSendLen, pCR->strRcvBuf, "\r\nCall-ID:");
    LIB_AppendHeader(strSendBuf, &nSendLen, pCR->strRcvBuf, "\r\nCSeq:");
    
    if(strReason) { nSendLen += sprintf(&strSendBuf[nSendLen], "\r\nReason: %s", strReason); }
    
	nSendLen += sprintf(&strSendBuf[nSendLen], "\r\nContent-Length: 0\r\n\r\n");
	strSendBuf[nSendLen] = '\0';
    
    if(pCR->ot_type == OT_TYPE_ORIG)
    {
#ifdef IPV6_MODE
        if(pCR->bIPV6)
        {
            ret = rawUDP_SendByNameNoCRC_V6(C_ORIG_STACK_IP_V6, C_ORIG_STACK_PORT, strCscfIp, nCscfPort, strSendBuf, nSendLen, C_NETWORK_QOS);
            if(ret > 0) { ret = 0;  }   // FIXME: V6 함수에서는 전송한 BYTE가 결과로 return된다.
            else        { ret = -1; }
        }
        else
#endif  // IPV6_MODE
        {
            ret = rawUDP_SendByNameNoCRC(C_ORIG_STACK_IP, C_ORIG_STACK_PORT, strCscfIp, nCscfPort, strSendBuf, nSendLen, C_NETWORK_QOS);
        }
    }
    else
    {
#ifdef IPV6_MODE
        if(pCR->bIPV6)
        {
            ret = rawUDP_SendByNameNoCRC_V6(C_TERM_STACK_IP_V6, C_TERM_STACK_PORT, strCscfIp, nCscfPort, strSendBuf, nSendLen, C_NETWORK_QOS);
            if(ret > 0) { ret = 0;  }   // FIXME: V6 함수에서는 전송한 BYTE가 결과로 return된다.
            else        { ret = -1; }
        }
        else
#endif  // IPV6_MODE
        {
            ret = rawUDP_SendByNameNoCRC(C_TERM_STACK_IP, C_TERM_STACK_PORT, strCscfIp, nCscfPort, strSendBuf, nSendLen, C_NETWORK_QOS);
        }
    }
    
	if(ret == 0)
    {
#ifdef IPV6_MODE
        if(pCR->bIPV6)
        {
            statistic_v6.ssw.n_response_code_out[nRespCode] ++;
            statistic_v6.ssw.n_response_code_out_sum[pCR->ot_type] ++;
        }
        else
#endif
        {
            statistic.ssw.n_response_code_out[nRespCode] ++;
            statistic.ssw.n_response_code_out_sum[pCR->ot_type] ++;
        }
    }
    else
    {
        Log.printf(LOG_ERR, "[RSSW] RSSW_SendResponse(%d) errno=%d[%s]!!!\n", ret, errno, strerror(errno));
    }
    
    // FIXIT: Trace...
    //	TraceMsg(nRespCode, strCscfIp, nCscfPort);
    // MPBX에서는 REGISTER 사용 안함
#ifdef INCLUDE_REGI
    if((C_LOG_REGISTER == false) && (pCR->nSipMsg_ID == METHOD_REGISTER))   /* no printout */ ;
    else
#endif
    {
        if (C_LOG_LEVEL == LOG_LV1)
        {
            Log.color(COLOR_YELLOW, LOG_LV1, "[RSSW] >>>--->>> Send Message(%d) RESPONSE [%d] To=[%s:%d] >>>--->>>\n%s\n", nSendLen, nRespCode, strCscfIp, nCscfPort, strSendBuf);
        }
        else
        {
            Log.color(COLOR_YELLOW, LOG_LV2, "[RSSW] >>>--->>> Send Message(%d) RESPONSE [%d] To=[%s:%d] Call-OD=[%s] >>>--->>>\n", nSendLen, nRespCode, strCscfIp, nCscfPort, pCR->info.strCall_ID);
        }
    }
    return(true);
}

////#pragma mark -
//#pragma mark CSCF에서 수신한 메세지를 SAM으로 전송하기 위해 메세지를 분석/추가/삭제 하는 함수 들

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSSW_GetServiceKeyAndDPFromRouteHeader
 * CLASS-NAME     :
 * PARAMETER    IN: strRoute       - Route Header field 문자 열
 *             OUT: strServiceKey  - strRoute에 있는 Service Key 정보
 *              IN: MAX_SVCKEY_LEN - strServiceKey 버퍼의 최대 길이
 *             OUT: strDP          - strRoute에 있는 orig/term 정보
 * RET. VALUE     : -
 * DESCRIPTION    : Route header에서 ServiceKey와 DP를 구하는 함수
 * REMARKS        : <sip:SvcKey@domain;lr;orig/term>
 **end*******************************************************/
int RSSW_GetServiceKeyAndDPFromRouteHeader(char *strRoute, char *strServiceKey, int MAX_SVCKEY_LEN, char *strDP)
{
        // 20160614 mPBX BMT
    // mpbx psi trigger는 service_key는 mpbx로 올라오지만, dp정보(orig/term은 없다)
    
#if 0   // original source
    // Route header에 DP가 있는지 확인...
    if     (strstr(strRoute, ";orig")) { strcpy(strDP, "orig"); }
    else if(strstr(strRoute, ";term")) { strcpy(strDP, "term"); }
    else                               { return(false); }
    
    // "<sip:ServiceKey@domain;lr;DP>"에서 ServiceKey를 구함
    if(!LIB_GetStrFromAToB(strRoute, "sip:", "@", strServiceKey, MAX_SVCKEY_LEN)) { return(false); }
    
#else   // psi trigger 반영 소스 20160614
    if     (strstr(strRoute, ";orig")) { strcpy(strDP, "orig"); }
    else if(strstr(strRoute, ";term")) { strcpy(strDP, "term"); }
    else
    {
        char    strTemp[128];
        
        // PSI의 경우 servce_key.cfg 파일에서 Name있는지 확인 후 Key 값으로 올린다. (검색은 Name, 올리는 건 Key... sungho 요청 
        if(!LIB_GetStrFromAToB(strRoute, "sip:", "@", strTemp, MAX_SVCKEY_LEN)) { return(false); }
 
        if(g_SvcKey.get_service_key(strTemp, strServiceKey) == true)
        {
            sprintf(strDP, "term");   // PSI Trigger인 경우 term으로 하기로 SRM과 협의 됨(추후 변경가능성 있음)
            return(true);
        }
        
        /*
        if(g_SvcKey.is_service_key(strServiceKey) == true)
        {
            sprintf(strDP, "term");   // PSI Trigger인 경우 term으로 하기로 SRM과 협의 됨(추후 변경가능성 있음)
            return(true);
        }*/
        return(false);
    }
    
    // "<sip:ServiceKey@domain;lr;DP>"에서 ServiceKey를 구함 (기존 orig/term이 있는 경우)strDP
    if(!LIB_GetStrFromAToB(strRoute, "sip:", "@", strServiceKey, MAX_SVCKEY_LEN)) { return(false); }
#endif

    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSSW_GetFromToUserID
 * CLASS-NAME     : -
 * PARAMETER INOUT: pCR - Call Register 구조체 포인터
 * RET. VALUE     : BOOL
 * DESCRIPTION    : RURI나 From 에서 SIP메세지를 전송한 User-Id를 구하는 함수
 * REMARKS        : User_ID는 일반적으로는 From, 긴급호의 경우에만 예외적으로 RURI에서 구함
 **end*******************************************************/
bool RSSW_GetFromToUserID(CR_DATA *pCR)
{
	char        strTemp[1024];
    URI_TYPE    nUriType;
    
    /*
     * Call-ID: 이 함수 들어오기전에 이미 추출해서 pCR->info.strCall_ID에 저장되어 있음
     */
    
    /*
     * From: Header에서 주소 부분만 추출
     */
    if(LIB_GetStrFromAToB(pCR->strRcvBuf, "\r\nFrom:", ASCII_CRLF, strTemp, sizeof(strTemp)) == false)
    {
        Log.printf(LOG_ERR, "RSSW_GetFromToUserID() there's no From: header\n");
        return(false);
    }

    if(LIB_GetUriFromHeader(strTemp, &nUriType, pCR->info.strFrom, sizeof(pCR->info.strFrom)) == false)
    {
        Log.printf(LOG_ERR, "RSSW_GetFromToUserID() From: Header Error: there's no tel: or sip:\n");
        return(false);
    }
    
    /*
     * To: Header에서 주소 부분만 추출
     */
    if(LIB_GetStrFromAToB(pCR->strRcvBuf, "\r\nTo:", ASCII_CRLF, strTemp, sizeof(strTemp)) == false)
    {
        Log.printf(LOG_ERR, "RSSW_GetFromToUserID() there's no To: header\n");
        return(false);
    }
    
    if(LIB_GetUriFromHeader(strTemp, &nUriType, pCR->info.strTo, sizeof(pCR->info.strTo)) == false)
    {
        Log.printf(LOG_ERR, "RSSW_GetFromToUserID() To: Header Error: there's no tel: or sip:\n");
        return(false);
    }
    
#if 0       // 20160908 - strUser_ID 사용안하게 변경
    /*
     * UserID 추출 - 일반적으로 From과 동일, 긴급호는 RURI에서 추출
     */
    // FIXME: - User_ID 이거 실제로 SCM에서 사용되나 ?
    if((pCR->nSipMsg_ID == METHOD_INVITE) && (pCR->info.strDP[0] == 't'))    // INVITE 이면서 DP=term... 긴급호
    {
        if(LIB_GetStrFromAToB(pCR->strRcvBuf, "sip:", " SIP/2.0\r\n", pCR->info.strUser_ID, sizeof(pCR->info.strUser_ID)))
        {
            LIB_DeleteStartSpace(pCR->info.strUser_ID); // 시작부분 SPACE 삭제
            return(true);
        }
        else if(LIB_GetStrFromAToB(pCR->strRcvBuf, "tel:", " SIP/2.0\r\n", pCR->info.strUser_ID, sizeof(pCR->info.strUser_ID)))
        {
            LIB_DeleteStartSpace(pCR->info.strUser_ID); // 시작부분 SPACE 삭제
            return(true);
        }
        else
        {
            // RURI에 'sip:' or 'tel:'이 없음
            Log.printf(LOG_ERR, "RSSW_GetFromToUserID() RURI - there's not tel: neither sip:\n");
            return(false);
        }
    }
    else
    {
        strncpy(pCR->info.strUser_ID, pCR->info.strFrom, sizeof(pCR->info.strUser_ID)-1);    // 일반적으로 From과 동일
    }
#endif
    
    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSSW_GetServiceKeyInMessage
 * CLASS-NAME     : -
 * PARAMETER INOUT: pCR - Call Register 구조체 포인터
 * RET. VALUE     : BOOL
 * DESCRIPTION    : SIP 메세지에서 ServiceKey를 구하는 함수
 * REMARKS        : ServiceKey는 Route header에 들어 있거나 환경파일에 설정되어 있음
 *                : case 1) first route에 ServiceKey와 DP를 갖는 경우 (일반 호)
 *                :         Route: <sip:KTCard1@222.106.149.34;orig;lr>,<sip:1910@125.152.96.20;OdiPsi=KTCard1;lr>
 *                :         Route: <sip:V_MMAD@ascs02-grp.octave.com;lr;orig>, <sip:125.152.0.163:5070;OdiPsi=V_MMAD;orig;lr>
 *                : case 2) Request URI에 ServiceKey를 갖는 경우 (REGISTER Method only) => arraySvcKeyList에서 확인, DP는 "term"으로 Fix
 *                :         REGISTER sip:ascs01.octave.com;lr;v_mtrs SIP/2.0
 *                : case 3) ServiceKey가 없는 경우(긴급호) => arraySvcKeyList에서 확인, DP는 "term"으로 Fix
 *                :         INVITE tel:119 SIP/2.0
 *                : 1), 2), 3)의 case가 아니면 Error....
 **end*******************************************************/
bool RSSW_GetServiceKeyInMessage(CR_DATA *pCR)
{
    char    strRoute[512];      // 20170112 : 256 -> 512 로 변경 (Route가 긴 경우가 있음 - ruri등이 포함됨)
    char    strRURI[128];
    
    // Initial Message가 아니면 ServiveKey  DP 없음

    switch(pCR->nSipMsg_ID)
    {
        case METHOD_INVITE:
        case METHOD_MESSAGE:
        case METHOD_SUBSCRIBE:
        case METHOD_REFER:
        case METHOD_REGISTER:  break;
        default:               return(false);
    }

    // Route: Header 가 있는 경우 case 1)
    if(LIB_GetSipHeaderValue(pCR->strRcvBuf, "\r\nRoute:", strRoute, sizeof(strRoute)))
    {
        if(RSSW_GetServiceKeyAndDPFromRouteHeader(strRoute, pCR->info.strServiceKey, sizeof(pCR->info.strServiceKey), pCR->info.strDP))
        {
            Log.printf(LOG_LV1, "RSSW_GetServiceKeyInMessage(#1) ServiceKey=%s, DP=%s\n", pCR->info.strServiceKey, pCR->info.strDP);
            return(true);
        }
    }

    // Route가 없거나 Route에 ServiceKey & DP 가 없는 경우 ServiceList에서 찾음 case 2), case 3)
    // case 2)의 경우 service.cfg에 ServiceKey를 찾아서 동일한 ServiceKey로 변환되게 설정되어 있음, ex) mmtel이 포함되어 있으면 mmtel ServiceKey로 변환
    // case 3)의 경우 service.cfg에 URI를 찾아서 ServiceKey로 변환되게 설정되어 있음, ex) tel:119가 포함되어 있으면 mmtel ServiceKey로 변혼
    // 결과적으로 case 2), case 3)는  ruri에 service key가 있던 없던 특정 문자가 있으면 특정 서비스키로 변환되므로 service.cfg 파일을 잘 작성하면 동일한 내용이 된다.
    if(!LIB_GetStrFromAToB(pCR->strRcvBuf, "", "SIP/2.0", strRURI, sizeof(strRURI))) { return(false); }

    if(g_SvcKey.get_service_key(strRURI, pCR->info.strServiceKey) == true)
    {
        snprintf(pCR->info.strDP, sizeof(pCR->info.strDP), "term");   // FIXIT; why term ??
        Log.printf(LOG_LV1, "RSSW_GetServiceKeyInMessage(#2) ServiceKey=%s, DP=%s\n", pCR->info.strServiceKey, pCR->info.strDP);
        return(true);
    }

    return(false);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSSW_GetRoutingInfoFromHashTable
 * CLASS-NAME     : -
 * PARAMETER INOUT: pCR - Call Register 구조체 포인터
 * RET. VALUE     : BOOL
 * DESCRIPTION    : HashTable 에서 SAM으로 전송하는데 필요한 데이터를 추출하는 함수
 * REMARKS        : FIXIT - HashTable에 있는 데이터를 여기서 이용할 필요가 있을까?
 *                :         없는 경우에 Insert를 하고(나중에 쓰기 위해) 있으면 그냥 넘어가는 것이 어떤지??
 **end*******************************************************/
bool RSSW_GetRoutingInfoFromHashTable(CR_DATA *pCR)
{
    HASH_ITEM   hashTable;

    if(pCR->ot_type == OT_TYPE_ORIG)     // ORIG Stack에서 수신한 메세지 임
    {
        if(g_Hash_O.find_hash(pCR->info.strCall_ID, &hashTable))
        {
            pCR->info.bExistInHashTable = true;
            
            pCR->bd          = hashTable.BoardNo;
            pCR->ch          = hashTable.ChannelNo;
            pCR->info.nTrace = hashTable.nTrace;
#ifdef IPV6_MODE
            pCR->bIPV6       = hashTable.bIPv6;
#endif
#ifdef SES_HA_MODE
#ifdef SES_ACTSTD_MODE /* 20241126 kdh add */
#else
            pCR->bIsMoved      = hashTable.bIsMoved;
            pCR->moved_bd      = hashTable.PeerBoardNo;

            // 다음은 ORIG인  경우만 해당함 (이중화에서 ACK를 받았는지 아닌지를 구분하기 위해서)
            // INVITE/CANCEL/4xx 에 대한 ACK등을 구분해야 하나 INVITE 외에는 어짜피  SRM->SCM->SPY 채널이 해제될 것이기에 다 ACK-INVITE라 생각하고 구현

            if((pCR->nSipMsg_ID == METHOD_ACK) && (hashTable.bIsReceiveACK == false))
            {
                int ret;
                
                hashTable.bIsReceiveACK = true;
                ret = g_Hash_O.ha_update_ack(pCR->info.strCall_ID, true);     // bIsReceiveACK update
                
                HASH_SendHaUpdateRQ(pCR, true);     // sync active-standby
            }
            
            pCR->bIsReceiveACK = hashTable.bIsReceiveACK;
#endif /* SES_ACTSTD_MODE */
#endif /* SES_HA_MODE */
//            strcpy(pCR->info.strCscfIp, hashTable.DestIp);
//            pCR->info.nCscfPort = hashTable.DestPort;
#ifdef DEBUG_MODE
            Log.color(COLOR_MAGENTA, LOG_LV1, "[O] %s() g_Hash_O.find_hash OK !!! (call-ID=%s\n", __FUNCTION__, pCR->info.strCall_ID);
#endif
            return(true);
        }
        else
        {
            if(g_ScmMap.find(pCR->info.strCall_ID) == true)     // SCM에 이미 할당 요청 중 (재전송)
            {
                Log.printf(LOG_LV1, "[RSSW] retransmission call-ID=%s\n", pCR->info.strCall_ID);
                pCR->info.bRetransmission = true;
                return(true);
            }

            pCR->info.bExistInHashTable = false;

#ifdef DEBUG_MODE
            Log.color(COLOR_MAGENTA, LOG_LV1, "[O] ------- RSSW_GetRoutingInfoFromHashTable() g_Hash_O.find_hash fail (call-ID=%s)\n", pCR->info.strCall_ID);
#endif
            // ORIG STACK에서 Initial Message인 경우에는 hash Table에 없음 - 이경우는 ServiceKey를 구할 수 있음 - 정상 case
            if((pCR->nSipMsgType == SIP_TYPE_REQUEST) && (RSSW_GetServiceKeyInMessage(pCR) == true))
            {
                pCR->info.bInitialMessage = true;
                return(true);
            }

            // Service Key 없음... Hash 없음 - 이 경우 - FXIIT:
            // SPY가 다운되었다 살아난 경우...
            return(false);
        }
    }
    else
    {
        if (g_Hash_T.find_hash(pCR->info.strCall_ID, &hashTable))
        {
            pCR->info.bExistInHashTable = true;
            
            pCR->bd          = hashTable.BoardNo;
            pCR->ch          = hashTable.ChannelNo;
            pCR->info.nTrace = hashTable.nTrace;
#ifdef IPV6_MODE
            pCR->bIPV6       = hashTable.bIPv6;
#endif
#ifdef SES_HA_MODE
#ifdef SES_ACTSTD_MODE /* 20241126 kdh add */
#else
            pCR->bIsMoved    = hashTable.bIsMoved;
            pCR->moved_bd    = hashTable.PeerBoardNo;
#endif /* SES_ACTSTD_MODE */
#endif /* SES_HA_MODE */
//            strcpy(pCR->info.strCscfIp, hashTable.DestIp);
//            pCR->info.nCscfPort = hashTable.DestPort;
            return(true);
        }
        else
        {
            pCR->info.bExistInHashTable = false;
            // FIXIT - Hash 구조 변경 후 구현할 것
            // g_HashTable->Insert()
            // TERM STACK에서 Hash Table이 없다면 이건  HashTable에 문제가 생긴 경우 임
            // SPY가 다운되었다 살아난 경우...
        }
    }
    
    return(false);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSSW_GetBoardChannelFromTagOrCallId
 * CLASS-NAME     : -
 * PARAMETER INOUT: pCR - Call Register 구조체 포인터
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 수신한 SIP 메세지에서 board/channel을 구하는 함수
 * REMARKS        : ORIG인 경우 Tag 정보에서 TERM인 경우 Call-ID에서 Board/Channel을 가져 올수 있다.
 **end*******************************************************/
bool RSSW_GetBoardChannelFromTagOrCallId(CR_DATA *pCR)
{
    char    strTemp[1024];
    char    *ptrBdCh;
    int     board = -1, channel = -1;
    
    if(pCR->ot_type == OT_TYPE_ORIG)    // ORIG는 TAG에 board/channel 정보가 있다 (From or To)
    {
        // To: Header 에서 Tag 검색
        if(LIB_GetSipHeaderValue(pCR->strRcvBuf, "\r\nTo:", strTemp, sizeof(strTemp)))
        {
            if((ptrBdCh = strstr(strTemp, "ObC")) != NULL)
            {
                sscanf(ptrBdCh, "ObC-%x-%d", &board, &channel);
                if((board != -1) && (channel != -1)) { pCR->bd = board; pCR->ch = channel; return(true); }
                else                                 { return(false); }
            }
        }
        
        // From: Header 에서 Tag 검색
        if(LIB_GetSipHeaderValue(pCR->strRcvBuf, "\r\nFrom:", strTemp, sizeof(strTemp)))
        {
            if((ptrBdCh = strstr(strTemp, "ObC")) != NULL)
            {
                sscanf(ptrBdCh, "ObC-%x-%d", &board, &channel);
                if((board != -1) && (channel != -1)) { pCR->bd = board; pCR->ch = channel; return(true); }
                else                                 { return(false); }
            }
        }
    }
    else        // TERM은 Call-ID에 board/channel 정보가 있다.
    {
        // Call-ID: Header 에서 Tag 검색
        if(LIB_GetSipHeaderValue(pCR->strRcvBuf, "\r\nCall-ID:", strTemp, sizeof(strTemp)))
        {
            if((ptrBdCh = strstr(strTemp, "TbC")) != NULL)
            {
                sscanf(ptrBdCh, "TbC-%x-%d", &board, &channel);
                if((board != -1) && (channel != -1)) { pCR->bd = board; pCR->ch = channel; return(true); }
                else                                 { return(false); }
            }
        }
    }
    return(false);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSSW_ReceivedSipParsing
 * CLASS-NAME     : -
 * PARAMETER INOUT: pCR - Call Register 구조체 포인터
 * RET. VALUE     : BOOL
 * DESCRIPTION    : CSCF(또는 MRF...)에서 수신한 SIP 메세지를 분석하느 함수
 * REMARKS        : Receive SIP from SAM
 **end*******************************************************/
bool RSSW_ReceivedSipParsing(CR_DATA *pCR)
{
    // CSeq와 Call-ID는 이전에 이미 구했음
    // From/To Header - 반드시 있어야 함
    if((RSSW_GetFromToUserID(pCR)) == false)
    {
        Log.printf(LOG_ERR, "RSSW_ReceivedSipParsing() Invalid SIP message Call-ID=%s !!\n", pCR->info.strCall_ID);
        g_ErrSvrList.add_msgerr(pCR->info.strCscfIp);   // ERROR 메세지를 보낸 CSCF로 등록
        return(false);
    }
    
    // TODO: more SIP Syntax check here.....
    
    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSSW_GetRoutingInfo
 * CLASS-NAME     : -
 * PARAMETER INOUT: pCR - Call Register 구조체 포인터
 * RET. VALUE     : BOOL
 * DESCRIPTION    : CSCF(또는 MRF...)에서 수신한 SIP 메세지를 분석하느 함수
 * REMARKS        : Receive SIP from SAM
 **end*******************************************************/
bool RSSW_GetRoutingInfo(CR_DATA *pCR)
{

    // Hash Table에서 SAM 정보를 가져온다.
    // 이때 Initial Message들은 Hash에 정보가 없다. (또한 SPY Down이후에도 HashTable에 정보가 없을 수 있다.)
    if(RSSW_GetRoutingInfoFromHashTable(pCR) == false)
    {
        if(C_HASH_FAIL_ROUTE == false) { return(false); }       // Hash에 없으면 복구하지 않고 버린다... (버리는게 더 좋은 경우도 있음.. 재전송 방지 등)
        
        // Hash Table이 없는 경우에 ORIG인 경우 Tag 정보에서 TERM인 경우 Call-ID에서 Board/Channel을 가져 올수 있다.
        if(RSSW_GetBoardChannelFromTagOrCallId(pCR) == false)
        {
            // ACK 는 SPY에서 채널할당 실패나 AS블록으로 INVITE에 대한 503/500등을 보냈을때도 올 수 있다.
            // 이경우 Hash에 없는 것이 당연하며 SPY에서 CallLeg이 바로 종료가 된다.
//            if(pCR->nSipMsg_ID != METHOD_ACK)
//            {
//                Log.printf(LOG_ERR, "RSSW_GetRoutingInfo() Can't Get Routing Info from HashTable !!\n");
//            }
            
            return(false);
        }
        else
        {
            // mPBX BMT 과정에서 이경우는 100% BYE에 대해서만 발생했는데.... sipp에  BYE 재전송으로 잡힌다. (원인은 누군가 4,195초간격으로 일부 Hash를 삭제함 - 범인미정)
            // 그래서 여기서 BYE에 대한 200을 보내고 SAM으로 BYE를 보냐면 200 재전송이 찍힐 수 있는데... BYE 재전송보다는 나은지 확인 필요
            if((C_BYE_NO_HASH_200 == true) && (pCR->nSipMsg_ID == METHOD_BYE))
            {
                RSSW_SendResponse(pCR, 200, NULL);      // BYE에 대한 200도 보내고 SAM에 BYE도 보낸다.. 혹 SAM 채널이 불복될지도...
            }

            Log.color(COLOR_CYAN, LOG_LV2, "RSSW_GetRoutingInfo() RSSW_GetBoardChannelFromTagOrCallId() bd=%d, ch=%d, msg=%d %s !!\n", pCR->bd, pCR->ch, pCR->nSipMsg_ID,pCR->info.strCSeq); // - 절체시 너무 많다. 500cps로 쏘면 수천개 (재전송 포함)
        }
    }
    
    if(pCR->info.bRetransmission == true) { return(true); }
    
    if(C_AS_BLOCK == true)
    {
        if(pCR->info.bInitialMessage == true)
        {
            Log.color(COLOR_MAGENTA, LOG_INF, "RSSW_GetRoutingInfo() receive Initial Message but AS BLOCK Call-ID: %s!!\n", pCR->info.strCall_ID);
            RSSW_SendResponse(pCR, 480, "SIP;cause=415;text=\"AS BLOCKED\"");
            return(false);
        }
        else
        {
            // Block이라도 기존 진행하던 call은 처리 함
            Log.printf(LOG_LV1, "RSSW_GetRoutingInfo() AS BLOCK - but continue received non initial message \n");
        }
    }

    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSSW_Change_Content_Type
 * CLASS-NAME     : -
 * PARAMETER INOUT: pCR        - Call Register 구조체 포인터
 *             OUT: strSendBuf - 보낼 SIP 메세지
 * RET. VALUE     : BOOL
 * DESCRIPTION    : SAM으로 전송할 메세지에 Contet-Type Header를 수정하는 함수
 * REMARKS        : application/3gpp-ims+xml을 수정 add 170117
 **end*******************************************************/
bool RSSW_Change_Content_Type(CR_DATA *pCR, char *strSendBuf)
{
    char    *pStart, *pCRLF, *pWrite;
    
    if((pStart = strstr(strSendBuf, "\r\nContent-Type:")) != NULL)
    {
        pStart += 15;  // 15 = strlen("\r\nContent-Type:")
        
        if((pCRLF = strstr(pStart, "\r\n")) == NULL) { return(false); }     // ERROR
        
        *pCRLF = '\0';          // set NULL
        
        if((pWrite = strstr(pStart, "3gpp-ims+xml")) != NULL)
        {
            pWrite[8] = '-';    // change '+' -> '-' , "3gpp-ims+xml" -> "3gpp-ims-xml"
            *pCRLF = '\r';      // restore
            return(true);
        }
        
        *pCRLF = '\r';          // restore
    }
    return(false);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSSW_Insert_P_BT_ChInfo
 * CLASS-NAME     : -
 * PARAMETER INOUT: pCR        - Call Register 구조체 포인터
 *             OUT: strSendBuf - 보낼 SIP 메세지
 * RET. VALUE     : BOOL
 * DESCRIPTION    : SAM으로 전송할 메세지에 P-BT-ChInfo Header를 추가하는 함수
 * REMARKS        : nCall-ID 앞에 추가
 **end*******************************************************/
bool RSSW_Insert_P_BT_ChInfo(CR_DATA *pCR, char *strSendBuf)
{
    char    *ptrCallID;
    char    *ptrWrite;
    char    strTempBuf[MAX_BUF_SIZE+1];
	char	strTempBtChinfo[512];		// [JIRA AS-206] 
	int		nAddLen = 0;				// [JIRA AS-206] 
    
    if((ptrCallID = strcasestr(strSendBuf, "\r\nCall-ID:")) == NULL) { return(false); }
    
    strcpy(strTempBuf, ptrCallID);      // Call-ID 이후 부분 Copy
    
    ptrWrite  = ptrCallID;              

	// [JIRA AS-206] START
	nAddLen = sprintf(strTempBtChinfo, "\r\nP-BT-ChInfo: board=%d;channel=%d;stack=%s;route=%s:%d",
                        pCR->bd, pCR->ch, (pCR->ot_type == OT_TYPE_ORIG) ? "ORIG": "TERM",
                        pCR->info.strCscfIp, pCR->info.nCscfPort);

	if(strlen(strSendBuf) + nAddLen >= SAFETY_MAX_BUF_SIZE) {
		Log.printf(LOG_ERR, "RSSW_Insert_P_BT_ChInfo() P_BT_chInfo+RcvBuf Size(%d) >= MAX_BUF_SIZE(%d) !!!\n", nAddLen+strlen(strSendBuf), SAFETY_MAX_BUF_SIZE);
        return(false);
	}
	
	sprintf(ptrWrite, "%s%s", strTempBtChinfo, strTempBuf);
	// [JIRA AS-206] END

    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSSW_Insert_P_BT_ServiceKey
 * CLASS-NAME     : -
 * PARAMETER INOUT: pCR        - Call Register 구조체 포인터
 *             OUT: strSendBuf - 보낼 SIP 메세지
 * RET. VALUE     : BOOL
 * DESCRIPTION    : SAM으로 전송할 메세지에 P-BT-ServiceKey Header를 추가하는 함수
 * REMARKS        : nCall-ID 앞에 추가
 **end*******************************************************/
bool RSSW_Insert_P_BT_ServiceKey(CR_DATA *pCR, char *strSendBuf)
{
    char    *ptrCallID;
    char    *ptrWrite;
    char    strTempBuf[MAX_BUF_SIZE+1];
	char	strTempPbtServiceKey[128];	// [JIRA AS-206]
	int		nAddLen = 0;				// [JIRA AS-206]
    
    if((ptrCallID = strcasestr(strSendBuf, "\r\nCall-ID:")) == NULL) { return(false); }
    
    strcpy(strTempBuf, ptrCallID);      // Call-ID 이후 부분 Copy
    
    ptrWrite  = ptrCallID;              // Call-ID 이전까지는 그대로 두고 Call-ID 앞에 P-BT-ServiceKey를 INSERT한다.
    
	// [JIRA AS-206] START
#ifdef IPV6_MODE
    if(pCR->bIPV6)
    {
        nAddLen = sprintf(strTempPbtServiceKey, "\r\nP-BT-ServiceKey: %s;%s%s;IPv6", pCR->info.strServiceKey, pCR->info.strDP, (pCR->info.nTrace)?";TrAcE":""); // [JIRA AS-206]
    }
    else
    {
        nAddLen = sprintf(strTempPbtServiceKey, "\r\nP-BT-ServiceKey: %s;%s%s",      pCR->info.strServiceKey, pCR->info.strDP, (pCR->info.nTrace)?";TrAcE":""); // [JIRA AS-206]
    }
#else
    nAddLen = sprintf(strTempPbtServiceKey, "\r\nP-BT-ServiceKey: %s;%s%s", pCR->info.strServiceKey, pCR->info.strDP, (pCR->info.nTrace)?";TrAcE":""); // [JIRA AS-206]
#endif
	
	if(strlen(strSendBuf) + nAddLen >= SAFETY_MAX_BUF_SIZE) { // [JIRA AS-206] - 20170719
		Log.printf(LOG_ERR, "RSSW_Insert_P_BT_ServiceKey() P-BT-ServiceKey+RcvBuf Size(%d) >= MAX_BUF_SIZE(%d) !!!\n", nAddLen+strlen(strSendBuf), SAFETY_MAX_BUF_SIZE);
        return(false);
	}
	
	sprintf(ptrWrite, "%s%s", strTempPbtServiceKey, strTempBuf);
	// [JIRA AS-206] END
    
    return(true);
}

extern int TraceMsg();


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSSW_AddSpyViaHeader
 * CLASS-NAME     :
 * PARAMETER INOUT: pCR    - 수신한 SIP 메세지 구조체 포인터
 *             OUT: strSendBuf - 보낼 SIP 메세지
 * RET. VALUE     : BOOL
 * DESCRIPTION    : SAM으로 보낼 메세지에 SPY VIA를 추가하는 함수
 * REMARKS        : SIP Request는 자신의 Via를 추가해서 보낸다.
 *                : 이때 branch는 수신된 First Via branch를 이용하여 생성한다.
 *                : ex) SPY branch = first_via_branch-bd00-ch00000
 **end*******************************************************/
bool RSSW_AddSpyViaHeader(CR_DATA *pCR, char *strSendBuf)
{
    char    *ptrFirstVia, *ptrValue, *ptrBranch;
    char    *ptrCRLF, *ptrSemicolon, *ptrComma;
    char    *ptrWrite;
    char    strFirstViaBranch[128];
    char    strTempBuf[MAX_BUF_SIZE+1];
    int     nAddLen = 0; // [JIRA AS-206]  

    /*
     * 첫번째 Via의 Branch를 구한다.
     */
    if((ptrFirstVia = strcasestr(strSendBuf, "\r\nVia:")) == NULL)    // Via 없음 - 에러처리 (RV 버그 아니면 발생안됨)
    {
        Log.printf(LOG_ERR, "RSSW_AddSpyViaHeader() Can't find Via: header Method=[%d] !!!\n", pCR->nSipMsg_ID);
        return(false);
    }
    
    ptrValue = ptrFirstVia + 6;   // Header size strlen("\r\nVia:") 만큼 포인터 이동
    
    if((ptrCRLF = strstr(ptrValue, "\r\n")) == NULL)                // Via 끝에 CRLF 없음 - 에러처리 (발생안됨)
    {
        Log.printf(LOG_ERR, "RSSW_AddSpyViaHeader() Can't find CRLF of Via: header !!!\n");
        return(false);
    }
    
    *ptrCRLF = '\0';    // set NULL - Via 내용의 마지막을 NULL로 설정해서 문자열 처리를 쉽게 한다.
    {
        // 한줄에 여러개의 Via가 있는 경우 첫번째 Via만 처리하기 위해서 NULL 처리 (add 151124 by SMCHO)
        if((ptrComma = strchr(ptrValue, ',')) != NULL) { *ptrComma = '\0'; }
        
        if((ptrBranch = strstr(ptrValue, "branch=")) == NULL)
        {
            Log.printf(LOG_ERR, "RSSW_AddSpyViaHeader() Can't find branch in Via: header !!!\n");
            *ptrCRLF = '\r';                    // restore
            return(false);                      // Via branch가 없음 - 에러처리 (발생안됨)
        }
        
        bzero(strFirstViaBranch, sizeof(strFirstViaBranch));
        ptrBranch += 7;                         // +7("branch=") 다음으로 포인터 이동
        if((ptrSemicolon = strchr(ptrBranch, ';')) != 0)    // branch=다음에 ';'세미콜론이 있는 경우
        {
            *ptrSemicolon = '\0';                       // set NULL for copy
            strncpy(strFirstViaBranch, ptrBranch, sizeof(strFirstViaBranch)-1);       // branch= 다음부터 ';' 앞까지 복사
            *ptrSemicolon = ';';                        // restore
        }
        else                                                // branch=다음에 ';'세미콜론이 없는 경우
        {
            strncpy(strFirstViaBranch, ptrBranch, sizeof(strFirstViaBranch)-1);    // branch= 다음부터CRLF 앞까지 복사
        }
        
        if(ptrComma != NULL) { *ptrComma = ','; }   // restore multi-via (151124)
    }
    *ptrCRLF = '\r';    // restore
    
    /*
     * First Via의 Branch를 이용하여 SPY Via를 만들어서 SendBuf의 Via: 앞에 추가한다.
     * Via: SIP/2.0/UDP SPY_IP:PORT;branch=first_via_branch-bd000-ch00000 형태
     */
    strcpy(strTempBuf, ptrFirstVia);        // First Via 이후 부분을 Temp로 복사(SPY Via를 INSERT 하기 위해서)
    
    ptrWrite = ptrFirstVia;                 // First Via 이전까지는 그대로 두고 First Via 앞에 SPY Via를 INSERT한다.
    
	// [JIRA AS-206] START
    if(pCR->info.bCSRN == true)             // CSRN인 경우 Via Header앞에 CSRN 정보 Set
    {
#ifdef IPV6_MODE
        if(pCR->bIPV6)
        {
            nAddLen = sprintf(ptrWrite, "\r\nP-Csrn-Info: 1\r\nVia: SIP/2.0/UDP [%s]:%d;branch=%s-bd%03d-ch%05d", 
				pCR->info.strMyStackIp, C_SAM_PORT, strFirstViaBranch, pCR->bd, pCR->ch);
			//	pCR->info.strMyStackIp, SAM_UDP_SERVER_PORT, strFirstViaBranch, pCR->bd, pCR->ch);
        }
        else
#endif
        {
            nAddLen = sprintf(ptrWrite, "\r\nP-Csrn-Info: 1\r\nVia: SIP/2.0/UDP %s:%d;branch=%s-bd%03d-ch%05d", 
				pCR->info.strMyStackIp, C_SAM_PORT, strFirstViaBranch, pCR->bd, pCR->ch);
			//	pCR->info.strMyStackIp, SAM_UDP_SERVER_PORT, strFirstViaBranch, pCR->bd, pCR->ch);
        }
    }
    else
    {
#ifdef IPV6_MODE
        if(pCR->bIPV6)
        {
            nAddLen = sprintf(ptrWrite, "\r\nVia: SIP/2.0/UDP [%s]:%d;branch=%s-bd%03d-ch%05d", 
				pCR->info.strMyStackIp, C_SAM_PORT, strFirstViaBranch, pCR->bd, pCR->ch);
			//	pCR->info.strMyStackIp, SAM_UDP_SERVER_PORT, strFirstViaBranch, pCR->bd, pCR->ch);
        }
        else
#endif
        {
            nAddLen = sprintf(ptrWrite, "\r\nVia: SIP/2.0/UDP %s:%d;branch=%s-bd%03d-ch%05d", 
				pCR->info.strMyStackIp, C_SAM_PORT, strFirstViaBranch, pCR->bd, pCR->ch);
			//	pCR->info.strMyStackIp, SAM_UDP_SERVER_PORT, strFirstViaBranch, pCR->bd, pCR->ch);
        }
    }
    ptrWrite += nAddLen;           

    if((nAddLen + pCR->nRcvLen) >= SAFETY_MAX_BUF_SIZE) // [JIRA AS-206] - 20170719
    {
        Log.printf(LOG_ERR, "RSSW_AddSpyViaHeader() Via+RcvBuf Size(%d) >= MAX_BUF_SIZE(%d) !!!\n", nAddLen+pCR->nRcvLen, SAFETY_MAX_BUF_SIZE);
        return(false);
    }
	// [JIRA AS-206] END
    
    strcpy(ptrWrite, strTempBuf);           // First Via 이후 부분을 Append
    //Log.color(COLOR_CYAN, LOG_LV1, "RSSW_AddSpyViaHeader(2) strSendBuf=\n%s\n", strSendBuf);
    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : is_DialInPrefix
 * CLASS-NAME     :
 * PARAMETER INOUT: pCR - 수신한 SIP 메세지 구조체 포인터
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 수신된 RURI가 DIAL_IN_PREFIX 인지 확인하는 함수
 * REMARKS        : DIAL_IN인 Initial INVITE인 경우 채널할당을 하지 않고
 *                : RURI를 parsing하여 board, channel 번호를 구한다.
 *                : channel release를 고려하여 hash 를 update 해야함... (이중화 등등.... FIXIT)
 **end*******************************************************/
bool is_DialInPrefix(CR_DATA *pCR)
{
    char    strRURI[256];
    char    *ptr;
    int     bd = -1, ch = -1;
    
    if(!LIB_GetStrFromAToB(pCR->strRcvBuf, "INVITE", "SIP/2.0", strRURI, sizeof(strRURI))) { return(false); }
    LIB_delete_white_space(strRURI);
//Log.color(COLOR_RED, LOG_LV1, "%s() #1 RURI[%s] = DIAL_IN_PREFIX\n", __FUNCTION__, strRURI);

    if((strncmp("tel:", strRURI, 4) != 0) && (strncmp("sip:", strRURI, 4) != 0)) { return(false); }
    
    // 특정(@, ;, :) 문자 이후 삭제
    if((ptr = strchr(strRURI, '@')) != NULL) { *ptr = '\0'; }
    if((ptr = strchr(strRURI, ';')) != NULL) { *ptr = '\0'; }
    if((ptr = strchr(strRURI, ':')) != NULL) { *ptr = '\0'; }
    
    ptr = &strRURI[4];      // skip 'tel:' or 'sip:'
    if(strncmp("+82", ptr, 3) == 0) { ptr += 3; }   // skip '+82'
    
    // RURI = DIAL_IN_PFX+BD(2자리)+CH(5자리)
    if(strncmp(ptr, C_DIAL_IN_PREFIX, C_DIAL_IN_PREFIX_LEN) == 0)
    {
//Log.color(COLOR_RED, LOG_LV1, "%s() RURI[%s] = DIAL_IN_PREFIX\n", __FUNCTION__, ptr);
        if(strlen(ptr) != C_DIAL_IN_PREFIX_LEN+7) { return(false); }
        
        sscanf(&ptr[C_DIAL_IN_PREFIX_LEN], "%2d%5d", &bd, &ch);
        if(((0 <= bd) && (bd < MAX_SES_BOARD)) && ((0 <= ch) && (ch < MAX_SES_CH)))
        {
            pCR->bd = bd,
            pCR->ch = ch;
//Log.color(COLOR_RED, LOG_LV1, "%s() RURI[%s] = DIAL_IN_PREFIX bd=%d, ch=%d\n", __FUNCTION__, ptr, pCR->bd, pCR->ch);
            return(true);
        }
    }
    
//Log.color(COLOR_RED, LOG_LV1, "%s() #flase RURI[%s] = DIAL_IN_PREFIX\n", __FUNCTION__, ptr);
    return(false);
}

//#pragma mark -
//#pragma mark QUEUE에서 메세지를 꺼내 와서 SAM으로 전송하는 함수 들

/* Procedure Header
 *pdh********************************************************
 * PROCEDURE-NAME : RSSW_MakeSendBufferAndSendToSAM
 * CLASS-NAME     : -
 * PARAMETER    IN: pCR - Call Register 구조체 포인터
 * RET. VALUE     : BOOL
 * DESCRIPTION    : CSCF(SSW)에서 수신한 SIP 메세지를 SAM으로 전송하는 함수
 * REMARKS        : CR free는 여기서 안함
 **end*******************************************************/
bool RSSW_MakeSendBufferAndSendToSAM(CR_DATA *pCR)
{
    int     ret;
    int     nSendLen = 0;
    char    strSendBuf[SAFETY_MAX_BUF_SIZE]; // [JIRA AS-206]    

    bzero(strSendBuf, sizeof(strSendBuf));
    strcpy(strSendBuf, pCR->strRcvBuf);     // 일단 복사

    switch(pCR->nSipMsgType)
    {
        case SIP_TYPE_REQUEST:
            if(RSSW_AddSpyViaHeader(pCR, strSendBuf) == false)   // Via가 없으면 Error 처리를 하고 보내지 않는다.
            {
                RSSW_SendResponse(pCR, 400, "SIP;cause=505;text=\"Invalid SIP message\"");
                return(false);
            }
            break;
            
        case SIP_TYPE_RESPONSE:
            LIB_DelSpyViaHeader(pCR, strSendBuf);     // DelSpyVia는 false라도 그냥 전송 한다. (어짜피 떼어야 할 Via가 없기 문에)
            break;
            
        default:
            Log.printf(LOG_ERR, "[RSSW][%d] RSSW_MakeSendBufferAndSendToSAM undefined SIP Message Type %d !!\n", pCR->ot_type, pCR->nSipMsgType);
            return(false);
    }

	// FIXIT - AddSpyVia와 DelSpyVia에서 통합하면 memcpy(strcpy)를 한번 줄일 수 있다. - 성능이 떨어지면 검토 필요
    if(RSSW_Insert_P_BT_ChInfo(pCR, strSendBuf) == false) { // [JIRA AS-206]      // FIXIT: 다 넣을 것인지 Initial Message에만 넣을 것인지??
		return(false);
	}

    RSSW_Change_Content_Type(pCR, strSendBuf);      // 170117 for applicaiton/3gpp-ims+xml

	// ServiceKey를 SAM으로 전송하면 SAM에서 Service Key 검색을 위해 service.cfg 파일을 관리할 필요가 없다.
    if(pCR->info.bInitialMessage) { 
		if(RSSW_Insert_P_BT_ServiceKey(pCR, strSendBuf) == false) { // [JIRA AS-206] 
			return(false); 
		}
	}

#ifdef SB_MODE
	RSAM_Add_1stRecordRouteTo200INVITE(pCR, strSendBuf); 
	if(pCR->nSipMsg_ID != METHOD_INVITE) RSAM_Delete_FirstRoute(pCR, strSendBuf);
#endif

    if((nSendLen = (int)strlen(strSendBuf)) >= MAX_BUF_SIZE) // [JIRA AS-206] 
    {
        Log.printf(LOG_ERR, "RSSW_MakeSendBufferAndSendToSAM() strSendBuf Size(%d) >= MAX_BUF_SIZE(%d) !!!\n", nSendLen, MAX_BUF_SIZE);
        return(false);
    }
    
#ifdef SES_HA_MODE
	#ifdef SES_ACTSTD_MODE /* 20241126 kdh add */
	ret = UDP_SendMessage(C_SES_IP[MY_SIDE], C_SES_PORT, strSendBuf, nSendLen); /* 20241126 kdh add */
	#else
    if(pCR->bIsMoved) { ret = UDP_SendMessage(C_SES_IP[pCR->moved_bd], C_SES_PORT, strSendBuf, nSendLen); }
    else              { ret = UDP_SendMessage(C_SES_IP[pCR->bd],       C_SES_PORT, strSendBuf, nSendLen); }
	#endif /* SES_ACTSTD_MODE */
#else /* SES_HA_MODE */
//	#ifdef ONE_SERVER_MULTI_SES  // 20240123 bible : ONE Server - Multi SES <-> SES_HA_MODE ¿Í ¹èÅ¸ÀûÀ¸·Î Ã³¸®
//	ret = UDP_SendMessage(C_SES_IP[pCR->bd], C_SES_PORT[pCR->bd], strSendBuf, nSendLen);
//	#else
    ret = UDP_SendMessage(C_SES_IP[pCR->bd], C_SES_PORT, strSendBuf, nSendLen);
//	#endif /* ONE_SERVER_MULTI_SES */
#endif /* SES_HA_MODE */

//#ifdef ONE_SERVER_MULTI_SES // 20240123 bible : ONE Server - Multi SES <-> SES_HA_MODE ¿Í ¹èÅ¸ÀûÀ¸·Î Ã³¸®
//	if(ret == 0) { Log.printf(LOG_ERR, "%s() UDP_SendMessage(0) To=%s:%d fail\n", __FUNCTION__, C_SES_IP[pCR->bd], C_SES_PORT[pCR->bd]); }
//#else
	#ifdef SES_ACTSTD_MODE /* 20241126 kdh add */
	if(ret == 0) { Log.printf(LOG_ERR, "%s() UDP_SendMessage(0) To=%s:%d fail\n", __FUNCTION__, C_SES_IP[MY_SIDE], C_SES_PORT); }
	#else
    if(ret == 0) { Log.printf(LOG_ERR, "%s() UDP_SendMessage(0) To=%s:%d fail\n", __FUNCTION__, C_SES_IP[pCR->bd], C_SES_PORT); }
	#endif
//#endif
    
#ifdef DEBUG_MODE
    if (C_LOG_LEVEL >= LOG_LV2)
    {
// 20230324 bible : SES_HA_MODE define 추가
#ifdef SES_HA_MODE
	#ifndef SES_ACTSTD_MODE /* 20241126 kdh add */
        if(pCR->bIsMoved)
        {
            if (pCR->nSipMsgType == SIP_TYPE_RESPONSE)
                Log.color(COLOR_YELLOW, LOG_LV2, "[RSSW] >>>--->>> Send(%d) RESPONSE [%d] SAM-M[%d][%s:%d] Call-ID=[%s] >>>--->>>\n", ret, pCR->nSipMsg_ID, pCR->moved_bd, C_SES_IP[pCR->moved_bd], C_SES_PORT, pCR->info.strCall_ID);
            else
                Log.color(COLOR_YELLOW, LOG_LV2, "[RSSW] >>>--->>> Send(%d) REQUEST  [%s] SAM-M[%d][%s:%d] Call-ID=[%s] >>>--->>>\n", ret, STR_SIP_METHOD[pCR->nSipMsg_ID], pCR->moved_bd, C_SES_IP[pCR->moved_bd], C_SES_PORT, pCR->info.strCall_ID);
        }
        else
        {
	#endif /* SES_ACTSTD_MODE */
// 20230324 bible : SES_HA_MODE define 추가
#endif


//#ifdef ONE_SERVER_MULTI_SES // 20240123 bible : ONE Server - Multi SES <-> SES_HA_MODE ¿Í ¹èÅ¸ÀûÀ¸·Î Ã³¸®
//            if (pCR->nSipMsgType == SIP_TYPE_RESPONSE)
//                Log.color(COLOR_YELLOW, LOG_LV2, "[RSSW] >>>--->>> Send(%d) RESPONSE [%d] SAM[%d][%s:%d] Call-ID=[%s] >>>--->>>\n", ret, pCR->nSipMsg_ID, pCR->bd, C_SES_IP[pCR->bd], C_SES_PORT[pCR->bd], pCR->info.strCall_ID);
//            else
//                Log.color(COLOR_YELLOW, LOG_LV2, "[RSSW] >>>--->>> Send(%d) REQUEST  [%s] SAM[%d][%s:%d] Call-ID=[%s] >>>--->>>\n", ret, STR_SIP_METHOD[pCR->nSipMsg_ID], pCR->bd, C_SES_IP[pCR->bd], C_SES_PORT[pCR->bd], pCR->info.strCall_ID);
//#else /* ONE_SERVER_MULTI_SES */
	#ifdef SES_ACTSTD_MODE /* 20241126 kdh add */
			if (pCR->nSipMsgType == SIP_TYPE_RESPONSE)
                Log.color(COLOR_YELLOW, LOG_LV2, "[RSSW] >>>--->>> Send(%d) RESPONSE [%d] SAM[%d][%s:%d] Call-ID=[%s] >>>--->>>\n", ret, pCR->nSipMsg_ID, MY_SIDE, C_SES_IP[MY_SIDE], C_SES_PORT, pCR->info.strCall_ID);
            else
                Log.color(COLOR_YELLOW, LOG_LV2, "[RSSW] >>>--->>> Send(%d) REQUEST  [%s] SAM[%d][%s:%d] Call-ID=[%s] >>>--->>>\n", ret, STR_SIP_METHOD[pCR->nSipMsg_ID], MY_SIDE, C_SES_IP[MY_SIDE], C_SES_PORT, pCR->info.strCall_ID);
	#else /* SES_ACTSTD_MODE */
            if (pCR->nSipMsgType == SIP_TYPE_RESPONSE)
                Log.color(COLOR_YELLOW, LOG_LV2, "[RSSW] >>>--->>> Send(%d) RESPONSE [%d] SAM[%d][%s:%d] Call-ID=[%s] >>>--->>>\n", ret, pCR->nSipMsg_ID, pCR->bd, C_SES_IP[pCR->bd], C_SES_PORT, pCR->info.strCall_ID);
            else
                Log.color(COLOR_YELLOW, LOG_LV2, "[RSSW] >>>--->>> Send(%d) REQUEST  [%s] SAM[%d][%s:%d] Call-ID=[%s] >>>--->>>\n", ret, STR_SIP_METHOD[pCR->nSipMsg_ID], pCR->bd, C_SES_IP[pCR->bd], C_SES_PORT, pCR->info.strCall_ID);
	#endif /* SES_ACTSTD_MODE */
//#endif /* ONE_SERVER_MULTI_SES */

// 20230324 bible : SES_HA_MODE define 추가
#ifdef SES_HA_MODE 
	#ifndef SES_ACTSTD_MODE /* 20241126 kdh add */
        }
	#endif
// 20230324 bible : SES_HA_MODE define 추가
#endif
    }
    else
#endif /* DEBUG_MODE */

#ifdef SES_HA_MODE /* SES_HA_MODE 20230324 bible */
	#ifndef SES_ACTSTD_MODE /* 20241126 kdh add */
    {
        if(pCR->bIsMoved)
        {
            Log.color(COLOR_MAGENTA, LOG_LV1, "SES Board Moved %d -> %d !!\n", pCR->bd, pCR->moved_bd);
            Log.color(COLOR_CYAN, LOG_LV1, "[RSSW] >>>--->>> Send(%d) Message To SAM[%d][%s:%d] >>>--->>>\n%s\n", ret, pCR->moved_bd, C_SES_IP[pCR->moved_bd], C_SES_PORT, strSendBuf);
        }
        else
        {
	#endif /* SES_ACTSTD_MODE */
#endif /* SES_HA_MODE 20230324 bible */


//#ifdef ONE_SERVER_MULTI_SES // 20240123 bible : ONE Server - Multi SES <-> SES_HA_MODE ¿Í ¹èÅ¸ÀûÀ¸·Î Ã³¸®
//			Log.color(COLOR_CYAN, LOG_LV1, "[RSSW] >>>--->>> Send(%d) Message To SAM[%d][%s:%d] >>>--->>>\n%s\n", ret, pCR->bd, C_SES_IP[pCR->bd], C_SES_PORT[pCR->bd], strSendBuf);
//#else
	#ifdef SES_ACTSTD_MODE /* 20241126 kdh add */
			Log.color(COLOR_CYAN, LOG_LV1, "[RSSW] >>>--->>> Send(%d) Message To SAM[%d][%s:%d] >>>--->>>\n%s\n", ret, MY_SIDE, C_SES_IP[MY_SIDE], C_SES_PORT, strSendBuf);
	#else
            Log.color(COLOR_CYAN, LOG_LV1, "[RSSW] >>>--->>> Send(%d) Message To SAM[%d][%s:%d] >>>--->>>\n%s\n", ret, pCR->bd, C_SES_IP[pCR->bd], C_SES_PORT, strSendBuf);
	#endif /* SES_ACTSTD_MODE */
//#endif

#ifdef SES_HA_MODE /* SES_HA_MODE 20230324 bible */
	#ifndef SES_ACTSTD_MODE /* 20241126 kdh add */
        }
    }
	#endif /* SES_ACTSTD_MODE */
#endif /* SES_HA_MODE 20230324 bible */
    if(pCR->info.bInitialMessage)
    {
		//FIXIT: --- what ?? (이미 Hash는 SCM 응답 수신시 Update 했고.....???)
    }
    
    // FIXIT: trace
    if((0 <= pCR->info.nTrace) && (pCR->info.nTrace < 10))       // trace index: 0 ~ 9
    {

//#ifdef ONE_SERVER_MULTI_SES // 20240123 bible : ONE Server - Multi SES <-> SES_HA_MODE ¿Í ¹èÅ¸ÀûÀ¸·Î Ã³¸®
//		g_Trace.trace(pCR->info.nTrace, pCR->bd, pCR->ch, pCR->ot_type, C_SES_IP[pCR->bd], C_SES_PORT[pCR->bd], strSendBuf);
//#else
	#ifdef SES_ACTSTD_MODE /* 20241126 kdh add */
		g_Trace.trace(pCR->info.nTrace, MY_SIDE, pCR->ch, pCR->ot_type, C_SES_IP[MY_SIDE], C_SES_PORT, strSendBuf);
	#else
        g_Trace.trace(pCR->info.nTrace, pCR->bd, pCR->ch, pCR->ot_type, C_SES_IP[pCR->bd], C_SES_PORT, strSendBuf);
	#endif /* SES_ACTSTD_MODE */
//#endif /* ONE_SERVER_MULTI_SES */
    }

    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSSW_SendMessageToSAM
 * CLASS-NAME     : -
 * PARAMETER    IN: pCR - Call Register 구조체 포인터
 * RET. VALUE     :
 * DESCRIPTION    : CSCF(SSW)에서 수신한 SIP 메세지를 SAM으로 전송하는 함수
 * REMARKS        : UDP 수신시 할당한 메모리를 여기서 free해야 함.
 **end*******************************************************/
/*
 * 1. Parsing Received SIP Message & check Syntax Error
 * 2. Get Get Routing Info.
 * 3. if initial Message then send ChAllocRQ to SCM
 * 4. if not initial Message then send to SAM
 */
void RSSW_SendMessageToSAM(CR_DATA *pCR)
{
    /*
     * 1. 수신한 SIP 메세지를 Parsing
     */
    if(RSSW_ReceivedSipParsing(pCR) == false)
    {
		#ifdef FAULT_LOG_MODE //v4.4.2-1.1.0 MYUNG 20230906
        char tmp_msg[128];
        memset(tmp_msg, 0x00, sizeof(tmp_msg));
        sprintf(tmp_msg, "%s:%d", pCR->info.strCscfIp, pCR->info.nCscfPort);

        flcm_manager.ADD_Fault_FLCM(tmp_msg, 1);
        #endif

        if(pCR->nSipMsg_ID == METHOD_INVITE)        // 170202 - INVITE 인데 Parsing Error인 경우 (From/To) 가 긴 경우 400 전송
        {
            RSSW_SendResponse(pCR, 400, "SIP;cause=505;text=\"Invalid SIP message\"");
        }

        g_CR.free(pCR);      // free - UDP 수신시 alloac한 메모리
        return;
    }
    
    /*
     * 2. Get Routing Info.
     */
    if(RSSW_GetRoutingInfo(pCR) == false)
    {
		// Hash Table에 정보가 없더라도... BYE/CANCEL/ACK는 SPY에서 처리함다.
        
        if(pCR->nSipMsg_ID == METHOD_BYE)
        {
            RSSW_SendResponse(pCR, 200, NULL);      // BYE는 200으로 답한다.
        }
        else if(pCR->nSipMsg_ID == METHOD_CANCEL)
        {
            RSSW_SendResponse(pCR, 200, NULL);      // CANCEL 는 200으로 답한다.
        }
        else if(pCR->nSipMsg_ID != METHOD_ACK)
        {
			// ACK 는 SPY에서 채널할당 실패나 AS블록으로 INVITE에 대한 503/500등을 보냈을때도 올 수 있다.
			// 이경우 Hash에 없는 것이 당연하며 SPY에서 CallLeg이 바로 종료가 된다.
            Log.printf(LOG_INF, "[RSSW] RSSW_GetRoutingInfo() fail - Msg=[%d] Call-ID = %s\n", pCR->nSipMsg_ID, pCR->info.strCall_ID);
        }
        g_CR.free(pCR);      // free - UDP 수신시 alloac한 메모리
        return;
    }
    
	// INVITE retransmission 인경우 처리하지 않음... SCM에서 결과가 안오고 T/O 나면 그 retransmission 된 정보로 다시 진행 됨
    if(pCR->info.bRetransmission == true)
    {
        Log.color(COLOR_CYAN, LOG_LV3, "[RSSW] retransmission Call-ID=%s method=%s\n", pCR->info.strCall_ID, STR_SIP_METHOD[pCR->nSipMsg_ID]);
        g_CR.free(pCR);      // free - UDP 수신시 alloac한 메모리
        return;
    }


//    if((pCR->nSipMsg_ID == METHOD_REFER) && (C_REFER_405 == true))
    if((pCR->nSipMsg_ID == METHOD_REFER) || (pCR->nSipMsg_ID == METHOD_SUBSCRIBE) || (pCR->nSipMsg_ID == METHOD_NOTIFY))
    {
        RSSW_SendResponse(pCR, 405, NULL);
        g_CR.free(pCR);      // free - UDP 수신시 alloac한 메모리
        return;
    }
 
    /*
     * 3. initlai message이면 board/channel 할당을 위해 SCM에 요청한다.
     *    이때 별도의 MAP이나 QUEUE를 이용하지 않고 SET TIMER를 이용하여 예외 처리한다.
     */
    if(pCR->info.bInitialMessage)
    {
#ifdef MPBX_MODE                        // RCS, mPBX Dial-In conference에서 사용
        if(is_DialInPrefix(pCR))       // add 20160719 by SMCHO
        {
            time_t  t_now  = time(NULL);

            if(HASH_InsertAndSync(pCR, t_now) == false)
            {
                RSSW_SendResponse(pCR, 500, "SIP;cause=500;text=\"Internal Hash Error\"");
                g_CR.free(pCR);
                return;
            }
        }
        else
#endif
        {
			if(C_USE_MESSAGE == false) {	// 20170725 - TAS Message X
				if(pCR->nSipMsg_ID == METHOD_MESSAGE) // 20170724
				{
					RSSW_SendResponse(pCR, 405, NULL);
					g_CR.free(pCR);
					return;
				}
			}

            g_ScmMap.insert(pCR);       // SCM MAP을 사용하는 경우 100을 SCM에서 ChAlloc 결과를 받으면 보낸다.
            
            if(SEND_ChAllocRQToSCM(pCR) == false)
            {
                g_ScmMap.erase(pCR);    // 요청 실패시 MAP에서 삭제
                
                RSSW_SendResponse(pCR, 503, "SIP;cause=507;text=\"Internal Error\"");
                g_CR.free(pCR);     // free - UDP 수신시 alloac한 메모리
                return;
            }

			// ChAllocRQ를 보낸 경우에는 free(pCR)를 하지 않는다. ChAllocRP 수신 시 또는 T/O시에 free를 한다.
            return;
        }
    }
    else
    {
		// INVITE인데 initial 이 아니면 reINVITE이고 100을 여기에서 보낸다.
        if(pCR->nSipMsg_ID == METHOD_INVITE)
        {
#ifdef SAM_SEND_100
#ifdef SES_HA_MODE /* 20230324 bible */
	#ifdef SES_ACTSTD_MODE /* 20241126 kdh add */
			if((pCR->ot_type == OT_TYPE_ORIG))
	#else
			// ORIG인데 ACK 수신전에 다시 INVITE가 오는 경우는 재전송의 경우 임
            if((pCR->ot_type == OT_TYPE_ORIG) && (pCR->bIsReceiveACK == false))
	#endif /* SES_ACTSTD_MODE */
#else /* SES_HA_MODE */
			if((pCR->ot_type == OT_TYPE_ORIG))
#endif	/* SES_HA_MODE 20230324 bible */
            {
                if(RSSW_GetServiceKeyInMessage(pCR) == true)
                {
                    pCR->info.bInitialMessage = true;   // true 로 setting 해야 Service P-BT-ServiceKey를 메시지에 추가 함
                }
                else        // ServiceKey가 없으면 재전송이 아니라 re-INVITE 일 가능성이 높음
                {
                    RSSW_SendResponse(pCR, 100, NULL);
                }
            }
            else
#endif /* SAM_SEND_100 */
            {
                RSSW_SendResponse(pCR, 100, NULL); // re-INVITE인 경우 100을 SPY가 보냄
            }
        }
    }
    
    /*
     * 4. Inital Message가 아니면 SAM으로 전송한다.
     */
    bool rr = RSSW_MakeSendBufferAndSendToSAM(pCR);

	if(pCR->nSipMsg_ID == METHOD_BYE && rr)
		global_test_spy_send_to_sam_bye_count++;

    g_CR.free(pCR);      // free - UDP 수신시 alloac한 메모리
}


//#pragma mark -
//#pragma mark QUEUE에서 꺼낸 메세지를 처리하고 SAM으로 전송하는 함수 들

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSSW_DequeueThread_TERM
 * CLASS-NAME     : -
 * PARAMETER    IN: arg - dequeue thread index (multi-thread)
 * RET. VALUE     : NULL
 * DESCRIPTION    : CSCF(SSW)에서 수신한 SIP 메세지를 Queue에서 꺼내서 처리하는 함수
 * REMARKS        : TERM Stack에서 수신한 SIP 메세지만 처리
 **end*******************************************************/
void *RSSW_DequeueThread_TERM(void* arg)
{
    size_t     index = (size_t)arg;;
    int     count = 0;
    CR_DATA     *pCR = NULL;
    
    Log.printf(LOG_INF, "RSSW_DequeueThread_TERM() #%d START !!!\n", index);
    
    while(1)
    {
        if(g_nHA_State == HA_Active)
        {
            if(Q_Ssw_TERM.dequeue((void **)&pCR) == false)        // queue empty
            {
                msleep(1);     // 50 msec
                count = 0;      // 연속처리 count RESET
                continue;
            }

            RSSW_SendMessageToSAM(pCR);     // CSCF에서 수신한 메세지를 SAM으로 보내고 pCR 메모리 free
            
            if(++count < MAX_DEQUEUE_COUNT_SSW) { continue; }   // 메세지가 많을 경우 연속 10개 까지만 처리하고  sleep 하기 위한 루틴
        }
        
        // Active 가 아닌 경우 또는 10개이상 연속처리한 경우  sleep 후 retry ...
        msleep(1);         // 10 msec;
        count = 0;          // 연속처리 count RESET
        continue;
    }
    
//    Log.printf(LOG_ERR, "RSSW_DequeueThread_TERM() #%d END !!!\n", index);
    return(NULL);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSSW_DequeueThread_ORIG
 * CLASS-NAME     : -
 * PARAMETER    IN: arg - dequeue thread index (multi-thread)
 * RET. VALUE     : NULL
 * DESCRIPTION    : CSCF(SSW)에서 수신한 SIP 메세지를 Queue에서 꺼내서 처리하는 함수
 * REMARKS        : ORIG Stack에서 수신한 SIP 메세지만 처리
 **end*******************************************************/
void *RSSW_DequeueThread_ORIG(void* arg)
{
    size_t  index = (size_t)arg;;
    int     count = 0;
    CR_DATA     *pCR = NULL;
    
    Log.printf(LOG_INF, "RSSW_DequeueThread_ORIG() #%d START !!!\n", index);
    
    while(1)
    {
        if(g_nHA_State == HA_Active)
        {
            if(Q_Ssw_ORIG.dequeue((void **)&pCR) == false)        // queue empty
            {
                msleep(1);         // 50 msec
                count = 0;          // 연속처리 count RESET
                continue;
            }

            RSSW_SendMessageToSAM(pCR);     // CSCF에서 수신한 메세지를 SAM으로 보내고 pCR 메모리 free

            if(++count < MAX_DEQUEUE_COUNT_SSW) { continue; }   // 메세지가 많을 경우 연속 10개 까지만 처리하고  sleep 하기 위한 루틴
        }
        
        // Active 가 아닌 경우 또는 10개이상 연속처리한 경우  sleep 후 retry ...
        msleep(1);             // 10 msec;
        count = 0;              // 연속처리 count RESET
        continue;
    }
    
//    Log.printf(LOG_ERR, "RSSW_DequeueThread_ORIG() #%d END !!!\n", index);
    return(NULL);
}


//#pragma mark -
//#pragma mark SSW Server - CSCF(SSW)에서 SIP 메세지를 수신하고 QUEUE에 넣는 함수 들

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSSW_PrintReceiveSIP
 * CLASS-NAME     : -
 * PARAMETER    IN: pCR - Call Register 구조체 포인터
 * RET. VALUE     : -
 * DESCRIPTION    : 수신한 SIP 메시지를 Log파일에 출력하는 함수
 * REMARKS        :
 **end*******************************************************/
inline void RSSW_PrintReceiveSIP(CR_DATA *pCR)
{
    switch(C_LOG_LEVEL)
    {
        case LOG_LV3:   // FIXME: LV3 일때도 찍을 것인지 ??
        case LOG_LV2:
            if(pCR->nSipMsgType == SIP_TYPE_REQUEST)
            {
                Log.color(COLOR_GREEN, LOG_LV2, "[RSSW] ---<<<--- Recv Message [%s] From=[%s] Call-ID=[%s] ---<<<---\n", STR_SIP_METHOD[pCR->nSipMsg_ID], pCR->info.strCscfAddr, pCR->info.strCall_ID);
            }
            else
            {
                Log.color(COLOR_GREEN, LOG_LV2, "[RSSW] ---<<<--- Recv Message [%d] From=[%s] Call-ID=[%s] ---<<<---\n", pCR->nSipMsg_ID, pCR->info.strCscfAddr, pCR->info.strCall_ID);
            }
            return;

        case LOG_LV1:
            if(pCR->nSipMsgType == SIP_TYPE_REQUEST)
            {
                Log.color(COLOR_GREEN, LOG_LV1, "[RSSW] ---<<<--- Recv Message [%s] From=[%s] ---<<<---\n%s\n", STR_SIP_METHOD[pCR->nSipMsg_ID], pCR->info.strCscfAddr, pCR->strRcvBuf);
            }
            else
            {
                Log.color(COLOR_GREEN, LOG_LV1, "[RSSW] ---<<<--- Recv Message [%d] From=[%s] ---<<<---\n%s\n", pCR->nSipMsg_ID, pCR->info.strCscfAddr, pCR->strRcvBuf);
            }
            return;
            
        default: return;
    }
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : CheckBlackList
 * CLASS-NAME     : -
 * PARAMETER    IN: pCR - Call Register 구조체 포인터
 * RET. VALUE     : -
 * DESCRIPTION    : 수신한 메세지가 Black List로 부터 온 것인지 확인하는 함수
 * REMARKS        : Contact 메세지를 기준으로 검사
 *                : Add 20140918 by SMCHO
 **end*******************************************************/
bool CheckBlackList(CR_DATA *pCR)
{
    char        strTemp[128];   // 128 보다 크면 뒷 부분 짤라 버림
    char        strIpAddr[64];
    int         nPort;

    
    if(LIB_GetStrUntilCRLF(pCR->strRcvBuf, "\r\nContact:", strTemp, sizeof(strTemp)))
    {
#ifdef IPV6_MODE
        IP_TYPE     eIpType;
        struct in6_addr peer6;

        if(LIB_GetIpAddrInContactHeaderV6(strTemp, strIpAddr, sizeof(strIpAddr), &nPort, &eIpType))
        {
            if(eIpType == IP_TYPE_V6)
            {
                inet_pton(AF_INET6, strIpAddr, (void *)&peer6);
                
                if(g_BlackList_V6.is_black_list(peer6) == true)
                {
                    Log.printf(LOG_INF, "[BLACK LIST] IPv6 IP=%s from=%s\n", strIpAddr, pCR->info.strCscfIp);
                    return(true);
                }
            }
            else
            {
                if(g_BlackList.is_black_list(strIpAddr) == true)
                {
                    Log.printf(LOG_INF, "[BLACK LIST] IP=%s from=%s\n", strIpAddr, pCR->info.strCscfIp);
                    
                    return(true);
                }
            }
        }
#else
        if(LIB_GetIpAddrInContactHeader(strTemp, strIpAddr, sizeof(strIpAddr), &nPort))
        {
            if(g_BlackList.is_black_list(strIpAddr) == true)
            {
                Log.printf(LOG_INF, "[BLACK LIST] IP=%s from=%s\n", strIpAddr, pCR->info.strCscfIp);
                return(true);
            }
        }
#endif  // IPV6_MODE
        
    }
    return(false);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSSW_PushToQueue
 * CLASS-NAME     : -
 * PARAMETER    IN: pCR - Call Register 구조체 포인터
 * RET. VALUE     : BOOL
 * DESCRIPTION    : CSCF에서 수신한 SIP메세지를 종류별 Queue에 push하는 함수
 * REMARKS        : REG/OPT/그외 3가지 종류의 Queue 사용
 **end*******************************************************/
bool RSSW_PushToQueue(CR_DATA *pCR)
{
    bool    result;
    int     n_reg_q_size;
    
    /*
     * 등록되지 않은 SSW(CSCF)에서온 메세지는 응답하지 않고 무시한다.
     */
    switch(RSSW_CheckRegisteredSSW(pCR))
    {
        case -1:    g_ErrSvrList.add_unreg(pCR->info.strCscfIp);    return(false);    // 미등록 SSW - ERROR SSW 정보 저장 후 종료
        case 0:     return(false);      // BLOCK된 SSW - 아무일도 안함, 종료
        case 1:
        default:    break;              // 정상 Case
    }
    
    /*
     * SIP 메세지 종류와 RESPONSE일 경우 CSeq Header를 구한다.
     * StartLine이나 CSeq Header는 반드시 있어야 하며 없으면 Syntax Error(4xx) 처리하지 않고 바로 버림 (SIP 메세지가 아닐 확률이 높음)
     */
    if(LIB_GetSipTypeAndMethod(pCR->strRcvBuf, (int *)&pCR->nSipMsgType, &pCR->nSipMsg_ID) == false) { return(false); }
    
    // OPTIONS에 대한 Response를 별도 Queue로 처리하기 위해서 CSeq의 값을 구해 온다.
    if(LIB_GetSipHeaderValue(pCR->strRcvBuf, "\r\nCSeq:", pCR->info.strCSeq, sizeof(pCR->info.strCSeq)) == false)
    {
				
	    #ifdef FAULT_LOG_MODE //v4.4.2-1.1.0 MYUNG 20230906
        char tmp_msg[128];
        memset(tmp_msg, 0x00, sizeof(tmp_msg));
        sprintf(tmp_msg, "%s:%d", pCR->info.strCscfIp, pCR->info.nCscfPort);

        flcm_manager.ADD_Fault_FLCM(tmp_msg, 1);
        #endif


        if(pCR->nSipMsg_ID == METHOD_INVITE) { RSSW_SendResponse(pCR, 400, "SIP;cause=505;text=\"Invalid SIP message\""); }
        Log.printf(LOG_ERR, "RSSW_PushToQueue() Can't Get CSeq Header From=[%s]!!\n%s\n", pCR->info.strCscfAddr, pCR->strRcvBuf);
        return(false);
    }

    
    switch(pCR->nSipMsgType)        // 통계
    {
        case SIP_TYPE_REQUEST:            
#ifdef IPV6_MODE
            if(pCR->bIPV6)
            {
                statistic_v6.ssw.n_request[pCR->nSipMsg_ID] ++;
                statistic_v6.ssw.n_in_sum[pCR->ot_type] ++;
            }
            else
#endif
            {
                statistic.ssw.n_request[pCR->nSipMsg_ID] ++;
                statistic.ssw.n_in_sum[pCR->ot_type] ++;
            }
            break;
            
        case SIP_TYPE_RESPONSE:
#ifdef IPV6_MODE
            if(pCR->bIPV6)
            {
                statistic_v6.ssw.n_response_code[pCR->nSipMsg_ID] ++;
                statistic_v6.ssw.n_in_sum[pCR->ot_type] ++;
            }
            else
#endif
            {
                statistic.ssw.n_response_code[pCR->nSipMsg_ID] ++;
                statistic.ssw.n_in_sum[pCR->ot_type] ++;
            }
            break;
            
        default: return(false);
    }

    /*
     * REGISTER는 너무 많아서 별도의 Thread에서 처리 함. - mmtel이면 SPY에서 바로 처리하고 아니면 SRM에 보내서 처리 함.
     */
    if(pCR->nSipMsg_ID == METHOD_REGISTER)
    {
#ifdef	SOIP_REGI
		if(C_LOG_REGISTER) { RSSW_PrintReceiveSIP(pCR); }   // enqueue 전에 print 해야 함

		RSSW_GetServiceKeyInMessage(pCR);

		g_ScmMap.insert(pCR);
		if(SEND_ChAllocRQToSCM(pCR) == false)
		{
			Log.printf(LOG_ERR, "[REGISTER] SCM ALLOC FAIL!!!!\n");
			g_ScmMap.erase(pCR);
			RSSW_SendResponse(pCR, 200, NULL);
			return(false);      
		}
		
		RSSW_SendResponse(pCR, 200, NULL);
		return(true);      // Queue에 안 넣었기 때문에 CR을 해제 하기 위해서 false를 return;
#else
#ifndef INCLUDE_REGI    // mPBX 에서는 일단 REGI. 처리를 하지 않는다.
        RSSW_SendResponse(pCR, 200, NULL);
        return(false);      // Queue에 안 넣었기 때문에 CR을 해제 하기 위해서 false를 return;

#else
        if(C_LOG_REGISTER) { RSSW_PrintReceiveSIP(pCR); }   // enqueue 전에 print 해야 함
        
        pCR->bIsDeregi = REGI_IsDeREGISTER(pCR);            // add 20170222 - REGISTER 메세지를 분석해서 REGI/DeREGI를 구분함
        
        STAT_ADD_REGISTER_IN(pCR);                          // statictic
        
        // START Add 20170214 - REG Queue가 과도하게 쌓이면 에러처리한다.
        // 실제 HP BL460 G8 보드에서 REG Thread 1개당 약 200 CPS 정도만 처리가 가능하다.. 최대 5개이므로 1000 CPS 이상 들어오면 Queuq가 늘어남
        if((n_reg_q_size = Q_Reg.size()) >= C_MAX_REG_Q_SIZE)
        {
            RSSW_SendResponse(pCR, 503, "SIP;cause=500;text=\"Internal Queue Full\"");      // 20170307: 480 -> 503 KT 이상훈 차장 협의 내용
            STAT_ADD_REGISTER_FAIL(pCR);                       // statictic
            
            // REG Q가 Full이 되어 400이 나가게 되면 Alarm을 발생한다.
            if(g_bReqQ_Alarm == false)
            {
                g_bReqQ_Alarm = true;
                SEND_Reg_Q_Alarm(ALARM_ON);
            }
            return(false);
        }
        
        // Alarm이 발생되었다가 일정 갯수 이하로 떨어지면 Alarm을 OFF 한다.
        if(g_bReqQ_Alarm == true)
        {
            if(n_reg_q_size < C_REG_Q_ALM_LIMIT)
            {
                g_bReqQ_Alarm = false;
                SEND_Reg_Q_Alarm(ALARM_OFF);
            }
        }
        
        // End Add

#   ifdef DEBUG
        Log.printf(LOG_LV1, "[RSSW][%d] REGISTER enqueue OK!!!\n", pCR->ot_type);
#   endif
        if(Q_Reg.enqueue(pCR) == false)
        {
            RSSW_SendResponse(pCR, 503, "SIP;cause=500;text=\"Internal Queue Error\"");     // 20170307: 480 -> 503 KT 이상훈 차장 협의 내용
            STAT_ADD_REGISTER_FAIL(pCR);                       // statictic
            Log.printf(LOG_ERR, "[RSSW] REGISTER enqueue Fail!!!\n");
            return(false);
        }

        RSSW_SendResponse(pCR, 200, NULL);          // REGI인 경우에는 여기서 200을 바로 보낸다.
        return(true);
#endif	//	ifndef INCLUDE_REGI
#endif	//	SOIP_REGI
    }
    
    /*
     * OPTIONS Method이거나  CSeq에 OPTIONS가 있으면(OPTIONS에 대한 응답인 경우) - 너무 많아서 별도의 queue/thread에서 처리 함.
     */
    if ((pCR->nSipMsg_ID  == METHOD_OPTIONS)
    || ((pCR->nSipMsgType == SIP_TYPE_RESPONSE) && (strcasestr(pCR->info.strCSeq, "OPTIONS") != 0)) )
    {
        if(C_LOG_OPTIONS) { RSSW_PrintReceiveSIP(pCR); }      // enqueue 전에 print 해야 함
        
        if(Q_Opt.enqueue(pCR) == false)
        {
            Log.printf(LOG_ERR, "[RSSW] OPTIONS enqueue Fail!!!\n");
            return(false);
        }

        return(true);
    }
    
    /*
     * 일반적인 SIP 메세지 - REGISTER도 아니고 OPTIONS도 아닌 경우 (O/T를 구분해서 Queue에 저장 - 성능상 이유, 변경 가능)
     */

    if(LIB_GetSipHeaderValue(pCR->strRcvBuf, "\r\nCall-ID:", pCR->info.strCall_ID, sizeof(pCR->info.strCall_ID)) == false)
    {
		#ifdef FAULT_LOG_MODE //v4.4.2-1.1.0 MYUNG 20230906
		char tmp_msg[128];
		memset(tmp_msg, 0x00, sizeof(tmp_msg));
		sprintf(tmp_msg, "%s:%d", pCR->info.strCscfIp, pCR->info.nCscfPort);

		flcm_manager.ADD_Fault_FLCM(tmp_msg, 1);
		#endif
        Log.printf(LOG_ERR, "%s() Invalid SIP Message: Can't Get Call-ID Header !!\n", __FUNCTION__);
        g_ErrSvrList.add_msgerr(pCR->info.strCscfIp);   // ERROR 메세지를 보낸 CSCF로 등록
        if(pCR->nSipMsg_ID == METHOD_INVITE) { RSSW_SendResponse(pCR, 400, "SIP;cause=505;text=\"Invalid SIP message\""); }     // add 20161027
        return(false);
    }

    RSSW_PrintReceiveSIP(pCR);       // enqueue 전에 print 해야 함
    
    if(pCR->ot_type == OT_TYPE_ORIG)
    {
#ifdef IPV6_MODE
        if(pCR->bIPV6)
        {
            strcpy(pCR->info.strMyStackIp,     C_ORIG_STACK_IP_V6);
            strcpy(pCR->info.strMyStackIpPort, C_ORIG_STACK_IP_PORT_V6);
        }
        else
#endif
        {
            strcpy(pCR->info.strMyStackIp,     C_ORIG_STACK_IP);
            strcpy(pCR->info.strMyStackIpPort, C_ORIG_STACK_IP_PORT);
        }
        
        pCR->info.nMyStackPort = C_ORIG_STACK_PORT;
        result = Q_Ssw_ORIG.enqueue(pCR);
    }
    else
    {
#ifdef IPV6_MODE
        if(pCR->bIPV6)
        {
            strcpy(pCR->info.strMyStackIp,     C_TERM_STACK_IP_V6);
            strcpy(pCR->info.strMyStackIpPort, C_TERM_STACK_IP_PORT_V6);
        }
        else
#endif
        {
            strcpy(pCR->info.strMyStackIp,     C_TERM_STACK_IP);
            strcpy(pCR->info.strMyStackIpPort, C_TERM_STACK_IP_PORT);
        }
        
        pCR->info.nMyStackPort = C_TERM_STACK_PORT;
        result = Q_Ssw_TERM.enqueue(pCR);
    }
    
    if(result == false)
    {
        // FIXIT: - CSCF로 에러 메세지 전송 여부... REQUEST와 RESPONSE에 따라 구분...
        Log.printf(LOG_ERR, "[RSSW] SIP from SSW enqueue Fail!!!\n");
        return(false);
    }

    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSSW_ServerStart
 * CLASS-NAME     : -
 * PARAMETER    IN: ot_type - ORIG/TERM stack 여부
 * RET. VALUE     : 0
 * DESCRIPTION    : CSCF에서 SIP 메세지를 수신하여 Queue에 push하는 함수
 * REMARKS        : HA_Active인 경우에 실행 됨
 **end*******************************************************/
int RSSW_ServerStart(int ot_type)
{
    int     socket, nRcv;
	int     udp_err_cnt = 0;//, malloc_err_cnt = 0;
    char    comment[64];
    char    stack_ip[MAX_IPADDR_LEN+1];
    int     stack_port;
    CR_DATA *pCR;

    if(ot_type == OT_TYPE_ORIG) { strcpy(stack_ip, C_ORIG_STACK_IP); stack_port = C_ORIG_STACK_PORT; }
    else                        { strcpy(stack_ip, C_TERM_STACK_IP); stack_port = C_TERM_STACK_PORT; }
    
    while(g_nHA_State != HA_Active) { usleep(100); }    // standby는 UDP Server를 생성하지 않고 대기... wait
    
//    usleep(1000);       // imsi
#ifdef IPV6_MODE
    if(C_IP_MODE_V6 == true)
    {
        //Make IPv6/IPv6 Dual stack UDP Server for SSW(CSCF)
        if((socket = UDP_MakeServer_V6(stack_port)) < 0)
        {
            Log.printf(LOG_LV1, "[RSSW][%d] UDP-Server IPv4/IPv6 Dual Stack create retry #1 port=%d [%d]\n", ot_type, stack_port, socket);
            usleep(1000 * 200);           // STANDBY -> ACTIVE로 변경된 경우 Virtual IP가 등록안되면 서버 생성이 실패됨 (잠시후 다시 시도) // 20241222 kdh chg :: 2 ms -> 200 ms
            if((socket = UDP_MakeServer_V6(stack_port)) < 0)
            {
                Log.printf(LOG_ERR, "[RSSW][%d] UDP-Server IPv4/IPv6 Dual Stack create Error! (port=%d) [%d]\n", ot_type, stack_port, socket);
                shut_down(0);
            }
        }
        Log.color(COLOR_MAGENTA, LOG_INF, "[RSSW][%d] UDP-Server IPv4/IPv6 Dual Stack create OK !! (port=%d)\n", ot_type, stack_port);
    }
    else
    {
        // V6로 컴파일 되어도 config가 V4면 V4 socket 생성
        if((socket = UDP_MakeServerByIp(stack_ip, stack_port)) < 0)             //Make UDP Server for SSW(CSCF)
        {
            Log.printf(LOG_LV1, "[RSSW][%d] UDP-Server create retry#1 (%s:%d)\n", ot_type, stack_ip, stack_port);
            usleep(1000 * 200); // STANDBY -> ACTIVE로 변경된 경우 Virtual IP가 등록안되면 서버 생성이 실패됨 (잠시후 다시 시도) // 20241222 kdh chg :: 2 ms -> 200 ms
            if((socket = UDP_MakeServerByIp(stack_ip, stack_port)) < 0)             //Make UDP Server for SSW(CSCF)
            {
                Log.printf(LOG_ERR, "[RSSW][%d] UDP-Server create Error! (%s:%d)\n", ot_type, stack_ip, stack_port);
                shut_down(0);
            }
        }
    }
#else
	//Make UDP Server for SSW(CSCF)
	if((socket = UDP_MakeServerByIp(stack_ip, stack_port)) < 0)
    {
        Log.printf(LOG_LV1, "[RSSW][%d] UDP-Server create retry#1 (%s:%d)\n", ot_type, stack_ip, stack_port);
        usleep(1000 * 200);           // STANDBY -> ACTIVE로 변경된 경우 Virtual IP가 등록안되면 서버 생성이 실패됨 (잠시후 다시 시도) // 20241222 kdh chg :: 2 ms -> 200 ms
        if((socket = UDP_MakeServerByIp(stack_ip, stack_port)) < 0)             //Make UDP Server for SSW(CSCF)
        {
            Log.printf(LOG_ERR, "[RSSW][%d] UDP-Server create Error! (%s:%d)\n", ot_type, stack_ip, stack_port);
            shut_down(0);
        }
	}
#endif  // IPV6_MODE
    
	nRcv = 16 * 1024 * 1024; // 16777216
	setsockopt(socket, SOL_SOCKET, SO_RCVBUF, &nRcv, sizeof(nRcv));
    
	while(1)
	{
		if(g_nHA_State != HA_Active)            // HA 상태 변경시 Server Close
		{
			close(socket);
			Log.printf(LOG_INF, "[RSSW][%d] close UDP Server - HA State Change Active -> %d !!!\n", ot_type, g_nHA_State);
            return(0);
		}
        
        if((pCR = g_CR.alloc()) == NULL)     // pCR Allocation
        {
            Log.printf(LOG_ERR, "[RSSW][%d] CR_alloc() fail !!\n", ot_type);
            continue;
        }

        pCR->info.nTrace = -1;              // Initialize
        pCR->ot_type = ot_type;

        /*
         * UDP(SIP) 메세지 수신 및 에외 처리
         */
#ifdef IPV6_MODE
        pCR->nRcvLen = UDP_ServerWaitMessageEx_V6(socket, pCR->strRcvBuf, MAX_BUF_SIZE, &pCR->peer6);
#else
        pCR->nRcvLen = UDP_ServerWaitMessageByIpEx(socket, stack_ip, pCR->strRcvBuf, MAX_BUF_SIZE, &pCR->peer);
#endif
		if(pCR->nRcvLen <= 0)                   //  UDP 수신 에러
		{
			Log.printf(LOG_ERR, "[RSSW][%d] UDP Receive Error. err=%d, ret=%d, count=%d\n", ot_type, errno, pCR->nRcvLen, udp_err_cnt);
            
			if (++udp_err_cnt >= 3)
			{
//				udp_err_cnt = 0;    // RESET

#ifdef OAMSV5_MODE
				char    szCommentV5[8][64];
                memset(szCommentV5, 0x00, sizeof(szCommentV5));
                sprintf(szCommentV5[0], "SPY %c-RSSW Server Socket Error(count=%d)", (ot_type == OT_TYPE_ORIG) ? 'O': 'T', g_nSswServerResetCount[ot_type]);

                SEND_AlarmV2(MSG_ID_FLT, FLT_SPY_RSSW_UDP_SERVER, MY_BLK_ID, FLT_SPY_RSSW_UDP_SERVER, 0, ALARM_ON, 1, szCommentV5);
#else
                snprintf(comment, sizeof(comment), "SPY %c-RSSW Server Socket Error(count=%d)", (ot_type == OT_TYPE_ORIG) ? 'O': 'T', g_nSswServerResetCount[ot_type]);
                
                SEND_FaultMessage(FLT_SPY_RSSW_UDP_SERVER, ALARM_ON, 0, 2, comment);
#endif
                
                // 9번 이상 (3 * 3) 연속으로 UDP Read 실패가 발생하면 SPY를 Down 시키고 다시 실행 시킨다.
                if (++g_nSswServerResetCount[ot_type] >= 3) { shut_down(0); }
                
                close(socket);
                g_CR.free(pCR);
                return(0);
			}
            
            g_CR.free(pCR);
			continue;
		}
		else if(pCR->nRcvLen >= MAX_BUF_SIZE)       // 비정상적으로 큰 메세지 에러 처리..
		{
            RSSW_sockaddr_to_string(pCR);   // pCR->peer 정보를 info.strCscfAddr/Port로 변환
#ifdef OAMSV5_MODE
            char    szCommentV5[8][64];
            memset(szCommentV5, 0x00, sizeof(szCommentV5));
            sprintf(szCommentV5[0], "SPY RSSW %c-Server Rcv UDP Length(%d) Error. From=[%s:%d]", (ot_type == OT_TYPE_ORIG) ? 'O': 'T', pCR->nRcvLen, pCR->info.strCscfIp, pCR->info.nCscfPort);

            SEND_AlarmV2(MSG_ID_FLT, FLT_SPY_RSSW_UDP_SERVER, MY_BLK_ID, FLT_SPY_RSSW_UDP_SERVER, 0, ALARM_ON, 1, szCommentV5);
#else
            snprintf(comment, sizeof(comment), "SPY RSSW %c-Server Rcv UDP Length(%d) Error. From=[%s:%d]", (ot_type == OT_TYPE_ORIG) ? 'O': 'T', pCR->nRcvLen, pCR->info.strCscfIp, pCR->info.nCscfPort);
            
            SEND_FaultMessage(FLT_SPY_RSSW_UDP_SERVER, ALARM_ON, 0, 2, comment);
#endif

            Log.printf(LOG_ERR, "%s!!\n", comment);
            g_CR.free(pCR);
            continue;
		}
        else                                            // 정상 데이터 수신
        {
            /*
             * 수신한 UDP source 주소를 SIP 메세지를 보낸 SSW(CSCF, MS, ...) Address로 변환
             */

            RSSW_sockaddr_to_string(pCR);   // pCR->peer 정보를 info.strCscfAddr/Port로 변환
            
#ifdef DEBUG_MODE
#       ifdef IPV6_MODE
            Log.color(COLOR_MAGENTA, LOG_LV1, "%s() IP-TYPE = %s[%d], Addr = %s\n", __FUNCTION__, (pCR->bIPV6)?"V6":"V4", pCR->bIPV6, pCR->info.strCscfAddr);
#       else
            Log.color(COLOR_MAGENTA, LOG_LV1, "%s() IP-TYPE = V4, Addr = %s\n", __FUNCTION__,  pCR->info.strCscfAddr);
#       endif
#endif

            /*
             * 정상적으로 수신된 UDP(SIP) 메세지 처리 - 에러 카운터를 RESET하고 QUEUE에 저장한다.
             */
            udp_err_cnt                     = 0;        // RESET error counter
            g_nSswServerResetCount[ot_type] = 0;        // RESET error counter
            pCR->strRcvBuf[pCR->nRcvLen]    = '\0';     // set NULL to end of received Message

            if((C_BLACK_LIST_USE == true) && (CheckBlackList(pCR) == true)) { g_CR.free(pCR); continue; }  // Add 20140918 by SMCHO - black List 이면 무시
            
            if(RSSW_PushToQueue(pCR) == false) { g_CR.free(pCR); }   // QUEUE에 넣지 못한 경우에는 앞에서 Allocation한 pCR 메모리를 free 한다
        }
	}
    
    return(0);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSSW_ServerThread_TERM
 * CLASS-NAME     : -
 * PARAMETER    IN: arg - don't care
 * RET. VALUE     : NULL
 * DESCRIPTION    : SIP TERM STACK Server Thread
 * REMARKS        : Receive SIP from CSCF(SSW)
 **end*******************************************************/
void* RSSW_ServerThread_TERM(void* arg)
{
    size_t     index = (size_t)arg;       // if 64bit the use int64_t
    
    Log.printf(LOG_INF, "[RSSW][T] RSSW_ServerThread(%d) START !!!\n", index);
    
    while(1)
    {
        if(g_nHA_State == HA_Active)
        {
            RSSW_ServerStart(OT_TYPE_TERM);
            Log.printf(LOG_INF, "[RSSW][T] RSSW_ServerThread() RSSW Server STOP !! Retry !!!\n");
        }
        
        // Active 가 아닌 경우 retry ...
        usleep(100);
        continue;
    }
    
    return(NULL);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSSW_ServerThread_ORIG
 * CLASS-NAME     : -
 * PARAMETER    IN: arg - don't use
 * RET. VALUE     : NULL
 * DESCRIPTION    : SIP ORIG STACK Server Thread
 * REMARKS        : Receive SIP from CSCF(SSW)
 **end*******************************************************/
void *RSSW_ServerThread_ORIG(void* arg)
{
    size_t     index = (size_t)arg;       // if 64bit the use int64_t
    
    Log.printf(LOG_INF, "[RSSW][O] RSSW_ServerThread(%d) START !!!\n", index);
    
    while(1)
    {
        if(g_nHA_State == HA_Active)
        {
            RSSW_ServerStart(OT_TYPE_ORIG);
            Log.printf(LOG_INF, "[RSSW][O] RSSW_ServerThread() RSSW Server STOP !! Retry !!!\n");
        }
        
        // Active 가 아닌 경우 retry ...
        usleep(100);
        continue;
    }
    
    return(NULL);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSSW_main
 * CLASS-NAME     : -
 * PARAMETER      : -
 * RET. VALUE     : BOOL
 * DESCRIPTION    : Receive SIP message from CSCF(SSW) main function
 * REMARKS        : ORIG/TERM 구분해서 SIP 메시지 수신 및 처리 Thread 생성
 **end*******************************************************/
bool RSSW_main(void)
{
    size_t          index;      // if 64bit the use int64_t
    pthread_attr_t  tattr;
    pthread_t       tid;
    sched_param     param;
    
    // pthread 우선 순위 변경
    pthread_attr_init (&tattr);                         // initialized with default attributes
    pthread_attr_getschedparam (&tattr, &param);        // safe to get existing scheduling param
    param.sched_priority = SPY_SERVER_THREAD_PRIORITY;  // set the priority; 14
    pthread_attr_setschedparam (&tattr, &param);        // setting the new scheduling param
    
    // Make UDP SIP Server Thread for ORIG
    if(pthread_create(&tid, &tattr, &RSSW_ServerThread_ORIG, NULL))
    {
        Log.printf(LOG_ERR, "[RSSW][O] Can't ceate thread for RSSW_ServerThread_ORIG() reason=%d(%s)\n", errno, strerror(errno));
        return(false);
    }
	pthread_detach(tid);
    msleep(10);

    // Make UDP SIP Server Thread for TERM
    if(pthread_create(&tid, &tattr, &RSSW_ServerThread_TERM, NULL))
    {
        Log.printf(LOG_ERR, "[RSSW][T] Can't ceate thread for RSSW_ServerThread_TERM() reason=%d(%s)\n", errno, strerror(errno));
        return(false);
    }
	pthread_detach(tid);
    msleep(10);

    // Make Dequeue Thread for ORIG (send message to SAM)
    for(index = 0; index < C_MAX_RSSW_ORIG_THREAD; index ++)
    {
        if(pthread_create(&tid, NULL, RSSW_DequeueThread_ORIG, (void *)index))
        {
            Log.printf(LOG_ERR, "[RSSW][O] Can't ceate thread for RSSW_DequeueThread_ORIG(#%d) reason=%d(%s)\n", index, errno, strerror(errno));
            return(false);
        }
        pthread_detach(tid);
        msleep(10);
    }
    
    // Make Dequeue Thread for TERM (send message to SAM)
    for(index = 0; index < C_MAX_RSSW_TERM_THREAD; index ++)
    {
        if(pthread_create(&tid, NULL, RSSW_DequeueThread_TERM, (void *)index))
        {
            Log.printf(LOG_ERR, "[RSSW][T] Can't ceate thread for RSSW_DequeueThread_TERM(#%d) reason=%d(%s)\n", index, errno, strerror(errno));
            return(false);
        }
        pthread_detach(tid);
        msleep(10);
    }
    
#ifdef INCLUDE_REGI
    // Make Dequeue Thread for REGISTER Message Processing(ORIG/TERM 공통)
    for(index = 0; index < C_MAX_RSSW_REG_THREAD; index ++)
    {
        if(pthread_create(&tid, NULL, RSSW_DequeueThread_REG, (void *)index))
        {
            Log.printf(LOG_ERR, "[RSSW][T] Can't ceate thread for RSSW_DequeueThread_REG(#%d) reason=%s\n", index, strerror(errno));
            return(false);
        }
        pthread_detach(tid);
        msleep(10);
    }
    
    // 20151020 bible - regiCPS 처리
    void InitRegiCPScnt(void);
    void *SPY_RegisterCPSThread(void *arg);
    
    InitRegiCPScnt();
    pthread_create(&tid, NULL, SPY_RegisterCPSThread, NULL);
    
#endif  // INLCUDE_REGI
    
    // Make Dequeue Thread for OPTIONS Message Processing(ORIG/TERM 공통)
    for(index = 0; index < C_MAX_RSSW_OPT_THREAD; index ++)
    {
        if(pthread_create(&tid, NULL, RSSW_DequeueThread_OPT, (void *)index))
        {
            Log.printf(LOG_ERR, "[RSSW][T] Can't ceate thread for RSSW_DequeueThread_OPT(#%d) reason=%s\n", index, strerror(errno));
            return(false);
        }
        pthread_detach(tid);
        msleep(10);
    }
    
    return(true);
}


