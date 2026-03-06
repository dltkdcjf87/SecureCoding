
#ifndef __LIB_NSMCOM_H__
#define	__LIB_NSMCOM_H__

// C++ only Version
#include <stdint.h>

#define MAX_BUF_SIZE        8192
#define SAFETY_MAX_BUF_SIZE MAX_BUF_SIZE+1+512
// for splite string
#define MAX_STRs_CNT        16
#define MAX_STRs_LEN        2048
typedef struct
{
	char    cnt;
	char    str[MAX_STRs_CNT][MAX_STRs_LEN+1];
} STRs;

#define PARAM_STX           0x01
#define PARAM_ETX           0x02

// 사용여부 미확인 Library
extern  int     LIB_ExtendResourceLimit(int resource);
extern	char 	*LIB_OpenMMAP(char *filename, int *size);

// 문자열 삭제/변환 Library
extern	void 	LIB_split_string(char *str, int split, STRs *parms);
extern  void    LIB_split_const_string(const char *instr, int split, STRs *param);
extern	int 	LIB_split_string_into_2(char *str, int split, char *s1, char *s2);
extern  int     LIB_split_const_string_into_2(const char *instr, int split, char *var1, char *var2);

extern	void 	LIB_delete_white_space(char *str);
extern	void 	LIB_delete_comment(char *str, int comment);

// 문자/문자열/숫자 변환 함수
extern	int 	LIB_isxdigit(char *str, int length);
extern	int 	LIB_isdigit(char *str, int length);
extern	int 	LIB_isbcd(int bcd);
extern	int 	LIB_str2int(char *str);
extern	int 	LIB_atoi(char *str, int length);
extern  int     LIB_hex2int(const char hex);
extern  void    LIB_toupper(char *str);
extern  void	LIB_strcpy(char* dst, char* src);

// AS Process ID/NAME 관련 함수
extern  int     LIB_get_as_id(uint8_t *id);
extern  int     LIB_get_server_side_id(const char *name, uint8_t *id);
extern	int 	LIB_is_running_process(const char *name, int *pid);
extern	int 	LIB_set_run_pid(const char *name);
extern	int 	LIB_unset_run_pid(const char *name);

#if 0   // 삭제 예정 - SAM/SRM에서 각자 구현해서 사용
// SAM/SRM과 통신하기 위해 메세지를 구성하기 위한 함수(STX/ETX를 이용하여 key-value 형태로 저장)
extern int      LIB_GetIntInStxEtxMsg(char *str, const char *key, int *value);
extern int      LIB_GetStrInStxEtxMsg(char *str, const char *key, char *value, int maxlen);
extern void     LIB_SetStrToSendMsgBody(char *send, int *body_len, const char *key, const char *value);
extern void     LIB_SetIntToSendMsgBody(char *send, int *body_len, const char *key, int value);
extern void     LIB_SetHexToSendMsgBody(char *send, int *body_len, const char *key, const char *value);
#endif
extern	void 	split_string_into_n(char *str, int split, char **out, int *cnt, int max_cnt, int max_len);
extern	void	LIB_make_printable_string(char *str);

#if 0
extern	void 	LIB_HexDump(char *str, int length);
#endif

//// 삭제예정 (예전에 사용하는 함수 이름으로 지금은 변경되어 사용되지 않음)
//#define split_string            LIB_split_string
//#define split_string_into_2     LIB_split_string_into_2
//#define delete_comment          LIB_delete_comment
//#define delete_white_space      LIB_delete_white_space
//#define ExtendResourceLimit     LIB_ExtendResourceLimit
//#define LIB_get_server_name     LIB_get_server_side_id

#endif // __LIB_NSMC_H__
