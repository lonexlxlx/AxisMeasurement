#pragma once

#include <QThread>
#include <QObject>
#include <iostream>

#include "moveControl.h"

using namespace std;

class axisGoHome_thread  : public QThread
{
	Q_OBJECT

public:
	axisGoHome_thread(moveControl* moveDevicePtr);
	~axisGoHome_thread();

	void run();
	void setGoHomeAxis(int goHomeAxis);//更改回原的轴号
	void goHome(int goHomeAxis);

	int axisNumber;//需要回原点的轴号1-8表示对应轴回原点，其他数字含义参照run()函数相关说明
	long axis1_stopSteplength;//轴一回原后停止位置
	moveControl* moveCardPtr;//运动控制卡指针

signals:
	void GoHomeProgress(QString inf);//返回轴号
	void goHomeFinished(int axisNum,bool normalFlag);
};
