#include "moveThread.h"

moveThread::moveThread(short index, moveControl* devicePtr)
{
	threadIndex =index;
	axisNumber = index + 1;
	moveCardPtr=devicePtr;
};
moveThread::~moveThread()
{
	terminate();
};
void moveThread::run()
{
	while (!isInterruptionRequested())
	{
		moveCardPtr->updateAxisStatus(axisNumber);
		//moveControl::updateAxisStatus(axisNumber);
		emit updateAxisInf(axisNumber);//发送信号
		msleep(200);
	};
};