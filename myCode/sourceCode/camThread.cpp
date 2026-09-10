//#pragma execution_character_set("gbk")
//#pragma execution_character_set("utf-8")

#include "camThread.h"

camThread::camThread(cam_device* camera, Mat* image, int index)
{
	cameraPtr = camera;
	imgPtr = image;
	camIndex = index;
};
camThread::~camThread()
{
	terminate();
	if (cameraPtr != NULL)
	{
		delete cameraPtr;
	}
	if (imgPtr != NULL)
	{
		delete imgPtr;
	}
};
void camThread::camThread::run()
{
	if ((cameraPtr == NULL) || (imgPtr == NULL))
	{
		return;
	}
	while (!isInterruptionRequested())
	{
		//std::cout << "Thread_Trigger:" << cameraPtr->softTrigger() << std::endl;
		//std::cout << "Thread_Readbuffer:" << cameraPtr->ReadBuffer(*imagePtr) << std::endl;
		/*emit mess();*/
		//m_lock.lock();这里互斥锁也可以暂时不需要了
		//cout << "线程正在运行" << endl;
		emit Display(imgPtr,QString("cam%1").arg(camIndex),0);//发送信号 img_display_label接收并显示
		msleep(300);
		//m_lock.unlock();
	}
};
/*
void camThread::getCameraPtr(cam_device* camera)
{
	cameraPtr = camera;
};
void camThread::getImagePtr(Mat* image)
{
	imgPtr = image;
};
void camThread::getCameraIndex(int index)
{
	camIndex = index;
};

//下面两个函数不需要了

void camThread::threadPause()
{
	m_lock.lock();
};
void camThread::threadResume()
{
	m_lock.unlock();
};
*/
