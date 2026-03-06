/* File Header
 *fsh**************************************************************
 ******************************************************************
 **
 **  FILE : SPY_rsam.cpp
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
 DESCRIPTION    : SAM에서 SIP 메세지를 수신하여 CSCF로 전달하는 함수들
 REMARKS        :
 *end*************************************************************/

#include "SPY_def.h"
#include "SPY_xref.h"


//#pragma mark -
//#pragma mark SIP Server - Dequeue and Send SIP to SSW


//#pragma mark -
//#pragma mark 기타 라이브러리 성격의 함수 들

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_delete_doamin
 * CLASS-NAME     :
 * PARAMETER INOUT: str  - domain을 삭제할  문자열
 * RET. VALUE     : -
 * DESCRIPTION    : 문자열에서 domain(@ip:port) 부분을 삭제한다.
 * REMARKS        : ';' 앞까지 삭제하고 이후는 그대로 복사한다.
 *                : ex) abc@ip;xxx;yyy -> abc;xxx;yyy
 **end*******************************************************/
void LIB_delete_doamin(char *str)
{
	char 	*s, *d;
	char	domain = 0;
    
	s = d = str;
	while (*s)
	{
		if((*s == '@') && (domain == 0)) { domain = 1; s++; }
        else
		{
			if(domain == 1)
			{
                // 두번 '@' 부터는 삭제안하기 위해 domain을 2로 변경
				if(*s == ';') { domain = 2; *d++ = *s++; }
				else          { s++; }
			}
			else
			{
				*d++ = *s++;
			}
		}
	}
    
	*d = '\0';
}


//#pragma mark -
//#pragma mark 수신한 SIP 메세지를 이용하여 CSCF로 전송하기 위한 SIP 메세지를 만드는 함수 들

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSAM_CopyRcvBufToSendBuf
 * CLASS-NAME     :
 * PARAMETER    IN: pCR        - Call Register 구조체 포인터
 *             OUT: strSendBuf - CSCF로 보낼 SIP 메세지
 * RET. VALUE     : BOOL
 * DESCRIPTION    : CSCF로 보낼 SIP 메세지를 만드는 함수
 * REMARKS        : strSendBuf에 strRcvBuf를 Copy 한다.
 *      RequestRUI에 uri=sip이 있는 경우 sip:으로 전송하고  user=phone이 있는 경우 tel:로 전송 함
 *      ex) SIP
 *          변경전: [INVITE sip:#88*@222.106.149.38:5002;phone-context=+82;uri=sip SIP/2.0]
 *          변경후: [INVITE sip:#88*@222.106.149.38:5002;phone-context=+82 SIP/2.0]
 *      ex) TEL
 *          변경전: [INVITE sip:#88*@222.106.149.38:5002;user=phone;phone-context=+82 SIP/2.0]
 *          변경후: [INVITE tel:#88*;phone-context=+82 SIP/2.0]
 **end*******************************************************/
bool RSAM_CopyRcvBufToSendBuf(CR_DATA *pCR, char *strSendBuf)
{
    char    *ptr, *ptrNext, *ptrCRLF;
    char    *ptrRURI, *ptrTemp;
    char    strRURI[512];
//    STRs    param;
    
    if((ptrCRLF = strstr(pCR->strRcvBuf, "\r\n")) == 0)
    {
        Log.printf(LOG_ERR, "[RSAM] can't get Request Line !!!\n");
        return(false);
    }
    
    // Request Line 길이 검사
    if((ptrCRLF - pCR->strRcvBuf) >= sizeof(strRURI))
    {
        Log.printf(LOG_ERR, "[RSAM] Request Line size To BIG = %d !!!\n", (ptrCRLF - pCR->strRcvBuf));
        return(false);
    }
    
    *ptrCRLF = '\0';        // set NULL for copy
    strcpy(strRURI, pCR->strRcvBuf);
    *ptrCRLF = '\r';        // restore NULL -> \r

#ifdef CCP_MODE
    // CCP 인 경우 INVITE만 RURI 변환(SC에서 tel 처리 못함)
    if(pCR->nSipMsg_ID != METHOD_INVITE)
    {
        strcpy(strSendBuf, pCR->strRcvBuf);
        return(true);
    }
#endif
    
    // uri=sip을 먼저 검사
    if((ptrRURI = strstr(strRURI, ";uri=sip")) != 0)   // SAM은 항상 SIP으로 전송함 (";uri=sip" 만 삭제), tel: 이라도 @doamin이 없기 때문에 sip:으로 변경 불가능 - 그냥 전송
    {
        ptr = strstr(pCR->strRcvBuf, ";uri=sip");   // strRURI에 있으니 당연히 strRcvBuf에도 있음
        ptrNext = ptr + 8;                          // 8 = strlen(";uri=sip"); // ";uri=sip" 다음 주소 point
        
        *ptrRURI = '\0';                        // set NULL for Copy
        
        strcpy(strSendBuf, strRURI);            // ";uri=sip" 전까지 strRURI  에서 copy
        strcat(strSendBuf, ptrNext);            // ";uri=sip" 이후는 strRcvBuf에서 copy
    }
    else if((ptrRURI = strstr(strRURI, ";user=phone")) != 0) // TEL로 변경하여 전송
    {
        ptr = strstr(pCR->strRcvBuf, ";user=phone");    // strRURI에 있으니 당연히 strRcvBuf에도 있음
        ptrNext = ptr + 11;                             // 11 = strlen(";user=phone"); // ";user=phone" 다음 주소 point
        
        *ptrRURI = '\0';                                // set NULL
        
        if((ptrTemp = strstr(strRURI, "sip:")) != 0)    // sip:을 tel:로 바꾸고 ";user=phone"부분을 삭제하고 전송
        {
            memcpy(ptrTemp, "tel", 3);                  // sip -> tel
            LIB_delete_doamin(strRURI);                 // domain 부분 삭제
        }
        else if((ptrTemp = strstr(strRURI, "tel:")) != 0)       // ";user=phone"부분을 삭제하고 전송
        {
           // nothing to do
        }
        else
        {
            Log.printf(LOG_ERR, "[RSAM] Request URI neither sip: nor tel: !!!\n");
            return(false);
        }

        strcpy(strSendBuf, strRURI);            // ";user=phone" 전까지 strRURI  에서 copy
        strcat(strSendBuf, ptrNext);            // ";user=phone" 이후는 strRcvBuf에서 copy
    }
    else    // 그대로 전송
    {
        strcpy(strSendBuf, pCR->strRcvBuf);
    }

    return(true);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSAM_Merge_P_BT_Contact
 * CLASS-NAME     : -
 * PARAMETER INOUT: strSendBuf - CSCF로 보낼 SIP 메세지
 * RET. VALUE     : -
 * DESCRIPTION    : Contact Header와 P-BT-Contact Header 병합
 * REMARKS        : SAM에 Contact뒤에 메세지를 붙일수 없어서 SPY에서 붙임
 *      +--------------------+  or +--------------------+  =>  +--------------------+
 *      |          A         |     |          A         |      |          A         |
 *      +--------------------+     +--------------------+      +--------------------+
 *       \r\nP-BT-Contact: aaa      \r\nContact: bbb           |          B         |
 *      +--------------------+     +--------------------+      +--------------------+
 *      |          B         |     |          B         |       \r\nContact: bbbaaa
 *      +--------------------+     +--------------------+      +--------------------+
 *       \r\nContact: bbb           \r\nP-BT-Contact: aaa      |          C         |
 *      +--------------------+     +--------------------+      +--------------------+
 *      |          C         |     |          C         |
 *      +--------------------+     +--------------------+
 **end*******************************************************/
void RSAM_Merge_P_BT_Contact(char *strSendBuf)
{
    char    *ptrContact, *ptrPBtContact;
    char    *ptrA, *ptrB, *ptrC;
    char    *ptrCRLF, *ptrCRLF2;
    char    *wptr;
    char    *ptrStart;
    char    strContact[512], strPBtContact[512];
    ssize_t     /*sizeA,*/ sizeB, sizeC;
    ssize_t     send_buf_len;
    
    // NO "P-BT-Contact:" or "Contact:" Header then return
    if((ptrContact    = strcasestr(strSendBuf, "\r\nContact:"))      == NULL) { return; }
    if((ptrPBtContact = strcasestr(strSendBuf, "\r\nP-BT-Contact:")) == NULL) { return; }
    
    ptrContact    += 2;    // +2 = skip "/r/n"
    ptrPBtContact += 2;
    
    if((ptrCRLF  = strstr(ptrContact,    "\r\n")) == NULL) { return; }
    if((ptrCRLF2 = strstr(ptrPBtContact, "\r\n")) == NULL) { return; }
    
    // copy Contact: header value
    bzero(strContact,    sizeof(strContact));
    ptrStart = ptrContact + 8;              // 8 = strlen("Contact:");
    if(*ptrStart == ' ') { ptrStart ++; }   // SPACE이면 다음 Point로 이동
    *ptrCRLF = '\0';                        // set NULL for copy
    strcpy(strContact, ptrStart);
    *ptrCRLF = '\r';                        // restore
    
    // copy P-BT-Contact: header value
    bzero(strPBtContact, sizeof(strPBtContact));
    ptrStart = ptrPBtContact + 13;          // 13 = strlen("P-BT-Contact:");
    if(*ptrStart == ' ') { ptrStart ++; }   // SPACE이면 다음 Point로 이동
    *ptrCRLF2 = '\0';                       // set NULL for copy
    strcpy(strPBtContact, ptrStart);
    *ptrCRLF2 = '\r';                       // restore

    ptrA = (char *)strSendBuf;
    send_buf_len = strlen(strSendBuf);
    
    if(ptrContact > ptrPBtContact)          // P-BT-Contact이 먼저나오고 Contact:이 뒤에 나오는 경우
    {
        ptrB = ptrCRLF2 + 2;       // +2 = /r/n
        ptrC = ptrCRLF  + 2;
        
//        sizeA = ptrPBtContact - ptrA;
        sizeB = ptrContact    - ptrB;
        sizeC = send_buf_len - (ptrC - ptrA);
        
        wptr = ptrPBtContact;
    }
    else                                    // Contact이 먼저 나오는 경우
    {
        ptrB = ptrCRLF  + 2;
        ptrC = ptrCRLF2 + 2;
        
//        sizeA = ptrContact    - ptrA;
        sizeB = ptrPBtContact - ptrB;
        sizeC = send_buf_len - (ptrC - ptrA);

        
        wptr = ptrContact;
    }
    
// 20210916    memcpy(wptr, ptrB, sizeB);  // A+B
    memmove(wptr, ptrB, sizeB);  // A+B
    wptr += sizeB;
    wptr += sprintf(wptr, "Contact: %s;%s\r\n", strContact, strPBtContact);
// 20210916    memcpy(wptr, ptrC, sizeC);
    memmove(wptr, ptrC, sizeC);
    wptr += sizeC;
    *wptr = '\0';
    
//    send_buf_len = wptr - ptrA;
    return;
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : Change_AccebtToAccept
 * CLASS-NAME     : -
 * PARAMETER INOUT: strSendBuf - CSCF로 보낼 SIP 메세지
 * RET. VALUE     : -
 * DESCRIPTION    : Accept를 지우고 Accebt가 있으면 Accebt를 Accept로 변경하는 함수
 * REMARKS        : RV Stack에서 Accept-Contact을 설정하면 Accept가 자동으로
 *                : Accept-Contact과 같게 설정되는 오류가 있음
 *                : 그래서 SAM에서 Accebt로 설정하면 SPY에서 기존 Accept를 지우고
 *                : Accebt를 Accept로 변경하여 전송
 **end*******************************************************/
void RSAM_Change_AccebtToAccept(char *strSendBuf)
{
    char    *ptrAccebt;
    
    // 1. Delete Accept Header (현재까지 일반적인 경우 AS에서 Accept를 보내지는 않음)
    LIB_DelSipHeaderNameAndValue(strSendBuf, "\r\nAccept:");
    
    // 2. Accebt가 있으면 Accept로 변경
    if((ptrAccebt = strcasestr(strSendBuf, "Accebt:")) == NULL) { return; }    // NO "Accebt:" Header then return
    ptrAccebt[4] = 'p';     // change 'b'->'p'
}



/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSAM_Change_AequireToRequire
 * CLASS-NAME     : -
 * PARAMETER INOUT: strSendBuf - CSCF로 보낼 SIP 메세지
 * RET. VALUE     : -
 * DESCRIPTION    : Aequire Header -> Require Header로 변경
 * REMARKS        : Require Header가 있으면 변경없이 Aequire만 삭제한다.
 **end*******************************************************/
void RSAM_Change_AequireToRequire(char *strSendBuf)
{
    char    *ptrAequire;
    
    if(strcasestr(strSendBuf, "Require:") != NULL)
    {
        // "Require:" Header 가 있는 경우에는 Aequire 만 삭제한다.
        LIB_DelSipHeaderNameAndValue(strSendBuf, "\r\nAequire:");
        return;
    }
    
    // Aequire를 Require로 변경
    if((ptrAequire = strcasestr(strSendBuf, "Aequire:")) == NULL) { return; }    // NO "Aequire:" Header then return
    ptrAequire[0] = 'R';     // change 'A'->'R'
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSAM_Change_Content_Type
 * CLASS-NAME     : -
 * PARAMETER INOUT: strSendBuf - CSCF로 보낼 SIP 메세지
 * RET. VALUE     : -
 * DESCRIPTION    : Content-Type Header 수정
 * REMARKS        : 3gpp-ims-xml -> 3gpp-ims+xml
 **end*******************************************************/
bool RSAM_Change_Content_Type(char *strSendBuf)
{
    char    *pStart, *pCRLF, *pWrite;
    
    if((pStart = strstr(strSendBuf, "\r\nContent-Type:")) != NULL)
    {
        pStart += 15;   // 15 = strlen("\r\nContent-Type:")
        
        if((pCRLF = strstr(pStart, "\r\n")) == NULL) { return(false); }     // ERROR
        
        *pCRLF = '\0';          // set NULL
        
        if((pWrite = strstr(pStart, "3gpp-ims-xml")) != NULL)
        {
            pWrite[8] = '+';    // change '+' -> '-' , "3gpp-ims+xml" -> "3gpp-ims-xml"
            *pCRLF = '\r';      // restore
            return(true);
        }
        
        *pCRLF = '\r';          // restore
    }
    return(false);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSAM_Delete_MultiSessionExpires
 * CLASS-NAME     : -
 * PARAMETER INOUT: strSendBuf - CSCF로 보낼 SIP 메세지
 * RET. VALUE     : -
 * DESCRIPTION    : 중복된 Session-Expires 헤더 삭제
 * REMARKS        : RV에서 중복으로 SE Header를 붙이는 경우가 있음
 **end*******************************************************/
void RSAM_Delete_MultiSessionExpires(char *strSendBuf)
{
    int     nSE_count;
    
    nSE_count = LIB_GetHeaderCount(strSendBuf, "\r\nSession-Expires:");
    
    if(nSE_count >= 2)
    {
        // 2개 이상인 경우 하나만 두고 나머지는 삭제...
        for(int i = 1; i < nSE_count; i ++)
        {
            LIB_DelSipHeaderNameAndValue(strSendBuf, "\r\nSession-Expires:");
        }
    }
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSAM_Change_SDPTosdp
 * CLASS-NAME     : -
 * PARAMETER INOUT: strSendBuf - CSCF로 보낼 SIP 메세지
 * RET. VALUE     : -
 * DESCRIPTION    : Content-Type: application/SDP 를 sdp로 변경
 * REMARKS        : RV Stack bug....
 **end*******************************************************/
void RSAM_Change_SDPTosdp(char *strSendBuf)
{
    char    *ptrCType;
    
    if((ptrCType = strstr(strSendBuf, "Content-Type: application/SDP")) != NULL)
    {
        ptrCType[26] = 's';
        ptrCType[27] = 'd';
        ptrCType[28] = 'p';
    }
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : AddSupportedTimerToNoBodyMessage
 * CLASS-NAME     : -
 * PARAMETER    IN: pCR        - Call Register 구조체 포인터
 *           INOUT: strSendBuf - CSCF로 보낼 SIP 메세지
 * RET. VALUE     : -
 * DESCRIPTION    : 메시지에 Supported: timer 를 추가
 * REMARKS        : Body가 없는 경우에만 추가한다.
 **end*******************************************************/
bool RSAM_AddSupportedTimerToNoBodyMessage(CR_DATA *pCR, char *strSendBuf)
{
    char    strSupported[256];
	char	strTempSupported[512];	// [JIRA AS-206]
    char    *ptrEnd;
    bool    bSupported = false;
    int     imsi;
	int		nAddLen = 0;			// [JIRA AS-206]
    
    if((ptrEnd = strstr(strSendBuf, "\r\n\r\n")) == NULL)
    {
        Log.printf(LOG_ERR, "%s() can't find end-of-message !!!\n", __FUNCTION__);
        return true;
    }
    
    if(ptrEnd[4] != '\0')  // Message Body가 존재 하는 경우 임
    {
        Log.printf(LOG_LV1, "%s() message body exist !!!\n", __FUNCTION__);
        return true;;
    }
    
    // 1. 이미 Supported: timer가 있는지 조사, 있으면 그냥 리턴
    if((LIB_GetStrUntilCRLF(strSendBuf, "\r\nSupported:", strSupported, sizeof(strSupported))) == true)
    {
        if(strcasestr(strSupported, "timer") != NULL) { return true; }   // already exist
        
    // 2. Supported는 있지만 timer는 없는 경우, 내용만 복사하고 삭제 (아래에서 삽입)
        bSupported = true;
        LIB_DeleteHeader(strSendBuf, (int *)&imsi, "\r\nSupported:");
        ptrEnd = strstr(strSendBuf, "\r\n\r\n");        // 삭제가 되면서 ptrEnd가 변경 되었음
    }
    
    // 3. 메시지의 끝에 Supportred추가
    // [JIRA AS-206] START
    if(bSupported) { nAddLen = sprintf(strTempSupported, "\r\nSupported: %s,timer\r\n\r\n", strSupported); }    // Supported가 있는 경우
    else           { nAddLen = sprintf(strTempSupported, "\r\nSupported: timer\r\n\r\n");                  }    // Supported가 없는 경우
	
	if(strlen(strSendBuf) + nAddLen >= SAFETY_MAX_BUF_SIZE) { // [JIRA AS-206] - 20170719
		Log.printf(LOG_ERR, "RSAM_AddSupportedTimerToNoBodyMessage() timer+RcvBuf Size(%d) >= MAX_BUF_SIZE(%d) !!!\n", nAddLen+strlen(strSendBuf), SAFETY_MAX_BUF_SIZE);
        return(false);
	}
	
	strcpy(ptrEnd, strTempSupported);
	// [JIRA AS-206] END
	
	return(true);
	
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : Add_SupportedTimerToUPDATE
 * CLASS-NAME     : -
 * PARAMETER    IN: pCR        - Call Register 구조체 포인터
 *           INOUT: strSendBuf - CSCF로 보낼 SIP 메세지
 * RET. VALUE     : -
 * DESCRIPTION    : UPDATE 메시지에 Supported: timer 를 추가
 * REMARKS        : TAS RV Stack config issue add 141202 by SMCHO
 *                : RV stack에 config 설정 시 SRM에서 relay되는 timer와
 *                : 중복되어 전송되는 문제가 있음(TAS에서만)
 **end*******************************************************/
bool RSAM_Add_SupportedTimerToUPDATE(CR_DATA *pCR, char *strSendBuf) // [JIRA AS-206]
{
    if((pCR->nSipMsg_ID != METHOD_UPDATE) || (C_ADD_UPDATE_TIMER == false)) { return true; }
    
    if(RSAM_AddSupportedTimerToNoBodyMessage(pCR, strSendBuf) == false) // [JIRA AS-206]
		return false;

	return true;
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSAM_Add_SupportedTimerTo200UPDATE
 * CLASS-NAME     : -
 * PARAMETER    IN: pCR        - Call Register 구조체 포인터
 *           INOUT: strSendBuf - CSCF로 보낼 SIP 메세지
 * RET. VALUE     : -
 * DESCRIPTION    : UPDATE 메시지에 Supported: timer 를 추가
 * REMARKS        : TAS RV Stack config issue add 141202 by SMCHO
 *                : RV stack에 config 설정 시 SRM에서 relay되는 timer와
 *                : 중복되어 전송되는 문제가 있음(TAS에서만)
 **end*******************************************************/
bool RSAM_Add_SupportedTimerTo200UPDATE(CR_DATA *pCR, char *strSendBuf) // [JIRA AS-206]
{
    if(C_ADD_200_UPDATE_TIMER == false) { return true; }
    if((pCR->nSipMsgType != SIP_TYPE_RESPONSE) || (pCR->nSipMsg_ID != 200) || (strstr(pCR->info.strCSeq, "UPDATE") == NULL)) { return true; }
    
    if(RSAM_AddSupportedTimerToNoBodyMessage(pCR, strSendBuf) == false) // [JIRA AS-206]
		return false;

	return true;
}

//#ifdef MPBX_MODE
#if defined(MPBX_MODE) || defined(SB_MODE)
/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSAM_Add_1stRecordRouteTo200INVITE
 * CLASS-NAME     : -
 * PARAMETER    IN: pCR        - Call Register 구조체 포인터
 *           INOUT: strSendBuf - CSCF로 보낼 SIP 메세지
 * RET. VALUE     : -
 * DESCRIPTION    : INVITE에 대한 200 메시지에 RecordRoute(O_SPY_IP) 를 추가
 * REMARKS        : mPBX KT 요청사항 (허희수 책임)
 **end*******************************************************/
void RSAM_Add_1stRecordRouteTo200INVITE(CR_DATA *pCR, char *strSendBuf)
{
    // INVITE-200이 아니거나 re-INVITE에 대한 200이면 처리하지 않음 - Initial INVITE에 대한 200에만 적용
#ifndef SB_MODE
    if(((pCR->nSipMsg_ID != 183) && (pCR->nSipMsg_ID != 200)) || (pCR->bIsReceiveACK == true) || (strstr(pCR->info.strCSeq, "INVITE") == NULL)) { return; }
#endif
    char    strIMSI[MAX_BUF_SIZE+1];
    char    *pStart;

#ifdef SB_MODE
	if(pCR->nSipMsg_ID != METHOD_INVITE || pCR->bIsReceiveACK == true) return;
#endif   
    // 맨처음 RecordRoute에 SPY IP를 넣는 방법
    if((pStart = strcasestr(strSendBuf, "\r\nRecord-Route:")))
    {
        strcpy(strIMSI, pStart);    // "\r\nRecord-Route:" 이하를 strIMSI에 copy
		#ifdef SB_MODE
        sprintf(pStart, "\r\nRecord-Route: <sip:%s;lr>%s", C_ORIG_STACK_IP_PORT, strIMSI);
		#else
        if(pCR->bIPV6 == true) { sprintf(pStart, "\r\nRecord-Route: <sip:mpbx@[%s]:%d;lr>%s", C_ORIG_STACK_IP_V6, C_ORIG_STACK_PORT, strIMSI); }
        else                   { sprintf(pStart, "\r\nRecord-Route: <sip:mpbx@%s;lr>%s", C_ORIG_STACK_IP_PORT, strIMSI);                       }
		#endif
    }
#if 1   // Record-Route가 없으면 끝에 추가해야 하나 말아야 하나 ??
    else if((pStart = strstr(strSendBuf, "\r\n\r\n")))
    {
        strcpy(strIMSI, pStart);    // "\r\n\r\n" 이하를 strIMSI에 cop
		#ifdef SB_MODE
        sprintf(pStart, "\r\nRecord-Route: <sip:%s;lr>%s", C_ORIG_STACK_IP_PORT, strIMSI);                       
        #else
        if(pCR->bIPV6 == true) { sprintf(pStart, "\r\nRecord-Route: <sip:mpbx@[%s]:%d;lr>%s", C_ORIG_STACK_IP_V6, C_ORIG_STACK_PORT, strIMSI); }
        else                   { sprintf(pStart, "\r\nRecord-Route: <sip:mpbx@%s;lr>%s", C_ORIG_STACK_IP_PORT, strIMSI);                       }
		#endif
    }	
#endif
}

char *RASM_FindLastHeader(char *strBuf, char *strFindHeader, int nHeaderSize)
{
    char    *pStart, *pNext;
    
    // 하나도 없으면  NULL 리턴
    if((pStart = strcasestr(strBuf, strFindHeader)) == NULL) { return(NULL); }
    
    while(1)
    {
        if((pNext = strcasestr(pStart+nHeaderSize, strFindHeader)))
        {
            pStart = pNext;     // 있으면 마지막 찾은것 부터 다시 검색
        }
        else
        {
            return(pStart);     // 마지막 주소 리턴
        }
    }
    
}

/* Procedure Header
**pdh********************************************************
* PROCEDURE-NAME : RSAM_Add_LastRecordRouteTo200INVITE
* CLASS-NAME     : -
* PARAMETER    IN: pCR        - Call Register 구조체 포인터
*           INOUT: strSendBuf - CSCF로 보낼 SIP 메세지
* RET. VALUE     : -
* DESCRIPTION    : INVITE에 대한 200 메시지에 RecordRoute(O_SPY_IP) 를 추가
* REMARKS        : mPBX KT 요청사항 (허희수 책임)
**end*******************************************************/
void RSAM_Add_LastRecordRouteTo200INVITE(CR_DATA *pCR, char *strSendBuf)
{
    // INVITE-200이 아니거나 re-INVITE에 대한 200이면 처리하지 않음 - Initial INVITE에 대한 200에만 적용
    if(((pCR->nSipMsg_ID != 183) && (pCR->nSipMsg_ID != 200)) || (pCR->bIsReceiveACK == true) || (strstr(pCR->info.strCSeq, "INVITE") == NULL)) { return; }
    
    char    strIMSI[MAX_BUF_SIZE+1];
    char    *pLast, *pCRLF;
    
    // 15 = strlen("\r\nRecord-Route:")
    if((pLast = RASM_FindLastHeader(strSendBuf, (char *)"\r\nRecord-Route:", 15)))
    {
        if((pCRLF = strstr(pLast+15, "\r\n")) == NULL) { Log.printf(LOG_ERR, "%s() Can't find CRLF of Last Record-Route !!!\n", __FUNCTION__); return; }
        
        strcpy(strIMSI, pCRLF);    // "\r\n" 이하를 strIMSI에 copy
		#ifdef IPV6_MODE
        if(pCR->bIPV6 == true) { sprintf(pCRLF, "\r\nRecord-Route: <sip:mpbx@[%s]:%d;lr>%s", C_ORIG_STACK_IP_V6, C_ORIG_STACK_PORT, strIMSI); }
        else                   { sprintf(pCRLF, "\r\nRecord-Route: <sip:mpbx@%s;lr>%s", C_ORIG_STACK_IP_PORT, strIMSI);                       }
		#else
		sprintf(pCRLF, "\r\nRecord-Route: <sip:mpbx@%s;lr>%s", C_ORIG_STACK_IP_PORT, strIMSI); 
		#endif
    }
#if 1   // Record-Route가 없으면 끝에 추가해야 하나 말아야 하나 ??
    else if((pLast = strstr(strSendBuf, "\r\n\r\n")))
    {
        strcpy(strIMSI, pLast);    // "\r\n\r\n" 이하를 strIMSI에 copy
		#ifdef IPV6_MODE
        if(pCR->bIPV6 == true) { sprintf(pLast, "\r\nRecord-Route: <sip:mpbx@[%s]:%d;lr>%s", C_ORIG_STACK_IP_V6, C_ORIG_STACK_PORT, strIMSI); }
        else                   { sprintf(pLast, "\r\nRecord-Route: <sip:mpbx@%s;lr>%s", C_ORIG_STACK_IP_PORT, strIMSI);                       }
		#else
		sprintf(pLast, "\r\nRecord-Route: <sip:mpbx@%s;lr>%s", C_ORIG_STACK_IP_PORT, strIMSI);
		#endif
    }
#endif
}

#endif  // MPBX_MODE


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : Add_RequireTimerTo200INVITE
 * CLASS-NAME     : -
 * PARAMETER    IN: pCR        - Call Register 구조체 포인터
 *           INOUT: strSendBuf - CSCF로 보낼 SIP 메세지
 * RET. VALUE     : -
 * DESCRIPTION    : INVITE에 대한 200 메시지에 Require: timer 를 추가
 * REMARKS        : refresher=uas인 경우에 Require가 없이 전송 됨
 **end*******************************************************/
bool RSAM_Add_RequireTimerTo200INVITE(CR_DATA *pCR, char *strSendBuf) // [JIRA AS-206]
{
    char    strRequire[256], strBody[2048];
	char	strTempRequire[512];		// [JIRA AS-206]
    char    *ptrEnd;
    bool    bRequire = false, bBodyExist = false;
    int     imsi;
	int		nAddLen = 0;				// [JIRA AS-206]

    if(C_ADD_200_INVITE_TIMER == false) { return true; }
    if((pCR->nSipMsgType != SIP_TYPE_RESPONSE) || (pCR->nSipMsg_ID != 200) || (strstr(pCR->info.strCSeq, "INVITE") == NULL)) { return true; }

    // 1. Session-Expires 헤더가 없으면 그냥 return; (Supported: timer는 비교 안하고 skip)
    if((LIB_GetStrUntilCRLF(strSendBuf, "\r\nSession-Expires:", strRequire, sizeof(strRequire))) == false) { return true; }
    
    // 2. 이미 Require: timer가 있는지 조사, 있으면 그냥 리턴
    if((LIB_GetStrUntilCRLF(strSendBuf, "\r\nRequire:", strRequire, sizeof(strRequire))) == true)
    {
        if(strcasestr(strRequire, "timer") != NULL) { return true; }   // Require: timer가 존재하는 경우
        
        // Require는 있지만 timer는 없는 경우, 내용만 복사하고 삭제 (아래에서 삽입)
        bRequire = true;
        LIB_DeleteHeader(strSendBuf, (int *)&imsi, "\r\nRequire:");
    }
    
    // 3. 메시지 Header의 끝에(헤더와 body 사이) Require: timer 추가
    if((ptrEnd = strstr(strSendBuf, "\r\n\r\n")) != NULL)
    {
        if(ptrEnd[4] != '\0')   // Message Body가 존재 하는 경우 임
        {
            strncpy(strBody, &ptrEnd[4], 2047);
            strBody[2047] = '\0';
            bBodyExist = true;
        }
        
		// [JIRA AS-206] START
        if(bRequire)    // timer option tag 없이 Require가 있었던 경우 기존 내용 + timer
        {
			nAddLen = sprintf(strTempRequire, "\r\nRequire: %s,timer\r\n\r\n",   strRequire);
			if(nAddLen + strlen(strSendBuf) >= SAFETY_MAX_BUF_SIZE) { // [JIRA AS-206] - 20170719
				Log.printf(LOG_ERR, "RSAM_Add_RequireTimerTo200INVITE() timer+RcvBuf Size(%d) >= MAX_BUF_SIZE(%d) !!!\n", nAddLen+strlen(strSendBuf), SAFETY_MAX_BUF_SIZE);
				return(false);
			}
			
			if(bBodyExist)	{ sprintf(ptrEnd, "%s%s", strTempRequire, strBody); }
			else			{ sprintf(ptrEnd, "%s", strTempRequire); }
        }
        else
        {
			nAddLen = sprintf(strTempRequire, "\r\nRequire: timer\r\n\r\n");
			if(nAddLen + strlen(strSendBuf) >= SAFETY_MAX_BUF_SIZE) { // [JIRA AS-206] - 20170719
                Log.printf(LOG_ERR, "RSAM_Add_RequireTimerTo200INVITE() timer+RcvBuf Size(%d) >= MAX_BUF_SIZE(%d) !!!\n", nAddLen+strlen(strSendBuf), SAFETY_MAX_BUF_SIZE);
                return(false);
            }	
	
            if(bBodyExist) 	{ sprintf(ptrEnd, "%s%s", strTempRequire, strBody); }
			else 			{ sprintf(ptrEnd, "%s", strTempRequire); }
        }
		// [JIRA AS-206] END
    }
    else
    {
        Log.printf(LOG_ERR, "RSAM_Add_RequireTimerTo200INVITE() can't find end-of-message !!!\n");
    }

	return true;
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSAM_AddSpyViaHeader
 * CLASS-NAME     :
 * PARAMETER INOUT: pCR    - 수신한 SIP 메세지 구조체 포인터
 *             OUT: strSendBuf - 보낼 SIP 메세지
 * RET. VALUE     : BOOL
 * DESCRIPTION    : SSW로 보낼 메세지에 SPY VIA를 추가하는 함수
 * REMARKS        : SIP Request는 자신의 Via를 추가해서 보낸다.
 *                : 이때 branch는 SAM에서 수신된 First Via branch를 이용하여 생성한다.
 *                : ex) SPY branch = first_via_branch-bd00-ch00000
 **end*******************************************************/
bool RSAM_AddSpyViaHeader(CR_DATA *pCR, char *strSendBuf)
{
	char    *ptrFirstVia, *ptrValue, *ptrBranch;
    char    *ptrCRLF, *ptrSemicolon, *ptrComma;
    char    *ptrWrite;
    char    strFirstViaBranch[128];
    char    strTempBuf[MAX_BUF_SIZE+1];
    int     nAddLen = 0;                // [JIRA AS-206]
    /*
     * 첫번째 Via의 Branch를 구한다.
     */
    if((ptrFirstVia = strcasestr(strSendBuf, "\r\nVia:")) == NULL)    // Via 없음 - 에러처리 (RV 버그 아니면 발생안됨)
    {
        Log.printf(LOG_ERR, "RSAM_AddSpyViaHeader() Can't find Via: header !!!\n");
        return(false);
    }
    
    ptrValue = ptrFirstVia + 6;   // Header size strlen("\r\nVia:") 만큼 포인터 이동
    
    if((ptrCRLF = strstr(ptrValue, "\r\n")) == NULL)                // Via 끝에 CRLF 없음 - 에러처리 (발생안됨)
    {
        Log.printf(LOG_ERR, "RSAM_AddSpyViaHeader() Can't find CRLF of Via: header !!!\n");
        return(false);
    }
    
    *ptrCRLF = '\0';    // set NULL - Via 내용의 마지막을 NULL로 설정해서 문자열 처리를 쉽게 한다.
    {
        // 한줄에 여러개의 Via가 있는 경우 첫번째 Via만 처리하기 위해서 NULL 처리 (add 151124 by SMCHO)
        if((ptrComma = strchr(ptrValue, ',')) != NULL) { *ptrComma = '\0'; }
        
        if((ptrBranch = strstr(ptrValue, "branch=")) == NULL)
        {
            Log.printf(LOG_ERR, "RSAM_AddSpyViaHeader() Can't find branch in Via: header !!!\n");
            *ptrCRLF = '\r';                    // restore
            return(false);                      // Via branch가 없음 - 에러처리 (발생안됨)
        }
        
        ptrBranch += 7;                         // +7("branch=") 다음으로 포인터 이동
        if((ptrSemicolon = strchr(ptrBranch, ';')) != 0)    // branch=다음에 ';'세미콜론이 있는 경우
        {
            *ptrSemicolon = '\0';                       // set NULL for copy
//            strcpy(strFirstViaBranch, ptrBranch);       // branch= 다음부터 ';' 앞까지 복사
            snprintf(strFirstViaBranch, 128, "%s", ptrBranch);       // branch= 다음부터 ';' 앞까지 복사
            *ptrSemicolon = ';';                        // restore
        }
        else                                                // branch=다음에 ';'세미콜론이 없는 경우
        {
//            strcpy(strFirstViaBranch, ptrBranch);    // branch= 다음부터CRLF 앞까지 복사
            snprintf(strFirstViaBranch, 128, "%s", ptrBranch);    // branch= 다음부터CRLF 앞까지 복사
        }
        
        if(ptrComma != NULL) { *ptrComma = ','; }   // restore multi-via (151124)
    }
    *ptrCRLF = '\r';    // restore
    
    /*
     * First Via의 Branch를 이용하여 SPY Via를 만들어서 SendBuf의 Via: 앞에 추가한다.
     * Via: SIP/2.0/UDP SPY_IP:PORT;branch=first_via_branch-bd000-ch00000 형태
     */
    strcpy(strTempBuf, ptrFirstVia);        // First Via 이후 부분을 Temp로 복사(SPY Via를 INSERT 하기 위해서)
    
	ptrWrite = ptrFirstVia; 
 
	// [JIRA AS-206] START
#ifdef IPV6_MODE
    if(pCR->bIPV6)
    {
        nAddLen = sprintf(ptrWrite, "\r\nVia: SIP/2.0/UDP [%s]:%d;branch=%s-bd%03d-ch%05d", pCR->info.strMyStackIp, pCR->info.nMyStackPort, strFirstViaBranch, pCR->bd, pCR->ch);
    }
    else
#endif
    {
        nAddLen = sprintf(ptrWrite, "\r\nVia: SIP/2.0/UDP %s:%d;branch=%s-bd%03d-ch%05d", pCR->info.strMyStackIp, pCR->info.nMyStackPort, strFirstViaBranch, pCR->bd, pCR->ch);
    }
    
    ptrWrite += nAddLen;    
    
    if((nAddLen + pCR->nRcvLen) >= SAFETY_MAX_BUF_SIZE)
    {
        Log.printf(LOG_ERR, "RSAM_AddSpyViaHeader() Via+RcvBuf Size(%d) >= MAX_BUF_SIZE(%d) !!!\n", nAddLen+pCR->nRcvLen, SAFETY_MAX_BUF_SIZE);
        return(false);
    }
	// [JIRA AS-206] END
    
    strcpy(ptrWrite, strTempBuf);           // First Via 이후 부분을 Append
    //Log.color(COLOR_CYAN, LOG_LV1, "RSAM_AddSpyViaHeader(2) strSendBuf=\n%s\n", strSendBuf);
    return(true);
}


//#ifdef MPBX_MODE
#if defined(MPBX_MODE) || defined(SB_MODE)
/* Procedure Header
 ***pdh********************************************************
 ** PROCEDURE-NAME : RSAM_Delete_FirstRoute
 ** CLASS-NAME     : -
 ** PARAMETER    IN: pCR        - Call Register 구조체 포인터
 **           INOUT: strSendBuf - CSCF로 보낼 SIP 메세지
 ** RET. VALUE     : -
 ** DESCRIPTION    : 첫번째 route가 SPY 주소이면 삭제
 ** REMARKS        :
 ***end*******************************************************/
void RSAM_Delete_FirstRoute(CR_DATA *pCR, char *strSendBuf)
{
    char    *pStart, *pCRLF;
    char    *pAddrStart, *pAddrEnd;
    char    strRoute[64];
	char 	strTemp[SAFETY_MAX_BUF_SIZE];
//	Log.printf(LOG_ERR, "####TEST 1#######\n %s",strSendBuf);
    if((pStart = strstr(strSendBuf, "\r\nRoute:")) == NULL) { return; }     // no Route header
    if((pCRLF  = strstr(pStart+8,   "\r\n"))       == NULL) { return; }     // no CRLF ---> bug
    
	strcpy(strTemp,pCRLF);
    *pCRLF = '\0';      // set NULL
    
 //   Log.printf(LOG_ERR, "####TEST 2#######\n %s",strSendBuf);
 //  Log.printf(LOG_ERR, "####TEST 2-1#######\n %s",pCRLF+1);
#ifdef IPV6_MODE
    if(pCR->bIPV6 == true)
    {
        // IPv6reference 는 반드시 '['와 ']'이 있어야 한다.
         if((pAddrStart = strchr(pStart, '[')) == NULL)
        {
            Log.printf(LOG_ERR, "%s() '[' is missing in 1st IPv6 Route Header\n", __FUNCTION__);
            *pCRLF = '\r';      // restore
            return;
        }
        pAddrStart ++;      // skip '['
        
        if((pAddrEnd = strchr(pAddrStart, ']')) == NULL)
        {
            Log.printf(LOG_ERR, "%s() ']' is missing in 1st IPv6 Route Header\n", __FUNCTION__);
            *pCRLF = '\r';      // restore
            return;
        }
        
        *pAddrEnd = '\0';   // set NULL
        if(strlen(pAddrStart) > 63)
        {
            Log.printf(LOG_ERR, "%s() 1st  Route IPv6 Length too BIG [%s]\n", __FUNCTION__, pAddrStart);
        }
        else
        {
            strcpy(strRoute, pAddrStart);
        }

        *pCRLF    = '\r';   // restore
        *pAddrEnd = ']';    // restore

        if(LIB_ipv6cmp(strRoute, C_IN6_SPY) == true)
        {
			// LIB_strcpy
            strcpy(pStart, pCRLF);      // 첫번째 route를 건너 뛰고 overwrite
        }
    }
    else
#endif
    {
        if(strstr(pStart, C_TERM_STACK_IP) != NULL)       // find
        {
            *pCRLF = '\r';              // restore
            strcpy(pStart,strTemp);      // 첫번째 route를 건너 뛰고 overwrite
          //  strcpy(pStart, pCRLF);      // 첫번째 route를 건너 뛰고 overwrite
        }
        else
        {
            *pCRLF = '\r';              // restore
        }
    }
//    Log.printf(LOG_ERR, "####TEST 3#######\n %s",strSendBuf);
}

#endif

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSAM_MakeSendBuffer
 * CLASS-NAME     :
 * PARAMETER INOUT: pCR        - Call Register 구조체 포인터
 *             OUT: nSendLen   - CSCF로 보낼 SIP 메세지 길이
 *             OUT: strSendBuf - CSCF로 보낼 SIP 메세지
 * RET. VALUE     : BOOL
 * DESCRIPTION    : CSCF로 보낼 SIP 메세지를 만드는 함수
 * REMARKS        :
 **end*******************************************************/
bool RSAM_MakeSendBuffer(CR_DATA *pCR, int *nSendLen, char *strSendBuf)
{
    *nSendLen = 0;

    switch(pCR->nSipMsgType)
    {
        case SIP_TYPE_REQUEST:
            if(RSAM_CopyRcvBufToSendBuf(pCR, strSendBuf) == false) { return(false); }
            if(RSAM_AddSpyViaHeader(pCR, strSendBuf)     == false) { return(false); } // SAM Via가 없으면 Error 처리를 하고 보내지 않는다.
            break;
            
        case SIP_TYPE_RESPONSE:
            strcpy(strSendBuf, pCR->strRcvBuf);
            LIB_DelSpyViaHeader(pCR, strSendBuf);       // DelSpyVia는 false라도 그냥 전송 한다. (어짜피 떼어야 할 Via가 없기 문에)
            
            if(C_DEL_18X_SUPPORTED_FLAG)      // TAS only
            {
                if((180 <= pCR->nSipMsg_ID) && (pCR->nSipMsg_ID < 190))
                {
                    LIB_DelSipHeaderNameAndValue(strSendBuf, "\r\nSupported:");
                }
            }
            break;
            
        default:
            Log.printf(LOG_ERR, "[RSAM] undefined SIP Message Type %d !!\n", pCR->nSipMsgType);
            return(false);
    }
    
    RSAM_Merge_P_BT_Contact(strSendBuf);
    RSAM_Delete_MultiSessionExpires(strSendBuf);
    LIB_DelSipHeaderNameAndValue(strSendBuf, "\r\nP-BT-ChInfo:");   // DELETE BT Private Header
    
    RSAM_Change_SDPTosdp(strSendBuf);                           // FIXIT - RV3.1.1 이슈 ... RV4.5에서도 같은 현상이 있는지 확인....
    RSAM_Change_AccebtToAccept(strSendBuf);
    RSAM_Change_AequireToRequire(strSendBuf);
    RSAM_Change_Content_Type(strSendBuf);                       // add 170117
    if(RSAM_Add_SupportedTimerToUPDATE(pCR, strSendBuf) == false) return false; // [JIRA AS-206]          // add 141202 by SMCHO
    if(RSAM_Add_RequireTimerTo200INVITE(pCR, strSendBuf) == false) return false; // [JIRA AS-206]          // add 141202 by SMCHO
    if(RSAM_Add_SupportedTimerTo200UPDATE(pCR, strSendBuf) == false) return false; // [JIRA AS-206]        // add 151120 by SMCHO
#ifdef SB_MODE
	if(pCR->nSipMsg_ID != METHOD_INVITE) RSAM_Delete_FirstRoute(pCR, strSendBuf); //MIN 2021.03.18 ADD    
	if(pCR->nSipMsg_ID == METHOD_INVITE && pCR->bIsReceiveACK==true) RSAM_Delete_FirstRoute(pCR, strSendBuf); //MIN 2021.03.18 ADD    
#endif

#ifdef MPBX_MODE
//    RSAM_Add_LastRecordRouteTo200INVITE(pCR, strSendBuf);       // add 20161005
    RSAM_Add_1stRecordRouteTo200INVITE(pCR, strSendBuf);        // add 20161005
    RSAM_Delete_FirstRoute(pCR, strSendBuf);                    // add 20161018
#endif
    *nSendLen = (int)strlen(strSendBuf);

    // [JIRA AS-206] START
    if(*nSendLen >= MAX_BUF_SIZE)
    {
        Log.printf(LOG_ERR, "RSAM_MakeSendBuffer() nSendLen(%d) >= MAX_BUF_SIZE(%d) !!!\n", nSendLen, MAX_BUF_SIZE);
        return(false);
    }
    // [JIRA AS-206] END
    
    return(true);
}

//#pragma mark -
//#pragma mark SAM에서 수신한 메세지를 CSCF로 전송하기 위해 메세지를 분석/추가/삭제 하는 함수 들

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSAM_GetRoutingInfoFromChInfoHeader
 * CLASS-NAME     : -
 * PARAMETER INOUT: pCR - Call Register 구조체 포인터
 * RET. VALUE     : BOOL
 * DESCRIPTION    : SAM에서 수신한 메세지의 P-BT-ChInfo에서 CSCF로 전송하는데 필요한 데이터를 추출하는 함수
 * REMARKS        : P-BT-ChInfo: board=xxxx;channel=xxxx;stack=ORIG;route=xxx.xxx.xxx.xxx:5060;ip=ipv6
                    FIXIT -- IPv6 Info 추가 필요
 **end*******************************************************/
bool RSAM_GetRoutingInfoFromChInfoHeader(CR_DATA *pCR)
{
    char    strChInfo[128];
    char    *pRoute, *pPort, *ptr;
    char    *pStack;
    char    strRoute[256];
    
    if(LIB_GetStrFromAToB(pCR->strRcvBuf, "P-BT-ChInfo:", ASCII_CRLF, strChInfo, sizeof(strChInfo)) == false)
    {
        // 주로 481 메시지가 해당함 (stack에서 sam에 callback 하지 않고 바로 응답하는 경우 임
        // 이경우 O/T 구분을 할 수 없기 때문에 에러처리가 된다.
#ifdef DEBUG_MODE
        Log.color(COLOR_MAGENTA, LOG_LV2, "[RSAM] STACK auto-generated msg - no P-BT-ChInfo Header [%d] CSeq=%s !!\n", pCR->nSipMsg_ID, pCR->info.strCSeq);
#endif
        return(false);
    }
    
    LIB_delete_white_space(strChInfo);
    
#ifdef IPV6_MODE
    if((ptr = strstr(strChInfo, ";ip=ipv6")) != NULL) { pCR->bIPV6 = true; *ptr = '\0'; }    // ;ip 이하 삭제
#endif
    
    if((strstr(strChInfo, "board=") == NULL) || (strstr(strChInfo, "channel=") == NULL)
    || ((pStack = strstr(strChInfo, "stack=")) == NULL)
    || ((pRoute = strstr(strChInfo, "route=")) == NULL))
    {
        Log.printf(LOG_ERR, "[RSAM] P-BT-ChInfo: ERR(%s) !!\n", strChInfo);
        return(false);
    }
    
    // GET board & channel
    sscanf(strChInfo, "board=%d;channel=%d", (int *)&pCR->bd, (int *)&pCR->ch);

    // GET Route
#ifdef IPV6_MODE
    if(pCR->bIPV6)
    {
        strcpy(strRoute, pRoute + 6);   // +6 = strlen("route=")

        // V6에서 ':'이 없으면 무조건 에러... 
        if((pPort = strrchr(strRoute, ':')) == NULL)       // 마지막 ':'를 찾는다... V6의 경우 IP구분과 PORT 구분이 동일... 그렇지만 SAM에서 무조건 port를 뒤에 붙인다.(SPY->SAM도 동일)
        {
            Log.printf(LOG_ERR, "[RSAM] P-BT-ChInfo: No port info. in route ERR(%s) !!\n", strChInfo);
            return(false);
        }
        
        *pPort = '\0';
//        strcpy(pCR->info.strCscfIp, strRoute);
        snprintf(pCR->info.strCscfIp, sizeof(pCR->info.strCscfIp), "%s", strRoute);
        pCR->info.nCscfPort = atoi(pPort+1);        // ':' 다음 부터 port

        snprintf(pCR->info.strCscfAddr, sizeof(pCR->info.strCscfAddr), "%s:%d", pCR->info.strCscfIp, pCR->info.nCscfPort);
#ifdef DEBUG_MODE
        Log.color(COLOR_MAGENTA, LOG_LV1, "[RSAM] %s() IPv6 CSCF Addr=%s, IP=%s, PORT=%d\n", __FUNCTION__, pCR->info.strCscfAddr, pCR->info.strCscfIp, pCR->info.nCscfPort);
#endif
    }
    else
#endif
    {
        strcpy(strRoute, pRoute + 6);   // +6 = strlen("route=")

        if((pPort = strchr(strRoute, ':')) == NULL)
        {
            Log.printf(LOG_INF, "[RSAM] P-BT-ChInfo: No port info. in route ERR(%s) but continue !!\n", strChInfo);
            
//            strcpy(pCR->info.strCscfIp, strRoute);
            snprintf(pCR->info.strCscfIp, sizeof(pCR->info.strCscfIp), "%s", strRoute);
            pCR->info.nCscfPort = 5060;                 // set Default port
//            return(false);  -
        }
        else
        {
            *pPort = '\0';
//            strcpy(pCR->info.strCscfIp, strRoute);
            snprintf(pCR->info.strCscfIp, sizeof(pCR->info.strCscfIp), "%s", strRoute);
            pCR->info.nCscfPort = atoi(pPort+1);        // ':' 다음 부터 port
        }
        snprintf(pCR->info.strCscfAddr, sizeof(pCR->info.strCscfAddr), "%s:%d", pCR->info.strCscfIp, pCR->info.nCscfPort);
#ifdef DEBUG_MODE
        Log.printf(LOG_LV1, "[RSAM] %s() IPv4 CSCF Addr=%s, IP=%s, PORT=%d\n", __FUNCTION__, pCR->info.strCscfAddr, pCR->info.strCscfIp, pCR->info.nCscfPort);
#endif
    }

    
//Log.printf(LOG_LV2, "[RSAM] P-BT-ChInfo route=%s CSCF=%s:%d ip=%s\n", pCR->info.strCscfAddr, pCR->info.strCscfIp, pCR->info.nCscfPort, (pCR->bIPV6)?"ipv6":"ipv4");

    // GET O/T Info.
    if(pStack[6] == 'O')        // stack=ORIG
    {
        pCR->ot_type           = OT_TYPE_ORIG;
        pCR->info.nMyStackPort = C_ORIG_STACK_PORT;
#ifdef IPV6_MODE
        if(pCR->bIPV6)
        {
            strcpy(pCR->info.strMyStackIpPort, C_ORIG_STACK_IP_PORT_V6);
            strcpy(pCR->info.strMyStackIp,     C_ORIG_STACK_IP_V6);
        }
        else
#endif
        {
            strcpy(pCR->info.strMyStackIpPort, C_ORIG_STACK_IP_PORT);
            strcpy(pCR->info.strMyStackIp,     C_ORIG_STACK_IP);
        }
    }
    else if(pStack[6] == 'T')   // stack=TERM
    {
        pCR->ot_type           = OT_TYPE_TERM;
        pCR->info.nMyStackPort = C_TERM_STACK_PORT;
#ifdef IPV6_MODE
        if(pCR->bIPV6)
        {
            strcpy(pCR->info.strMyStackIpPort, C_TERM_STACK_IP_PORT_V6);
            strcpy(pCR->info.strMyStackIp,     C_TERM_STACK_IP_V6);
        }
        else
#endif
        {
            strcpy(pCR->info.strMyStackIpPort, C_TERM_STACK_IP_PORT);
            strcpy(pCR->info.strMyStackIp,     C_TERM_STACK_IP);
        }
    }
    else
    {
        Log.printf(LOG_ERR, "[RSAM] P-BT-ChInfo: ORIG/TERM ERR(%d) !!\n", strChInfo);
        return(false);
    }

    pCR->audit.bPBTChInfo = true;
#ifdef IPV6_MODE
    Log.printf(LOG_LV1, "[RSAM] P-BT-ChInfo bd=%d, ch=%d, OT=%c, route=%s %s\n", pCR->bd, pCR->ch, pStack[6], pCR->info.strCscfAddr, (pCR->bIPV6)?"ip=ipv6":"");
#else
    Log.printf(LOG_LV1, "[RSAM] P-BT-ChInfo bd=%d, ch=%d, OT=%c, route=%s\n", pCR->bd, pCR->ch, pStack[6], pCR->info.strCscfAddr);
#endif
    return(true);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSAM_GetRoutingInfoFromHashTable
 * CLASS-NAME     : -
 * PARAMETER INOUT: pCR - Call Register 구조체 포인터
 * RET. VALUE     : BOOL
 * DESCRIPTION    : HashTable 에서 CSCF로 전송하는데 필요한 데이터를 추출하는 함수
 * REMARKS        : FIXIT - HashTable에 있는 데이터를 여기서 이용할 필요가 있을까?
 *                :         없는 경우에 Insert를 하고(나중에 쓰기 위해) 있으면 그냥 넘어가는 것이 어떤지??
 **end*******************************************************/
bool RSAM_GetRoutingInfoFromHashTable(CR_DATA *pCR)
{
    HASH_ITEM   hashTable;

	// 20170724 START
    if((pCR->audit.bPBTChInfo == false) && (pCR->nSipMsg_ID == METHOD_ACK))
    {
        if(g_Hash_T.find_hash(pCR->info.strCall_ID, &hashTable))
        {
            pCR->info.bExistInHashTable = true;
            pCR->info.nTrace = hashTable.nTrace;
            

                pCR->bd             = hashTable.BoardNo;
                pCR->ch             = hashTable.ChannelNo;
                //                pCR->info.nTrace    = hashTable.nTrace;
                pCR->info.nCscfPort = hashTable.nCscfPort;
#ifdef IPV6_MODE
                if(hashTable.bIPv6)
                {
                    pCR->bIPV6             = true;
                    strcpy(pCR->info.strMyStackIpPort, C_TERM_STACK_IP_PORT_V6);
                    strcpy(pCR->info.strMyStackIp,     C_TERM_STACK_IP_V6);
                }
                else
#endif
                {
                    strcpy(pCR->info.strMyStackIpPort, C_TERM_STACK_IP_PORT);
                    strcpy(pCR->info.strMyStackIp,     C_TERM_STACK_IP);
                }
                
                pCR->info.nMyStackPort = C_TERM_STACK_PORT;
                
                //                strcpy(pCR->info.strCscfIp, hashTable.strCscfIp);
                snprintf(pCR->info.strCscfIp, sizeof(pCR->info.strCscfIp), "%s", hashTable.strCscfIp);
                snprintf(pCR->info.strCscfAddr, sizeof(pCR->info.strCscfAddr), "%s:%d", pCR->info.strCscfIp, pCR->info.nCscfPort);
        }
        return(true);
    }
	// 20170724 END

    if(pCR->ot_type == OT_TYPE_TERM)
    {
        if(g_Hash_T.find_hash(pCR->info.strCall_ID, &hashTable))
        {
            pCR->info.bExistInHashTable = true;
            pCR->info.nTrace = hashTable.nTrace;
            
            if(pCR->audit.bPBTChInfo == false)  // P-BT-ChInfo가 없었으면 Routing 정보를 Hash에서 저장
            {
                pCR->bd             = hashTable.BoardNo;
                pCR->ch             = hashTable.ChannelNo;
//                pCR->info.nTrace    = hashTable.nTrace;
                pCR->info.nCscfPort = hashTable.nCscfPort;
#ifdef IPV6_MODE
                if(hashTable.bIPv6)
                {
                    pCR->bIPV6             = true;
                    strcpy(pCR->info.strMyStackIpPort, C_TERM_STACK_IP_PORT_V6);
                    strcpy(pCR->info.strMyStackIp,     C_TERM_STACK_IP_V6);
                }
                else
#endif
                {
                    strcpy(pCR->info.strMyStackIpPort, C_TERM_STACK_IP_PORT);
                    strcpy(pCR->info.strMyStackIp,     C_TERM_STACK_IP);
                }

                pCR->info.nMyStackPort = C_TERM_STACK_PORT;

//                strcpy(pCR->info.strCscfIp, hashTable.strCscfIp);
                snprintf(pCR->info.strCscfIp, sizeof(pCR->info.strCscfIp), "%s", hashTable.strCscfIp);
                snprintf(pCR->info.strCscfAddr, sizeof(pCR->info.strCscfAddr), "%s:%d", pCR->info.strCscfIp, pCR->info.nCscfPort);
            }
        }
        else
        {
            // SAM -> SPY로 오는 Initial Message는 전부 TERM 이다.
            // FIXIT: SAM에서 new call-leg info를 P-header로 보내주는 방향으로 수정 할 것
            //        외냐하면 SPY는 INVITE/ReINVITE를 구분못하지만 SAM은 할 수 있음
            switch(pCR->nSipMsg_ID)
            {
                case METHOD_INVITE:
                case METHOD_MESSAGE:
                case METHOD_SUBSCRIBE:
                case METHOD_REGISTER:
                    if((pCR->audit.bPBTChInfo == false) || (HASH_InsertAndSync(pCR, time(NULL))) == false)
                    {
                        pCR->info.bExistInHashTable = false;
#ifdef IPV6_MODE
                        if(pCR->bIPV6) { statistic_v6.sam.n_hash_fail ++; }
                        else
#endif
                        {
                            statistic.sam.n_hash_fail ++;
                        }
                        Log.printf(LOG_ERR, "[%d][%5d] RSAM_GetRoutingInfoFromHashTable() Initial Message has no P-BT-ChInfo or Hash Inser Fail MsgId = %d!!\n", pCR->bd, pCR->ch, pCR->nSipMsg_ID);
                        return(false);
                    }
                    break;
                    
                default:
                    pCR->info.bExistInHashTable = false;
#ifdef IPV6_MODE
                    if(pCR->bIPV6) { statistic_v6.sam.n_hash_fail ++; }
                    else
#endif
                    {
                        statistic.sam.n_hash_fail ++;
                    }
                    
                    // Hash에 없어도 P-BT-ChInfo등으로 인해 경우에 따라서 전송이 가능하다.
                    Log.color(COLOR_RED, LOG_LV3, "[%d][%5d] RSAM_GetRoutingInfoFromHashTable() Not Found in T_Hash Table MsgId=%d !!\n", pCR->bd, pCR->ch, pCR->nSipMsg_ID);
                    return(false);
            }
        }
    }
    else
    {
        if (g_Hash_O.find_hash(pCR->info.strCall_ID, &hashTable))
        {
            pCR->info.bExistInHashTable = true;
            pCR->info.nTrace = hashTable.nTrace;
            
//#ifdef MPBX_MODE
#if defined(MPBX_MODE) || defined(SB_MODE)
            if(hashTable.bIsReceiveACK == true) { pCR->bIsReceiveACK = true; }      // 20161005
#endif
            if(pCR->audit.bPBTChInfo == false)  // P-BT-ChInfo가 없었으면 Routing 정보를 Hash에서 저장
            {
                pCR->bd             = hashTable.BoardNo;
                pCR->ch             = hashTable.ChannelNo;
//                pCR->info.nTrace    = hashTable.nTrace;
                pCR->info.nCscfPort = hashTable.nCscfPort;
#ifdef IPV6_MODE
                if(hashTable.bIPv6)
                {
                    pCR->bIPV6 = true;
                    strcpy(pCR->info.strMyStackIpPort, C_ORIG_STACK_IP_PORT_V6);
                    strcpy(pCR->info.strMyStackIp,     C_ORIG_STACK_IP_V6);
                }
                else
#endif
                {
                    strcpy(pCR->info.strMyStackIpPort, C_ORIG_STACK_IP_PORT);
                    strcpy(pCR->info.strMyStackIp,     C_ORIG_STACK_IP);
                }

                pCR->info.nMyStackPort = C_ORIG_STACK_PORT;

//                strcpy(pCR->info.strCscfIp, hashTable.strCscfIp);
                snprintf(pCR->info.strCscfIp, sizeof(pCR->info.strCscfIp), "%s", hashTable.strCscfIp);
                snprintf(pCR->info.strCscfAddr, sizeof(pCR->info.strCscfAddr), "%s:%d", pCR->info.strCscfIp, pCR->info.nCscfPort);

            }
        }
        else
        {
            // ORIG로는 SAM -> SPY Initial Message가 없다.
            pCR->info.bExistInHashTable = false;
#ifdef IPV6_MODE
            if(pCR->bIPV6) { statistic_v6.sam.n_hash_fail ++; }
            else
#endif
            {
                statistic.sam.n_hash_fail ++;
            }
            // Hash에 없어도 P-BT-ChInfo등으로 인해 경우에 따라서 전송이 가능하다.
            Log.color(COLOR_RED, LOG_LV3, "[%d][%5d] RSAM_GetRoutingInfoFromHashTable() Not Found in O_Hash Table SipMsg_ID=%d CSeq=%s!!\n", pCR->bd, pCR->ch, pCR->nSipMsg_ID, pCR->info.strCSeq);
            return(false);
        }
    }
    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSAM_GetRoutingInfo
 * CLASS-NAME     : -
 * PARAMETER INOUT: pCR - Call Register 구조체 포인터
 * RET. VALUE     : BOOL
 * DESCRIPTION    : SAM에서 수신한 메세지와 HashTable에서 CSCF로 전송하는데 필요한 데이터를 추출하는 함수
 * REMARKS        :
 **end*******************************************************/
bool RSAM_GetRoutingInfo(CR_DATA *pCR)
{
    // FIXME: - P-BT-ChInfo 정보를 가져왔는지 변수에 넣어서 HashTable을 뒤지지 않게 변경 (단, Trace에 대한 정보를 SAM에서 관리하게 수정 필요)
    // FIXME: P-BT-ChInfo가 없는 경우 SSW로 보낼 수 없다 (Hash에도 없기 때문) -> RV Stack에서 오는 481인 경우가 많은데.. Via를 보고 보낼 것인지 결정이 필요
    RSAM_GetRoutingInfoFromChInfoHeader(pCR);   // P-BT-ChInfo: 에서, board, channel, route, ipv6 정보를 가져옴 (모든 메시지에 존재하는 P-헤더)

    if(RSAM_GetRoutingInfoFromHashTable(pCR) == false)          // Hash Table에서 CSCF 정보를 가져온다.
    {
        
#if 0   // 20170112: - 해당 정보가 Hash에 없는 경우 ACK와 200(BYE)만 전송하고 나머지는 전송하지 않는다.
        if(C_HASH_FAIL_ROUTE == false) { return(false); }       // Hash에 없으면 복구하지 않고 버린다... (버리는게 더 좋은 경우도 있음.. 재전송 방지 등)
#else
        if(pCR->nSipMsg_ID != METHOD_ACK)
        {
           if((pCR->nSipMsg_ID == 200) && (strstr(pCR->info.strCSeq, "BYE") != NULL)) { /* 200-BYE continue send */ }
           else if((pCR->nSipMsg_ID == 200) && (strstr(pCR->info.strCSeq, "MESSAGE") != NULL)) { /* 200-MESSAGE continue send */ } // 20240520 kdh - SoIP MESSAGE response 처리보완
           else
           {
               // ACK도 아니고 200-BYE도 아니면 전송하지 않음
               Log.color(COLOR_MAGENTA, LOG_LV3, "[%d][%5d] RSAM_GetRoutingInfo() Can't found CSCF info. in HashTable  Method=%d, CSeq=%s !!\n", pCR->bd, pCR->ch, pCR->nSipMsg_ID, pCR->info.strCSeq);
               return(false);
           }
        }
#endif
        
        if(pCR->audit.bPBTChInfo == false) { return(false); }   // P-BT-ChInfo도 없고, Hash도 없다.. 해결 불가능
    }

    // FIXME: CANCEL에 대한 ACK 인 경우에 SAM에서 P-BT-ChInfo를 붙이지 않고, 타이밍상으로 SCM에서 채널 해제를 수신했을 수도 있다...
    //        이 경우에도 ACK를 CSCF로 보내려면 Via나, Route등을 가지고 메시지를 보낼 수 없나 검토 필요, 방법이 없으면 시나리오에서 지연해서 채널해제를 해야 함
    
    // 메세지를 보내야 할 Destiantion(CSCF) 주소를 구하지 못한 경우에는 메세지를 보낼 수 없다.
    if(pCR->info.strCscfIp[0] == '\0')
    {
        Log.printf(LOG_ERR, "[RSAM] Can't get CSCF IP from P-BT-ChInfo & HashTable !!\n");
        return(false);
    }
    
    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSAM_ReceivedSipParsing
 * CLASS-NAME     : -
 * PARAMETER INOUT: pCR - Call Register 구조체 포인터
 * RET. VALUE     : BOOL
 * DESCRIPTION    : SAM에서 수신한 SIP 메세지를 분석하느 함수
 * REMARKS        : Receive SIP from SAM
 **end*******************************************************/
bool RSAM_ReceivedSipParsing(CR_DATA *pCR)
{
    /* -  UDP 수신시 검사 했음
    if(LIB_GetSipTypeAndMethod(pCR->strRcvBuf, (int *)&pCR->nSipMsgType, &pCR->nSipMsg_ID) == false)
    {
        Log.head(LOG_ERR, "RSAM_ReceivedSipParsing() SIP Start Line Error =", pCR->strRcvBuf, 64);
        return(false);
    }
     */
    
    
    if(LIB_GetSipHeaderValue(pCR->strRcvBuf, "\r\nCSeq:", pCR->info.strCSeq, sizeof(pCR->info.strCSeq)) == false)
    {
        Log.printf(LOG_ERR, "RSAM_ReceivedSipParsing() Can't Get CSeq Header !!\n");
        return(false);
    }
    
    if(LIB_GetSipHeaderValue(pCR->strRcvBuf, "\r\nCall-ID:", pCR->info.strCall_ID, sizeof(pCR->info.strCall_ID)) == false)
    {
        Log.printf(LOG_ERR, "RSAM_ReceivedSipParsing() Can't Get Call-ID Header !!\n");
        return(false);
    }
    
    if(RSAM_GetRoutingInfo(pCR) == false) { return(false); }        // V6 정보는 여기에서 세팅 됨
    
    switch(pCR->nSipMsgType)
    {
        case SIP_TYPE_REQUEST:
#ifdef IPV6_MODE
            if(pCR->bIPV6) { statistic_v6.sam.n_request[pCR->nSipMsg_ID] ++; }
            else
#endif
            {
                statistic.sam.n_request[pCR->nSipMsg_ID] ++;
            }
            break;
            
        case SIP_TYPE_RESPONSE:
#ifdef IPV6_MODE
            if(pCR->bIPV6) { statistic_v6.sam.n_response_code[pCR->nSipMsg_ID] ++; }
            else
#endif
            {
                statistic.sam.n_response_code[pCR->nSipMsg_ID] ++;
            }
            break;
            
        default:
            Log.printf(LOG_ERR, "RSAM_ReceivedSipParsing() undefined SipMsgType !!\n");
            return(false);
    }

#if 0
    if(C_OUTBOUND_PROXY)      // Outbound Proxy가 설정되어 있으면 무조건 Proxy로 보냄(테스트 등등 특수용도로 사용)
    {
        strcpy(pCR->info.strCscfIp, C_OUTBOUND_IP);
        pCR->info.nCscfPort = 5060;
    }
#endif
    
    return(true);
}


//#pragma mark -
//#pragma mark QUEUE에서 메세지를 꺼내 와서 CSCF로 전송하는 함수 들


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSAM_SendMessageToSSW
 * CLASS-NAME     : -
 * PARAMETER    IN: pCR - Call Register 구조체 포인터
 * RET. VALUE     :
 * DESCRIPTION    : SAM에서 수신한 SIP 메세지를 SSW로 전송하는 함수
 * REMARKS        : UDP 수신시 할당한 메모리를 여기서 free해야 함.
 **end*******************************************************/
void RSAM_SendMessageToSSW(CR_DATA *pCR)
{
    int     ret;
    char    strSendBuf[SAFETY_MAX_BUF_SIZE];	// [JIRA AS-206] - 20170719   
    int     nSendLen = 0;
    
    if(RSAM_ReceivedSipParsing(pCR) == false)
    {
        g_CR.free(pCR);      // free - UDP 수신시 alloac한 메모리
        return;
    }

    bzero(strSendBuf, sizeof(strSendBuf));
//    strSendBuf[0] = '\0';   // set NULL
    if(RSAM_MakeSendBuffer(pCR, &nSendLen, strSendBuf) == false)
    {
        Log.printf(LOG_ERR, "[RSAM] Make Send Buffer ERROR !!!\n");
        g_CR.free(pCR);      // free - UDP 수신시 alloac한 메모리
        return;
    }
   
Log.color(COLOR_YELLOW, LOG_LV1, "[RSAM] after RSAM_MakeSendBuffer(), nSendLen=%d\n", nSendLen);
 
#ifdef IPV6_MODE
    if(pCR->bIPV6)
    {
        ret = rawUDP_SendByNameNoCRC_V6(pCR->info.strMyStackIp, pCR->info.nMyStackPort, pCR->info.strCscfIp, pCR->info.nCscfPort, strSendBuf, nSendLen, C_NETWORK_QOS);
        Log.color(COLOR_YELLOW, LOG_LV1, "[RSAM][%d] rawUDP_SendByNameNoCRC_V6(%s:%d -> %s:%d, strSendBuf, %d, %d)\n", ret, pCR->info.strMyStackIp, pCR->info.nMyStackPort, pCR->info.strCscfIp, pCR->info.nCscfPort, nSendLen, C_NETWORK_QOS);
        if(ret > 0) { ret = 0;  }   // FIXME: V6 함수에서는 전송한 BYTE가 결과로 return된다.
        else        { ret = -1; }
    }
    else
    {
        ret = rawUDP_SendByNameNoCRC(pCR->info.strMyStackIp, pCR->info.nMyStackPort, pCR->info.strCscfIp, pCR->info.nCscfPort, strSendBuf, nSendLen, C_NETWORK_QOS);
        Log.color(COLOR_YELLOW, LOG_LV1, "[RSAM][%d] rawUDP_SendByNameNoCRC(%s:%d -> %s:%d, strSendBuf, %d, %d)\n", ret, pCR->info.strMyStackIp, pCR->info.nMyStackPort, pCR->info.strCscfIp, pCR->info.nCscfPort, nSendLen, C_NETWORK_QOS);
    }
#else
    ret = rawUDP_SendByNameNoCRC(pCR->info.strMyStackIp, pCR->info.nMyStackPort, pCR->info.strCscfIp, pCR->info.nCscfPort, strSendBuf, nSendLen, C_NETWORK_QOS);

    Log.color(COLOR_YELLOW, LOG_LV1, "[RSAM][%d] rawUDP_SendByNameNoCRC(%s:%d -> %s:%d, strSendBuf, %d, %d)\n", ret, pCR->info.strMyStackIp, pCR->info.nMyStackPort, pCR->info.strCscfIp, pCR->info.nCscfPort, nSendLen, C_NETWORK_QOS);
#endif  // IPV6_MODE

    if(ret == 0)
    {
#ifdef IPV6_MODE
        if(pCR->bIPV6) { statistic_v6.sam.n_out_sum[pCR->ot_type] ++; }
        else
#endif
        {
            statistic.sam.n_out_sum[pCR->ot_type] ++;   // OT 구분
        }
        
        if (C_LOG_LEVEL == LOG_LV1)
        {
            Log.color(COLOR_YELLOW, LOG_LV1, "[RSAM] >>>--->>> Send(%d) Message To=[%s] >>>--->>>\n%s\n", nSendLen, pCR->info.strCscfAddr, strSendBuf);
        }
        else
        {
            // FIXME: LV3 or LV2 뭘로 할지??
            if (pCR->nSipMsgType == SIP_TYPE_RESPONSE)
                Log.color(COLOR_YELLOW, LOG_LV2, "[RSAM] >>>--->>> Send(%d) RESPONSE [%d] To=[%s] Call-ID=[%s] >>>--->>>\n", nSendLen, pCR->nSipMsg_ID, pCR->info.strCscfAddr, pCR->info.strCall_ID);
            else
                Log.color(COLOR_YELLOW, LOG_LV2, "[RSAM] >>>--->>> Send(%d) REQUEST  [%s] To=[%s] Call-ID=[%s] >>>--->>>\n", nSendLen, STR_SIP_METHOD[pCR->nSipMsg_ID], pCR->info.strCscfAddr, pCR->info.strCall_ID);
        }
        
        if((0 <= pCR->info.nTrace) && (pCR->info.nTrace < MAX_TRACE_LIST))       // trace index: 0 ~ 9 or -1(trace not exist)
        {
            g_Trace.trace(pCR->info.nTrace, pCR->bd, pCR->ch, pCR->ot_type, pCR->info.strCscfIp, pCR->info.nCscfPort, strSendBuf);
        }
    }
    else
    {
        Log.printf(LOG_ERR, "[RSAM] RSAM_SendMessageToSSW(%d) errno=%d[%s]!!!\n", ret, errno, strerror(errno));
    }
    
    g_CR.free(pCR);      // free - UDP 수신시 alloac한 메모리
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSAM_DequeueThread
 * CLASS-NAME     : -
 * PARAMETER    IN: arg - ses_no
 * RET. VALUE     : NULL
 * DESCRIPTION    : SAM에서 수신한 SIP 메세지를 Queue에서 꺼내서 처리하는 함수
 * REMARKS        : Receive SIP from SAM
 **end*******************************************************/
void* RSAM_DequeueThread(void* arg)
{
// 20241205 kdh add m4.4.1-v2.1.2 MULTI_Q_MODE
//    size_t  ses_no = (size_t)arg;
	SAMQ_TH_INFO *thInfo = (SAMQ_TH_INFO*)arg;

    int     count = 0;
    CR_DATA     *pCR = NULL;
    NSM_PTR_Q   *ptrQ;
    
    Log.printf(LOG_INF, "[RSAM] RSAM_DequeueThread SES_%d START !!!\n", thInfo->bd);

#ifdef MULTI_Q_MODE // 20241205 kdh add m4.4.1-v2.1.2
    ptrQ = &Q_Sam[thInfo->bd][thInfo->queueNo];
#else
    ptrQ = &Q_Sam[thInfo->bd];
#endif // MULTI_Q_MODE
    while(1)
    {
        if(g_nHA_State == HA_Active)
        {
//            if(Q_Sam[ses_no].is_empty()) { msleep(10); count = 0; continue; }
        
            if(ptrQ->dequeue((void **)&pCR) == false)        // queue empty
            {
                msleep(1);     // 10 msec
                count = 0;      // 연속처리 count RESET
                continue;
            }

            RSAM_SendMessageToSSW(pCR);
			// SCM 채널할당 문제로 여기서 pCR을 free() 할 수 없음
            
            if(++count < MAX_DEQUEUE_COUNT_SAM) { continue; }    // 메세지가 많을 경우 연속 MAX_DEQUEUE_COUNT_SAM 개 까지만 처리하고  sleep 하기 위한 루틴
        }

//		Log.printf(LOG_INF, "@#@# TEST [RSAM] RSAM_DequeueThread SES_%d 5000 over !!!\n", ses_no);
        
        msleep(1);         // Active 가 아닌 경우 또는 MAX_DEQUEUE_COUNT_SAM 이상 연속처리한 경우  sleep 후 retry ...
        count = 0;         // 연속처리 count RESET
        continue;
    }
    
    return(NULL);
}

//#pragma mark -
//#pragma mark SIP Server - SAM에서 SIP 메세지를 수신하고 QUEUE에 넣는 함수 들

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSAM_CheckSesAddress
 * CLASS-NAME     : -
 * PARAMETER    IN: address - UDP(SIP)메세지를 수신한 Address
 * RET. VALUE     : -1(ERR)/SES#
 * DESCRIPTION    : 입력 주소가 SES Address인지 검사하는 함수
 * REMARKS        :
 **end*******************************************************/
int RSAM_CheckSesAddress(uint32_t address)
{
    int index;
    
    for(index = 0; index < C_SES_BOARD; index++)
	{
		if(address == C_SES_INET_ADDR[index]) { return(index); }
	}
    
    return(-1);
}

#ifdef MULTI_Q_MODE // 20241205 kdh add m4.4.1-v2.1.2
int RSAM_GetSamQNo(int ch)
{
    int threadNo;

    threadNo = 0;

    threadNo = ch % C_MAX_RSAM_SES_THREAD;

    return threadNo;
}

int RSAM_GetChFromViaHeader(CR_DATA *pCR)
{
    char strChInfo[512];
    char* ptr;
    int ch;

    ch = -1;

    memset(strChInfo, 0x00, sizeof(strChInfo));

    if(LIB_GetStrFromAToB(pCR->strRcvBuf, "Via:", ASCII_CRLF, strChInfo, sizeof(strChInfo)) == false)
    {
        // 주로 481 메시지가 해당함 (stack에서 sam에 callback 하지 않고 바로 응답하는 경우 임
        // 이경우 O/T 구분을 할 수 없기 때문에 에러처리가 된다.
        #ifdef DEBUG_MODE
        Log.color(COLOR_MAGENTA, LOG_LV2, "RSAM_GetChFromViaHeader TEST Err parsing Via!!\n");
        #endif 
        return ch;
    }
    
    LIB_delete_white_space(strChInfo);
    
    if((ptr = strstr(strChInfo, "-ch")) == NULL)
    {   
        Log.printf(LOG_ERR, "RSAM_GetChFromViaHeader TEST Via: bd-ch ERR(%s) !!\n", strChInfo);
        return ch;
    }
    
    ptr += 3;
    
    sscanf(ptr, "%5d", &ch);
    return ch;
}

int RSAM_GetChFromPBTHeader(CR_DATA *pCR)
{
    char strChInfo[128];
    int bd, ch;

    bd = -1;
    ch = -1;

    memset(strChInfo, 0x00, sizeof(strChInfo));

    if(LIB_GetStrFromAToB(pCR->strRcvBuf, "P-BT-ChInfo:", ASCII_CRLF, strChInfo, sizeof(strChInfo)) == false)
    {
        // 주로 481 메시지가 해당함 (stack에서 sam에 callback 하지 않고 바로 응답하는 경우 임
        // 이경우 O/T 구분을 할 수 없기 때문에 에러처리가 된다.
        #ifdef DEBUG_MODE
        Log.color(COLOR_MAGENTA, LOG_LV2, "RSAM_GetChFromPBTHeader TEST Err parsing P-BT-ChInfo!!\n");
        #endif
        return ch;
    }

    LIB_delete_white_space(strChInfo);

    if((strstr(strChInfo, "board") == NULL) || (strstr(strChInfo, "channel") == NULL))
    {
        Log.printf(LOG_ERR, "RSAM_GetChFromPBTHeader TEST P-BT-ChInfo: bd-ch ERR(%s) !!\n", strChInfo);
        return ch;
    }

    sscanf(strChInfo, "board=%d;channel=%d",(int *)&bd, (int *)&ch);

	return ch;
}
#endif // MULTI_Q_MODE

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSAM_ServerStart
 * CLASS-NAME     : -
 * PARAMETER      : -
 * RET. VALUE     : 0
 * DESCRIPTION    : SAM에서 SIP 메세지를 수신하여 Queue에 push하는 함수
 * REMARKS        : HA_Active인 경우에 실행 됨
 **end*******************************************************/
int RSAM_ServerStart(void)
{
    int     socket;
    int     nRcv;
    int     ses_no = -1;
	int     err_cnt = 0;
    int        samQNo = 0; // 20241205 kdh add  m4.4.1-v2.1.2 MULTI_Q_MODE
    int        rcvCh = -1; // 20241205 kdh add  m4.4.1-v2.1.2 MULTI_Q_MODE
    char    comment[64];
    CR_DATA *pCR;
    NSM_PTR_Q  *ptrQ; // 20241205 kdh add m4.4.1-v2.1.2 MULTI_Q_MODE


	//Make UDP Server for SAM
	if((socket = UDP_MakeServer(C_SAM_PORT)) < 0)
//	if((socket = UDP_MakeServer(SAM_UDP_SERVER_PORT)) < 0)
	{
		Log.printf(LOG_ERR, "[RSAM] UDP-Server create Error! (port=%d)\n", C_SSW_PORT);
		shut_down(0);
	}
    
	nRcv = 16 * 1024 * 1024; // 16777216
	setsockopt(socket, SOL_SOCKET, SO_RCVBUF, &nRcv, sizeof(nRcv));

	while(1)
	{
		if(g_nHA_State != HA_Active)            // HA 상태 변경시 Server Close
		{
			close(socket);

			Log.printf(LOG_INF, "[RSAM] close UDP Server - HA State Change Active -> %d !!!\n", g_nHA_State);
            return(0);
		}
        
        if((pCR = g_CR.alloc()) == NULL)     // pCR Allocation
        {
            Log.printf(LOG_ERR, "[RSAM] CR_alloc() fail !!\n");
            continue;
        }

        pCR->info.nTrace = -1;              // Initialize 
        
		if((pCR->nRcvLen = UDP_ServerWaitMessageEx(socket, pCR->strRcvBuf, MAX_BUF_SIZE, &pCR->peer)) <= 0)
		{
			Log.printf(LOG_ERR, "[RSAM] Receiving UDP Message Error. (err=%d) (rtn=%d)\n", errno, pCR->nRcvLen);
            
			if (++err_cnt >= 3)
			{
				err_cnt = 0;    // RESET
				Log.printf(LOG_ERR, "[RSAM] UDP Socket Error#1. (err=%d) Reseting socket... (count=%d,%d)\n", errno, err_cnt, g_nSamServerResetCount);
				close(socket);
               
#ifdef OAMSV5_MODE
				char    szCommentV5[8][64];
				memset(szCommentV5, 0x00, sizeof(szCommentV5));
				sprintf(szCommentV5[0], "SPY RSAM Server Socket Error(count=%d)", g_nSamServerResetCount);

				SEND_AlarmV2(MSG_ID_FLT, FLT_SPY_RSAM_UDP_SERVER, MY_BLK_ID, FLT_SPY_RSAM_UDP_SERVER, 0, ALARM_ON, 1, szCommentV5); 
#else
                snprintf(comment, sizeof(comment), "SPY RSAM Server Socket Error(count=%d)", g_nSamServerResetCount);
                SEND_FaultMessage(FLT_SPY_RSAM_UDP_SERVER, ALARM_ON, 0, 2, comment);
#endif
                
                // 9번 이상 (3 * 3) 연속으로 UDP Read 실패가 발생하면 SPY를 Down 시키고 다시 실행 시킨다.
                if (++g_nSamServerResetCount >= 3) { shut_down(0); }
                g_CR.free(pCR);
                return(0);
			}
			continue;
		}
		else if(pCR->nRcvLen >= MAX_BUF_SIZE)   // strRcvBuf[MAX_BUF_SIZE+1] : 추가할 NULL 고려되어 있음
		{
			Log.printf(LOG_ERR, "[RSAM] Receiving UDP Message Size(%d) Error. from %s !!\n", pCR->nRcvLen, inet_ntoa(pCR->peer.sin_addr));
            g_CR.free(pCR);
			continue;
		}

		err_cnt                = 0;    // RESET
		g_nSamServerResetCount = 0;    // RESET

        pCR->strRcvBuf[pCR->nRcvLen] = '\0';    // set NULL to end of received Message
        
		if((ses_no = RSAM_CheckSesAddress(int(pCR->peer.sin_addr.s_addr))) < 0)
		{
			Log.printf(LOG_LV2, "[RSAM] SIP from unknown address[%s] - not SAM.\n", inet_ntoa(pCR->peer.sin_addr));
            g_CR.free(pCR);
			continue;
		}
        
        if(LIB_GetSipTypeAndMethod(pCR->strRcvBuf, (int *)&pCR->nSipMsgType, &pCR->nSipMsg_ID) == false)
        {
            Log.head(LOG_ERR, "[RSAM]  Received SIP Start Line Error =", pCR->strRcvBuf, 64);
            g_CR.free(pCR);
            continue;
        }

#if DEBUG_MODE
//        Log.color(COLOR_CYAN, LOG_LV2, "[RSAM] ---<<<--- Recv Message from SAM[%d] ---<<<--- Msg_ID=%d\n", ses_no, pCR->nSipMsg_ID);
        Log.color(COLOR_CYAN, LOG_LV1, "[RSAM] ---<<<--- Recv Message(%d) from SAM[%d] ---<<<---\n%s\n", pCR->nRcvLen, ses_no, pCR->strRcvBuf);
#endif

#ifdef MULTI_Q_MODE // 20241205 kdh add m4.4.1-v2.1.2
        if( (rcvCh = RSAM_GetChFromPBTHeader(pCR)) < 0)
            rcvCh = RSAM_GetChFromViaHeader(pCR);

        if(rcvCh < 0)
            rcvCh = 0;

        samQNo = RSAM_GetSamQNo(rcvCh);
        ptrQ = &Q_Sam[ses_no][samQNo];
        Log.color(COLOR_CYAN, LOG_LV1, "RSAM_ServerStart() TEST bd=%d, ch=%d, samQNo=%d\n", ses_no, rcvCh, samQNo);
#else
        ptrQ = &Q_Sam[ses_no];
#endif // MULTI_Q_MODE


        // Push to Queue
        if(ptrQ->enqueue(pCR) == false)
        {
            Log.printf(LOG_ERR, "[RSAM] SIP from SAM enqueue Fail!!!\n");
            g_CR.free(pCR);
        }
	}
    
    return(0);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSAM_ServerThread
 * CLASS-NAME     : -
 * PARAMETER    IN: arg - don't care
 * RET. VALUE     : NULL
 * DESCRIPTION    : SIP Server Thread main
 * REMARKS        : Receive SIP from SAM
 **end*******************************************************/
void* RSAM_ServerThread(void* arg)
{
    while(1)
    {
        if(g_nHA_State == HA_Active)
        {
            RSAM_ServerStart();
            Log.printf(LOG_INF, "[RSAM] RSAM_ServerThread() RSAM Server STOP !! Retry !!!\n");
        }

        // Active 가 아닌 경우 retry ...
        usleep(100);
        continue;
    }
    
    return(NULL);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSAM_main
 * CLASS-NAME     : -
 * PARAMETER      : -
 * RET. VALUE     : BOOL
 * DESCRIPTION    : Receive SIP message from SAM main function
 * REMARKS        :
 **end*******************************************************/
bool RSAM_main(void)
{
    size_t     index;
	SAMQ_TH_INFO*   argment; // 20241205 kdh add m4.4.1-v2.1.2 MULTI_Q_MODE
    pthread_attr_t  tattr;
    pthread_t       tid;
    sched_param     param;
    
    // pthread 우선 순위 변경
    pthread_attr_init (&tattr);                         // initialized with default attributes
    pthread_attr_getschedparam (&tattr, &param);        // safe to get existing scheduling param
    param.sched_priority = SPY_SERVER_THREAD_PRIORITY;  // set the priority; 14
    pthread_attr_setschedparam (&tattr, &param);        // setting the new scheduling param
    
    // Make Server Thread
	pthread_create(&tid, &tattr, &RSAM_ServerThread, NULL);
	pthread_detach(tid);
    
    for(index = 0; index < C_SES_BOARD; index ++)       // SES board 수 만큼 Thread 생성
    {
        for(int i = 0; i < C_MAX_RSAM_SES_THREAD; i ++)
        {
			// 20241205 kdh add m4.4.1-v2.1.2 MULTI_Q_MODE
            argment = (SAMQ_TH_INFO*)malloc(sizeof(SAMQ_TH_INFO));
            memset(argment, 0x00, sizeof(SAMQ_TH_INFO));
            argment->bd = index;
            argment->queueNo = i;

            if(pthread_create(&tid, NULL, RSAM_DequeueThread, (void *)argment))
            {
                Log.printf(LOG_ERR, "[RSAM] Can't ceate thread for RSAM_DequeueThread(%s, #%d)\n", strerror(errno), index);
				free(argment);
                return(false);
            }
            pthread_detach(tid);
            usleep(1000);
        }
    }
    
    return(true);
}

