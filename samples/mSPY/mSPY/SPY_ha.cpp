//
//  SPY_ha.cpp
//  nsmSPY
//
//  Created by SMCHO on 2016. 5. 31..
//  Copyright  2016년 SMCHO. All rights reserved.
//

#include "SPY_def.h"
#include "SPY_xref.h"



#pragma mark -
#pragma mark 이중화 절체시 ACK를 수신하지 못한 채널에 대새 HAsh를 삭제하는 함수

#if 0
/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : HASH_DeletehaInfo
 * CLASS-NAME     : -
 * PARAMETER    IN: nBoardNo   - Delete할 SES board number
 * PARAMETER    IN: nChannelNo - Delete할 SES channel number
 * RET. VALUE     : BOOL
 * DESCRIPTION    : SCM에서 수신한 ChAllocRP를 처리하는 함수
 * REMARKS        :
 **end*******************************************************/
bool HASH_DeletehaInfo(int nBoardNo, int nChannelNo)
{
    if(g_Hash_O.delete_hash(nBoardNo, nChannelNo)) { Log.printf(LOG_LV1, "%s() [O] SCM Free(%d,%d) - OK  \n", __FUNCTION__, nBoardNo, nChannelNo); }
    else                                           { Log.printf(LOG_LV1, "%s() [O] SCM Free(%d,%d) - FAIL\n", __FUNCTION__, nBoardNo, nChannelNo); }
    
    if(g_Hash_T.delete_hash(nBoardNo, nChannelNo)) { Log.printf(LOG_LV1, "%s() [T] SCM Free(%d,%d) - OK  \n", __FUNCTION__, nBoardNo, nChannelNo); }
    else                                           { Log.printf(LOG_LV1, "%s() [T] SCM Free(%d,%d) - FAIL\n", __FUNCTION__, nBoardNo, nChannelNo); }
    
    return(true);
}
#endif

#pragma mark -
#pragma mark SCM에서 할당 받으면 CallLeg 정보(HashTable)를 Stanby로 전송하는 함수

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : HASH_InsertAndSync
 * CLASS-NAME     : -
 * PARAMETER    IN: pCR   - Call Register 구조체 포인터
 *              IN: t_now - Active가 HashTable에 INSERT한 시간
 * RET. VALUE     : BOOL
 * DESCRIPTION    : HashTable에 INSERT하고 이중화를 위해 정보를 STANDBY로 보내는 함수
 * REMARKS        :
 **end*******************************************************/
bool HASH_InsertAndSync(CR_DATA *pCR, time_t t_now)
{
    HA_HASH_SYNC    send;
#ifdef DEBUG
    int             result;
#endif
    
    
    pCR->info.nTrace = g_Trace.exist(pCR->info.strFrom, pCR->info.strTo, pCR->info.strServiceKey, pCR->bd, pCR->ch);  // Call Trace 정보
    
    /*
     * Insert To HashTable
     */
    Log.color(COLOR_CYAN, LOG_LV1, "[HASH] HASH_InsertAndSync() OT=%d\n", pCR->ot_type);
    if(pCR->ot_type == OT_TYPE_ORIG)
    {
#ifdef IPV6_MODE
        if(g_Hash_O.insert_hash(pCR->info.strCall_ID, pCR->info.strCscfIp, pCR->info.nCscfPort, pCR->bd, pCR->ch, pCR->info.nTrace, t_now, pCR->bIPV6) == false)
#else
            if(g_Hash_O.insert_hash(pCR->info.strCall_ID, pCR->info.strCscfIp, pCR->info.nCscfPort, pCR->bd, pCR->ch, pCR->info.nTrace, t_now) == false)
#endif
            {
                Log.printf(LOG_ERR, "[O] HASH_InsertAndSync() Hash_O Insert Fail - CallId=%s, Board=%d, Ch=%d, Cscf=%s:%d, \n", pCR->info.strCall_ID, pCR->bd, pCR->ch, pCR->info.strCscfIp,  pCR->info.nCscfPort);
                return(false);
            }
    }
    else
    {
#ifdef IPV6_MODE
        if(g_Hash_T.insert_hash(pCR->info.strCall_ID, pCR->info.strCscfIp, pCR->info.nCscfPort, pCR->bd, pCR->ch, pCR->info.nTrace, t_now, pCR->bIPV6) == false)
#else
            if(g_Hash_T.insert_hash(pCR->info.strCall_ID, pCR->info.strCscfIp, pCR->info.nCscfPort, pCR->bd, pCR->ch, pCR->info.nTrace, t_now) == false)
#endif
            {
                Log.printf(LOG_ERR, "[T] HASH_InsertAndSync() Hash_T Insert Fail - CallId=%s, Board=%d, Ch=%d, Cscf=%s:%d, \n", pCR->info.strCall_ID, pCR->bd, pCR->ch, pCR->info.strCscfIp,  pCR->info.nCscfPort);
                return(false);
            }
    }
    
    /*
     * Send to Standby SPY
     */
    bzero(&send, sizeof(send));
    
    send.BoardNo   = pCR->bd;                       // SRM Board 번호
    send.ChannelNo = pCR->ch;                       // SRM Channel 번호
    send.nTrace    = pCR->info.nTrace;              // Index of Trace Item
    send.nCscfPort = pCR->info.nCscfPort;;          // CSCF Port number
    send.tInsert   = t_now;                         // Insert Time
    send.bDeath    = false;                         // FIXIT - SES Down flag - 사용 여부 확인
    send.ot_type   = pCR->ot_type;                  // O/T로 Hash를 나눈경우
    strncpy(send.strCall_ID, pCR->info.strCall_ID, sizeof(send.strCall_ID)-1);  // Call 단위 구분되는 값
    strncpy(send.strCscfIp,  pCR->info.strCscfIp,  sizeof(send.strCscfIp)-1);   // CSCF IP address
    
#ifdef DEBUG
    if((result = ssendsig(sizeof(send), SCMD_HASH_INSERT_RQ, (uint8_t *)&send)) != 0)
    {
        if(result != 6) Log.printf(LOG_ERR, "HASH_InsertAndSync() ssendsig err=%d, Call-ID=%s\n", result, send.strCall_ID);
    }
#else
    ssendsig(sizeof(send), SCMD_HASH_INSERT_RQ, (uint8_t *)&send);
#endif
    return(true);
}



#pragma mark -
#pragma mark S-BUS 메세지를 전송하는 functions (peer SPY로 메시지를 전달하는 함수) 

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : HASH_HaUpdateRQ
 * CLASS-NAME     : -
 * PARAMETER INOUT: pCR - Call Register 구조체 포인터
 *                : bIsReceiveACK - ACK 수신 여부
 * RET. VALUE     : BOOL
 * DESCRIPTION    : HashTable 일부 정보를 이중화 peer SPY로 전달하는 함수
 * REMARKS        :
 **end*******************************************************/
void HASH_SendHaUpdateRQ(CR_DATA *pCR, bool bIsReceiveACK)
{
    HA_HASH_UPDATE    send;
    
    strcpy(send.strCall_ID, pCR->info.strCall_ID);  // key
    send.bIsReceiveACK = bIsReceiveACK;
    
    ssendsig(sizeof(send), SCMD_HASH_HA_UPDATE_RQ, (uint8_t *)&send);
}


#pragma mark -
#pragma mark S-BUS callback fuctions (peer에서 수신한 메시지 처리 함수)

// 20241202 kdh add
bool HASH_InsertTableRQ(XBUS_MSG *rXBUS)
{
	void* msg;

	msg = malloc(sizeof(XBUS_MSG));
	memset(msg, 0x00, sizeof(XBUS_MSG));
	memcpy((char*)msg, (char*)rXBUS, sizeof(XBUS_MSG));

	return (Q_Sync_recv.enqueue(msg));
}

// 20241202 kdh add
void* HASH_InsertTableThread(void* arg)
{
	XBUS_MSG *rXBUS = NULL;
	static int recv_cnt = 0;

	while(1)
	{
		if(g_nHA_State != HA_Standby)
		{
			if(recv_cnt != 0)
			{
				Log.printf(LOG_INF, "HASH_InsertTableThread() recv_cnt reset !! (%d)\n", recv_cnt);
				recv_cnt = 0;
			}
			msleep(1);
			continue;
		}
		
		if(Q_Sync_recv.dequeue((void **)&rXBUS) == false)        // queue empty
        {
            msleep(1);     // 50 msec
            continue;
        }

		HASH_InsertTable(rXBUS);

		recv_cnt++;
		if( (recv_cnt%10000) == 0 )
			Log.printf(LOG_INF, "HASH_InsertTableThread() recv_cnt=(%d)\n", recv_cnt);

		free(rXBUS);
	}
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : HASH_InsertTableRQ
 * CLASS-NAME     : -
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : Active Standby간 HashTable의 내용을 Sync 하는 함수
 * REMARKS        : copy from active to standby  (FIXIT: class로 이동 ??)
 **end*******************************************************/
//bool HASH_InsertTableRQ(XBUS_MSG *rXBUS)
bool HASH_InsertTable(XBUS_MSG *rXBUS) // 20241202 kdh chg
{
    HA_HASH_SYNC    *rcv = (HA_HASH_SYNC *)rXBUS->Data;
    
    if (rcv->strCall_ID[0] == '\0')
    {
        Log.printf(LOG_ERR, "HASH_InsertTable() - received Call_ID is NULL\n");
        return(false);
    }
    
    if(rcv->ot_type == OT_TYPE_ORIG)
    {
#ifdef IPV6_MODE
        if(g_Hash_O.insert_hash(rcv->strCall_ID, rcv->strCscfIp, rcv->nCscfPort, rcv->BoardNo, rcv->ChannelNo, rcv->nTrace, rcv->tInsert, rcv->bIPv6) == false)
        {
            Log.printf(LOG_ERR, "[O] Standby Hash INSERT Fail - Board=%d, Ch=%d, nTrace=%d, Cscf=%s:%d, CallId=%s, IP=%s\n", rcv->BoardNo, rcv->ChannelNo, rcv->nTrace, rcv->strCscfIp, rcv->nCscfPort, rcv->strCall_ID, (rcv->bIPv6 == true)?"v6":"v4");
        }
#else
        if(g_Hash_O.insert_hash(rcv->strCall_ID, rcv->strCscfIp, rcv->nCscfPort, rcv->BoardNo, rcv->ChannelNo, rcv->nTrace, rcv->tInsert) == false)
        {
            Log.printf(LOG_ERR, "[O] Standby Hash INSERT Fail - Board=%d, Ch=%d, nTrace=%d, Cscf=%s:%d, CallId=%s\n", rcv->BoardNo, rcv->ChannelNo, rcv->nTrace, rcv->strCscfIp, rcv->nCscfPort, rcv->strCall_ID);
        }
#endif
        
    }
    else
    {
#ifdef IPV6_MODE
        if(g_Hash_T.insert_hash(rcv->strCall_ID, rcv->strCscfIp, rcv->nCscfPort, rcv->BoardNo, rcv->ChannelNo, rcv->nTrace, rcv->tInsert, rcv->bIPv6) == false)
        {
            Log.printf(LOG_ERR, "[T] Standby Hash INSERT Fail - Board=%d, Ch=%d, nTrace=%d, Cscf=%s:%d, CallId=%s, IP=%s\n", rcv->BoardNo, rcv->ChannelNo, rcv->nTrace, rcv->strCscfIp, rcv->nCscfPort, rcv->strCall_ID, (rcv->bIPv6 == true)?"v6":"v4");
        }
#else
        if(g_Hash_T.insert_hash(rcv->strCall_ID, rcv->strCscfIp, rcv->nCscfPort, rcv->BoardNo, rcv->ChannelNo, rcv->nTrace, rcv->tInsert) == false)
        {
            Log.printf(LOG_ERR, "[T] Standby Hash INSERT Fail - Board=%d, Ch=%d, nTrace=%d, Cscf=%s:%d, CallId=%s\n", rcv->BoardNo, rcv->ChannelNo, rcv->nTrace, rcv->strCscfIp, rcv->nCscfPort, rcv->strCall_ID);
        }
#endif
        
        
    }
    return(true);
}

#define SYNC_COUNT_1_LOOP       80

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : send_sync
 * CLASS-NAME     :
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : HashTable의 내용을 standby로 전송 하는 함수
 * REMARKS        : 초당 5,000개가 최대임 20msec 씩 sleep을 하니까 1초에 sleep 을 기준으로
 *                : 500 Loop가 실행이 되며 Loop당 100개씩 보내면 약 5,000TPS가 된다.
 *                : (sleep 기준이므로 실제 전송하는 시간까지 포함하면 5,000이 안됨)
 *                : 전송할 데이터가 15만개인경우 이론으로는 30초이나 실제 31~32초가 걸림
 *                : 최대 호처리중 Sync가 발생하면 안되니  Loop ekd 90번씩 전송 (4,000 TPS)
 *                : 10만건 전송에 25~26초, 15만건은 약 38~39초
 **end*******************************************************/
void *send_sync(void* arg)
{
    //    int         size_map, size_list, count = 0;
    int         result;
    size_t      o_map_size, t_map_size;
    size_t      o_list_size, t_list_size;
    int         o_send_count = 0, t_send_count = 0;
    list<char *>    oList;
    list<char *>    tList;
    string          strString;
    char            *strCall_ID;
    size_t  nLen;
    list<char *>::iterator aPos;
    map<string, size_t>::iterator itr;
    HA_HASH_SYNC    send;
    HASH_ITEM       HashItem;
    //    HASH_ITEM       *ptrHash;
    char    strComment[64];
    
    o_map_size = g_Hash_O.size();
    t_map_size = g_Hash_T.size();
    
    for(itr = g_Hash_O.m_CallID_Map.begin(); itr != g_Hash_O.m_CallID_Map.end(); itr ++)
    {
        strString = itr->first;
        nLen = strString.size() + 1;
        strCall_ID = (char *)malloc(nLen);
        strcpy(strCall_ID, strString.c_str());
        
        oList.push_back(strCall_ID);        // 모든 oMap 을 list에 저장
    }
    
    o_list_size = (int)oList.size();
    
    Log.color(COLOR_YELLOW, LOG_INF, "%s() START O-HASH SYNC (map=%d, list=%d)\n", __FUNCTION__, o_map_size, o_list_size);
    snprintf(strComment, 63, "START O-HASH SYNC(%d, %d)",(int)o_map_size, (int)o_list_size);
    SEND_StatusMessage(STS_ID_SPY_CH_SYNC, strComment);
    
    for(aPos = oList.begin(); aPos != oList.end(); ++aPos)
    {
        strCall_ID = *aPos;
        
//        Log.color(COLOR_YELLOW, LOG_LV2, "%s() O-HASH Sync CAll-ID=[%s]\n", __FUNCTION__, strCall_ID);
        
        if(g_Hash_O.find_hash((char *)strCall_ID, (HASH_ITEM *)&HashItem) == true)
        {
            bzero(&send, sizeof(send));
            
            send.BoardNo   = HashItem.BoardNo;                  // SRM Board 번호
            send.ChannelNo = HashItem.ChannelNo;                // SRM Channel 번호
            send.nTrace    = HashItem.nTrace;                   // Index of Trace Item
            send.nCscfPort = HashItem.nCscfPort;;               // CSCF Port number
            send.tInsert   = HashItem.tInsert;                  // Insert Time
            send.bDeath    = HashItem.bDeath;                   // FIXIT - SES Down flag - 사용 여부 확인
            send.ot_type   = OT_TYPE_ORIG;                      // O/T로 Hash를 나눈경우
            strncpy(send.strCall_ID, HashItem.Call_ID.c_str(), 255);  // Call 단위 구분되는 값
            strncpy(send.strCscfIp,  HashItem.strCscfIp, 63);        // CSCF IP address
            
            if(send.strCall_ID[0] == '\0') { continue; }         // already deleted ?

            if((result = ssendsig(sizeof(send), SCMD_HASH_INSERT_RQ, (uint8_t *)&send)) != 0)
            {
                Log.printf(LOG_ERR, "HASH_SyncHashTable(O) ssendsig err=%d, Call-ID=%s\n", result, send.strCall_ID);
            }
            else
            {
                Log.color(COLOR_YELLOW, LOG_LV2, "%s() O-HASH Sync bd=%d, ch=%d, Call-ID=[%s] CSCF=[%s]\n", __FUNCTION__, send.BoardNo, send.ChannelNo, send.strCall_ID, send.strCscfIp);

                o_send_count ++;
            }
            
            if((o_send_count % SYNC_COUNT_1_LOOP) == 0) { msleep(15); } // 20241128 kdh chg msleep(20) -> 15
        }
        
        free(strCall_ID);
    }
    
    Log.color(COLOR_YELLOW, LOG_INF, "%s() End O-HASH Sync msend=%d\n", __FUNCTION__, o_send_count);
    snprintf(strComment, 63, "End O-HASH SYNC(%d)",(int)o_send_count);
    SEND_StatusMessage(STS_ID_SPY_CH_SYNC, strComment);
    msleep(100);
    

    for(itr = g_Hash_T.m_CallID_Map.begin(); itr != g_Hash_T.m_CallID_Map.end(); itr ++)
    {
        strString = itr->first;
        nLen = strString.size() + 1;
        strCall_ID = (char *)malloc(nLen);
        strcpy(strCall_ID, strString.c_str());
        
        tList.push_back(strCall_ID);          // 모든 tMap 을 list에 저장
    }
    
    t_list_size = (int)tList.size();
    
    Log.color(COLOR_YELLOW, LOG_INF, "%s() START T-HASH Sync(map=%d, list=%d)\n", __FUNCTION__, t_map_size, t_list_size);
    snprintf(strComment, 63, "START T-HASH SYNC(%d, %d)",(int)t_map_size, (int)t_list_size);
    SEND_StatusMessage(STS_ID_SPY_CH_SYNC, strComment);
    
    for(aPos = tList.begin(); aPos != tList.end(); ++aPos)
    {
        strCall_ID = *aPos;
        
//        Log.color(COLOR_YELLOW, LOG_LV2, "%s() T-HASH Sync CAll-ID=[%s]\n", __FUNCTION__, strCall_ID);
        
        if(g_Hash_T.find_hash((char *)strCall_ID, (HASH_ITEM *)&HashItem) == true)
        {
            bzero(&send, sizeof(send));
            
            send.BoardNo   = HashItem.BoardNo;                  // SRM Board 번호
            send.ChannelNo = HashItem.ChannelNo;                // SRM Channel 번호
            send.nTrace    = HashItem.nTrace;                   // Index of Trace Item
            send.nCscfPort = HashItem.nCscfPort;;               // CSCF Port number
            send.tInsert   = HashItem.tInsert;                  // Insert Time
            send.bDeath    = HashItem.bDeath;                   // FIXIT - SES Down flag - 사용 여부 확인
            send.ot_type   = OT_TYPE_TERM;                      // O/T로 Hash를 나눈경우
            strncpy(send.strCall_ID, HashItem.Call_ID.c_str(), 255);  // Call 단위 구분되는 값
            strncpy(send.strCscfIp,  HashItem.strCscfIp, 63);        // CSCF IP address
            
            if(send.strCall_ID[0] == '\0') { continue; }         // already deleted ?
            
            if((result = ssendsig(sizeof(send), SCMD_HASH_INSERT_RQ, (uint8_t *)&send)) != 0)
            {
                Log.printf(LOG_ERR, "HASH_SyncHashTable(O) ssendsig err=%d, Call-ID=%s\n", result, send.strCall_ID);
            }
            else
            {
                Log.color(COLOR_YELLOW, LOG_LV2, "%s() T-HASH Sync bd=%d, ch=%d, Call-ID=[%s] CSCF=[%s]\n", __FUNCTION__, send.BoardNo, send.ChannelNo, send.strCall_ID, send.strCscfIp);
                t_send_count ++;
            }
            
            if((t_send_count % SYNC_COUNT_1_LOOP) == 0) { msleep(15); } // 20241128 kdh chg msleep(20) -> 15
        }
        free(strCall_ID);
    }
    
    Log.color(COLOR_YELLOW, LOG_INF, "%s() End T-HASH Sync msend=%d\n", __FUNCTION__, t_send_count);
    Log.printf(LOG_INF, "%s() Hash Map Sync Finish\n", __FUNCTION__);
    snprintf(strComment, 63, "END T-HASH SYNC(%d)",(int)t_send_count);
    SEND_StatusMessage(STS_ID_SPY_CH_SYNC, strComment);
    SEND_StatusMessage(STS_ID_SPY_CH_SYNC, "HASH MAP SYNC FINISH !!");
    
    return(0);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : HASH_SyncAllTableRQ
 * CLASS-NAME     :
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : Active Standby간 HashTable의 내용을 Sync 하는 함수
 * REMARKS        : copy from active to standby (FIXIT: class로 이동 ??)
 **end*******************************************************/
void HASH_SyncAllTableRQ(void)
{
    pthread_t   tid;
    
    pthread_create(&tid, NULL, send_sync, NULL);
    return;
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : HASH_HaUpdateRQ
 * CLASS-NAME     : -
 * PARAMETER    IN: rXBUS -  XBUS로 수신한 메세지 포인터
 * RET. VALUE     : -
 * DESCRIPTION    : Active Standby간 HashTable의 내용을  update 하는 함수
 * REMARKS        :
 **end*******************************************************/
void HASH_HaUpdateRQ(XBUS_MSG *rXBUS)
{
    HA_HASH_UPDATE    *rcv = (HA_HASH_UPDATE *)rXBUS->Data;
    
    if (rcv->strCall_ID[0] == '\0')
    {
        Log.printf(LOG_ERR, "%s() - received Call_ID is NULL\n", __FUNCTION__);
        return;
    }

// 20230324 bible : SES_HA_MODE define 추가
#ifdef SES_HA_MODE
#ifdef SES_ACTSTD_MODE /* 20241126 kdh add */
	/* 필요한가 ??  */
#else
    g_Hash_O.ha_update_ack(rcv->strCall_ID, rcv->bIsReceiveACK);     // bIsReceiveACK update
#endif
#else
	// SES_HA_MODE가 아닌 경우 bIsReceiveACK 값 Update 의미 없음
#endif
}

