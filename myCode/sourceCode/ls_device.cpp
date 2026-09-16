#include "ls_device.h"

ls_device::ls_device()
{
	lsOpenflag = false;
	lsErrorMap[0x0000] = "Normal completion";//初始化错误代码地图
	lsErrorMap[0x1000] = "The operation to open the communication path failed.";
	lsErrorMap[0x1001] = "The communication path has not been established";
	lsErrorMap[0x1002] = "The operation to send a command failed";
	lsErrorMap[0x1003] = "The operation to receive a response failed";
	lsErrorMap[0x1004] = "A time-out occurred while receiving a response.(*)";
	lsErrorMap[0x1005] = "The operation to allocate memory failed.";
	lsErrorMap[0x1006] = "An invalid parameter was passed.";
	lsErrorMap[0x1007] = "The received response data is invalid.";
	lsErrorMap[0x100A] = "The USB interface is already open.";
};
ls_device::~ls_device()
{

};
void ls_device::openLs()
{
	lsOpenflag = checkReturnCode(LS9IF_UsbOpen());
};
void ls_device::closeLs()
{	
	if (lsOpenflag)
	{
		lsOpenflag = checkReturnCode(LS9IF_CommClose());
		lsOpenflag = false;
	}	
};
bool ls_device::checkReturnCode(int nRc)//c++中int是32位与库中的LONG数据对应
{
	if (nRc != (int)LS9IF_RC_OK)//返回代码的定义在p34页
	{
		emit LsErrorInf(lsErrorMap[nRc]);//lsErrorMap[nRc]nRc
		return false;
	};
	return true;
};
bool ls_device::tryGetLsMeasurementValue(int outNumber, float& value)
{
	if (outNumber < 1 || outNumber > 4 || !lsOpenflag) return false;
	LS9IF_MEASURE_DATA measurementData = {};
	if (!checkReturnCode(LS9IF_GetMeasurementValue(&measurementData))) return false;
	value = measurementData.stMesureValue[outNumber - 1].fValue;
	return true;
}

float ls_device::getLsMeasurementValue(int outNumber)//outNumber1-4表示获取out口对应的结果，为其他值则取出全部的测量结果
{
	float value = 0;
	if (tryGetLsMeasurementValue(outNumber, value)) return value;
	if (outNumber < 1 || outNumber > 4) {
		LS9IF_MEASURE_DATA measurementData = {};
		if (!lsOpenflag || !checkReturnCode(LS9IF_GetMeasurementValue(&measurementData))) return 99999;
		for (int i = 0; i < 4; ++i) lsMeasureResult[i] = measurementData.stMesureValue[i].fValue;
		return 8888;
	}
	return 99999;
};
