
#pragma once

#include <QtWidgets/QMainWindow>
#include <opencv2/opencv.hpp>
#include <iostream>

#include "GalaxyIncludes.h"


using namespace cv;
using namespace std;

class cam_device :public QMainWindow,public ICaptureEventHandler//, public IDeviceOfflineEventHandler, public ICaptureEventHandler
{
	Q_OBJECT
signals:
	void cameraErrorInf(QString errorInf);
public:

	cam_device();
	~cam_device();

	void initInf(gxstring sn, int cam_number);
	void openCam();
	void startCapture();
	void stopCapture();
	void imgFormatConvert(CImageDataPointer objImagePtr);//对采集到的图像进行转换
	void closeCam();
	void DoOnImageCaptured(CImageDataPointer& objImageDataPointer, void* pUserParam);
	void setExposeTime(int newTime);
	void setGain(double newGain);
	void saveImg(string imgPath,int width, int height,int mode);
	void unInit();//释放所有打开的资源所有相机执行一次即可

	gxstring      camSn;//相机Sn号
	int           camNumber;//相机序号

	//流指针，指向相机打开的流
	CGXDevicePointer m_objDevicePtr; //设备指针，指向打开的相机
	CImageDataPointer objSingleImagePtr;//采集单帧时的图像指针
	GX_DEVICE_OFFLINE_CALLBACK_HANDLE m_hDeviceOffline; //掉线事件
	GX_DEVICE_CLASS_LIST objDeviceClass;//设备类型
	CGXDeviceInfo m_deviceInfo;//设备信息
	string m_firmName;//厂商名称
	string m_productName;//设备型号
	string m_serialNumName;//序列号
	string m_ipName;//ip地址
	string m_maskName;//掩码
	string m_macName;//MAC地址
	string m_bitDepth;
	cv::Mat capturedImg; //相机采集的图片
	int m_width;//相机采集图像的尺寸（这里注意和图片中的尺寸进行区分）
	int m_height;
	bool isOpenCam; //相机是否打开
	bool isOpenStream; //相机流是否打开
	bool isOffline; //设备是否掉线
	bool isTrigger; //相机是否触发

	GX_PIXEL_FORMAT_ENTRY emPixelFormat;//相机采集到的图像格式
	int imgExposeTime;//照片曝光时间
	int exposeTime; //相机曝光时间
	double m_dGain;
	double m_dGainMax;//相机可以设置的最大增益
	double m_dGainMin;//相机可以设置的最小增益
	QString m_captureMode;//相机采集模式"continuous"/"single"
	string m_triggerMode;
	string m_triggerSource;

private:

	CGXFeatureControlPointer m_objRemoteFeatureControlPtr; //（远端）设备属性控制器指针
	CGXFeatureControlPointer m_objFeatureControlPtr; //本地和流属性控制器
	CGXStreamPointer m_objStreamPtr;
	CFloatFeaturePointer objFloatPtr;


	class CSampleDeviceOfflineEventHandler : public IDeviceOfflineEventHandler
	{
	public:
		void DoOnDeviceOfflineEvent(void* pUserParam)
		{
			cout << "收到设备掉线事件！" << endl;
		};
	};

	// 用户继承属性更新事件处理类
	class CSampleFeatureEventHandler : public IFeatureEventHandler
	{
	public:
		void DoOnFeatureEvent(const GxIAPICPP::gxstring& strFeatureName, void* pUserParam)
		{
			cout << "收到曝光结束事件" << endl;
		}
	};


};
