//
//  SPY_alm.cpp
//  smSPY
//
//  Created by SMCHO on 2014. 4. 7..
//  Copyright (c) 2014년 SMCHO. All rights reserved.
//

#include "SPY_def.h"
#include "SPY_xref.h"

#pragma mark -
#pragma mark SSW 상태 ALARM을 전송 함수들

#ifndef ONCMS
/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SEND_ExtServerStatusUpdateToDBM_ECS_only
 * CLASS-NAME     :
 * PARAMETER    IN: ot_type    - 발생한 곳이 ORIG인지 TERM인지를 나타냄
 *              IN: alarm_falg - ALARM상태, 0이면 ALARM_OFF/ 1이면 ON 
 *              IN: strIP      - ALARM이 발생된 SSW IP 
 * RET. VALUE     : int
 * DESCRIPTION    : DBM에 SSW 상태를 UPDATE ECS(긴급호)용 
 * REMARKS        : 긴급호에서만 사용 
 **end*******************************************************/
int SEND_ExtServerStatusUpdateToDBM_ECS_only(char ot_type, int alarm_flag, const char *strIP)
{
    time_t  clock;
    struct  tm *mydate;
    int     result;
    S_TO_ALTIBASE send;
    
    bzero(&send, sizeof(send));
    
    send.CODE[0]  = 'S';
    send.threadId = 0;
    send.trigger  = 0;
    
    clock  = time(NULL);
    mydate = localtime(&clock);
    
    snprintf(send.sql, sizeof(send.sql), "exec ECS_SSW_UPDATE_SPY(%d, %d, '%s', '%4d%02d%02d%02d%02d%02d')", MY_AS_ID, alarm_flag, strIP,
            mydate->tm_year+1900, mydate->tm_mon+1, mydate->tm_mday, mydate->tm_hour, mydate->tm_min, mydate->tm_sec);
    
    if((result = msendsig(sizeof(send), DBM, MSG_ID_DBM, (uint8_t *)&send)) != 0)
        Log.printf(LOG_ERR, "SEND_ExtServerStatusUpdateToDBM_ECS_only(%d) Fail - Data=%s\n", result, send.sql);
    else
        Log.printf(LOG_LV1, "SEND_ExtServerStatusUpdateToDBM_ECS_only(0) OK - Data=%s\n", send.sql);
    
    return(result);
}

#endif

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SEND_ExtServerStatusAlarm
 * CLASS-NAME     :
 * PARAMETER    IN: strSswID   - ALARM이 발생한 외부 연동 서버 ID 
 #              IN: strIP      - ALARM이 발생한 외부 연동 서버 IP 
 *              IN: alarm_falg - ALARM상태, 0이면 ALARM_OFF/ 1이면 ON 
 *              IN: time_down  - ALARM이 발생된 시간  (time_t) 
 *              IN: ot_type    - 발생한 곳이 ORIG인지 TERM인지를 나타냄
 * RET. VALUE     : int
 * DESCRIPTION    : DBM에 SSW 상태를 UPDATE ECS(긴급호)용
 * REMARKS        : 긴급호에서만 사용
 **end*******************************************************/
void SEND_ExtServerStatusAlarm(char *strSswID, char *strIP, int alarm_onoff, time_t time_down, int ot_type)
{
//    int     result;
#ifdef OAMSV5_MODE
	int		nIndex;
	char	szCommentV5[8][64];

	nIndex = atoi(strSswID);
	memset(szCommentV5, 0x00, sizeof(szCommentV5));
	sprintf(szCommentV5[0], "%d", nIndex);
	if (alarm_onoff == -1)
	{
		sprintf(szCommentV5[1], "Alarm initialize");
		alarm_onoff = ALARM_OFF;
	}
	else
		sprintf(szCommentV5[1], "%s", (alarm_onoff == ALARM_ON) ? "Fail" : "Succ");

	SEND_AlarmV2(MSG_ID_ALM, ALM_SPY_SSW_DISCONNECT, MY_BLK_ID, ALM_SPY_SSW_DISCONNECT, nIndex, alarm_onoff, 2, szCommentV5);
	
	Log.printf(LOG_INF, "EXT. Server Status Chanage: ID=%d, CSCF (%s) %s\n", nIndex, strIP, (alarm_onoff == ALARM_ON) ? "Fail" : "Succ"); 	
#else
    char    extend[8], comment[64];
    
    memcpy(&extend[0], &time_down, 4);
    memcpy(&extend[4], strSswID,   3);
    extend[7] = '\0';
    
    // 170112: comment 문구 변경 "SoftSwitch" -> "CSCF"
    snprintf(comment, sizeof(comment), "CSCF (%s) : %s", strIP, (alarm_onoff == ALARM_ON) ? "DOWN": "UP");
    SEND_AlarmMessage(ALM_SPY_SSW_DISCONNECT, alarm_onoff, ALM_SPY_SSW_DISCONNECT, extend, comment);
    
    Log.printf(LOG_INF, "EXT. Server Status Chanage: ID=%s Comment=%s\n", strSswID, comment);
#endif
    
#ifdef ECS_MODE     // 긴급호는 ONCMS가 아님 - 공통DB 연동 없이 자체 DB(DBS)만 연동하는 구조 임
    if (C_ECS_UPDATE)
    {
//        result = SEND_ExtServerStatusUpdateToDBM_ECS_only(ot_type, alarm_onoff, strIP);
        SEND_ExtServerStatusUpdateToDBM_ECS_only(ot_type, alarm_onoff, strIP);
    }
#endif  // ECS_MODE
}


#pragma mark -
#pragma mark ALARM/FAULT를 보내는 함수

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SEND_AlarmMessage
 * CLASS-NAME     :
 * PARAMETER    IN: cmd_no   - 발생한 ALARM 종류
 *              IN: flag     - ALARM 상태 (ON/OFF)
 *              IN: alarm_id - 발생된 ALARM의 ID
 *              IN: extend   - ALARM에 대한 추가 정보
 *              IN: comment  - ALARM에 대한 comment
 * RET. VALUE     : xbus_t
 * DESCRIPTION    : ALARM 메세지를 MGM으로 전송하는 함수
 * REMARKS        :
 **end*******************************************************/
int SEND_AlarmMessage(int cmd_no, int flag, int alarm_id, const char *extend, const char *comment)
{
    ALM_MSG     alarm;
    
    bzero(&alarm, sizeof(alarm));
    
    alarm.header.Len    = sizeof(alarm);
    alarm.header.MsgID  = MSG_ID_ALM;
    alarm.header.CmdNo  = cmd_no;
    alarm.header.From   = MY_BLK_ID;
    alarm.header.Time   = (int)time(NULL);
    alarm.header.To     = MGM;
    alarm.header.Type   = MMC_TYPE_END;
    
    alarm.CaseNo        = flag;
    alarm.MoId          = MY_BLK_ID;
    alarm.alarmFlag     = flag;
    alarm.alarmId       = alarm_id;
    
    memcpy((char *)alarm.Extend,   extend,  8);
    strncpy((char *)alarm.comment, comment, 63);
    
    return(msendsig(sizeof(alarm), MGM, MSG_ID_ALM, (uint8_t *)&alarm));
}

#ifdef OAMSV5_MODE
int SEND_AlarmV2(int type, int cmd_no, int xbusId, int alarmId, int indexId, int alarmFlag, int commentCnt, const char comments[][64])
{
    ALM_MSG_V2   alarm;

    bzero(&alarm, sizeof(alarm));

    alarm.header.Len    = sizeof(alarm);
    alarm.header.MsgID  = type;    //MSG_ID_ALM/MSG_ID_FLT
    alarm.header.CmdNo  = cmd_no;
    alarm.header.From   = xbusId;
    alarm.header.Time   = (int)time(NULL);
    alarm.header.To     = MGM;
    alarm.header.Type   = MMC_TYPE_END;

    alarm.alarmId       = alarmId;
    alarm.indexId       = indexId;
    alarm.alarmFlag     = alarmFlag;
    alarm.commentCnt    = commentCnt;

    for (int i = 0; i < commentCnt && i < 8; ++i) {
        if (comments[i] != NULL) {
            strncpy(alarm.comment[i], comments[i], sizeof(alarm.comment[i]) - 1);
            alarm.comment[i][sizeof(alarm.comment[i]) - 1] = '\0';
        } else {
            alarm.comment[i][0] = '\0';
        }
    }

    return(msendsig(sizeof(alarm), MGM, type, (uint8_t *)&alarm));
}
#endif

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SEND_FaultMessage
 * CLASS-NAME     :
 * PARAMETER    IN: cmd_no   - 발생한 FAULT 종류
 *              IN: flag     - ALARM 상태 (ON/OFF)
 *              IN: alarm_id - FIXIT: 0로 FIX해서 사용 (GODRM)
 *              IN: extend   - FAULT 에 대한 추가 정보
 *              IN: comment  - FAULT 에 대한 comment
 * RET. VALUE     : xbus_t
 * DESCRIPTION    : FAULT 메세지를 MGM으로 전송하는 함수
 * REMARKS        :
 **end*******************************************************/
int SEND_FaultMessage(int cmd_no, int flag, int alarm_id, char extend, const char *comment)
{
    ALM_MSG     alarm;
    
    Log.printf(LOG_ERR, "SEND_FaultMessage: flag=%s, alarmId=%d, comment=%s\n", (flag == ALARM_ON) ? "ON":"OFF", alarm_id, comment);
    
    bzero(&alarm, sizeof(alarm));
    
    alarm.header.Len    = sizeof(alarm);
    alarm.header.MsgID  = MSG_ID_FLT;
    alarm.header.CmdNo  = cmd_no;
    alarm.header.From   = MY_BLK_ID;
    alarm.header.Time   = (int)time(NULL);
    alarm.header.To     = MGM;
    alarm.header.Type   = MMC_TYPE_END;
    
    alarm.CaseNo        = 0;
    alarm.MoId          = MY_BLK_ID;
    alarm.alarmFlag     = flag;
    alarm.alarmId       = alarm_id;
    
    alarm.Extend[0]     = extend;
    
    strncpy((char *)alarm.comment, comment, 63);
    
    return(msendsig(sizeof(alarm), MGM, MSG_ID_ALM, (uint8_t *)&alarm));    // FIXIT: - 이거 FLT로 보내야 하는거 아닌가?? GODRM 소스부터 ALM으로 보냄?? 
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SEND_StatusMessage
 * CLASS-NAME     :
 * PARAMETER    IN: cmd_no   - STATUS 메시지 종류
 *              IN: comment  - STATUS 에 대한 comment
 * RET. VALUE     : xbus_t
 * DESCRIPTION    : STATUS 메세지를 MGM으로 전송하는 함수
 * REMARKS        :
 **end*******************************************************/
int SEND_StatusMessage(uint16_t cmd_no, const char *comment)
{
    StsMsgRP_S     status;
    
    Log.printf(LOG_LV3, "SEND_StatusMessage: cmd_no=%d, comment=%s\n", cmd_no, comment);
    
    bzero(&status, sizeof(status));

    
    status.header.Len    = sizeof(StsMsgRP_S);
    status.header.MsgID  = MSG_ID_STS;
    status.header.CmdNo  = cmd_no;
    status.header.From   = MY_BLK_ID;
    status.header.Time   = (uint32_t)time(NULL);
    status.header.To     = 0;
    status.header.Type   = 0;

    
    status.CaseNo        = 0;
    status.MoId          = MY_BLK_ID;

    
    strncpy((char *)status.comment, comment, 63);
    
    return(msendsig(sizeof(status), MGM, MSG_ID_STS, (uint8_t *)&status));
}


#pragma mark -
#pragma mark AS BLOCK ALARM 을 보내는 함수

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SEND_AsBlockAlarm
 * CLASS-NAME     :
 * PARAMETER      : -
 * RET. VALUE     : 0
 * DESCRIPTION    : AS BLOCK 상태 ALATM을 보내는 함수
 * REMARKS        :
 **end*******************************************************/
int SEND_AsBlockAlarm(void)
{
#ifdef OAMSV5_MODE
    char    szCommentV5[8][64];
    
    memset(szCommentV5, 0x00, sizeof(szCommentV5));
    sprintf(szCommentV5[0], "%s", C_AS_BLOCK ? "Block" : "Unblock");
    
    SEND_AlarmV2(MSG_ID_ALM, ALM_SPY_AS_BLOCK, MY_BLK_ID, ALM_SPY_AS_BLOCK, 0, C_AS_BLOCK, 1, szCommentV5);
    
    Log.printf(LOG_INF, "AS-%d %s !!!\n", MY_AS_ID, C_AS_BLOCK ? "Block" : "Unblock");
#else
    char    extend[8];
    char    comment[64];
    
    bzero(extend,  sizeof(extend));
    bzero(comment, sizeof(comment));

    extend[0]  = (C_AS_BLOCK) ? 4 : 0;

    SEND_AlarmMessage(ALM_SPY_AS_BLOCK, C_AS_BLOCK, ALM_SPY_AS_BLOCK, extend, comment);
#endif
    
    return(0);
}


#pragma mark -
#pragma mark REGISTER QUEUE ALARM 을 보내는 함수

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SEND_Reg_Q_Alarm
 * CLASS-NAME     :
 * PARAMETER    IN: alarm_falg - ALARM상태, 0이면 ALARM_OFF/ 1이면 ON 
 * RET. VALUE     : -
 * DESCRIPTION    : REGISTER QUEUE 상태 ALATM을 보내는 함수
 * REMARKS        : add 170303
 **end*******************************************************/
 void SEND_Reg_Q_Alarm(char alarm_onoff)
{
#ifdef OAMSV5_MODE
    char    szCommentV5[8][64];
    
    memset(szCommentV5, 0x00, sizeof(szCommentV5));
    sprintf(szCommentV5[0], "%s", (alarm_onoff == ALARM_ON) ? "OCCUR" : "Clear");
    
    SEND_AlarmV2(MSG_ID_ALM, ALM_REG_Q_FULL_ALARM, MY_BLK_ID, ALM_REG_Q_FULL_ALARM, 0, alarm_onoff, 1, szCommentV5);
    
    Log.printf(LOG_INF, "REGISTER QUEUE FULL ALARM : %s\n", (alarm_onoff == ALARM_ON) ? "OCCUR" : "Clear");
#else
    char    extend[8];
    char    comment[64];
    
    bzero(extend,  sizeof(extend));
    bzero(comment, sizeof(comment));
    
    
    snprintf(comment, sizeof(comment), "REGISTER QUEUE FULL ALARM: %s", (alarm_onoff == ALARM_ON) ? "ON": "OFF");
    SEND_AlarmMessage(ALM_REG_Q_FULL_ALARM, alarm_onoff, ALM_REG_Q_FULL_ALARM, extend, comment);
    
    Log.printf(LOG_INF, "REG. Q Status Chanage: %s\n", comment);
#endif
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SEND_Reg_Q_Alarm2
 * CLASS-NAME     :
 * PARAMETER    IN: alarm_falg - ALARM상태, 0이면 ALARM_OFF/ 1이면 ON 
 * RET. VALUE     : -
 * DESCRIPTION    : REGISTER QUEUE 상태 ALATM을 보내는 함수
 * REMARKS        : 주기적으로 보내기 때문에 Log Level을 1으로
 **end*******************************************************/
void SEND_Reg_Q_Alarm2(char alarm_onoff)
{
#ifdef OAMSV5_MODE
    char    szCommentV5[8][64];
    
    memset(szCommentV5, 0x00, sizeof(szCommentV5));
    sprintf(szCommentV5[0], "%s", (alarm_onoff == ALARM_ON) ? "OCCUR" : "Clear");
    
    SEND_AlarmV2(MSG_ID_ALM, ALM_REG_Q_FULL_ALARM, MY_BLK_ID, ALM_REG_Q_FULL_ALARM, 0, alarm_onoff, 1, szCommentV5);
    
    Log.printf(LOG_LV1, "REGISTER QUEUE FULL ALARM : %s\n", (alarm_onoff == ALARM_ON) ? "OCCUR" : "Clear");
#else
    char    extend[8];
    char    comment[64];
    
    bzero(extend,  sizeof(extend));
    bzero(comment, sizeof(comment));
    
    
    snprintf(comment, sizeof(comment), "REGISTER QUEUE FULL ALARM: %s", (alarm_onoff == ALARM_ON) ? "ON": "OFF");
    SEND_AlarmMessage(ALM_REG_Q_FULL_ALARM, alarm_onoff, ALM_REG_Q_FULL_ALARM, extend, comment);
    
    Log.printf(LOG_LV1, "[REG] %s\n", comment);
#endif
}


#pragma mark -
#pragma mark SPY와 연관된 ALARM 을 주기적으로 보내는 함수

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : RSSW_DequeueThread_TERM
 * CLASS-NAME     : -
 * PARAMETER    IN: arg - unused
 * RET. VALUE     : NULL
 * DESCRIPTION    : 와 연관된 ALATM을 주기적으로 보내는 Thread
 * REMARKS        :
 **end*******************************************************/
void *AlarmSendThread(void* arg)
{
    while(true)
    {
        sleep(C_ALARM_SEND_TIME);
  
        if(g_nHA_State != HA_Active)   	// Active가 아니면 wait 
        {
            continue;
        }
        
        // ALARM for AS BLOCK Status - ON 만 보냄 (20170306일 현재)
        if(C_AS_BLOCK)
        {
            SEND_AsBlockStatusToEMS();
            SEND_AsBlockAlarm();
        }

#ifdef INCLUDE_REGI
        // ALARM for AS REG Queue BLOCK Status
        if(g_bReqQ_Alarm == true) { SEND_Reg_Q_Alarm2(ALARM_ON);  }
        else                      { SEND_Reg_Q_Alarm2(ALARM_OFF); }
#endif
    }
    
    return(NULL);
}

/* Procedure Header
 **pdh********************************************************
 * PROCEDURE-NAME : SEND_AsBlockAlarm
 * CLASS-NAME     :
 * PARAMETER      : -
 * RET. VALUE     : 0
 * DESCRIPTION    : SPY와 연관된 ALATM을 주기적으로 보내는 Thread를 생성하는 함수 
 * REMARKS        :
 **end*******************************************************/
void CreateAlarmSendTherad(void)
{
    pthread_t   tid;
    
    if(pthread_create(&tid, NULL, AlarmSendThread, NULL))
    {
        Log.printf(LOG_ERR, "Can't ceate Alarm Send Therad reason=%d(%s)\n", errno, strerror(errno));
        return;
    }
    pthread_detach(tid);
}
