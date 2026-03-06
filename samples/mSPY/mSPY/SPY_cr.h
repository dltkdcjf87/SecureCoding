
#ifndef __SPY_CR_H__
#define	__SPY_CR_H__

#include <stdint.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define MAX_BUF_SIZE		8192        // [JIRA AS-206] : 4096 -> 8192
#define SAFETY_MAX_BUF_SIZE	MAX_BUF_SIZE+1+512	// [JIRA AS-206] - 20170719
#define MAX_CR_ALLOC_ERR    5           //  5번 연속 에러 발생 시  SPY kill

enum SIP_TYPE
{
    SIP_TYPE_UNDEF = -1, SIP_TYPE_REQUEST, SIP_TYPE_RESPONSE
};

enum URI_TYPE
{
    URI_TYPE_UNDEFINED = -1, URI_TYPE_SIP, URI_TYPE_TEL
};


enum SIP_METHOD
{
    METHOD_UNDEF     = -1,
    METHOD_INVITE    = 0,   METHOD_ACK,         METHOD_CANCEL,      METHOD_BYE,         METHOD_MESSAGE,
    METHOD_REGISTER  = 5,   METHOD_OPTIONS,     METHOD_INFO,        METHOD_PRACK,       METHOD_UPDATE,
    METHOD_SUBSCRIBE = 10,  METHOD_NOTIFY,      METHOD_REFER,       METHOD_END
};

typedef struct
{
//    bool    bUseStackT;             // CSCF로 전송시 ORIG 또는 TERM stack 중 어느것을 사용하는지 여부
    bool    bExistInHashTable;      // 해당 정보가 HashTable에 있는지 없는지 여부 (있으면 true);
    bool    bInitialMessage;        // SIP Initial Message 여부 (이 경우 SCM에 채널할당하고, HashTable에 Insert해야 함)
    bool    bRetransmission;        // 재전송 여부
    bool    bCSRN;                  // SCM에 채널할당 결과가 CSRN인 경우 setting(TAS only)
    
    /*
     * SIP 메세지를 보내거나 받을 CSCF(SSW) 주소
     */
#ifdef IPV6_MODE
//    bool    bIPv6;                  // 메세지를 송/수신할 Network이 IPv6인지 IPv4인지를 나타냄 (true면 IPv6)
#endif
    char    strCscfAddr[64];        // CSCF Address (IP:PORT or IP) FIXIT - 필요한가, IP PORT만 있으면 되지 않나?
    char	strCscfIp[64];          // CSCF IP   (strCscfAddr 에서 IP 부분)
    int     nCscfPort;              // CSCF PORT (strCscfAddr 에서 PORT 부분, 없으면 5060)
    
    /*
     * SIP 메세지를 보내거나 받을 SPY 주소(SPY의 ORIG/TERM Stack 주소)
     */
    char	strMyStackIpPort[70];   // ORIG/TERM SPY IP:PORT
    char	strMyStackIp[64];       // ORIG/TERM SPY IP
    int     nMyStackPort;           // ORIG/TERM SPY PORT
#ifdef IPV6_MODE
//    char	strMyStackIpV6[64];     // ORIG/TERM SPY IP for IPv6
#endif

    char    strCSeq[32];            // CSeq: header string (SIP_TYPE이 RESPONSE인 경우에 사용)
    char	strCall_ID[256];        // SIP 메시지의 Call ID
    char    strFrom[256];           // From: Header 정보  username@domain 또는 전화번호
    char    strTo[256];             // To:   Header 정보  username@domain 또는 전화번호
#if 0       // 20160908 - strUser_ID 사용안하게 변경
    char    strUser_ID[128];        // User-ID From 정보 (단, 긴급호는 RURI 정보) - FIXIT: 이거 SCM 채할당시 쓰긴하데.. 진짜 필요한거 맞나??
#endif
    char    strServiceKey[64];      // ServiceKey - InitalMessage인 경우에 Route 또는 ruri에 있음
    char    strDP[8];               // FIXIT - DP - 사용여부 ?? 삭제 검토 ??
    int     nTrace;
} SIP_INFO;

typedef struct
{
    time_t      t_alloc;            // CR이 Alloc된 시간
    bool        bFree;              // 이미 free가 되었는지 확인하는 함수
    bool        bPBTChInfo;         // 수신한 메세지에 P-BT-ChInfo Header가 있었는지 여부
} CR_AUDIT;


// CSCF/SAM 에서 수신한 SIP 메세지를 저장하는 구조체
typedef struct
{
    CR_AUDIT    audit;              // CR Audit용 정보
#ifdef IPV6_MODE
    bool        bIPV6;
    struct  sockaddr_in6 peer6;     // IPv6 CSCF IP/PORT (IPv4/v6 dual)
#endif
    struct  sockaddr_in peer;       // 메세지를 보낸 CSCF IP/PORT (IPv4 only)
    int64_t     timer_id;           // SCM 채널 할당시 사용하는 타이머 ID(예외 처리 용)
    
    char        ot_type;            // SIP 메세지를 ORIG STACK에서 받았는지 TERM STACK에서 받았는지를 나타냄
    bool        bIsDeregi;          // add 20170222 - REGISTER message type이 REGI.인지 DEREGI.인지를 나타냄 (true면 deregi) - TAS 에서만 사용
    SIP_TYPE    nSipMsgType;        // REQUEST/RESPONSE
    int         nSipMsg_ID;         //  0 ~ 12 까지는 Request 100 ~ 699: Response
    SIP_INFO    info;               // SIP 메세지를 분석한 내용
    
#ifdef SES_HA_MODE
#ifdef SES_ACTSTD_MODE /* 20241126 kdh add */
	/* not use pair mode */
#else
    bool        bIsMoved;           // SES HA 로 인해 보드가 변경되면 true로 Setting하고 이후 routing을 BoardNo로 하지 않고 PeerBoard로 한다.
    bool        bIsReceiveACK;      // 20161005
    uint8_t     moved_bd;           // SES가 HA 절체가 되어 실제 메시지를 보내야할 SES Board 번호

#endif
#endif
    uint8_t     bd;                 // SAM board number
    int         ch;                 // SAM channel number
    
    int         nRcvLen;                    // 수신한 UDP(SIP) 메세지 길이
    char        strRcvBuf[MAX_BUF_SIZE+1];  // 수신한 SIP 메세지
} CR_DATA;

/* Class Header
 **pdh********************************************************
 * CLASS-NAME     : SPY_CR
 * HISTORY        : 2014/04 - 최초 작성
 * DESCRIPTION    : SPY에서 사용하는 Call Register Class
 *                : 호에 대한 정보를 저장하는 Register
 * REMARKS        :
 **end*******************************************************/
class SPY_CR
{
private:
    pthread_mutex_t lock_mutex;
    int     malloc_err_cnt;         // malloc이 연속적으로 실패한 횟수
    int     sum_malloc_err_cnt;     // malloc 실패 횟 수
    void    clear(void);
    
public:
    int     sum_malloc_cnt;         // malloc 성공 횟 수
    int     sum_free_cnt;           // free 횟 수 (malloc_cnt != free_cnt이면 문제가 있음)
	SPY_CR();
	~SPY_CR();
    
    CR_DATA *alloc(void);           // CR_DATA memory를 allocation 하는 함수
    void    free(CR_DATA *);        // CR_DATA memory를 free 하는 함수

};

#endif // __SPY_CR_H__

