//#pragma execution_character_set("gbk")
//#pragma execution_character_set("utf-8")
#include "cam_device.h"


cam_device::cam_device()
{
	camSn = "";
	isOpenCam = false;
	isOpenStream = false;
	isOffline = false;
	isTrigger = false;
	imgExposeTime = -1;
	exposeTime = 300;
	m_dGain = 0;
	m_dGainMax = 24;
	m_dGainMin = 0;
	m_hDeviceOffline = NULL;
	m_firmName = "";
	m_productName = "";
	m_serialNumName = "";
	m_ipName = "";
	m_maskName = "";
	m_macName = "";
	m_width = 0;
	m_height = 0;
	m_bitDepth = "";
	m_captureMode = "continuous";
	m_triggerMode = "";
	m_triggerSource = "";
};

cam_device::~cam_device()
{};

void cam_device::initInf(gxstring sn, int cam_number)
{
	camSn = sn;
	camNumber = cam_number;
};
void cam_device::openCam()
{
	try
	{
		if (!isOpenCam)
		{
			IGXFactory::GetInstance().Init();
			gxdeviceinfo_vector   m_vectorDeviceInfo;
			IGXFactory::GetInstance().UpdateDeviceList(500, m_vectorDeviceInfo);
			m_objDevicePtr = IGXFactory::GetInstance().OpenDeviceBySN(camSn, GX_ACCESS_EXCLUSIVE);//在头文件的定义中m_objDevicePtr是设备指针指向打开的相机，一共有四种方式打开摄像机如使用ip地址，mac地址等，最后一个参数是打开相机的模式参考开发手册，只读、控制、独占三种，这里是独占
			//上述指针已经指向了打开的设备，所以下文就使用这个指针获取设备控制
			m_objRemoteFeatureControlPtr = m_objDevicePtr->GetRemoteFeatureControl();//m_objDevicePtr这里是头文件中定义的设备控制指针，GetRemoteFeatureControl()获取远端设置层属性控制器，控制远端设备的所有功能
			m_objFeatureControlPtr = m_objDevicePtr->GetFeatureControl();
			uint32_t nStreamNum = m_objDevicePtr->GetStreamCount();//GetStreamCount是获取流通道个数，并且可以看出来nStreamNum是个临时变量，出了这个函数就会被销毁
			if ((nStreamNum > 0) && (!isOpenStream))//还要判断相机流通道已经打开，本文件实际上多次出现了使用这两者进行判断（固定套路），本处是如果流没开
			{
				cout << "流指针执行正常！" << endl;
				m_objStreamPtr = m_objDevicePtr->OpenStream(0);//使用流指针指向相机打开的流，并设定标识流状态的bool值
				isOpenStream = true;
			}
			/*设备信息获取*/
			m_deviceInfo = m_objDevicePtr->GetDeviceInfo();//m_deviceInfo是在库中定义的一种类或者结构体，可以通过调用其中的各种函数获得设备参数
			m_firmName = m_deviceInfo.GetVendorName();
			m_productName = m_deviceInfo.GetModelName();
			m_ipName = m_deviceInfo.GetIP();
			m_maskName = m_deviceInfo.GetSubnetMask();
			m_macName = m_deviceInfo.GetMAC();
			m_width = m_objRemoteFeatureControlPtr->GetIntFeature("Width")->GetValue();
			m_height = m_objRemoteFeatureControlPtr->GetIntFeature("Height")->GetValue();
			m_bitDepth = m_objRemoteFeatureControlPtr->GetEnumFeature("PixelSize")->GetValue();
			m_dGain = m_objRemoteFeatureControlPtr->GetFloatFeature("Gain")->GetValue();
			m_dGainMax = m_objRemoteFeatureControlPtr->GetFloatFeature("Gain")->GetMax();
			m_dGainMin = m_objRemoteFeatureControlPtr->GetFloatFeature("Gain")->GetMin();
			objDeviceClass = m_objDevicePtr->GetDeviceInfo().GetDeviceClass();
			//曝光设置
			m_objRemoteFeatureControlPtr->GetFloatFeature("ExposureTime")->SetValue(exposeTime);
			//触发模式
			m_objRemoteFeatureControlPtr->GetEnumFeature("TriggerSelector")->SetValue("FrameStart");//设置触发源，这个地方不太懂，这个触发源具体指的是什么？
			m_objRemoteFeatureControlPtr->GetEnumFeature("TriggerMode")->SetValue("Off");//设置触发方式，on是外触发，off是内触发
			//自动曝光
			m_objRemoteFeatureControlPtr->GetEnumFeature("ExposureAuto")->SetValue("Off");
			cout << "打开相机执行完了" << camNumber << endl;
		};
	}
	catch (CGalaxyException& e)//这里是捕捉相机出现的错误并输出
	{
		cout << "相机内部错误码: " << e.GetErrorCode() << endl;
		cout << "相机内部错误描述信息: " << e.what() << endl;
		QString camInf = QString("相机%1 打开错误!").arg(camNumber);
		emit cameraErrorInf(camInf);
	}
	catch (std::exception& e)//这里是捕捉程序在运行过程中出现的错误并输出
	{
		cout << "程序错误描述信息: " << e.what() << endl;
		QString camInf = QString("相机%1 打开错误!").arg(camNumber);
		emit cameraErrorInf(camInf);

	};
	isOpenCam = true;
};
void cam_device::startCapture()//"Continuous"连续采集；“Single”单帧采集
{
	try
	{
		if (isOpenCam && isOpenStream)
		{
			if (m_captureMode == "continuous")
			{

				//m_objRemoteFeatureControlPtr->GetEnumFeature("AcquisitionMode")->SetValue("Continuous");这句有可能可以不用设置采集模式
				cout << "startContinuousCapture" << endl;
				m_objStreamPtr->RegisterCaptureCallback(this, NULL);//注册用户采集回调函数（具体作用前面已经说过了）。参数 1 是用户回调对象（this是常值指针，指向当前的这个DH_MER类,通过它可以访问类中的所有成员）；参数 2 是用户私有参数。如果注册了采集回调函数就不能通过外部触发来进行采集了
				m_objStreamPtr->StartGrab();
				m_objRemoteFeatureControlPtr->GetCommandFeature("AcquisitionStart")->Execute();//好像每次使用这种GetCommandFeature("....")->Execute()都是要使用两次
			}
			else
			{
				//单帧采集程序会崩溃，所以不要使用
				m_objStreamPtr->StartGrab();
				m_objRemoteFeatureControlPtr->GetCommandFeature("AcquisitionStart")->Execute();//好像每次使用这种GetCommandFeature("....")->Execute()都是要使用两次
				objSingleImagePtr == m_objStreamPtr->GetImage(500); //超时时间使用60ms
				imgFormatConvert(objSingleImagePtr);
				m_objRemoteFeatureControlPtr->GetCommandFeature("AcquisitionStop")->Execute();
				m_objStreamPtr->StopGrab();
				m_objStreamPtr->Close();
			};
		};
	}
	catch (CGalaxyException& e)//这里是捕捉相机出现的错误并输出
	{
		cout << "相机内部错误码: " << e.GetErrorCode() << endl;
		cout << "相机内部错误描述信息: " << e.what() << endl;
		QString camInf = QString("相机%1 开始采集执行错误(%2)!").arg(camNumber).arg(m_captureMode);
		emit cameraErrorInf(camInf);
	}
	catch (std::exception& e)//这里是捕捉程序在运行过程中出现的错误并输出
	{
		cout << "程序错误描述信息: " << e.what() << endl;
		QString camInf = QString("相机%1 开始采集执行错误(%2)!").arg(camNumber).arg(m_captureMode);
		emit cameraErrorInf(camInf);
	};
};
void cam_device::stopCapture()
{
	try
	{
		if (isOpenStream)
		{
			//发送停采命令
			m_objRemoteFeatureControlPtr->GetCommandFeature("AcquisitionStop")->Execute();
			m_objStreamPtr->StopGrab();
			if (m_captureMode == "continuous")
			{
				m_objStreamPtr->UnregisterCaptureCallback();
			};
		};
	}
	catch (CGalaxyException& e)//这里是捕捉相机出现的错误并输出
	{
		cout << "相机内部错误码: " << e.GetErrorCode() << endl;
		cout << "相机内部错误描述信息: " << e.what() << endl;
		QString camInf = QString("相机%1 结束采集执行错误(%2)!").arg(camNumber).arg(m_captureMode);
		emit cameraErrorInf(camInf);
	}
	catch (std::exception& e)//这里是捕捉程序在运行过程中出现的错误并输出
	{
		cout << "程序错误描述信息: " << e.what() << endl;
		QString camInf = QString("相机%1 结束采集执行错误(%2)!").arg(camNumber).arg(m_captureMode);
		emit cameraErrorInf(camInf);
	};
};
void cam_device::closeCam()
{
	try
	{
		if (isOpenCam)
		{
			cout << "closecam1" << endl;
			if (isOpenStream)
			{
				cout << "closecam2" << endl;
				//发送停采命令
				m_objStreamPtr->Close(); //关闭相机流（关于流的定义见csdn收藏内容）
				isOpenStream = false;
			}
			cout << "closecam3" << endl;
			m_objDevicePtr->Close(); //关闭相机，释放相机资源
			isOpenCam = false;//一般顺序是发送停止采集命令→停止采集流→注销采集回调→关闭相机流
			cout << "closecam4" << endl;
		};
	}
	catch (CGalaxyException& e)
	{
		cout << "错误码: " << e.GetErrorCode() << endl;
		cout << "错误描述信息: " << e.what() << endl;
		QString camInf = QString("相机%1 关闭执行错误!").arg(camNumber);
		emit cameraErrorInf(camInf);
	}
	catch (std::exception& e)
	{
		cout << "错误描述信息: " << e.what() << endl;
		QString camInf = QString("相机%1 关闭执行错误!").arg(camNumber);
		emit cameraErrorInf(camInf);
	};
};
void cam_device::DoOnImageCaptured(CImageDataPointer& objImageDataPointer, void* pUserParam)
{
	imgFormatConvert(objImageDataPointer);
}
void cam_device::setExposeTime(int newTime)
{
	try
	{
		if (newTime >= 0 && newTime <= 30000)
		{
			exposeTime = newTime;
			m_objRemoteFeatureControlPtr->GetFloatFeature("ExposureTime")->SetValue(exposeTime);
			cout << "更新曝光时间执行完了" << exposeTime << endl;
		};
	}
	catch (CGalaxyException& e)
	{
		cout << "错误码: " << e.GetErrorCode() << endl;
		cout << "错误描述信息: " << e.what() << endl;
		QString camInf = QString("相机%1 设置曝光时间执行错误，请检查曝光值是否合理!").arg(camNumber);
		emit cameraErrorInf(camInf);
	}
	catch (std::exception& e)
	{
		cout << "错误描述信息: " << e.what() << endl;
		QString camInf = QString("相机%1 设置曝光时间执行错误，请检查曝光值是否合理!").arg(camNumber);
		emit cameraErrorInf(camInf);
	};
};
void cam_device::setGain(double newGain)
{
	try
	{

		if (newGain >= 0 && newGain <= 24)
		{
			m_dGain = newGain;
			m_objRemoteFeatureControlPtr->GetFloatFeature("Gain")->SetValue(m_dGain);
			cout << "设置曝光执行完了" << m_dGain << endl;
		}
	}
	catch (CGalaxyException& e)
	{
		cout << "错误码: " << e.GetErrorCode() << endl;
		cout << "错误描述信息: " << e.what() << endl;
		QString camInf = QString("相机%1 设置增益执行错误，请检查增益值是否合理!").arg(camNumber);
		emit cameraErrorInf(camInf);
	}
	catch (std::exception& e)
	{
		cout << "错误描述信息: " << e.what() << endl;
		QString camInf = QString("相机%1 设置增益执行错误，请检查增益值是否合理!").arg(camNumber);
		emit cameraErrorInf(camInf);
	};
};

void cam_device::saveImg(string imgPath,int width,int height,int mode)//mode为0表示为彩色图像，mode为1表示灰度图像
{
	cout << "camsave" << imgPath << endl;
	cv::Mat PicCvt;
	if (mode == 0)
	{
		PicCvt = capturedImg;
	}
	else if (mode == 1)
	{
		cvtColor(capturedImg, PicCvt, COLOR_BGR2GRAY);
	}; 
	if (width==0 || height==0)
	{
		imwrite(imgPath, PicCvt);
	}
	else
	{
		cv::Mat resizedPic;
		cv::resize(PicCvt, resizedPic, Size(width, height), 0, 0, INTER_LINEAR);
		imwrite(imgPath, resizedPic);
	}
};

void cam_device::imgFormatConvert(CImageDataPointer objImagePtr)
{
	if (GX_FRAME_STATUS_SUCCESS == objImagePtr->GetStatus())
	{
		//cout << "采集成功" << endl;
		//图像获取为完整帧，可以读取图像宽、高、数据格式等
		capturedImg.create(m_height, m_width, CV_8UC3);
		//对采集到的图像格式进行判断
		GX_PIXEL_FORMAT_ENTRY emPixelFormat = objImagePtr->GetPixelFormat();
		//cout << "采集到的图像格式为" << emPixelFormat << endl;
		void* pRGB24Buffer = NULL;//创建一个转换格式后图像指针
		if (emPixelFormat == GX_PIXEL_FORMAT_MONO8)
		{
			//cout << "图像格式是A" << endl;
			pRGB24Buffer = objImagePtr->ConvertToRGB24(GX_BIT_0_7, GX_RAW2RGB_NEIGHBOUR, true);
		}
		else if (emPixelFormat == GX_PIXEL_FORMAT_MONO10)
		{
			//cout << "图像格式是B" << endl;
			pRGB24Buffer = objImagePtr->ConvertToRGB24(GX_BIT_2_9, GX_RAW2RGB_NEIGHBOUR, true);
		}
		//可以看出来图像格式为与上面的不同，所以下面这个if是我自己加的
		else if (emPixelFormat == GX_PIXEL_FORMAT_BAYER_GR8)
		{
			//cout << "图像格式是c" << endl;
			pRGB24Buffer = objImagePtr->ConvertToRGB24(GX_BIT_0_7, GX_RAW2RGB_NEIGHBOUR, true);
		};
		if (pRGB24Buffer != NULL)
		{
			//cout << "有图像" << endl;
			double d = m_objRemoteFeatureControlPtr->GetFloatFeature("ExposureTime")->GetValue();
			imgExposeTime = int(d);
			//cout << "照片曝光时间为" << imgExposeTime << endl;
			memcpy(capturedImg.data, pRGB24Buffer, (m_width) * (m_height) * 3);
		}
		else
		{
			//cout << "无图像" << endl;
		};
	}
	else
	{
		//cout << "采集失败！" << endl;
	};
};
void cam_device::unInit()
{
	IGXFactory::GetInstance().Uninit();
};
