//#pragma execution_character_set("gbk")
//#pragma execution_character_set("utf-8")

#pragma once

#include <QThread>
#include <QObject>
//#include <QMutex>
#include <opencv2/opencv.hpp>
#include <iostream>

#include "cam_device.h"

using namespace std;
using namespace cv;


class camThread :public QThread
{
	Q_OBJECT

public:
	camThread(cam_device* camera, Mat* image, int index);
	~camThread();

	void run();
	/*
	void getCameraPtr(cam_device* camera);
	void getImagePtr(Mat* image);
	void getCameraIndex(int index);
	void threadPause();
	void threadResume();
	*/
	
signals:
	void mess();
	void Display(const Mat* image, QString source,int displayMode);

private:
	//QMutex m_lock;
	cam_device* cameraPtr = NULL;
	cv::Mat* imgPtr = NULL;
	int camIndex;
};


