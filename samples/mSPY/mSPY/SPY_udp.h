//
//  SPY_udp.h
//  smSPY
//
//  Created by SMCHO on 2014. 4. 16..
//  Copyright (c) 2014년 SMCHO. All rights reserved.
//

#ifndef __SPY_UDP_H__
#define __SPY_UDP_H__

#include <iostream>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <unistd.h>

typedef struct in_addr	IN_ADDR;

#define DEFAULT_TTL 0x40  /* Just hard code the ttl in the ip header.*/

#ifdef __APPLE__
#   define	IP_HEADER_LEN	sizeof(struct ip)
#   define	UDP_HEADER_LEN	sizeof(struct udphdr)
#else
#   define	IP_HEADER_LEN	sizeof(struct iphdr)
#   define	UDP_HEADER_LEN	sizeof(struct udphdr)
#endif

#define MTU_SIZE 1472


//SPY_udp.cpp
extern  int     UDP_MakeServer(uint16_t port);
extern  int     UDP_MakeServerByIp(char * ip, uint16_t port);
extern  int     UDP_MakeClientAndSendMessage(char *ip, uint16_t port, char *data, int length);
extern  int     UDP_ServerWaitMessageEx(int fd, char *buf, int maxbuf, struct sockaddr_in *addr);
extern  int     UDP_ServerWaitMessageByIpEx(int fd, char* strIpAddr, char *buf, int maxbuf, struct sockaddr_in *addr);

extern  int     UDP_RawSocketSendByIpNoCRC(char *my_ip, uint16_t sport, char *dest_name, uint16_t dest_port, char *data, int length, uint8_t iTos);

#endif /* defined(__SPY_UDP_H__) */
