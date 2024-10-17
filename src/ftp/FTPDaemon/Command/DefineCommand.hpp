#pragma once
#include "FTPStrategy.hpp"
#include "FTPCommand.hpp"

// #define FEAT_COMMAND                "FEAT" //서버가 추가한 기능 목록 보기
// #define PASV_COMMAND                "PASV" //수동 모드 들어가기.
// #define PORT_COMMAND                "PORT" //서버 접속에 필요한 주소 및 포트 지정.
// #define LIST_COMMAND                "LIST" //지정한 경우 파일이나 디렉터리 정보를 반환. 지정하지 않은 경우 현재 작업 디렉터리 정보 반환.
// #define NLST_COMMAND                "NLST" //지정한 디렉터리의 파일 이름 목록 반환.
// #define RETR_COMMAND                "RETR" //파일 복사본 전송
// #define STOR_COMMAND                "STOR" //데이터 입력 및 서버 쪽 파일로 저장.
// #define APPE_COMMAND                "APPE" //이어서 추가.
// #define STOU_COMMAND                "STOU" //파일을 저만의 방식으로 저장.
// #define AUTH_COMMAND                "AUTH" //인증/보안 구조
// #define USER_COMMAND                "USER" //인증 사용자 이름.

constexpr const char * ABOR_COMMAND     = "ABOR";
constexpr const char * ACCT_COMMAND     = "ACCT";
constexpr const char * ADAT_COMMAND     = "ADAT";
constexpr const char * ALLO_COMMAND     = "ALLO";
constexpr const char * APPE_COMMAND     = "APPE";
constexpr const char * AUTH_COMMAND     = "AUTH";
constexpr const char * AVBL_COMMAND     = "AVBL";
constexpr const char * CCC_COMMAND      = "CCC";
constexpr const char * CDUP_COMMAND     = "CDUP";
constexpr const char * CONF_COMMAND     = "CONF";
constexpr const char * CSID_COMMAND     = "CSID";
constexpr const char * CWD_COMMAND      = "CWD";
constexpr const char * DELE_COMMAND     = "DELE";
constexpr const char * DSIZ_COMMAND     = "DSIZ";
constexpr const char * ENC_COMMAND      = "ENC";
constexpr const char * EPRT_COMMAND     = "EPRT";
constexpr const char * EPSV_COMMAND     = "EPSV";
constexpr const char * FEAT_COMMAND     = "FEAT";
constexpr const char * HELP_COMMAND     = "HELP";
constexpr const char * HOST_COMMAND     = "HOST";
constexpr const char * LANG_COMMAND     = "LANG";
constexpr const char * LIST_COMMAND     = "LIST";
constexpr const char * LPRT_COMMAND     = "LPRT";
constexpr const char * LPSV_COMMAND     = "LPSV";
constexpr const char * MDTM_COMMAND     = "MDTM";
constexpr const char * MFCT_COMMAND     = "MFCT";
constexpr const char * MFF_COMMAND      = "MFF";
constexpr const char * MFMT_COMMAND     = "MFMT";
constexpr const char * MIC_COMMAND      = "MIC";
constexpr const char * MKD_COMMAND      = "MKD";
constexpr const char * MLSD_COMMAND     = "MLSD";
constexpr const char * MLST_COMMAND     = "MLST";
constexpr const char * MODE_COMMAND     = "MODE";
constexpr const char * NLST_COMMAND     = "NLST";
constexpr const char * NOOP_COMMAND     = "NOOP";
constexpr const char * OPTS_COMMAND     = "OPTS";
constexpr const char * PASS_COMMAND     = "PASS";
constexpr const char * PASV_COMMAND     = "PASV";
constexpr const char * PBSZ_COMMAND     = "PBSZ";
constexpr const char * PORT_COMMAND     = "PORT";
constexpr const char * PROT_COMMAND     = "PROT";
constexpr const char * PWD_COMMAND      = "PWD";
constexpr const char * QUIT_COMMAND     = "QUIT";
constexpr const char * REIN_COMMAND     = "REIN";
constexpr const char * REST_COMMAND     = "REST";
constexpr const char * RETR_COMMAND     = "RETR";
constexpr const char * RMD_COMMAND      = "RMD";
constexpr const char * RMDA_COMMAND     = "RMDA";
constexpr const char * RNFR_COMMAND     = "RNFR";
constexpr const char * RNTO_COMMAND     = "RNTO";
constexpr const char * SITE_COMMAND     = "SITE";
constexpr const char * SIZE_COMMAND     = "SIZE";
constexpr const char * SMNT_COMMAND     = "SMNT";
constexpr const char * SPSV_COMMAND     = "SPSV";
constexpr const char * STAT_COMMAND     = "STAT";
constexpr const char * STOR_COMMAND     = "STOR";
constexpr const char * STOU_COMMAND     = "STOU";
constexpr const char * STRU_COMMAND     = "STRU";
constexpr const char * SYST_COMMAND     = "SYST";
constexpr const char * THMB_COMMAND     = "THMB";
constexpr const char * TYPE_COMMAND     = "TYPE";
constexpr const char * USER_COMMAND     = "USER";
constexpr const char * XCUP_COMMAND     = "XCUP";
constexpr const char * XMKD_COMMAND     = "XMKD";
constexpr const char * XPWD_COMMAND     = "XPWD";
constexpr const char * XRCP_COMMAND     = "XRCP";
constexpr const char * XRMD_COMMAND     = "XRMD";
constexpr const char * XRSQ_COMMAND     = "XRSQ";
constexpr const char * XSEM_COMMAND     = "XSEM";
constexpr const char * XSEN_COMMAND     = "XSEN";
constexpr const char * START_COMMAND    = "START";
constexpr const char * DEFAULT_COMMAND  = "DEFAULT";

#define REQUEST_TO_ANALYSIS         RequestToAnalysis, ResultFromAnalysis

#define DATA_CHANNEL_CONNECT        SendRequest, ConnectDataChannel, GetResponse, ResponseToAnalysis, CheckDataChannelConnection, SendResponse
#define GET_MODE_DATA_TRANSFER      GetMode, DataTransfer, GetResponse, ResponseToAnalysis, SendResponse
#define PUT_MODE_DATA_TRANSFER      PutMode, DataTransfer, GetResponse, ResponseToAnalysis, SendResponse

using DefaultCommand = FTPCommand<Error, REQUEST_TO_ANALYSIS, SendRequest, GetResponse, ResponseToAnalysis, SendResponse>;
using FeatCommand    = FTPCommand<Error, REQUEST_TO_ANALYSIS, SendRequest, GetFEATResponse, ResponseToAnalysis, SendResponse>;
using PasvCommand    = FTPCommand<Error, REQUEST_TO_ANALYSIS, SendRequest, GetResponse, ResponseToAnalysis, PassiveResponse, SetPassiveMode, SendResponse>;
using PortCommand    = FTPCommand<Error, REQUEST_TO_ANALYSIS, PortRequest, SendRequest, GetResponse, ResponseToAnalysis, CheckPortResponse, SetActiveMode, SendResponse>;
using AuthCommand    = FTPCommand<Error, AuthRequest>;
using UserCommand    = FTPCommand<Error, SetUser, REQUEST_TO_ANALYSIS, SendRequest, GetResponse, ResponseToAnalysis, SendResponse>;
using StartCommand   = FTPCommand<Error, GetClientIP, ConnectToServer, GetResponse, ResponseToAnalysis, CheckServer, ServerReady>;

using ListCommand    = FTPCommand<Error, REQUEST_TO_ANALYSIS, DATA_CHANNEL_CONNECT, GET_MODE_DATA_TRANSFER>;
using RetrCommand    = FTPCommand<Error, REQUEST_TO_ANALYSIS, DATA_CHANNEL_CONNECT, GET_MODE_DATA_TRANSFER>;
using StorCommand    = FTPCommand<Error, REQUEST_TO_ANALYSIS, DATA_CHANNEL_CONNECT, PUT_MODE_DATA_TRANSFER>;