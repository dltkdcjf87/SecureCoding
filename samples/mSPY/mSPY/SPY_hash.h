//
//  SPY_hash.h
//  smSPY
//
//  Created by SMCHO on 2014. 3. 20..
//  Copyright (c) 2014년 SMCHO. All rights reserved.
//

#ifndef __SPY_HHASH_H__
#define __SPY_HHASH_H__

#include <iostream>
#include <map>
#include <list>
#include <stdint.h>
#include <sys/time.h>
#include <string.h>

extern uint8_t MY_SIDE; /* 20241126 kdh add */ 

using namespace std;

#ifndef __APPLE__
namespace stdext = ::__gnu_cxx;
#endif

/*
struct cTraceItem
{
	char strFrom[32];
	char strTo[32];
	char strCallId[128];
	char strSvcKey[64];
	unsigned short nMax;
	unsigned short nCount;
};
*/

typedef struct
{
#ifdef SES_HA_MODE
    bool    bIsMoved;           // SES HA 로 인해 보드가 변경되면 true로 Setting하고 이후 routing을 BoardNo로 하지 않고 PeerBoard로 한다.
    bool    bIsReceiveACK;      // ACK 를 수신한 통화중 상태인지를 나타내는 flag (SES가 통화중에만 이중화... 통화중이 아니면 절체시 release 처리(SCM으로 통보)해야 함)
    uint8_t PeerBoardNo;        // SES가 HA 절체가 되면 보내야할 Board
// 20230324 bible : #endif 위치 변경
#endif
    uint8_t BoardNo;			// SRM Board 번호
    int     ChannelNo;			// SRM Channel 번호
    int     nTrace;             // Index of Trace Item
// 20230324 bible : #endif 위치 변경    
//#endif

    char    strCscfIp[64];      // CSCF IP address
    int     nCscfPort;          // CSCF Port number
    
    string  Call_ID;			// Call 단위 구분되는 값
    
    time_t  tInsert;            // Insert Time
    
    bool    bDeath;             // FIXIT - SES Down flag - 사용 여부 확인
                                // board가 죽었을 죽기전 Callleg과 죽은후 생성된Callleg을 비교하기 위해서는 필요하지만
                                // Hash를 생성할 수 있으면 필요 없지 않을까?
                                // 그런데 이전에 생성된 Hash를 다 지워야 메모리 누수가 없을 듯.... 검토 필요
#ifdef IPV6_MODE
    bool    bIPv6;              // 상대편이 IPv6 인지 IPv4인지를 나타냄
#endif
} HASH_ITEM;


typedef struct
{
    int     BoardNo;			// SRM Board 번호
    int     ChannelNo;			// SRM Channel 번호
    int     nTrace;             // Index of Trace Item
    
    int     nCscfPort;          // CSCF Port number

    time_t  tInsert;            // Insert Time
    bool    bDeath;             // FIXIT - SES Down flag - 사용 여부 확인
    
    char    ot_type;            // O/T로 Hash를 나눈경우
#ifdef IPV6_MODE
    bool    bIPv6;              // IPv6 호 인지 v4호 인지 구분자
#endif
    
    char	strCall_ID[256];    // Call 단위 구분되는 값
    char    strCscfIp[64];      // CSCF IP address
} HA_HASH_SYNC;

typedef struct
{
    char	strCall_ID[256];    // Call 단위 구분되는 값
    bool    bIsReceiveACK;
} HA_HASH_UPDATE;

/* Class Header
 **pdh********************************************************
 * CLASS-NAME     : SPY_HASH_MAP
 * HISTORY        : 2014/03 - edit by SMCHO
 * DESCRIPTION    : SIP메세지를 보낼 CSCF 및 SAM 정보를 찾기위해
 *                : CallLeg별로 정보를 Map에 저장하고 관리하는 Class
 *                :
 * REMARKS        :ORIG/TERM으로 나누어서 사용하게 변경 됨
 **end*******************************************************/
class SPY_HASH_MAP
{
private:
    pthread_mutex_t     hash_mutex;
	multimap<int, size_t>  m_ChInfo_Map;       // Key 가 Board/Channel인 Map
    
public:
	map<string, size_t>    m_CallID_Map;       // Key 가 Call-Id인 Map
    
	SPY_HASH_MAP();
	~SPY_HASH_MAP();

	void    clear();                        // Clear All Items
	int     size();                         // 전체 m_CallID_Map 개수를 계산하는 함수
    int     ch_size();                      // 전체 m_ChInfo_Map 개수를 계산하는 함수
#ifdef IPV6_MODE
    bool    insert_hash(char *strCall_ID, char *strCscfIp, int nCscfPort, int nBoardNo, int nChannelNo, int nTrace, time_t tInsert, bool nIPv6);
#else
	bool    insert_hash(char *strCall_ID, char *strCscfIp, int nCscfPort, int nBoardNo, int nChannelNo, int nTrace, time_t tInsert);     // 새로운 CallLeg에 대한 정보를 추가하는 함수
#endif
	bool    find_hash  (char *strCall_ID, HASH_ITEM *ptrItem=NULL);     // MAP에서 Call-ID(Key)에 해당하는 item을 찾는 함수
	bool    update_hash(char *strCall_ID, HASH_ITEM *ptrItem);          // 에서 Call-ID(Key)에 해당하는 item을 Update하는 함수
	bool    delete_hash(int BoardNo, int ChannelNo);                    // Board+Channel을 Key로 Map을 삭제하는 함수
	int     dead_mark  (int BoardNo, int MaxCh);                        // Board과 연관된 모든 Call을 Death Mark하는 함수
    
    int     delete_by_board(int BoardNo, int MaxCh);                    // Board과 연관된 모든 Call을 삭제하는 함수 (unuse)
    
#ifdef SES_HA_MODE
    int     ha_move_mark(int down_board, int move_board, int MAX_CH, bool bIsORIG);   // down된 보드와 연관된 모든 Call을 move_board로 변경하는 함수
    int     check_twice_moved(int down_board, int move_board, int MAX_CH, bool bIsORIG);    // 한번 move된 board가 다시 반대로 move 되는 경우  이중화 데이터가 없어서 채널 해제해야 함

    bool    ha_update_ack(char *strCall_ID, bool bReceiveAck);          // HA시 통화중인지 구분을 위해 ACK를 수신한 경우에 Update하는 함수
#endif
//    int     trace_check(int BoardNo, int ChannelNo);                    // Trace Check
};



#endif // __SPY_HHASH_H__
