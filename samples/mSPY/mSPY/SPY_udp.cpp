/* File Header
 *fsh**************************************************************
 ******************************************************************
 **
 **  FILE : SPY_udp.cpp
 **
 ******************************************************************
 ******************************************************************
 BLOCK          : SPY
 SUBSYSTEM      : CMS
 SOR-NAME       :
 VERSION        : V4.X
 DATE           : 2014/04/
 AUTHOR         : SEUNG-MO, CHO
 HISTORY        :
 PROCESS(TASK)  :
 PROCEDURES     :
 DESCRIPTION    : SPY에서 사용하는 UDP 함수들
 REMARKS        :
 *end*************************************************************/
#ifndef USE_GODRM

#include "SPY_udp.h"


typedef struct sockaddr		SA;


#pragma mark -
#pragma mark UDP Client 관련 함수

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : UDP_MakeClientAndSendMessage
 * CLASS-NAME     :
 * PARAMETER    IN: ip   - UDP Server IP address
 *              IN: port - UDP Server Port 번호
 *              IN: data - 전송하고자하는 데이터 포인터
 *              IN: length - 전송하고자하는 데이터 length
 * RET. VALUE     : -1(ERR)/N(Send Length)
 * DESCRIPTION    : UDP Client를 생성하고 데이터를 전송하는 함수
 * REMARKS        :
 **end*******************************************************/
int UDP_MakeClientAndSendMessage(char *ip, uint16_t port, char *data, int length)
{
	int		fd, n;
	struct sockaddr_in 	addr;
    
	if((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)  { return(-1); }
    
	bzero((char *) &addr, sizeof(addr));
    
	addr.sin_family      = AF_INET;
	addr.sin_port        = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip);
    
	if((n = sendto(fd, data, length, 0, (SA *)&addr, sizeof(addr))) < 0)
	{
		close(fd);
		return(-1);
	}
    
	close(fd);
	return(n);
}


#pragma mark -
#pragma mark UDP Serve 관련 함수

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : UDP_MakeServer
 * CLASS-NAME     :
 * PARAMETER    IN: port - UDP Server Port 번호
 * RET. VALUE     : -1(ERR)/socket_fd
 * DESCRIPTION    : UDP Server를 생성하는 함수
 * REMARKS        : 모든 IP에 대해서 해당 port로 bind
 **end*******************************************************/
int UDP_MakeServer(uint16_t port)
{
	int 	fd;
	struct 	sockaddr_in addr;
    
	if((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)  { return(-1); }
    
	bzero(&addr, sizeof(addr));
    
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port        = htons(port);
    
	if(bind(fd, (SA *) &addr, sizeof(addr)) < 0)  { return(-1); }
    
	return(fd);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : UDP_MakeServerByIp
 * CLASS-NAME     :
 * PARAMETER    IN: ip   - UDP Server IP address
 *              IN: port - UDP Server Port 번호
 * RET. VALUE     : -1(ERR)/socket_fd
 * DESCRIPTION    : UDP Server를 생성하는 함수
 * REMARKS        : 특정 IP, Port로 bind
 **end*******************************************************/
int UDP_MakeServerByIp(char * ip, uint16_t port)
{
	int 	fd;
	struct 	sockaddr_in addr;
    
	if((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)  { return(-1); }
    
	bzero(&addr, sizeof(addr));
    
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ip);
	addr.sin_port        = htons(port);
    
	if(bind(fd, (SA *) &addr, sizeof(addr)) < 0)  { return(-1); }
    
	return(fd);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : UDP_ServerWaitMessageEx
 * CLASS-NAME     :
 * PARAMETER    IN: fd       - Socket FD
 *             OUT: buf      - 수신된 UDP메세지가 저장될 버퍼
 *              IN: maxbuf   - 버퍼의 최대 길이
 *             OUT: cli_addr - UDP 메세지를 보내온 Client 주소를 저장할 구조체
 * RET. VALUE     : N(수신 데이터 길에)
 * DESCRIPTION    : UDP Client로 부터 메세지가 수신되기를 기다리는 함수
 * REMARKS        : T/O 없음, ANY_ADDR로 wait
 **end*******************************************************/
int UDP_ServerWaitMessageEx(int fd, char *buf, int maxbuf, struct sockaddr_in *cli_addr)
{
	int		cli_len;

	bzero(cli_addr, sizeof(struct sockaddr_in));
    
	cli_addr->sin_family      = AF_INET;
	cli_addr->sin_addr.s_addr = INADDR_ANY;
    
	cli_len = sizeof(struct sockaddr_in);
    
	return(recvfrom(fd, buf, maxbuf, 0, (SA *)cli_addr, (socklen_t *)&cli_len));
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : UDP_ServerWaitMessageEx
 * CLASS-NAME     :
 * PARAMETER    IN: fd       - Socket FD
 *              IN: ip       - UDP Server IP(내 주소로 특정 IP로만 데이터 수신)
 *             OUT: buf      - 수신된 UDP메세지가 저장될 버퍼
 *              IN: maxbuf   - 버퍼의 최대 길이
 *             OUT: cli_addr - UDP 메세지를 보내온 Client 주소를 저장할 구조체
 * RET. VALUE     : N(수신 데이터 길에)
 * DESCRIPTION    : UDP Client로 부터 메세지가 수신되기를 기다리는 함수
 * REMARKS        : T/O 없음, 특정 IP로 wait
 **end*******************************************************/
int UDP_ServerWaitMessageByIpEx(int fd, char *ip , char *buf, int maxbuf, struct sockaddr_in *cli_addr)
{
	int		cli_len;

	bzero(cli_addr, sizeof(struct sockaddr_in));
    
	cli_addr->sin_family      = AF_INET;
	cli_addr->sin_addr.s_addr = inet_addr(ip);
    
	cli_len = sizeof(struct sockaddr_in);
    
	return(recvfrom(fd, buf, maxbuf, 0, (SA *)cli_addr, (socklen_t *)&cli_len));
}


#pragma mark -
#pragma mark IP, UDP 프로토콜 check sum을 만드는 함수

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : ip_check_sum
 * CLASS-NAME     :
 * PARAMETER    IN: addr - IP Header 구조체 주소
 *              IN: len  - IP Header의 길이
 * RET. VALUE     : N(IP check sum)
 * DESCRIPTION    : IP Header의 check sum을 만드는 함수
 * REMARKS        :
 **end*******************************************************/
uint16_t ip_check_sum(uint16_t *addr, int len)
{
	int         sum = 0;
	uint16_t 	answer = 0;
	uint16_t 	*w = addr;
	int         nleft = len;
    
	while (nleft > 1)  { sum += *w++; nleft -= 2; }
    
	/* mop up an odd uint8_t, if necessary */
	if (nleft == 1)
    {
		*(u_char *)(&answer) = *(u_char *)w ;
		sum += answer;
	}
    
	/* add back carry outs from top 16 bits to low 16 bits */
	sum  = (sum >> 16) + (sum & 0xffff);    /* add hi 16 to low 16 */
	sum += (sum >> 16);                     /* add carry */
	answer = ~sum;                          /* truncate to 16 bits */
    
	return(answer);
}


struct psuedo_hdr
{
	IN_ADDR     source_address;
	IN_ADDR     dest_address;
	uint8_t     place_holder;
	uint8_t 	protocol;
	uint16_t 	length;
} psuedo_hdr;

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : udp_check_sum
 * CLASS-NAME     :
 * PARAMETER    IN: addr - UDP Header 구조체 주소
 *              IN: len  - UDP Header의 길이
 *              IN: saddr - UDP source address
 *              IN: daddr - UDP destination address
 * RET. VALUE     : N(UDP check sum)
 * DESCRIPTION    : IP Header의 check sum을 만드는 함수
 * REMARKS        : 현재 사용 안함 (UDP는 check sum 생략이 가능)
 **end*******************************************************/
uint16_t udp_check_sum(char *packet, int length, IN_ADDR saddr, IN_ADDR daddr)
{
	char        *psuedo_packet;
	uint16_t	checksum;
    
	psuedo_hdr.protocol     = IPPROTO_UDP;
	psuedo_hdr.length       = htons(length);
	psuedo_hdr.place_holder = 0;
    
	psuedo_hdr.source_address = saddr;
	psuedo_hdr.dest_address   = daddr;
    
	if((psuedo_packet = (char *)malloc(sizeof(psuedo_hdr) + length)) == NULL)  { return(0); }
    
	memcpy(psuedo_packet, &psuedo_hdr, sizeof(psuedo_hdr));
	memcpy((psuedo_packet + sizeof(psuedo_hdr)), packet, length);
    
	checksum = (uint16_t)ip_check_sum((uint16_t *)psuedo_packet, (length + sizeof(psuedo_hdr)));
	free(psuedo_packet);
    
	return(checksum);
}


#pragma mark -
#pragma mark RAW Socket으로 UDP를 만드는 함수

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : ip_header_gen
 * CLASS-NAME     :
 * PARAMETER   OUT: sPDU   - IP Header 시작 pointer
 *              IN: saddr  - source address
 *              IN: daddr  - dest.  address
 *              IN: length - 전송할 Data Length(IP/UDP 헤더 포함)
 *              IN: iTos   - type of service
 * RET. VALUE     : N(UDP check sum)
 * DESCRIPTION    : IP Protocel Header를 만드는 함수
 * REMARKS        :
 **end*******************************************************/
void ip_header_gen(char *sPDU, IN_ADDR saddr, IN_ADDR daddr, uint16_t length, uint8_t iTos )
{
    
#ifdef __APPLE__
    struct ip *iphdr = (struct ip *)sPDU;
    
	bzero((char *)iphdr, IP_HEADER_LEN);
    
	iphdr->ip_v   = IPVERSION;
	iphdr->ip_hl  = 5;
	iphdr->ip_tos = iTos;
	iphdr->ip_len = htons(length);
	iphdr->ip_off = htons(IP_DF);
	iphdr->ip_ttl = DEFAULT_TTL;
	iphdr->ip_p   = IPPROTO_UDP;
    
	iphdr->ip_src.s_addr = saddr.s_addr;
	iphdr->ip_dst.s_addr = daddr.s_addr;
    
	iphdr->ip_sum = ip_check_sum((uint16_t *)iphdr, IP_HEADER_LEN);
#else
    struct iphdr *iphdr = (struct iphdr *)sPDU;;

	bzero((char *)iphdr, IP_HEADER_LEN);
    
    iphdr->version  = IPVERSION;
	iphdr->ihl      = 5;
	iphdr->tos      = iTos;
	iphdr->tot_len  = htons(length);
	iphdr->frag_off = htons(IP_DF);
	iphdr->ttl      = DEFAULT_TTL;
	iphdr->protocol = IPPROTO_UDP;
    
	iphdr->saddr    = saddr.s_addr;
	iphdr->daddr    = daddr.s_addr;
    
	iphdr->check    = ip_check_sum((uint16_t *)iphdr, IP_HEADER_LEN);
#endif
	return;
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : ip_header_gen2
 * CLASS-NAME     :
 * PARAMETER   OUT: sPDU   - IP Header 시작 pointer
 *              IN: saddr  - source address
 *              IN: daddr  - dest.  address
 *              IN: length - 전송할 Data Length(IP/UDP 헤더 포함)
 *              IN: iTos   - type of service
 *              IN: flag_id
 *              IN: flag_offset - 이번에 보낼 데이터 시작 part offset
 *              IN: flag_on
 * RET. VALUE     : N(UDP check sum)
 * DESCRIPTION    : IP Protocel Header를 만드는 함수
 * REMARKS        : fragmentation된 UDP Header 생성
 **end*******************************************************/
void ip_header_gen2(char *sPDU, IN_ADDR saddr, IN_ADDR daddr, uint16_t length, uint8_t iTos, uint16_t flag_id, uint16_t flag_offset, char flag_on )
{
    
#ifdef __APPLE__
    struct ip *iphdr = (struct ip *)sPDU;

    bzero((char *)iphdr, IP_HEADER_LEN);
    
    iphdr->ip_v   = IPVERSION;
	iphdr->ip_hl  = 5;
    iphdr->ip_tos = iTos;
	iphdr->ip_len = htons(length);
	iphdr->ip_id  = htons(flag_id);
	if (flag_on)
		iphdr->ip_off = htons(IP_MF | (IP_OFFMASK & (flag_offset >> 3)));
	else
		iphdr->ip_off = htons(IP_OFFMASK & (flag_offset >> 3));
	iphdr->ip_ttl = DEFAULT_TTL;
	iphdr->ip_p   = IPPROTO_UDP;
    
	iphdr->ip_src.s_addr = saddr.s_addr;
	iphdr->ip_dst.s_addr = daddr.s_addr;
    
	iphdr->ip_sum = ip_check_sum((uint16_t *)iphdr, IP_HEADER_LEN);
#else   // LINUX
	struct iphdr *iphdr = (struct iphdr *)sPDU;
    
    bzero((char *)iphdr, IP_HEADER_LEN);
    
    iphdr->version = IPVERSION;
	iphdr->ihl     = 5;
    iphdr->tos     = iTos;
	iphdr->tot_len = htons(length);
	iphdr->id = htons(flag_id);
	if (flag_on)
		iphdr->frag_off = htons(IP_MF | (IP_OFFMASK & (flag_offset >> 3)));
	else
		iphdr->frag_off = htons(IP_OFFMASK & (flag_offset >> 3));
	iphdr->ttl      = DEFAULT_TTL;
	iphdr->protocol = IPPROTO_UDP;
    
	iphdr->saddr    = saddr.s_addr;
	iphdr->daddr    = daddr.s_addr;
    
	iphdr->check = ip_check_sum((uint16_t *)iphdr, IP_HEADER_LEN);
#endif
    
	return;
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : udp_header_gen
 * CLASS-NAME     :
 * PARAMETER   OUT: sPDU   - UDP Header 시작 pointer
 *              IN: sport  - source port 번호
 *              IN: dport  - dest.  port 번호
 *              IN: length - 전송할 Data Length(UDP 헤더 포함, IP 헤더 제외)
 * RET. VALUE     : N(UDP check sum)
 * DESCRIPTION    : UDP Protocel Header를 만드는 함수
 * REMARKS        :
 **end*******************************************************/
void udp_header_gen(char *sPDU, uint16_t sport, uint16_t dport, uint16_t length)
{
	struct udphdr *udp;
    
	udp = (struct udphdr *)sPDU;
    
#ifdef __APPLE__
    udp->uh_sport = htons(sport);
	udp->uh_dport = htons(dport);
	udp->uh_ulen  = htons(length);
	udp->uh_sum   = 0;
#else
	udp->source = htons(sport);
	udp->dest = htons(dport);
	udp->len  = htons(length);
	udp->check   = 0;
#endif
	return;
}



/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : UDP_RawSocketSendByIpNoCRC
 * CLASS-NAME     :
 * PARAMETER    IN: my_ip     - UDP source IP address
 *              IN: sport     - UDP source port 번호
 *              IN: dest_ip   - UDP destination IP address
 *              IN: dest_port - UDP destination port 번호
 *              IN: data      - 전송할 data pinter
 *              IN: length    - 전송할 데이터 길이
 *              IN: iTos      - type of service
 * RET. VALUE     : -(ERR)/0(OK)
 * DESCRIPTION    : Raw Socket으로 UDP 프로토콜을 만들어 데이터를 보내는 함수
 * REMARKS        : source/dest ip를 setting 해서 보내기 위해서 사용
 **end*******************************************************/
int UDP_RawSocketSendByIpNoCRC(char *my_ip, uint16_t sport, char *dest_ip, uint16_t dest_port, char *data, int length, uint8_t iTos)
{
	char 	sPDU[IP_HEADER_LEN + UDP_HEADER_LEN + 8192];
	IN_ADDR saddr, daddr;
	struct 	sockaddr_in sock_addr;
	struct 	udphdr *udphdr;
	int 	sockfd;
    int     on = 1;
    int     nIndex = 0, nPart = length;
    int     nSendLen;
	struct  sockaddr_in sockname;
	socklen_t	socklen = sizeof(sockname);
    
	saddr.s_addr = inet_addr(my_ip);
	daddr.s_addr = inet_addr(dest_ip);
    
	if((sockfd = socket(PF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)  { return(-2); }
    
    
	getsockname(sockfd, (struct sockaddr*)&sockname, &socklen);      // 로컬 소켓 정보를 가져온다, FXIT: - 사용여부 확인

    // IP_HDRINCL(IP Header Include)를 Set해야 RAW SOCKET을 사용할 수 있다.
	if(setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, (char *)&on, sizeof(on)) < 0) { close(sockfd); return(-3); }
    
	if(nPart <= MTU_SIZE)    // fregmentation이 없는 경우
    {
        nSendLen = IP_HEADER_LEN + UDP_HEADER_LEN + nPart;      // UDP로 전송할 데이터 길이
        ip_header_gen(sPDU, saddr, daddr, nSendLen, iTos);      // IP Protocol Header를 생성

        udphdr = (struct udphdr *)(sPDU + IP_HEADER_LEN);
        udp_header_gen((char *)udphdr, sport, dest_port, (UDP_HEADER_LEN + nPart)); // UDP Protocol Header 생성
        
        memcpy(&sPDU[IP_HEADER_LEN+UDP_HEADER_LEN], data, nPart);   // 전송할 데이터를 sPDU 데이터 부분에 복사
        
#ifdef __APPLE__
//		udphdr->uh_sum = udp_check_sum((char *)udphdr, (nPart + UDP_HEADER_LEN), saddr, daddr);
        udphdr->uh_sum = 0;  // NO CRC
#else   // LINUX
//        udphdr->check = udp_check_sum((char *)udphdr, (nPart + UDP_HEADER_LEN), saddr, daddr);
        udphdr->check = 0;
#endif
        
        // SEND DATA to DESTINATION
        bzero(&sock_addr, sizeof(sock_addr));
        
        sock_addr.sin_family = AF_INET;
        sock_addr.sin_port   = htons(dest_port);
        sock_addr.sin_addr   = daddr;
        if(sendto(sockfd, &sPDU, nSendLen, 0, (SA *)&sock_addr, sizeof(sock_addr)) < 0) { close(sockfd); return(-6); }
    }
    else     // fregmentation 이 있는 경우
	{
		nPart   -= MTU_SIZE;
		nSendLen = IP_HEADER_LEN + UDP_HEADER_LEN + MTU_SIZE;
        
//		ip_header_gen2(sPDU, saddr, daddr, nSendLen, iTos, (uint16_t)&sockfd, 0, TRUE); FIXIT: &sockd or sockd
        ip_header_gen2(sPDU, saddr, daddr, nSendLen, iTos, (uint16_t)sockfd, 0, true);
        
		udphdr = (struct udphdr *)(sPDU + IP_HEADER_LEN);

		udp_header_gen((char *)udphdr, sport, dest_port, (UDP_HEADER_LEN + length));
        
        memcpy(&sPDU[IP_HEADER_LEN+UDP_HEADER_LEN], data, length);
        
		memset(&sock_addr,'\0',sizeof(sock_addr));
		sock_addr.sin_family = AF_INET;
		sock_addr.sin_port = htons(dest_port);
		sock_addr.sin_addr = daddr;
        
		if(sendto(sockfd, &sPDU, nSendLen, 0,(SA *)&sock_addr, sizeof(sock_addr)) < 0)
        {
			close(sockfd);
			return(-3);
		}
        
		nIndex++;
        
		while(nPart > MTU_SIZE)
		{
			nPart   -= MTU_SIZE;
			nSendLen = IP_HEADER_LEN+MTU_SIZE;
//			ip_header_gen2(sPDU, saddr, daddr, nSendLen, iTos, (uint16_t)&sockfd, UDP_HEADER_LEN+MTU_SIZE*nIndex, TRUE); FIXIT: &sockd or sockd
            ip_header_gen2(sPDU, saddr, daddr, nSendLen, iTos, (uint16_t)sockfd, UDP_HEADER_LEN+MTU_SIZE*nIndex, true);
            
			memcpy(&sPDU[IP_HEADER_LEN], data+(MTU_SIZE*nIndex), MTU_SIZE);
            
			memset(&sock_addr,'\0',sizeof(sock_addr));
			sock_addr.sin_family = AF_INET;
			sock_addr.sin_port = htons(dest_port);
			sock_addr.sin_addr = daddr;
            
			if(sendto(sockfd, &sPDU, nSendLen, 0x0, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) < 0)
            {
				close(sockfd);
				return(-4);
			}
			nIndex++;
		}
        
		nSendLen = IP_HEADER_LEN+nPart;
//		ip_header_gen2(sPDU, saddr, daddr, nSendLen, iTos, (uint16_t)&sockfd, UDP_HEADER_LEN+MTU_SIZE*nIndex, FALSE); FIXIT: &sockd or sockd
        ip_header_gen2(sPDU, saddr, daddr, nSendLen, iTos, (uint16_t)sockfd, UDP_HEADER_LEN+MTU_SIZE*nIndex, true);
        
		memcpy(&sPDU[IP_HEADER_LEN], data+(MTU_SIZE*nIndex), nPart);
        
		memset(&sock_addr,'\0',sizeof(sock_addr));
		sock_addr.sin_family = AF_INET;
		sock_addr.sin_port = htons(dest_port);
		sock_addr.sin_addr = daddr;
		if(sendto(sockfd,&sPDU,nSendLen,0x0,(struct sockaddr *)&sock_addr, sizeof(sock_addr)) < 0)
        {
			close(sockfd);
			return(-5);
		}
	}
    
	close(sockfd);
    
	return(0);
}

#endif  // USE_GODRM
