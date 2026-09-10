#pragma once


#include <QThread>
#include <QObject>
#include<iostream>
#include"ls_device.h"

using namespace std;

class lsThread : public QThread
{
	Q_OBJECT


public:
	lsThread(ls_device* lsDevice);
	~lsThread();
	ls_device* lsPtr;

	void run();
	int sleepTime;//循环时间
	float currentResult[4];

signals:
	void updateLsResult();
};
