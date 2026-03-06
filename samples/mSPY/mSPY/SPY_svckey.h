//
//  SPY_svckey.h
//  smSPY
//
//  Created by SMCHO on 2014. 4. 4..
//  Copyright (c) 2014년 SMCHO. All rights reserved.
//

#ifndef __SPY_SVCKEY_H__
#define __SPY_SVCKEY_H__

#include <iostream>
#include <map>
#include <string.h>

#include "libnsmcom.h"

using namespace std;

#define MAX_SVCKEY_LIST         200
typedef struct
{
    char strSvcName[64];
    char strSvcKey[64];
} SVCKEY_LIST;

/* Class Header
 **pdh********************************************************
 * CLASS-NAME     : SVCKEY_TBL
 * HISTORY        : 2014/04 - 최초 작성
 * DESCRIPTION    : service_key.cfg 파일을 읽어서 처리하고 관리하는 Class
 *                :
 * REMARKS        :
 **end*******************************************************/
class SVCKEY_TBL
{
private:
    pthread_mutex_t     lock_mutex;             // mutex for memory access
    pthread_mutex_t     file_mutex;             // mutex for service_key.cfg file access
    
    char            m_filename[256];
    SVCKEY_LIST     m_SvcKeyList[MAX_SVCKEY_LIST];
    int             m_nSvcKeyListCount;
	
//    bool    insert_file(const char *strIP, bool bBlock);    // service_key.cfg file에 insert
//    bool    update_file(const char *strIP, bool bBlock);    // service_key.cfg file에 update
//    bool    erase_file(const char *strIP);                  // service_key.cfg file에서 delete
//    bool    backup_file(void);                              // 파일 작업전 backup하는 함수
    void    print_log(void);
    
public:
    SVCKEY_TBL(void);
	~SVCKEY_TBL(void);
    
	void    clear(void);                    // map을 초기화 하는 함수
	int     size(void);                     // map에 등록된 SSW 개수를 구하는 함수
	bool    read(const char *fname);        // service_key.cfg 파일을 읽어서 map에 저장하는 함수
    bool    get_service_key(const char *strRURI, char *strServiceKey); // RURI를 분석하여 ServiceKey를 구하는 함수
    bool is_service_key(const char *strServiceKey);     // 입력 ServiceKey가 등록되어 있는지를 확인하는 함수

};

#endif /* defined(__SPY_SVCKEY_H__) */
