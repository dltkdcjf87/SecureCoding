

#ifndef __SPY_SCM_H__
#define __SPY_SCM_H__


// TO_TIME_WAIT_RESPONSE_FROM_SCM 은 TO_TIME_SCM_ALLOC보다 작아야 함
// T/O과 SCM에서 응답지연 타임이 절묘하게 맞을 경우 pCR 메모리 free로 인행 down 가능성 배제를 위해
#define TO_TIME_SCM_ALLOC               10
#define TO_TIME_WAIT_RESPONSE_FROM_SCM  TO_TIME_SCM_ALLOC-2

#define	CMD_SCM_CH_FREE_RQ      0x1000
#define	CMD_SCM_CH_FREE_RP      0x1001
#define	CMS_SCM_CH_ALLOC_RQ     0x1002
#define	CMS_SCM_CH_ALLOC_RP     0x1003

#define CMS_SCM_OPTION_REPORT   0x1020

#define MAX_SVCKEY_LEN          64
#define MAX_USER_ID_LEN         64

/* response result */
#define SCM_CONFIRM             0x01
#define SCM_REJECT              0x02

/* reason value */
#define ALLOCATION_SUCCESS      0x00
#define RE_ALLOCATION           0x01
#define UNKNOWN_SERVICE         0x02
#define SERVICE_BLOCKED         0x03
#define OVERLOAD_DENY           0x04
#define SERVICE_NUM_EXCEED      0x05

#define DEALLOCATION_SUCCESS    0x06
#define DEALLOC_NOT_FOUND       0x07

#define BLOCKED_TAS_CSFB        0x0A    // add 120703 for TAS Orig/Term BLOCK (0x06 -> 0x0A)
#define BLOCKED_TAS_CSRN        0x0B    // add 120703 for TAS Orig/Term BLOCK (0x07 -> 0x0B)

/*
 * SPY <-> SCM 간 메세지 구조 정의
 */

// 20240909 bible : pack 1byte
#pragma pack(push,1)

typedef struct
{
	uint16_t  id;
	uint8_t   response;
	uint8_t   reason;
} SCM_HEAD;

typedef struct
{
	SCM_HEAD    header;
    char        serviceKey[MAX_SVCKEY_LEN];
    char        userId[MAX_USER_ID_LEN];
    char        dp[4];                      // FIXIT - ?? why string ??
    uint8_t     side;
    time_t      req_time;                   //채널 할당 요구를 보낸 시간(for AUDIT)
    CR_DATA *pCR;                   //SIP 구조체 포인터
    int64_t     timer_id;
} S_SCM_CH_ALLOC_RQ;

typedef struct
{
	SCM_HEAD    header;
    uint16_t    board;
	// 20240909 bible : Modify uint16_t -> uint32_t
    //uint16_t    ch;
    uint32_t    ch;
    uint16_t    callType;
    uint16_t    reserved;
    
    //이하는 CHALLOCRQ에서 보낸 내용 그대로 리턴 됨
    char        serviceKey[MAX_SVCKEY_LEN];
    char        userId[MAX_USER_ID_LEN];
    char        dp[4];
    uint8_t     side;
    time_t      req_time;                   //채널 할당 요구를 보낸 시간(for AUDIT)
    CR_DATA *pCR;                   //SIP 구조체 포인터
    int64_t     timer_id;
} R_SCM_CH_ALLOC_RP;

typedef struct
{
	SCM_HEAD    header;
    uint16_t    board;
	// 20240909 bible : Modify uint16_t -> uint32_t
	//uint16_t    ch;
	uint32_t    ch;
} S_SCM_CH_FREE_RQ;

typedef struct
{
	SCM_HEAD    header;
    uint16_t    board;
	// 20240909 bible : Modify uint16_t -> uint32_t
    //uint16_t    ch;
    uint32_t    ch;
    uint16_t    callType;
    uint16_t    userId[MAX_USER_ID_LEN];
} R_SCM_CH_FREE_RP;

typedef struct
{
    SCM_HEAD	header;
    char        contact[32];
    char        to[64];
    uint16_t    code;
    char        audio_video[64];
} S_SCM_OPTION_REPORT;

// 20240909 bible : pack 1byte
#pragma pack(pop)

#endif  // __SPY_SCM_H__

