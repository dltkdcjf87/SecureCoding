/* File Header
 *fsh**************************************************************
 ******************************************************************
 **
 **  FILE : SPY_xbus.cpp
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
 DESCRIPTION    : XBUS 관련 functions
 REMARKS        :
 *end*************************************************************/

#include "SPY_def.h"
#include "SPY_xref.h"
#include "VersionHistory.h"
//#include "SSWServer.h"
//#include "SAMServer.h"




#pragma mark -
#pragma mark WEB(DGM 경유)에서 온 EXT. Server 등록/변경/삭제 및 AS BLOCK/UNBLOCK 처리 함수


/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : SPY_SendResponseToWEB
 * CLASS-NAME     : -
 * PARAMETER    IN: recv   - WEB에서 수신한 메세지 pointer
 *              IN: result - WEB으로 전송할 처리 결과
 * RET. VALUE     : -
 * DESCRIPTION    : WEB 메세지 처리 결과 송신(0(ERR)/1(OK))
 * REMARKS        : SPY -> DGM -> WEB
 **end*******************************************************/
void SPY_SendResponseToWEB(R_CMD_FROM_WEB *recv, char result, char reason)
{
    S_RESULT_TO_WEB send;
    
    send.hdr     = recv->hdr;
	send.hdr.len = sizeof(DGM_HDR)+1;
	send.result  = result;
    
    msendsig(sizeof(DGM_HDR)+1, DGM, MSG_ID_DGM_RELAY, (uint8_t *)&send);
}

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : WEB_AsBlockRQ
 * CLASS-NAME     : -
 * PARAMETER    IN: rPDU - XBUS로 수신한 Block/Unblock 요청 PDU pointer
 * RET. VALUE     : -
 * DESCRIPTION    : AS BLOCK/UNBLOCK Request
 * REMARKS        : WEB -> DGM -> SPY
 **end*******************************************************/
int WEB_AsBlockRQ(DGM_HDR *rPDU)
{
    R_AS_BLOCK_RQ   *recv = (R_AS_BLOCK_RQ *)rPDU;
    bool            bBlock;

    bBlock = recv->block - '0';
    
    // 현재와 다르면 SET
    if(C_AS_BLOCK != bBlock) { SPY_SetAndWriteAsBlockStatus(bBlock); }
    
    // FILE UPDATE는 ACTIVE/STANDBY 모두 하지만 응답은 ACTIVE만 한다.
    if (g_nHA_State != HA_Active) { return(0); }
    
    SPY_SendResponseToWEB((R_CMD_FROM_WEB *)rPDU, '1', '0');
    
    Log.printf(LOG_INF, "WEB_AsBlockRQ() Request=%d Current=%d\n", bBlock, C_AS_BLOCK);
    
    SEND_AsBlockAlarm();

    return(0);
}

#ifdef IPV6_MODE
/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : WEB_ExtSvrRegRQ_V6
 * CLASS-NAME     : -
 * PARAMETER    IN: rPDU - XBUS로 수신한 외부 서버 등록/변경/삭제 요청 PDU
 * RET. VALUE     : -
 * DESCRIPTION    : Ext. Server INSERT/UPDATE/DELETE Request
 * REMARKS        : WEB -> DGM -> SPY
 **end*******************************************************/
int WEB_ExtSvrRegRQ_V6(DGM_HDR *rPDU)
{
    R_EXT_SVR_REG_RQ_V6    *recv = (R_EXT_SVR_REG_RQ_V6 *)rPDU;
    EXTSVR_LIST         extInfo;
    char                strCheckTime[4];
    char                result = '1';  // Default OK
    char                reason = '0';
    
    bzero(&extInfo,     sizeof(extInfo));
    bzero(strCheckTime, sizeof(strCheckTime));
    
    strncpy(strCheckTime,  recv->checkTime, sizeof(strCheckTime)-1);    // Time: 30, 60, 120, 180 SEC
    strncpy(extInfo.sswid, recv->sswid,     sizeof(extInfo.sswid)-1);   // SSWID: 0 ~ 999
    strncpy(extInfo.ip,    recv->ip,        sizeof(extInfo.ip)-1);
    LIB_change_space_to_null(extInfo.ip, sizeof(extInfo.ip));
    
    extInfo.checkTime = atoi(strCheckTime);     // FIXIT: 이거 3자리 String 맞나???
    extInfo.block     = recv->block    - '0';   // FIXIT: check ??? ascii or number ??
    extInfo.tryCount  = recv->tryCount - '0';   // FIXIT: check ??? ascii or number ??
    extInfo.monitor   = recv->monitor  - '0';   // FIXIT: check ??? ascii or number ??
    
    Log.printf(LOG_INF, "WEB_ExtSvrRegRQ_V6() START - Current List Count=%d\n",  g_ExtList_V6.size());
    
    switch(recv->action[0])
    {
        case 'I':
            Log.printf(LOG_LV2, "SSW INSERT Id=[%s] IP=[%s] block=%d checkTime=%d, tryCount=%d\n", extInfo.sswid, extInfo.ip, extInfo.block, extInfo.checkTime, extInfo.tryCount);
            
            if(g_ExtList_V6.exist(&extInfo) == false)
            {
                if(g_ExtList_V6.insert(&extInfo, true) == false) { result = '0'; reason = '1'; }
            }
            else
            {
                if(g_ExtList_V6.update(&extInfo) == false) { result = '0'; reason = '2'; }
            }
            break;
            
        case 'D':
            Log.printf(LOG_LV2, "SSW DELETE Id=[%s] IP=[%s]\n", extInfo.sswid, extInfo.ip);
            
            if(g_ExtList_V6.erase(&extInfo) == false) { result = '0'; reason = '3'; }
            break;
            
        case 'U':
            Log.printf(LOG_LV2, "SSW UPDATE Id=[%s] IP=[%s] block=%d checkTime=%d, tryCount=%d\n", extInfo.sswid, extInfo.ip, extInfo.block, extInfo.checkTime, extInfo.tryCount);
            
            if(g_ExtList_V6.update(&extInfo)  == false) { result = '0'; reason = '4'; }
            break;
            
        default:
            Log.printf(LOG_ERR, "WEB_ExtSvrRegRQ_V6() Undefined Action=%c\n", recv->action[0]);
            result = '0';       // ERROR
            reason = '5';
            break;
    }
    
    if (g_nHA_State != HA_Active) { return(0); }        // FILE UPDATE는 ACTIVE/STANDBY 모두 하지만 응답은 ACTIVE만 한다.
    
    SPY_SendResponseToWEB((R_CMD_FROM_WEB *)rPDU, result, reason);
    
    Log.printf(LOG_INF, "WEB_ExtSvrRegRQ() END - Current List Count=%d\n",  g_ExtList_V6.size());
    return(0);
}
#endif  // IPV6_MODE

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : WEB_ExtSvrRegRQ
 * CLASS-NAME     : -
 * PARAMETER    IN: rPDU - XBUS로 수신한 외부 서버 등록/변경/삭제 요청 PDU
 * RET. VALUE     : -
 * DESCRIPTION    : Ext. Server INSERT/UPDATE/DELETE Request
 * REMARKS        : WEB -> DGM -> SPY
 **end*******************************************************/
int WEB_ExtSvrRegRQ(DGM_HDR *rPDU)
{
    R_EXT_SVR_REG_RQ    *recv = (R_EXT_SVR_REG_RQ *)rPDU;
    EXTSVR_LIST         extInfo;
    char                strCheckTime[4];
    char                result = '1';  // Default OK
    char                reason = '0';
    
    
    bzero(&extInfo,     sizeof(extInfo));
    bzero(strCheckTime, sizeof(strCheckTime));
    
    strncpy(strCheckTime,  recv->checkTime, sizeof(strCheckTime)-1);    // Time: 30, 60, 120, 180 SEC
    strncpy(extInfo.sswid, recv->sswid,     sizeof(extInfo.sswid)-1);   // SSWID: 0 ~ 999
    strncpy(extInfo.ip,    recv->ip,        sizeof(extInfo.ip)-1);
    LIB_change_space_to_null(extInfo.ip, sizeof(extInfo.ip));
                   
    extInfo.checkTime = atoi(strCheckTime);     // FIXIT: 이거 3자리 String 맞나???
    extInfo.block     = recv->block    - '0';   // FIXIT: check ??? ascii or number ??
    extInfo.tryCount  = recv->tryCount - '0';   // FIXIT: check ??? ascii or number ??
    extInfo.monitor   = recv->monitor  - '0';   // FIXIT: check ??? ascii or number ??
    
    Log.printf(LOG_LV2, "WEB_ExtSvrRegRQ() START - Current List Count=%d\n",  g_ExtList.size());

    switch(recv->action[0])
    {
        case 'I':
            Log.printf(LOG_LV2, "SSW INSERT Id=[%s] IP=[%s] block=%d checkTime=%d, tryCount=%d\n", extInfo.sswid, extInfo.ip, extInfo.block, extInfo.checkTime, extInfo.tryCount);
            
            if(g_ExtList.exist(&extInfo) == false)
            {
                if(g_ExtList.insert(&extInfo, true) == false) { result = '0'; reason = '1'; }
            }
            else
            {
                if(g_ExtList.update(&extInfo) == false) { result = '0'; reason = '2'; }
            }
            break;
            
        case 'D':
            Log.printf(LOG_LV2, "SSW DELETE Id=[%s] IP=[%s]\n", extInfo.sswid, extInfo.ip);
            
            if(g_ExtList.erase(&extInfo) == false) { result = '0'; reason = '3'; }
            break;
            
        case 'U':
            Log.printf(LOG_LV2, "SSW UPDATE Id=[%s] IP=[%s] block=%d checkTime=%d, tryCount=%d\n", extInfo.sswid, extInfo.ip, extInfo.block, extInfo.checkTime, extInfo.tryCount);
            
            if(g_ExtList.update(&extInfo)  == false) { result = '0'; reason = '4'; }
            break;
            
        default:
            Log.printf(LOG_ERR, "WEB_ExtSvrRegRQ() Undefined Action=%c\n", recv->action[0]);
            result = '0';       // ERROR
            reason = '5';
            break;
    }

    if (g_nHA_State != HA_Active) { return(0); }        // FILE UPDATE는 ACTIVE/STANDBY 모두 하지만 응답은 ACTIVE만 한다.

    SPY_SendResponseToWEB((R_CMD_FROM_WEB *)rPDU, result, reason);
    
    Log.printf(LOG_LV2, "WEB_ExtSvrRegRQ() END - Current List Count=%d\n",  g_ExtList.size());
    return(0);
}

#ifdef IPV6_MODE
/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : WEB_ExtSvrListRegRQ_V6
 * CLASS-NAME     : -
 * PARAMETER    IN: rPDU - XBUS로 수신한 외부 서버 List 등록 요청 PDU
 * RET. VALUE     : -
 * DESCRIPTION    : Multi Ext. Server INSERT Request
 * REMARKS        : WEB -> DGM -> SPY
 **end*******************************************************/
int WEB_ExtSvrListRegRQ_V6(DGM_HDR *rPDU)
{
    R_EXT_SVR_LIST_REG_RQ   *recv = (R_EXT_SVR_LIST_REG_RQ *)rPDU;
    R_LIST_REG_ITEM         *rData;
    EXTSVR_LIST     extInfo;
    char            strCheckTime[4];
    char            strCount[4];
    int             nCount;
    char            result = '1';   // '1': OK
    char            reason = '0';
    
    if (g_nHA_State == HA_Active) { g_ExtList_V6.update_tbl_ssw_config(); }    // UPDATE ORACLE
    
    bzero(strCount, sizeof(strCount));
    strncpy(strCount,  recv->count, sizeof(strCount)-1);
    nCount = atoi(strCount);
    
    Log.printf(LOG_LV2, "WEB_ExtSvrListRegRQ_V6() START - Request List Count=%d\n", nCount);
    
    rData = (R_LIST_REG_ITEM *)&recv->data_ptr;     // List Data pointer
    
    g_ExtList_V6.all_alarm_off();                   // 모든 ALARM을 OFF 후 clear
    g_ExtList_V6.clear();
    
    for(int i = 0; i < nCount; i ++)
    {
        bzero(&extInfo,     sizeof(extInfo));
        bzero(strCheckTime, sizeof(strCheckTime));
        
        strncpy(strCheckTime,  rData->checkTime, sizeof(strCheckTime)-1);
        strncpy(extInfo.sswid, rData->sswid,     sizeof(extInfo.sswid)-1);
        strncpy(extInfo.ip,    rData->ip,        sizeof(extInfo.ip)-1);
        LIB_change_space_to_null(extInfo.ip, sizeof(extInfo.ip));
        
        extInfo.checkTime = atoi(strCheckTime);     // FIXIT: 이거 3자리 String 맞나???
        
        extInfo.block     = rData->block    - '0';  // FIXIT: check ??? ascii or number ??
        extInfo.tryCount  = rData->tryCount - '0';  // FIXIT: check ??? ascii or number ??
        extInfo.monitor   = rData->monitor  - '0';  // FIXIT: check ??? ascii or number ??
        
        Log.printf(LOG_LV2, "WEB_ExtSvrListRegRQ_V6() INSERT Id=[%s] IP=[%s] block=%d checkTime=%d, tryCount=%d\n", extInfo.sswid, extInfo.ip, extInfo.block, extInfo.checkTime, extInfo.tryCount);
        
        if(g_ExtList_V6.insert(&extInfo, false) == false)      // FILE에는 저장하지 않고 나중에 한꺼번에 save_file()
        {
            Log.printf(LOG_LV2, "WEB_ExtSvrListRegRQ_V6() g_ExtList.insert fail.. so skip this line[%d]... continue\n", nCount);
            // LIST 이므로 하나가 에러가 나더라도 계속 진행
        }
        
        rData ++;       // NEXT List pointer
    }
    
    if(g_ExtList_V6.save_file() == false) { result = '0'; reason = '9'; }
    SPY_SendResponseToWEB((R_CMD_FROM_WEB *)rPDU, result, reason);
    
    Log.printf(LOG_INF, "WEB_ExtSvrListRegRQ() END - Current List Count=%d\n",  g_ExtList_V6.size());
    return(0);
}
#endif  // IPV6_MODE

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : Check_Configure_File
 * CLASS-NAME     : cSPY
 * PARAMETER      : IN
 * RET. VALUE     : -
 * DESCRIPTION    : Check Configure file when receive Update Event from Web
 * REMARKS        : by Web
 *                  2019.01.21 dw.choi
 **end*******************************************************/
void Check_Configure_File(char* _fileName, char* _returnResult, int _smallestSize=0)
{
    if( strlen(_fileName) <= 0 )
    {
        Log.printf(LOG_ERR, "[Check_Configure_File] wrong file Path\n", _fileName);
        strcpy(_returnResult, "0");
        return;
    }

    struct stat fst;
    if( stat(_fileName, &fst) == 0 ) // 2019.01.21 dw.choi - check file exist
    {
        if( fst.st_size < _smallestSize ) // 2019.01.21 dw.choi - check whether file size is too small
        {
            Log.printf(LOG_ERR, "[Check_Configure_File] path=%s\n", _fileName);
            Log.printf(LOG_ERR, "[Check_Configure_File] fst.st_size=%d, (minimum size is %d)\n", fst.st_size, _smallestSize);
            strcpy(_returnResult, "2");
            return;
        }

        FILE *confFile;
        if( (confFile = fopen(_fileName, "r+")) == NULL )
        {
            Log.printf(LOG_ERR, "[Check_Configure_File] path=%s\n", _fileName);
            Log.printf(LOG_ERR, "[Check_Configure_File] Can't open file(path=%s)\n", _fileName);
            strcpy(_returnResult, "4");

            return;
        }
        fclose(confFile);
    }
    else // 2019.01.21 dw.choi - file not exist
    {
        Log.printf(LOG_ERR, "[Check_Configure_File] file=%s is not exist.\n", _fileName);
        strcpy(_returnResult, "3");
        return;
    }
    strcpy(_returnResult, "1");
}

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : WEB_ExtSvrListRegRQ
 * CLASS-NAME     : -
 * PARAMETER    IN: rPDU - XBUS로 수신한 외부 서버 List 등록 요청 PDU
 * RET. VALUE     : -
 * DESCRIPTION    : Multi Ext. Server INSERT Request
 * REMARKS        : WEB -> DGM -> SPY
 **end*******************************************************/
int WEB_ExtSvrListRegRQ(DGM_HDR *rPDU)
{
    R_EXT_SVR_LIST_REG_RQ   *recv = (R_EXT_SVR_LIST_REG_RQ *)rPDU;
    R_LIST_REG_ITEM         *rData;
    EXTSVR_LIST     extInfo;
    char            strCheckTime[4];
    char            strCount[4];
    int             nCount;
    char            result = '1';   // '1': OK
    char            reason = '0';
    
    if (g_nHA_State == HA_Active) { g_ExtList.update_tbl_ssw_config(); }    // UPDATE ORACLE
    
    bzero(strCount, sizeof(strCount));
    strncpy(strCount,  recv->count, sizeof(strCount)-1);
    nCount = atoi(strCount);
    
    Log.printf(LOG_LV2, "WEB_ExtSvrListRegRQ() START - Request List Count=%d\n", nCount);
    
    rData = (R_LIST_REG_ITEM *)&recv->data_ptr;     // List Data pointer
    
    g_ExtList.all_alarm_off();                   // 모든 ALARM을 OFF 후 clear
    g_ExtList.clear();
    
    for(int i = 0; i < nCount; i ++)
    {
        bzero(&extInfo,     sizeof(extInfo));
        bzero(strCheckTime, sizeof(strCheckTime));
        
        strncpy(strCheckTime,  rData->checkTime, sizeof(strCheckTime)-1);
        strncpy(extInfo.sswid, rData->sswid,     sizeof(extInfo.sswid)-1);
        strncpy(extInfo.ip,    rData->ip,        sizeof(extInfo.ip)-1);
        LIB_change_space_to_null(extInfo.ip, sizeof(extInfo.ip));

        extInfo.checkTime = atoi(strCheckTime);     // FIXIT: 이거 3자리 String 맞나???
        
        extInfo.block     = rData->block    - '0';  // FIXIT: check ??? ascii or number ??
        extInfo.tryCount  = rData->tryCount - '0';  // FIXIT: check ??? ascii or number ??
        extInfo.monitor   = rData->monitor  - '0';  // FIXIT: check ??? ascii or number ??
        
        Log.printf(LOG_LV2, "ExtList INSERT Id=[%s] IP=[%s] block=%d checkTime=%d, tryCount=%d\n", extInfo.sswid, extInfo.ip, extInfo.block, extInfo.checkTime, extInfo.tryCount);
        
        if(g_ExtList.insert(&extInfo, false) == false)      // FILE에는 저장하지 않고 나중에 한꺼번에 save_file()
        {
            Log.printf(LOG_LV2, "WEB_ExtSvrListRegRQ() g_ExtList.insert fail.. so skip this line[%d]... continue\n", nCount);
            // LIST 이므로 하나가 에러가 나더라도 계속 진행 
        }

        rData ++;       // NEXT List pointer
    }
    
    if(g_ExtList.save_file() == false) { result = '0'; reason = '9'; }
    SPY_SendResponseToWEB((R_CMD_FROM_WEB *)rPDU, result, reason);
    
    Log.printf(LOG_LV2, "WEB_ExtSvrListRegRQ() END - Current List Count=%d\n",  g_ExtList.size());
    return(0);
}

#ifdef IPV6_MODE
//#define WEB_UPLOAD_EXTLIST_V6_FILENAME     "/home/mcpas/cfg/extlist_v6.cfg"

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : WEB_ExtSvrFileRegRQ_V6
 * CLASS-NAME     : -
 * PARAMETER    IN: rPDU - XBUS로 수신한 외부 서버 List 등록 요청 PDU
 * RET. VALUE     : -
 * DESCRIPTION    : Ext. Server List File INSERT Request
 * REMARKS        : WEB -> DGM -> SPY
 **end*******************************************************/
int WEB_ExtSvrFileRegRQ_V6(DGM_HDR *rPDU)
{
    bool result;
	char c_result[2];
	memset(c_result, 0x00, 2);
	strcpy(c_result, "1");
	Check_Configure_File("/home/mcpas/cfg/extlist_v6.cfg", c_result);

	if( strcmp(c_result, "1") == 0 )
	{
	    g_ExtList_V6.all_alarm_off();                   // 모든 ALARM을 OFF 후 clear
    
	    if((result = g_ExtList_V6.reread_from_WEB(C_WEB_EXTLIST_NAME_V6)) == true)
    	{
        	SPY_SendResponseToWEB((R_CMD_FROM_WEB *)rPDU, '1', '0');
	    }
	    else
	    {
    	    SPY_SendResponseToWEB((R_CMD_FROM_WEB *)rPDU, '0', '1');
	    }
	}
	else
	{
		result = false;
		SPY_SendResponseToWEB((R_CMD_FROM_WEB *)rPDU, '0', c_result[0]);
	}
    
    Log.printf(LOG_INF, "WEB_ExtSvrFileRegRQ_V6() %s Current List Count=%d\n", (result) ? "OK": "Fail", g_ExtList_V6.size());
    return(0);
}
#endif

//#define WEB_UPLOAD_EXTLIST_FILENAME     "/home/mcpas/cfg/extlist.cfg"
/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : WEB_ExtSvrFileRegRQ
 * CLASS-NAME     : -
 * PARAMETER    IN: rPDU - XBUS로 수신한 외부 서버 List 등록 요청 PDU
 * RET. VALUE     : -
 * DESCRIPTION    : Ext. Server List File INSERT Request
 * REMARKS        : WEB -> DGM -> SPY
 **end*******************************************************/
int WEB_ExtSvrFileRegRQ(DGM_HDR *rPDU)
{
    bool result;
	char c_result[2];
    memset(c_result, 0x00, 2);
    strcpy(c_result, "1");
    Check_Configure_File("/home/mcpas/cfg/extlist.cfg", c_result);

    if( strcmp(c_result, "1") == 0 )
    {
	    g_ExtList.all_alarm_off();                   // 모든 ALARM을 OFF 후 clear
    
	    if((result = g_ExtList.reread_from_WEB(C_WEB_EXTLIST_NAME)) == true)
    	{
        	SPY_SendResponseToWEB((R_CMD_FROM_WEB *)rPDU, '1', '0');
	    }
    	else
	    {
    	    SPY_SendResponseToWEB((R_CMD_FROM_WEB *)rPDU, '0', '1');
	    }
	}
	else
	{
		result = false;
		SPY_SendResponseToWEB((R_CMD_FROM_WEB *)rPDU, '0', c_result[0]);
	}
    
    Log.printf(LOG_INF, "WEB_ExtSvrFileRegRQ() %s Current List Count=%d\n", (result) ? "OK": "Fail", g_ExtList.size());
    return(0);
}

#pragma mark -
#pragma mark WEB(DGM 경유)에서 온 BLACK LIST UPDATE/ACTIVE 처리 함수

#ifdef IPV6_MODE
/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : WEB_BlackListUpdateRQ_V6
 * CLASS-NAME     :
 * PARAMETER    IN: rPDU - XBUS로 수신한 BLACK List Update 요청 PDU
 * RET. VALUE     : -
 * DESCRIPTION    : Black List Update Request
 * REMARKS        : Web에서 요청(DGM 경유)
 *                : Add 20140918 by SMCHO
 **end*******************************************************/
int WEB_BlackListUpdateRQ_V6(DGM_HDR *rPDU)
{
    Log.printf(LOG_INF, "IPv6 Black List Update Request From Web[%d]\n", rPDU->relay_MsgId);
    
    switch(g_BlackList_V6.read_again())
    {
        case BLIST_OK:
            if(g_BlackList_V6.size() == 0)
            {
                Log.printf(LOG_ERR, "Black List Update Success but No IP List\n");
                SPY_SendResponseToWEB((R_CMD_FROM_WEB *)rPDU, '0', '0'+BLIST_IP_NOT_EXIST);
                return(0);
            }
            break;
        case BLIST_FILEOPEN_ERR:
            SPY_SendResponseToWEB((R_CMD_FROM_WEB *)rPDU, '0', '0'+BLIST_FILEOPEN_ERR);
            return(0);
            
        case BLIST_FILENAME_ERR:
            SPY_SendResponseToWEB((R_CMD_FROM_WEB *)rPDU, '0', '0'+BLIST_FILENAME_ERR);
            return(0);
            
        default:
            SPY_SendResponseToWEB((R_CMD_FROM_WEB *)rPDU, '0', '0'+BLIST_UNDEFINED_ERR);
            return(0);
    }
    
    SPY_SendResponseToWEB((R_CMD_FROM_WEB *)rPDU, '1', '0');
    return(0);
}
#endif

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : WEB_BlackListUpdateRQ
 * CLASS-NAME     :
 * PARAMETER    IN: rPDU - XBUS로 수신한 BLACK List Update 요청 PDU
 * RET. VALUE     : -
 * DESCRIPTION    : Black List Update Request
 * REMARKS        : Web에서 요청(DGM 경유)
 *                : Add 20140918 by SMCHO
 **end*******************************************************/
int WEB_BlackListUpdateRQ(DGM_HDR *rPDU)
{
    Log.printf(LOG_INF, "Bkack List Update Request From Web[%d]\n", rPDU->relay_MsgId);
    
    switch(g_BlackList.read_again())
    {
        case BLIST_OK:
            if(g_BlackList.size() == 0)
            {
                Log.printf(LOG_ERR, "Black List Update Success but No IP List\n");
                SPY_SendResponseToWEB((R_CMD_FROM_WEB *)rPDU, '0', '0'+BLIST_IP_NOT_EXIST);
                return(0);
            }
            break;
        case BLIST_FILEOPEN_ERR:
            SPY_SendResponseToWEB((R_CMD_FROM_WEB *)rPDU, '0', '0'+BLIST_FILEOPEN_ERR);
            return(0);

        case BLIST_FILENAME_ERR:
            SPY_SendResponseToWEB((R_CMD_FROM_WEB *)rPDU, '0', '0'+BLIST_FILENAME_ERR);
            return(0);

        default:
            SPY_SendResponseToWEB((R_CMD_FROM_WEB *)rPDU, '0', '0'+BLIST_UNDEFINED_ERR);
            return(0);
    }
    
    SPY_SendResponseToWEB((R_CMD_FROM_WEB *)rPDU, '1', '0');
    return(0);
}

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : WEB_BlackListActivateRQ
 * CLASS-NAME     :
 * PARAMETER      : IN
 * RET. VALUE     : -
 * DESCRIPTION    : Black List Activate Request
 * REMARKS        : Web에서 요청(DGM 경유)
 *                : Add 20140918 by SMCHO
 *                : black list는 extlist와는 달리 개별 update가 없다.
 *                : 그래서 spy에서 write를 하지 않기 때문에 파일 권한이 바뀔일이 없다.
 *                : 그러므로 web에서 지속적인 update가 가능하고 extlist처럼 move해서 처리할 필요가 없다.
 **end*******************************************************/
int WEB_BlackListActivateRQ(DGM_HDR *rPDU)
{
    BLACKLIST_ACT_REQ   *rcv = (BLACKLIST_ACT_REQ *)rPDU;
    bool    need_save = false;
    
    Log.printf(LOG_INF, "Bkack List Activate Request From Web[%d]\n", rPDU->relay_MsgId);

    
    if(rcv->act == '1') { if(C_BLACK_LIST_USE == false) { C_BLACK_LIST_USE = true;  need_save = true; } }
    else                { if(C_BLACK_LIST_USE == true)  { C_BLACK_LIST_USE = false; need_save = true; } }
    
    SPY_SendResponseToWEB((R_CMD_FROM_WEB *)rPDU, '1', '0'+C_BLACK_LIST_USE);

    cfg.SetBool("ETC", "BLACK_LIST_USE", C_BLACK_LIST_USE);
    
    if(cfg.SaveFile()) { Log.printf(LOG_INF, "WEB_BlackListActivateRQ() BLACK_LIST_USE = %d cfg.SaveFile() SAVE OK !!\n",   C_BLACK_LIST_USE); }
    else               { Log.printf(LOG_ERR, "WEB_BlackListActivateRQ() BLACK_LIST_USE = %d cfg.SaveFile() SAVE FAIL !!\n", C_BLACK_LIST_USE); }
    
    return(0);
}


#pragma mark -
#pragma mark M-BUS callback - TRACE Request form WEB fuctions


/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : WEB_TraceListRQ
 * CLASS-NAME     :
 * PARAMETER    IN: rPDU - XBUS로 수신한 외부 서버 등록/변경/삭제 요청 PDU
 * RET. VALUE     : -
 * DESCRIPTION    : Trace List 정보를 Web(DGM 경유)으로 전송
 * REMARKS        :
 **end*******************************************************/
int WEB_TraceListRQ(DGM_HDR *rPDU)
{
    R_CMD_FROM_WEB  *recv = (R_CMD_FROM_WEB *)rPDU;
    S_TRACE_LIST_RP send;
	int     nIndex;
    
	bzero(&send,   sizeof(S_TRACE_LIST_RP));
    
	send.hdr     = recv->hdr;          // copy header
	send.hdr.len = sizeof(S_TRACE_LIST_RP);
    
    nIndex  = sizeof(DGM_HDR);
    nIndex += g_Trace.sprint_trace(send.data);

    msendsig(nIndex, DGM, MSG_ID_DGM_RELAY, (uint8_t *)&send);
    
    Log.printf(LOG_INF, "%s() result(%d)=%s\n", __FUNCTION__, nIndex, send.data);

	return(0);
}

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : WEB_TraceAddRQ
 * CLASS-NAME     :
 * PARAMETER    IN: rPDU - XBUS로 수신한 외부 서버 등록/변경/삭제 요청 PDU
 * RET. VALUE     : -
 * DESCRIPTION    : Trace 정보를 TABLE에 추가 하는 함수
 * REMARKS        :
 **end*******************************************************/
int WEB_TraceAddRQ(DGM_HDR *rPDU)
{
	R_TRACE_ADD_RQ  *recv = (R_TRACE_ADD_RQ *)rPDU;
    TRACE_LIST      trace;
    char    strMax[5];
    int     nIndex;
    
    bzero(&trace,  sizeof(trace));
    
    strncpy(trace.strFrom,   recv->from, sizeof(trace.strFrom)-1);
    strncpy(trace.strTo,     recv->to,   sizeof(trace.strTo)-1);
    strncpy(trace.strSvcKey, recv->svc,  sizeof(trace.strSvcKey)-1);
    strncpy(strMax,          recv->max,  sizeof(strMax)-1);
    
	// Web에서 NULL대신 SPACE로 채워서 보냄
    LIB_change_space_to_null(trace.strFrom,   sizeof(trace.strFrom));
    LIB_change_space_to_null(trace.strTo,     sizeof(trace.strTo));
    LIB_change_space_to_null(trace.strSvcKey, sizeof(trace.strSvcKey));
    LIB_change_space_to_null(strMax,          sizeof(strMax));
    trace.nMaxCount = atoi(strMax);
    
    if(trace.strFrom[0]   == '\0') { strcpy(trace.strFrom,   "-"); }
    if(trace.strTo[0]     == '\0') { strcpy(trace.strTo,     "-"); }
    if(trace.strSvcKey[0] == '\0') { strcpy(trace.strSvcKey, "-"); }
    
    g_Trace.insert(&trace, &nIndex);
    
	return(0);
}

/* Procedure Header
 **pdh*******************************************************
 * PROCEDURE-NAME : WEB_TraceDelRQ
 * CLASS-NAME     :
 * PARAMETER    IN: rPDU - XBUS로 수신한 외부 서버 등록/변경/삭제 요청 PDU
 * RET. VALUE     : -
 * DESCRIPTION    : Trace 정보를 TABLE에 삭제 하는 함수
 * REMARKS        :
 **end*******************************************************/
int WEB_TraceDelRQ(DGM_HDR *rPDU)
{
	R_TRACE_DEL_RQ  *recv = (R_TRACE_DEL_RQ *)rPDU;
	char    strIdx[3];
    
    bzero(strIdx,  sizeof(strIdx));
    strncpy(strIdx, recv->index, sizeof(strIdx)-1);
    
    g_Trace.erase(atoi(strIdx));
    
	return(0);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MBUS_MsgFromDGM
 * CLASS-NAME     :
 * PARAMETER    IN: len     - 수신한 메세지 길이
 *              IN: msg_ptr - 수신한 메세지 포인터
 * RET. VALUE     : -
 * DESCRIPTION    : DGM에서 수신받은 XBUS 메세지를 처리하는 함수
 * REMARKS        : WEB->DGM->SPY
 **end*******************************************************/
int MBUS_MsgFromDGM(int len, XBUS_MSG *xbus_msg)
{
    DGM_HDR *rPDU = (DGM_HDR*)xbus_msg->Data;

#ifdef DEBUG_MODE
    Log.printf(LOG_LV1, "%s() MsdId = %d(0x%x)\n", __FUNCTION__, rPDU->relay_MsgId, rPDU->relay_MsgId);
#endif
    switch(rPDU->relay_MsgId)
    {
        case WCMD_SSW_REG:          WEB_ExtSvrRegRQ(rPDU);      return(0);
        case WCMD_SSWLIST_REG:      WEB_ExtSvrListRegRQ(rPDU);  return(0);
        case WCMD_SSWFILE_REG:      WEB_ExtSvrFileRegRQ(rPDU);  return(0);
            
        case WCMD_AS_BLOCK:         WEB_AsBlockRQ(rPDU);        return(0);
            
		case WCMD_TRACE_LIST_REQ:                         WEB_TraceListRQ(rPDU); return(0);
		case WCMD_TRACE_ADD_REQ:    WEB_TraceAddRQ(rPDU); WEB_TraceListRQ(rPDU); return(0);
		case WCMD_TRACE_DEL_REQ:    WEB_TraceDelRQ(rPDU); WEB_TraceListRQ(rPDU); return(0);

        case WCMD_BLACKLIST_REG:    WEB_BlackListUpdateRQ(rPDU);    return(0);
        case WCMD_BLACKLIST_ACT:    WEB_BlackListActivateRQ(rPDU);  return(0);
        
#ifdef IPV6_MODE
        case WCMD_BLACKLIST_REG_V6:      WEB_BlackListUpdateRQ_V6(rPDU);    return(0);
        case WCMD_SSW_REG_V6:            WEB_ExtSvrRegRQ_V6(rPDU);          return(0);
        case WCMD_SSWLIST_REG_V6:        WEB_ExtSvrListRegRQ_V6(rPDU);      return(0);
        case WCMD_SSWFILE_REG_V6:        WEB_ExtSvrFileRegRQ_V6(rPDU);      return(0);
#endif
        default: SPY_SendResponseToWEB((R_CMD_FROM_WEB *)rPDU, '0', '8'); return(0);     // ERROR RP

    }
    return(0);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : MBUS_callback
 * CLASS-NAME     :
 * PARAMETER    IN: len     - 수신한 메세지 길이
 *              IN: msg_ptr - 수신한 메세지 포인터
 * RET. VALUE     : -
 * DESCRIPTION    : XBUS 메세지를 처리하는 함수
 * REMARKS        : 내부 m-bus 콜백 처리 함수
 **end*******************************************************/
int MBUS_callback(int len, uint8_t *msg_ptr)
{
	XBUS_MSG        *xbus_msg = (XBUS_MSG*)msg_ptr;

#ifdef DEBUG_MODE
    Log.printf(LOG_LV1, "%s() XBUS callback from [%s] MsgId = 0x%x\n", __FUNCTION__, get_module_name(xbus_msg->hdr.From), xbus_msg->hdr.MsgId);
#endif
	switch(xbus_msg->hdr.MsgId)
	{
		case MSG_ID_SCM:        MBUS_MsgFromSCM(len, xbus_msg); return(0);  // SES 채널
#if 1
        case MSG_ID_MMC:        MBUS_MsgFromMGM2(len, xbus_msg); return(0);  // MMC
#else   // OLD MMC
		case MSG_ID_MMC:        MBUS_MsgFromMGM(len, xbus_msg); return(0);  // MMC
#endif
		case MSG_ID_STA:        MBUS_MsgFromSGM(len, xbus_msg); return(0);  // 통계
//		case MSG_ID_FROM_DBM:                                   return(0);
		case MSG_ID_DGM_RELAY:  MBUS_MsgFromDGM(len, xbus_msg); return(0);  // WEB
	}
    
	return(0);
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SUBS_callback
 * CLASS-NAME     :
 * PARAMETER    IN: len   - 수신한 메세지 길이
 *              IN: r_msg - 수신한 메세지 포인터
 * RET. VALUE     : -
 * DESCRIPTION    : S-BUS 메세지 수신 처리 함수
 * REMARKS        : XBUS Callback 함수
 **end*******************************************************/
int SUBS_callback(int len, uint8_t *r_msg)
{
	XBUS_MSG    *rXBUS = (XBUS_MSG *)r_msg;

    switch (rXBUS->hdr.MsgId)
    {
        case SCMD_HASH_INSERT_RQ:    HASH_InsertTableRQ(rXBUS); break;   // HashTable Insert 요청 Active -> StandBy
        case SCMD_HASH_SYNC_RQ:      HASH_SyncAllTableRQ();     break;   // HashTable Sync 요청 Standby -> Active (모든 HashTable Sync 요청)
        case SCMD_HASH_HA_UPDATE_RQ: HASH_HaUpdateRQ(rXBUS);    break;   // HashTable Update 요청
        default: Log.printf(LOG_ERR, "UNDEFINED CMD=%d\n", rXBUS->hdr.MsgId); break;
    }
	return(0);
}


#pragma mark -
#pragma mark X-BUS HA functions

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : haUnassign
 * CLASS-NAME     :
 * PARAMETER    IN: arg - 사용 안함
 * RET. VALUE     : -
 * DESCRIPTION    : My XBUS State Change to UNASSIGN
 * REMARKS        : XBUS Callback Function
 **end*******************************************************/
void haUnassign(int arg)
{
    Log.color(COLOR_RED, LOG_INF, "MY HA State Change [%s -> Unassign]\n", (g_nHA_State == HA_Standby) ? "Standby" : "Active");
    
	if     (g_nHA_State == HA_Active)  
	{ 
		Log.printf(LOG_INF, "HA State [Active  -> Unassign]\n"); 

		// 20250714 bible : 절체 시 알람 초기화
		g_ExtList.all_alarm_off();
#ifdef IPV6_MODE
		g_ExtList_V6.all_alarm_off();
#endif       
#ifdef INCLUDE_REGI
		g_bReqQ_Alarm = false;	// Q-Alarm 초기화
#endif
        //
	}
	else if(g_nHA_State == HA_Standby) { Log.printf(LOG_INF, "HA State [Standby -> Unassign]\n"); }
	else                               { Log.printf(LOG_INF, "HA State [%d      -> Unassign]\n", g_nHA_State); }
    
	g_nHA_State = HA_UnAssign;
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : haActive
 * CLASS-NAME     :
 * PARAMETER    IN: arg - 사용 안함
 * RET. VALUE     : -
 * DESCRIPTION    : My XBUS State Change to ACTIVE
 * REMARKS        : XBUS Callback Function
 **end*******************************************************/
void haActive(int arg)
{
    Log.color(COLOR_RED, LOG_INF, "MY HA State Change [%s -> Active]\n", (g_nHA_State == HA_Standby) ? "Standby" : "UnAssign");

	g_nHA_State = HA_Active;
    
    // initialize global variabes
    g_nSamServerResetCount               = 0;
    g_nSswServerResetCount[OT_TYPE_ORIG] = 0;
    g_nSswServerResetCount[OT_TYPE_TERM] = 0;
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : request_sync
 * CLASS-NAME     :
 * PARAMETER    IN: arg - 사용 안함
 * RET. VALUE     : -
 * DESCRIPTION    :이중화 데이터 Sync 요청을 STANDBY로 전송하는 함수
 * REMARKS        :
 **end*******************************************************/
void *request_sync(void* arg)
{
    sleep(3);       // 너무 빨리 동기화 데이터가 오면 빠질 가능성이 있어서 3초 delay (실제적으로는 빠질 가능성은 없지만 delay 진행)
    
    g_Hash_O.clear();       // SYNC 요청전에 가지고 있던 값은 모두 삭제
    g_Hash_T.clear();       // SYNC 요청전에 가지고 있던 값은 모두 삭제
    Log.color(COLOR_YELLOW, LOG_INF, "%s() SEND REQUST MAP SYNC to ACTIVE\n", __FUNCTION__);
    ssendsig(0, SCMD_HASH_SYNC_RQ, NULL);
    
    sleep(1);

    return (NULL);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : haStandby
 * CLASS-NAME     :
 * PARAMETER    IN: arg - 사용 안함
 * RET. VALUE     : -
 * DESCRIPTION    : My XBUS State Change to STANDBY
 * REMARKS        : XBUS Callback Function
 **end*******************************************************/
void haStandby(int arg)
{
    pthread_t   tid;
    
    Log.color(COLOR_RED, LOG_INF, "MY HA State Change [%s -> Standby]\n", (g_nHA_State == HA_Active) ? "Active" : "UnAssign");

	// 20250714 bible : 절체 시 알람 초기화
	g_ExtList.all_alarm_off();
#ifdef IPV6_MODE
	g_ExtList_V6.all_alarm_off();
#endif       
#ifdef INCLUDE_REGI
	g_bReqQ_Alarm = false;  // Q-Alarm 초기화
#endif  
    //
   
    // 이전 상태가 Active가 아니였으면 Hash Table Sync 요청
	if (g_nHA_State != HA_Active)
    {
        pthread_create(&tid, NULL, request_sync, NULL);
    }
    
	g_nHA_State = HA_Standby;
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : ProessDownInSES
 * CLASS-NAME     :
 * PARAMETER    IN: moId     - XBUS Module ID
 *              IN: nBoardNo - SES Board Number
 * RET. VALUE     : -
 * DESCRIPTION    : Module State Change Report from XBUS
 * REMARKS        : XBUS Callback Function
 **end*******************************************************/
void ProessDownInSES(uint8_t moId, int nBoardNo)
{
    int     nCount;
    
    nCount  = g_Hash_O.dead_mark(nBoardNo, MAX_SES_CH);
    nCount += g_Hash_T.dead_mark(nBoardNo, MAX_SES_CH);
    
    Log.printf(LOG_INF, "SES[%d]-%s Down - Call (%d) dead...\n", nBoardNo, get_module_name(moId), nCount);
}

#ifdef SES_HA_MODE
#ifdef SES_ACTSTD_MODE /* 20241126 kdh add */
	/* SES_ACTSTD_MODE */
#else
typedef struct
{
    uint8_t     down_board;
    uint8_t     down_moId;
} S_SES_HA_START;

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SES_HA_ProcessDown
 * CLASS-NAME     :
 * PARAMETER    IN: moId     - XBUS Module ID
 *              IN: nBoardNo - SES Board Number
 * RET. VALUE     : BOOL
 * DESCRIPTION    : Module State Change Report from XBUS
 * REMARKS        : XBUS Callback Function
 **end*******************************************************/
bool SES_HA_ProcessDown(uint8_t moId, int nBoardNo)
{
    int             nCount = 0;
    int             nTwice = 0;
    int32_t         nPeerBoardNo;
    S_SES_HA_START  send;
    
    Log.color(COLOR_MAGENTA, LOG_INF, "SES[%d]-%s Down - SES_HA_START \n", nBoardNo, get_module_name(moId));
    
    switch(nBoardNo)
    {
        case 0:     // peer = 1
        case 2:     // peer = 3
        case 4:     // peer = 5
        case 6:     // peer = 7         // 160831 by SMCHO (Max SES 6 -> 10)
        case 8:     // peer = 9         // 160831 by SMCHO (Max SES 6 -> 10)
            nPeerBoardNo = nBoardNo + 1;
            break;
        case 1:     // peer = 0
        case 3:     // peer = 2
        case 5:     // peer = 4
        case 7:     // peer = 6         // 160831 by SMCHO (Max SES 6 -> 10)
        case 9:     // peer = 8         // 160831 by SMCHO (Max SES 6 -> 10)
            nPeerBoardNo = nBoardNo - 1;
            break;
            
        default:
            Log.printf(LOG_INF, "%s() undefined SES board(%d)'s module(%s) down \n", __FUNCTION__, nBoardNo, get_module_name(moId));
            return(false);
    }
    
    if(g_nHA_State == HA_Active)    // ACTIVE만 CMS/SAM/SRM으로 START 메시지 전송
    {
        send.down_board = nBoardNo;
        send.down_moId  = moId;
        
        msendsig(sizeof(S_SES_HA_START), SCM,                 MSG_ID_SES_HA_START, (uint8_t *)&send);   // send to SCM_A & SCM_B
        msendsig(sizeof(S_SES_HA_START), SRM_00+nPeerBoardNo, MSG_ID_SES_HA_START, (uint8_t *)&send);   // send to peer SRM
        msendsig(sizeof(S_SES_HA_START), SAM_00+nPeerBoardNo, MSG_ID_SES_HA_START, (uint8_t *)&send);   // send to peer SAM
        
        // SAM/SRM이 속한 Board로 보낸다 (죽은 놈은 못받고 살아있는 놈은 받아서.. 자신의 데이터  clear)
//        msendsig(sizeof(S_SES_HA_START), SRM_00+nBoardNo, MSG_ID_SES_HA_START, (uint8_t *)&send);   // send to SRM    (SRM 은 안보내도 된다고 함) - 20160606
        msendsig(sizeof(S_SES_HA_START), SAM_00+nBoardNo, MSG_ID_SES_HA_START, (uint8_t *)&send);   // send to SAM
    }
    
    nCount  = g_Hash_O.ha_move_mark(nBoardNo, nPeerBoardNo, MAX_SES_CH, true);    // FIXIT - MAX_SES_CH = ??
    nCount += g_Hash_T.ha_move_mark(nBoardNo, nPeerBoardNo, MAX_SES_CH, false);

    nTwice  = g_Hash_O.check_twice_moved(nBoardNo, nPeerBoardNo, MAX_SES_CH, true);
    Log.color(COLOR_MAGENTA, LOG_INF, "HASH.ha_move_mark(board %d -> %d) count=%d, twice=%d\n", nBoardNo, nPeerBoardNo, nCount, nTwice);

    return(true);
}
#endif /* SES_ACTSTD_MODE */
#endif /* SES_HA_MODE */

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : haModuleStateChange
 * CLASS-NAME     :
 * PARAMETER    IN: moId  - XBUS Module ID
 *              IN: state - 변경된 XBUS State(0-down, 1-alive)
 * RET. VALUE     : -
 * DESCRIPTION    : Module State Change Report from XBUS
 * REMARKS        : XBUS Callback Function
 **end*******************************************************/
void haModuleStateChange(uint8_t moId, int state)
{
	if(moId == DBM_A)       // DBM State Change
	{
		g_DBMAlive[0] = state;
		Log.printf(LOG_INF, "DBM_A state change -> %d...\n", state);
		return;
	}
    
    if(moId == DBM_B)       // DBM State Change
	{
		g_DBMAlive[1] = state;
		Log.printf(LOG_INF, "DBM_B state change -> %d...\n", state);
		return;
	}
    
	if ((moId == COAM_A+MY_SIDE*16) && (state == 0))    // My COAM Down
	{
#ifdef IPV6_MODE
        char    command[256];
        
        sprintf(command, "ifconfig bond0:5 down;ifconfig bond0 inet6 del %s/64", C_ORIG_STACK_IP_V6);
        system(command);
#else
        system("ifconfig bond0:5 down");
//        system("ifconfig bond0:6 down");
#endif
		return;
	}
    
	if(state == 1) { return; }      // 실이난 경우에는 처리하지 않음
    
#ifdef SES_HA_MODE
    // network 절체시 soam down으로 오감지하는 case에 all system down이 발생됨
    // 알단 soam down 감지는 안하는 것으로... 20160608 일산 mPBX BMT
    // 향후 어떻게 처리하는게 좋은지는 시험을 통해서 결정 (SOAM이 죽으면 기존호는 안되나?? SCM SRM SAM 연계확인)

//    if     ((moId >= SOAM_00) && (moId <= SOAM_05)) { SES_HA_ProcessDown(moId, moId-SOAM_00); }
//    else if((moId >= SAM_00)  && (moId <= SAM_05))  { SES_HA_ProcessDown(moId, moId-SAM_00);  }
//    else if((moId >= SRM_00)  && (moId <= SRM_05))  { SES_HA_ProcessDown(moId, moId-SRM_00);  }
   
#ifdef SES_ACTSTD_MODE /* 20241126 kdh add */
	/* */
#else 
    if((moId >= SAM_00)  && (moId <= SAM_09))       { SES_HA_ProcessDown(moId, moId-SAM_00);  }     // 160831 by SMCHO (Max SES 6 -> 10)
    else if((moId >= SRM_00)  && (moId <= SRM_09)) 	{ SES_HA_ProcessDown(moId, moId-SRM_00);  }     // 160831 by SMCHO (Max SES 6 -> 10)
#endif /* SES_ACTSTD_MODE */
#else /* SES_HA_MODE */

    if(g_nHA_State != HA_Active) { return; }        // 예전 버전은 Active만 동작하게 되어 있음
    
    // SES Process Down
	if     ((moId >= SOAM_00) && (moId <= SOAM_09)) { ProessDownInSES(moId, moId-SOAM_00); } 			// 160831 by SMCHO (Max SES 6 -> 10)
	else if((moId >= SAM_00)  && (moId <= SAM_09))  { ProessDownInSES(moId, moId-SAM_00);  }            // 160831 by SMCHO (Max SES 6 -> 10)
	else if((moId >= SRM_00)  && (moId <= SRM_09)) 	{ ProessDownInSES(moId, moId-SRM_00);  }            // 160831 by SMCHO (Max SES 6 -> 10)
#endif /* SES_HA_MODE */
}


/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SPY_init_xbus
 * CLASS-NAME     :
 * PARAMETER      : -
 * RET. VALUE     : BOOL
 * DESCRIPTION    : initialize XBUS (HA CALLBACK, VERSION, ...)
 * REMARKS        :
 **end*******************************************************/
bool SPY_init_xbus(void)
{
    int     ret;
	pthread_t   tid; // 20241202 kdh add
    
    // SET Version Info
	set_module_version(MVER1,MVER2, MVER3, BUILD_DATE, AUTHOR, PKGNAME);
//    set_module_version(nVersion[0], nVersion[1], nVersion[2], strUpdate_Date, strUpdate_Owner, strUpdate_Desc);
    
    // Register callback Functions
	reg_callback(HA_UnAssign,   haUnassign);
	reg_callback(HA_Active,     haActive);
	reg_callback(HA_Standby,    haStandby);
//	reg_callback(HA_AddStandby, haAddStandby);
//	reg_callback(HA_RemoveStandby, haRemoveStandby);
    
    if((ret = init_btxbus(MY_BLK_ID, SUBS_callback, MBUS_callback)) != SUCCESS)
    {
        Log.printf(LOG_ERR, "[SPY_INIT] init_btxbus() fail [%d]!!\n", ret);
        return(false);
    }
    
    if((ret = reg_mod_state_change(haModuleStateChange)) != SUCCESS)
    {
        Log.printf(LOG_ERR, "[SPY_INIT] reg_mod_state_change() fail [%d]!!\n", ret);
        return(false);
    }

    pthread_create(&tid, NULL, HASH_InsertTableThread, NULL); // 20241202 kdh add

    return(true);
}

