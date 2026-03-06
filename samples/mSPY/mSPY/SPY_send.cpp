/* File Header
 *fsh**************************************************************
 ******************************************************************
 **
 **  FILE : SPY_send.cpp
 **
 ******************************************************************
 ******************************************************************
 BLOCK          : SPY
 SUBSYSTEM      : CMS
 SOR-NAME       :
 VERSION        : V4.X
 DATE           :
 AUTHOR         : SEUNG-MO, CHO
 HISTORY        :
 PROCESS(TASK)  :
 PROCEDURES     :
 DESCRIPTION    :
 REMARKS        :
 *end*************************************************************/

#include "SPY_def.h"
#include "SPY_xref.h"

typedef struct
{
    char    header[16];
    char    strSQL[112];
} S_TO_ORACLE;



#pragma mark -
#pragma mark Log Level을 ORACLE로 전송하는 함수

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SEND_LogLevelToEMS
 * CLASS-NAME     :
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : EMS ORACEL에 Log Level을 전송하는 함수
 * REMARKS        : SGM을 경유해서 EMS ORACLE에 전송
 **end*******************************************************/
void SEND_LogLevelToEMS(void)
{
    S_TO_ORACLE    send;
    int     nLen;
    
    bzero(&send, sizeof(send));
    
	send.header[0] = 1;
	nLen  = 16;        // sizeof(send.header) = 16
	nLen += snprintf(send.strSQL, sizeof(send.strSQL), "begin sp_setLogLevel(%d, %d, %d); end;", MY_AS_ID, MY_BLK_ID, C_LOG_LEVEL);
    
    msendsig(nLen, SGM, MSG_ID_SGM_QUERY, (uint8_t *)&send);
    
    Log.printf(LOG_LV1, "SEND_LogLevelToEMS(ORACLE) strSQL=%s\n", send.strSQL);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SEND_AsBlockStatusToEMS
 * CLASS-NAME     :
 * PARAMETER      : -
 * RET. VALUE     : -
 * DESCRIPTION    : EMS ORACEL에 AS BLOCK/UNBLOCK을 전송하는 함수
 * REMARKS        : SGM을 경유해서 EMS ORACLE에 전송
 **end*******************************************************/
void SEND_AsBlockStatusToEMS(void)
{
    S_TO_ORACLE    send;
    int     nLen;
    
    bzero(&send, sizeof(send));
    
	send.header[0] = 1;
	nLen  = 16;        // sizeof(send.header) = 16
	nLen += snprintf(send.strSQL, sizeof(send.strSQL), "UPDATE TBL_AS_CONFIG SET STATUS_FLAG=%d where AS_IDX=%d", C_AS_BLOCK, MY_AS_ID);
    
    msendsig(nLen, SGM, MSG_ID_SGM_QUERY, (uint8_t *)&send);
    
    Log.printf(LOG_LV1, "SEND_AsBlockStatusToEMS(ORACLE) strSQL=%s\n", send.strSQL);
}





