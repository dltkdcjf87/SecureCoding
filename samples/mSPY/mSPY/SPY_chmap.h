//
//  SPY_chmap.hpp
//  nsmSPY
//
//  Created by SMCHO on 2016. 6. 14..
//  Copyright  2016년 SMCHO. All rights reserved.
//


#ifndef __SPY_CHMAP_H__
#define __SPY_CHMAP_H__

#include <iostream>
#include <map>
#include <list>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "btxbus3.h"
#include "libnsmcom.h"
#include "libnsmlog.h"
#include "SPY_cr.h"

using namespace std;

/* Class Header
 **pdh********************************************************
 * CLASS-NAME     : SCM_CH_MAP
 * HISTORY        : 2016/06 - 최초 작성
 * DESCRIPTION    : SCM_CH_MAP Class
 *                :
 * REMARKS        :
 **end*******************************************************/
class SCM_CH_MAP
{
    friend void *SCM_Audit_thread(void *arg);

private:
    pthread_mutex_t     lock_mutex;             // mutex for memory access
    pthread_t           m_tid;

public:
    SCM_CH_MAP(void);
    ~SCM_CH_MAP(void);
    
    void    clear(void);                // map 을 초기화 하는 함수
    bool    insert(CR_DATA *pCR);       // map 에 정보를 등록
    bool    find(const char *strCall_ID);     // map 에 등록되어 있는지 확인하는 함수
    bool    erase(CR_DATA *pCR);        // map 에서 정보를 삭제
//    bool    audit(int nSEC);            // mSEC 이상 지난 map을 삭제하는 함수
    bool    audit_pCR(int nSEC);            // mSEC 이상 지난 map을 삭제하는 함수
    bool    audit_CallID(int nSEC);            // mSEC 이상 지난 map을 삭제하는 함수
    void    dump();

    map<string,    size_t>  m_map_CallID;       // Key 가 Call-Id인 Map
    map<CR_DATA *, size_t>  m_map_pCR;
    
    size_t  n_inseert;
    size_t  n_erase;
    size_t  n_audit_CR;
    size_t  n_audit_CallID;
};

#endif /* __SPY_CHMAP_H__ */


