//
//  libsmcfg.h
//  libsmcfg
//
//  Created by SMCHO on 2014. 6. 18..
//  Copyright (c) 2014년 SMCHO. All rights reserved.
//

/*
 * config 파일 구조 (SECTION/NAME)
 * 같은 SECTION에 같은 NAME이 있으면 안됨
 * KEY = SECTION.NAME
 
 [SECTION1]
 NAME1 = VALUE     # COMMENT
 NAME2 = VALUE     # COMMENT
 ..
 
 [SECTION2]
 NAME1 = VALUE     # COMMENT
 NAME3 = VALUE     # COMMENT
 ..
 */

#ifndef __LIB_NSMCFG_H__
#define __LIB_NSMCFG_H__


#include "libnsmcom.h"       // dependency - libsmcom.a


#define MAX_CFG_LINE            512

#ifndef MAX_CFG_NAME_LEN
#   define MAX_CFG_NAME_LEN     256
#endif

typedef struct
{
    bool    b_changed;      // 해당라인이 변경되었는지 여부
    bool    b_data_row;     // 해당 Row에 Key, Value가 있는지 여부
    char    *strRow;        // config file에서 읽어들인 Data 그대로
    char    *strKey;        // key = Section.Name
    char    *strName;       // Name = Valuse    # comment
    char    *strValue;      // Name = value      # comment
    char    *strComment;    // Name = value      # comment
} CFG_MEM;


class NSM_CFG
{
private:
    char    m_filename[MAX_CFG_NAME_LEN+1];     // config file name
    char    m_backup_name[MAX_CFG_NAME_LEN+32]; // config 백업 파일 이름

    CFG_MEM m_cfg_memory[MAX_CFG_LINE];
    int     m_nRow;
    
    void    malloc_and_copy(char **strAlloc, const char *strValue);
    void    malloc_and_copy(char **strAlloc, const char *strValue1, const char *strValue2, const char *strValue3);
    bool    is_config_changed(void);    // 최초 메모리에 읽어들인 config값에서 내용이 변경되었는지 여부
    bool    backup_file(void);          // config 파일을 backup 파일로 rename 하는 함수
    bool    restore_file(void);         // backup 파일을 config 파일로 rename 하는 함수
    
public:
    NSM_CFG();
    ~NSM_CFG();
    
    bool    LoadFile(const char *filename);
    bool    SaveFile(void);
    
    // config 메모리에서 값을 가져오는 함수
    bool    GetBool(const char *strSection, const char *strName, bool *bValue);
    bool    GetInt (const char *strSection, const char *strName, int  *nValue);
    bool    GetStr (const char *strSection, const char *strName, char *strValue, int MAX_LEN);
    
    // config 메모리의 값을 변경하는 함수(파일은 변경되지 않고 SaveFile 요청시 변경됨)
    bool    SetBool(const char *strSection, const char *strName, bool bValue);
    bool    SetInt (const char *strSection, const char *strName, int  nValue);
    bool    SetStr (const char *strSection, const char *strName, char *strValue);
    
    // config 메모리에 없는 값을 Insert하는 함수(SaveFile 요청시 파일에 Write)
    // FIXIT - 필요한가? Config파일을 빼먹지말고 자세히 만들면 필요없는데..

#ifdef LIB_DEBUG
    void    PrintMemory(void);      // Config 파일을 읽어들인 메모리를 출력하는 함수
#endif
};

#endif /* defined(__LIB_NSMCFG_H__) */
