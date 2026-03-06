
#ifndef __SPY_SSWLIST_H__
#define __SPY_SSWLIST_H__

#include <iostream>
#include <map>
#include <string.h>

#include "libsmcom.h"

using namespace std;

/* Class Header
 **pdh********************************************************
 * CLASS-NAME     : SSWLIST_TBL
 * HISTORY        : 2014/04 - 최초 작성
 * DESCRIPTION    : sswlist.cfg 파일을 읽어서 처리하고 관리하는 Class
 *                :
 * REMARKS        :
 **end*******************************************************/
class SSWLIST_TBL
{
private:
    pthread_mutex_t     lock_mutex;             // mutex for memory access
    pthread_mutex_t     file_mutex;             // mutex for sswlist.cfg file access

    char                m_filename[256];
    map<string, bool>   m_sswlist_map;         //    
    
	
    bool    insert_file(const char *strIP, bool bBlock);    // sswlist.cfg file에 insert
    bool    update_file(const char *strIP, bool bBlock);    // sswlist.cfg file에 update
    bool    erase_file(const char *strIP);                  // sswlist.cfg file에서 delete
    bool    backup_file(void);                              // 파일 작업전 backup하는 함수
    
public:
    SSWLIST_TBL(void);
	~SSWLIST_TBL(void);
    
	void    clear(void);                    // map을 초기화 하는 함수
	int     size(void);                     // map에 등록된 SSW 개수를 구하는 함수
	bool    read(const char *fname);        // sswlist.cfg 파일을 읽어서 map에 저장하는 함수
    bool    exist(const char *strIP);       // 해당 IP의 SSW가 등록되어 있는지 여부를 확인하는 함수
    bool    insert(const char *strIP, bool bBlock);     // map에 SSW를 등록
    bool    update(const char *strIP, bool bBlock);     // map에 SSW BLOCK여부를 UPDATE
    bool    select(const char *strIP, bool *bBlock);    // map에 SSW BLOCK여부를 SELECT
    bool    erase(const char *strIP);                   // map에서 해당 SSW 삭제
};

#endif /* defined(__SPY_SSWLIST_H__) */
