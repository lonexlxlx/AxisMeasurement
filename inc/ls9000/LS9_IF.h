//Copyright(c) 2012, KEYENCE CORPORATION. All rights reserved.
/** @file
@brief	LS9_IF.h
*/

#pragma once
#pragma managed(push, off)

#ifdef LS9_IF_EXPORTS
#define LS9_IF_API __declspec(dllexport)
#else
#define LS9_IF_API __declspec(dllimport)
#endif

/// Specifying the setting value storage hierarchy
typedef enum {
	LS9IF_SETTING_DEPTH_WRITE		= 0x00,		// Domain for writing settings
	LS9IF_SETTING_DEPTH_RUNNING		= 0x01,		// Domain for the running settings
	LS9IF_SETTING_DEPTH_SAVE		= 0x02,		// Domain for saving
} LS9IF_SETTING_DEPTH;

/// Specifying the setting mode
typedef enum {
	LS9IF_SETTING_KIND_SYSTEM = 0x01, 		// System setting
	LS9IF_SETTING_KIND_PROGRAM0 = 0x10, 	// Program 0
	LS9IF_SETTING_KIND_PROGRAM1 = 0x11, 	// Program 1
	LS9IF_SETTING_KIND_PROGRAM2 = 0x12, 	// Program 2
	LS9IF_SETTING_KIND_PROGRAM3 = 0x13, 	// Program 3
	LS9IF_SETTING_KIND_PROGRAM4 = 0x14, 	// Program 4
	LS9IF_SETTING_KIND_PROGRAM5 = 0x15, 	// Program 5
	LS9IF_SETTING_KIND_PROGRAM6 = 0x16, 	// Program 6
	LS9IF_SETTING_KIND_PROGRAM7 = 0x17, 	// Program 7
	LS9IF_SETTING_KIND_PROGRAM8 = 0x18, 	// Program 8
	LS9IF_SETTING_KIND_PROGRAM9 = 0x19, 	// Program 9
	LS9IF_SETTING_KIND_PROGRAM10 = 0x1A, 	// Program 10
	LS9IF_SETTING_KIND_PROGRAM11 = 0x1B, 	// Program 11
	LS9IF_SETTING_KIND_PROGRAM12 = 0x1C, 	// Program 12
	LS9IF_SETTING_KIND_PROGRAM13 = 0x1D, 	// Program 13
	LS9IF_SETTING_KIND_PROGRAM14 = 0x1E, 	// Program 14
	LS9IF_SETTING_KIND_PROGRAM15 = 0x1F, 	// Program 15
} LS9IF_SETTING_KIND;

/// Specifying the calibration of received light intensity
typedef enum {
	LS9IF_LIGHT_CALIB_MODE_START = 0x00,	// Start
	LS9IF_LIGHT_CALIB_MODE_TRIGGER = 0x01,	// Issue trigger
	LS9IF_LIGHT_CALIB_MODE_APPLY = 0x02,	// OK
	LS9IF_LIGHT_CALIB_MODE_CANCEL = 0x03,	// Cancel
	LS9IF_LIGHT_CALIB_MODE_INIT = 0x04,		// Initialize
} LS9IF_LIGHT_CALIB_MODE;

/// Target of calibration of received light intensity
typedef enum {
	LS9IF_LIGHT_CALIB_TARGET_RESERVE = 0x00,		// Reserve
	LS9IF_LIGHT_CALIB_TARGET_NORMAL = 0x01,			// Normal
} LS9IF_LIGHT_CALIB_TARGET;

/// Definition indicating the validity of measurement values
typedef enum {
	LS9IF_MEASURE_DATA_INFO_VALID	= 0x00,			// Normal measurement data
	LS9IF_MEASURE_DATA_INFO_WAIT	= 0x01,			// Output waiting data 
	LS9IF_MEASURE_DATA_INFO_PLUS_OVER	= 0x02,		// Above the plus limit range
	LS9IF_MEASURE_DATA_INFO_MINUS_OVER	= 0x03,		// Below the minus limit range
} LS9IF_MEASURE_DATA_INFO;

/// Definition indicating the tolerance judgement result of measurement values
typedef enum {
	LS9IF_JUDGE_RESULT_NONE = 0x00,
	LS9IF_JUDGE_RESULT_HH	= 0x01,		// HH
	LS9IF_JUDGE_RESULT_HI	= 0x02,		// HI
	LS9IF_JUDGE_RESULT_GO	= 0x04,		// GO
	LS9IF_JUDGE_RESULT_LO	= 0x08,		// LO
	LS9IF_JUDGE_RESULT_LL	= 0x10,		// LL
} LS9IF_JUDGE_RESULT;

/// Ethernet setting structure
typedef struct {
	BYTE	abyIpAddress[4];	// IP Address of the controller to be connected
	WORD	wPortNo;			// Port Number of the controller to be connected
	BYTE	reserve[2];			// Reserve
} LS9IF_ETHERNET_CONFIG;

/// Time structure
typedef struct {
	BYTE byYear;		// year(s) (in a range of 0 to 99 which means 2000 to 2009 years)
	BYTE byMonth;		// month(s) (1 to 12)
	BYTE byDay;			// day(s) (1 to 31)
	BYTE byHour;		// hour(s) (0 to 23)
	BYTE byMinute;		// minute(s) (0 to 59)
	BYTE bySecond;		// second(s) (0 to 59)
	BYTE reserve[2];	// Reserve
} LS9IF_TIME;

/// Structure of setting item specification 
typedef struct {
	BYTE	byType;			// Type
	BYTE	byCategory;		// Category
	BYTE	byItem;			// Item
	BYTE	reserve;		// Reserve
	BYTE	byTarget;		// Target
} LS9IF_TARGET_SETTING;

/// Measurement value structure
typedef struct {
	BYTE byDataInfo;		// Whether or not the measurement value is valid
	BYTE byJudge;			// Tolerance judgement result
	BYTE byTimZero;			// State of TIMING, auto ZERO
	FLOAT fValue;			// Measurement value
} LS9IF_MEASURE_VALUE;

/// Measurement result structure
typedef struct {
	BYTE byYear;							// year(s) (in a range of 0 to 99 which means 2000 to 2009 years)
	BYTE byMonth;							// month(s) (1 to 12)
	BYTE byDay;								// day(s) (1 to 31)
	BYTE byHour;							// hour(s) (0 to 23)
	BYTE byMinute;							// minute(s) (0 to 59)
	BYTE bySecond;							// second(s) (0 to 59)
	BYTE byMillsecond;						// Millisecond
	DWORD dwPulseCnt;						// Pulse count value
	BYTE byTotalJudge;						// Total judgement result
	LS9IF_MEASURE_VALUE stMesureValue[16];	// Measurement value information
} LS9IF_MEASURE_DATA;

/// Statistic sampling structure
typedef struct {
	BYTE byStatus;				// Statistic process status
	FLOAT fAverage;				// Average value
	FLOAT fMaximum;				// Maximum value
	FLOAT fMinimum;				// Minimum value
	FLOAT fMax_Min;				// Max-Min
	FLOAT fStdDeviation;		// Standard deviation
	DWORD dwDenominator;		// Parameter
	DWORD dwHH_Count;			// HH count
	DWORD dwHI_Count;			// HI count
	DWORD dwGO_Count;			// GO count
	DWORD dwLO_Count;			// LO count
	DWORD dwLL_Count;			// LL count
} LS9IF_STAT_SAMP;

/// Storage information structure
typedef struct {
	BYTE	byStatus;				// Storage status
	BYTE	reserve;				// Reserve
	WORD	wStorageVer;			// Storage version
	DWORD	dwStorageCnt;			// Storage count
} LS9IF_STORAGE_INFO;

/// Structure of storage data measurement value
typedef struct {
	BYTE	byDataInfo;
	BYTE	byJudge;
	FLOAT	fValue;
} LS9IF_STORAGE_VALUE;

/// Storage data structure
typedef struct {
	BYTE byYear;								// year(s) (in a range of 0 to 99 which means 2000 to 2009 years)
	BYTE byMonth;								// month(s) (1 to 12)
	BYTE byDay;									// day(s) (1 to 31)
	BYTE byHour;								// hour(s) (0 to 23)
	BYTE byMinute;								// minute(s) (0 to 59)
	BYTE bySecond;								// second(s) (0 to 59)
	BYTE byMillsecond;							// Millisecond
	BYTE byDstFlg;								// Daylight savings time flag
	DWORD dwPulseCnt;							// Pulse count value
	LS9IF_STORAGE_VALUE stStorageValue[16];		// Information of storage data measurement value
} LS9IF_STORAGE_DATA;

extern "C"
{
	// Function
	// Operation on DLL

	/**
	Get DLL Version
	@return	Version
	@note	Return 0x1230 in case of Ver.1.230
	*/
	LS9_IF_API DWORD WINAPI LS9IF_GetVersion(void);


	// Establishing/cutting communication path with the controller

	/**
	USB communication connection
	@return	Return code
	*/
	LS9_IF_API LONG WINAPI LS9IF_UsbOpen(void);

	/**
	Connect Ethernet communication
	@param	pstEthernetConfig	Ethernet setting
	@return	Return code
	*/
	LS9_IF_API LONG WINAPI LS9IF_EthernetOpen(LS9IF_ETHERNET_CONFIG* pstEthernetConfig);

	/**
	Cut communication path
	@return	Return code
	*/
	LS9_IF_API LONG WINAPI LS9IF_CommClose(void);


	// System control

	/**
	Reboot controller
	@return	Return code
	*/
	LS9_IF_API LONG WINAPI LS9IF_RebootController(void);

	/**
	Initialize
	@return	Return code
	*/
	LS9_IF_API LONG WINAPI LS9IF_RetrunToFactorySetting(void);


	// Measurement control

	/**
	Start 
	@param	byStatCh	Stat process Ch
	@return	Return code
	*/
	LS9_IF_API LONG WINAPI LS9IF_StatSampStart(BYTE byStatCh);

	/**
	Stop stat process
	@param	byStatCh	Stat process Ch
	@return	Return code
	*/
	LS9_IF_API LONG WINAPI LS9IF_StatSampStop(BYTE byStatCh);

	/**
	Clear stat process
	@param	byStatCh	Stat process Ch
	@return	Return code
	*/
	LS9_IF_API LONG WINAPI LS9IF_StatSampClear(BYTE byStatCh);

	/**
	Auto ZERO
	@param	byOnOff	0:OFF 1:ON
	@param	dwOut	OUT set as a processing target
	@return	Return code
	*/
	LS9_IF_API LONG WINAPI LS9IF_AutoZero(BYTE byOnOff, DWORD dwOut);

	/**
	TIMING
	@param	byOnOff	0:OFF 1:ON
	@param	dwOut	OUT set as a processing target
	@return	Return code
	*/
	LS9_IF_API LONG WINAPI LS9IF_Timing(BYTE byOnOff, DWORD dwOut);

	/**
	RESET
	@param	dwOut	OUT set as a processing target
	@return	Return code
	*/
	LS9_IF_API LONG WINAPI LS9IF_Reset(DWORD dwOut);

	/**
	Sync auto ZERO
	@param	byOnOff	0:OFF 1:ON
	@return	Return code
	*/
	LS9_IF_API LONG WINAPI LS9IF_SyncAutoZero(BYTE byOnOff);

	/**
	Sync TIMING
	@param	byOnOff	0:OFF 1:ON
	@return	Return code
	*/
	LS9_IF_API LONG WINAPI LS9IF_SyncTiming(BYTE byOnOff);

	/**
	Sync RESET
	@return	Return code
	*/
	LS9_IF_API LONG WINAPI LS9IF_SyncReset(void);

	/**
	Clear memory
	@return	Return code
	*/
	LS9_IF_API LONG WINAPI LS9IF_ClearMemory(void);

	/**
	Send setting
	@param	byDepth			Setting value storage hierarchy
	@param	stTargetSetting	Item setting
	@param	lDataSize		Data size
	@param	pbyDatas		Buffer which the setting data to be sent are stored
	@param	pdwError		Detailed error
	@return	Return code
	*/
	LS9_IF_API LONG WINAPI LS9IF_SetSetting(BYTE byDepth, 
		LS9IF_TARGET_SETTING stTargetSetting, 
		LONG lDataSize, 
		BYTE* pbyDatas, 
		DWORD* pdwError);

	/**
	Get setting
	@param	byDepth			Setting value storage hierarchy
	@param	stTargetSetting	Item setting
	@param	pbyDatas		Buffer receiving the acquired setting data
	@param	plDataSize		Data size
	@return	Return code
	*/
	LS9_IF_API LONG WINAPI LS9IF_GetSetting(BYTE byDepth, 
		LS9IF_TARGET_SETTING stTargetSetting, 
		BYTE* pbyDatas, 
		LONG* plDataSize);

	/**
	Reflecting request of the domain for writing settings
	@param	byDepth		Setting value storage hierarchy
	@param	pdwError	Detailed error
	@return	Return code
	*/
	LS9_IF_API LONG WINAPI LS9IF_ReflectSetting(BYTE byDepth, DWORD* pdwError);

	/**
	Updating of the domain for writing settings
	@param	byDepth	Setting value storage hierarchy
	@return	Return code
	*/
	LS9_IF_API LONG WINAPI LS9IF_RewriteTemporarySetting(BYTE byDepth);

	/**
	Check the process status of saving to the domain for saving
	@param	pbyBusy	Saving process status
	@return	Return code
	*/
	LS9_IF_API LONG WINAPI LS9IF_CheckMemoryAccess(BYTE* pbyBusy);

	/**
	Time and date set
	@param	stTime	Time and date
	@return	Return code
	*/
	LS9_IF_API LONG WINAPI LS9IF_SetTime(LS9IF_TIME stTime);

	/**
	Get time and date
	@param	pstTime	Time and date
	@return	Return code
	*/
	LS9_IF_API LONG WINAPI LS9IF_GetTime(LS9IF_TIME* pstTime);

	/**
	Change program
	@param	byProgNo	Program No.
	@return	Return code
	*/
	LS9_IF_API LONG WINAPI LS9IF_ChangeActiveProgram(BYTE byProgNo);

	/**
	Get active program No.
	@param	pbyProgNo	Program No.
	@return	Return code
	*/
	LS9_IF_API LONG WINAPI LS9IF_GetActiveProgram(BYTE* pbyProgNo);

	/**
	Perform calibration of received light intensity
	@param	byHead		Head
	@param	byMode		Mode
	@param	byTarget	Target of calibration of received light intensity
	@return	Return code
	*/
	LS9_IF_API LONG WINAPI LS9IF_LightCalib(BYTE byHead, BYTE byMode, BYTE byTarget);

	/**
	Confirm completion of calibration of received light intensity
	@param	pbyBusyFlg	Whether or not the process has been completed
	@param	pbyErrCode	Error code
	@return	Return code
	*/
	LS9_IF_API LONG WINAPI LS9IF_ConfirmLightCalib(BYTE* pbyBusyFlg, BYTE* pbyErrCode);

	/**
	Get measurement results
	@param	pstMeasureData	Measurement results
	@return	Return code
	*/
	LS9_IF_API LONG WINAPI LS9IF_GetMeasurementValue(LS9IF_MEASURE_DATA* pstMeasureData);

	/**
	Get statistic process data
	@param	byStatCh		Stat process Ch
	@param	pstStatSamp		Statistic process data
	@return	Return code
	*/
	LS9_IF_API LONG WINAPI LS9IF_GetStatSamp(BYTE byStatCh, LS9IF_STAT_SAMP* pstStatSamp);

	/**
	Start storage
	@return	Return code
	*/
	LS9_IF_API LONG WINAPI LS9IF_StartStorage(void);

	/**
	Stop storage
	@return	Return code
	*/
	LS9_IF_API LONG WINAPI LS9IF_StopStorage(void);

	/**
	Get storage status
	@param	pstStorageInfo	Storage status
	@return	Return code
	*/
	LS9_IF_API LONG WINAPI LS9IF_GetStorageStatus(LS9IF_STORAGE_INFO* pstStorageInfo);

	/**
	Get storage data
	@param	dwStartPoint	Start point
	@param	dwReadCount		Read count
	@param	pstStorageData	Storage data
	@return	Return code
	*/
	LS9_IF_API LONG WINAPI LS9IF_GetStorageData(DWORD lStartPoint, DWORD lReadCount, LS9IF_STORAGE_DATA* pstStorageData);
};
#pragma managed(pop)