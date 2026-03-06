/* File Header
 *fsh**************************************************************
 ******************************************************************
 **
 **  FILE : SPY_hash.cpp
 **
 ******************************************************************
 ******************************************************************
 BLOCK          : SPY
 SUBSYSTEM      : CMS
 SOR-NAME       :
 VERSION        : V4.X
 DATE           : 2014/03/
 AUTHOR         : SEUNG-MO, CHO
 HISTORY        :
 PROCESS(TASK)  :
 PROCEDURES     :
 DESCRIPTION    : SIP Call-Leg에 대한 정보를 저장하고 관리하는 Hash Table 함수
 REMARKS        :
 *end*************************************************************/

#include "btxbus3.h"
#include "SPY_hash.h"
#include "libnsmlog.h"

extern NSM_LOG  Log;
extern char     g_nHA_State;

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SPY_HASH_MAP
 * CLASS-NAME     : SPY_HASH_MAP
 * PARAMETER INOUT: -
 * RET. VALUE     : -
 * DESCRIPTION    : SPY_HASH_MAP 클래스 생성자
 * REMARKS        : 
 **end*******************************************************/
SPY_HASH_MAP::SPY_HASH_MAP()
{
    pthread_mutex_init(&hash_mutex, NULL);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : ~SPY_HASH_MAP
 * CLASS-NAME     : SPY_HASH_MAP
 * PARAMETER INOUT: -
 * RET. VALUE     : -
 * DESCRIPTION    : SPY_HASH_MAP 클래스 소멸자
 * REMARKS        :
 **end*******************************************************/
SPY_HASH_MAP::~SPY_HASH_MAP()
{
	clear();
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : clear
 * CLASS-NAME     : SPY_HASH_MAP
 * PARAMETER INOUT: -
 * RET. VALUE     : -
 * DESCRIPTION    : Hash Table에 저장된 모든 Item을 삭제
 * REMARKS        :
 **end*******************************************************/
void SPY_HASH_MAP::clear()
{
	map<string, size_t>::iterator itr;

    pthread_mutex_lock(&hash_mutex);
    {
        for(itr = m_CallID_Map.begin(); itr != m_CallID_Map.end(); itr++)
        {
            delete ((HASH_ITEM *)(itr->second));
        }
        
        m_CallID_Map.clear();

        m_ChInfo_Map.clear();
    }
    pthread_mutex_unlock(&hash_mutex);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : size
 * CLASS-NAME     : SPY_HASH_MAP
 * PARAMETER INOUT: -
 * RET. VALUE     : CallLeg 수
 * DESCRIPTION    : 전체 CallLeg 개수를 구하는 함수
 * REMARKS        :
 **end*******************************************************/
int SPY_HASH_MAP::size()
{
	return((int)m_CallID_Map.size());
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : size
 * CLASS-NAME     : SPY_HASH_MAP
 * PARAMETER INOUT: -
 * RET. VALUE     : CallLeg 수
 * DESCRIPTION    : 전체 CallLeg 개수를 구하는 함수
 * REMARKS        :
 **end*******************************************************/
int SPY_HASH_MAP::ch_size()
{
    return((int)m_ChInfo_Map.size());
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : insert_hash
 * CLASS-NAME     : SPY_HASH_MAP
 * PARAMETER    IN: strCall_ID - Call-Id 문자열값 (Key)
 *              IN: strCscfIp  - SIP 메세지를 보내거나 받는 상대편 CSCF IP
 *              IN: nCscfPort  - SIP 메세지를 보내거나 받는 상대편 CSCF PORT
 *              IN: nBoardNo   - SES Board Number
 *              IN: nChannelNo - SES Channel Number
 *              IN: nTrace     - Call Trace 정보
 *              IN: tInsert    - Hash에 Insert 하는 시간(Audit 용)
 * RET. VALUE     : BOOL
 * DESCRIPTION    : Hash Map에 새로운 CallLeg 정보를 INSERT 하는 함수
 * REMARKS        : HA Sync는 별도로 진행
 **end*******************************************************/
#ifdef IPV6_MODE
bool SPY_HASH_MAP::insert_hash(char *strCall_ID, char *strCscfIp, int nCscfPort, int nBoardNo, int nChannelNo, int nTrace, time_t tInsert, bool bIPv6)
#else
bool SPY_HASH_MAP::insert_hash(char *strCall_ID, char *strCscfIp, int nCscfPort, int nBoardNo, int nChannelNo, int nTrace, time_t tInsert)
#endif
{
	int         nBoardChannel;
	HASH_ITEM   *newHashItem;
    
	try
	{
		newHashItem = new HASH_ITEM;
	}
	catch (exception)
	{
		return(false);
	}
   
	// 20240909 bible : Modify nBoardNo << 16 -> nBoardNo << 24 
	//nBoardChannel = (nBoardNo << 16) | nChannelNo;
	nBoardChannel = (nBoardNo << 24) | nChannelNo;
    
	newHashItem->BoardNo    = nBoardNo;
	newHashItem->ChannelNo  = nChannelNo;
	newHashItem->Call_ID    = strCall_ID;
	newHashItem->bDeath     = false;
	newHashItem->nTrace     = nTrace;
    newHashItem->tInsert    = tInsert;
    newHashItem->nCscfPort  = nCscfPort;
#ifdef IPV6_MODE
    newHashItem->bIPv6      = bIPv6;
#endif
#ifdef SES_HA_MODE
#ifdef SES_ACTSTD_MODE /* 20241126 kdh add */
#else
    newHashItem->bIsMoved      = false;
    newHashItem->bIsReceiveACK = false;
    newHashItem->PeerBoardNo   = nBoardNo; // 기존 board로 초기화
#endif /* SES_ACTSTD_MODE */
#endif

    strcpy(newHashItem->strCscfIp, strCscfIp);
    
    pthread_mutex_lock(&hash_mutex);
    {
        m_ChInfo_Map.insert(make_pair(nBoardChannel,        (size_t)newHashItem));
        m_CallID_Map.insert(make_pair(newHashItem->Call_ID, (size_t)newHashItem));
    }
	pthread_mutex_unlock(&hash_mutex);
    
Log.color(COLOR_CYAN, LOG_LV1, "[HASH] Insert Call-ID=%s\n", newHashItem->Call_ID.c_str());
	return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : find_hash
 * CLASS-NAME     : SPY_HASH_MAP
 * PARAMETER    IN: strCall_ID - Call-Id 문자열값 (Key)
 *             OUT: ptrItem    - strCall_ID에 해당하는 MAP의 cHashItem 포인터
 * RET. VALUE     : BOOL
 * DESCRIPTION    : Hash Map에서 Call-ID에 해당하는 정보를 찾는 함수
 * REMARKS        :
 **end*******************************************************/
bool SPY_HASH_MAP::find_hash(char* strCall_ID, HASH_ITEM *ptrItem)
{
    string  strKey = strCall_ID;
    map<string, size_t>::iterator itr;

    if(ptrItem == NULL) { return(false); }
    
	pthread_mutex_lock(&hash_mutex);
    {
        itr = m_CallID_Map.find(strKey);
        
        if(itr != m_CallID_Map.end())
        {
            *ptrItem = *((HASH_ITEM *)itr->second);
            pthread_mutex_unlock(&hash_mutex);
            
            return(true);
        }
    }
	pthread_mutex_unlock(&hash_mutex);

	return(false);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : update_hash
 * CLASS-NAME     : SPY_HASH_MAP
 * PARAMETER    IN: strCall_ID - Call-Id 문자열값 (Key)
 *              IN: ptrItem    - Update 할 cHashItem 포인터
 * RET. VALUE     : BOOL
 * DESCRIPTION    : Hash Map에서 Call-ID에 해당하는 정보를 Update 하는 함수
 * REMARKS        : HA Sync는 별도로 수행햐야 함
 **end*******************************************************/
bool SPY_HASH_MAP::update_hash(char* strCall_ID, HASH_ITEM *ptrItem)
{
    string  strKey = strCall_ID;
	map<string, size_t>::iterator itr;
    
    pthread_mutex_lock(&hash_mutex);
    {
        itr = m_CallID_Map.find(strKey);
        
        if (itr != m_CallID_Map.end())
        {
            memcpy((void *)itr->second, ptrItem, sizeof(HASH_ITEM));
//            *((HASH_ITEM *)itr->second) = *ptrItem;
            pthread_mutex_unlock(&hash_mutex);

            return(true);
        }
    }
	pthread_mutex_unlock(&hash_mutex);
	return(false);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : delete_hash
 * CLASS-NAME     : SPY_HASH_MAP
 * PARAMETER    IN: BoardNo   - Board   번호(Key)
 *              IN: ChannelNo - Channel 번호(Key)
 * RET. VALUE     : BOOL
 * DESCRIPTION    : Hash Map에서 board/channel에 해당하는 MAP을 삭제하는 함수
 * REMARKS        : CallID_MAP과 ChInfo_Map 둘 다 삭제
 **end*******************************************************/
bool SPY_HASH_MAP::delete_hash(int BoardNo, int ChannelNo)
{
	multimap<int, size_t>::iterator    itr;
	HASH_ITEM   *ptrItem;
	int         nBoardChannel;
   
	// 20240909 bible : Modify nBoardNo << 16 -> nBoardNo << 24  
    //nBoardChannel = (BoardNo << 16) | ChannelNo;
    nBoardChannel = (BoardNo << 24) | ChannelNo;
    
	pthread_mutex_lock(&hash_mutex);
    {
        try
        {
            for(itr = m_ChInfo_Map.lower_bound(nBoardChannel); itr != m_ChInfo_Map.upper_bound(nBoardChannel); ++itr)
            {
                ptrItem = ((HASH_ITEM *)(itr->second));
                m_CallID_Map.erase(ptrItem->Call_ID);
                delete ptrItem;
            }
        }
        catch (exception)
        {
            pthread_mutex_unlock(&hash_mutex);
            return(false);
        }
        
        m_ChInfo_Map.erase(nBoardChannel);
    }
	pthread_mutex_unlock(&hash_mutex);
    
	return(true);
}



/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : delete_by_board
 * CLASS-NAME     : SPY_HASH_MAP
 * PARAMETER    IN: BoardNo - Board   번호(Key)
 *              IN: MaxCh   - Baord의 최대 Channel 수
 * RET. VALUE     : delete_count
 * DESCRIPTION    : Hash Map에서 board에 해당하는 모든 MAP을 삭제하는 함수
 * REMARKS        : GODRM 최종 소스에서는 사용 안하는 함수...... why ??
 **end*******************************************************/
int SPY_HASH_MAP::delete_by_board(int BoardNo, int MaxCh)
{
	multimap<int, size_t>::iterator    itr;
	HASH_ITEM   *ptrItem;
    int         wBoardNo;
	int         nBoardChannel;
    int         count = 0;
    
    // FIXIT: - 이거 보드당 18,000번 돌아야 하는데..... 너무 무식하지 않나??
    // 현재는 사용안하기는 하는데........
    // 차라리 보드 ChInfo_Map을 보드 별로 만드는게 좋지 않을까????
   
	// 20240909 bible : Modify nBoardNo << 16 -> nBoardNo << 24 
    //wBoardNo = (BoardNo << 16) & 0xFFFF0000;
    wBoardNo = (BoardNo << 24) & 0xFF000000;
    
    pthread_mutex_lock(&hash_mutex);
    {
        for(int nLoop = 0; nLoop < MaxCh; nLoop++)
        {
            nBoardChannel = wBoardNo | nLoop;
            
            for(itr = m_ChInfo_Map.lower_bound(nBoardChannel); itr != m_ChInfo_Map.upper_bound(nBoardChannel); ++ itr)
            {
                ptrItem = ((HASH_ITEM *)(itr->second));
                m_CallID_Map.erase(ptrItem->Call_ID);
                delete ptrItem;
                count ++;
            }
            
            m_ChInfo_Map.erase(nBoardChannel);
        }
    }
    pthread_mutex_unlock(&hash_mutex);
    return(count);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : DeadByBoard
 * CLASS-NAME     : SPY_HASH_MAP
 * PARAMETER    IN: BoardNo - Board   번호(Key)
 *              IN: MaxCh   - Baord의 최대 Channel 수
 * RET. VALUE     : BOOL
 * DESCRIPTION    : Hash Map에서 board에 해당하는 모든 MAP의 item에 death mark하는 함수
 * REMARKS        : FIXIT - mark할 지 삭제 할 지...
 **end*******************************************************/
int SPY_HASH_MAP::dead_mark(int BoardNo, int MaxCh)
{
	multimap<int, size_t>::iterator    itr;
	HASH_ITEM   *ptrItem;
	int         wBoardNo;
    int         nBoardChannel;
    int         count = 0;
    
    // FIXIT: - 이거 보드당 18,000번 돌아야 하는데..... 너무 무식하지 않나??
    // 현재는 사용안하기는 하는데........
    // 차라리 보드 ChInfo_Map을 보드 별로 만드는게 좋지 않을까????
   
	// 20240909 bible : Modify nBoardNo << 16 -> nBoardNo << 24 
    //wBoardNo = (BoardNo << 16) & 0xFFFF0000;
    wBoardNo = (BoardNo << 24) & 0xFF000000;

    pthread_mutex_lock(&hash_mutex);
    {
        for(int nLoop = 0; nLoop < MaxCh; nLoop++)
        {
            nBoardChannel = wBoardNo | nLoop;
            
            for(itr = m_ChInfo_Map.lower_bound(nBoardChannel); itr != m_ChInfo_Map.upper_bound(nBoardChannel); ++ itr)
            {
                ptrItem = ((HASH_ITEM *)(itr->second));
                ptrItem->bDeath = true;
                count++;
            }
        }
    }
    pthread_mutex_unlock(&hash_mutex);
	return(count);
}

#ifdef SES_HA_MODE
extern int  SendChDeallocRQ_SCM(int nBoard, int nChannel);
extern bool HASH_DeletehaInfo(int nBoardNo, int nChannelNo);

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : ha_move_mark
 * CLASS-NAME     : SPY_HASH_MAP
 * PARAMETER    IN: down_board - down 된 Board   번호(Key)
 *              IN: move_board - SES HA 결과로 변경되는 Board 번호
 *              IN: MaxCh   - Baord의 최대 Channel 수
 *              IN: bIsORIG - Hash가 ORIG것인지 여부
 * RET. VALUE     : BOOL
 * DESCRIPTION    : Hash Map에서 board에 해당하는 모든 MAP의 item에 death mark하는 함수
 * REMARKS        : FIXIT - mark할 지 삭제 할 지...
 **end*******************************************************/
int SPY_HASH_MAP::ha_move_mark(int down_board, int move_board, int MAX_CH, bool bIsORIG)
{
    multimap<int, size_t>::iterator    itr;
    list<int>           aList;
    list<int>::iterator itr_list;
    HASH_ITEM   *ptrItem;
    int         wBoardNo;
    int         nBoardChannel;
    int         count = 0;
    int         n_delete_count = 0;
    
    // FIXIT: - 이거 보드당 18,000번 돌아야 하는데..... 너무 무식하지 않나??
    // 현재는 사용안하기는 하는데........
    // 차라리 보드 ChInfo_Map을 보드 별로 만드는게 좋지 않을까????
   
	// 20240909 bible : Modify nBoardNo << 16 -> nBoardNo << 24 
    //wBoardNo = (down_board << 16) & 0xFFFF0000;
    wBoardNo = (down_board << 24) & 0xFF000000;
    
    if(bIsORIG == false)        // TERM
    {
        pthread_mutex_lock(&hash_mutex);
        {
            for(int nLoop = 0; nLoop < MAX_CH; nLoop++)     // FIXIT - channel이 32767 을 넘으면 문제가 발생한다.... 현재는 18,000이지만 대용량에서는 문제...
            {
                nBoardChannel = wBoardNo | nLoop;
                
                for(itr = m_ChInfo_Map.lower_bound(nBoardChannel); itr != m_ChInfo_Map.upper_bound(nBoardChannel); ++ itr)
                {
                    ptrItem = ((HASH_ITEM *)(itr->second));

                    if(ptrItem->bIsMoved == false)
                    {
#ifdef DEBUG_MODE
                        Log.color(COLOR_CYAN, LOG_LV1, "[T] %s() bd=%d, ch=%d first moved=%d\n", __FUNCTION__, ptrItem->BoardNo, ptrItem->ChannelNo, move_board);
#endif
                        ptrItem->bIsMoved    = true;
                        ptrItem->PeerBoardNo = move_board;
                        count++;
                    }
                }
            }
        }
        pthread_mutex_unlock(&hash_mutex);
    }
    else        // ORIG
    {
        pthread_mutex_lock(&hash_mutex);
        {
            for(int nLoop = 0; nLoop < MAX_CH; nLoop++)     // FIXIT - channel이 32767 을 넘으면 문제가 발생한다.... 현재는 18,000이지만 대용량에서는 문제...
            {
                nBoardChannel = wBoardNo | nLoop;
                
                for(itr = m_ChInfo_Map.lower_bound(nBoardChannel); itr != m_ChInfo_Map.upper_bound(nBoardChannel); ++ itr)
                {
                    ptrItem = ((HASH_ITEM *)(itr->second));

                    if(ptrItem->bIsMoved == false)
                    {
#ifdef DEBUG_MODE
                        Log.color(COLOR_CYAN, LOG_LV1, "[O] %s() bd=%d, ch=%d first moved=%d\n", __FUNCTION__, ptrItem->BoardNo, ptrItem->ChannelNo, move_board);
#endif
                        if(ptrItem->bIsReceiveACK == false)
                        {
                            aList.push_back((int)nLoop);   // push hCallleg
                            n_delete_count ++;
                        }
                        
                        ptrItem->bIsMoved    = true;
                        ptrItem->PeerBoardNo = move_board;
                        count++;
                    }
                }
            }
        }
        pthread_mutex_unlock(&hash_mutex);
    }
    
    if(n_delete_count == 0) { return(count); }

#ifdef MPBX_MODE    // 170206
    {
        int         nChannelNo;
        
        // ACK 수신전에 절체된 채널은 이중화를 하지 않기 때문에 삭제해야 한다.
        for(itr_list = aList.begin(); itr_list != aList.end(); ++itr_list)
        {
            nChannelNo = (int)*itr_list;
            
#ifdef SES_ACTSTD_MODE /* 20241126 kdh add */
			SendChDeallocRQ_SCM(MY_SIDE, nChannelNo);
#else
            SendChDeallocRQ_SCM(down_board, nChannelNo);        //SCM에 통보
#endif /* SES_ACTSTD_MODE */
            //        HASH_DeletehaInfo  (down_board,  nChannelNo);       //SPY Hash 삭제 - SCM에서 ChDeallocRQ를 받으면 채널 해제 요청을 한다.
        }
    }
#endif
    
#ifdef DEBUG_MODE
    Log.color(COLOR_CYAN, LOG_LV1, "%s() bd=%d END....\n", __FUNCTION__, down_board);
#endif
    return(count);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : check_twice_moved
 * CLASS-NAME     : SPY_HASH_MAP
 * PARAMETER    IN: down_board - down 된 Board 번호(Key)
 *              IN: move_board - down 된 Board 의 peer 번호
 *              IN: MaxCh   - Baord의 최대 Channel 수
 *              IN: bIsORIG - Hash가 ORIG것인지 여부
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 한번 move된 board가 다시 반대로 move 되는 경우를 검사하는 함수
 * REMARKS        : 한번 move된 board가 다시 반대로 move 되는 경우  이중화 데이터가 없어서 채널 해제해야 함
 **end*******************************************************/
int SPY_HASH_MAP::check_twice_moved(int down_board, int move_board, int MAX_CH, bool bIsORIG)
{
    multimap<int, size_t>::iterator    itr;
    list<int>           aList;
    list<int>::iterator itr_list;
    HASH_ITEM   *ptrItem;
    int         nChannelNo;
    int         wBoardNo;
    int         nBoardChannel;
    int         n_twice_count  = 0;

    
    // 이전에 down되어서 넘어온 채널이 다시 다운 되었는지를 확인하기 위해서는 다운된 board가 아니라 peer 보드 번호로 check한다.
    // 20240909 bible : Modify nBoardNo << 16 -> nBoardNo << 24
    //wBoardNo = (move_board << 16) & 0xFFFF0000;
    wBoardNo = (move_board << 24) & 0xFF000000;
    
    // move되었다가 반대로 move되었는지는 ORIG만 check 한다. (ORIG chnnel 번호로 ChDeAlloc 한번만 하면 됨)
    pthread_mutex_lock(&hash_mutex);
    {
        for(int nLoop = 0; nLoop < MAX_CH; nLoop++)     // FIXIT - channel이 32767 을 넘으면 문제가 발생한다.... 현재는 18,000이지만 대용량에서는 문제...
        {
            nBoardChannel = wBoardNo | nLoop;
            
            for(itr = m_ChInfo_Map.lower_bound(nBoardChannel); itr != m_ChInfo_Map.upper_bound(nBoardChannel); ++ itr)
            {
                ptrItem = ((HASH_ITEM *)(itr->second));
                
                if((ptrItem->bIsMoved == true) && (ptrItem->PeerBoardNo == down_board))
                {
#ifdef DEBUG_MODE
                    Log.color(COLOR_CYAN, LOG_LV1, "[O] %s() bd=%d, ch=%d twice moved=%d\n", __FUNCTION__, ptrItem->BoardNo, ptrItem->ChannelNo, move_board);
#endif
                    aList.push_back((int)nLoop);   // push hCallleg
//                    b_twice_moved = true;
                    n_twice_count ++;
                }
            }
        }
    }
    pthread_mutex_unlock(&hash_mutex);
    
    
    if(n_twice_count == 0) { return(0); }
    
    if(g_nHA_State == HA_Active)
    {
        // ACK 수신전에 절체된 채널은 이중화를 하지 않기 때문에 삭제해야 한다.
        for(itr_list = aList.begin(); itr_list != aList.end(); ++itr_list)
        {
            nChannelNo = (int)*itr_list;
            
#ifdef SES_ACTSTD_MODE /* 20241126 kdh add */
			SendChDeallocRQ_SCM(MY_SIDE, nChannelNo);
#else
            SendChDeallocRQ_SCM(move_board, nChannelNo);        // SCM에 통보 (down board가 아닌 original borard 번호로....)
#endif /* SES_ACTSTD_MODE */
//            HASH_DeletehaInfo  (down_board,  nChannelNo);       // SPY Hash 삭제 - SCM에서 ChDeallocRQ를 받으면 채널 해제 요청을 한다.
        }
    }
    
#ifdef DEBUG_MODE
    Log.color(COLOR_CYAN, LOG_INF, "%s() down bd=%d twice count=%d(SendDeAllocRQ_SCM) END....\n", __FUNCTION__, down_board, n_twice_count);
#endif
    return(n_twice_count);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : ha_update_ack
 * CLASS-NAME     : SPY_HASH_MAP
 * PARAMETER    IN: strCall_ID  - Call-Id 문자열값 (Key)
 *              IN: bReceiveAck - Update 할 ACK 수신 정보
 * RET. VALUE     : BOOL
 * DESCRIPTION    : Hash Map에서 Call-ID에 해당하는 정보를 Update 하는 함수
 * REMARKS        : HA Sync는 별도로 수행햐야 함
 **end*******************************************************/
bool SPY_HASH_MAP::ha_update_ack(char* strCall_ID, bool bReceiveAck)
{
    string  strKey = strCall_ID;
    map<string, size_t>::iterator itr;
    HASH_ITEM   *ptrItem;
    
    pthread_mutex_lock(&hash_mutex);
    {
        itr = m_CallID_Map.find(strKey);
        
        if (itr != m_CallID_Map.end())
        {
            ptrItem = (HASH_ITEM *)(itr->second);
            ptrItem->bIsReceiveACK = bReceiveAck;

            pthread_mutex_unlock(&hash_mutex);
            
            return(true);
        }
    }
    pthread_mutex_unlock(&hash_mutex);
    return(false);
}
#endif /* SES_HA_MODE */
