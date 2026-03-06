//
//  SPY_libv6.hpp
//  nsmSPY
//
//  Created by SMCHO on 2016. 4. 7..
//  Copyright  2016년 SMCHO. All rights reserved.
//

#ifndef __SPY_LIBV6_H__
#define __SPY_LIBV6_H__

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "SPY_def.h"
#include "SPY_xref.h"


#define IPTYPE_V4	0
#define IPTYPE_V6	1


enum HOST_TYPE
{
    HOST_TYPE_UNDEFINED = -1, HOST_TYPE_IPV4, HOST_TYPE_IPV6, HOST_TYPE_HOSTNAME
};

enum IP_TYPE
{
    IP_TYPE_UNDEFINED = -1, IP_TYPE_V4, IP_TYPE_V6
};

int     LIB_isIPv6(const char *strIP);
bool    LIB_ipv6cmp(const char *strIP_A, const char *strIP_B);
bool    LIB_ipv6cmp(const char *strIP_A, struct in6_addr in6_B);

bool    LIB_DelSpyViaHeaderV6(CR_DATA *pCR, char *strSendBuf);

bool    LIB_GetIpAddrInContactHeaderV6(const char *strHeader, char *strIpAddr, int MAX_LEN, int *nPort, IP_TYPE *IpType);
bool    LIB_GetIpAddrInContactHeaderV4(const char *strHeader, char *strIpAddr, int MAX_LEN, int *nPort);
IP_TYPE LIB_get_ip_type(struct in6_addr addr, char *strIP, int MAX_LEN);
bool    LIB_GetAddrFromViaHeaderV6(const char *strSipBuf, char *strIpAddr, int MAX_LEN, int *nPort);

#endif /* __SPY_LIBV6_H__ */
