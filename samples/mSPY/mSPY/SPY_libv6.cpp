//
//  SPY_libv6.cpp
//  nsmSPY
//
//  Created by SMCHO on 2016. 4. 7..
//  Copyright  2016년 SMCHO. All rights reserved.
//


#include "SPY_libv6.h"

extern int LIB_GetAddrFromViaHeader(const char *strBuf, char *ip, int MAX_LEN, int *port);

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_isIPv6
 * CLASS-NAME     : -
 * PARAMETER    IN: strIP - IP Address string
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 입력 IP가 IPv6 이면 true, 아니면 false
 * REMARKS        : FIXIT
 **end*******************************************************/
int LIB_isIPv6(const char *strIP)
{
    if((strchr(strIP,'.') == NULL) && (strchr(strIP, ':') != NULL)) { return(true); }
    
    return(false);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_ipv6cmp
 * CLASS-NAME     : -
 * PARAMETER    IN: strIP_A - IPv6 Address string A
 * PARAMETER    IN: strIP_B - IPv6 Address string B
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 입력 IP A/B가 같으면 true, 아니면 false
 * REMARKS        :
 **end*******************************************************/
bool LIB_ipv6cmp(const char *strIP_A, const char *strIP_B)
{
    struct in6_addr in6_A;
    struct in6_addr in6_B;

    if(inet_pton(AF_INET6, strIP_A, &in6_A) != 1) { return(false); }    // IP_A string을 IPv6 주소로 변경
    if(inet_pton(AF_INET6, strIP_B, &in6_B) != 1) { return(false); }    // IP_B string을 IPv6 주소로 변경
    
    if(IN6_ARE_ADDR_EQUAL(&in6_A, &in6_B))        { return(true);  }
    
    return(false);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_ipv6cmp
 * CLASS-NAME     : -
 * PARAMETER    IN: strIP_A - IPv6 Address string of A
 * PARAMETER    IN: strIP_B - IPv6 Address of B
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 입력 IP A/B가 같으면 true, 아니면 false
 * REMARKS        :
 **end*******************************************************/
bool LIB_ipv6cmp(const char *strIP_A, struct in6_addr in6_B)
{
    struct in6_addr in6_A;
    
    if(inet_pton(AF_INET6, strIP_A, &in6_A) != 1) { return(false); }    // IP_A string을 IPv6 주소로 변경
    
    if(IN6_ARE_ADDR_EQUAL(&in6_A, &in6_B))        { return(true);  }
    
    return(false);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_DelSpyViaHeaderV6
 * CLASS-NAME     :
 * PARAMETER INOUT: pCR    - 수신한 SIP 메세지 구조체 포인터
 *             OUT: strSendBuf - 보낼 SIP 메세지
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 보낼 RESPONSE 메세지의 SPY VIA를 삭제하는 함수
 * REMARKS        : SIP Response는 자신의 Via를 삭제하고 보낸다.
 **end*******************************************************/
bool LIB_DelSpyViaHeaderV6(CR_DATA *pCR, char *strSendBuf)
{
#ifdef IPV6_MODE
    char    strViaIp[64];
    char    *ptrVia, *ptrCRLF, *ptrComma;
    char    *ptrSip20_1, *ptrSip20_2;
    int     nPort;
    struct in6_addr in6_via;


    // FIXIT 20160322 comment by SMCHO - IPv6 의 경우 IP를 축약할 수 있기 문에 IPv4처럼 문자열 비교를 하면 안된다...
    //                                 - API나 숫자로 변환해서 비교해야 할 듯..... (KT 테스트 과정에서 버그 발견됨)
    if(LIB_GetAddrFromViaHeaderV6(strSendBuf, strViaIp, sizeof(strViaIp), &nPort) == false) { return(false); }    // Via 없음 - 에러처리 (RV 버그 아니면 발생안됨)
    
    inet_pton(AF_INET6, strViaIp, &in6_via);        // Via IP string을 IPv6 주소로 변경
    
#ifdef DEBUG_MODE
    Log.color(COLOR_MAGENTA, LOG_LV1, "[RSAM] %s() Via=%s\n", __FUNCTION__, strViaIp);
#endif
    
    if(IN6_ARE_ADDR_EQUAL(&C_IN6_SPY, &in6_via))    // 1st Via == spy IPv6 address 이면 1st Via 삭제
    {
        if((ptrVia  = strstr(strSendBuf, "\r\nVia:")) == NULL) { return(false); }   // Via 없음
        if((ptrCRLF = strstr(ptrVia+2, "\r\n"))       == NULL) { return(false); }    // /r/n 없음 ?? Error
        
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
    }
#ifdef DEBUG_MODE
    else
    {
        Log.printf(LOG_ERR, "[RSAM] %s() Via=%s is Not SPY VIA(C_IN6_SPY)\n", __FUNCTION__, strViaIp);
    }
#endif
#endif
    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_isIPv6
 * CLASS-NAME     : -
 * PARAMETER    IN: addr  - IPv6 address struct
 * PARAMETER   OUT: strIP - IP Address string
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 입력 address가 IPv6 이면 true, 아니면 false
 * REMARKS        :
 **end*******************************************************/
IP_TYPE LIB_get_ip_type(struct in6_addr addr, char *strIP, int MAX_LEN)
{
    char    strTempIP[46];
    char    *ptr;
    
    strTempIP[0] = '\0';     // initialize
    
    if(inet_ntop(AF_INET6, (void *)&addr, strTempIP, sizeof(strTempIP)) == NULL) { return(IP_TYPE_UNDEFINED); }
    
    if(strchr(strTempIP, '.') == NULL) { snprintf(strIP, MAX_LEN, "%s", strTempIP); return(IP_TYPE_V6);     }

    if((ptr = strrchr(strTempIP, ':')) == NULL) { return(IP_TYPE_UNDEFINED); }   // '.'도 없고 ::ffff:xxx.xxx.xxx.xxx 도 아닌경우

    snprintf(strIP, MAX_LEN, "%s", ptr+1);  // ::ffff:xxx.xxx.xxx.xxx case
    return (IP_TYPE_V4);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_get_host_type
 * CLASS-NAME     : -
 * PARAMETER    IN: strAddr  - Host Address string
 *             OUT: hostType - 입력된 주소의 종류
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 입력 주소가 IPv4인지 IPv6인지 아니면 Hostname 인지 여부를 확인하는 함수
 * REMARKS        :
 **end*******************************************************/
void LIB_get_host_type(const char *strAddr, HOST_TYPE *hostType)
{
    int     ip[4];
    int     n;
    char    *ptr;
    
    ptr = (char *)strAddr;
    
    // '['로 시작하거나 '.'없이 ':'이 나타나면 IPv6로 판단함
    {
        if(*strAddr == '[') { *hostType = HOST_TYPE_IPV6; return; }     // '['로 시작하면 IPv6 Address
        while(*ptr)
        {
            if(*ptr == '.')      { break; }      // IPv4
            else if(*ptr == ':') { *hostType = HOST_TYPE_IPV6; return; }
            else                 { ptr ++; }
        }
    }
    
    //
    // 위에서 안걸리면 IPv4나  Hostname이라고 판단
    n = sscanf(strAddr, "%d.%d.%d.%d", &ip[0],&ip[1],&ip[2],&ip[3]);

    if(n !=4) { *hostType = HOST_TYPE_HOSTNAME; return; }       // n 이 4가 아니면 IPv4아님
    
    for(int i = 0; i < 4; i ++)
    {
        if(ip[i] > 255) { *hostType = HOST_TYPE_UNDEFINED; return; }    // ipv4는 255보다 크면 안됨
    }
    
    *hostType = HOST_TYPE_IPV4; return;
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_GetIpAddrInContactHeaderV6
 * CLASS-NAME     :
 * PARAMETER    IN: strHeader - SIP Header에서 추출한 URI string
 *             OUT: strIpAddr - strHeader에서 추출한 IP ADDRESS
 *              IN: MAX_LEN   - strIpAddr의 최대 길이
 *             OUT: nPort     - strHeader에서 추출한 Port 번호(없으면 default 5060)
 *             OUT: IpType    - Contact의 IP가 IPv4 인지 IPv6인지 여부
 * RET. VALUE     : BOOL
 * DESCRIPTION    : Contact: header에서 IP Address부분만 추출
 * REMARKS        : sip: or sips: 가 없는 경우 에러가 발생됨 (tel:은 에러)
 *                : FIXIT - IP가 아닌 Hostname인 경우 처리 안되어 있음 (DNS Query ???)
 *                : IPv6용  - FIXIT - malloc 할 필요가 있나?? (아래 V4도 동일)
 **end*******************************************************/
bool LIB_GetIpAddrInContactHeaderV6(const char *strHeader, char *strIpAddr, int MAX_LEN, int *nPort, IP_TYPE *IpType)
{
    char    *ptr, *pStart;
    char    *strCopiedHeader;
    size_t  len;
    HOST_TYPE   eHostType;
    hostent *host;
    char ipaddrs[128];
    
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
    *nPort = 5060;
    
    // Contact:의 시작부분에 display-name 이 있는지 확인, 있으면 skip
    {
        // 따옴표는 항상 쌍으로 있어야 하기 때문에 두번째 따옴표 위치로 pointer 이동 (못찾으면 error)
        // display-name 안에 "sip:" or "sips:"이 있으면 IP를 찾는데 문제가 발생함
        while(*ptr == ' ') { ptr ++; }      // 문자열 시작부분의 SPACE 삭제
        if(*ptr == '"') { ptr ++; if((ptr = strchr((char *)ptr, '"')) == NULL) { return(false); } }
    }
    
    // Contact에는 sip: sips: 또는 absoluteURI 만 가능하다.
    {
        pStart = ptr;   // display-name을 제외한 실제 Contact의 start
        if     ((ptr = strcasestr(pStart, "sip:"))  != NULL) { ptr += 4; }
        else if((ptr = strcasestr(pStart, "sips:")) != NULL) { ptr += 5; }
        else                                                 { return(false); }
    }
    
    // string 조작을 편하게 하기 위해 sip: 이하를 memory로 copy 하는데
    // 모든 공백은 제외하고, IPAddress를 자나서 존재하는 '>', ';', ',' 이전까지만 copy
    {
        len = strlen(ptr);
        strCopiedHeader = (char *)malloc(len+1);
        
        
        pStart = strCopiedHeader;
        
        while(*ptr)
        {
            if(*ptr == ' ') { ptr++; continue; }    // skip SPACE
            if((*ptr == '>') || (*ptr == ';') || (*ptr == ',')) { break; }
            
            *pStart++ = *ptr++;    // copy
        }
        
        *pStart = '\0';     // set End of String
    }
    
    // IPAddress 에 userinfo가 있으면('@'로 판단) 시작위치를 userinfo@ 다음으로 이동
    {
        if((ptr = strchr(strCopiedHeader, '@')) != NULL) { pStart = ptr+1; }
        else                                             { pStart = strCopiedHeader; }
    }

    LIB_get_host_type(pStart, &eHostType);
    
    switch(eHostType)
    {
        case HOST_TYPE_IPV4:
            *IpType = IP_TYPE_V4;
            if((ptr = strchr(pStart, ':')) != NULL) { *ptr = '\0'; *nPort = atoi(ptr+1); }   // port번호가 있으면 (':') 이하 삭제
            break;
            
        case HOST_TYPE_IPV6:
            *IpType = IP_TYPE_V6;
            pStart ++;      // skip '['
            
            if((ptr = strchr(pStart, ']')) == NULL) { free(strCopiedHeader); return(false); }  // error - IPv6에서 '['는 있는데 ']' 없음
            
            *ptr = '\0';    // ']' 이하 삭제
            if(*(ptr+1) == ':') {*nPort = atoi(ptr+2); }    // 삭제된 ']'다음이 ':'이면 port 번호가 있음 -> "]:port"
            break;
            
        case HOST_TYPE_HOSTNAME:
            if((host = gethostbyname2(pStart, AF_INET)) != NULL)
            {
                *IpType = IP_TYPE_V4;
                inet_ntop(AF_INET, host->h_addr_list[0], ipaddrs, sizeof(ipaddrs));
            }
            else if((host = gethostbyname2(pStart, AF_INET6)) != NULL)
            {
                *IpType = IP_TYPE_V6;
                inet_ntop(AF_INET6, host->h_addr_list[0], ipaddrs, sizeof(ipaddrs));
            }
            else
            {
                *IpType = IP_TYPE_UNDEFINED;
                printf("%s() can't get ip_addr to host[%s]\n", __FUNCTION__, pStart);
                free(strCopiedHeader);
                return(false);
            }
            
            pStart = ipaddrs;       // inet_ntop로 구한 IP를 pStart에 넣고 아래에서 strIpAddr로 복사
            break;
            
        default:
            printf("%s() host[%s] type error[%d]\n", __FUNCTION__, pStart, eHostType);
            free(strCopiedHeader);
            return(false);
    }

    if(strlen(pStart) >= MAX_LEN) { free(strCopiedHeader); return(false); }    // Buffer Length Error
    
    strcpy(strIpAddr, pStart);
    free(strCopiedHeader);
    
    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_GetIpAddrInContactHeaderV4
 * CLASS-NAME     :
 * PARAMETER    IN: strHeader - SIP Header에서 추출한 URI string
 *             OUT: strIpAddr - strHeader에서 추출한 IP ADDRESS
 *              IN: MAX_LEN   - strIpAddr의 최대 길이
 *             OUT: nPort     - strHeader에서 추출한 Port 번호(없으면 default 5060)
 * RET. VALUE     : BOOL
 * DESCRIPTION    : Contact: header에서 IP Address부분만 추출
 * REMARKS        : sip: or sips: 가 없는 경우 에러가 발생됨 (tel:은 에러)
 **end*******************************************************/
bool LIB_GetIpAddrInContactHeaderV4(const char *strHeader, char *strIpAddr, int MAX_LEN, int *nPort)
{
    char    *ptr, *pStart;
    char    *strCopiedHeader;
    size_t  len;

    ptr = (char *)strHeader;
    *nPort = 5060;
    
    // Contact:의 시작부분에 display-name 이 있는지 확인, 있으면 skip
    {
        // 따옴표는 항상 쌍으로 있어야 하기 때문에 두번째 따옴표 위치로 pointer 이동 (못찾으면 error)
        // display-name 안에 "sip:" or "sips:"이 있으면 IP를 찾는데 문제가 발생함
        while(*ptr == ' ') { ptr ++; }      // 문자열 시작부분의 SPACE 삭제
        if(*ptr == '"') { ptr ++; if((ptr = strchr((char *)ptr, '"')) == NULL) { return(false); } }
    }
    
    // Contact에는 sip: sips: 또는 absoluteURI 만 가능하다.
    {
        pStart = ptr;   // display-name을 제외한 실제 Contact의 start
        if     ((ptr = strcasestr(pStart, "sip:"))  != NULL) { ptr += 4; }
        else if((ptr = strcasestr(pStart, "sips:")) != NULL) { ptr += 5; }
        else                                                 { return(false); }
    }
    
    // string 조작을 편하게 하기 위해 sip: 이하를 memory로 copy 하는데
    // 모든 공백은 제외하고, IPAddress를 자나서 존재하는 '>', ';', ',' 이전까지만 copy
    {
        len = strlen(ptr);
        strCopiedHeader = (char *)malloc(len+1);
        
        
        pStart = strCopiedHeader;
        
        while(*ptr)
        {
            if(*ptr == ' ') { ptr++; continue; }    // skip SPACE
            if((*ptr == '>') || (*ptr == ';') || (*ptr == ',')) { break; }
            
            *pStart++ = *ptr++;    // copy
        }
        
        *pStart = '\0';     // set End of String
    }
    
    // IPAddress 에 userinfo가 있으면('@'로 판단) 시작위치를 userinfo@ 다음으로 이동
    {
        if((ptr = strchr(strCopiedHeader, '@')) != NULL) { pStart = ptr+1; }
        else                                             { pStart = strCopiedHeader; }
    }

    if((ptr = strchr(pStart, ':')) != NULL) { *ptr = '\0'; *nPort = atoi(ptr+1); }   // port번호가 있으면 (':') 이하 삭제

    if(strlen(pStart) >= MAX_LEN) { free(strCopiedHeader); return(false); }    // Buffer Length Error
    
    strcpy(strIpAddr, pStart);
    free(strCopiedHeader);
    
    return(true);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_GetAddrFromViaHeaderV6
 * CLASS-NAME     :
 * PARAMETER    IN: strSipBuf - Received SIP Message
 *             OUT: strIpAddr - strSipBuf의 1st Via 에서 추출한 IP ADDRESS
 *              IN: MAX_LEN   - strIpAddr의 최대 길이
 *             OUT: nPort     - strSipBuf의 1st Via 에서 추출한 Port 번호(없으면 default 5060)
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 첫번째 Via에서 ip와 port 추출
 * REMARKS        : via format = Via: SIP/2.0/UDP ip:port;branch=...
 **end*******************************************************/
bool LIB_GetAddrFromViaHeaderV6(const char *strSipBuf, char *strIpAddr, int MAX_LEN, int *nPort)
{
    char    strVia[1024];       // 170525: 256 -> 1024
    char    *ptr;
    char    *pStart, *pEnd;
    ssize_t len;
    
    /* RFC 3261
     Via               =  ( "Via" / "v" ) HCOLON via-parm *(COMMA via-parm)
     via-parm          =  sent-protocol LWS sent-by *( SEMI via-params )
     sent-by           =  host [ COLON port ]
     host              =  hostname / IPv4address / IPv6reference
     IPv6reference     =  "[" IPv6address "]"
    */
    if((LIB_GetStrFromAToB(strSipBuf, "\r\nVia:", "\r\n", strVia, sizeof(strVia))) == false) { return(false); }
    if((ptr = strchr(strVia, ';')) != NULL) { *ptr = '\0'; }        // ';' 이하 삭제(branch, ... 등)
    
//    LIB_DeleteStartSpace(strVia);
    
    // IPv6reference 는 반드시 '['와 ']'이 있어야 한다.
    if((pStart = strchr(strVia, '[')) == NULL)
    {
        Log.printf(LOG_ERR, "%s() '[' is missing in 1st Via Header\n", __FUNCTION__);
        return(false);
    }
    
    if((pEnd = strchr(strVia, ']')) == NULL)
    {
        Log.printf(LOG_ERR, "%s() ']' is missing in 1st Via Header\n", __FUNCTION__);
        return(false);
    }
    
    *pEnd = '\0';   // set NULL
    
    // port 번호([ COLON port ])가 있는지 검사
    if(*(pEnd+1) == ':') { *nPort = atoi(pEnd+2); }
    else                 { *nPort = 5060;         }

    
    if((len = strlen(pStart+1)) >= MAX_LEN)
    {
        Log.printf(LOG_ERR, "%s() MAX_LEN Error len(%d) > %d\n", __FUNCTION__, len, MAX_LEN);
        return(false);
    }
    strcpy(strIpAddr, pStart+1);
    
    return(true);
}

