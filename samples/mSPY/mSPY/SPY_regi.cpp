
/* File Header
 *fsh**************************************************************
 ******************************************************************
 **
 **  FILE : SPY_regi.cpp
 **
 ******************************************************************
 ******************************************************************
 BLOCK          : SPY
 SUBSYSTEM      : CMS
 SOR-NAME       :
 VERSION        : V4.X
 DATE           : 2014/01/
 AUTHOR         : SEUNG-MO, CHO
 HISTORY        :
 PROCESS(TASK)  :
 PROCEDURES     :
 DESCRIPTION    : REGISTER 메세지를 SPY에서 Queuing 처리하는 파일
 REMARKS        :
 *end*************************************************************/

#include "SPY_def.h"
#include "SPY_xref.h"



#ifdef INCLUDE_REGI

typedef struct
{
    int  board;
    int  channel;
    char sessionid;
    char dumy[7];
} DBM_HEAD;

typedef struct
{
    DBM_HEAD    head;
    char        strSQL[2048];
} DBM_QUERY;

#define DBM_MSG_ID		0xC001

void SPY_DeRegisterInfoToDB(CR_DATA * pCR);
void SPY_RegisterInfoToDB(CR_DATA *pCR, char *strBodyBuf);

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : REGI_GetDataFromFromHeader
 * CLASS-NAME     : -
 * PARAMETER    IN: strRcvBuf - 수신한 REGISTER 메세지
 *             OUT: strUser   - From Header의 User Part
 *              IN: MAX_USER  - strUser Buffer Size
 *             OUT: strScscf  - From Header의 Host Part
 *              IN: MAX_SCSCF - strScscf Buffer Size
 * RET. VALUE     : -
 * DESCRIPTION    : From Header에서 User와 Host 정보를 구하는 함수
 * REMARKS        : From: <User@Host>;......
 **end*******************************************************/
bool REGI_GetDataFromFromHeader(const char *strRcvBuf, char *strUser, int MAX_USER, char *strScscf, int MAX_SCSCF)
{
    int     nLength = 0;
    char    strFrom[256];
    char    strTemp[1024];
    
    bzero(strScscf, MAX_SCSCF);
    bzero(strUser,  MAX_USER);
    
    if(LIB_GetStrUntilCRLF(strRcvBuf, "\r\nFrom:", strFrom, sizeof(strFrom)))
    {
        nLength = (int)strlen(strFrom);
        for(int i = 0; i < nLength; i ++) { if(strFrom[i] == ';') { strFrom[i] = '\0'; break; } }   // delete tag, if exist
        
        // strFrom a@b => strTemp = a@b, strUser =a, strScscf = b,  strFrom=a => strTemp = a, strUser=NULL, strScscf = NULL
        LIB_GetAddrFromURI(strFrom, strTemp, sizeof(strTemp), strUser, MAX_USER, strScscf);
        
        if(strScscf[0] == '\0')
        {
            strncpy(strScscf, strTemp, MAX_SCSCF-1);        // user@Host 가 아닌경우 즉, user만 있는 경우
        }
    }
    else
    {
        Log.printf(LOG_ERR, "%s() From header not exist...\n%s\n", __FUNCTION__, strRcvBuf);
        return(false);
    }
    
    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : REGI_GetDataFromToHeader
 * CLASS-NAME     : -
 * PARAMETER    IN: strRcvBuf - 수신한 REGISTER 메세지
 *             OUT: strTo     - To Header의 User@Host Part
 *              IN: MAX_TO    - strTo buffer size
 * RET. VALUE     : -
 * DESCRIPTION    : To Header에서 User@Host:Port 정보를 구하는 함수
 * REMARKS        : To: <User@Host:Port>;...... 에서 <>만 제외
 **end*******************************************************/
bool REGI_GetDataFromToHeader(const char *strRcvBuf, char *strTo, int MAX_TO)
{
    bzero(strTo, MAX_TO);
    
    if(LIB_GetStrUntilCRLF(strRcvBuf, "\r\nTo:", strTo, MAX_TO))
    {
        LIB_delete_comment(strTo, ';');     // ';' 이하 삭제
        LIB_delete_comment(strTo, '>');     // '>' 이하 삭제
        LIB_delete_special_character(strTo, '<');
    }
    else
    {
        Log.printf(LOG_ERR, "%s() To header not exist...\n%s\n", __FUNCTION__, strRcvBuf);
        return(false);
    }
    
    return(true);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : REGI_ToHeaderSizeFix
 * CLASS-NAME     : -
 * PARAMETER    IN: strTo       - To Header
 *              IN: nSizeLimit  - To Header Size Limit(NULL 제외)
 * RET. VALUE     : -
 * DESCRIPTION    : To Header Size가 Limit 보다 크면 Limit에 맞게 줄이는 함수
 * REMARKS        : add 20150714 by SMCHO
 **end*******************************************************/
void REGI_ToHeaderSizeFix(char *strTo, int nSizeLimit)
{
    int     nLen, nLenHost, nUserLimit;
    char    strUser[512], strHost[512];
    
    if((nLen = (int)strlen(strTo)) <= nSizeLimit) { return; }
    if((nLen >= 512) || (nSizeLimit >= 512))
    {
        Log.printf(LOG_ERR, "[REGISTER] REGI_ToHeaderSizeFix() strTo length(%d) too BIG\n", nLen);
        return;
    }
    
    LIB_split_string_into_2(strTo, '@', strUser, strHost);
    
    if((nLenHost = (int)strlen(strHost)) >= nSizeLimit)  // Domain민 해도 Limit를 초과하는 경우, Domain을 자른다
    {
        // 이 경우 '@'를 포함하지 않고 그냉 도메인만 짤라서 보낸다.
        //        strHost[nSizeLimit] = NULL;     // set NULL(EOS)
        strHost[nSizeLimit] = 0;
        strcpy(strTo, strHost);
        return;
    }
    
    nUserLimit = nSizeLimit - nLenHost - 1;     // Limit에서 HostLen를 빼면 User가 가질수 있는 최대 길이가 나옴 (-1 = '@' 1자리도 포함해야 함)
    //    strUser[nUserLimit] = NULL;                 // strUser 뒷부분 cut
    strUser[nUserLimit] = 0;
    sprintf(strTo, "%s@%s", strUser, strHost);
    return;
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : REGI_GetDataFromContactHeader
 * CLASS-NAME     : -
 * PARAMETER    IN: strRcvBuf - 수신한 REGISTER 메세지
 *             OUT: strContact     - Contact Header의 User@Host Part
 *              IN: MAX_CONTACT    - strContact buffer size
 * RET. VALUE     : -
 * DESCRIPTION    : To Header에서 User@Host:Port 정보를 구하는 함수
 * REMARKS        : To: <User@Host:Port>;.. 에서 Port까지 제외한 User@Host
 **end*******************************************************/
bool REGI_GetDataFromContactHeader(const char *strRcvBuf, char *strContact, int MAX_CONTACT)
{
    char    strTemp[512];
    
    bzero(strContact, MAX_CONTACT);
    
    if(LIB_GetStrUntilCRLF(strRcvBuf, "\r\nContact:", strTemp, sizeof(strTemp)))
    {
        if(LIB_GetAddrFromFromToHeader(strTemp, strContact, MAX_CONTACT) == false)
        {
            strncpy(strContact, strTemp, MAX_CONTACT-1);
        }
    }
    else
    {
        Log.printf(LOG_ERR, "%s() Contact header not exist...\n%s\n", __FUNCTION__, strRcvBuf);
        return(false);
    }
    
    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : REGI_SendMessgeToDBM
 * CLASS-NAME     : -
 * PARAMETER    IN: send      - DBM으로 전송할 Packet
 *              IN: nSendLen  - 전송할 Packet 길이(Header 포함)
 #              IN: strFrom   - REGISTER의 From:
 * RET. VALUE     : -
 * DESCRIPTION    : DBM 으로 전송할 메시지를 받아서 헤더를 Setting하고 전송 하는 함수
 * REMARKS        :
 **end*******************************************************/
void SEND_RegisterMessageToDBM(DBM_QUERY *send, int nSendLen, const char *strFrom)
{
    int     ret;
    
    // set Header
    send->head.board     = 0xFFFFFFFF;
    send->head.channel   = 0xFFFFFFFF;
    send->head.sessionid = 0;
    
    if((ret = msendsig(nSendLen, DBM, DBM_MSG_ID, (uint8_t *)send)) != 0)
    {
        Log.printf(LOG_ERR, "%s() send to DBM Fail reason=%d\n", __FUNCTION__, ret);
    }

    if(C_LOG_REGISTER == true)
    {
        if(C_LOG_LEVEL == LOG_LV1) { Log.printf(LOG_LV1, "[REGI] send(%d) %s\n", ret, send->strSQL); }
        else                       { Log.printf(LOG_LV2, "[REGI] send(%d) LTE_UpdateRegistrationInfo(%s)\n", ret, strFrom); }
    }
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SPY_DeRegisterInfoToDB
 * CLASS-NAME     : -
 * PARAMETER    IN: pCR       - 수신한 SIP 메세지 구조체 포인터
 * RET. VALUE     : -
 * DESCRIPTION    : 수신한 De-REGISTER 메세지를 공통 DB를 update 하는 함수
 * REMARKS        : add 150826 by SMCHO
 **end*******************************************************/
void SPY_DeRegisterInfoToDB(CR_DATA *pCR)
{
    int     nSendLen = 0;
    char    strTo[256];
    char    strScscf[128];
    char    strContact[256];
    char    strUser[128];
    DBM_QUERY   send;
    
//    g_RegiSts.in_deregi ++;     // statictic
    
    /*
     * REGISTER SIP Header에서 가져오는 데이터 (User, Scscf, To)
     *  - From Header에서 데이터를 가져오는 Parameter(User, Scscf) - User@Scscf는 From Header에서 가져온다
     *  - To: Header 에서 user@host 부분 구해 옴 (PORT는 있으면 포함) - Body가 아닌 Real Header에서 구하게 변경 (20140328 - USIM 이동성 이슈)
     */
    if(REGI_GetDataFromFromHeader(pCR->strRcvBuf, strUser, sizeof(strUser), strScscf, sizeof(strScscf)) == false)
    {
        Log.printf(LOG_LV2, "[REGI] DeREGI REGI_GetDataFromFromHeader() fail\n");
        STAT_ADD_REGISTER_FAIL_DEREGI(pCR);
        return;
    }
    if(REGI_GetDataFromToHeader(pCR->strRcvBuf, strTo, sizeof(strTo)) == false)
    {
        Log.printf(LOG_LV2, "[REGI] DeREGI REGI_GetDataFromToHeader() fail\n");
        STAT_ADD_REGISTER_FAIL_DEREGI(pCR);
        return;
    }
    REGI_ToHeaderSizeFix(strTo, 64);    // add 20150915 - KT 요구사항
    
    /*
     * REGI의 경우에는 Body에서 Contact을 구하지만 De-REGI의 경우에는 Body가 없으므로 Header에서 Contact 정보를 구해온다.
     * Contact: Header 에서 user@host 부분 구해 옴(PORT도 삭제)
     */
    REGI_GetDataFromContactHeader(pCR->strRcvBuf, strContact, sizeof(strContact));
    
    bzero(&send, sizeof(send));
    
    nSendLen = snprintf(send.strSQL, sizeof(send.strSQL), "noncall exec LTE_UpdateDeregi_Nobody(0, 0, 1, '%s', '%s', '%s', '%c', '%c', 'sip:%s')",
                        strTo, strTo, strContact, '0', '0', strScscf);
    
    nSendLen += sizeof(DBM_HEAD);
    
    SEND_RegisterMessageToDBM(&send, nSendLen, strTo);
    
//    g_RegiSts.in_deregi_succ ++;    // statictic
    STAT_ADD_REGISTER_SUCCESS_DEREGI(pCR);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SPY_RegisterInfoToDB
 * CLASS-NAME     : -
 * PARAMETER    IN: pCR       - 수신한 SIP 메세지 구조체 포인터
 *              IN: strRcvBuf - 수신한 REGISTER 메세지 Body 포인터
 * RET. VALUE     : -
 * DESCRIPTION    : 수신한 REGISTER 메세지를 분석해서 공통 DB를 update 하는 함수
 * REMARKS        : FIXIT: 최종으로 Test된  버전(regqSPY)로 수정 필요
 **end*******************************************************/
void SPY_RegisterInfoToDB(CR_DATA *pCR, char *strBodyBuf)
{
    int     nSendLen = 0;
    int     nExpires = 0;
	int		nRet = false;
    char    cAudio = '0', cVideo = '0', cmmtel = '0';
	char	cIpsec = '0', cSMSip = '0'; //[JIRA AS-211]
//    char    *strBodyBuf;
    DBM_QUERY   send;
    char    strTemp[1024];
    char    strExpires[64];
    char    strScscf[128];
    char    strUser[128];
//    char    strFrom[256];
    char    strTo[1024];
    char    strContact[256];
    char    strPrivateId[256];
    char    strSipInstance[256];
    char    strUserAgent[256];
    char    strPAccessNetworkInfo[256];
    char    strPVisitedNetworkId[256];
    char    strTtaMcid[128];
    
//    bzero(&send,            sizeof(send));
    bzero(strExpires,       sizeof(strExpires));
    bzero(strContact,       sizeof(strContact));
    bzero(strPrivateId,     sizeof(strPrivateId));
    bzero(strSipInstance,   sizeof(strSipInstance));
    bzero(strUserAgent,     sizeof(strUserAgent));
    bzero(strTtaMcid,       sizeof(strTtaMcid));
    bzero(strPAccessNetworkInfo, sizeof(strPAccessNetworkInfo));
    bzero(strPVisitedNetworkId,  sizeof(strPVisitedNetworkId));

    bzero(strScscf,         sizeof(strScscf));
    bzero(strUser,          sizeof(strUser));
//    bzero(strFrom,          sizeof(strFrom));
    bzero(strTo,            sizeof(strTo));
    
    
//    g_RegiSts.in_regi ++;       // statictic

    /*
     * REGISTER SIP Header에서 가져오는 데이터 (User, Scscf, To)
     *  - From Header에서 데이터를 가져오는 Parameter(User, Scscf) - User@Scscf는 From Header에서 가져온다
     *  - To: Header 에서 user@host 부분 구해 옴 (PORT는 있으면 포함) - Body가 아닌 Real Header에서 구하게 변경 (20140328 - USIM 이동성 이슈)
     */
    if(REGI_GetDataFromFromHeader(pCR->strRcvBuf, strUser, sizeof(strUser), strScscf, sizeof(strScscf)) == false)
    {
        Log.printf(LOG_LV2, "[REGI] REGI REGI_GetDataFromFromHeader() fail\n");
        STAT_ADD_REGISTER_FAIL_REGI(pCR);
        return;
    }
    
    if(REGI_GetDataFromToHeader(pCR->strRcvBuf, strTo, sizeof(strTo)) == false)
    {
        Log.printf(LOG_LV2, "[REGI] REGI REGI_GetDataFromToHeader() fail\n");
        STAT_ADD_REGISTER_FAIL_REGI(pCR);
        return;
    }
    REGI_ToHeaderSizeFix(strTo, 64);    // add 20150915 - KT 요구사항
    
    /*
     * 이하 Bdoy에서 데이터를 가져오는 Parameters (User, Scscf, To를 제외한 나머지는 모두 Body에 있음)
     */
    
    
    /*
     * Get Private User ID: body의 Authorization Header 에 포함된 username 정보
     */
    if(LIB_GetStrUntilCRLF(strBodyBuf, "Authorization:", strTemp, sizeof(strTemp)))
    {
        STRs values;
        
        LIB_split_const_string(strTemp, ',', &values);
        
        for(int i = 0; i < values.cnt; i ++)
        {
            if(strcasestr(values.str[i], "username="))
            {
                LIB_split_string_into_2(values.str[i], '=', strTemp, strPrivateId);
                LIB_delete_special_character(strPrivateId, '"');
                
                break;
            }
        }
    }
    
    /*
     * Get Contact & instance & audio/video info:  body의 Contact Header에 포함되어 있음
     */
    if(LIB_GetStrUntilCRLF(strBodyBuf, "Contact:", strTemp, sizeof(strTemp)))
    {
        STRs values;
        
        LIB_split_const_string(strTemp, ';', &values);
        
        // contact의 첫번째 정보가 contact address
        LIB_delete_white_space(values.str[0]);
        LIB_delete_special_character(values.str[0], '<');
        LIB_delete_special_character(values.str[0], '>');
        if((strncasecmp(values.str[0], "sip:", 4) == 0) || (strncasecmp(values.str[0], "tel:", 4) != 0))
        {
            strcpy(strContact, &values.str[0][4]);      // sip 이거나 sip 도 아니고 tel도 아닌 경우
        }
        else
        {
            strcpy(strContact, values.str[0]);          // tel 인 경우
        }
        
        
        for(int i = 1; i < values.cnt; i ++)
        {
            // find gruu +sip,instance=“<urn:uuid:00000000-0000-000000-00000000>”
            if(strcasestr(values.str[i], "sip.instance="))
            {
                LIB_split_string_into_2(values.str[i], '=', strTemp, strSipInstance);
                LIB_delete_special_character(strSipInstance, '"');
                LIB_delete_special_character(strSipInstance, '<');
                LIB_delete_special_character(strSipInstance, '>');
            }
            else if(strcasecmp(values.str[i], "audio") == 0) { cAudio = '1'; }
            else if(strcasecmp(values.str[i], "video") == 0) { cVideo = '1'; }
            else if(strstr(values.str[i], "mmtel") != 0) { cmmtel = '1'; }
			else if(strncasecmp(values.str[i], "ipsec=", strlen("ipsec=")) == 0) { cIpsec = '1'; }
			else if(strncasecmp(values.str[i], "+g.3gpp.smsip", strlen("+g.3gpp.smsip")) == 0) { cSMSip = '1'; }
        }
        
        // 20141024 - Contact:에 Expires가 있으면 가져온다.
        for(int i = 1; i < values.cnt; i ++)
        {
            if(strcasestr(values.str[i], "Expires="))
            {
                LIB_split_string_into_2(values.str[i], '=', strTemp, strExpires);
                nExpires = atoi(strExpires);
                break;
            }
        }
    }
    
    /*
     * 20141024 - Expire: 를 Body에서 가져오던 것을 Header로 변경 (KT 요청사항 - iPhone 6 issue)
     *          - 헤더의 Expires:에서 값을 가져온다, 만약 헤더가 없으면 위에서 구한 Body의 Contact의 expires 파라미터에서 가져온 값이 유지된다.
     *          - RFC 3261 와 차이접 (KT 요구사항) -  RFC3261는 Contact이 우선 임
     */
    if(LIB_GetStrUntilCRLF(pCR->strRcvBuf, "Expires:", strExpires, sizeof(strExpires))) { nExpires = atoi(strExpires); }
    
    /*
     * Get Visited Network Id:  body의 P-Visited-Network-ID: Header에 포함되어 있음
     */
    LIB_GetStrUntilCRLF(strBodyBuf, "P-Visited-Network-ID:", strPVisitedNetworkId, sizeof(strPVisitedNetworkId));
    LIB_delete_special_character(strPVisitedNetworkId, '"');
    
    /*
     * Get Access Network Info:  body의 P-Access-Network-Info: Header에 포함되어 있음
     */
    LIB_GetStrUntilCRLF(strBodyBuf, "P-Access-Network-Info:", strPAccessNetworkInfo, sizeof(strPAccessNetworkInfo));
    LIB_delete_special_character(strPAccessNetworkInfo, '"');
    
    /* 20151022 - sunki PVNI / PANI 각각 판단 */
    /* 20151022 - sunkiPANI & PVNI는 regi, deregi 상관없이 정보가 없으면 'NONE'=> lee sang hoon */
    //if((strstr(strPAccessNetworkInfo, "3gpp") == 0) && (nExpires == 0))
    if(strstr(strPAccessNetworkInfo, "3gpp") == 0)
    {
        sprintf(strPAccessNetworkInfo, "%s", "NONE");
    }
    //if((strlen(strPVisitedNetworkId) == 0) && (nExpires == 0))
    if(strlen(strPVisitedNetworkId) == 0)
    {
        sprintf(strPVisitedNetworkId,  "%s", "NONE");
    }
    
    /*
     * Get User-Agent:  body의 User-Agent: Header에 포함되어 있음
     */
    LIB_GetStrUntilCRLF(strBodyBuf, "User-Agent:", strUserAgent, sizeof(strUserAgent));
    
    /*
     * Get P-TTA-MCID-Info:  body의 P-TTA-MCID-Info: Header에 포함되어 있음
     */
    if((nRet = LIB_GetStrUntilCRLF(strBodyBuf, "P-MCID-info:", strTtaMcid, sizeof(strTtaMcid)) ) == false)
	{
		LIB_GetStrUntilCRLF(strBodyBuf, "P-TTA-MCID-Info:", strTtaMcid, sizeof(strTtaMcid));
	}
	else
	{
		if(strstr(strTtaMcid, "ver=1.0"))
		{
			bzero(strTtaMcid, sizeof(strTtaMcid));
			strcpy(strTtaMcid, "ver=2.0");
		}
	}
    
    bzero(&send, sizeof(send));
    
	if(C_USE_REGI_API4 == true) //[JIRA AS-211]
	{
		nSendLen = snprintf(send.strSQL, sizeof(send.strSQL), 
						"noncall exec LTE_UpdateRegistrationInfo4(0, 0, 1, '%s', '%s', '%s', '%s', '%c', '%s', '%s', '%s', 'sip:%s', '%s', '%s', '%c', '%c', '%c', '%s', '%c', '%c')",
                        strTo, strTo, strPrivateId, strContact, (nExpires > 0) ? '1': '0', (nExpires > 0) ? strExpires: "0",
                        strPVisitedNetworkId, strPAccessNetworkInfo, /*sip*/strScscf, strSipInstance, strUserAgent,
                        cAudio, cVideo, cmmtel, strTtaMcid, cIpsec, cSMSip);
	}
	else
	{
		nSendLen = snprintf(send.strSQL, sizeof(send.strSQL), "noncall exec LTE_UpdateRegistrationInfo3(0, 0, 1, '%s', '%s', '%s', '%s', '%c', '%s', '%s', '%s', 'sip:%s', '%s', '%s', '%c', '%c', '%c', '%s')",
                        strTo, strTo, strPrivateId, strContact, (nExpires > 0) ? '1': '0', (nExpires > 0) ? strExpires: "0",
                        strPVisitedNetworkId, strPAccessNetworkInfo, /*sip*/strScscf, strSipInstance, strUserAgent,
                        cAudio, cVideo, cmmtel, strTtaMcid);
	}
    
    nSendLen += sizeof(DBM_HEAD);
    
    SEND_RegisterMessageToDBM(&send, nSendLen, strTo);
    
//    g_RegiSts.in_regi_succ ++;      // statictic
    STAT_ADD_REGISTER_SUCCESS_REGI(pCR);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SPY_Analyze_REGISTER
 * CLASS-NAME     : -
 * PARAMETER    IN: pCR - 수신한 SIP 메세지 구조체 포인터
 * RET. VALUE     : -
 * DESCRIPTION    : 수신한 REGISTER 메세지를 분석해서 REGI/DeREGI를 구분하는 함수
 * REMARKS        : add 20150714 by SMCHO
 **end*******************************************************/
void SPY_Analyze_REGISTER(CR_DATA *pCR)
{
    int     nLength     = 0;
    char    *strBodyBuf = 0;
    
    if(LIB_GetIntUntilCRLF(pCR->strRcvBuf, "Content-Length:", &nLength))
    {
        if(nLength != 0)    // body가 있음..
        {
            // BODY 시작 point 설정 - TAS(mmtel) REGISTER는 BODY가 "REGISTER"로 시작됨
            if((strBodyBuf = strstr(pCR->strRcvBuf, "\r\nREGISTER ")) == 0)
            {
                SPY_DeRegisterInfoToDB(pCR);                // DE-REGI (Body에 REGISTER가 없음)
            }
            else
            {
                SPY_RegisterInfoToDB(pCR, strBodyBuf);      // REGI
            }
        }
        else        // cLength = 0
        {
            SPY_DeRegisterInfoToDB(pCR);                    // DE-REGI (Body에  Content-Length가 0)
        }
    }
    else
    {
        SPY_DeRegisterInfoToDB(pCR);                        // DE-REGI (Body에  Content-Length가 없음)
    }
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : REGI_IsDeREGISTER
 * CLASS-NAME     : -
 * PARAMETER    IN: pCR  - 수신한 SIP 메세지 구조체 포인터
 * RET. VALUE     : BOOL - true이면 DeREG, false이면 REGI.
 * DESCRIPTION    : 수신한 REGISTER 메세지를 분석해서 REGI/DeREGI를 구분하는 함수
 * REMARKS        : add 20170222 by SMCHO
 **end*******************************************************/
bool REGI_IsDeREGISTER(CR_DATA *pCR)
{
    int     nLength     = 0;
    char    *strBodyBuf = 0;
    
    if(LIB_GetIntUntilCRLF(pCR->strRcvBuf, "Content-Length:", &nLength))
    {
        if(nLength != 0)    // body가 있음..
        {
            // BODY 시작 point 설정 - TAS(mmtel) REGISTER는 BODY가 "REGISTER"로 시작됨
            if((strBodyBuf = strstr(pCR->strRcvBuf, "\r\nREGISTER ")) != 0)
            {
                return(false);  // REGI
            }
        }
    }

    return(true);               // DE-REGI (Body에 REGISTER가 없거나, Content-Length Header가 없거나 0인 경우)
}

#define MAX_DEQUEUE_COUNT_REG   10      // QUEUE메세지 연속 처리 횟수 (CPU 부하 고려) - FIXIT: goto config

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSSW_DequeueThread_REG
 * CLASS-NAME     : -
 * PARAMETER    IN: arg - dequeue thread index (multi-thread)
 * RET. VALUE     : -
 * DESCRIPTION    : REGISTER 메세지를 처리하는 Thread
 * REMARKS        : 이미 수신한 REG에 대한 처리이기 때문에 Active여부와 관계없이
 *                : QUEUE에 있는 것은 처리한다.
 **end*******************************************************/
void *RSSW_DequeueThread_REG(void *arg)
{
    CR_DATA *pCR = NULL;
    int     count = 0;

    while(true)
    {
        if(Q_Reg.dequeue((void **)&pCR) == false)        // queue empty
        {
            msleep(50);         // 50 msec
            count = 0;          // 연속처리 count RESET
            continue;
        }
        
//        SPY_UpdateRegInfoToDB(pCR);
        SPY_Analyze_REGISTER(pCR);
        g_CR.free(pCR);
        
        
        // 10개이상 연속처리한 경우  sleep 후 retry ...
        if(++count < MAX_DEQUEUE_COUNT_REG) { continue; }   // 메세지가 많을 경우 연속 10개 까지만 처리하고  sleep 하기 위한 루틴
        msleep(50);             // 50 msec;
        count = 0;              // 연속처리 count RESET
        continue;
    }
}


#endif  // INCLUDE_REGI


#ifdef SOIP_REGI


void RefineAndSetStrToSendMsgBody(sSAMtoSRM *send, int *body_len, const char *key, char *value)
{
    // 131108 add by SMCHO
    // 특정 단말에서 올라온 메세지에 ASCII 특수 코드가 올라오는 경우가 있음
    // 이중 STX/ETX는 SRM으로 전송될 경우 문제가 발생되기 문에 SRM에서 삭제하고 보내게 수정
    LIB_delete_special_character(value, PARAM_STX);
    LIB_delete_special_character(value, PARAM_ETX);

    *body_len += sprintf(&send->body[*body_len], "%s=%c%s%c ", key, PARAM_STX, value, PARAM_ETX);
}


void Send3rdREGISTER(CR_DATA *pCR)
{
	int     intValue = 0;
	char    strRURI[128], strTo[1024], strFrom[256], strP_KT_UE_IP[128], TEMP_TEXT[128];
	sSAMtoSRM   send;
	int         body_len = 0;


	if(!LIB_GetIntUntilCRLF(pCR->strRcvBuf, "Expires:", &intValue))
    { 
		Log.printf(LOG_ERR, "Send3rdREGISTER() Expires header not exist...\n");
        SendChDeallocRQ_SCM(pCR->bd, pCR->ch);
        return;
    }

    if(intValue == 0)
    {
		Log.printf(LOG_ERR, "Send3rdREGISTER() Expires value = 0...\n");
        SendChDeallocRQ_SCM(pCR->bd, pCR->ch);
        return;
    }

	if(!LIB_GetStrUntilCRLF(pCR->strRcvBuf, "sip:", strRURI, sizeof(strRURI)))
	{   
    	if(!LIB_GetStrUntilCRLF(pCR->strRcvBuf, "tel:", strRURI, sizeof(strRURI)))
    	{
			Log.printf(LOG_ERR, "Send3rdREGISTER() ruri not exist...\n");
        	SendChDeallocRQ_SCM(pCR->bd, pCR->ch);
        	return;
    	}
	}

	for(int i = 0; strRURI[i]; i ++)      // strRURI[i] == NULL 까지 loop
	{
    	if((strRURI[i] == '@') || (strRURI[i] == ':') || (strRURI[i] == '>') || (strRURI[i] == ';'))
    	{
        	strRURI[i] = '\0';
        	break;
    	}
	}

    if(!LIB_GetStrUntilCRLF(pCR->strRcvBuf, "From:", strFrom, sizeof(strFrom)))
    {
		Log.printf(LOG_ERR, "Send3rdREGISTER() From header not exist...\n");
        SendChDeallocRQ_SCM(pCR->bd, pCR->ch);
        return;
    }

    for(int i = 0; i < strFrom[i]; i ++)    // delete tag, if exist
    {
        if(strFrom[i] == ';') { strFrom[i] = '\0'; break; }
    }


    if(!LIB_GetStrUntilCRLF(pCR->strRcvBuf, "To:", strTo, sizeof(strTo)))
    {
		Log.printf(LOG_ERR, "Send3rdREGISTER() To header not exist...\n");
        SendChDeallocRQ_SCM(pCR->bd, pCR->ch);
        return;
    }

    LIB_delete_special_character(strTo, '<');

    for(int i = 4; strTo[i]; i ++)
    {
        if((strTo[i] == ':') || (strTo[i] == '>') || (strTo[i] == ';'))
        {
            strTo[i] = '\0';
            break;
        }
    }

    memset(strP_KT_UE_IP, 0, sizeof(strP_KT_UE_IP));
    LIB_GetStrUntilCRLF(pCR->strRcvBuf, "P-KT-UE-IP:", strP_KT_UE_IP, sizeof(strP_KT_UE_IP));


    bzero(&send, sizeof(send));
    send.head.nBoardNo   = pCR->bd;
    send.head.nChannelNo = pCR->ch;
    send.head.nMessageId = 0x3017;
	send.head.nLowCallId = 0x10000;
//    send.head.nLowCallId = 0x10000+m_nThreadId;

    // set mandatory
    RefineAndSetStrToSendMsgBody(&send, &body_len, "head.from",  strFrom);
    RefineAndSetStrToSendMsgBody(&send, &body_len, "head.to",    strTo);             // change 131029 by SMCHO
    RefineAndSetStrToSendMsgBody(&send, &body_len, "head.ruri",  strRURI);           // add 131029 by SMCHO
    RefineAndSetStrToSendMsgBody(&send, &body_len, "svckey",     pCR->info.strServiceKey);
    RefineAndSetStrToSendMsgBody(&send, &body_len, "dp",         pCR->info.strDP);

    // set optional
    if(strP_KT_UE_IP[0]) { RefineAndSetStrToSendMsgBody(&send, &body_len, "p_kt_ue_ip", strP_KT_UE_IP); }
    if(strP_KT_UE_IP[0]) { RefineAndSetStrToSendMsgBody(&send, &body_len, "rhead.p_kt_ue_ip", strP_KT_UE_IP); }  // add 131029 by SMCHO

    msendsig(sizeof(SRM_Head)+strlen(send.body), SRM_00+pCR->bd, 0x6001, (BYTE *)&send);
	if(C_LOG_REGISTER) { Log.printf(LOG_LV1, "[REGISTER] to SRM(%s)\n", send.body); }


    return;
}

void Send3rdREGISTER_TAS(CR_DATA *pCR)
{
    int     nLength = 0;
    char    *strRecvBuf;
    char    strTo[1024], strFrom[256], strP_KT_UE_IP[128];
    char    strContact[1024], strPAccessNetworkInfo[256], strPVisitedNetworkId[256];
    char    strExpires[64], strScscf[128], strSipInstance[128], strPrivateId[128];
    char    strTemp[512], strUserAgent[256];
    char    strUser[128], strTtaMcid[128];
    sSAMtoSRM   send;
    int         body_len = 0;

    bzero(strExpires,       sizeof(strExpires));
    bzero(strContact,       sizeof(strContact));
    bzero(strPrivateId,     sizeof(strPrivateId));
    bzero(strP_KT_UE_IP,    sizeof(strP_KT_UE_IP));
    bzero(strSipInstance,   sizeof(strSipInstance));
    bzero(strPAccessNetworkInfo, sizeof(strPAccessNetworkInfo));
    bzero(strPVisitedNetworkId,  sizeof(strPVisitedNetworkId));
    bzero(strUserAgent,     sizeof(strUserAgent));         // add 120925
    bzero(strTtaMcid,       sizeof(strTtaMcid));

	if((strRecvBuf = strstr(pCR->strRcvBuf, "\r\nREGISTER ")) == 0)
    {
        Log.printf(LOG_ERR, "Send3rdREGISTER_TAS() Not Found REGISTER Body...\n");
        SendChDeallocRQ_SCM(pCR->bd, pCR->ch);
        return;
    }

	bzero(strFrom,  sizeof(strFrom));
    bzero(strUser,  sizeof(strUser));
    bzero(strScscf, sizeof(strScscf));
    if(!LIB_GetStrUntilCRLF(pCR->strRcvBuf, "From:", strFrom, sizeof(strFrom)))
    {
        Log.printf(LOG_ERR, "Send3rdREGISTER_TAS() From header not exist...\n");
        SendChDeallocRQ_SCM(pCR->bd, pCR->ch);
        return;
    }

    LIB_GetAddrFromURI(strFrom, strTemp, sizeof(strTemp), strUser, sizeof(strUser), strScscf);

    if(strScscf[0] =='\0')
    {
		strncpy(strScscf, strTemp, sizeof(strScscf)-1);
    }

	if(!LIB_GetStrUntilCRLF(strRecvBuf, "From:", strFrom, sizeof(strFrom)))
    {
        Log.printf(LOG_ERR, "Send3rdREGISTER_TAS() From header not exist(in body)...\n");
        SendChDeallocRQ_SCM(pCR->bd, pCR->ch);
        return;
    }

    nLength = strlen(strFrom);
    for(int i = 0; i < nLength; i ++)    // delete tag, if exist
    {
        if(strFrom[i] == ';') { strFrom[i] = '\0'; break; }
    }

	if(!LIB_GetStrUntilCRLF(pCR->strRcvBuf, "To:", strTo, sizeof(strTo)))
    {
        Log.printf(LOG_ERR, "Send3rdREGISTER_TAS() To header not exist...\n");
        SendChDeallocRQ_SCM(pCR->bd, pCR->ch);
        return;
    }

	if(LIB_GetStrUntilCRLF(strRecvBuf, "Contact:", strContact, sizeof(strContact)))
    {
        STRs values;

        LIB_split_const_string(strContact, ';', &values);

		for(int i = 1; i < values.cnt; i ++)
        {
            if(strcasestr(values.str[i], "sip.instance=")) // find gruu +sip.instance=“<urn:uuid:00000000-0000-000000-00000000>”
            {
                LIB_split_string_into_2(values.str[i], '=', strTemp, strSipInstance);
                LIB_delete_special_character(strSipInstance, '"');
                break;
            }
        }

        for(int i = 1; i < values.cnt; i ++)
        {
            if(strcasestr(values.str[i], "Expires="))
            {
                LIB_split_string_into_2(values.str[i], '=', strTemp, strExpires);
                break;
            }
        }
    }

	LIB_GetStrUntilCRLF(pCR->strRcvBuf, "Expires:", strExpires, sizeof(strExpires));

	if(LIB_GetStrUntilCRLF(strRecvBuf, "Authorization:", strTemp, sizeof(strTemp)))
    {
        STRs values;

        LIB_split_const_string(strTemp, ',', &values);

        for(int i = 0; i < values.cnt; i ++)
        {
            if(strcasestr(values.str[i], "username="))
            {
                LIB_split_string_into_2(values.str[i], '=', strTemp, strPrivateId);
                LIB_delete_special_character(strPrivateId, '"');
                break;
            }
        }
    }

    LIB_GetStrUntilCRLF(strRecvBuf, "P-KT-UE-IP:",              strP_KT_UE_IP,         sizeof(strP_KT_UE_IP));
    LIB_GetStrUntilCRLF(strRecvBuf, "P-Access-Network-Info:",   strPAccessNetworkInfo, sizeof(strPAccessNetworkInfo));
    LIB_GetStrUntilCRLF(strRecvBuf, "P-Visited-Network-ID:",    strPVisitedNetworkId,  sizeof(strPVisitedNetworkId));
    LIB_GetStrUntilCRLF(strRecvBuf, "User-Agent:",              strUserAgent,          sizeof(strUserAgent));              // add 120925
    LIB_GetStrUntilCRLF(strRecvBuf, "P-TTA-MCID-Info:",         strTtaMcid,            sizeof(strTtaMcid));                // add 140401

    bzero(&send, sizeof(send));
    send.head.nBoardNo   = pCR->bd;
    send.head.nChannelNo = pCR->ch;
    send.head.nMessageId = 0x3017;
    send.head.nLowCallId = 0x10000;

	RefineAndSetStrToSendMsgBody(&send, &body_len, "head.from",      strFrom);
    RefineAndSetStrToSendMsgBody(&send, &body_len, "head.to",        strTo);       // to = RURI
    RefineAndSetStrToSendMsgBody(&send, &body_len, "svckey",         pCR->info.strServiceKey);
    RefineAndSetStrToSendMsgBody(&send, &body_len, "dp",             pCR->info.strDP);
    RefineAndSetStrToSendMsgBody(&send, &body_len, "rhead.expires",  strExpires);

	if(strP_KT_UE_IP[0])  { RefineAndSetStrToSendMsgBody(&send, &body_len, "p_kt_ue_ip",         strP_KT_UE_IP); }
	if(strP_KT_UE_IP[0])  { RefineAndSetStrToSendMsgBody(&send, &body_len, "rhead.p_kt_ue_ip",         strP_KT_UE_IP); } // 20240322 kdh - add
    if(strContact[0])     { RefineAndSetStrToSendMsgBody(&send, &body_len, "rhead.contact",      strContact); }
    if(strScscf[0])       { RefineAndSetStrToSendMsgBody(&send, &body_len, "rhead.s_cscf",       strScscf); }
    if(strSipInstance[0]) { RefineAndSetStrToSendMsgBody(&send, &body_len, "rhead.sip_instance", strSipInstance); }
    if(strPrivateId[0])   { RefineAndSetStrToSendMsgBody(&send, &body_len, "rhead.private_id",   strPrivateId); }

    if(strPAccessNetworkInfo[0]) { RefineAndSetStrToSendMsgBody(&send, &body_len, "rhead.p_access_network_info", strPAccessNetworkInfo); }
    if(strPVisitedNetworkId[0])  { RefineAndSetStrToSendMsgBody(&send, &body_len, "rhead.p_visited_network_id", strPVisitedNetworkId); }

    if(strUserAgent[0])   { RefineAndSetStrToSendMsgBody(&send, &body_len, "rhead.user_agent",      strUserAgent); }    // add 120925
    if(strTtaMcid[0])     { RefineAndSetStrToSendMsgBody(&send, &body_len, "rhead.p_tta_mcid_info", strTtaMcid); }      // add 140401

    msendsig(sizeof(SRM_Head)+strlen(send.body), SRM_00+pCR->bd, 0x6001, (BYTE *)&send);
    if(C_LOG_REGISTER) { Log.printf(LOG_LV1, "[REGISTER-TAS] to SRM(%s)\n", send.body); }

    return;
}
#endif //SOIP_REGI


//#endif  // INCLUDE_REGI



