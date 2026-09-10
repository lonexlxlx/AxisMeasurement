#pragma once

/*-----------------------------------------------------------*/
/* Ringnet                                                  */
/*-----------------------------------------------------------*/
#define RTN_SUCCESS				0

#define RTN_MALLOC_FAIL						-100 /* malloc memory fail */
#define RTN_FREE_FAIL							-101 /* free memory or delete the object fail */
#define RTN_NULL_POINT						-102 /* the param point input is null */ 
#define RTN_ERR_ORDER						-103 /* call the function order is wrong, some msg isn't validable */
#define RTN_PCI_NULL							-104 /* the pci address is empty, can't access the pci device*/
#define RTN_PARAM_OVERFLOW			-105 /* the param input is too larget*/
#define RTN_LINK_FAIL							-106 /* the two ports both link fail*/ 
#define RTN_IMPOSSIBLE_ERR				-107 /* it means the system or same function work wrong*/
#define RTN_TOPOLOGY_CONFLICT			-108 /* the id conflict*/
#define RTN_TOPOLOGY_ABNORMAL		-109 /* scan the net abnormal*/
#define RTN_STATION_ALONE				-110 /* the device no id, it means the device id is 0xF0 */
#define RTN_WAIT_OBJECT_OVERTIME	-111 /* multi thread wait for object overtime */
#define RTN_ACCESS_OVERFLOW			-112 /* data[i];  i is larger than the define */
#define RTN_NO_STATION						-113 /* the station accessed not existent */
#define RTN_OBJECT_UNCREATED			-114 /* the object not created yet*/
#define RTN_PARAM_ERR						-115 /* the param input is wrong*/
#define RTN_PDU_CFG_ERR                   -116/*Pdu DMA Cfg Err*/
#define RTN_PCI_FPGA_ERR					-117 /*PCI op err or FPGA op err*/
#define RTN_CHECK_RW_ERR					-118	/*data write to reg, then rd out, and check err */
#define RTN_REMOTE_UNEABLE				-119 /*the device which will be ctrl by net can't be ctrl by net function*/ 

#define RTN_NET_REQ_DATA_NUM_ZERO		-120 /*mail op or pdu op req data num can't be 0*/
#define RTN_WAIT_NET_OBJECT_OVERTIME	-121 /* net op multi thread wait for object overtime */
#define RTN_WAIT_NET_RESP_OVERTIME		-122 /* Can't wait for resp */
#define RTN_WAIT_NET_RESP_ERR				-123 /*wait mailbox op err*/
#define RTN_INITIAL_ERR								-124 /*initial the device err*/
#define RTN_PC_NO_READY							-125 /*find the station'pc isn't work*/ 
#define RTN_STATION_NO_EXIST					-126 
#define RTN_MASTER_FUNCTION					-127 /* this funciton only used by master */

#define RTN_NOT_ALL_RETURN							-128 /* the GT_RN_PtProcessData funciton fail return */
#define RTN_NUM_NOT_EQUAL							-129 /* the station number of RingNet do not equal  the station number of CFG */

#define RTN_CHECK_STATION_ONLINE_NUM_ERR		-130/*Check no slave*/
#define RTN_FILE_ERR_OPEN								-131/*open file error*/
#define RTN_FILE_ERR_FORMAT							-132/*parse file error*/
#define RTN_FILE_ERR_MISSMATCH					-133/*file info is not match with the actual ones*/
#define RTN_DMALIST_ERR_MISSMATCH			-134/*can't find the slave*/

#define RTN_REQUSET_MAIL_BUS_OVERTIME		-150 /*Requset Mail Bus Err*/
#define RTN_INSTRCTION_ERR							-151 /*instrctions err*/
#define RTN_MAIL_RESP_REQ_ERR						-152 /*RN_MailRespReq  err*/
#define RTN_CTRL_SRC_ERR								-153 /* the controlled source  is error */
#define RTN_PACKET_ERR									-154 /*packet is error*/
#define RTN_STATION_ID_ERR							-155 /*the device id is not in the right rang*/
#define RTN_WAIT_NET_PDU_RESP_OVERTIME	- 156 /*net pdu op wait overtime*/
#define RTN_ETHCAT_ENC_POS_ERR					-157/**/

#define RTN_IDLINK_PACKET_ERR				-200 /*ilink master  decode err! packet_length is not match*/
#define RTN_IDLINK_PACKET_END_ERR		-201 /* the ending of ilink packet is not 0xFF*/
#define RTN_IDLINK_TYPER_ERR					-202 /* the type of ilink module is error*/
#define RTN_IDLINK_LOST_CNT 					-203 /* the ilink module has lost connection*/
#define RTN_IDLINK_CTRL_SRC_ERR			-204 /* the controlled source of ilink module is error */
#define RTN_IDLINK_UPDATA_ERR				-205 /* the ilink module updata error*/
#define RTN_IDLINK_NUM_ERR					-206 /* the ilink num larger the IDLINK_MAX_NUM(30) */
#define RTN_IDLINK_NUM_ZERO					-207 /* the ilink num is zero */

#define RTN_NO_PACKET							301 /* no valid packet */
#define RTN_RX_ERR_PDU_PACKET				-302 /* ERR PDU PACKET */
#define RTN_STATE_MECHINE_ERR				-303 
#define RTN_PCI_DSP_UN_FINISH				304
#define RTN_SEND_ALL_FAIL						-305
#define RTN_STATION_CLOSE					310
#define RTN_STATION_RESP_FAIL				311		

#define RTN_UPDATA_MODAL_ERR			-330 /* update the modal in normal way fail*/

#define RTN_NO_MAIL_DATA						340 /*There is no mail data*/
#define RTN_NO_PDU_DATA						341 /*There is no pdu data*/


#define RTN_FILE_PARAM_NUM_ERR					-500
#define RTN_FILE_PARAM_LEN_ERR					-501
#define RTN_FILE_MALLOC_FAIL							-502
#define RTN_FILE_FREE_FAIL								-503
#define RTN_FILE_PARAM_ERR							-504
#define RTN_FILE_NOT_EXSITS							505
#define RTN_FILE_CREATE_FAIL							510
#define RTN_FILE_DELETE_FAIL							511
#define RTN_FIFE_CRC_CHECK_ERR					-512
#define RTN_FIFE_FUNCTION_ID_RETURN_ERR	-600

#define RTN_DLL_WINCE									-800
#define RTN_DLL_WIN32									-801

#define RTN_XML_STATION_ERR						-900//dma config file confilit with slave type


GT_API GT_RN_GetEncPos(short encoder, double *pValue, short count, unsigned long *pClock);
GT_API GT_RN_GetAxisError(short axis, double *pValue, short count, unsigned long *pClock);
GT_API GT_RN_GetPrfMode(short axis, long* pValue, short count, unsigned long *pClock);
GT_API GT_RN_GetAuEncPos(short encoder, double *pValue, short count, unsigned long *pClock);
GT_API GT_RN_GetCaptureStatus(short encoder, short* pStatus, long *pValue, short count, unsigned long *pClock);
GT_API GT_RN_GetSts(short axis, long* pSts, short count, unsigned long *pClock);
GT_API GT_RN_GetPowerSts(long* pValue);
GT_API GT_RN_GetEcatAxisACTArray(short axis, short *pCur, short* pTorque, short count);
GT_API GT_RN_PtSpaceArray(short profile, short *pSpace, short fifo, short count);
GT_API GT_RN_GetDoEx(short doType, long* pValue);
GT_API GT_RN_GetDiEx(short diType, long* pValue);
GT_API GT_RN_GetDo(short doType, long* pValue);
GT_API GT_RN_GetDi(short diType, long* pValue);
GT_API GT_RN_GetSts(short axis, long* pSts, short count, unsigned long *pClock);

GT_API GTN_LoadRingNetConfig(short core,char*pFile);
GT_API GTN_SaveRingNetConfig(short core,char *pFile);

#define TERMINAL_LOAD_MODE_NONE             (0)
#define TERMINAL_LOAD_MODE_BOOT             (2)

typedef struct
{
	unsigned short type;
	short id;
	long status;
	unsigned long synchCount;
	unsigned long ringNetType;
	unsigned long portStatus;
	unsigned long sportDropCount;
	unsigned long reserve[7];
} TTerminalStatus;

GT_API GT_TerminalInit(short detect);
GT_API GT_GetTerminalVersion(short core,short station,TVersion *pTerminalVersion);
GT_API GT_SetTerminalPermit(short core,short index,short dataType,unsigned short permit);
GT_API GT_GetTerminalPermit(short core,short index,short dataType,unsigned short *pPermit);
GT_API GT_SetTerminalPermitEx(short core,short station,short dataType,short *permit,short index=1,short count=1);
GT_API GT_GetTerminalPermitEx(short core,short station,short dataType,short *pPermit,short index=1,short count=1);

GT_API GT_ProgramTerminalConfig(short loadMode);
GT_API GT_GetTerminalConfigLoadMode(short *pLoadMode);
GT_API GT_FindStation(short core,short station,unsigned long time);

GT_API GTN_TerminalInit(short core,short detect=1);
GT_API GTN_GetTerminalVersion(short core,short index,TVersion *pTerminalVersion);
GT_API GTN_SetTerminalPermit(short core,short index,short dataType,unsigned short permit);
GT_API GTN_SetTerminalPermitEx(short core,short station,short dataType,short *permit,short index,short count);
GT_API GTN_GetTerminalPermitEx(short core,short station,short dataType,short *pPermit,short index,short count);

GT_API GTN_FindStation(short core,short station,unsigned long time);
GT_API GTN_GetTerminalPermit(short core,short index,short dataType,unsigned short *pPermit);
GT_API GTN_ProgramTerminalConfig(short core,short loadMode);
GT_API GTN_GetTerminalConfigLoadMode(short core,short *pLoadMode);

GT_API GTN_ReadPhysicalMap(void);
GT_API ConvertPhysical(short core,short dataType,short terminal,short index);

GT_API GTN_SetTerminalSafeMode(short core,short index,short safeMode);
GT_API GTN_GetTerminalSafeMode(short core,short index,short *pSafeMode);
GT_API GTN_ClearTerminalSafeMode(short core,short index);
GT_API GTN_GetTerminalStatus(short core,short index,TTerminalStatus *pTerminalStatus);
GT_API GTN_GetTerminalType(short core,short count,unsigned short *pType,short *pTypeConnect=NULL);
GT_API GTN_GetLinkPortTxInfo(short core,unsigned long *info,unsigned short *reminderNum,unsigned short *pCRxNum,unsigned short *lPTxNum );
GT_API GTN_GetRNMasterInfo(short core,unsigned short *pPhyId,unsigned short *pType,unsigned short *pInfo);
GT_API GTN_SetRNMasterInfo(short core,unsigned short phyId,unsigned short type,unsigned short info);


	//仨联，多模组指令
GT_API GTN_RN_MltPcPduRd(short core,unsigned char* pData, unsigned char des_id,unsigned short byte_start_offset,unsigned short byte_num);
GT_API GTN_RN_MltPcPduRdUpdate(short core,unsigned char des_id);
GT_API GTN_RN_MltPcPduWr(short core,unsigned char* pData, unsigned char des_id, unsigned short byte_start_offset,unsigned short byte_num);
GT_API GTN_RN_MltPcPduWrUpdate(short core,unsigned char des_id);
/*-----------------------------------------------------------*/
/* Config of module                                          */
/*-----------------------------------------------------------*/
#define TERMINAL_OPERATION_NONE             (0)
#define TERMINAL_OPERATION_SKIP             (1)
#define TERMINAL_OPERATION_CLEAR            (2)
#define TERMINAL_OPERATION_RESET_MODULE     (3)

#define TERMINAL_OPERATION_PROGRAM          (11)

typedef struct
{
	unsigned long portACrcOkCnt;
	unsigned short portACrcErrorCnt;
	unsigned long portBCrcOkCnt;
	unsigned short portBCrcErrorCnt;
	unsigned long reserve;//目前用于读取FLASH总数据长度
} TRingNetCrcStatus;

typedef struct
{
	unsigned short errorCountReceive;
	unsigned short errorCountPackageDown;
	unsigned short errorCountPackageUp;
	/*unsigned long portACrcOkCnt;
	unsigned short portACrcErrorCnt;
	unsigned long portBCrcOkCnt;
	unsigned short portBCrcErrorCnt;*/
	unsigned short reserve[13];
} TTerminalError;

typedef struct
{
	short moduleDataType;
	short moduleDataIndex;
	short dataIndex;
	short dataCount;
} TTerminalMap;


GT_API GT_LoadTerminalConfig(short core,char *pFile);
GT_API GT_SaveTerminalConfig(short core,char *pFile);
GT_API GT_TerminalOn(short index);
GT_API GT_TerminalSynch(short index);
GT_API GT_GetRintNetCrcStatus(short index,TRingNetCrcStatus *pRingNetCrcStatus);
GT_API GT_GetTerminalStatus(short index,TTerminalStatus *pTerminalStatus);
GT_API GT_GetTerminalError(short core,short station,TTerminalError *pTerminalError);
GT_API GT_SetTerminalType(short count,short *pType);
GT_API GT_SetTerminalSafeMode(short core,short index,short safeMode);
GT_API GT_GetTerminalSafeMode(short core,short index,short *pSafeMode);
GT_API GT_ClearTerminalSafeMode(short port,short index);
GT_API GT_GetTerminalType(short count,unsigned short *pType,short *pTypeConnect);
GT_API GT_GetTerminalPhyId(short count,short *pPhyId);
GT_API GT_GetTerminalLinkStatus(short core,short count,short *ringNetType,short *pLinkStatus);
GT_API GT_SetTerminalMap(short dataType,short moduleIndex,TTerminalMap *pMap);
GT_API GT_GetTerminalMap(short dataType,short moduleIndex,TTerminalMap *pMap);
GT_API GT_ClearTerminalMap(short dataType);
GT_API GT_SetTerminalMode(short core,short index,unsigned short mode);
GT_API GT_GetTerminalMode(short core,short index,unsigned short *pMode);
GT_API GT_SetTerminalTest(short core,short station,short index,unsigned short value);
GT_API GT_GetTerminalTest(short core,short station,short index,unsigned short *pValue);
GT_API GT_SetTerminalOperation(short operation);
GT_API GT_GetTerminalOperation(short *pOperation);
GT_API GT_SetMailbox(short core,short station,unsigned short byteAddress,unsigned short *pData,unsigned short wordCount,unsigned short dataMode,unsigned short desId,unsigned short type);
GT_API GT_GetMailbox(short core,short station,unsigned short byteAddress,unsigned short *pData,unsigned short wordCount,unsigned short dataMode,unsigned short desId,unsigned short type);

GT_API GTN_LoadTerminalConfig(short core,char *pFile);
GT_API GTN_SaveTerminalConfig(short core,char *pFile);
GT_API GTN_TerminalOn(short core,short index);
GT_API GTN_TerminalSynch(short core,short index);
GT_API GTN_GetRingNetCrcStatus(short core,short index,TRingNetCrcStatus *pRingNetCrcStatus);
GT_API GTN_GetTerminalError(short core,short index,TTerminalError *pTerminalError);
GT_API GTN_SetTerminalType(short core,short count,short *pType);
GT_API GTN_GetTerminalPhyId(short core,short count,short *pPhyId);
GT_API GTN_GetTerminalLinkStatus(short core,short count,short *ringNetType,short *pLinkStatus);
GT_API GTN_SetTerminalMap(short core,short dataType,short moduleIndex,TTerminalMap *pMap);
GT_API GTN_GetTerminalMap(short core,short dataType,short moduleIndex,TTerminalMap *pMap);
GT_API GTN_ClearTerminalMap(short core,short dataType);
GT_API GTN_SetTerminalMode(short core,short station,unsigned short mode);
GT_API GTN_GetTerminalMode(short core,short station,unsigned short *pMode);
GT_API GTN_SetTerminalTest(short core,short station,short index,unsigned short value);
GT_API GTN_GetTerminalTest(short core,short station,short index,unsigned short *pValue);
GT_API GTN_SetTerminalOperation(short core,short operation);
GT_API GTN_GetTerminalOperation(short core,short *pOperation);
GT_API  GTN_SetMailbox(short core,short station,unsigned short byteAddress,unsigned short *pData,unsigned short wordCount,unsigned short dataMode,unsigned short desId,unsigned short type);
GT_API GTN_GetMailbox(short core,short station,unsigned short byteAddress,unsigned short *pData,unsigned short wordCount,unsigned short dataMode,unsigned short desId,unsigned short type);
GT_API GTN_GetUuid(short core,char *pCode,short count);

GT_API GTN_GetTerminalPhyId(short core,short count,short *pPhyId);

GT_API GTN_Ringnet_MailUserRd(short core,unsigned short addr, unsigned short* data,unsigned short data_num, unsigned char data_mode, unsigned char des_id);
GT_API GTN_Rintnet_MailUserWr(short core,unsigned short addr, unsigned short* data,unsigned short data_num, unsigned char data_mode, unsigned char des_id);

