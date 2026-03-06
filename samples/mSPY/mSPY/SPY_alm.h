
#ifndef __SPY_ALM_H__
#define __SPY_ALM_H__

#define FLT_SPY_RSAM_UDP_SERVER     1
#define FLT_SPY_RSSW_UDP_SERVER     420

#ifdef OAMSV5_MODE
#define ALM_SPY_AS_BLOCK            601			// 0x259
#define ALM_SPY_SSW_DISCONNECT      300			// 0x12C
#define ALM_REG_Q_FULL_ALARM        402			// 0x192
#else
#define ALM_SPY_AS_BLOCK            0x12B
#define ALM_SPY_SSW_DISCONNECT      0x138
#define ALM_REG_Q_FULL_ALARM        0x121       // 170303, 170309 0x121 ·Î È®Á¤
#endif

#define STS_ID_SPY_CH_SYNC          4500

//typedef struct
//{
//	MMCHD       hdr;
//	uint8_t     type;
//	uint8_t     location;       // process ID
//	uint8_t     alarmFlag;      // flag = on(1)/off(0)
//	uint8_t     reserved;
//	int         alarmId;
//	uint8_t     extend[8];
//	char        comment[64];
//} ALARM_MSG;


typedef struct
{
	char	CODE[2];
	short	trigger;
	int     threadId;
	char	reserved[8];
	char	sql[1024];
} S_TO_ALTIBASE;

#endif  //__SPY_ALM_H__
