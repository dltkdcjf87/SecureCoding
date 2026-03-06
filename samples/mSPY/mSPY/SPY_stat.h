

#ifndef __SPY_STA_H__
#define __SPY_STA_H__

#include "SPY_cr.h"

#define CMD_STA_SEND_ONLY       0x00	// 통계 자료 전송 만, 초기화 안함
#define CMD_STA_SEND_AND_RESET  0x01	// 통계 자료 전송 후 초기화
#define CMD_STA_RESET_ONLY      0x02	// 통계 자료 초기화 만
#define CMD_STA_RESULT          0x03	// 통계 결과를 SGM에게 전송

typedef struct
{
	uint16_t    len;
	uint8_t     command;
	uint8_t     reserv;
} STA_HDR;



typedef struct
{
    int     scm_fail_count;
    int     scm_resp_count;
} STA_SPY;

typedef struct
{
    int     n_request[METHOD_END];
    int     n_request_err;              // ERROR 통계 추가 검토
    int     n_response_code[700];
    int     n_in_sum;
    int     n_out_sum[2];                   // 0: ORIG, 1: TERM
    int     n_hash_fail;
} STA_SAM;

typedef struct
{
    int     n_request[METHOD_END];
    int     n_request_err;              // FIXIT: 추가 검토
    int     n_response_code[700];
    int     n_response_code_out[700];
    int     n_response_code_out_sum[2];     // 0: ORIG, 1: TERM
    int     n_in_sum[2];                    // 0: ORIG, 1: TERM
    int     n_out_sum_O;
    int     n_out_sum_T;
} STA_SSW;
typedef struct
{
    STA_SPY     spy;
    STA_SAM     sam;
    STA_SSW     ssw;
} STA_INFO;


#define MAX_STA_RESPONSE_CODE       46
#define MAX_STA_REQUEST_CDDE        13

static int SPY_STS_RESP_CODE[MAX_STA_RESPONSE_CODE] =
{
    100, 180, 183, 200, 300, 301, 302, 305, 400, 401,
    402, 403, 404, 405, 406, 407, 408, 410, 413, 414,
    415, 416, 420, 421, 423, 480, 481, 482, 483, 484,
    485, 486, 487, 491, 493, 500, 501, 502, 503, 504,
    505, 513, 600, 603, 604, 606
    
};

typedef struct
{
	STA_HDR	header;
    
    int     in_request[MAX_STA_REQUEST_CDDE];
    int     in_response_code[MAX_STA_RESPONSE_CODE];

	int     in_msg_o;
	int     in_msg_t;
	int     in_total_msg;
    
    int     out_request[MAX_STA_REQUEST_CDDE];
    int     out_response_code[MAX_STA_RESPONSE_CODE];

	int     out_msg_o;
	int     out_msg_t;
	int     out_total_msg;
	int     scm_error;
} SPY_STS;


typedef struct
{
    STA_HDR header;
    int     in_regi;            // Regi 총 count
    int     in_regi_succ;       // Regi 처리 결과 성공 count
    int     in_regi_fail;       // Regi 처리 결과 실패 count
    int     in_deregi;          // De-regi 총 count
    int     in_deregi_succ;     // De-regi 처리 결과 성공 count
    int     in_deregi_fail;     // De-regi 처리 결과 실패 count
} SPY_REGI_STS;

#endif  // __SPY_STA_H__
