#pragma once

#include <QThread>
#include <QObject>
#include <iostream>
#include"moveControl.h"
using namespace std;

class moveThread :public QThread
{
	Q_OBJECT;
public:
	moveThread(short index, moveControl* devicePtr);
	~moveThread();
	moveControl* moveCardPtr;//运动控制器指针


	void run();

signals:
	void updateAxisInf(short axisNumber);//

private:
	short axisNumber;//线程对应的轴号
	short threadIndex;//线程索引号

	
};
