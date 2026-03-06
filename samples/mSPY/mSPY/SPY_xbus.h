//
//  SPY_xbus.h
//  smSPY
//
//  Created by SMCHO on 2014. 3. 27..
//  Copyright (c) 2014년 SMCHO. All rights reserved.
//

#ifndef __SPY_XBUS_H__
#define __SPY_XBUS_H__

// M-BUS Command
#define MSG_ID_SCM              0x0101
#define MSG_ID_DBM              0xC001

// S-BUS Command
#define SCMD_HASH_INSERT_RQ     0x0401          // Hash Table Insert 요청 (Active  -> Standby)
#define SCMD_HASH_SYNC_RQ       0x0404          // Hash Table Sync   요청 (Standby -> Active)
#define SCMD_HASH_HA_UPDATE_RQ  0x0406          // Hash Table Update 요청 (Standby -> Active)

/*
 * SPY <-> DGM <-> WEB 간 메세지 구조 정의
 */
// WEB-CMD From WEB(via DGM)
#define WCMD_SSW_REG            0x00
#define WCMD_SSWLIST_REG        0x01
#define WCMD_AS_BLOCK           0x02
#define WCMD_TRACE_LIST_REQ     0x03
#define WCMD_TRACE_ADD_REQ      0x04
#define WCMD_TRACE_DEL_REQ      0x05
#define WCMD_BLACKLIST_REG      0x06    // add by SMCHO 20140918
#define WCMD_BLACKLIST_ACT      0x07    // add by SMCHO 20140918
#define WCMD_SSWFILE_REG        0x08    // add by SMCHO 20140924

#ifdef IPV6_MODE
#   define WCMD_BLACKLIST_REG_V6     0x09
#   define WCMD_SSW_REG_V6			0x10	// IPv6 용 CSCF 등록 수정 삭제
#   define WCMD_SSWLIST_REG_V6		0x11	// IPv6 용 CSCF list 등록
#   define WCMD_SSWFILE_REG_V6		0x12	// IPv6 용 CSCF 파일 등록
#endif

typedef struct
{
	short       len;
	uint16_t    msgId;
	int         sockFd;
	int         threadId;
	uint16_t    relay_ToId;
	uint16_t    relay_MsgId;
} DGM_HDR;

typedef struct
{
	DGM_HDR     hdr;
	char        asId[2];
	char        block;
} R_AS_BLOCK_RQ;

typedef struct
{
	DGM_HDR     hdr;
	char        result;     // '1': OK, '0': Fail => not number, character
    char        reason;     // result가 '0' 인 경우에 만 setting
} S_RESULT_TO_WEB;

typedef struct
{
	DGM_HDR     hdr;
	char        data[2048];
} R_CMD_FROM_WEB;

typedef struct
{
	DGM_HDR     hdr;
	char        action[6];
	char        sswid[3];
	char        ip[32];
	char        sspcode[4];
	char        block;
	char        monitor;
	char        checkTime[3];
	char        tryCount;
} R_EXT_SVR_REG_RQ;

#ifdef IPV6_MODE
typedef struct
{
    DGM_HDR     hdr;
    char        action[6];
    char        sswid[3];
    char        ip[64];
    char        sspcode[4];
    char        block;
    char        monitor;
    char        checkTime[3];
    char        tryCount;
} R_EXT_SVR_REG_RQ_V6;
#endif
typedef struct
{
	char        sswid[3];
	char        ip[32];
	char        sspcode[4];
	char        block;
	char        monitor;
	char        checkTime[3];
	char        tryCount;
} R_LIST_REG_ITEM;

typedef struct
{
	DGM_HDR     hdr;
	char        count[3];
	char        data_ptr;
} R_EXT_SVR_LIST_REG_RQ;

typedef struct
{
	DGM_HDR     hdr;
	char        data[1860];
} S_TRACE_LIST_RP;

typedef struct
{
	DGM_HDR     hdr;
	char        index[2];
	char        from[64];
	char        to[64];
	char        svc[64];
	char        max[4];
} R_TRACE_ADD_RQ;

typedef struct
{
	DGM_HDR     hdr;
	char        index[2];
} R_TRACE_DEL_RQ;

typedef struct
{
    DGM_HDR hdr;
    char    dummy;
} BLACKLIST_UPD_REQ;    // add by SMCHO 20140918 - Black List Update Requst

typedef struct
{
    DGM_HDR hdr;
    char    result;     // '0': false, '1': OK
    char    reason;     // '1': size==0, '2':file open error
} BLACKLIST_UPD_REP;    // add by SMCHO 20140918 - Respose for BLACKLIST_REQ

typedef struct
{
    DGM_HDR hdr;
    char    act;        // '0': stop, '1': act
} BLACKLIST_ACT_REQ;    // add by SMCHO 20140918 - Black List Update Requst

typedef struct
{
    DGM_HDR hdr;
    char    result;     // '0': false, '1': OK
    char    state;      // '1': act, '0': stop
} BLACKLIST_ACT_REP;    // add by SMCHO 20140918 - Respose for BLACKLIST_REQ


#endif  // __SPY_XBUS_H__

