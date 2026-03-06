/* File Header
*fsh********************************************************
************************************************************
**
**  FILE : LIB_com.c
**
************************************************************
************************************************************
  SYSTEM         :
  BLOCK          : LIB
  SOR-NAME       :
  VERSION        : V2.0
  DATE           : 2004/01/23
  AUTHOR         : SEUNG-MO, CHO
  HISTORY        :
  PROCESS(TASK)  : 
  PROCEDURES     : 
  DESCRIPTION    : 
  REMARKS        : 
  HISTORY        :
*end*******************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include	"libnsmcom.h"

void LIB_strcpy(char* dst, char* src)
{
    /* Date    : 20240105
 	 * Version : OrigoAS 5.0.0
 	 * OS      : RHEL8.5
 	 * GCC     : 8.5 
 	 * Desc    : OS ¹öÀü¾÷ µÇ¸é¼­ strcpy¿¡ dst¿Í srcÀÇ Æ÷ÀÎÅÍ¸¦  °°Àº ¹öÆÛ·Î ÁöÁ¤ÇÏ¸é ¿À·ù°¡ ¹ß»ýÇÔ
 	 * */
    char temp[SAFETY_MAX_BUF_SIZE]; 
	int  dst_len;

    memset(temp, 0x00, sizeof(temp));
	dst_len = 0;

	if(src == NULL || dst == NULL) { return; }
	if( (SAFETY_MAX_BUF_SIZE-1) < strlen(src) ) { return; }
	
	strncpy(temp, src, SAFETY_MAX_BUF_SIZE - 1);

	dst_len = strlen(dst);
	if( dst_len < strlen(temp) ) { return; }

	strncpy(dst, temp, dst_len);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_ExtendResourceLimit
 * CLASS-NAME     :
 * PARAMETER    IN: resource - Limit를 변경할 resource
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 시스템 리소스를 max까지 확장
 * REMARKS        :
 **end*******************************************************/
int LIB_ExtendResourceLimit(int resource)
{
	struct rlimit	limit;

	if(getrlimit(resource, &limit) < 0) { return(0); }

	limit.rlim_cur = limit.rlim_max;
	if(setrlimit(resource, &limit) < 0) { return(0); }
	return(1);
}

#if 0
/* Procedure Header
*pdh********************************************************
  PROCEDURE-NAME : LIB_HexDump
  PARAMETER      : 
  RET. VALUE     : 
  RECEIVED MSG.  : 
  SENT  MSG.     : 
  PROCEDURES     : 
  DESCRIPTION    : 
  REMARKS        : 
*end*******************************************************/
void LIB_HexDump(char *str, int length)
{
    int i;

	printf("%d : ", length);
	for(i = 0; i < length; i ++)
		printf("%02X ", (unsigned char)*str++);

	printf("\n");
}
#endif

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_OpenMMAP
 * CLASS-NAME     :
 * PARAMETER    IN: filename - MMAP에 매핑할 파일이름
 *             OUT: size - 매핑된 파일 Size
 * RET. VALUE     : 0(ERR)/address(매핑 주소)
 * DESCRIPTION    : 시스템 리소스를 max까지 확장
 * REMARKS        :
 **end*******************************************************/
char *LIB_OpenMMAP(char *filename, int *size)
{
	int     fp;
	char    *p;

	if((fp = open(filename, O_RDONLY)) < 0) { return((char *)0); }

	*size = (int)lseek(fp, 0L, SEEK_END);    // or fstat

	if(*size == 0)
	{
		close(fp);
		return((char *)0);
	}
	p = (char *)mmap(0, *size, PROT_READ, MAP_PRIVATE, fp, 0);
	close(fp);

	return(p);
}



#pragma mark -
#pragma mark is Series Library functions

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_isxdigit
 * CLASS-NAME     :
 * PARAMETER    IN: str    - 16진수 인지 검사할 문자열
 *              IN: length - 검사할 문자열의 길이
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 입력된 문자열이 16진수인지 아닌지를 검사하는 함수
 * REMARKS        :
 **end*******************************************************/
int LIB_isxdigit(char *str, int length)
{
	int		i, start;

	if(strncmp(str, "0x", 2) == 0)
		start = 2;
	else
		start = 0;

	for(i = start; i < length; i ++)
	{
		if(isxdigit(str[i]) == 0) { return(0); }
	}
	return(1);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_isdigit
 * CLASS-NAME     :
 * PARAMETER    IN: str    - 10진수 인지 검사할 문자열
 *              IN: length - 검사할 문자열의 길이
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 입력된 문자열이 10진수인지 아닌지를 검사하는 함수
 * REMARKS        :
 **end*******************************************************/
int LIB_isdigit(char *str, int length)
{
	int		i;

	for(i = 0; i < length; i ++)
	{
		if(isdigit(str[i]) == 0) { return(0); }
	}
	return(1);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_isbcd
 * CLASS-NAME     :
 * PARAMETER    IN: bcd -  BCD값인지 인지 검사할 문자
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 입력된 문자가 BCD인지 아닌지를 검사하는 함수
 * REMARKS        :
 **end*******************************************************/
int LIB_isbcd(int bcd)
{
	if((bcd & 0x0F) > 9) return(0);
	if(((bcd & 0xF0) >> 4) > 9) return(0);
	return(1);
}


#pragma mark -
#pragma mark String 관련 Library functions

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_str2int
 * CLASS-NAME     :
 * PARAMETER    IN: str - 숫자로로 변환할 문자열(10진수 16진수)
 * RET. VALUE     : int(숫자)
 * DESCRIPTION    : 입력된 문자열을 숫자로 변환 함수
 * REMARKS        : 입력문자열은 10 or 16 진수
 **end*******************************************************/
int LIB_str2int(char *str)
{
	int     base;

	if(strncmp(str, "0x", 2) == 0) base = 16;
	else 						   base = 10;

	return((int)strtol(str, (char **)0, base));
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_atoi
 * CLASS-NAME     :
 * PARAMETER    IN: str    - 숫자로로 변환할 문자열(10진수만)
 *              IN: length - 검사할 문자열의 길이
 * RET. VALUE     : int(숫자)
 * DESCRIPTION    : 입력된 문자열을 숫자로 변환하는 함수
 * REMARKS        : 16진수는 LIB_str2int 사용
 **end*******************************************************/
int LIB_atoi(char *str, int length)
{
	char	buf[32];

	if(length > 32) return(0);

	strncpy(buf, str, length);
	buf[length] = '\0';

	return(atoi(buf));
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_hex2int
 * CLASS-NAME     :
 * PARAMETER    IN: hex - 숫자로로 변환할 16진수 문자
 * RET. VALUE     : int
 * DESCRIPTION    : 입력된 16진수 문자를 숫자로 변환하는 함수
 * REMARKS        :
 **end*******************************************************/
int LIB_hex2int(const char hex)
{
	if(('0' <= hex) && (hex <= '9')) { return(hex - '0'); }
    
	switch(hex)
	{
        case 'a': case 'A': return(10);
        case 'b': case 'B': return(11);
        case 'c': case 'C': return(12);
        case 'd': case 'D': return(13);
        case 'e': case 'E': return(14);
        case 'f': case 'F': return(15);
	}
	return(-1);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_delete_white_space
 * CLASS-NAME     :
 * PARAMETER INOUT: str - space를 제거할 문자열
 * RET. VALUE     : -
 * DESCRIPTION    : 문자열에서 space를 제거
 * REMARKS        : 입력문자열이 변경됨
 **end*******************************************************/
void LIB_delete_white_space(char *str)
{
	char 	*s, *d;
	char	quotation = 0;

	s = d = str;
	while (*s)  
	{
		if(*s == '"') { quotation ^= 1; s++; continue; }

		if(!quotation)
		{
			if(isspace(*s)) { s++; }
			else 			{ *d++ = *s++; }
		}
		else
		{
			*d++ = *s++;
		}
	}
	*d = '\0';
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_split_string
 * CLASS-NAME     :
 * PARAMETER    IN: str   - 나누어질 원본 문자열
 *              IN: split - 나누는 기준이되는 구분자
 *             OUT: parms - 나누어진 문자열이 저장되는 변수
 * RET. VALUE     : -
 * DESCRIPTION    : 문자열을 여러개로 나눔(split를 기준으로)
 * REMARKS        : 입력 문자열이 변경됨 
 *                : --> parms는 반드시 초기화 후에 호출해야 함
 **end*******************************************************/
void LIB_split_string(char *str, int split, STRs *parms)
{
    char    *ptr;
    char    var[MAX_STRs_LEN];
    
    if(parms->cnt >= MAX_STRs_CNT) return;
    
    if((ptr = strchr(str, split)) == 0)
    {
        if(str[0] != 0)
        {
            strcpy(parms->str[parms->cnt], str);
            parms->cnt ++;
        }
        return;
    }
    
    *ptr++ = 0;
    strcpy(parms->str[parms->cnt], str);
    strcpy(var, ptr);
    //    delete_white_space(var);
    
    parms->cnt ++;
    LIB_split_string(var, split, parms);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_split_const_string
 * CLASS-NAME     :
 * PARAMETER    IN: instr - 나누어질 원본 문자열
 *              IN: split - 나누는 기준이되는 구분자
 *             OUT: parms - 나누어진 문자열이 저장되는 변수
 * RET. VALUE     : -
 * DESCRIPTION    : 문자열을 여러개로 나눔(split를 기준으로)
 * REMARKS        : 입력문자열 변경 없음
 **end*******************************************************/
void LIB_split_const_string(const char *instr, int split, STRs *param)
{
    char    *str;
    
    str = (char *)malloc(strlen(instr)+1);
    strcpy(str, instr);

    bzero(param, sizeof(STRs));
    LIB_split_string(str, split, param);       // request-line
    free(str);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_split_string_into_2
 * CLASS-NAME     :
 * PARAMETER    IN: str   - 나누어질 원본 문자열
 *              IN: split - 나누는 기준이되는 구분자
 *             OUT: var1  - 나누어진 첫 번째 문자열이 저장되는 변수
 *             OUT: var2  - 나누어진 두 번째 문자열이 저장되는 변수
 * RET. VALUE     : -
 * DESCRIPTION    : 입력문자열을 2개로 나눔
 * REMARKS        : 입력 문자열이 변경됨
 **end*******************************************************/
int LIB_split_string_into_2(char *str, int split, char *var1, char *var2)
{
    char *p;
    
    p = strchr(str, split);
    
    if(p == NULL) return (0);
    
    *p++ = '\0';
    strcpy(var1, str);
    strcpy(var2, p);
    return(1);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_split_const_string_into_2
 * CLASS-NAME     :
 * PARAMETER    IN: instr - 나누어질 원본 문자열
 *              IN: split - 나누는 기준이되는 구분자
 *             OUT: var1  - 나누어진 첫 번째 문자열이 저장되는 변수
 *             OUT: var2  - 나누어진 두 번째 문자열이 저장되는 변수
 * RET. VALUE     : -
 * DESCRIPTION    : 입력문자열을 2개로 나눔
 * REMARKS        : 입력문자열 변경 없음
 **end*******************************************************/
int LIB_split_const_string_into_2(const char *instr, int split, char *var1, char *var2)
{
    int     ret;
    char    *str;
    
    str = (char *)malloc(strlen(instr)+1);
    strcpy(str, instr);

    ret = LIB_split_string_into_2(str, split, var1, var2);
    free(str);
    return(ret);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_delete_comment
 * CLASS-NAME     :
 * PARAMETER INOUT: str         - comment를 삭제할 입력 문자열
 *              IN: comment_var - 삭제할 comment 문자
 * RET. VALUE     : -
 * DESCRIPTION    : str에서 comment_var 이하 삭제
 * REMARKS        : 원본 문자열이 변경 됨
 **end*******************************************************/
void LIB_delete_comment(char *str, int comment_var)
{
	char *p;
    
	p = strchr(str, comment_var);
    
	if(p == NULL) return;
    
	*p = '\0';
}


/* Procedure Header
 *pdh********************************************************
 PROCEDURE-NAME : LIB_toupper
 PARAMETER      : IN
 RET. VALUE     : -NONE-
 RECEIVED MSG.  :
 SENT  MSG.     :
 PROCEDURES     :
 DESCRIPTION    :
 REMARKS        :
 *end*******************************************************/
void LIB_toupper(char *str)
{
    char    quotation = 0;
    
    while (*str)
    {
        if(*str == '"') { quotation ^= 1; str++; continue; }
        
        if(!quotation) { *str = toupper(*str); }
        
        str++;
    }
}


#if 0
/* Procedure Header
*pdh********************************************************
  PROCEDURE-NAME : split_string_into_n
  PARAMETER      : IN IN OUT OUT
  RET. VALUE     : TABLE Element count
  RECEIVED MSG.  : 
  SENT  MSG.     : 
  PROCEDURES     : 
  DESCRIPTION    : a/b/c/d/e... ø°º≠ a,b,c,d,e ...∏¶ √√
  REMARKS        : Recursive function
*end*******************************************************/
void split_string_into_n(char *str, int split, char **elem, int *cnt, int max_cnt, int max_len)
{
	char	*ptr;
	char	var[1024];

	if(*cnt >= max_cnt) return;

	if((ptr = strchr(str, split)) == 0) 
	{ 
		if(str[0] != 0)
		{
			elem[*cnt] = (char *)malloc(max_len+1);
			strncpy(elem[*cnt], str, max_len);
			delete_white_space(elem[*cnt]);
			(*cnt) ++;
		}
		return; 
	}

	*ptr++ = 0;
	elem[*cnt] = (char *)malloc(max_len+1);
	strncpy(elem[*cnt], str, max_len);
	strncpy(var, ptr, 1023);

	(*cnt) ++;
	split_string_into_n(var, split, elem, cnt, max_cnt, max_len);
}

/* Procedure Header
*pdh********************************************************
  PROCEDURE-NAME : LIB_make_printable_string
  PARAMETER      : 
  RET. VALUE     : 
  RECEIVED MSG.  : 
  SENT  MSG.     : 
  PROCEDURES     : 
  DESCRIPTION    : 
  REMARKS        : 
*end*******************************************************/
void LIB_make_printable_string(char *str)
{   
	char *s, *d;

	s = d = str;
	while (*s)
	{
		if(!isprint(*s)) { s++; }
		else             { *d++ = *s++; }
	}
	*d = '\0';
}
#endif


#pragma mark -
#pragma mark AS Process 관련 Library functions

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_get_as_id
 * CLASS-NAME     :
 * PARAMETER    UT: id   -  AS ID
 * RET. VALUE     : BOOL
 * DESCRIPTION    : role.def파일에서 as id를 구하는 함수
 * REMARKS        : 0~7
 **end*******************************************************/
int LIB_get_as_id(uint8_t *id)
{
	FILE	*fp;
	char	buf[80], var[80], val[80];
    
	if((fp = fopen("/home/mcpas/cfg/role.def", "r")) == NULL) { return(0); 	}
    
	while(fgets(buf, 79, fp))
	{
		if(LIB_split_string_into_2(buf, '=', var, val) == 0) continue;
		LIB_delete_white_space(var);
		LIB_delete_white_space(val);
        
		if(!strcasecmp("INDEX", var))
		{
			*id = atoi(val);
			fclose(fp);
			return(1);
		}
	}
	fclose(fp);
	return(0);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_get_server_side_id
 * CLASS-NAME     :
 * PARAMETER    IN: name - AS 서버(CMS/SES/...)의 ID를 구하는 함수
 *             OUT: id   - 서버 ID
 * RET. VALUE     : BOOL
 * DESCRIPTION    : role.def파일에서 server side id를 구하는 함수
 * REMARKS        : A(0) or B(1)
 **end*******************************************************/
int LIB_get_server_side_id(const char *name, uint8_t *id)
{
	FILE	*fp;
	char	buf[80], var[80], val[80];
    
    
	if((fp = fopen("/home/mcpas/cfg/role.def", "r")) == NULL) { return(0); 	}
    
	while(fgets(buf, 79, fp))
	{
		if(LIB_split_string_into_2(buf, '=', var, val) == 0) continue;
		LIB_delete_white_space(var);
		LIB_delete_white_space(val);
        
		if(!strcasecmp(name, var))
		{
			*id = atoi(val);
			fclose(fp);
			return(1);
		}
	}
	fclose(fp);
	return(0);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_set_run_pid
 * CLASS-NAME     :
 * PARAMETER    IN: name - /var/run/mcpas에 저장할 process name
 * RET. VALUE     : BOOL
 * DESCRIPTION    : /var/run/mcpas에 proess_id를 저장하는 함수
 * REMARKS        : 저장된 pid는 프로세스 중복 실행 방지에 활용됨
 **end*******************************************************/
int LIB_set_run_pid(const char *name)
{
	char    fname[64];
	FILE    *fp;

	snprintf(fname, 63, "/var/run/mcpas/%s.pid", name);
	if((fp = fopen(fname, "w+")) == 0) { return(0); }
	fprintf(fp, "%d", getpid());
	fclose(fp);
	return(1);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_unset_run_pid
 * CLASS-NAME     :
 * PARAMETER    IN: name - /var/run/mcpas에서 삭제할 process name
 * RET. VALUE     : BOOL
 * DESCRIPTION    : /var/run/mcpas/name.pid 파일을 삭제하는 함수
 * REMARKS        :
 **end*******************************************************/
int LIB_unset_run_pid(const char *name)
{
	char    fname[64];

	snprintf(fname, 63, "/var/run/mcpas/%s.pid", name);
	if(remove(fname) < 0) { return(0); }
	return(1);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_is_running_process
 * CLASS-NAME     :
 * PARAMETER    IN: name  - 실행중인지 확인할 process name (PATH 제외)
 *             OUT: ps_id - 실행 중인 pid(실행중이 아니면 0)
 * RET. VALUE     : BOOL(1이면 실행 중 그외는 0)
 * DESCRIPTION    : 해당 프로세스가 실행 중인지 확인 하는 함수
 * REMARKS        :
 **end*******************************************************/
int LIB_is_running_process(const char *name, int *ps_id)
{
	char    fname[64], buf[80], var[64], val[64];
	int		pid;
	FILE    *fp;
    
    *ps_id  = 0;        // initial

	snprintf(fname, 63, "/var/run/mcpas/%s.pid", name);
	if((fp = fopen(fname, "r")) == 0) { return(0); }
	fscanf(fp, "%d", &pid);
	fclose(fp);

	*ps_id = pid;

	snprintf(fname, 63, "/proc/%d/status", pid);
	if((fp = fopen(fname, "r")) == 0) { return(0); }

	if(fgets(buf, 79, fp) == 0) { fclose(fp); return(0); }

	if(LIB_split_string_into_2(buf, ':', var, val) == 0) { fclose(fp); return(0); }
	LIB_delete_white_space(var);
	LIB_delete_white_space(val);

	if(strcasecmp("Name", var)) { fclose(fp); return(0); }

	if(strcmp(name, val) == 0) { fclose(fp); return(1); }
	else					   { fclose(fp); return(0); }
}



#pragma mark -
#pragma mark STX-ETX Library functions (use SAM/DGM/...)

#if 0   // SPY/SAM에서 각자 구현해서 사용
/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_GET_IntInMsgFromSRM
 * CLASS-NAME     :
 * PARAMETER    IN: str   - 찾아야할 대상이 포함되어 있는 전체 문자열
 *              IN: key   - 찾을값의 Key가 되는 문자열
 *             OUT: value - 찾은 값 (value에 해당하는 값)
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 문자열에서 특정 Key의 값에 해당하는 정수를 구하는 함수
 * REMARKS        : "key=STXvalueETX"
 **end*******************************************************/
int LIB_GET_IntInMsgFromSRM(char *str, const char *key, int *value)
{
    int     slen;
    char    *ptr, *pSTX, *pETX;
    char    svalue[16];
    
    *value = 0;     // initialize
    
    if((ptr  = strcasestr(str, key))   == NULL) { return(false); }
    if((pSTX = strchr(ptr, PARAM_STX)) == NULL) { return(false); }
    if((pETX = strchr(ptr, PARAM_ETX)) == NULL) { return(false); }
    
    slen = (int)(pETX - pSTX);
    if((slen >= 11) || (slen <= 0)) { return(false); }  // MAX 10 digit
    
    bzero(svalue, 16);
    strncpy(svalue, pSTX+1, slen-1);
    *value = atoi(svalue);
    return(true);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_GET_StrInMsgFromSRM
 * CLASS-NAME     :
 * PARAMETER    IN: str    - 찾아야할 대상이 포함되어 있는 전체 문자열
 *              IN: key    - 찾을값의 Key가 되는 문자열
 *             OUT: value  - 찾은 값 (value에 해당하는 값)
 *              IN: maxlen - value 문자열의 최대 길이
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 문자열에서 특정 Key의 값에 해당하는 문자열을 구하는 함수
 * REMARKS        : "key=STXvalueETX"
 **end*******************************************************/
int LIB_GET_StrInMsgFromSRM(char *str, const char *key, char *value, int maxlen)
{
    int     slen;
    char    *ptr, *pSTX, *pETX;
    
    value[0] = '\0';    // initialize
    
    if((ptr  = strcasestr(str, key))   == NULL) { return(false); }
    if((pSTX = strchr(ptr, PARAM_STX)) == NULL) { return(false); }
    if((pETX = strchr(ptr, PARAM_ETX)) == NULL) { return(false); }
    
    slen = (int)(pETX - pSTX);
    
    if(slen <= 0) { return(false); }
    
    if(slen > maxlen) { slen = maxlen; }
    
    strncpy(value, pSTX+1, slen-1);
    value[slen-1] = '\0';
    return(true);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_SetStrToSendMsgBody
 * CLASS-NAME     :
 * PARAMETER INOUT: send     - 값을 저장할 전체 문자열
 *           INOUT: body_len - 전체 문자열(send)의 길이
 *              IN: key      - 저장할 Key가 되는 문자열
 *              IN: value    - 저장할 값 (문자열)
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 문자열에서 value가 문자열인 key-value 문자열을 추가하는 함수
 * REMARKS        : "key=STXvalueETX"
 **end*******************************************************/
void LIB_SetStrToSendMsgBody(char *send, int *body_len, const char *key, const char *value)
{
    // FIXIT - buffer overflow  case....
    *body_len += sprintf(&send[*body_len], "%s=%c%s%c ", key, PARAM_STX, value, PARAM_ETX);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_SetIntToSendMsgBody
 * CLASS-NAME     :
 * PARAMETER INOUT: send     - 값을 저장할 전체 문자열
 *           INOUT: body_len - 전체 문자열(send)의 길이
 *              IN: key      - 저장할 Key가 되는 문자열
 *              IN: value    - 저장할 값 (정수)
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 문자열에서 value가 정수인 key-value 문자열을 추가하는 함수
 * REMARKS        : "key=STXvalueETX"
 **end*******************************************************/
void LIB_SetIntToSendMsgBody(char *send, int *body_len, const char *key, int value)
{
    *body_len += sprintf(&send[*body_len], "%s=%c%d%c ", key, PARAM_STX, value, PARAM_ETX);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : LIB_SetHexToSendMsgBody
 * CLASS-NAME     :
 * PARAMETER INOUT: send     - 값을 저장할 전체 문자열
 *           INOUT: body_len - 전체 문자열(send)의 길이
 *              IN: key      - 저장할 Key가 되는 문자열
 *              IN: value    - 저장할 값 (16진수 문자열)
 * RET. VALUE     : BOOL
 * DESCRIPTION    : 문자열에서 value가 16진수 문자열인 key-value 문자열을 추가하는 함수
 * REMARKS        : "key=STXvalueETX"
 **end*******************************************************/
void LIB_SetHexToSendMsgBody(char *send, int *body_len, const char *key, const char *value)
{
    *body_len += sprintf(&send[*body_len], "%s=%c%02x%02x%02x%02x%c ", key, PARAM_STX, value[0], value[1], value[2], value[3], PARAM_ETX);
}
#endif
