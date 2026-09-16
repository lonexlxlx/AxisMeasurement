#pragma once
//#include "StdAfx.h
//using namespace std;
#include <iostream>
#include<windows.h>
#include<WINDEF.H>
#include <QtWidgets/QMainWindow>
#include<map>
#include <LS9_IF.h>
#include "LS9_ErrorCode.h"


class ls_device:public QMainWindow
{
	Q_OBJECT
public:
	ls_device();
	~ls_device();

	//变量
	bool lsOpenflag;
	float lsMeasureResult[4];//用于存储ls的全部测量结果
	std::map <int, QString> lsErrorMap;
	
	void openLs();
	void closeLs();
	bool tryGetLsMeasurementValue(int outNumber, float& value);
	float getLsMeasurementValue(int outNumber);
	bool checkReturnCode(int nRc);
signals:
	void LsErrorInf(QString errorInf);//int
};

