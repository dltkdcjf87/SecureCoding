
#ifndef __SPY_CFG_H__
#define __SPY_CFG_H__

typedef struct
{
    char        n_level;            // 로그 레벨 0~3(0:OFF, 1:상세, 2:보통, 3:간단)
    bool        b_options;          // OPTIONS SIP 메시지 출력 여부
    bool        b_register;         // REGISTER SIP 메시지 출력 여부
    uint32_t    n_max_size;         // (단위: MBytes) 로그 파일의 최대 크키(MAX_SIZE 보다 크면 백업됨)
    bool        b_outbund_proxy;    // SIP 송신을 특정 서버로만 하고자 할때 설정해서 사용(특수한 경우 ??)
    char        s_outbound_ip[MAX_IPADDR_LEN+1];    // OUTBOUND PROXY 사용시 송신 IP, port는 5060으로 FIX
} CFG_DEBUG;

typedef struct
{
    bool        b_as_block;         // AS 블록여부
    int         n_qos;              // Diff-Serv QoS 값
    
    char        s_ostack_ip[MAX_IPADDR_LEN+1];          // ORIG STACK IP
    int         n_ostack_port;                      // ORIG STACK PORT
    char        s_ostack_ip_port[MAX_IPADDR_LEN+1];     // ORIG STACK IP:PORT
    
    char        s_tstack_ip[MAX_IPADDR_LEN+1];          // TERM STACK IP
    int         n_tstack_port;                      // TERM STACK PORT
    char        s_tstack_ip_port[MAX_IPADDR_LEN+1];     // TERM STACK IP:PORT
#ifdef IPV6_MODE
    char        s_ostack_ip_v6[MAX_IPADDR_LEN+1];       // ORIG STACK IP
    char        s_ostack_ip_port_v6[MAX_IPADDR_LEN+1];  // ORIG STACK IP:PORT
    char        s_tstack_ip_v6[MAX_IPADDR_LEN+1];       // TERM STACK IP
    char        s_tstack_ip_port_v6[MAX_IPADDR_LEN+1];  // TERM STACK IP:PORT
    struct in6_addr     n_in6_spy;                      // SPY IPV6 address
    bool        b_ip_mode_v6;                           // IPv6로 동작을 할지 v4로 동작할지 여부
#endif
    int         n_ssw_port;         // 옥타브 스위치 기본 SIP 포트번호
    int         n_sam_port;         // 옥타브 스위치 기본 SIP 포트번호
    int         n_report_time;      // 주기적 report(log file에 write) 시간
    int         n_alarm_send_time;  // 주기적으로 Alarm 정보를 보내는 주기 (add 20170306)
    int         n_scm_map_audit_time;   // SCM MAP Audit시 얼마 이상이면 해제 할지에 해당하는 시간(1~5, defaut = 3)
    bool        b_syntax_check;     // SIP Syntax check 사용 여부
    bool        b_ecs_update;
    bool        b_hash_fail_route;
    bool        b_bye_no_hash_200;  // RSSW에서 HAsh에 없더라도 BYE에 대한 200을 SPY가 보낼지 여부
//    bool        b_use_scm_map;      // SCM 채널할당전에 MAP을 사용할지 여부(기존에는 Timer를 사용했으나 SCM에서 중복 할당하여 타이머 해제시 down 되는 문제가 있었음)
    
    int         n_max_req_q_size;   // REG Queue의 최대 사이즈로 이 Size보다 크면 480을 전송하고 queue에 넣지 않는다.
    int         n_reg_q_alm_limit;  // add 170303 REQ Q 가 Full이 되어 Alarm이 발생한 후 이 Limit 이하로 떨어지면 Alarm 을 OFF 한다.
} CFG_MODULE;

typedef struct
{
    int         n_rssw_o_thread;    // SSW에서 메세지를 수신해서 SAM으로 전송하는 Thread 수(ORIG)
    int         n_rssw_t_thread;    // SSW에서 메세지를 수신해서 SAM으로 전송하는 Thread 수(ORIG)
    int         n_rssw_reg_thread;  // SSW에서 REGISTER 메세지를 수신해서 처리하는 Thread 수
    int         n_rssw_opt_thread;  // SSW에서 OPTIONS 메세지를 수신해서 처리하는 Thread 수
    int         n_rsam_ses_thread;  // SAM에서 수신한 SIP 처리 쓰레드 개수 (SES 보드별  갯수, 전체 갯수 아님)
} CFG_THREAD;


typedef struct
{
    int         n_ses_board;        // 실장된 SES Board 수 (연속적으로 0부터 시작, 즉 n_ses_board가 3이면 0,1,2 보드가 실장됨)
// 20240123 bible : ONE Server - Multi SES
#ifdef ONE_SERVER_MULTI_SES
	int			n_ses_port[MAX_SES_BOARD];
#else
    int         n_ses_port;         // SAM RV Stack UPD port number
#endif
    char        s_ses_ip[MAX_SES_BOARD][MAX_IPADDR_LEN+1];  // SES IP Address (string)
    uint32_t    n_ses_ip[MAX_SES_BOARD];                    // SES IP Address inet_addr(s_ses_ip)
} CFG_SES;

typedef struct
{
    int         n_overload_resp_code;                   // Overload시 호를 제한할 때 사용하는 Response code
    bool        b_tas_block_480;                        // TAS인 경우 SERVICE BLOCK을 하면 503대신 480을 보낸다
    bool        b_del_18x_supported;                    // 18X의 Supported Header를 삭제하고 SSW로 전송할지 여부(일부 단말 이슈)
    bool        b_refer_405;                            // REFER를 수신하면 SPY에서 405를 전송할지 여부
    char        s_black_list_name[MAX_FNAME_LEN+1];     // BLACK LIST 파일명 (full path)
#ifdef IPV6_MODE
    char        s_black_list_name_v6[MAX_FNAME_LEN+1];  // IPv6
#endif
    char        s_web_extlist_name[MAX_FNAME_LEN+1];     // BLACK LIST 파일명 (full path)
#ifdef IPV6_MODE
    char        s_web_extlist_name_v6[MAX_FNAME_LEN+1];  // IPv6
#endif
    bool        b_black_list_use;                       // BLACK LIST 파일 사용 여부
    bool        b_add_update_timer;                     // UPDATE에   Supported: timer 추가 여부
    bool        b_add_200_invite_timer;                 // 200-INVITE에 Require: timer 추가 여부
    bool        b_add_200_update_timer;                 // 200-UPDATE에 Require: timer 추가 여부
    char        s_tas_register_skey[MAX_FNAME_LEN+1];   // SPY REGISTER를 처리할 Service Key
    char        s_dial_in_pfx[MAX_DN_LEN+1];            // TAS의  RCS_PFX와  동일 기능
    int         n_dial_in_pfx_len;                      // strlen(s_dial_in_pfx)
	bool		b_use_message;
	bool		b_use_regi_api_4;						//[JIRA AS-211]
} CFG_ETC;

typedef struct
{
    CFG_DEBUG   debug;
    CFG_MODULE  module;
    CFG_THREAD  thread;
    CFG_SES     ses;
    CFG_ETC     etc;
} SPY_CFG;

#define C_LOG_LEVEL           spy_cfg.debug.n_level
#define C_LOG_MAX_SIZE        spy_cfg.debug.n_max_size
#define C_LOG_OPTIONS         spy_cfg.debug.b_options
#define C_LOG_REGISTER        spy_cfg.debug.b_register
#define C_OUTBOUND_PROXY      spy_cfg.debug.b_outbund_proxy
#define C_OUTBOUND_IP         spy_cfg.debug.s_outbound_ip

#define C_AS_BLOCK            spy_cfg.module.b_as_block
#define C_NETWORK_QOS         spy_cfg.module.n_qos

#define C_ORIG_STACK_IP       spy_cfg.module.s_ostack_ip
#define C_ORIG_STACK_PORT     spy_cfg.module.n_ostack_port
#define C_ORIG_STACK_IP_PORT  spy_cfg.module.s_ostack_ip_port

#define C_TERM_STACK_IP       spy_cfg.module.s_tstack_ip
#define C_TERM_STACK_PORT     spy_cfg.module.n_tstack_port
#define C_TERM_STACK_IP_PORT  spy_cfg.module.s_tstack_ip_port
#define C_HASH_FAIL_ROUTE     spy_cfg.module.b_hash_fail_route
#define C_BYE_NO_HASH_200     spy_cfg.module.b_bye_no_hash_200
//#define C_USE_SCM_MAP         spy_cfg.module.b_use_scm_map
#define C_SCM_MAP_AUDIT_TIME  spy_cfg.module.n_scm_map_audit_time
#define C_REPORT_TIME         spy_cfg.module.n_report_time
#define C_ALARM_SEND_TIME     spy_cfg.module.n_alarm_send_time          // Add 20170306
#define C_MAX_REG_Q_SIZE      spy_cfg.module.n_max_req_q_size
#define C_REG_Q_ALM_LIMIT     spy_cfg.module.n_reg_q_alm_limit          // add 170303

#ifdef IPV6_MODE
#   define C_ORIG_STACK_IP_V6       spy_cfg.module.s_ostack_ip_v6
#   define C_ORIG_STACK_IP_PORT_V6  spy_cfg.module.s_ostack_ip_port_v6
#   define C_TERM_STACK_IP_V6       spy_cfg.module.s_tstack_ip_v6
#   define C_TERM_STACK_IP_PORT_V6  spy_cfg.module.s_tstack_ip_port_v6
#   define C_IN6_SPY                spy_cfg.module.n_in6_spy
#   define C_IP_MODE_V6             spy_cfg.module.b_ip_mode_v6

#   define C_BLACK_LIST_NAME_V6     spy_cfg.etc.s_black_list_name_v6
#   define C_WEB_EXTLIST_NAME_V6    spy_cfg.etc.s_web_extlist_name_v6
#endif


#define C_SSW_PORT            spy_cfg.module.n_ssw_port
#define C_SAM_PORT            spy_cfg.module.n_sam_port //MIN ADD 20210415
#define C_SYNTAX_CHECK        spy_cfg.module.b_syntax_check
#ifdef ECS_MODE
#   define C_ECS_UPDATE          spy_cfg.module.b_ecs_update

#endif

#define C_MAX_RSSW_ORIG_THREAD    spy_cfg.thread.n_rssw_o_thread
#define C_MAX_RSSW_TERM_THREAD    spy_cfg.thread.n_rssw_t_thread
#define C_MAX_RSSW_REG_THREAD     spy_cfg.thread.n_rssw_reg_thread
#define C_MAX_RSSW_OPT_THREAD     spy_cfg.thread.n_rssw_opt_thread
#define C_MAX_RSAM_SES_THREAD     spy_cfg.thread.n_rsam_ses_thread

#define C_SES_BOARD           spy_cfg.ses.n_ses_board
#define C_SES_PORT            spy_cfg.ses.n_ses_port
#define C_SES_IP              spy_cfg.ses.s_ses_ip
#define C_SES_INET_ADDR       spy_cfg.ses.n_ses_ip

#define C_DIAL_IN_PREFIX          spy_cfg.etc.s_dial_in_pfx
#define C_DIAL_IN_PREFIX_LEN      spy_cfg.etc.n_dial_in_pfx_len
#define C_TAS_BLOCK_480_FLAG      spy_cfg.etc.b_tas_block_480
#define C_DEL_18X_SUPPORTED_FLAG  spy_cfg.etc.b_del_18x_supported
#define C_REFER_405               spy_cfg.etc.b_refer_405
#define C_BLACK_LIST_NAME         spy_cfg.etc.s_black_list_name
#define C_WEB_EXTLIST_NAME        spy_cfg.etc.s_web_extlist_name
#define C_BLACK_LIST_USE          spy_cfg.etc.b_black_list_use
#define C_ADD_UPDATE_TIMER        spy_cfg.etc.b_add_update_timer
#define C_ADD_200_UPDATE_TIMER    spy_cfg.etc.b_add_200_update_timer
#define C_ADD_200_INVITE_TIMER    spy_cfg.etc.b_add_200_invite_timer
#define C_TAS_REGISTER_SKEY       spy_cfg.etc.s_tas_register_skey
#define C_OVERLOAD_RESP_CODE      spy_cfg.etc.n_overload_resp_code
#define C_USE_MESSAGE			  spy_cfg.etc.b_use_message		// 20170725
#define C_USE_REGI_API4			  spy_cfg.etc.b_use_regi_api_4	//[JIRA AS-211]

#define MAX_DEQUEUE_COUNT_SSW   5000      // QUEUE메세지 연속 처리 횟수 (CPU 부하 고려) - FIXIT: goto config

#define MAX_DEQUEUE_COUNT_SAM   5000      // QUEUE메세지 연속 처리 횟수 (CPU 부하 고려)


#endif  // __SPY_CFG_H__
