#pragma once


#include <QThread>
#include <QObject>
#include <QMutex>
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
	bool latestResult(int outputIndex, float& value, qint64& sampledAtMs) const;
	int sleepTime;//循环时间
	float currentResult[4];

private:
	mutable QMutex m_resultMutex;
	bool m_resultValid[4] = { false, false, false, false };
	qint64 m_resultTimestampMs[4] = { 0, 0, 0, 0 };

signals:
	void updateLsResult();
};
