#include "axisGoHome_thread.h"

axisGoHome_thread::axisGoHome_thread(moveControl* moveDevicePtr)
{
	moveCardPtr = moveDevicePtr;
	moveCardPtr->setCurrentAxis(9);
	axis1_stopSteplength = 120000;
};
axisGoHome_thread::~axisGoHome_thread()
{};

void axisGoHome_thread::setGoHomeAxis(int goHomeAxis)
{
	axisNumber = goHomeAxis;
};
void axisGoHome_thread::goHome(int goHomeAxis)
{
	//设置回原轴号
	cout << "gohome" << endl;
	moveCardPtr->setCurrentAxis(goHomeAxis);
	//先回限位
	emit GoHomeProgress(QString("正在回原点：粗糙度相机 正在回限位!").arg(goHomeAxis));
	moveCardPtr->clearStatus();
	moveCardPtr->goToLimit();
	do
	{
		//cout << "回限位状态" << (moveCardPtr->tHomeSts[moveCardPtr->currentAxisIndex].run) << endl;
		moveCardPtr->getToLimitStatus();
		msleep(200);
	} while (moveCardPtr->tHomeSts[moveCardPtr->currentAxisIndex].run);
	msleep(500);
	emit GoHomeProgress(QString("正在回原点：粗糙度相机 正在搜索原点!").arg(goHomeAxis));
	moveCardPtr->zeroPosition();
	cout << "轴" << goHomeAxis << " 回限位完成 " << endl;
	//移动轴，并开启高速捕获
	cout << "去第一个点" << endl;
	moveCardPtr->setMoveMode("Trap");
	moveCardPtr->setTrapPrm(moveCardPtr->axisCore[moveCardPtr->currentAxisIndex], moveCardPtr->currentAxisNumber, moveCardPtr->trapIndexSearch[moveCardPtr->currentAxisIndex], moveCardPtr->trapIndexSearchStepLength[moveCardPtr->currentAxisIndex], moveCardPtr->trapIndexSearchVel[moveCardPtr->currentAxisIndex]);
	moveCardPtr->startTrap();
	msleep(500);
	moveCardPtr->clearStatus();//清除已经触发的限位报警信号
	do
	{
		moveCardPtr->updateAxisStatus(moveCardPtr->currentAxisNumber);
		msleep(200);
	} while (!moveCardPtr->bFlagArrive[moveCardPtr->currentAxisIndex] && moveCardPtr->bFlagMotion[moveCardPtr->currentAxisIndex]);
	msleep(500);
	cout << "第一个点到达" << "停止规划位置:" << (moveCardPtr->dPrfPos[moveCardPtr->currentAxisIndex]) << " 停止实际位置:" << (moveCardPtr->dEncodePos[moveCardPtr->currentAxisIndex])<< endl;
	moveCardPtr->setTrapPrm(moveCardPtr->axisCore[moveCardPtr->currentAxisIndex], moveCardPtr->currentAxisNumber, moveCardPtr->trapMoveToIndex[moveCardPtr->currentAxisIndex], moveCardPtr->trapIndexStepLength[moveCardPtr->currentAxisIndex], moveCardPtr->trapMoveToIndexVel[moveCardPtr->currentAxisIndex]);
	moveCardPtr->startTrigge();
	moveCardPtr->startTrap();
	do
	{
		moveCardPtr->getTriggeStatus();
		//moveCardPtr->updateAxisStatus(moveCardPtr->currentAxisNumber);
	} while (!(moveCardPtr->triggerSts[moveCardPtr->currentAxisIndex].done));
	moveCardPtr->stopMove("smooth","currentAxis");
	msleep(500);
	moveCardPtr->updateAxisStatus(moveCardPtr->currentAxisNumber);
	//轴低速回index
	cout << "回原停止规划位置1:" << (moveCardPtr->dPrfPos[moveCardPtr->currentAxisIndex]) << " 回原停止实际位置1:" << (moveCardPtr->dEncodePos[moveCardPtr->currentAxisIndex])<<" 原点捕获位置：" << (moveCardPtr->indexPos[moveCardPtr->currentAxisIndex]) << endl;
	long stepLength;
	if (goHomeAxis==1|| goHomeAxis==2)
	{
		 stepLength = (moveCardPtr->indexPos[moveCardPtr->currentAxisIndex]) + (moveCardPtr->dPrfPos[moveCardPtr->currentAxisIndex]) - (moveCardPtr->dEncodePos[moveCardPtr->currentAxisIndex]) + 15;
	}
	else if (goHomeAxis == 5)
	{
		 stepLength = (moveCardPtr->indexPos[moveCardPtr->currentAxisIndex]) + (moveCardPtr->dPrfPos[moveCardPtr->currentAxisIndex]) - (moveCardPtr->dEncodePos[moveCardPtr->currentAxisIndex])-10;
	}
	else
	{
		stepLength = (moveCardPtr->indexPos[moveCardPtr->currentAxisIndex]) + (moveCardPtr->dPrfPos[moveCardPtr->currentAxisIndex]) - (moveCardPtr->dEncodePos[moveCardPtr->currentAxisIndex]);
	};
	cout << "原点规划位置（补偿计算）：" << stepLength << "  原点捕获位置(实际位置换算值)：" << (moveCardPtr->indexPos[moveCardPtr->currentAxisIndex]) << endl;
	moveCardPtr->setTrapPrm(moveCardPtr->axisCore[moveCardPtr->currentAxisIndex], moveCardPtr->currentAxisNumber, moveCardPtr->trapMoveToIndex[moveCardPtr->currentAxisIndex], stepLength, moveCardPtr->trapMoveToIndexVel[moveCardPtr->currentAxisIndex]);
	moveCardPtr->startTrap();
	do
	{
		//cout << "正在低速回原点" << endl;
		moveCardPtr->updateAxisStatus(moveCardPtr->currentAxisNumber);
		msleep(200);
	} while (!moveCardPtr->bFlagArrive[moveCardPtr->currentAxisIndex] && moveCardPtr->bFlagMotion[moveCardPtr->currentAxisIndex]);
	moveCardPtr->indexPos[moveCardPtr->currentAxisIndex] = 0;//原点捕获位置再次清零
	msleep(500);
	moveCardPtr->clearStatus();
	moveCardPtr->updateAxisStatus(moveCardPtr->currentAxisNumber);
	cout << "回原停止规划位置1:" << (moveCardPtr->dPrfPos[moveCardPtr->currentAxisIndex]) << " 回原停止实际位置1:" << (moveCardPtr->dEncodePos[moveCardPtr->currentAxisIndex]) << " 原点捕获位置：" << (moveCardPtr->indexPos[moveCardPtr->currentAxisIndex]) << endl;

	moveCardPtr->zeroPosition();
	moveCardPtr->clearStatus();
	if (goHomeAxis == 1)//如果回原的是1轴（粗糙度轴则在向正限位方向退一定角度）
	{
		moveCardPtr->setMoveMode("Trap");
		moveCardPtr->setTrapPrm(moveCardPtr->axisCore[moveCardPtr->currentAxisIndex], moveCardPtr->currentAxisNumber, moveCardPtr->trapIndexSearch[moveCardPtr->currentAxisIndex], axis1_stopSteplength, moveCardPtr->trapHighVelAuto[moveCardPtr->currentAxisIndex]);
		moveCardPtr->startTrap();
		do
		{
			moveCardPtr->updateAxisStatus(moveCardPtr->currentAxisNumber);
			msleep(200);
		} while (!moveCardPtr->bFlagArrive[moveCardPtr->currentAxisIndex] && moveCardPtr->bFlagMotion[moveCardPtr->currentAxisIndex]);
	};
	cout << "轴" << moveCardPtr->currentAxisNumber << " 回原完成 " << endl;
	emit GoHomeProgress(QString("正在回原点：粗糙度相机 回原点完成!").arg(goHomeAxis));
	/*
	do
	{
		moveCardPtr->updateAxisStatus(moveCardPtr->currentAxisNumber);
		msleep(200);
	} while (!moveCardPtr->bFlagArrive[moveCardPtr->currentAxisIndex] && moveCardPtr->bFlagMotion[moveCardPtr->currentAxisIndex]);
	moveCardPtr->getTriggeStatus();
	msleep(500);
	//轴低速回index
	emit GoHomeProgress(QString("正在回原点：轴%1 正在移动至原点!").arg(goHomeAxis));
	moveCardPtr->updateAxisStatus(moveCardPtr->currentAxisNumber);
	cout << "搜索停止规划位置:" << (moveCardPtr->dPrfPos[moveCardPtr->currentAxisIndex]) << "  搜索停止实际位置:" << (moveCardPtr->dEncodePos[moveCardPtr->currentAxisIndex]) << endl;
	long stepLength = (moveCardPtr->indexPos[moveCardPtr->currentAxisIndex]) + (moveCardPtr->dPrfPos[moveCardPtr->currentAxisIndex]) - (moveCardPtr->dEncodePos[moveCardPtr->currentAxisIndex]);
	cout << "原点规划位置（补偿计算）：" << stepLength << "  原点捕获位置(实际位置换算值)：" << (moveCardPtr->indexPos[moveCardPtr->currentAxisIndex]) << endl;
	moveCardPtr->setTrapPrm(moveCardPtr->axisCore[moveCardPtr->currentAxisIndex], moveCardPtr->currentAxisNumber, moveCardPtr->trapMoveToIndex[moveCardPtr->currentAxisIndex], stepLength, moveCardPtr->trapMoveToIndexVel[moveCardPtr->currentAxisIndex]);
	moveCardPtr->startTrap();
	do
	{
		//cout << "正在低速回原点" << endl;
		moveCardPtr->updateAxisStatus(moveCardPtr->currentAxisNumber);
		msleep(200);
	} while (!moveCardPtr->bFlagArrive[moveCardPtr->currentAxisIndex] && moveCardPtr->bFlagMotion[moveCardPtr->currentAxisIndex]);
	moveCardPtr->indexPos[moveCardPtr->currentAxisIndex] = 0;//原点捕获位置再次清零
	msleep(500);
	moveCardPtr->clearStatus();
	moveCardPtr->updateAxisStatus(moveCardPtr->currentAxisNumber);
	cout << "回原点处的规划位置:" << moveCardPtr->dPrfPos[moveCardPtr->currentAxisIndex] << "回原点结束后的实际位置：" << moveCardPtr->dEncodePos[moveCardPtr->currentAxisIndex] << endl;
	moveCardPtr->zeroPosition();
	cout << "轴" << moveCardPtr->currentAxisNumber << " 回原完成 " << endl;
	emit GoHomeProgress(QString("正在回原点：轴%1 回原点完成!").arg(goHomeAxis));
	*/
};

void axisGoHome_thread::run()//
{
	if (axisNumber < 9)//选择了1-8轴中的单轴回原点
	{
		goHome(axisNumber);
		emit GoHomeProgress(QString("粗糙度相机 回原点完成!").arg(axisNumber));
	}
	else if (axisNumber == 9)
	{
		for (int i = 1; i < 3; i++)
		{
			moveCardPtr->setCurrentAxis(i);
			moveCardPtr->clearStatus();
			moveCardPtr->moveEnable();
			moveCardPtr->updateAxisStatus(i);
			int index = i - 1;
			bool axisNormalFlag = (moveCardPtr->bFlagAlarm[index]) || (moveCardPtr->bFlagMError[index]) || (moveCardPtr->bFlagPosLimit[index])
				|| (moveCardPtr->bFlagNegLimit[index]) || (moveCardPtr->bFlagAbruptStop[index]) || !(moveCardPtr->bFlagServoOn[index]);
			if (axisNormalFlag)
			{
				emit GoHomeProgress(QString("粗糙度相机 状态异常，请在手动控制页面处理后再次尝试").arg(i));
				emit goHomeFinished(i, false);
				return;
			};
		};
		for (int i = 5; i < 6; i++)
		{
			moveCardPtr->setCurrentAxis(i);
			moveCardPtr->clearStatus();
			moveCardPtr->moveEnable();
			moveCardPtr->updateAxisStatus(i);
			int index = i - 1;
			bool axisNormalFlag = (moveCardPtr->bFlagAlarm[index]) || (moveCardPtr->bFlagMError[index]) || (moveCardPtr->bFlagPosLimit[index])
				|| (moveCardPtr->bFlagNegLimit[index]) || (moveCardPtr->bFlagAbruptStop[index]) || !(moveCardPtr->bFlagServoOn[index]);
			if (axisNormalFlag)
			{
				emit GoHomeProgress(QString("粗糙度相机 状态异常，请在手动控制页面处理后再次尝试").arg(i));
				emit goHomeFinished(i, false);
				return;
			};
		};
		//i=9表示对1,2,5轴都进行回原点
		goHome(1);
		goHome(2);
		goHome(5);
		emit GoHomeProgress(QString("粗糙度相机、孔径轴、光幕轴回原点完成 !").arg(axisNumber));
	};
	emit goHomeFinished(axisNumber,true);
};
