/* File Header
 *fsh**************************************************************
 ******************************************************************
 **
 **  FILE : LIB_nsmcfg.cpp
 **
 ******************************************************************
 ******************************************************************
 BLOCK          : LIB
 SUBSYSTEM      : LIB
 SOR-NAME       :
 VERSION        : V4.X
 DATE           : 2014/06/
 AUTHOR         : SEUNG-MO, CHO
 HISTORY        :
 PROCESS(TASK)  :
 PROCEDURES     :
 DESCRIPTION    : Config 파일을 처리하는 Class
 REMARKS        :
 *end*************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/stat.h>

#include "libnsmcfg.h"


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : NSM_CFG
 * CLASS-NAME     : NSM_CFG
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : constructor
 * REMARKS        :
 **end*******************************************************/
NSM_CFG::NSM_CFG()
{
    bzero(&m_cfg_memory, sizeof(CFG_MEM)*MAX_CFG_LINE);
    bzero(m_backup_name, sizeof(m_backup_name));
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : ~NSM_CFG
 * CLASS-NAME     : NSM_CFG
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : destructor
 * REMARKS        :
 **end*******************************************************/
NSM_CFG::~NSM_CFG()
{
    for(int i = 0; i < m_nRow; i ++)
    {
        free(m_cfg_memory[i].strRow);
        
        if(m_cfg_memory[i].b_data_row == true)
        {
            free(m_cfg_memory[i].strKey);
            free(m_cfg_memory[i].strName);
            free(m_cfg_memory[i].strValue);
        }
    }
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : malloc_and_copy
 * CLASS-NAME     : NSM_CFG
 * PARAMETER   OUT: strAlloc - Memory Allocation할 변수
 *              IN: strValue - strAlloc에 copy할 내용
 * RET. VALUE     : BOOL
 * DESCRIPTION    : strAlloc에 strValue의 크기만큼 메모리를 할당하고 복사하는 함수
 * REMARKS        :
 **end*******************************************************/
void NSM_CFG::malloc_and_copy(char **strAlloc, const char *strValue)
{
    int size = (int)strlen(strValue) + 1;
    
    *strAlloc = (char *)malloc(size);
    
    if(strAlloc != NULL) { strcpy(*strAlloc, strValue); }
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : malloc_and_copy
 * CLASS-NAME     : NSM_CFG
 * PARAMETER   OUT: strAlloc  - Memory Allocation할 변수
 *              IN: strValue1 - strAlloc에 copy할 내용
 *              IN: strValue2 - strAlloc에 copy할 내용
 *              IN: strValue3 - strAlloc에 copy할 내용
 * RET. VALUE     : BOOL
 * DESCRIPTION    : strAlloc에 strValue(1+2+3)의 크기만큼 메모리를 할당하고 복사하는 함수
 * REMARKS        :
 **end*******************************************************/
void NSM_CFG::malloc_and_copy(char **strAlloc, const char *strValue1, const char *strValue2, const char *strValue3)
{
    int size = (int)strlen(strValue1) + (int)strlen(strValue2) + (int)strlen(strValue3) + 1;
    
    *strAlloc = (char *)malloc(size);
    
    if(strAlloc != NULL)
    {
        strcpy(*strAlloc, strValue1);
        strcat(*strAlloc, strValue2);
        strcat(*strAlloc, strValue3);
    }
}


#pragma mark -
#pragma mark Config 파일을 읽고/쓰는 함수 들

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LoadFile
 * CLASS-NAME     : NSM_CFG
 * PARAMETER    IN: filename - config file name(full path)
 * RET. VALUE     : BOOL
 * DESCRIPTION    : config 파일을 읽어서 내부 메모리에 저장하는 함수
 * REMARKS        :
 **end*******************************************************/
bool NSM_CFG::LoadFile(const char *filename)
{
    FILE    *fp;
    char	buf[1024];
    char	strName[32], strValue[1024];
    int     index;
    char    strSection[128];

    bzero(strSection, sizeof(strSection));
    bzero(m_filename, sizeof(m_filename));
    
    strncpy(m_filename, filename, MAX_CFG_NAME_LEN);

    if((fp = fopen(m_filename, "r")) == NULL) { return(false); }
    
    for(index = 0; index < MAX_CFG_LINE; index ++)
    {
        if(fgets(buf, 1023, fp) == NULL) { break; }     // EOL
        
        malloc_and_copy(&m_cfg_memory[index].strRow, buf);
        m_cfg_memory[index].strComment = strchr(m_cfg_memory[index].strRow, '#');   // COMMENT 위치 저장
        
        LIB_delete_comment(buf, '#');
		LIB_delete_white_space(buf);

		// 20240909 kdh - add :: buf under-flow
		if(strlen(buf) <= 0) continue;

		if(buf[strlen(buf)-1] == ';')  { buf[strlen(buf)-1] = '\0'; }   // delete end of line mark(';')
   
		// 20240909 kdh - add :: buf under-flow
		if(strlen(buf) <= 0) continue;

		if((*buf == '[') && (buf[strlen(buf)-1] == ']'))
		{
            buf[strlen(buf)-1] = '\0';
            strcpy(strSection, buf+1);
            continue;
        }
        
        if(LIB_split_string_into_2(buf, '=', strName, strValue) == true)
        {
            m_cfg_memory[index].b_data_row = true;
            
            malloc_and_copy(&m_cfg_memory[index].strKey,   strSection, ".", strName);
            malloc_and_copy(&m_cfg_memory[index].strName,  strName);
            malloc_and_copy(&m_cfg_memory[index].strValue, strValue);
        }
    }
    
    m_nRow = index;         // Config Row(Line) 수

	fclose(fp);
    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : is_config_changed
 * CLASS-NAME     : NSM_CFG
 * PARAMETER      : -
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 최초 메모리에 읽어들인 config값에서 내용이 변경되었는지 여부를 확인 함수
 * REMARKS        :
 **end*******************************************************/
bool NSM_CFG::is_config_changed(void)
{
    for(int i = 0; i < m_nRow; i ++)
    {
        if(m_cfg_memory[i].b_changed == true) { return(true); }
    }
    
    return(false);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : backup_file
 * CLASS-NAME     : NSM_CFG
 * PARAMETER      : -
 * RET. VALUE     : BOOL
 * DESCRIPTION    : config 파일을 backup 파일로 rename 하는 함수
 * REMARKS        : backup_name = config_filename.time_t
 **end*******************************************************/
bool NSM_CFG::backup_file(void)
{
    struct  timeval t_now;
    
    gettimeofday(&t_now, NULL);
    snprintf(m_backup_name, sizeof(m_backup_name), "%s.%ld", m_filename, t_now.tv_sec);
    
    if(rename(m_filename, m_backup_name) == 0) { return (true); }

    return(false);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : restore_file
 * CLASS-NAME     : NSM_CFG
 * PARAMETER      : -
 * RET. VALUE     : BOOL
 * DESCRIPTION    : backup 파일을 config 파일로 rename 하는 함수
 * REMARKS        :
 **end*******************************************************/
bool NSM_CFG::restore_file(void)
{
    if(m_backup_name[0] == '\0') { return(false); }
    
    if(rename(m_backup_name, m_filename) == 0)
    {
        m_backup_name[0] = '\0';    // 백업파일 없음을 표시
        return(0);
    }
    
#ifdef LIB_DEBUG
    printf("restore_file() fail : %s->%s[%d, %s]\n", m_filename, m_backup_name, errno, strerror(errno));
#endif
    return(errno);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SaveFile
 * CLASS-NAME     : NSM_CFG
 * PARAMETER      : -
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 메모리의 config 정보를 파일에 저장하는 함수
 * REMARKS        :
 **end*******************************************************/
bool NSM_CFG::SaveFile(void)
{
    FILE    *fp;
    char	buf[1024];
    struct stat cfg_stat;

#if 0
    if(is_config_changed() == false) { return(true); }  // no change
#endif
    backup_file();  // 백업 성공 여부와 관계 없이 저장 함
    
    if((fp = fopen(m_filename, "w")) == NULL) { return(false); }
    
    for(int i = 0; i < m_nRow; i ++)
    {
        if(m_cfg_memory[i].b_changed == false)
        {
            // 변경이 없는 라인은 그대로 write
            fputs(m_cfg_memory[i].strRow, fp);
        }
        else
        {
            if(m_cfg_memory[i].strComment == NULL)
            {
                snprintf(buf, sizeof(buf), "%-15s = %-21s\n", m_cfg_memory[i].strName, m_cfg_memory[i].strValue);
            }
            else
            {
                snprintf(buf, sizeof(buf), "%-15s = %-21s %s", m_cfg_memory[i].strName, m_cfg_memory[i].strValue, m_cfg_memory[i].strComment);
            }
            fputs(buf, fp);
        }
    }
    
    fclose(fp);

    if(stat(m_filename, &cfg_stat) != 0)
    {
        restore_file();
		return(false);
    }
    
    if(cfg_stat.st_size == 0)   // file size == 0
    {
        restore_file();
		return(false);
    }

    return(true);
}

#pragma mark -
#pragma mark 메모리에 읽어들인 Config 값을 얻어오는 함수 들

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : GetBool
 * CLASS-NAME     : NSM_CFG
 * PARAMETER    IN: strSection - 저장된 데이터의 Section
 *              IN: strName    - 저장된 데이터의 이름
 *             OUT: bValue     - strKey에 해당하는 Value
 * RET. VALUE     : BOOL
 * DESCRIPTION    : strKey에 해당하는 값을 bool 형태로 구하는 함수
 * REMARKS        : section.name=value 형태
 **end*******************************************************/
bool NSM_CFG::GetBool(const char *strSection, const char *strName, bool *bValue)
{
    char    strKey[128];
    
    bzero(strKey, sizeof(strKey));
    snprintf(strKey, 127, "%s.%s", strSection, strName);
    
    for(int i = 0; i < m_nRow; i ++)
    {
        if(m_cfg_memory[i].b_data_row != true) { continue; }
           
        if(strcasecmp(m_cfg_memory[i].strKey, strKey) == 0)
        {
            if((strcasecmp(m_cfg_memory[i].strValue, "True") == 0)
            || (strcasecmp(m_cfg_memory[i].strValue, "Yes")  == 0))
            {
                *bValue = true;
            }
            else
            {
                *bValue = false;
            }
            
            return(true);
        }
    }
    
    return(false);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : GetInt
 * CLASS-NAME     : NSM_CFG
 * PARAMETER    IN: strSection - 저장된 데이터의 Section
 *              IN: strName    - 저장된 데이터의 이름
 *             OUT: nValue     - strKey에 해당하는 Value
 * RET. VALUE     : BOOL
 * DESCRIPTION    : strKey에 해당하는 값을 int 형태로 구하는 함수
 * REMARKS        : section.name=value 형태
 **end*******************************************************/
bool NSM_CFG::GetInt(const char *strSection, const char *strName, int *nValue)
{
    char    strKey[128];
    
    bzero(strKey, sizeof(strKey));
    snprintf(strKey, 127, "%s.%s", strSection, strName);
    
    for(int i = 0; i < m_nRow; i ++)
    {
        if(m_cfg_memory[i].b_data_row != true) { continue; }
        
        if(strcasecmp(m_cfg_memory[i].strKey, strKey) == 0)
        {
            *nValue = atoi(m_cfg_memory[i].strValue);
            return(true);
        }
    }
    
    return(false);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : GetStr
 * CLASS-NAME     : NSM_CFG
 * PARAMETER    IN: strSection - 저장된 데이터의 Section
 *              IN: strName    - 저장된 데이터의 이름
 *             OUT: strValue   - strKey에 해당하는 Value
 *              IN: MAX_LEN    - strValue의 최대 길이
 * RET. VALUE     : BOOL
 * DESCRIPTION    : strKey에 해당하는 값을 string 형태로 구하는 함수
 * REMARKS        : section.name=value 형태
 **end*******************************************************/
bool NSM_CFG::GetStr(const char *strSection, const char *strName, char *strValue, int MAX_LEN)
{
    char    strKey[128];
    
    bzero(strKey, sizeof(strKey));
    snprintf(strKey, 127, "%s.%s", strSection, strName);
    
    for(int i = 0; i < m_nRow; i ++)
    {
        if(m_cfg_memory[i].b_data_row != true) { continue; }
        
        if(strcasecmp(m_cfg_memory[i].strKey, strKey) == 0)
        {
            if(strlen(m_cfg_memory[i].strValue) >= MAX_LEN) { return(false); }
            
            strcpy(strValue, m_cfg_memory[i].strValue);
            return(true);
        }
    }
    
    return(false);
}


#pragma mark -
#pragma mark Config 메모리값을 변경하는 함수 들

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SetBool
 * CLASS-NAME     : NSM_CFG
 * PARAMETER    IN: strSection - 저장된 데이터의 Section
 *              IN: strName    - 저장된 데이터의 이름
 *             OUT: bValue     - 변경할 Value의 값
 * RET. VALUE     : BOOL
 * DESCRIPTION    : Config 메모리값(bool)을 변경하는 함수
 * REMARKS        : section.name=value 형태
 **end*******************************************************/
bool NSM_CFG::SetBool(const char *strSection, const char *strName, bool bValue)
{
    char    strKey[128];
    
    bzero(strKey, sizeof(strKey));
    snprintf(strKey, 127, "%s.%s", strSection, strName);
    
    for(int i = 0; i < m_nRow; i ++)
    {
        if(m_cfg_memory[i].b_data_row != true) { continue; }
        
        if(strcasecmp(m_cfg_memory[i].strKey, strKey) == 0)
        {
            if((strcasecmp(m_cfg_memory[i].strValue, "True") == 0)
            || (strcasecmp(m_cfg_memory[i].strValue, "Yes")  == 0))
            {
                if(bValue == false)     // 변경이 있는 경우만 update
                {
                    free(m_cfg_memory[i].strValue);         // 기존 값은 삭제
                    malloc_and_copy(&m_cfg_memory[i].strValue, "False");
                    m_cfg_memory[i].b_changed = true;       // mark change
                }
            }
            else
            {
                if(bValue == true)      // 변경이 있는 경우만 update
                {
                    free(m_cfg_memory[i].strValue);         // 기존 값은 삭제
                    malloc_and_copy(&m_cfg_memory[i].strValue, "True");
                    m_cfg_memory[i].b_changed = true;       // mark change
                }
            }
            return(true);
        }
    }
    
    return(false);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SetInt
 * CLASS-NAME     : NSM_CFG
 * PARAMETER    IN: strSection - 저장된 데이터의 Section
 *              IN: strName    - 저장된 데이터의 이름
 *             OUT: bValue     - 변경할 Value의 값
 * RET. VALUE     : BOOL
 * DESCRIPTION    : Config 메모리값(int)을 변경하는 함수
 * REMARKS        : section.name=value 형태
 **end*******************************************************/
bool NSM_CFG::SetInt(const char *strSection, const char *strName, int nValue)
{
    char    strKey[128];
    char    strValue[32];
    
    bzero(strKey, sizeof(strKey));
    snprintf(strKey, 127, "%s.%s", strSection, strName);
    
    for(int i = 0; i < m_nRow; i ++)
    {
        if(m_cfg_memory[i].b_data_row != true) { continue; }
        
        if(strcasecmp(m_cfg_memory[i].strKey, strKey) == 0)
        {
            if(nValue != atoi(m_cfg_memory[i].strValue))    // 변경이 있는 경우만 update
            {
                free(m_cfg_memory[i].strValue);         // 기존 값은 삭제
                snprintf(strValue, sizeof(strValue), "%d", nValue);        // int -> str
                malloc_and_copy(&m_cfg_memory[i].strValue, strValue);
                m_cfg_memory[i].b_changed = true;       // mark change
            }
            return(true);
        }
    }
    
    return(false);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SetStr
 * CLASS-NAME     : NSM_CFG
 * PARAMETER    IN: strSection - 저장된 데이터의 Section
 *              IN: strName    - 저장된 데이터의 이름
 *             OUT: bValue     - 변경할 Value의 값
 * RET. VALUE     : BOOL
 * DESCRIPTION    : Config 메모리값(string)을 변경하는 함수
 * REMARKS        : section.name=value 형태
 **end*******************************************************/
bool NSM_CFG::SetStr(const char *strSection, const char *strName, char *strValue)
{
    char    strKey[128];
    
    bzero(strKey, sizeof(strKey));
    snprintf(strKey, 127, "%s.%s", strSection, strName);
    
    for(int i = 0; i < m_nRow; i ++)
    {
        if(m_cfg_memory[i].b_data_row != true) { continue; }
        
        if(strcasecmp(m_cfg_memory[i].strKey, strKey) == 0)
        {
            if(strcmp(m_cfg_memory[i].strValue, strValue) != 0) // 변경이 있는 경우만 update
            {
                free(m_cfg_memory[i].strValue);         // 기존 값은 삭제
                malloc_and_copy(&m_cfg_memory[i].strValue, strValue);
                m_cfg_memory[i].b_changed = true;       // mark change
            }
            return(true);
        }
    }
    
    return(false);
}

#pragma mark -
#pragma mark 디버깅용 함수들

#ifdef LIB_DEBUG
/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : PrintMemory
 * CLASS-NAME     : NSM_CFG
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : Config 파일에서 읽어들인 메모리 내용을 출력 함수
 * REMARKS        : key=value가 없는 라인은 skip
 **end*******************************************************/
void NSM_CFG::PrintMemory(void)
{
    printf("=================================================================================\n");
    printf("   config filename = %s  Line=%d\n", m_filename, m_nRow);
    printf("=================================================================================\n");
    
    for(int i = 0; i < m_nRow; i ++)
    {
        if(m_cfg_memory[i].b_data_row == true)
        {
            printf("[%3d] %s = %s\n", i, m_cfg_memory[i].strKey, m_cfg_memory[i].strValue);
        }
    }
}
#endif

