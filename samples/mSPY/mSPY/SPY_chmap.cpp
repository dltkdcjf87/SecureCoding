//
//  SPY_chmap.cpp
//  nsmSPY
//
//  Created by SMCHO on 2016. 6. 14..
//  Copyright  2016년 SMCHO. All rights reserved.
//

#include "SPY_def.h"
#include "SPY_xref.h"
#include "SPY_chmap.h"


extern  SPY_CR      g_CR;
extern  NSM_LOG     Log;
extern SCM_CH_MAP      g_ScmMap;

pthread_mutex_t rtc_mutex;
size_t          RTC_CLOCK;

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SCM_Audit_thread
 * CLASS-NAME     :
 * PARAMETER    IN: arg1 - not used
 * RET. VALUE     : -
 * DESCRIPTION    : T/O을 check하는 Thread를 실행하는 함수
 * REMARKS        : endless function
 **end*******************************************************/
void *SCM_Audit_thread(void *arg)
{
    struct  timespec    now;
    
    pthread_mutex_init(&rtc_mutex, NULL);
#ifndef __APPLE__
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
#endif
    RTC_CLOCK = now.tv_sec;
    
    while(1)
    {
        usleep(1 * 1000 * 1000);

        pthread_mutex_lock(&rtc_mutex);
        {
#ifndef __APPLE__
            clock_gettime(CLOCK_MONOTONIC_RAW, &now);
#endif
            RTC_CLOCK = now.tv_sec;
        }
        pthread_mutex_unlock(&rtc_mutex);
        
        g_ScmMap.audit_pCR(C_SCM_MAP_AUDIT_TIME);
        g_ScmMap.audit_CallID(C_SCM_MAP_AUDIT_TIME);
    }

    return(NULL);
}


#pragma mark -
#pragma mark SCM_CH_MAP member 함수


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SCM_CH_MAP
 * CLASS-NAME     : SCM_CH_MAP
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : constructor
 * REMARKS        :
 **end*******************************************************/
SCM_CH_MAP::SCM_CH_MAP()
{
    n_inseert      = 0;
    n_erase        = 0;
    n_audit_CallID = 0;
    n_audit_CR     = 0;
    
    pthread_mutex_init(&lock_mutex, NULL);

    pthread_create(&m_tid, 0, SCM_Audit_thread, (void *)this);
    pthread_detach(m_tid);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : ~SCM_CH_MAP
 * CLASS-NAME     : SCM_CH_MAP
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : destructor
 * REMARKS        :
 **end*******************************************************/
SCM_CH_MAP::~SCM_CH_MAP()
{
    pthread_cancel(m_tid);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : clear
 * CLASS-NAME     : SCM_CH_MAP
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    :
 * REMARKS        :
 **end*******************************************************/
void SCM_CH_MAP::clear()
{
    pthread_mutex_lock(&lock_mutex);
    {
        m_map_CallID.clear();
        m_map_pCR.clear();
    }
    pthread_mutex_unlock(&lock_mutex);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : insert
 * CLASS-NAME     : SCM_CH_MAP
 * PARAMETER    IN: pCR - Call Register 구조체 포인터
 * RET. VALUE     : -
 * DESCRIPTION    :
 * REMARKS        :
 **end*******************************************************/
bool SCM_CH_MAP::insert(CR_DATA *pCR)
{
    string  strCall_ID = pCR->info.strCall_ID;
    size_t  now = RTC_CLOCK;
    
    pthread_mutex_lock(&lock_mutex);
    {
        n_inseert ++;
        m_map_CallID.insert(make_pair(strCall_ID, now));
        m_map_pCR.insert(make_pair(pCR, now));
    }
    pthread_mutex_unlock(&lock_mutex);
    
    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : erase
 * CLASS-NAME     : SCM_CH_MAP
 * PARAMETER    IN: pCR - Call Register 구조체 포인터
 * RET. VALUE     : -
 * DESCRIPTION    :
 * REMARKS        :
 **end*******************************************************/
bool SCM_CH_MAP::erase(CR_DATA *pCR)
{
    string  strCall_ID;//  = pCR->info.strCall_ID;
    
    pthread_mutex_lock(&lock_mutex);
    {
        n_erase ++;
        m_map_pCR.erase(pCR);           // pCR 삭제가 실패하면, Call-ID를 삭제하지 않게 수정... 혹 segmentation fault 발생 가능성있음
        
        strCall_ID = pCR->info.strCall_ID;
        m_map_CallID.erase(strCall_ID);
    }
    pthread_mutex_unlock(&lock_mutex);
    
    return(true);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : find
 * CLASS-NAME     : SCM_CH_MAP
 * PARAMETER    IN: strKey - 검색할 key
 * RET. VALUE     : -
 * DESCRIPTION    :
 * REMARKS        :
 **end*******************************************************/
bool SCM_CH_MAP::find(const char *strKey)
{
    string  strCall_ID = strKey;
    map<string, size_t>::iterator  itr;
    
    pthread_mutex_lock(&lock_mutex);
    {
        if((itr = m_map_CallID.find(strCall_ID)) != m_map_CallID.end())
        {
            pthread_mutex_unlock(&lock_mutex);
            return(true);
        }
    }
    pthread_mutex_unlock(&lock_mutex);
    
    return(false);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : audit_pCR
 * CLASS-NAME     : SCM_CH_MAP
 * PARAMETER    IN: nSEC - nSEC(5)초 이상 지난 map을 삭제
 * RET. VALUE     : -
 * DESCRIPTION    :
 * REMARKS        :
 **end*******************************************************/
bool SCM_CH_MAP::audit_pCR(int nSEC)
{
    list<CR_DATA *>     list_pCR;
    map<CR_DATA *, size_t>::iterator  itr;
    list<CR_DATA *>::iterator aPos;
    CR_DATA     *pCR;
    string      strCall_ID;
    size_t      nCount;
    
    if(nSEC == 0) { return(true); }     // if 0 then stop_audit
    
    pthread_mutex_lock(&lock_mutex);
    {
        for(itr = this->m_map_pCR.begin(); itr != this->m_map_pCR.end(); itr ++)
        {
            if((RTC_CLOCK - itr->second) > nSEC)
            {
                list_pCR.push_back((CR_DATA *)itr->first);      // 모든 Map 을 list에 저장
            }
        }
    }
    pthread_mutex_unlock(&lock_mutex);
    
    
    if((nCount = list_pCR.size()) == 0) { return(true); }
    
    n_audit_CR += nCount;
    Log.color(COLOR_MAGENTA, LOG_INF, "[SCM] Channel Audit CR Count=%d\n", nCount);
    
    for(aPos = list_pCR.begin(); aPos != list_pCR.end(); ++aPos)
    {
        pCR = *aPos;
        pthread_mutex_lock(&lock_mutex);
        {
            m_map_pCR.erase(pCR);
        }
        pthread_mutex_unlock(&lock_mutex);
        
        g_CR.free(pCR);
    }
    
    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : audit_CallID
 * CLASS-NAME     : SCM_CH_MAP
 * PARAMETER    IN: nSEC - nSEC(5)초 이상 지난 map을 삭제
 * RET. VALUE     : -
 * DESCRIPTION    :
 * REMARKS        :
 **end*******************************************************/
bool SCM_CH_MAP::audit_CallID(int nSEC)
{
    list<string>     list_pCallID;
    map<string, size_t>::iterator  itr;
    list<string>::iterator aPos;
    string      key;
    size_t      nCount;
    
    if(nSEC == 0) { return(true); }     // if 0 then stop_audit
    
    pthread_mutex_lock(&lock_mutex);
    {
        for(itr = this->m_map_CallID.begin(); itr != this->m_map_CallID.end(); itr ++)
        {
            if((RTC_CLOCK - itr->second) > nSEC)
            {
                list_pCallID.push_back(itr->first);      // 모든 Map 을 list에 저장
            }
        }
    }
    pthread_mutex_unlock(&lock_mutex);
    
    
    if((nCount = list_pCallID.size()) == 0) { return(true); }
    
    n_audit_CallID += nCount;
    Log.color(COLOR_MAGENTA, LOG_INF, "[SCM] Channel Audit Call-ID Count=%d\n", nCount);
    
    for(aPos = list_pCallID.begin(); aPos != list_pCallID.end(); ++aPos)
    {
        key = *aPos;
        pthread_mutex_lock(&lock_mutex);
        {
            m_map_CallID.erase(key);
        }
        pthread_mutex_unlock(&lock_mutex);
    }
    
    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : dump
 * CLASS-NAME     : SCM_CH_MAP
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : MAP 내용을 config file로 dump
 * REMARKS        :
 **end*******************************************************/
void SCM_CH_MAP::dump(void)
{
    map<CR_DATA *, size_t>::iterator    itr;
    map<string, size_t>::iterator       itr2;
    string      strCall_ID;
    int         i;
    
    Log.printf(LOG_INF, "================================================================\n");
    Log.printf(LOG_INF, "[SCM_CH_MAP] START DUMP - pCR     size=%zd  RTC=%zd\n", m_map_pCR.size(), RTC_CLOCK);
    
    pthread_mutex_lock(&lock_mutex);
    {
        i = 0;
        
        for(itr = this->m_map_pCR.begin(); itr != this->m_map_pCR.end(); itr ++)
        {
            Log.printf(LOG_INF, "[SCM_CH_MAP] [    pCR] [%5d] pCR=%p, time=%zd(%d)\n", i++, itr->first, itr->second, RTC_CLOCK-itr->second);
        }
    }
    pthread_mutex_unlock(&lock_mutex);
    
    Log.printf(LOG_INF, "[SCM_CH_MAP] START DUMP - Call-ID size=%zd  RTC=%zd\n", m_map_CallID.size(), RTC_CLOCK);
    
    pthread_mutex_lock(&lock_mutex);
    {
        i = 0;
        
        for(itr2 = this->m_map_CallID.begin(); itr2 != this->m_map_CallID.end(); itr2 ++)
        {
            strCall_ID = itr2->first;
            
            Log.printf(LOG_INF, "[SCM_CH_MAP] [Call-ID] [%5d] Call-ID=%s, time=%zd(%d)\n", i++, strCall_ID.c_str(), itr2->second, RTC_CLOCK-itr2->second);

        }
    }
    pthread_mutex_unlock(&lock_mutex);
    Log.printf(LOG_INF, "================================================================\n");
}




