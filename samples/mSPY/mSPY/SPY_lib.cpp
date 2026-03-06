//
//  SPY_lib.cpp
//  SPY
//
//  Created by smcho on 11. 11. 30..
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include "SPY_lib.h"

//#define SAM_UDP_SERVER_PORT 5090

//#pragma mark -
//#pragma mark 문자열 관련 라이브러리

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_change_space_to_null
 * CLASS-NAME     :
 * PARAMETER      : INOUT
 * RET. VALUE     : -
 * DESCRIPTION    : 문자열에서 SPACE를  NULL로 변환
 * REMARKS        :
 **end*******************************************************/
void LIB_change_space_to_null(char *str, int len)
{
    for(int i = 0; i < len; i ++)
    {
        if(str[i] == ' ') { str[i] = '\0'; return; }
    }
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_delete_special_character
 * CLASS-NAME     : 
 * PARAMETER      : INOUT
 * RET. VALUE     : -
 * DESCRIPTION    : 문자열에서 특정 문자를 제거 
 * REMARKS        : 
 **end*******************************************************/
void LIB_delete_special_character(char *str, int value)
{
    char *s, *d;
    
    s = d = str;
    
    while(*s)
    {
        if(*s == value) { s++; }
        else            { *d++ = *s++; }
    }
    
    *d = '\0';
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_delete_special_character
 * CLASS-NAME     :
 * PARAMETER      : INOUT IN IN
 * RET. VALUE     : -
 * DESCRIPTION    : 문자열에서 특정 문자를 제거
 * REMARKS        : 제거할 문자가 2개('<', '>' 등 쌍으로 이루어진 기호를 삭제하는데 유용함
 **end*******************************************************/
void LIB_delete_special_character(char *str, int value, int value2)
{
    char *s, *d;
    
    s = d = str;
    
    while(*s)
    {
        if((*s == value) || (*s == value2)) { s++; }
        else                                { *d++ = *s++; }
    }
    
    *d = '\0';
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_DeleteStartSpace
 * CLASS-NAME     : 
 * PARAMETER      : INOUT
 * RET. VALUE     : -
 * DESCRIPTION    : 입력문자열의 시작부분 SPACE를 삭제 
 * REMARKS        : 문자열 중간의 SPACE는 삭제 안함
 **end*******************************************************/
void LIB_DeleteStartSpace(char *str)
{
    char    *s, *d;
    
    s = d = str;
    
    while(*s)
    {
        if(*s != ' ') { break; }
        else          { s++; }
    }
    
    if(s == d) { return; }  // no space
    
    while(*s) { *d++ = *s++; }
    
    *d = '\0';
}

//#pragma mark -
//#pragma mark SIP 메세지에서 URI(From/To/Request...) 관련 라이브러리

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_GetIpAddrInContactHeader
 * CLASS-NAME     :
 * PARAMETER    IN: strHeader - SIP Header에서 추출한 URI string
 *             OUT: strIpAddr - strHeader에서 추출한 IP ADDRESS
 *              IN: MAX_LEN   - strIpAddr의 최대 길이
 *             OUT: nPort     - strHeader에서 추출한 Port 번호(없으면 default 5060)
 * RET. VALUE     : BOOL
 * DESCRIPTION    : Contact: header에서 IP Address부분만 추출
 * REMARKS        : sip: or sips: 가 없는 경우 에러가 발생됨 (tel:은 에러)
 *                : FIXIT - IP가 아닌 Hostname인 경우 처리 안되어 있음 (DNS Query ???)
 **end*******************************************************/
bool LIB_GetIpAddrInContactHeader(const char *strHeader, char *strIpAddr, int MAX_LEN, int *nPort)
{
    char    *ptr, *pStart;
    char    *strCopiedHeader;
    size_t  len;
    
    /* RFC 3261
     Contact        =  ("Contact" / "m" ) HCOLON ( STAR / (contact-param *(COMMA contact-param)))
     contact-param  =  (name-addr / addr-spec) *(SEMI contact-params)
     name-addr      =  [ display-name ] LAQUOT addr-spec RAQUOT
     addr-spec      =  SIP-URI / SIPS-URI / absoluteURI
     display-name   =  *(token LWS)/ quoted-string
     absoluteURI    =  scheme ":" ( hier-part / opaque-part )
     
     SIP-URI        =  "sip:" [ userinfo ] hostport uri-parameters [ headers ]
     userinfo       =  ( user / telephone-subscriber ) [ ":" password ] "@"
     hostport       =  host [ ":" port ]
     host           =  hostname / IPv4address / IPv6reference
     
     IPv4address    =  1*3DIGIT "." 1*3DIGIT "." 1*3DIGIT "." 1*3DIGIT
     IPv6reference  =  "[" IPv6address "]"
     IPv6address    =  hexpart [ ":" IPv4address ]
    */
    
    ptr = (char *)strHeader;
    *nPort = 5060;      // default
    
    // Contact:의 시작부분에 display-name 이 있는지 확인, 있으면 skip
    // 따옴표는 항상 쌍으로 있어야 하기 때문에 두번째 따옴표 위치로 pointer 이동 (못찾으면 error)
    // display-name 안에 "sip:" or "sips:"이 있으면 IP를 찾는데 문제가 발생함
    while(*ptr == ' ') { ptr ++; }      // 문자열 시작부분의 SPACE 삭제
    if(*ptr == '"') { ptr ++; if((ptr = strchr((char *)ptr, '"')) == NULL) { return(false); } }
    
    pStart = ptr;   // display-name을 제외한 실제 Contact의 start
    
    // Contact에는 sip: sips: 또는 absoluteURI 만 가능하다.
    if     ((ptr = strcasestr(pStart, "sip:"))  != NULL) { ptr += 4; }
    else if((ptr = strcasestr(pStart, "sips:")) != NULL) { ptr += 5; }
    else                                                 { return(false); }
    while(*ptr == ' ') { ptr ++; }      // 문자열 시작부분의 SPACE 삭제
    
    // original header를 건드리지 않기 위해 copy해서 처리 함
    {
        len = strlen(ptr);
        strCopiedHeader = (char *)malloc(len+1);
        strcpy(strCopiedHeader, ptr);               // sip(s): 이하부터 copy
        
        if((ptr = strchr(strCopiedHeader, '>')) != NULL) { *ptr = '\0'; }   // '>' 이하 삭제
        if((ptr = strchr(strCopiedHeader, ';')) != NULL) { *ptr = '\0'; }   // ';' 이하 삭제
        
        // userinfo ('@'로 구분)이 있으면  skip
        if((ptr = strchr(strCopiedHeader, '@')) != NULL) { pStart = ptr+1; }
        else                                             { pStart = strCopiedHeader; }
        
        if((ptr = strchr(strCopiedHeader, ':')) != NULL) { *ptr = '\0'; *nPort = atoi(ptr+1); }   // port번호 (':') 이하 삭제
        
        if(strlen(pStart) >= MAX_LEN) { free(strCopiedHeader); return(false); }    // Buffer Length Error
        strcpy(strIpAddr, pStart);
        
        free(strCopiedHeader);
    }
    
    // IP인지 Hostname인지 확인하는 부분이 필요하면 여기 추가....
    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_GetUriFromHeader
 * CLASS-NAME     :
 * PARAMETER    IN: strHeader - SIP Header에서 추출한 URI string
 *             OUT: nUriType  - strHeader의 Uri Type (SIP or TEL)
 *             OUT: strURI    - strHeader에서 추출한 Address 부분
 *              IN: MAX_LEN   - strAddr의 최대 길이
 * RET. VALUE     : BOOL
 * DESCRIPTION    : From:/To: header에서 Address부분만 추출
 * REMARKS        : 대/소문자 구분 안함 (<sip:user@domain> => user@domain)
 *                :                 (tel:telno => telno)
 *                : sip: or tel: 이 없는 경우 에러가 발생됨
 **end*******************************************************/
bool LIB_GetUriFromHeader(const char *strHeader, URI_TYPE *nUriType, char *strURI, int MAX_LEN)
{
    char    *pHeader;
    int     len = 0;
    
    // if header does not include 'sip:' or 'tel:' then return false
    if     ((pHeader = strcasestr((char *)strHeader, "sip:"))  != NULL) { *nUriType = URI_TYPE_SIP; pHeader += 4; }
    else if((pHeader = strcasestr((char *)strHeader, "tel:"))  != NULL) { *nUriType = URI_TYPE_TEL; pHeader += 4; }
    else if((pHeader = strcasestr((char *)strHeader, "sips:")) != NULL) { *nUriType = URI_TYPE_SIP; pHeader += 5; } // FIXIT - SIPS도 일단 SIP으로 처리
    else                                                                { return(false); }  // error - no sip: or tel:
    
    while(*pHeader)
    {
        if(*pHeader == ' ') { pHeader++; continue; }    // skip SPACE
        if((*pHeader == '>') || (*pHeader == ':') || (*pHeader == ';')) { break; }
        
        if(++len >= MAX_LEN)    // NULL을 포함하기 위해 copy전에 크기 비교
        {
            Log.printf(LOG_ERR, "LIB_GetUriFromHeader() Addr size > MAX_LEN(%d)\n", MAX_LEN);
            return(false);
        }
        
        *strURI++ = *pHeader++;    // copy
    }
    
    *strURI = '\0';            // set NULL (EOS)
    
    
    return(true);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_GetUriFromHeader2
 * CLASS-NAME     :
 * PARAMETER    IN: strHeader - SIP Header에서 추출한 URI string
 *             OUT: strURI    - strHeader에서 추출한 Address 부분
 *              IN: MAX_LEN   - strAddr의 최대 길이
 * RET. VALUE     : BOOL
 * DESCRIPTION    : From:/To: header에서 sip/tel을 포함한 URI부분만 추출
 * REMARKS        :     (<sip:user@domain> => sip:user@domain)
 *                :     (tel:telno;tag...  => tel:telno)
 **end*******************************************************/
bool LIB_GetUriFromHeader2(const char *strHeader, char *strURI, int MAX_LEN)
{
    char    *pHeader;
    int     len = 0;
    
    // if header does not include 'sip:' or 'tel:' then return false
    if     ((pHeader = strcasestr((char *)strHeader, "sip:"))  != NULL) { strcpy(strURI, "sip:");  strURI += 4; pHeader += 4; }
    else if((pHeader = strcasestr((char *)strHeader, "tel:"))  != NULL) { strcpy(strURI, "tel:");  strURI += 4; pHeader += 4; }
    else if((pHeader = strcasestr((char *)strHeader, "sips:")) != NULL) { strcpy(strURI, "sips:"); strURI += 5; pHeader += 5; }
    else                                                   { return(false); }
    
    while(*pHeader)
    {
        if(*pHeader == ' ') { pHeader++; continue; }    // skip SPACE
        if((*pHeader == '>') || (*pHeader == ':') || (*pHeader == ';')) { break; }
        
        if(++len >= MAX_LEN)    // NULL을 포함하기 위해 copy전에 크기 비교
        {
            printf("LIB_GetUriFromHeader() Addr size > MAX_LEN(%d)\n", MAX_LEN);
            return(false);
        }
        
        *strURI++ = *pHeader++;    // copy
    }
    
    *strURI = '\0';            // set NULL (EOS)
    
    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_GetUriAndUserAndHostFromHeader
 * CLASS-NAME     :
 * PARAMETER    IN: strHeader    - SIP Header에서 추출한 URI string
 *             OUT: nUriType     - strHeader의 Uri Type (SIP or TEL)
 *             OUT: strURI       - strHeader에서 추출한 Address(User@Host) 부분
 *              IN: MAX_LEN      - strAddr의 최대 길이
 *             OUT: strUser      - strHeader에서 추출한 User 부분
 *              IN: MAX_USER_LEN - strUser의 최대 길이
 *             OUT: strHost      - strHeader에서 추출한 Host 부분
 *              IN: MAX_HOST_LEN - strHost의 최대 길이
 * RET. VALUE     : BOOL
 * DESCRIPTION    : <sip: user@host:xxx> bbb 에서 user@host, user, host부분을 추출하는 함수
 * REMARKS        : 대/소문자 구분 안함
 *                : ex) <tel/sip:user@host:xxx> strAddr=user@host, strUser=user, strHost=host
 *                :     <tel:user>              strAddr=user,      strUser=user, strHost=NULL
 *                :     <sip:host>              strAddr=host,      strUser=NULL, strHost=host
 **end*******************************************************/
bool LIB_GetUriAndUserAndHostFromHeader(const char *strHeader, URI_TYPE *nUriType, char *strURI, int MAX_LEN, char *strUser, int MAX_USER_LEN, char *strHost, int MAX_HOST_LEN)
{
    char    *pAt, *pHost;
    size_t  len = 0;
    
    if(LIB_GetUriFromHeader(strHeader, nUriType, strURI, MAX_LEN) == false) { return(false); }
    
    if((pAt = strstr(strURI, "@")) != NULL)    // User@Host case
    {
        pHost = pAt+1;
        
        if((len = pAt - strURI) >= MAX_USER_LEN)
        {
            Log.printf(LOG_ERR, "LIB_GetUriAndUserAndHostFromHeader() len(%d) >= MAX_USER_LEN(%d)\n", len, MAX_USER_LEN);
            return(false);
        }
        
        if((len = strlen(pHost)) >= MAX_HOST_LEN)
        {
            Log.printf(LOG_ERR, "LIB_GetUriAndUserAndHostFromHeader() len(%d) >= MAX_HOST_LEN(%d)\n", len, MAX_USER_LEN);
            return(false);
        }
        
        *pAt = '\0';        // set NULL for copy
        strcpy(strUser, strURI);
        strcpy(strHost, pHost);
        *pAt = '@';        // restore
    }
    else
    {
        switch (*nUriType)      // '@'가 없는 경우에는 URI Type에 따라 User만 있던지, Host만 있는 경우 임
        {
            case URI_TYPE_SIP:
                if((len = strlen(strURI)) >= MAX_HOST_LEN)
                {
                    Log.printf(LOG_ERR, "LIB_GetUriAndUserAndHostFromHeader() len(%d) >= MAX_HOST_LEN(%d)\n", len, MAX_HOST_LEN);
                    return(false);
                }
                
                strUser[0] = '\0';
                strcpy(strHost, strURI);
                break;
                
            case URI_TYPE_TEL:
                if((len = strlen(strURI)) >= MAX_USER_LEN)
                {
                    Log.printf(LOG_ERR, "LIB_GetUriAndUserAndHostFromHeader() len(%d) >= MAX_USER_LEN(%d)\n", len, MAX_USER_LEN);
                    return(false);
                }
                
                strcpy(strUser, strURI);
                strHost[0] = '\0';
                break;
                
            default: return(false);
        }
    }
    
    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_GetUserAndHostFromHeader
 * CLASS-NAME     :
 * PARAMETER    IN: strHeader    - SIP Header에서 추출한 URI string
 *             OUT: strUser      - strHeader에서 추출한 User 부분
 *              IN: MAX_USER_LEN - strUser의 최대 길이
 *             OUT: strHost      - strHeader에서 추출한 Host 부분
 *              IN: MAX_HOST_LEN - strHost의 최대 길이
 * RET. VALUE     : BOOL
 * DESCRIPTION    : <sip: user@host:xxx> bbb 에서 user@host, user, host부분을 추출하는 함수
 * REMARKS        : 대/소문자 구분 안함
 *                : ex) <tel/sip:user@host:xxx> strUser=user, strHost=host
 *                :     <tel:user>              strUser=user, strHost=NULL
 *                :     <sip:host>              strUser=NULL, strHost=host
 **end*******************************************************/
bool LIB_GetUserAndHostFromHeader(const char *strHeader, char *strUser, int MAX_USER_LEN, char *strHost, int MAX_HOST_LEN)
{
    char    strURI[256];
    char    *pAt, *pHost;
    ssize_t     len = 0;
    URI_TYPE    nUriType;
    
    if(LIB_GetUriFromHeader(strHeader, &nUriType, strURI, sizeof(strURI)) == false) { return(false); }
    
    if((pAt = strstr(strURI, "@")) != NULL)    // User@Host case
    {
        pHost = pAt+1;
        
        if((len = pAt - strURI) >= MAX_USER_LEN)
        {
            Log.printf(LOG_ERR, "LIB_GetUserAndHostFromHeader() len(%d) >= MAX_USER_LEN(%d)\n", len, MAX_USER_LEN);
            return(false);
        }
        
        if((len = strlen(pHost)) >= MAX_HOST_LEN)
        {
            Log.printf(LOG_ERR, "LIB_GetUserAndHostFromHeader() len(%d) >= MAX_HOST_LEN(%d)\n", len, MAX_HOST_LEN);
            return(false);
        }
        
        *pAt = '\0';        // set NULL for copy
        strcpy(strUser, strURI);
        strcpy(strHost, pHost);
        *pAt = '@';        // restore
    }
    else
    {
        switch (nUriType)      // '@'가 없는 경우에는 URI Type에 따라 User만 있던지, Host만 있는 경우 임
        {
            case URI_TYPE_SIP:
                if((len = strlen(strURI)) >= MAX_HOST_LEN)
                {
                    Log.printf(LOG_ERR, "LIB_GetUserAndHostFromHeader() len(%d) >= MAX_HOST_LEN(%d)\n", len, MAX_HOST_LEN);
                    return(false);
                }
                
                strUser[0] = '\0';
                strcpy(strHost, strURI);
                return(true);
                
            case URI_TYPE_TEL:
                if((len = strlen(strURI)) >= MAX_USER_LEN)
                {
                    Log.printf(LOG_ERR, "LIB_GetUserAndHostFromHeader() len(%d) >= MAX_USER_LEN(%d)\n", len, MAX_USER_LEN);
                    return(false);
                }
                
                strcpy(strUser, strURI);
                strHost[0] = '\0';
                return(true);
                
            default: return(false);
        }
    }
    
    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_GetUserFromHeader
 * CLASS-NAME     :
 * PARAMETER    IN: strHeader    - SIP Header에서 추출한 URI string
 *             OUT: strUser      - strHeader에서 추출한 User 부분
 *              IN: MAX_USER_LEN - strUser의 최대 길이
 * RET. VALUE     : BOOL
 * DESCRIPTION    : URI(<tel/sip:user@host:xxx>;tag=...;....) 문자열에서 user부분만 추출
 * REMARKS        : 대/소문자 구분 안함 <sip:host> 인 경우에는 strUser=host
 *                : 즉 Address(User[@Host]) 부분에서 '@' 앞 부분을 return
 **end*******************************************************/
bool LIB_GetUserFromHeader(const char *strHeader, char *strUser, int MAX_USER_LEN)
{
    char    strURI[256];
    char    *pAt;
    ssize_t     len = 0;
    URI_TYPE    nUriType;
    
    if(LIB_GetUriFromHeader(strHeader, &nUriType, strURI, sizeof(strURI)) == false) { return(false); }
    
    if((pAt = strstr(strURI, "@")) != NULL)    // User@Host case
    {
        if((len = pAt - strURI) >= MAX_USER_LEN)
        {
            printf("[ERROR] LIB_GetUserFromHeader() len(%zd) >= MAX_USER_LEN(%d)\n", len, MAX_USER_LEN);
            return(false);
        }
        
        *pAt = '\0';        // set NULL for copy
        strcpy(strUser, strURI);
        *pAt = '@';        // restore
    }
    else
    {
        switch (nUriType)      // '@'가 없는 경우에는 URI Type에 따라 User만 있던지, Host만 있는 경우 임
        {
            case URI_TYPE_SIP:
                strUser[0] = '\0';
                return(false);
                
            case URI_TYPE_TEL:
                if((len = strlen(strURI)) >= MAX_USER_LEN)
                {
                    printf("[ERROR] LIB_GetUserFromHeader() len(%zd) >= MAX_USER_LEN(%d)\n", len, MAX_USER_LEN);
                    return(false);
                }
                
                strcpy(strUser, strURI);
                return(true);
                
            default: return(false);
        }
    }
    
    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_GetHostFromHeader
 * CLASS-NAME     :
 * PARAMETER    IN: strHeader    - SIP Header에서 추출한 URI string
 *             OUT: strHost      - strHeader에서 추출한 Host 부분
 *              IN: MAX_HOST_LEN - strHost의 최대 길이
 * RET. VALUE     : BOOL
 * DESCRIPTION    : URI(<tel/sip:user@host:xxx>;tag=...;....) 문자열에서 host부분만 추출
 * REMARKS        : 대/소문자 구분 안함 <sip:host> 인 경우에는 strUser=host
 *                : 즉 Address(User[@Host]) 부분에서 '@' 뒷 부분을 return
 **end*******************************************************/
bool LIB_GetHostFromHeader(const char *strHeader, char *strHost, int MAX_HOST_LEN)
{
    char    strURI[256];
    char    *pAt, *pHost;
    size_t     len = 0;
    URI_TYPE    nUriType;
    
    if(LIB_GetUriFromHeader(strHeader, &nUriType, strURI, sizeof(strURI)) == false) { return(false); }
    
    if((pAt = strstr(strURI, "@")) != NULL)    // User@Host case
    {
        pHost = pAt+1;
        
        if((len = strlen(pHost)) >= MAX_HOST_LEN)
        {
            Log.printf(LOG_ERR, "LIB_GetHostFromHeader() len(%d) >= MAX_HOST_LEN(%d)\n", len, MAX_HOST_LEN);
            return(false);
        }

        strcpy(strHost, pHost);
    }
    else
    {
        switch (nUriType)      // '@'가 없는 경우에는 URI Type에 따라 User만 있던지, Host만 있는 경우 임
        {
            case URI_TYPE_SIP:
                if((len = strlen(strURI)) >= MAX_HOST_LEN)
                {
                    Log.printf(LOG_ERR, "LIB_GetHostFromHeader() len(%d) >= MAX_HOST_LEN(%d)\n", len, MAX_HOST_LEN);
                    return(false);
                }

                strcpy(strHost, strURI);
                return(true);
                
            case URI_TYPE_TEL:
                strHost[0] = '\0';
                return(false);
                
            default: return(false);
        }
    }
    
    return(true);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_GetAddrFromURI
 * CLASS-NAME     :
 * PARAMETER      : IN OUT IN OUT IN OUT
 * RET. VALUE     : BOOL
 * DESCRIPTION    : <sip: ...@...:xxx> bbb 에서 Address부분만 추출
 * REMARKS        : strUser@strHost 대/소문자 구분 안함
 **end*******************************************************/
bool LIB_GetAddrFromURI(const char *strURI, char *strAddr, int MAX_LEN, char *strUser, int MAX_USER_LEN, char *strHost)
{
    char    *ptrAddr;
    int     i, len, index;
    
    // header not include 'sip:' or 'tel:' then return FALSE
    // 20151208 bible 64bits 작업
    if((ptrAddr = strstr((char *)strURI, "sip:")) == NULL)
    {
        // 20151208 bible 64bits 작업
        if((ptrAddr = strstr((char *)strURI, "tel:")) == NULL) { return(false); }
    }
    
    ptrAddr += 4;               // 4: strlen("sip:")
    len = (int)strlen(ptrAddr);      // len: "sip:" 다음부터 NULL까지 문자열 길이
    
    if(len >= MAX_LEN)
    {
        Log.printf(LOG_ERR, "LIB_GetAddrFromURI() MAX_LEN Error len(%d) > %d\n", len, MAX_LEN);
        return(false);
    }
    
    for(i = 0, index = 0; i < len; i ++, ptrAddr ++)
    {
        if((*ptrAddr == '>') || (*ptrAddr == ';')) { break; }
        
        if(*ptrAddr != ' ')     // skip SPACE
        {
            strAddr[index++] = *ptrAddr;
        }
    }
    strAddr[index] = '\0';
    
    if(strstr(strAddr, "@"))
    {
        if((len = (int)strlen(strAddr)) >= MAX_USER_LEN)
        {
            Log.printf(LOG_ERR, "LIB_GetAddrFromURI() MAX_USER_LEN Error len(%d) > %d\n", len, MAX_USER_LEN);
            return(false);
        }
        
        strcpy(strUser, strAddr);
        LIB_split_string_into_2(strUser, '@', strUser, strHost);    // strHost = IP:PORT or IP
    }
    
    return(true);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_GetAddrFromFromToHeader
 * CLASS-NAME     :
 * PARAMETER      : IN OUT IN
 * RET. VALUE     : BOOL
 * DESCRIPTION    : From:/To: header에서 Address부분만 추출
 * REMARKS        : 대/소문자 구분 안함
 **end*******************************************************/
int LIB_GetAddrFromFromToHeader(const char *strHeader, char *strAddr, int MAX_LEN)
{
    char    *strAlloc;
    char    *ptrIMSI;
    char    *ptrAlloc;
    int     i, len, index;
    
    // strHeader를 변경하지 않기 위해 copy
    len = (int)strlen(strHeader) + 1;
    if((strAlloc = (char *)malloc(len)) == NULL) { return(false); }
    strcpy(strAlloc, strHeader);
    
    // header not include 'sip:' or 'tel:' then return false
    if((ptrAlloc = strstr(strAlloc, "sip:")) == NULL)
    {
        if((ptrAlloc = strstr(strAlloc, "tel:")) == NULL) { free(strAlloc); return(false); }
    }
    
    ptrAlloc += 4;               // 4: skip("sip:" or "tel:")
    
#ifdef _IPv6_MODE
    char    szTempIP[64], *pS;
    int     nTempPort = -1;
    memset(szTempIP, 0x00, sizeof(szTempIP));
    if (GetIPv6PortFromString(ptrAlloc, szTempIP, nTempPort) > 0)    // ptrAddr 이 IPv6이면 szTempIP에 IP ('[',']' 제거), nTempPort에 Port 정보 set
    {
        if((pS = strchr(ptrAlloc, '[')) != NULL)
        {
            *pS = '\0';
            sprintf(strAddr, "%s[%s]", ptrAlloc, szTempIP);
        }
        else
            sprintf(strAddr, "[%s]", szTempIP);
        free(strAlloc);
        return(true);
    }
#endif
    
    // delete after '>' or ';' or ':'
    if((ptrIMSI = strchr(ptrAlloc, '>')) != NULL) { *ptrIMSI = '\0'; }
    if((ptrIMSI = strchr(ptrAlloc, ':')) != NULL) { *ptrIMSI = '\0'; }
    if((ptrIMSI = strchr(ptrAlloc, ';')) != NULL) { *ptrIMSI = '\0'; }
    
    len = (int)strlen(ptrAlloc);     // len: "sip:" 다음부터 NULL까지 문자열 길이
    
    if(len >= MAX_LEN)
    {
        Log.printf(LOG_ERR, "LIB_GetAddrFromFromToHeader() MAX_LEN Error len(%d) > %d\n", len, MAX_LEN);
        free(strAlloc);
        return(false);
    }
    
    for(i = 0, index = 0; i < len; i ++, ptrAlloc ++)
    {
        // copy but skip SPACE
        if(*ptrAlloc != ' ') { strAddr[index++] = *ptrAlloc; }
    }
    strAddr[index] = '\0';
    free(strAlloc);
    
    return(true);
}


//#pragma mark -
//#pragma mark SIP 메세지에서 Get/Set 관련 라이브러리


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_GetAddrFromViaHeader
 * CLASS-NAME     : 
 * PARAMETER      : IN OUT IN OUT
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 첫번째 Via에서 ip와 port 추출 
 * REMARKS        : via format = Via: SIP/2.0/UDP ip:port;branch=...
 **end*******************************************************/
int LIB_GetAddrFromViaHeader(const char *strBuf, char *ip, int MAX_LEN, int *port)
{
    char    strVia[1024];
    char    *ptr, *ptrAddr,*ptrPort;
    ssize_t len;
    
    if((LIB_GetStrFromAToB(strBuf, "\r\nVia:", "\r\n", strVia, sizeof(strVia))) == false) { return(false); }
    if((ptr = strchr(strVia, ';')) != NULL) { *ptr = '\0'; }        // ';' 이하 삭제(branch, ... 등)
    
    LIB_DeleteStartSpace(strVia);
    
    if((ptrAddr = strchr(strVia, ' ')) == NULL) { return(false); }  // Via Format Error (Via: SIP/2.0/UDP ip:port;branch=...)
    ptrAddr ++;     // skip "SIP/2.0/UDP "
    
    // ptrAddr = ip:port or ip
    if((ptrPort = strchr(ptrAddr, ':')) != NULL)    // ip:port format
    {
        *ptrPort++ = '\0';
        *port = atoi(ptrPort);
    }
    else                                            // ip only
    {
        *port = 5060;   // default port number
    }
    
    if((len = strlen(ptrAddr)) >= MAX_LEN)
    {
        Log.printf(LOG_ERR, "LIB_GetAddrFromViaHeader() MAX_LEN Error len(%d) > %d\n", len, MAX_LEN);
        return(false); 
    }
    strcpy(ip, ptrAddr);
    
    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_GetAndDelHeader
 * CLASS-NAME     :
 * PARAMETER INOUT: strBuf        - SIP 메세지
 *              IN: strHeaderName - 찾고, 삭제할 SIP Header Name
 *             OUT: strValue      - SIP Header 내용
 *              IN: MAX_VALUE_LEN - strValue size
 * RET. VALUE     : BOOL
 * DESCRIPTION    : strBuf에서 strHeader에 해당하는 내용을 복사하고 Header는 삭제하는 함수
 * REMARKS        : 복사되는 strValue에는 strHeader 및 첫 SPACE는 포함되지 않음
 *                : strHeaderName이 "\r\n..."으로 시작되어야 함
 **end*******************************************************/
bool LIB_GetAndDelHeader(char *strBuf, const char *strHeaderName, char *strValue, int MAX_VALUE_LEN)
{
    char    *ptrHeader, *ptrValue, *ptrCRLF;
    ssize_t     len;
    
    if((ptrHeader = strcasestr((char *)strBuf, strHeaderName)) == NULL) { return(false); }
    if((ptrCRLF   = strcasestr((char *)ptrHeader+2, "\r\n"))   == NULL) { return(false); }
    
    ptrValue = ptrHeader + strlen(strHeaderName);   // Header 이름만큼 포인터 이동 (Header를 포함하지 않기 위해)
    if(*ptrValue == ' ') { ptrValue ++; }           // 메세지의 시작이 ' '(SPACE)이면 다음으로 포인터 이동
    
    if(ptrValue >= ptrCRLF) { return(false); }      // 에러 케이스
    
    len = ptrCRLF - ptrValue;
    if(len >= MAX_VALUE_LEN)
    {
        Log.printf(LOG_ERR, "LIB_GetAndDelHeader() MAX_LEN Error len(%d) > %d\n", len, MAX_VALUE_LEN);
        return(false);
    }
    
    // Copy
    memcpy(strValue, ptrValue, len);                // 해당 Header 내용을 복사
    strValue[len] = '\0';                           // set NULL

    // Delete
    // 20240105 - OrigoAS 5.0.0 LIB_strcpy
    LIB_strcpy(ptrHeader, ptrCRLF);                     // Header 내용 삭제(CRLF 이후 내용으로 overwrite)
    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_DelSipHeaderNameAndValue
 * CLASS-NAME     :
 * PARAMETER INOUT: strBuf        - CSCF로 보낼 SIP 메세지
 *              IN: strHeaderName - SIP Header name
 * RET. VALUE     : BOOL
 * DESCRIPTION    : strBuf에서 strHeader라인을 삭제하는 함수
 * REMARKS        : SIP header중 특정 라인을 삭제하는 함수 (single line)
 *                : strHeaderName이 "\r\n..."으로 시작되어야 함
 **end*******************************************************/
bool LIB_DelSipHeaderNameAndValue(char *strBuf, const char *strHeaderName)
{
    char    *ptrHeader, *ptrCRLF;
    char    strTemp[SAFETY_MAX_BUF_SIZE];

    if((ptrHeader = strstr(strBuf,      strHeaderName)) == NULL) { return(false); }
    if((ptrCRLF   = strstr(ptrHeader+2, "\r\n"))        == NULL) { return(false); }
  
	strcpy(strTemp,ptrCRLF);  
    strcpy(ptrHeader, strTemp);
   // strcpy(ptrHeader, ptrCRLF);
    return(true);
}



/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_GetStrFromAToB
 * CLASS-NAME     : 
 * PARAMETER      : IN IN IN OUT IN
 * RET. VALUE     : BOOL
 * DESCRIPTION    : strBuf에서 strA와 strB 사이의 string을 구하는 함수 
 * REMARKS        : 대/소문자 구분 안함 (strA가 NULL이면 처음부터)
 **end*******************************************************/
int LIB_GetStrFromAToB(const char *strBuf, const char *strA, const char *strB, char *strResult, int MAX_LEN)
{
    char    *ptrA, *ptrB;
    ssize_t     len;
    
    if(strA == NULL)
    { 
        ptrA = (char *)strBuf;  // strA가 NULL이면 처음부터...
    }
    else if((ptrA = strcasestr((char *)strBuf, strA)))
    {
        ptrA += strlen(strA);   // ptrA: strA 문자열 길이만큼 skip..
    }
    else
    {
        return(false);          // strA Not Found
    }
            
    if(strB == NULL)
    {
        if((len = strlen(ptrA)) >= MAX_LEN)
        {
            Log.printf(LOG_ERR, "LIB_GetStrFromAToB() MAX_LEN Error len(%d) > %d\n", len, MAX_LEN);
            return(false); 
        }
        strcpy(strResult, ptrA);
        return(true);
    }
    else if((ptrB = strcasestr(ptrA, strB)))
    {
        if(ptrA >= ptrB) { return(false); }
        
        len = ptrB - ptrA;      // string len
        if(len >= MAX_LEN)
        {
            Log.printf(LOG_ERR, "LIB_GetStrFromAToB() MAX_LEN Error len(%d) > %d\n", len, MAX_LEN);
            return(false); 
        }
        strncpy(strResult, ptrA, len);
        strResult[len] = '\0';
        return(true);
    }
    
    return(false);          // strB Not Found
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_GetStrUntilCRLF
 * CLASS-NAME     : 
 * PARAMETER      : IN IN OUT IN
 * RET. VALUE     : BOOL
 * DESCRIPTION    : Header: data\r\n 에서 data를 추출하는 함수
 * REMARKS        : strBuf에서 strHeader를 찾아서 data를 strOut에 copy
 **end*******************************************************/
bool LIB_GetStrUntilCRLF(const char *strBuf, const char *strHeader, char *strOut, int MAX_LEN)
{
    char    *pStart, *pEnd;
    ssize_t     len;
    
    if((pStart = strcasestr((char *)strBuf, strHeader)))
    {
        pStart += strlen(strHeader);        // Header Size 만큼 Skip
        
        if((pEnd = strstr(pStart, "\r\n")))
        {
            for( ; pStart < pEnd; )
            {
                if(*pStart == ' ') { pStart ++; }
                else
                {
                    len = pEnd-pStart;
                    if(len > (MAX_LEN-1)) { len = MAX_LEN-1;}
                    memcpy(strOut, pStart, len);
                    strOut[len] = '\0';
                    return(true);
                }
            }
        }
    }
    return(false);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_GetIntUntilCRLF
 * CLASS-NAME     :
 * PARAMETER      : IN IN OUT
 * RET. VALUE     : BOOL
 * DESCRIPTION    : Header: data\r\n 에서 data를 추출하는 함수
 * REMARKS        : strBuf에서 strHeader를 찾아서 data를 intOut에 copy
 **end*******************************************************/
bool LIB_GetIntUntilCRLF(const char *strBuf, const char *strHeader, int *intOut)
{
    char *pStart, *pEnd;
    char strOut[16];
    
    if((pStart = strcasestr((char *)strBuf, strHeader)))
    {
        if((pEnd = strstr(pStart, "\r\n")))
        {
            pStart += strlen(strHeader);
            for( ; pStart < pEnd; )
            {
                if(*pStart == ' ') { pStart ++; }
                else
                {
                    if((pEnd-pStart) > 15) { return(false); }   // int too big
                    
                    memcpy(strOut, pStart, pEnd-pStart);
                    strOut[pEnd-pStart] = '\0';
                    *intOut = atoi(strOut);
                    return(true);
                }
            }
        }
    }
    return(false);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_GetHeaderCount
 * CLASS-NAME     : 
 * PARAMETER      : IN OUT
 * RET. VALUE     : int
 * DESCRIPTION    : buf 에서 header가 몇개 있는지를 구하는 함수
 * REMARKS        : 같은 header 겟수 구하는 함수
 **end*******************************************************/
int LIB_GetHeaderCount(const char *strBuf, const char *strHeader)
{
    char    *ptrBuf, *ptr;
    int     count;
    ssize_t header_len;
    
    header_len = strlen(strHeader);       // header 문자열의 길이

    for(count = 0, ptr = ptrBuf = (char *)strBuf; ptr; ptrBuf = ptr)
    {
        if((ptr = strcasestr(ptrBuf, strHeader)))     // header exist in buf
        {
            count ++;
            ptr += header_len;      // header 문자열 길이만큼 skip.. 다음 mactching을 찾기 위해
        }
        else
        {
            return(count);
        }
    }
    return(count);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_GetSipRequestMethod
 * CLASS-NAME     : -
 * PARAMETER    IN: strRcvBuf - Received SIP Message string
 * RET. VALUE     : SIP_METHOD
 * DESCRIPTION    : 수신한 SIP Request 메세지에서 method를 구하는 함수
 * REMARKS        :
 **end*******************************************************/
int LIB_GetSipRequestMethod(char *strRcvBuf)
{
    for(int i = METHOD_INVITE; i < METHOD_END; i ++)
    {
        if(strncasecmp(strRcvBuf, STR_SIP_METHOD[i], strlen(STR_SIP_METHOD[i])) == 0) { return(i); }
    }
    
    return(METHOD_UNDEF);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_GetSipResponseCode
 * CLASS-NAME     : -
 * PARAMETER    IN: strRcvBuf - Received SIP Message string
 * RET. VALUE     : -1(ERR)/int(response code)
 * DESCRIPTION    : 수신한 SIP Request 메세지에서 method를 구하는 함수
 * REMARKS        :
 **end*******************************************************/
int LIB_GetSipResponseCode(char *strRcvBuf)
{
    char	*pStart;
    char	strCode[4];
    int		nCode;
    
    for(pStart = strRcvBuf+8; *pStart == ASCII_SPACE; pStart++) { } // 8 = skip "SIP/2.0 ", Code 앞 부분 SPACE 삭제
    
    strCode[0] = *pStart++;
    strCode[1] = *pStart++;
    strCode[2] = *pStart++;
    strCode[3] = '\0';
    
    nCode = atoi(strCode);
    
    if((100 <= nCode) && (nCode <= 699)) { return(nCode); }
    else								 { return(-1);    }
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_GetSipTypeAndMethod
 * CLASS-NAME     : -
 * PARAMETER    IN: strRcvBuf - Received SIP Message string
 *             OUT: type      - Received SIP Message Type
 *             OUT: method    - Received SIP Message Method or Status-Code
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 수신한 SIP 메세지에서 type과 method를 구하는 함수
 * REMARKS        :
 **end*******************************************************/
bool LIB_GetSipTypeAndMethod(char *strRcvBuf, int *type, int *method)
{
    char	*pCRLF;
    
    if((pCRLF = strstr(strRcvBuf, "\r\n")) == NULL)
    {
        strRcvBuf[128] = '\0';		// set NULL (최대 128자만 출력) - 어짜피 버릴 메시지이기에 복구하지 않는다.
        Log.printf(LOG_ERR, "LIB_GetSipTypeAndMethod() CRLF not found in received SIP message [%s...]\n", strRcvBuf);
        return(false);
    }
    
    if(pCRLF > (strRcvBuf + MAX_LINE_LEN))		// 998자를 넘으면 ERROR RFC 2822
    {
        Log.printf(LOG_ERR, "LIB_GetSipTypeAndMethod() Start Line Length Error nLen=%d\n", (pCRLF >= strRcvBuf) ? pCRLF-strRcvBuf : -1);
        return(false);
    }
    
    *pCRLF = '\0';                  // set NULL (string 처리를 간편하게 하기 위해) - 나중에  CR로 다시 복구
    
    if(strncmp(strRcvBuf, "SIP/2.0 ", 8) == 0)      // SIP Response - 8(include SPACE)
    {
        *type = SIP_TYPE_RESPONSE;
        
        if((*method = LIB_GetSipResponseCode(strRcvBuf)) > 99) { *pCRLF = '\r'; return(true); }
        
        Log.printf(LOG_ERR, "LIB_GetSipTypeAndMethod() Undefined Response Code [%s]\n", strRcvBuf);
        return(false);
    }
    else if(strstr(strRcvBuf, "SIP/2.0") != NULL)   // SIP Request
    {
        *type = SIP_TYPE_REQUEST;
        
        if((*method = LIB_GetSipRequestMethod(strRcvBuf)) != METHOD_UNDEF) { *pCRLF = '\r'; return(true);  }
        
        Log.printf(LOG_ERR, "LIB_GetSipTypeAndMethod() Undefined Request Method [%s]\n", strRcvBuf);
        return(false);
    }
    else                                            // Not SIP/2.0 message
    {
        *type = SIP_TYPE_UNDEF;
        
        Log.printf(LOG_ERR, "LIB_GetSipTypeAndMethod() Not SIP/2.0 message [%s]\n", strRcvBuf);
        return(false);	
    }
    
    return(false);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_GetSipHeaderValue
 * CLASS-NAME     :
 * PARAMETER    IN: strSipMsg - SIP 메세지
 *              IN: strHeader - SIP header
 *             OUT: strValue  - The Value of SIP header
 *              IN: MAX_SIZE  - strValue 배열 최대 크기
 * RET. VALUE     : BOOL
 * DESCRIPTION    : SIP메세지에서 특정헤터의 내용(Value)를 구하는 함수 함수
 * REMARKS        : 대/소문자 구분 안함
 **end*******************************************************/
int LIB_GetSipHeaderValue(const char *strSipMsg, const char *strHeader, char *strValue, int MAX_LEN)
{
    char    *ptrStart, *ptrEnd;
    ssize_t len;
    
    if((ptrStart = strcasestr((char *)strSipMsg, strHeader)))       // Header를 찾는다.
    {
        ptrStart += strlen(strHeader);                      // strHeader만큼 포인터 이동 (Header skip)
        if(*ptrStart == ' ') { ptrStart ++; }               // 첫번째 공백 skip
        if((ptrEnd = strstr(ptrStart, ASCII_CRLF)))         // 헤더의 끝부분을 찾는다.
        {
            len = ptrEnd - ptrStart;
            
            if(len >= MAX_LEN)
            {
                Log.printf(LOG_ERR, "LIB_GetSipHeaderValue(%s) Length Error len(%d) MAX=%d\n", (*strHeader == '\r') ? strHeader+2: strHeader, len, MAX_LEN);        // 출력시 CRLF가 있으면 제거하고 출력
                return(false);
            }

            memcpy(strValue, ptrStart, len);
            strValue[len] = '\0';                   // set NULL (end of string)
            return(true);
        }
    }
    
    return(false);          // strB Not Found
}

//#pragma mark -
//#pragma mark SIP 메세지 Add/Del 관련 라이브러리 (RSAM/RSSW 공통)

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_DelSpyViaHeaderV4
 * CLASS-NAME     :
 * PARAMETER INOUT: pCR    - 수신한 SIP 메세지 구조체 포인터
 *             OUT: strSendBuf - 보낼 SIP 메세지
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 보낼 RESPONSE 메세지의 SPY VIA를 삭제하는 함수
 * REMARKS        : SIP Response는 자신의 Via를 삭제하고 보낸다.
**end*******************************************************/

bool LIB_DelSpyViaHeaderV4(CR_DATA *pCR, char *strSendBuf)
{
    char    strVia[64];
    char    *ptrVia, *ptrCRLF, *ptrComma;
    char    *ptrSip20_1, *ptrSip20_2;
   
	memset(strVia, 0x00, sizeof(strVia));

    snprintf(strVia, sizeof(strVia), "\r\nVia: SIP/2.0/UDP %s", C_ORIG_STACK_IP);      // IP는 ORIG/TERM 동일 함 (PORT만 다름)

    // FIXIT - SPY Via가  Top Via가 아닌 경우......
    if((ptrVia = strstr(strSendBuf, strVia)) == NULL) { return(false); }    // My SPY Via 없음
    if((ptrCRLF = strstr(ptrVia+2, "\r\n"))  == NULL) { return(false); }    // /r/n 없음 ?? Error
    
    *ptrCRLF = '\0';    // set NULL (string 연산을  임시로 NULL로 set)
    
    
    if((ptrComma = strchr(ptrVia+2, ',')) == NULL)  // 한줄에 Via 1개
    {
        *ptrCRLF = '\r';            // restore
		// 20240105 - OrigoAS 5.0.0 LIB_strcpy
        LIB_strcpy(ptrVia, ptrCRLF);    // CRLF부터 over-write하면 1st Via가 삭제됨
    }
    else    // 한줄에 Via가 여러개 (Via: via1, via2, .., via N)
    {
        if((ptrSip20_1 = strstr(ptrVia,   "SIP/2.0")) == NULL) { return(false); } // 1st Via SIP/2.0/UDP
        if((ptrSip20_2 = strstr(ptrComma, "SIP/2.0")) == NULL) { return(false); } // 2nd Via SIP/2.0/UDP
        
        *ptrCRLF = '\r';                    // restore
		// 20240105 - OrigoAS 5.0.0 LIB_strcpy
        LIB_strcpy(ptrSip20_1, ptrSip20_2);    // 2nd Via SIP/2.0 부터 over-write하면 1st Via가 삭제됨
    }

    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_DelSpyViaHeader
 * CLASS-NAME     :
 * PARAMETER INOUT: pCR    - 수신한 SIP 메세지 구조체 포인터

 *             OUT: strSendBuf - 보낼 SIP 메세지
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 보낼 RESPONSE 메세지의 SPY VIA를 삭제하는 함수
* REMARKS        : SIP Response는 자신의 Via를 삭제하고 보낸다.
 **end*******************************************************/

bool LIB_DelSpyViaHeader(CR_DATA *pCR, char *strSendBuf)
{
#ifdef IPV6_MODE
    if(pCR->bIPV6) { return(LIB_DelSpyViaHeaderV6(pCR, strSendBuf)); }
    else
#endif
                   { return(LIB_DelSpyViaHeaderV4(pCR, strSendBuf)); }
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_AppendHeader
 * CLASS-NAME     :
 * PARAMETER   OUT: strSendBuf - 송신 SIP 메세지
 *           INOUT: nSendLen   - 송신 메세지 길이
 *              IN: strRcvBuf  - 수신 SIP 메세지
 *              IN: strHeader  - header name ("\r\n"으로 시작될 것)
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 수신한 메세지에서 특정 헤더를 송신 메세지의 뒷부분에 추가하는 함수
 * REMARKS        : strHeader는 반드시 "\r\n"으로 시작되어야 함
 **end*******************************************************/
bool LIB_AppendHeader(char *strSendBuf, int *nSendLen, const char *strRcvBuf, const char *strHeader)
{
    char    *pStart, *pEnd;
    ssize_t nCopyLen;
    
    if((pStart = strcasestr((char *)strRcvBuf, strHeader)))
    {
        if((pEnd = strstr(pStart+strlen(strHeader), "\r\n")))   // 헤더만큼 포인터 이동해서 검색
        {
            nCopyLen = pEnd - pStart;
            memcpy(&strSendBuf[*nSendLen], pStart, nCopyLen);   // CRLF 부터 CRLF 전 까지 복사
            *nSendLen += nCopyLen;
            return(true);
        }
    }
    return(false);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_DeleteHeader
 * CLASS-NAME     :
 * PARAMETER INOUT: strSendBuf - 송신 SIP 메세지
 *           INOUT: nSendLen   - 송신 메세지 길이
 *              IN: strHeader  - header name ("\r\n"으로 시작될 것)
 * RET. VALUE     : BOOL
 * DESCRIPTION    : strBuf에서 strHeader라인을 삭제하는 함수
 * REMARKS        : SIP header중 특정 라인을 삭제하는 함수 (single line)
 *                : (주의)Text base string인 경우만 가능
 *                : strHeaderName이 "\r\n..."으로 시작되어야 함
 **end*******************************************************/
bool LIB_DeleteHeader(char *strSendBuf, int *nSendLen, const char *strHeader)
{
    char    *ptrHeader, *ptrCRLF;
    
    if((ptrHeader = strstr(strSendBuf, strHeader)) == NULL) { return(false); }
    if((ptrCRLF   = strstr(ptrHeader+2, "\r\n")) == NULL) { return(false); }
    
	// 20240105 - OrigoAS 5.0.0 LIB_strcpy
	LIB_strcpy(ptrHeader, ptrCRLF);

    *nSendLen = (int)strlen(strSendBuf);
    
    return(true);
}

