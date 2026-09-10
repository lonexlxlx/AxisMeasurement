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
float ls_device::getLsMeasurementValue(int outNumber)//outNumber1-4表示获取out口对应的结果，为其他值则取出全部的测量结果
{
	LS9IF_MEASURE_DATA* pMeasurementData = new LS9IF_MEASURE_DATA;
	if (!checkReturnCode(LS9IF_GetMeasurementValue(pMeasurementData)))
	{
		return 99999;//LS9IF_MEASURE_DATA*获取测量结果的指针，LS9IF_GetMeasurementValue是获取结果的函数，返回长整形数值（错误码）
	}
	else if(1<=outNumber && outNumber<=4)
	{
		return pMeasurementData->stMesureValue[outNumber - 1].fValue;//将out中的数据传出。
	}
	else
	{
		for (int i = 0; i < 4; i++)
		{
			lsMeasureResult[i]= pMeasurementData->stMesureValue[i].fValue;
		}
		return 8888;
	};
};