
#if 0   // SSWLIST는 사용하지 않고 EXTLIST로 통합해서 사용 함

/* File Header
 *fsh**************************************************************
 ******************************************************************
 **
 **  FILE : SPY_sswlist.cpp
 **
 ******************************************************************
 ******************************************************************
 BLOCK          : SPY
 SUBSYSTEM      : CMS
 SOR-NAME       :
 VERSION        : V4.X
 DATE           : 2014/04/
 AUTHOR         : SEUNG-MO, CHO
 HISTORY        :
 PROCESS(TASK)  :
 PROCEDURES     :
 DESCRIPTION    : sswlist.cfg 파일을 읽어서 처리하고 관리하는 Class 함수
 REMARKS        :
 *end*************************************************************/

#include "SPY_sswlist.h"

#define	LOG_INF         5
#define LOG_ERR         4
#define	LOG_LV3         3
#define	LOG_LV2         2
#define	LOG_LV1         1

extern void Log.printf(char nDebugLevel, const char *fmt, ...);

//#pragma mark -
//#pragma mark 생성자/파괴자

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SSWLIST_TBL
 * CLASS-NAME     : SSWLIST_TBL
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : SSWLIST_TBL 클래스 생성자
 * REMARKS        :
 **end*******************************************************/
SSWLIST_TBL::SSWLIST_TBL(void)
{
    pthread_mutex_init(&lock_mutex, NULL);
    pthread_mutex_init(&file_mutex, NULL);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : ~SSWLIST_TBL
 * CLASS-NAME     : SSWLIST_TBL
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : SSWLIST_TBL 클래스 소멸자
 * REMARKS        :
 **end*******************************************************/
SSWLIST_TBL::~SSWLIST_TBL(void)
{
	clear();
}


//#pragma mark -
//#pragma mark sswlist.cfg를 읽어서 MAP에 INSERT하는 함수

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : read
 * CLASS-NAME     : SSWLIST_TBL
 * PARAMETER    IN: fname - sswlist.cfg 파일 이름(full path)
 * RET. VALUE     : BOOL
 * DESCRIPTION    : sswlist.fg 파일을 읽어서 map에 저장하는 함수
 * REMARKS        :
 **end*******************************************************/
bool SSWLIST_TBL::read(const char *fname)
{
    FILE	*fp;
    char    buf[256], strIP[32], strBLOCK[32];
    string  strKey;
    bool    bValue;
    
    if(strlen(fname) >= sizeof(m_filename))
    {
        Log.printf(LOG_ERR, "[SSWLIST_TBL] filename length is too big [%s]\n", fname);
        return(false);
    }
    
    strcpy(m_filename, fname);        // store filename(/.../sswlist.cfg)
    
    if((fp = fopen(m_filename, "r")) == NULL)
	{
		Log.printf(LOG_ERR, "[SSWLIST_TBL] Can't Open %s file\n", m_filename);
		return(false);
	}
    
    pthread_mutex_lock(&lock_mutex);
    {
        while(fgets(buf, 255, fp))
        {
            LIB_delete_comment(buf, '#');       // comment 이후 부분 삭제
            LIB_delete_white_space(buf);        // 공백 제거
            
            if(strlen(buf) == 0) { continue; }                              // skip empty line
            if((*buf == '[') && (buf[strlen(buf)-1] == ']')) { continue; }  // skip section line
            if(buf[strlen(buf)-1] == ';') { buf[strlen(buf)-1] = 0; }       // delete last ';'
            
            // IP=0(NON-BLOCK) or IP=1(BLOCK)
            if(LIB_split_string_into_2(buf, '=', strIP, strBLOCK) == false) { continue; }
            
            // IP와 Block정보를 key와 Value로 map에 저장
            strKey = strIP;
            bValue = atoi(strBLOCK);
            m_sswlist_map.insert(make_pair(strKey, bValue));
        }
    }
    pthread_mutex_unlock(&lock_mutex);
    
    fclose(fp);
	return(true);
}


//#pragma mark -
//#pragma mark MAP에 INSERT/UPDATE/DELETE하는 함수

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : insert
 * CLASS-NAME     : SSWLIST_TBL
 * PARAMETER    IN: strIP  - SSW IP address
 *              IN: bBlock - SSW Block 여부
 * RET. VALUE     : BOOL
 * DESCRIPTION    : SSW를 MAP에 추가하는 함수
 * REMARKS        :
 **end*******************************************************/
bool SSWLIST_TBL::insert(const char *strIP, bool bBlock)
{
    if(exist(strIP) == true) { return(true); }      // 이미 등록됨 OK
    
    string  strKey = strIP;

    pthread_mutex_lock(&lock_mutex);
    {
        m_sswlist_map.insert(make_pair(strKey, bBlock));
    }
    pthread_mutex_unlock(&lock_mutex);
    
    insert_file(strIP, bBlock);             // update sswlist.cfg file
    
	return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : update
 * CLASS-NAME     : SSWLIST_TBL
 * PARAMETER    IN: strIP  - SSW IP address
 *              IN: bBlock - SSW Block 여부
 * RET. VALUE     : BOOL
 * DESCRIPTION    : SSW BLOCK여부를 MAP에 UPDATE 하는 함수
 * REMARKS        :
 **end*******************************************************/
bool SSWLIST_TBL::update(const char *strIP, bool bBlock)
{
    if(exist(strIP) == false) { return(false); }      // 등록되어 있지 않음
    
    map<string, bool>::iterator itr;
    string  strKey = strIP;

    pthread_mutex_lock(&lock_mutex);
    {
        itr = m_sswlist_map.find(strKey);
        
        if(itr != m_sswlist_map.end())
        {
            if(itr->second == bBlock)
            {
                // 이전과 동일 값...
                pthread_mutex_unlock(&lock_mutex);
                return(true);
            }
            
            //  이전과 다른 값... map을 upate하고 file도 update한다.
            itr->second = bBlock;
            pthread_mutex_unlock(&lock_mutex);
            
            update_file(strIP, bBlock);             // update sswlist.cfg file
            return(true);
        }
    }
    pthread_mutex_unlock(&lock_mutex);
	return(false);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : select
 * CLASS-NAME     : SSWLIST_TBL
 * PARAMETER    IN: strIP  - SSW IP address
 *             OUT: bBlock - SSW BLOCK 여부
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 해당하는 IP의 SSW의 등록여부와 BLOCK 여부를 확인하는 함수
 * REMARKS        : mutex_lock을 사용해야 하는지 검토 필요 (속도 이슈)
 **end*******************************************************/
bool SSWLIST_TBL::select(const char *strIP, bool *bBlock)
{
    string  strKey = strIP;
    map<string, bool>::iterator itr;
    
//    pthread_mutex_lock(&lock_mutex);
    {
        itr = m_sswlist_map.find(strKey);
        
        if(itr != m_sswlist_map.end())
        {
            *bBlock = itr->second;
//            pthread_mutex_unlock(&lock_mutex);
            return(true);
        }
    }
//    pthread_mutex_unlock(&lock_mutex);
	return(false);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : erase
 * CLASS-NAME     : SSWLIST_TBL
 * PARAMETER    IN: strIP  - SSW IP address
 * RET. VALUE     : BOOL
 * DESCRIPTION    : SSW 정보를 MAP에서 삭제 하는 함수
 * REMARKS        :
 **end*******************************************************/
bool SSWLIST_TBL::erase(const char *strIP)
{
    string  strKey = strIP;
    
    pthread_mutex_lock(&lock_mutex);
    {
        m_sswlist_map.erase(strKey);
    }
    pthread_mutex_unlock(&lock_mutex);
    
    erase_file(strIP);
    
	return(true);
}


//#pragma mark -
//#pragma mark MAP 관련 기타 함수(초기화, size, exist..)

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : clear
 * CLASS-NAME     : SSWLIST_TBL
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : map에 저장된 모든 Item을 삭제
 * REMARKS        :
 **end*******************************************************/
void SSWLIST_TBL::clear(void)
{
    pthread_mutex_lock(&lock_mutex);
    {
        m_sswlist_map.clear();
    }
    pthread_mutex_unlock(&lock_mutex);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : size
 * CLASS-NAME     : SSWLIST_TBL
 * PARAMETER      : -
 * RET. VALUE     : map에 등록된 SSW List 수
 * DESCRIPTION    : 전체 SSW List 개수를 구하는 함수
 * REMARKS        :
 **end*******************************************************/
int SSWLIST_TBL::size(void)
{
	return(m_sswlist_map.size());
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : exist
 * CLASS-NAME     : SSWLIST_TBL
 * PARAMETER    IN: strIP  - SSW IP address
 * RET. VALUE     : BOOL
 * DESCRIPTION    : MAP에 해당하는 IP의 SSW가 등록되어 있는지 확인하는 함수
 * REMARKS        : mutex_lock을 사용해야 하는지 검토 필요
 **end*******************************************************/
bool SSWLIST_TBL::exist(const char *strIP)
{
    string  strKey = strIP;
    map<string, bool>::iterator itr;
    
//	pthread_mutex_lock(&lock_mutex);
    {
        itr = m_sswlist_map.find(strKey);
        
        if(itr != m_sswlist_map.end())
        {
//            pthread_mutex_unlock(&lock_mutex);
            return(true);
        }
    }
//	pthread_mutex_unlock(&lock_mutex);
	return(false);
}


//#pragma mark -
//#pragma mark sswlist.cfg 파일을 수정하는 함수

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : backup_file
 * CLASS-NAME     : SSWLIST_TBL
 * PARAMETER      :
 * RET. VALUE     : BOOL
 * DESCRIPTION    : sswlist.cfg 파일을 sswli.cfg.time으로 백업하는 함수
 * REMARKS        : time은 현재시간(time_t)
 **end*******************************************************/
bool SSWLIST_TBL::backup_file(void)
{
    FILE	*src, *dest;
    char    backup_name[256];
    char    buf[256];
    
    snprintf(backup_name, sizof(backup_name), "%s.%ld", m_filename, time(NULL));       // BACKUP filename = filename+time
    
    if((src = fopen(m_filename,  "r")) == NULL)
    {
        Log.printf(LOG_ERR, "[SSWLIST_TBL] backup_file() Can't Open src file %s \n", m_filename);
		return(false);
    }
    
    if((dest = fopen(backup_name,  "w")) == NULL)
    {
        Log.printf(LOG_ERR, "[SSWLIST_TBL] backup_file() Can't Open dest file %s \n", backup_name);
        fclose(src);
		return(false);
    }
    
    while(fgets(buf, 255, src))
    {
        fputs(buf, dest);
    }
    
    fclose(src);
    fclose(dest);
    
    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : insert_file
 * CLASS-NAME     : SSWLIST_TBL
 * PARAMETER    IN: strIP  - SSW IP address
 *              IN: bBlock - SSW Block 여부
 * RET. VALUE     : BOOL
 * DESCRIPTION    : SSW를 sswlist.cfg 파일에 추가하는 함수
 * REMARKS        : 파일의 끝에 append
 **end*******************************************************/
bool SSWLIST_TBL::insert_file(const char *strIP, bool bBlock)
{
    FILE	*fp;

    pthread_mutex_lock(&file_mutex);
    {
        backup_file();      // insert전 원래 파일을 백업

        if((fp = fopen(m_filename, "a+")) == NULL)
        {
            pthread_mutex_unlock(&file_mutex);
            Log.printf(LOG_ERR, "[SSWLIST_TBL] insert_file() Can't Open %s file\n", m_filename);
            return(false);
        }
        
        fprintf(fp, "%s  =    %d        # INSERTED by SSWLIST_TBL.insert_file()\n", strIP, bBlock);
        fclose(fp);
    }
    pthread_mutex_unlock(&file_mutex);
  
	return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : update_file
 * CLASS-NAME     : SSWLIST_TBL
 * PARAMETER    IN: strIP  - SSW IP address
 *              IN: bBlock - SSW Block 여부
 * RET. VALUE     : BOOL
 * DESCRIPTION    : SSW를 Block 여부를 sswlist.cfg 파일에 UPDATE하는 함수
 * REMARKS        : 원본을 백업으로 변경하고 백업을 원본이름으로 write..
 **end*******************************************************/
bool SSWLIST_TBL::update_file(const char *strIP, bool bBlock)
{
    FILE	*new_fp, *old_fp;
    char    backup_name[256];
    char    buf[256];
    char    *ptrComment;
    bool    done = false;

    snprintf(backup_name, sizeof(backup_name), "%s.%ld", m_filename, time(NULL));   // BACKUP filename = filename+time

    pthread_mutex_lock(&file_mutex);
    {
        if(rename(m_filename, backup_name) != 0)              // 작업 전 원래 파일을 백업파일로 move
        {
            Log.printf(LOG_ERR, "[SSWLIST_TBL] update_file() Can't backup file %s reason=%s \n", m_filename, strerror(errno));
            pthread_mutex_unlock(&file_mutex);
            return(false);
        }
        
        if((old_fp = fopen(backup_name,  "r")) == NULL)
        {
            Log.printf(LOG_ERR, "[SSWLIST_TBL] update_file() Can't Open backup file %s \n", backup_name);
            pthread_mutex_unlock(&file_mutex);
            return(false);
        }
        
        if((new_fp = fopen(m_filename,  "w")) == NULL)
        {
            Log.printf(LOG_ERR, "[SSWLIST_TBL] update_file() Can't Open file %s \n", m_filename);
            fclose(old_fp);
            pthread_mutex_unlock(&file_mutex);
            return(false);
        }
        
        // backup 파일을 한줄씩 읽어서 sswlist.cfg 파일로 write 또는 update
        while(fgets(buf, 255, old_fp))
        {
            if(done)        // update가 완료되었음
            {
                fputs(buf, new_fp);
            }
            else
            {
                // comment를 제외한 부분에서 strIP에서 비교하기 위해서 comment 시작부분을 NULL로 set
                ptrComment = strchr(buf, '#');
                if(ptrComment != NULL) { *ptrComment = '\0'; }      // set NULL for copy
                
                if(strstr(strIP, buf) != NULL)
                {
                    fprintf(new_fp, "%s  =    %d        # UPDATED by SSWLIST_TBL.insert_file()\n", strIP, bBlock);
                    done = true;        // update 완료
                }
                else
                {
                    if(ptrComment != NULL) { *ptrComment = '#'; }      // restore comment
                    fputs(buf, new_fp);
                }
            }
        }
        
        fclose(old_fp);
        fclose(new_fp);
    }
    pthread_mutex_unlock(&file_mutex);
	return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : erase_file
 * CLASS-NAME     : SSWLIST_TBL
 * PARAMETER    IN: strIP  - SSW IP address
 *              IN: bBlock - SSW Block 여부
 * RET. VALUE     : BOOL
 * DESCRIPTION    : SSW를 sswlist.cfg 파일에서 삭제하는 함수
 * REMARKS        :
 **end*******************************************************/
bool SSWLIST_TBL::erase_file(const char *strIP)
{
    FILE	*new_fp, *old_fp;
    char    backup_name[256];
    char    buf[256];
    char    *ptrComment;
    bool    done = false;
    
    snprintf(backup_name, sizeof(backup_name), "%s.%ld", m_filename, time(NULL));   // BACKUP filename = filename+time
    
    pthread_mutex_lock(&file_mutex);
    {
        if(rename(m_filename, backup_name) != 0)              // 작업 전 원래 파일을 백업파일로 move
        {
            Log.printf(LOG_ERR, "[SSWLIST_TBL] erase_file() Can't backup file %s reason=%s \n", m_filename, strerror(errno));
            pthread_mutex_unlock(&file_mutex);
            return(false);
        }
        
        if((old_fp = fopen(backup_name,  "r")) == NULL)
        {
            Log.printf(LOG_ERR, "[SSWLIST_TBL] erase_file() Can't Open backup file %s \n", backup_name);
            pthread_mutex_unlock(&file_mutex);
            return(false);
        }
        
        if((new_fp = fopen(m_filename,  "w")) == NULL)
        {
            Log.printf(LOG_ERR, "[SSWLIST_TBL] erase_file() Can't Open file %s \n", m_filename);
            fclose(old_fp);
            pthread_mutex_unlock(&file_mutex);
            return(false);
        }
        
        // backup 파일을 한줄씩 읽어서 sswlist.cfg 파일로 write 또는 update
        while(fgets(buf, 255, old_fp))
        {
            if(done)        // delete가 완료되었음
            {
                fputs(buf, new_fp);
            }
            else
            {
                // comment를 제외한 부분에서 strIP에서 비교하기 위해서 comment 시작부분을 NULL로 set
                ptrComment = strchr(buf, '#');
                if(ptrComment != NULL) { *ptrComment = '\0'; }      // set NULL for copy
                
                if(strstr(strIP, buf) != NULL)
                {
                    // strIP가 등록되어 있는 라인 삭제
                    done = true;
                }
                else
                {
                    if(ptrComment != NULL) { *ptrComment = '#'; }      // restore comment
                    fputs(buf, new_fp);
                }
            }
        }
        
        fclose(old_fp);
        fclose(new_fp);
    }
    pthread_mutex_unlock(&file_mutex);
	return(true);
}

#endif
