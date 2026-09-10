#include "lsThread.h"

lsThread::lsThread(ls_device* lsDevice)
{
	lsPtr = lsDevice;
	sleepTime = 300;
};

lsThread::~lsThread()
{
	terminate();
	if (lsPtr != NULL)
	{
		delete lsPtr;
	};
};
void lsThread::run()
{
	if (lsPtr == NULL)
	{
		return;
	};
	while (!isInterruptionRequested())
	{
		for (int i = 0; i < 4; i++)
		{
			currentResult[i] = lsPtr->getLsMeasurementValue(i+1);
		};
		
		emit updateLsResult();
		msleep(sleepTime);
	};
};

