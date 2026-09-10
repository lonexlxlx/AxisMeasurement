#pragma once

#define RES_LIMIT                       (8)
#define RES_ALARM                       (8)
#define RES_HOME                        (8)
#define RES_GPI                         (16)
#define RES_ARRIVE                      (8)
#define RES_MPG                         (7)

#define RES_ENABLE                      (8)
#define RES_CLEAR                       (8)
#define RES_GPO                         (16)

#define RES_DAC                         (12)
#define RES_STEP                        (8)
#define RES_PULSE                       (8)
#define RES_ENCODER                     (11)
#define RES_LASER                       (2)  

#define AXIS_MAX                        (8)
#define PROFILE_MAX                     (8)
#define CONTROL_MAX                     (8)

#define PRF_MAP_MAX                     (2)
#define ENC_MAP_MAX                     (2)

typedef struct DiConfig
{
	short active;
	short reverse;
	short filterTime;
} TDiConfig;

typedef struct CountConfig
{
	short active;
	short reverse;
	short filterType;

	short captureSource;
	short captureHomeSense;
	short captureIndexSense;
} TCountConfig;

typedef struct DoConfig
{
	short active;
	short axis;
	short axisItem;
	short reverse;
} TDoConfig;

typedef struct StepConfig
{
	short active;
	short axis;
	short mode;
	short parameter;
	short reverse;
} TStepConfig;

typedef struct DacConfig
{
	short active;
	short control;
	short reverse;
	short bias;
	short limit;
} TDacConfig;

// y=ax+b,y是电压，x是ADC输入
typedef struct AdcConfig
{
	short active;
	short reverse;
	double a;
	double b;
	short filterMode;
} TAdcConfig;

typedef struct ControlConfig
{
	short active;
	short axis;
	short encoder1;
	short encoder2;
	long  errorLimit;
	short filterType[3];
	short encoderSmooth;
	short controlSmooth;
} TControlConfig;

typedef struct ControlConfigEx
{
	short refType;
	short refIndex;

	short feedbackType;
	short feedbackIndex;

	long  errorLimit;
	short feedbackSmooth;
	short controlSmooth;
} TControlConfigEx;

typedef struct ProfileConfig
{
	short  active;
	double decSmoothStop;
	double decAbruptStop;
} TProfileConfig;

typedef struct AxisConfig
{
	short active;
	short alarmType;
	short alarmIndex;
	short limitPositiveType;
	short limitPositiveIndex;
	short limitNegativeType;
	short limitNegativeIndex;
	short smoothStopType;
	short smoothStopIndex;
	short abruptStopType;
	short abruptStopIndex;
	long  prfMap;
	long  encMap;
	short prfMapAlpha[PRF_MAP_MAX];
	short prfMapBeta[PRF_MAP_MAX];
	short encMapAlpha[ENC_MAP_MAX];
	short encMapBeta[ENC_MAP_MAX];
} TAxisConfig;

typedef struct McConfig
{
	TProfileConfig profile[PROFILE_MAX];
	TAxisConfig    axis[AXIS_MAX];
	TControlConfig control[CONTROL_MAX];
	TDacConfig     dac[RES_DAC];
	TStepConfig    step[RES_STEP];
	TCountConfig   encoder[RES_ENCODER];
	TCountConfig   pulse[RES_PULSE];
	TDoConfig      enable[RES_ENABLE];
	TDoConfig      clear[RES_CLEAR];
	TDoConfig      gpo[RES_GPO];
	TDiConfig      limitPositive[RES_LIMIT];
	TDiConfig      limitNegative[RES_LIMIT];
	TDiConfig      alarm[RES_ALARM];
	TDiConfig      home[RES_HOME];
	TDiConfig      gpi[RES_GPI];
	TDiConfig      arrive[RES_ARRIVE];
	TDiConfig      mpg[RES_MPG];
} TMcConfig;

GT_API GT_SaveConfig(char *pFile);
GT_API GT_SetDiConfig(short diType,short diIndex,TDiConfig *pDi);
GT_API GT_GetDiConfig(short diType,short diIndex,TDiConfig *pDi);
GT_API GT_SetDoConfig(short doType,short doIndex,TDoConfig *pDo);
GT_API GT_GetDoConfig(short doType,short doIndex,TDoConfig *pDo);
GT_API GT_SetStepConfig(short step,TStepConfig *pStep);
GT_API GT_GetStepConfig(short step,TStepConfig *pStep);
GT_API GT_SetDacConfig(short dac,TDacConfig *pDac);
GT_API GT_GetDacConfig(short dac,TDacConfig *pDac);
GT_API GT_SetAdcConfig(short adc,TAdcConfig *pAdc);
GT_API GT_GetAdcConfig(short adc,TAdcConfig *pAdc);
GT_API GT_SetCountConfig(short countType,short countIndex,TCountConfig *pCount);
GT_API GT_GetCountConfig(short countType,short countIndex,TCountConfig *pCount);
GT_API GT_SetControlConfig(short control,TControlConfig *pControl);
GT_API GT_GetControlConfig(short control,TControlConfig *pControl);
GT_API GT_SetControlConfigEx(short control,TControlConfigEx *pControl);
GT_API GT_GetControlConfigEx(short control,TControlConfigEx *pControl);
GT_API GT_SetProfileConfig(short profile,TProfileConfig *pProfile);
GT_API GT_GetProfileConfig(short profile,TProfileConfig *pProfile);
GT_API GT_SetAxisConfig(short axis,TAxisConfig *pAxis);
GT_API GT_GetAxisConfig(short axis,TAxisConfig *pAxis);
GT_API GT_ProfileScale(short axis,short alpha,short beta);
GT_API GT_EncScale(short axis,short alpha,short beta);

GT_API GT_EncSns(unsigned short sense);
GT_API GT_LmtSns(unsigned short sense);
GT_API GT_GpiSns(unsigned short sense);
GT_API GT_SetAdcFilter(short adc,short filterTime);


GT_API GT_GetConfigTable(short type,short *pCount);
GT_API GT_GetConfigTableAll();

GT_API GT_SetMcConfig(TMcConfig *pMc);
GT_API GT_GetMcConfig(TMcConfig *pMc);

GT_API GT_SetMcConfigToFile(TMcConfig *pMc,char *pFile);
GT_API GT_GetMcConfigFromFile(TMcConfig *pMc,char *pFile);

GT_API GT_LoadConfigEx(short core,char *pFile);
GT_API GT_SaveConfigEx(short core,char *pFile);
GT_API GT_LoadModuleConfig(short core,char *pFile);
GT_API GT_SaveModuleConfig(short core,char *pFile);

GT_API GTN_SaveConfig(short core,char *pFile);
GT_API GTN_SetDiConfig(short core,short diType,short diIndex,TDiConfig *pDi);
GT_API GTN_GetDiConfig(short core,short diType,short diIndex,TDiConfig *pDi);
GT_API GTN_SetDoConfig(short core,short doType,short doIndex,TDoConfig *pDo);
GT_API GTN_GetDoConfig(short core,short doType,short doIndex,TDoConfig *pDo);
GT_API GTN_SetStepConfig(short core,short step,TStepConfig *pStep);
GT_API GTN_GetStepConfig(short core,short step,TStepConfig *pStep);
GT_API GTN_SetDacConfig(short core,short dac,TDacConfig *pDac);
GT_API GTN_GetDacConfig(short core,short dac,TDacConfig *pDac);
GT_API GTN_SetAdcConfig(short core,short adc,TAdcConfig *pAdc);
GT_API GTN_GetAdcConfig(short core,short adc,TAdcConfig *pAdc);
GT_API GTN_SetCountConfig(short core,short countType,short countIndex,TCountConfig *pCount);
GT_API GTN_GetCountConfig(short core,short countType,short countIndex,TCountConfig *pCount);
GT_API GTN_SetControlConfig(short core,short control,TControlConfig *pControl);
GT_API GTN_GetControlConfig(short core,short control,TControlConfig *pControl);
GT_API GTN_SetControlConfigEx(short core,short control,TControlConfigEx *pControl);
GT_API GTN_GetControlConfigEx(short core,short control,TControlConfigEx *pControl);
GT_API GTN_SetProfileConfig(short core,short profile,TProfileConfig *pProfile);
GT_API GTN_GetProfileConfig(short core,short profile,TProfileConfig *pProfile);
GT_API GTN_SetAxisConfig(short core,short axis,TAxisConfig *pAxis);
GT_API GTN_GetAxisConfig(short core,short axis,TAxisConfig *pAxis);
GT_API GTN_ProfileScale(short core,short axis,short alpha,short beta);
GT_API GTN_EncScale(short core,short axis,short alpha,short beta);
typedef struct
{
	short active;
	short checkError;
	short linkError;
	short packageErrorCount;
	short pad[8];
} TExtModuleStatus;

typedef struct
{
	short type;
	short input;
	short output;
} TExtModuleType;

typedef struct
{
	short station;
	short module;
	short index;
} TExtIoMap;

GT_API GT_LoadExtModuleConfig(short core,char *pFile);
GT_API GT_SaveExtModuleConfig(short core,char *pFile);
GT_API GT_ExtModuleOn(short core,short station);
GT_API GT_ExtModuleOff(short core,short station);
GT_API GT_GetExtModuleStatus(short core,short station,TExtModuleStatus *pStatus);
GT_API GT_SetExtModuleId(short core,short station,short count,short *pId);
GT_API GT_GetExtModuleId(short core,short station,short count,short *pId);
GT_API GT_SetExtModuleReverse(short core,short station,short module,short inputCount,short *pInputReverse,short outputCount,short *pOutputReverse);
GT_API GT_GetExtModuleReverse(short core,short station,short module,short inputCount,short *pInputReverse,short outputCount,short *pOutputReverse);
GT_API GT_GetExtModuleCount(short core,short station,short *pCount);
GT_API GT_GetExtModuleType(short core,short station,short module,TExtModuleType *pModuleType);
GT_API GT_SetExtIoMap(short core,short type,short index,TExtIoMap *pMap);
GT_API GT_GetExtIoMap(short core,short type,short index,TExtIoMap *pMap);
GT_API GT_ClearExtIoMap(short core,short type);
GT_API GT_SetExtAoRange(short core,short index,double max,double min);
GT_API GT_GetExtAoRange(short core,short index,double *pMax,double *pMin);
GT_API GT_SetExtAiRange(short core,short index,double max,double min);
GT_API GT_GetExtAiRange(short core,short index,double *pMax,double *pMin);

GT_API GTN_LoadExtModuleConfig(short core,char *pFile);
GT_API GTN_SaveExtModuleConfig(short core,char *pFile);
GT_API GTN_SaveRingNetConfig(short core,char *pFile);
GT_API GTN_ExtModuleOn(short core,short station);
GT_API GTN_ExtModuleOff(short core,short station);
GT_API GTN_GetExtModuleStatus(short core,short station,TExtModuleStatus *pStatus);
GT_API GTN_SetExtModuleId(short core,short station,short count,short *pId);
GT_API GTN_GetExtModuleId(short core,short station,short count,short *pId);
GT_API GTN_SetExtModuleReverse(short core,short station,short module,short inputCount,short *pInputReverse,short outputCount,short *pOutputReverse);
GT_API GTN_GetExtModuleReverse(short core,short station,short module,short inputCount,short *pInputReverse,short outputCount,short *pOutputReverse);
GT_API GTN_GetExtModuleCount(short core,short station,short *pCount);
GT_API GTN_GetExtModuleType(short core,short station,short module,TExtModuleType *pModuleType);
GT_API GTN_SetExtIoMap(short core,short type,short index,TExtIoMap *pMap);
GT_API GTN_GetExtIoMap(short core,short type,short index,TExtIoMap *pMap);
GT_API GTN_ClearExtIoMap(short core,short type);
GT_API GTN_SetExtAoRange(short core,short index,double max,double min);
GT_API GTN_GetExtAoRange(short core,short index,double *pMax,double *pMin);
GT_API GTN_SetExtAiRange(short core,short index,double max,double min);
GT_API GTN_GetExtAiRange(short core,short index,double *pMax,double *pMin);


struct TScanCommandMotion
{
	long segmentNumber;
	short x;
	short y;
	long deltaX;
	long deltaY;
	long vel;
	long acc;
};

struct TScanCommandMotionDelay
{
	long delay;
};

struct TScanCommandDo
{
	short doType;
	short doMask;
	short doValue;
};

struct TScanCommandDoDelay
{
	long delay;
};

struct TScanCommandLaser
{
	short mask;
	short value;
} ;

struct TScanCommandLaserDelay
{
	long laserOnDelay;
	long laserOffDelay;
} ;

struct TScanCommandLaserPower
{
	long power;
} ;

struct TScanCommandLaserFrequency
{
	long frequency;
} ;

struct TScanCommandLaserPulseWidth
{
	long pulseWidth;
} ;

struct TScanCommandDa
{
	short daIndex;
	short daValue;
	double voltage;
} ;

GT_API GT_ScanCrdData(short code ,void *pScanCrdData,short crd=0,short port=1);

typedef struct
{
	short module;
	short fifo;
} TScanMap;

GT_API GT_SetScanMap(short core,short index,TScanMap *pScanMap);
GT_API GT_GetScanMap(short core,short index,TScanMap *pScanMap);
GT_API GT_ClearScanMap(short core);
GT_API GT_UpdateScanMap(short core);

GT_API GTN_SetScanMap(short core,short index,TScanMap *pMap);
GT_API GTN_GetScanMap(short core,short index,TScanMap *pMap);
GT_API GTN_ClearScanMap(short core);
GT_API GTN_UpdateScanMap(short core);

typedef struct
{
	short module;
	short fifo;
} TPosCompareMap;

GT_API GT_SetPosCompareMap(short core,short index,TPosCompareMap *pPosCompare);
GT_API GT_GetPosCompareMap(short core,short index,TPosCompareMap *pPosCompare);
GT_API GT_ClearPosCompareMap(short core);

GT_API GTN_SetPosCompareMap(short core,short index,TPosCompareMap *pMap);
GT_API GTN_GetPosCompareMap(short core,short index,TPosCompareMap *pMap);
GT_API GTN_ClearPosCompareMap(short core);

