//WZ20-15-001-80****************************************************************************


#include "program_1.h"

program_1::program_1(cam_device* cam0, cam_device* cam1, cam_device* cam2, moveControl* cradDevicePtr, ls_device* lsDevicePtr, QTableWidget* tablePtr, QSqlDatabase* DbPtr, roughnessFun* roughnessObjPtr)
{
	//指针初始化
	camPtrList[0] = cam0;
	camPtrList[1] = cam1;
	camPtrList[2] = cam2;
	moveControlPtr = cradDevicePtr;
	ls_devicePtr = lsDevicePtr;
	measureTablePtr = tablePtr;
	databasePtr = DbPtr;
	m_roughnessObjPtr = roughnessObjPtr;

	//其他参数初始化
	operatorName = "未输入";
	partsID = "为输入";
	stime = "";
	allFeatureFlag = "--";
	ngFeatureNum = 0;//NG特征数量
	measurePartsNum_all = 0;//该型号轴测量总数
	measurePartsNum_ok = 0;//该型号轴合格数量
	measurePartsNum_ng = 0;//该型号轴NG数量
	yield = 0;//该型号轴合格率

	//视觉检测算法参数初始化
	ImageRowSize_Cali = ImageRowSize * Calik;//图像换算后行距离
	ImageColSize_Cali = ImageColSize * Calik;//图像换算后列距离

	//跳动数据结构体初始化
	
	for (int i = 0; i < 1; i++)
	{
		myScrewRoundoutDataPtr[i]= new  screwRoundoutData();
	};
	//clearRoundoutData();
	clearScrewRoundOutData();
};
program_1::~program_1()
{
	terminate();
	if (originalImgPtr != NULL)
	{
		delete originalImgPtr;
	};
	if (processedImgPtr != NULL)
	{
		delete processedImgPtr;
	};
	if (ls_devicePtr != NULL)
	{
		delete ls_devicePtr;
	};
	if (moveControlPtr != NULL)
	{
		delete moveControlPtr;
	};
	for (int i = 0; i < 3; i++)
	{
		if (camPtrList[i] != NULL)
		{
			delete camPtrList[i];
		};
	};
};

void program_1::lsSensorMeasure_diameter(int positionNumber, int time)
{
	//使用光幕传感器进行直径测量
	moveControlPtr->setTrapPrm(moveControlPtr->axisCore[moveControlPtr->currentAxisIndex], moveControlPtr->currentAxisNumber, moveControlPtr->trapAuto[moveControlPtr->currentAxisIndex], ls_StepLength_axis5[positionNumber], moveControlPtr->trapHighVelAuto[moveControlPtr->currentAxisIndex]);
	moveControlPtr->startTrap();
	do
	{
		moveControlPtr->updateAxisStatus(moveControlPtr->currentAxisNumber);
		msleep(200);
	} while (!moveControlPtr->bFlagArrive[moveControlPtr->currentAxisIndex] && moveControlPtr->bFlagMotion[moveControlPtr->currentAxisIndex]);
	msleep(300);
	if (time == 1)
	{
		lsDiameterResult[positionNumber][0] = diameter_compensation( ls_devicePtr->getLsMeasurementValue(1)); //获取光幕传感器OUT1直径测量口的值
		//lsDiameterResult[positionNumber][7] = lsDiameterResult[positionNumber][0] / 2 + (ls_devicePtr->getLsMeasurementValue(4));//计算第1次的实际回转半径
		//lsDiameterResult[positionNumber][7] = lsDiameterResult[positionNumber][0] / 2 + (ls_devicePtr->getLsMeasurementValue(4));//计算第1次的实际回转半径
		emit lsResult(positionNumber + 1, lsDiameterResult[positionNumber][0]);
	}
	else
	{
		lsDiameterResult[positionNumber][1] =  diameter_compensation( ls_devicePtr->getLsMeasurementValue(1)); ;
		//lsDiameterResult[positionNumber][8] = lsDiameterResult[positionNumber][1] / 2 + (ls_devicePtr->getLsMeasurementValue(4));//计算第2次的实际回转半径
		emit lsResult(positionNumber + 1, lsDiameterResult[positionNumber][1]);
	};
};
//跳动测量相关函数******************************************************************************************************************************************************************************************************************************


//圆度测量相关函数******************************************************************************************************************************************************************************************************************************

/*
//该零件不需要测跳动
void program_1::lsSensorMeasure_roundness(int positionNumber, float intervalTime=50)//使用光幕传感器进行圆度测量
{
	// 初始化变量
	vector<float> diameter;
	//移动到位置读取光幕轴到边缘距离
	moveControlPtr->setCurrentAxis(5);
	moveControlPtr->setMoveMode("Trap");
	moveControlPtr->setTrapPrm(moveControlPtr->axisCore[moveControlPtr->currentAxisIndex], moveControlPtr->currentAxisNumber, moveControlPtr->trapAuto[moveControlPtr->currentAxisIndex], roundness_axis5_steplength[positionNumber], moveControlPtr->trapHighVelAuto[moveControlPtr->currentAxisIndex]);
	moveControlPtr->startTrap();
	do
	{
		moveControlPtr->updateAxisStatus(moveControlPtr->currentAxisNumber);
		msleep(200);
	} while (!moveControlPtr->bFlagArrive[moveControlPtr->currentAxisIndex] && moveControlPtr->bFlagMotion[moveControlPtr->currentAxisIndex]);
	long axis7_steplength = 360 * 500;//一个脉冲是0.002度;
	moveControlPtr->setCurrentAxis(7);
	moveControlPtr->zeroPosition();
	moveControlPtr->setMoveMode("Trap");
	moveControlPtr->setTrapPrm(moveControlPtr->axisCore[moveControlPtr->currentAxisIndex], moveControlPtr->currentAxisNumber, moveControlPtr->trapAuto[moveControlPtr->currentAxisIndex], axis7_steplength, moveControlPtr->trapHighVelAuto[moveControlPtr->currentAxisIndex]);
	moveControlPtr->startTrap();
	do
	{
		diameter.push_back(ls_devicePtr->getLsMeasurementValue(1));
		moveControlPtr->updateAxisStatus(moveControlPtr->currentAxisNumber);
		msleep(intervalTime);
	} while (moveControlPtr->bFlagMotion[moveControlPtr->currentAxisIndex]);
	//使用3西格玛准则对直径数据滤波
	double dataLength = diameter.size();//计算均值方差值范围
	double mean_diameter= (accumulate(diameter.begin(), diameter.end(), 0) / dataLength);
	double variance_diameter = 0;
	//cout << "原始圆度数据" << endl;
	for (int i = 0; i < dataLength; i++)
	{
		variance_diameter = variance_diameter + pow(diameter[i] - mean_diameter, 2);
		//cout << i + 1 << "  直径:" << diameter[i] << endl;
	};
	variance_diameter = variance_diameter / dataLength;
	double standardDeviation_diameter = sqrt(variance_diameter);
	double lowerBound_diameter = mean_diameter - 3 * standardDeviation_diameter;
	double upperBound_diameter = mean_diameter + 3 * standardDeviation_diameter;
	//cout << "均值" << mean_diameter << " 方差" << variance_diameter << " 标准差" << standardDeviation_diameter << " 上限" << upperBound_diameter << " 下限" << lowerBound_diameter << endl;
	vector<float>::iterator it;//剔除超范围数据
	for (it = diameter.begin(); it != diameter.end();)
	{
		if (*it >= upperBound_diameter || *it <= lowerBound_diameter)
		{
			it = diameter.erase(it);
		}
		else
		{
			++it;
		}
	};
	float maxDiameter, minDiameter;
	maxDiameter = *max_element(diameter.begin(), diameter.end());//圆度计算
	minDiameter = *min_element(diameter.begin(), diameter.end());
	roundnessResult[positionNumber][1] = (maxDiameter- minDiameter) / 2;
	pushback_vectors(roundnessResult[positionNumber][1], 10, roundnessResult[positionNumber][0], 0, 0, roundnessResult[positionNumber][2]);
	cout << positionNumber + 1 << "圆度测量数据:" << roundnessResult[positionNumber][1] << endl;
	//手动释放动态数组
	diameter.clear();
	diameter.shrink_to_fit();
};
*/

//基础函数***********************************************************************************************************************************************************************************************************************************************


string program_1::cam0_Measure_prepare(int positionNumber)
{
	
	moveControlPtr->setCurrentAxis(5);
	moveControlPtr->setMoveMode("Trap");
	moveControlPtr->setTrapPrm(moveControlPtr->axisCore[moveControlPtr->currentAxisIndex], moveControlPtr->currentAxisNumber, moveControlPtr->trapAuto[moveControlPtr->currentAxisIndex], cam0_StepLength_axis5[positionNumber], moveControlPtr->trapHighVelAuto[moveControlPtr->currentAxisIndex]);
	moveControlPtr->startTrap();
	do
	{
		moveControlPtr->updateAxisStatus(moveControlPtr->currentAxisNumber);
		msleep(200);
	} while (!moveControlPtr->bFlagArrive[moveControlPtr->currentAxisIndex] && moveControlPtr->bFlagMotion[moveControlPtr->currentAxisIndex]);
	camPtrList[0]->startCapture();
	string imgPath = dataSavePath + "\\cam0_" + to_string(positionNumber + 1) + ".bmp";
	cout << "图像地址" << imgPath <<endl;
	msleep(2000);
	moveControlPtr->updateAxisStatus(moveControlPtr->currentAxisNumber);
	camPtrList[0]->stopCapture();
	cam0_arriveOrgEncode_axis5[positionNumber] = moveControlPtr->dEncodePos[4];//将远心相机拍摄点位的实际编码器位置保存下来
	emit imgInf(originalImgPtr, "org", 0);
	camPtrList[0]->saveImg(imgPath, 0, 0, 0);
	cout << " cam0_Measure_prepare点位 " << positionNumber + 1 << " 执行完毕 " << endl;
	replace(imgPath.begin(), imgPath.end(), '\\', '/');
	return imgPath;
};
void program_1::cam0_roundoutMeasure_prepare(int positionNumber)
{
	//trap5轴
	moveControlPtr->setCurrentAxis(5);
	moveControlPtr->setMoveMode("Trap");
	moveControlPtr->setTrapPrm(moveControlPtr->axisCore[moveControlPtr->currentAxisIndex], moveControlPtr->currentAxisNumber, moveControlPtr->trapAuto[moveControlPtr->currentAxisIndex], screwPrepare[positionNumber][0], moveControlPtr->trapHighVelAuto[moveControlPtr->currentAxisIndex]);
	moveControlPtr->startTrap();
	do
	{
		moveControlPtr->updateAxisStatus(moveControlPtr->currentAxisNumber);
		msleep(200);
	} while (!moveControlPtr->bFlagArrive[moveControlPtr->currentAxisIndex] && moveControlPtr->bFlagMotion[moveControlPtr->currentAxisIndex]);
	camPtrList[0]->startCapture();
	emit programTips("相机已经到达采集位置，请确保机心夹安装正确。并且调整转台位置使零件对正！", 4);
	programConfirm = "doNotContinue";
	do
	{
		msleep(350);
		emit imgInf(originalImgPtr, "org", 0);
	} while (programConfirm == "doNotContinue");

	long axis7_stepIncrease = 360 / screwPrepare[positionNumber][3] * 500;
	long axis7_steplength = 0;
	moveControlPtr->setCurrentAxis(7);
	moveControlPtr->zeroPosition();
	moveControlPtr->setMoveMode("Trap");
	for (int i = 0; i < screwPrepare[positionNumber][3]; i++)
	{
		cout<<999999999999999<< endl;
		axis7_steplength = (i + 1) * axis7_stepIncrease;
		moveControlPtr->setTrapPrm(moveControlPtr->axisCore[moveControlPtr->currentAxisIndex], moveControlPtr->currentAxisNumber, moveControlPtr->trapAuto[moveControlPtr->currentAxisIndex], axis7_steplength, moveControlPtr->trapHighVelAuto[moveControlPtr->currentAxisIndex]);
		moveControlPtr->startTrap();
		cout << "七轴步长" << i + 1 << axis7_steplength << endl;
		do
		{
			moveControlPtr->updateAxisStatus(moveControlPtr->currentAxisNumber);
			msleep(200);
		} while (moveControlPtr->bFlagMotion[moveControlPtr->currentAxisIndex]);
		axis7_steplength = (i + 1) * axis7_stepIncrease;
		string imgPath = dataSavePath + "\\cam1_screw" + to_string(positionNumber + 1) + "_" + to_string(i + 1) + ".bmp";
		cout <<"螺纹地址：" << imgPath << endl;
		camPtrList[0]->saveImg(imgPath, 0, 0, 0);
		emit imgInf(originalImgPtr, "org", 0);
		replace(imgPath.begin(), imgPath.end(), '\\', '/');
		//cout << imgPath << endl;
		myScrewRoundoutDataPtr[positionNumber]->screwImgPath.push_back(imgPath);
	};
	camPtrList[0]->stopCapture();
	//移动5轴
	moveControlPtr->setCurrentAxis(5);
	moveControlPtr->setMoveMode("Trap");
	moveControlPtr->setTrapPrm(moveControlPtr->axisCore[moveControlPtr->currentAxisIndex], moveControlPtr->currentAxisNumber, moveControlPtr->trapAuto[moveControlPtr->currentAxisIndex], screwPrepare[positionNumber][1], moveControlPtr->trapHighVelAuto[moveControlPtr->currentAxisIndex]);
	moveControlPtr->startTrap();
	do
	{
		moveControlPtr->updateAxisStatus(moveControlPtr->currentAxisNumber);
		msleep(200);
	} while (moveControlPtr->bFlagMotion[moveControlPtr->currentAxisIndex]);
	//七轴旋转
	axis7_steplength = 0;
	moveControlPtr->setCurrentAxis(7);
	moveControlPtr->zeroPosition();
	moveControlPtr->setMoveMode("Trap");
	for (int i = 0; i < screwPrepare[positionNumber][3]; i++)
	{
		axis7_steplength = (i + 1) * axis7_stepIncrease;
		moveControlPtr->setTrapPrm(moveControlPtr->axisCore[moveControlPtr->currentAxisIndex], moveControlPtr->currentAxisNumber, moveControlPtr->trapAuto[moveControlPtr->currentAxisIndex], axis7_steplength, moveControlPtr->trapHighVelAuto[moveControlPtr->currentAxisIndex]);
		moveControlPtr->startTrap();
		do
		{
			moveControlPtr->updateAxisStatus(moveControlPtr->currentAxisNumber);
			msleep(200);
		} while (moveControlPtr->bFlagMotion[moveControlPtr->currentAxisIndex]);
		
		cout << "光幕旋转7轴步长" << axis7_steplength <<endl;
		
		float a=ls_devicePtr->getLsMeasurementValue(5);
		cout << ls_devicePtr->lsMeasureResult[0] << endl;
		cout << ls_devicePtr->lsMeasureResult[1] << endl;
		cout << ls_devicePtr->lsMeasureResult[2] << endl;
		cout << ls_devicePtr->lsMeasureResult[3] << endl;
		
		myScrewRoundoutDataPtr[positionNumber]->diameter.push_back(ls_devicePtr->lsMeasureResult[0]);
		myScrewRoundoutDataPtr[positionNumber]->out2.push_back(ls_devicePtr->lsMeasureResult[1]);
		myScrewRoundoutDataPtr[positionNumber]->out3.push_back(ls_devicePtr->lsMeasureResult[2]);
		myScrewRoundoutDataPtr[positionNumber]->out4.push_back((ls_devicePtr->lsMeasureResult[1] - ls_devicePtr->lsMeasureResult[2]) / 2);
	};
	//trap5轴
	moveControlPtr->setCurrentAxis(5);
	moveControlPtr->setMoveMode("Trap");
	moveControlPtr->setTrapPrm(moveControlPtr->axisCore[moveControlPtr->currentAxisIndex], moveControlPtr->currentAxisNumber, moveControlPtr->trapAuto[moveControlPtr->currentAxisIndex], screwPrepare[positionNumber][2], moveControlPtr->trapHighVelAuto[moveControlPtr->currentAxisIndex]);
	moveControlPtr->startTrap();
	do
	{
		moveControlPtr->updateAxisStatus(moveControlPtr->currentAxisNumber);
		msleep(200);
	} while (!moveControlPtr->bFlagArrive[moveControlPtr->currentAxisIndex] && moveControlPtr->bFlagMotion[moveControlPtr->currentAxisIndex]);
	camPtrList[0]->startCapture();
	//trap7轴
	axis7_steplength = 0;
	moveControlPtr->setCurrentAxis(7);
	moveControlPtr->zeroPosition();
	moveControlPtr->setMoveMode("Trap");
	for (int i = 0; i < screwPrepare[positionNumber][3]; i++)
	{
		cout << 999999999999999 << endl;
		axis7_steplength = (i + 1) * axis7_stepIncrease;
		moveControlPtr->setTrapPrm(moveControlPtr->axisCore[moveControlPtr->currentAxisIndex], moveControlPtr->currentAxisNumber, moveControlPtr->trapAuto[moveControlPtr->currentAxisIndex], axis7_steplength, moveControlPtr->trapHighVelAuto[moveControlPtr->currentAxisIndex]);
		moveControlPtr->startTrap();
		cout << "七轴步长" << i + 1 << axis7_steplength << endl;
		do
		{
			moveControlPtr->updateAxisStatus(moveControlPtr->currentAxisNumber);
			msleep(200);
		} while (moveControlPtr->bFlagMotion[moveControlPtr->currentAxisIndex]);
		axis7_steplength = (i + 1) * axis7_stepIncrease;
		string imgPath = dataSavePath + "\\cam1_screwReferance" + to_string(positionNumber + 1) + "_" + to_string(i + 1) + ".bmp";
		cout << "螺纹地址：" << imgPath << endl;
		camPtrList[0]->saveImg(imgPath, 0, 0, 0);
		emit imgInf(originalImgPtr, "org", 0);
		replace(imgPath.begin(), imgPath.end(), '\\', '/');
		//cout << imgPath << endl;
		myScrewRoundoutDataPtr[positionNumber]->referanceImgPath.push_back(imgPath);
	};
	camPtrList[0]->stopCapture();
};

void program_1::clearScrewRoundOutData()
{
	for (int i=0; i < 1; i++)
	{
		myScrewRoundoutDataPtr[i]->diameter.clear();
		myScrewRoundoutDataPtr[i]->diameter.shrink_to_fit();
		myScrewRoundoutDataPtr[i]->out2.clear();
		myScrewRoundoutDataPtr[i]->out2.shrink_to_fit();
		myScrewRoundoutDataPtr[i]->out3.clear();
		myScrewRoundoutDataPtr[i]->out3.shrink_to_fit();
		myScrewRoundoutDataPtr[i]->out4.clear();
		myScrewRoundoutDataPtr[i]->out4.shrink_to_fit();
		myScrewRoundoutDataPtr[i]->screwImgPath.clear();
		myScrewRoundoutDataPtr[i]->screwImgPath.shrink_to_fit();
		myScrewRoundoutDataPtr[i]->referanceImgPath.clear();
		myScrewRoundoutDataPtr[i]->referanceImgPath.shrink_to_fit();
	};
}
/*
void program_1::cam1_Measure_prepare(int positionNumber)
{
	cam1PathList.clear();
	cam1PathList.shrink_to_fit();
	//trap5轴
	moveControlPtr->setCurrentAxis(5);
	moveControlPtr->setMoveMode("Trap");
	moveControlPtr->setTrapPrm(moveControlPtr->axisCore[moveControlPtr->currentAxisIndex], moveControlPtr->currentAxisNumber, moveControlPtr->trapAuto[moveControlPtr->currentAxisIndex], cam1_prepare[positionNumber][0], moveControlPtr->trapHighVelAuto[moveControlPtr->currentAxisIndex]);
	moveControlPtr->startTrap();
	do
	{
		moveControlPtr->updateAxisStatus(moveControlPtr->currentAxisNumber);
		msleep(200);
	} while (!moveControlPtr->bFlagArrive[moveControlPtr->currentAxisIndex] && moveControlPtr->bFlagMotion[moveControlPtr->currentAxisIndex]);
	//trap2轴
	moveControlPtr->setCurrentAxis(2);
	moveControlPtr->setMoveMode("Trap");
	moveControlPtr->setTrapPrm(moveControlPtr->axisCore[moveControlPtr->currentAxisIndex], moveControlPtr->currentAxisNumber, moveControlPtr->trapAuto[moveControlPtr->currentAxisIndex], cam1_prepare[positionNumber][1], moveControlPtr->trapHighVelAuto[moveControlPtr->currentAxisIndex]);
	moveControlPtr->startTrap();
	do
	{
		moveControlPtr->updateAxisStatus(moveControlPtr->currentAxisNumber);
		msleep(200);
	} while (!moveControlPtr->bFlagArrive[moveControlPtr->currentAxisIndex] && moveControlPtr->bFlagMotion[moveControlPtr->currentAxisIndex]);
	camPtrList[1]->startCapture();
	msleep(500);
	emit programTips("相机已经到达采集位置，请确保机心夹安装正确。并且调整转台位置使零件对正！",4);
	//孔径相机暂停测试部分
	programConfirm = "doNotContinue";
	do
	{
		msleep(150);
		emit imgInf(originalImgPtr, "org", cam1_prepare[positionNumber][4]);
	} while (programConfirm == "doNotContinue");
	long axis7_stepIncrease = 360 / cam1_prepare[positionNumber][3] * 500;
	long axis7_steplength = 0;
	moveControlPtr->setCurrentAxis(7);
	moveControlPtr->zeroPosition();
	moveControlPtr->setMoveMode("Trap");
	for (int i = 0; i < cam1_prepare[positionNumber][3]; i++)
	{
		moveControlPtr->setTrapPrm(moveControlPtr->axisCore[moveControlPtr->currentAxisIndex], moveControlPtr->currentAxisNumber, moveControlPtr->trapAuto[moveControlPtr->currentAxisIndex], axis7_steplength, moveControlPtr->trapHighVelAuto[moveControlPtr->currentAxisIndex]);
		moveControlPtr->startTrap();
		do
		{
			moveControlPtr->updateAxisStatus(moveControlPtr->currentAxisNumber);
			msleep(200);
		} while (moveControlPtr->bFlagMotion[moveControlPtr->currentAxisIndex]);
		axis7_steplength = (i + 1) * axis7_stepIncrease;
		string imgPath = dataSavePath + "\\cam1_" + to_string(positionNumber + 1) + "_" + to_string(i + 1) + ".bmp";

		camPtrList[1]->saveImg(imgPath,0,0,0);
		emit imgInf(originalImgPtr, "org", cam1_prepare[positionNumber][4]);
		replace(imgPath.begin(), imgPath.end(), '\\', '/');
		//cout << imgPath << endl;
		cam1PathList.push_back(imgPath);
	};
	camPtrList[1]->stopCapture();
	//2轴退回
	moveControlPtr->setCurrentAxis(2);
	moveControlPtr->setTrapPrm(moveControlPtr->axisCore[moveControlPtr->currentAxisIndex], moveControlPtr->currentAxisNumber, moveControlPtr->trapAuto[moveControlPtr->currentAxisIndex], cam1_prepare[positionNumber][2], moveControlPtr->trapHighVelAuto[moveControlPtr->currentAxisIndex]);
	moveControlPtr->startTrap();
	do
	{
		moveControlPtr->updateAxisStatus(moveControlPtr->currentAxisNumber);
		msleep(200);
	} while (!moveControlPtr->bFlagArrive[moveControlPtr->currentAxisIndex] && moveControlPtr->bFlagMotion[moveControlPtr->currentAxisIndex]);
	cout << " cam1_Measure_prepare准备 " << positionNumber + 1 << " 执行完毕 " << endl;
};
*/
void program_1::cam2_Measure_prepare(int positionNumber)
{
	string roughnessFolder;//新建粗糙度文件夹命名为序号
	roughnessFolder = dataSavePath + "\\" + to_string(positionNumber + 1);
	string command;
	command = "mkdir -p " + roughnessFolder;
	system(command.c_str());
	moveControlPtr->setCurrentAxis(5);//移动5轴
	moveControlPtr->setMoveMode("Trap");
	moveControlPtr->setTrapPrm(moveControlPtr->axisCore[moveControlPtr->currentAxisIndex], moveControlPtr->currentAxisNumber, moveControlPtr->trapAuto[moveControlPtr->currentAxisIndex], cam2_parpare[positionNumber][0], moveControlPtr->trapHighVelAuto[moveControlPtr->currentAxisIndex]);
	moveControlPtr->startTrap();
	do
	{
		moveControlPtr->updateAxisStatus(moveControlPtr->currentAxisNumber);
		msleep(200);
	} while (!moveControlPtr->bFlagArrive[moveControlPtr->currentAxisIndex] && moveControlPtr->bFlagMotion[moveControlPtr->currentAxisIndex]);
	moveControlPtr->setCurrentAxis(1);//1轴第一次trap移动(预先移动)
	moveControlPtr->setMoveMode("Trap");
	long axis1_steplength = cam2_parpare[positionNumber][1] + 5000 * (cam2_radius[positionNumber][1] - cam2_radius[positionNumber][0])-500 ;//5个脉冲为1微米
	if (abs(axis1_steplength - cam2_parpare[positionNumber][1]) >= 10000)
	{
		//计算一轴移动步长可能出现了异常,则1轴退回不执行测量
		emit updateDeviceInf("粗糙度移动步长计算错误，请检查工件是否安装正确！");
		axis1_steplength = 120000;
		moveControlPtr->setTrapPrm(moveControlPtr->axisCore[moveControlPtr->currentAxisIndex], moveControlPtr->currentAxisNumber, moveControlPtr->trapAuto[moveControlPtr->currentAxisIndex], axis1_steplength, moveControlPtr->trapHighVelAuto[moveControlPtr->currentAxisIndex]);
		moveControlPtr->startTrap();
		do
		{
			moveControlPtr->updateAxisStatus(moveControlPtr->currentAxisNumber);
			msleep(200);
		} while (!moveControlPtr->bFlagArrive[moveControlPtr->currentAxisIndex] && moveControlPtr->bFlagMotion[moveControlPtr->currentAxisIndex]);
	}
	else {
		cout << "S_F:" << cam2_parpare[positionNumber][1] << "S_C:" << axis1_steplength << "D_F:" << cam2_radius[positionNumber][0] << "D_A:" << cam2_radius[positionNumber][1] << endl;
		moveControlPtr->setTrapPrm(moveControlPtr->axisCore[moveControlPtr->currentAxisIndex], moveControlPtr->currentAxisNumber, moveControlPtr->trapAuto[moveControlPtr->currentAxisIndex], axis1_steplength, moveControlPtr->trapHighVelAuto[moveControlPtr->currentAxisIndex]);
		moveControlPtr->startTrap();
		do
		{
			moveControlPtr->updateAxisStatus(moveControlPtr->currentAxisNumber);
			msleep(200);
		} while (!moveControlPtr->bFlagArrive[moveControlPtr->currentAxisIndex] && moveControlPtr->bFlagMotion[moveControlPtr->currentAxisIndex]);
		//cout << "予以动" << endl;
		camPtrList[2]->startCapture();//开始次移动和拍照
		for (int i = 0; i < 8; i++)
		{
			axis1_steplength = cam2_parpare[positionNumber][1] + 5000 * (cam2_radius[positionNumber][1] - cam2_radius[positionNumber][0]) - 45 * (i - 1)-150 ;
			moveControlPtr->setTrapPrm(moveControlPtr->axisCore[moveControlPtr->currentAxisIndex], moveControlPtr->currentAxisNumber, moveControlPtr->trapAuto[moveControlPtr->currentAxisIndex], axis1_steplength, moveControlPtr->trapLowVelAuto[moveControlPtr->currentAxisIndex]);
			moveControlPtr->startTrap();
			roughnessPicPath[i] = roughnessFolder + "\\" + to_string(i + 1) + ".bmp";
			do
			{
				moveControlPtr->updateAxisStatus(moveControlPtr->currentAxisNumber);
				emit imgInf(originalImgPtr, "org", 0);
				msleep(200);
			} while (!moveControlPtr->bFlagArrive[moveControlPtr->currentAxisIndex] && moveControlPtr->bFlagMotion[moveControlPtr->currentAxisIndex]);
			camPtrList[2]->saveImg(roughnessPicPath[i], 5120, 5120, 1);
		};//以下是1轴退回以及对图像地址进行处理
		camPtrList[2]->stopCapture();
		moveControlPtr->setTrapPrm(moveControlPtr->axisCore[moveControlPtr->currentAxisIndex], moveControlPtr->currentAxisNumber, moveControlPtr->trapAuto[moveControlPtr->currentAxisIndex], cam2_parpare[positionNumber][2], moveControlPtr->trapHighVelAuto[moveControlPtr->currentAxisIndex]);
		moveControlPtr->startTrap();
		replace(roughnessFolder.begin(), roughnessFolder.end(), '\\', '/');
		QString roughnessImgPath;
		try {
			pair<float, string> result = m_roughnessObjPtr->processFolder(roughnessFolder);
			float max_value = result.first;
			string max_filename = result.second;
			replace(max_filename.begin(), max_filename.end(), '\\', '/');
			roughnessImgPath = QString::fromStdString(max_filename);
			if (!max_filename.empty()) {
				cout << "最大值: " << max_value << endl;
				cout << "对应图像的图像名称: " << roughnessImgPath.toStdString() << endl;
			}
			else {
				cout << "数组为空" << endl;
			}
		}
		catch (...) {
			emit updateDeviceInf("粗糙度测量选择最清晰图像执行错误");
		};
		try {
			roughnessResult[positionNumber][2] = m_roughnessObjPtr->rougthnessCaculate(roughnessImgPath);
			//cout << "粗糙度值" << roughnessResult[positionNumber][2] << endl;
			resultVectorList.push_back(roughnessResult[positionNumber][2]);
			pushback_vectors(resultVectorList, 9, roughnessResult[positionNumber][0], 0, 0, roughnessResult[positionNumber][1]);
		}
		catch (...) {
			emit updateDeviceInf("粗糙度计算执行错误");
		};
		do
		{
			moveControlPtr->updateAxisStatus(moveControlPtr->currentAxisNumber);
			msleep(200);
		} while (!moveControlPtr->bFlagArrive[moveControlPtr->currentAxisIndex] && moveControlPtr->bFlagMotion[moveControlPtr->currentAxisIndex]);
	}

	//cout << " cam2_Measure_prepare点位 " << positionNumber + 1 << " 执行完毕 " << endl;
};

void program_1::saveAsExcel()//将主界面上的表格保存为excel文件
{
	string excelPath = dataSavePath + "\\measureResult.xlsx";

	//string excelPath = "C:\\Users\\Administrator\\Desktop\\measureResult.xlsx";
	cout << "excel   " << excelPath << endl;
	CoInitializeEx(NULL, COINITBASE_MULTITHREADED);
	QAxObject* excel = new QAxObject;
	QAxObject* workbooks;
	QAxObject* range;
	QAxObject* colm;
	QAxObject* row;
	QAxObject* font;
	QAxObject* cell;
	if (excel->setControl("Excel.Application"))
	{
		excel->dynamicCall("SetVisible (bool Visible)", false);
		excel->setProperty("DisplayAlerts", false);
		workbooks = excel->querySubObject("WorkBooks");            //获取工作簿集合
		workbooks->dynamicCall("Add");                                        //新建一个工作簿
		workbook = excel->querySubObject("ActiveWorkBook");        //获取当前工作簿
		worksheet = workbook->querySubObject("Worksheets(int)", 1);

		int rowCount = measureTablePtr->rowCount();
		cout << "行数" << rowCount << endl;
		int columnCount = measureTablePtr->columnCount();
		//添加Excel表头数据
		bool mergeCellsFlag;
		mergeCellsFlag = mergeCells("A1", "H1", "检测结果通用上传模板");
		mergeCellsFlag = mergeCells("I1", "M2", "");
		mergeCellsFlag = mergeCells("A3", "A4", "特性名称");
		mergeCellsFlag = mergeCells("B3", "B4", "检测尺寸及技术要求");
		mergeCellsFlag = mergeCells("D2", "E2", partNub);
		mergeCellsFlag = mergeCells("G2", "H2", partName);
		for (int i = 3; i < 14; i++)
		{
			cell = worksheet->querySubObject("Cells(int,int)", 3, i);
			cell->dynamicCall("SetValue(const QString&)", QVariant("零件编号\t"));
		}
		cell = worksheet->querySubObject("Cells(int,int)", 4, 3);
		cell->dynamicCall("SetValue(const QString&)", partsID);
		cell = worksheet->querySubObject("Cells(int,int)", 2, 1);
		cell->dynamicCall("SetValue(const QString&)", QVariant("送检单位\t"));
		cell = worksheet->querySubObject("Cells(int,int)", 2, 3);
		cell->dynamicCall("SetValue(const QString&)", QVariant("图号\t"));
		cell = worksheet->querySubObject("Cells(int,int)", 2, 6);
		cell->dynamicCall("SetValue(const QString&)", QVariant("名称\t"));
		font = worksheet->querySubObject("Range(const QString&)", "A2:M4")->querySubObject("Font");// 表头单元格字体设置
		font->setProperty("Bold", true);// 设置单元格字体加粗
		font->setProperty("Size", 11);// 设置单元格字体大小
		font = worksheet->querySubObject("Range(const QString&)", "A1:H1")->querySubObject("Font");// 表头单元格字体设置
		font->setProperty("Bold", true);// 设置单元格字体加粗
		font->setProperty("Size", 36);// 设置单元格字体大小
		colm = worksheet->querySubObject("Columns(const QString&)", "A");
		colm->setProperty("ColumnWidth", 10);
		colm = worksheet->querySubObject("Columns(const QString&)", "B");
		colm->setProperty("ColumnWidth", 20);
		colm = worksheet->querySubObject("Columns(const QString&)", "C");
		colm->setProperty("ColumnWidth", 20);
		colm = worksheet->querySubObject("Columns(const QString&)", "D:M");
		colm->setProperty("ColumnWidth", 10);
		row = worksheet->querySubObject("Rows(const QString&)", "1");
		row->setProperty("RowHeight", 50);
		row = worksheet->querySubObject("Rows(const QString&)", "2:4");
		row->setProperty("RowHeight", 30);

		/*
		for (int i = 1; i <= columnCount; i++)
		{
			cell = worksheet->querySubObject("Cells(int,int)", 1, i);
			cell->setProperty("RowHeight", 40);
			QString st1 = (measureTablePtr->horizontalHeaderItem(i - 1)->data(0)).toString();

			cell->dynamicCall("SetValue(const QString&)", measureTablePtr->horizontalHeaderItem(i - 1)->data(0).toString());
			cell->querySubObject("Font")->setProperty("Bold", true);
			cell->querySubObject("Interior")->setProperty("Color", QColor(220, 220, 220));

		};
		*/
		//将数据写入excel
		for (int j = 5; j <= rowCount + 4; j++)
		{
			QVariant textVar;
			QString textForVariant;
			QString featureIndex;
			QString measureTimes;
			QString featureName;
			QString minResult;
			QString maxResult;
			QString upperDeviation;
			QString lowerDeviation;
			QString nominalSize;

			featureIndex = measureTablePtr->item(j - 5, 0)->text();
			featureName = measureTablePtr->item(j - 5, 1)->text();
			minResult = measureTablePtr->item(j - 5, 3)->text();
			maxResult = measureTablePtr->item(j - 5, 4)->text();
			measureTimes = measureTablePtr->item(j - 5, 5)->text();
			nominalSize = measureTablePtr->item(j - 5, 6)->text();
			lowerDeviation = measureTablePtr->item(j - 5, 7)->text();
			upperDeviation = measureTablePtr->item(j - 5, 8)->text();
			//写入特征索引
			textVar = featureIndex.toFloat();
			cell = worksheet->querySubObject("Cells(int,int)", j, 1);
			cell->dynamicCall("SetValue(const QString)", featureIndex);

			cell = worksheet->querySubObject("Cells(int,int)", j, 2);

			//写入特征公称尺寸以及公差
			if (featureName == "倒角尺寸")
			{
				textForVariant = QString("%1×R%2(%3;%4)").arg(measureTimes).arg(nominalSize).arg(addSymbol(upperDeviation)).arg(addSymbol(lowerDeviation));
			}
			else if (featureName == "直径")
			{
				textForVariant = QString("Φ%1(%2;%3)").arg(nominalSize).arg(addSymbol(upperDeviation)).arg(addSymbol(lowerDeviation));
			}
			else if (featureName == "中径")
			{
				textForVariant = QString("Φ%1(%2;%3)").arg(nominalSize).arg(addSymbol(upperDeviation)).arg(addSymbol(lowerDeviation));
			}
			else if (featureName == "孔径")
			{
				if (measureTimes == "1")
				{
					textForVariant = QString("Φ%1(%2;%3)").arg(nominalSize).arg(addSymbol(upperDeviation)).arg(addSymbol(lowerDeviation));
				}
				else
				{
					textForVariant = QString("%1×Φ%2(%3;%4)").arg(measureTimes).arg(nominalSize).arg(addSymbol(upperDeviation)).arg(addSymbol(lowerDeviation));
				};
			}
			else if (featureName == "倒角角度")
			{
				if (measureTimes == "1")
				{
					textForVariant = QString("%1°(%2°;%3°)").arg(nominalSize).arg(addSymbol(upperDeviation)).arg(addSymbol(lowerDeviation));
				}
				else
				{
					textForVariant = QString("%1×%2°(%3°;%4°)").arg(measureTimes).arg(nominalSize).arg(addSymbol(upperDeviation)).arg(addSymbol(lowerDeviation));
				};
			}
			else
			{
				textForVariant = QString("%1(%2;%3)").arg(nominalSize).arg(addSymbol(upperDeviation)).arg(addSymbol(lowerDeviation));
			};
			cell->dynamicCall("SetValue(const QString&)", textForVariant);

			//写入测量结果
			cell = worksheet->querySubObject("Cells(int,int)", j, 3);
			if (minResult == maxResult)
			{
				textForVariant = maxResult;
			}
			else
			{
				textForVariant = QString("%1 - %2").arg(minResult).arg(maxResult);
			};
			cell->dynamicCall("SetValue(const QString&)", textForVariant);

		};

		range = worksheet->querySubObject("UsedRange");
		QAxObject* cells = range->querySubObject("Columns");
		//cells->dynamicCall("AutoFit");//这句代码可以使得所有单元格自适应宽度

		range->setProperty("HorizontalAlignment", -4108);//水平居中
		range->setProperty("VerticalAlignment", -4108);//垂直居中
		QAxObject* border = range->querySubObject("Borders");
		border->setProperty("Color", QColor(0, 0, 0));

		QString fileName = QString(QString::fromLocal8Bit(excelPath.c_str()));
		workbook->dynamicCall("SaveAs(const QString&)", QDir::toNativeSeparators(fileName)); //保存至fileName
		workbook->dynamicCall("Close()");                                                   //关闭工作簿
		excel->dynamicCall("Quit()");                                                       //关闭excel
		delete excel;
		excel = NULL;
		workbook = NULL;
		worksheet = NULL;
	};
};
bool program_1::mergeCells(QString start, QString end, QString value)
{
	if (worksheet == NULL)
		return false;
	QAxObject* mergeRange = worksheet->querySubObject("range(const Qvariant&)", QVariant("=(" + start + ": " + end + ")"));
	if (mergeRange == NULL)
		return false;

	mergeRange->setProperty("MergeCells", true); // 合并单元格
	mergeRange->setProperty("Value", value);
	return true;
}

void program_1::creatDatabaseTable()
{
	cout << "creatDatabaseTable" << endl;


	//databasePtr->exec("DROP TABLE Student");表格的首字母必须为英文的大写，否则创建失败(不一定对还需要测试)
	QString comand = QString("CREATE TABLE %1 ("
		"日期 VARCHAR(10) NOT NULL , "
		"工件型号 VARCHAR(20) NOT NULL, "
		"检验员姓名 VARCHAR(20) NOT NULL, "
		"检验员ID VARCHAR(20) NOT NULL, "
		"全特征测量结果 VARCHAR(4) NOT NULL, ").arg(partNub);
	QString comandMidle = "";
	for (int i = 1; i < 19; i++)
	{
		comandMidle +=
			QString(
				"直径D%1合格标志 VARCHAR(4) NOT NULL, "
				"直径D%1测量值 FLOAT NOT NULL, "
				"直径D%1下限值 FLOAT NOT NULL, "
				"直径D%1上限值 FLOAT NOT NULL, ").arg(i);
	};
	/*
	for (int i = 1; i < 5; i++)
	{
		comandMidle +=
			QString(
				"圆角R%1合格标志 VARCHAR(4) NOT NULL, "
				"圆角R%1测量值 FLOAT NOT NULL, "
				"圆角R%1下限值 FLOAT NOT NULL, "
				"圆角R%1上限值 FLOAT NOT NULL, ").arg(i);
	};
	for (int i = 1; i < 5; i++)
	{
		comandMidle +=
			QString(
				"长度L%1合格标志 VARCHAR(4) NOT NULL, "
				"长度L%1测量值 FLOAT NOT NULL, "
				"长度L%1下限值 FLOAT NOT NULL, "
				"长度L%1上限值 FLOAT NOT NULL, ").arg(i);
	};
	for (int i = 1; i < 7; i++)
	{
		comandMidle +=
			QString(
				"粗糙度Ra%1合格标志 VARCHAR(4) NOT NULL, "
				"粗糙度Ra%1测量值 FLOAT NOT NULL, "
				"粗糙度Ra%1下限值 FLOAT NOT NULL, "
				"粗糙度Ra%1上限值 FLOAT NOT NULL, ").arg(i);
	};*/
	comandMidle.chop(2);
	comandMidle += ")";
	comand += comandMidle;
	//cout << comand.toStdString() << endl;
	databasePtr->exec(comand);
	/*
	databasePtr->exec(QString("CREATE TABLE %1 ("
		"日期 VARCHAR(10) NOT NULL , "
		"工件型号 VARCHAR(20) NOT NULL, "
		"检验员姓名 VARCHAR(20) NOT NULL, "
		"检验员ID VARCHAR(20) NOT NULL, "
		"全特征测量结果 VARCHAR(4) NOT NULL, "
		"直径D1合格标志 VARCHAR(4) NOT NULL, "
		"直径D1测量值 FLOAT NOT NULL, "
		"直径D1下限值 FLOAT NOT NULL, "
		"直径D1上限值 FLOAT NOT NULL, "
		"直径D2合格标志 VARCHAR(10) NOT NULL, "
		"直径D2测量值 FLOAT NOT NULL, "
		"直径D2下限值 FLOAT NOT NULL, "
		"直径D2上限值 FLOAT NOT NULL)").arg(partNub)
	);*/

	databasePtr->commit();
};
void program_1::writeToDatabase()
{
	/*
	QString valueText=QString("'%1' ,'%2','%3','%4','%5'").arg(QString::fromLocal8Bit(stime)).arg(partNub).arg(operatorName).arg(operatorID).arg(allFeatureFlag);
	for (int i=0; i < 2; i++)
	{
		valueText += QString(",'%1',%2,%3,%4").arg(lsMeasureFlag[i]).arg(lsDiameterResult_average[i]).arg(lsLowerLimit[i]).arg(lsUpperLimit[i]);
	};
	databasePtr->exec(QString("insert into %1 values(%2)").arg(partNub).arg(valueText));
	//databasePtr->exec(QString("insert into %1 values('EE','ee','ee','ee','ok','ok',1,1,1,'ok',1,1,1)").arg(partNub));
	databasePtr->commit();
	*/
};
void program_1::zeroMeasureNub()
{
	allFeatureFlag = "--";
	ngFeatureNum = 0;//NG特征数量
	measurePartsNum_all = 0;//该型号轴测量总数
	measurePartsNum_ok = 0;//该型号轴合格数量
	measurePartsNum_ng = 0;//该型号轴NG数量
	yield = 0;//该型号轴合格率
	emit measureStatistics(allFeatureFlag, ngFeatureNum, measurePartsNum_all, yield);
};
void program_1::pushback_vectors(vector<double>& fResult, int fType, double fIndex, double fNominalsize, double fLowerTolerance, double fUpperTolerance)
{
	//将对应测量结果写入动态数组储存
	double fLowerSize = fNominalsize + fLowerTolerance;//下公差尺寸
	double fUpperSize = fNominalsize + fUpperTolerance;//上公差尺寸
	double tolerance = fUpperTolerance - fLowerTolerance;
	int resultLength = fResult.size();
	cout << "size:" << resultLength << endl;
	if (resultLength)
	{
		double minResult = *min_element(fResult.begin(), fResult.end());
		double maxResult = *max_element(fResult.begin(), fResult.end());
		double fQualify[3];//0号是min的判断结果，1号是max的判断结果；2号是综合判断结果

		if (minResult >= fLowerSize && minResult <= fUpperSize) {//特征检出尺寸是否位于合格尺寸内。是否合格0合格1不合格
			fQualify[0] = 0;
		}
		else {
			fQualify[0] = 1;
		};
		if (maxResult >= fLowerSize && maxResult <= fUpperSize) {//特征检出尺寸是否位于合格尺寸内。是否合格0合格1不合格
			fQualify[1] = 0;
		}
		else {
			fQualify[1] = 1;
		};
		if (fQualify[0] == 1 || fQualify[1] == 1)
		{
			fQualify[2] = 1;
			allFeatureFlag = "NG";
			ngFeatureNum++;
		}
		else
		{
			fQualify[2] = 0;
		};
		minResult = round(minResult * 10000) / 10000;//元整保留4位小数
		maxResult = round(maxResult * 10000) / 10000;
		/*
		vector<double> temp_fGlobal;
		temp_fGlobal.push_back(fIndex);
		temp_fGlobal.push_back(fType);
		temp_fGlobal.push_back(fQualify);
		temp_fGlobal.push_back();
		temp_fGlobal.push_back(fNominalsize);
		temp_fGlobal.push_back(fLowerTolerance);
		temp_fGlobal.push_back(fUpperTolerance);
		fGlobal.push_back(temp_fGlobal);
		*/

		//将测量结果显示在桌面表格上
		int row = measureTablePtr->rowCount();
		measureTablePtr->insertRow(row);//增加一行
		QTableWidgetItem* item[9];
		for (int i = 0; i < 9; i++)
		{
			item[i] = new QTableWidgetItem("");
			item[i]->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);//居中设置（有可能可以不用）
		};
		item[0]->setText(QString::number(fIndex));//第一列特征编号
		measureTablePtr->setItem(row, 0, item[0]);
		item[1]->setText(featureMap[fType]);//显示特征名称
		measureTablePtr->setItem(row, 1, item[1]);
		item[2]->setText(featureQualityMap[fQualify[2]]);//显示测量判断结果
		if (fQualify[2])
		{
			item[2]->setForeground(QColor("#DC2626"));
			item[2]->setBackground(QColor("#FEF2F2"));
		}
		else
		{
			item[2]->setForeground(QColor("#16A34A"));
		};
		measureTablePtr->setItem(row, 2, item[2]);
		item[3]->setText(QString::number(minResult, 'f', 4));//显示测量最小值
		if (fQualify[0])
		{
			if ((fLowerSize - minResult) / tolerance > 0.1|| ( minResult-fUpperSize) / tolerance > 0.1)
			{
				item[3]->setForeground(QColor("#DC2626"));
				item[3]->setBackground(QColor("#FEF2F2"));
			}
			else
			{
				item[3]->setForeground(QColor("#B45309"));
				item[3]->setBackground(QColor("#FFF7ED"));
			}
		}
		else
		{
			item[3]->setForeground(QColor("#16A34A"));
		};
		measureTablePtr->setItem(row, 3, item[3]);
		item[4]->setText(QString::number(maxResult, 'f', 4));//显示测量最大值
		if (fQualify[1])
		{
			if ((fLowerSize - maxResult) / tolerance > 0.1 || ( maxResult-fUpperSize) / tolerance > 0.1)
			{
				item[4]->setForeground(QColor("#DC2626"));
				item[4]->setBackground(QColor("#FEF2F2"));
			}
			else
			{
				item[4]->setForeground(QColor("#B45309"));
				item[4]->setBackground(QColor("#FFF7ED"));
			};
		}
		else
		{
			item[4]->setForeground(QColor("#16A34A"));
		};
		measureTablePtr->setItem(row, 4, item[4]);
		item[5]->setText(QString::number(resultLength));//显示测量次数
		measureTablePtr->setItem(row, 5, item[5]);
		item[6]->setText(QString::number(fNominalsize));//显示公称值
		measureTablePtr->setItem(row, 6, item[6]);
		item[7]->setText(QString::number(fLowerTolerance));//显示下限值
		measureTablePtr->setItem(row, 7, item[7]);
		item[8]->setText(QString::number(fUpperTolerance));//显示上限值
		measureTablePtr->setItem(row, 8, item[8]);
		fResult.clear();
		fResult.shrink_to_fit();
	}
	else {
		fResult.clear();
		fResult.shrink_to_fit();
	};

};
void program_1::clearVector(vector <float>& list)
{
	list.clear();
	list.shrink_to_fit();
};


//远心相机图像检测算法******************************************************************************************************************************************************************************************************************************
void program_1::Straight_TLineFP_P(double* TLineFP_result, HObject ho_Image, int Line1, int Line2, int Line3, int Line4,
	int Metrology1, int Metrology2, int Metrology3, int Metrology4)
{
	// Local iconic variables
	HObject  ho_LineContours, ho_LineContour;
	// Local control variables
	HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices;
	HTuple  hv_RowLine, hv_ColumnLine, hv_RowBegin, hv_ColBegin;
	HTuple  hv_RowEnd, hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;
	HTuple  hv_Width, hv_Height;
	GetImageSize(ho_Image, &hv_Width, &hv_Height);


	CreateMetrologyModel(&hv_MetrologyHandle);

	hv_Line.Clear();
	hv_Line[0] = Line1;
	hv_Line[1] = Line2;
	hv_Line[2] = Line3;
	hv_Line[3] = Line4;
	AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, Metrology1, Metrology2, Metrology3, Metrology4,
		HTuple(), HTuple(), &hv_LineIndices);
	SetMetrologyObjectParam(hv_MetrologyHandle, "all", "measure_distance", 0.3);
	ApplyMetrologyModel(ho_Image, hv_MetrologyHandle);

	GetMetrologyObjectMeasures(&ho_LineContours, hv_MetrologyHandle, "all", "all",
		&hv_RowLine, &hv_ColumnLine);
	if (HDevWindowStack::IsOpen())
		SetColor(HDevWindowStack::GetActive(), "yellow");
	if (HDevWindowStack::IsOpen())
		SetLineWidth(HDevWindowStack::GetActive(), 2);
	if (HDevWindowStack::IsOpen())
		DispObj(ho_LineContours, HDevWindowStack::GetActive());
	GetMetrologyObjectResultContour(&ho_LineContour, hv_MetrologyHandle, 0, "all",
		1.5);
	//get_metrology_object_model_contour (ModelContour, MetrologyHandle, 'all', 1.5)
	if (HDevWindowStack::IsOpen())
		SetColor(HDevWindowStack::GetActive(), "blue");
	if (HDevWindowStack::IsOpen())
		SetLineWidth(HDevWindowStack::GetActive(), 2);
	if (HDevWindowStack::IsOpen())
		DispObj(ho_LineContour, HDevWindowStack::GetActive());
	FitLineContourXld(ho_LineContour, "tukey", -1, 0, 5, 2, &hv_RowBegin, &hv_ColBegin,
		&hv_RowEnd, &hv_ColEnd, &hv_Nr, &hv_Nc, &hv_Dist);
	cout << " hv_result():" << hv_RowBegin[0].D() << "  " << hv_ColBegin[0].D() << "  " << hv_RowEnd[0].D() << "  " << hv_ColEnd[0].D() << endl;
	TLineFP_result[0] = hv_RowBegin[0].D();
	TLineFP_result[1] = hv_ColBegin[0].D();
	TLineFP_result[2] = hv_RowEnd[0].D();
	TLineFP_result[3] = hv_ColEnd[0].D();
}

//模板匹配找特征点
void program_1::TemplateMatching_TLineFP_P(double* TLineFP_result, HObject ho_Image, HTuple hv_ModelFile, int MatchingLine1, int MatchingLine2, int MatchingLine3, int MatchingLine4,
	int Metrology1, int Metrology2, int Metrology3, int Metrology4,
	double ShapeModel1, double ShapeModel2, double ShapeModel3, double ShapeModel4, double ShapeModel5) {
	// Local iconic variables
	HObject  ho_ModelContours, ho_LineContours;
	HObject  ho_LineContour;

	// Local control variables
	HTuple  hv_Width, hv_Height, hv_ReusedModelID;
	HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
	HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
	HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
	HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
	HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
	HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
	HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

	//读取模板和图像进行模板匹配*
	GetImageSize(ho_Image, &hv_Width, &hv_Height);

	//hv_ModelFile = "./programParmeter/program0_1080new/cam0_10-460-209.sbm";
	ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
	//创建测量模型
	CreateMetrologyModel(&hv_MetrologyHandle);
	//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
	hv_Line.Clear();
	hv_Line[0] = MatchingLine1;
	hv_Line[1] = MatchingLine2;
	hv_Line[2] = MatchingLine3;
	hv_Line[3] = MatchingLine4;

	//AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 70,
	//	HTuple(), HTuple(), &hv_LineIndices);
	AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, Metrology1, Metrology2, Metrology3, Metrology4,
		HTuple(), HTuple(), &hv_LineIndices);
	SetMetrologyObjectParam(hv_MetrologyHandle, "all", "measure_distance", 0.3);
	//获取读取的模板的轮廓，区域坐标等信息
	GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
	GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
	GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
		&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
	//进行模板匹配
	//FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.2,
	//	1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
	FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, ShapeModel1,
		ShapeModel2, ShapeModel3, "least_squares", ShapeModel4, ShapeModel5, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
	//显示形状特征模板匹配结果


	VectorAngleToRigid(hv_ReusedRefPointRow, hv_ReusedRefPointCol, 0, hv_Row3, hv_Column3,
		hv_Angle, &hv_HomMat2D);
	//affine_trans_contour_xld (ModelContours, ContoursAffinTrans, HomMat2D)
	//dev_set_color ('green')
	//dev_display (ContoursAffinTrans)
	//对测量模型执行变换
	AlignMetrologyModel(hv_MetrologyHandle, hv_Row3, hv_Column3, hv_Angle);
	//获取变换后的测量模型的轮廓
	GetMetrologyObjectMeasures(&ho_LineContours, hv_MetrologyHandle, "all", "all",
		&hv_RowLine, &hv_ColumnLine);
	//卡尺找直线
	ApplyMetrologyModel(ho_Image, hv_MetrologyHandle);

	if (HDevWindowStack::IsOpen())
		SetColor(HDevWindowStack::GetActive(), "yellow");
	if (HDevWindowStack::IsOpen())
		SetLineWidth(HDevWindowStack::GetActive(), 2);
	if (HDevWindowStack::IsOpen())
		DispObj(ho_LineContours, HDevWindowStack::GetActive());
	GetMetrologyObjectResultContour(&ho_LineContour, hv_MetrologyHandle, 0, "all",
		1.5);
	//get_metrology_object_model_contour (ModelContour, MetrologyHandle, 'all', 1.5)
	if (HDevWindowStack::IsOpen())
		SetColor(HDevWindowStack::GetActive(), "blue");
	if (HDevWindowStack::IsOpen())
		SetLineWidth(HDevWindowStack::GetActive(), 2);
	if (HDevWindowStack::IsOpen())
		DispObj(ho_LineContour, HDevWindowStack::GetActive());
	FitLineContourXld(ho_LineContour, "tukey", -1, 0, 5, 2, &hv_RowBegin, &hv_ColBegin,
		&hv_RowEnd, &hv_ColEnd, &hv_Nr, &hv_Nc, &hv_Dist);
	//输出模板匹配后检测直线两端点的位置，即特征点位置。
	//double* TLineFP_result = new double[3];
	cout << " hv_result():" << hv_RowBegin[0].D() << "  " << hv_ColBegin[0].D() << "  " << hv_RowEnd[0].D() << "  " << hv_ColEnd[0].D() << endl;
	TLineFP_result[0] = hv_RowBegin[0].D();
	TLineFP_result[1] = hv_ColBegin[0].D();
	TLineFP_result[2] = hv_RowEnd[0].D();
	TLineFP_result[3] = hv_ColEnd[0].D();
	cout << hv_ModelFile << endl;
};

//模板匹配求倒角
double program_1::TemplateMatching_Chamfer_P(HObject ho_Image, HTuple hv_ModelFile, double ShapeModel1, double ShapeModel2, double ShapeModel3, double ShapeModel4, double ShapeModel5,
	int Rectangle1, int Rectangle2, int Rectangle3, int Rectangle4,
	int EdgesSubP1, int EdgesSubP2, int EdgesSubP3,
	bool Segment, int SegmentContour1, int SegmentContour2, int SegmentContour3,
	double SelectContour1, double SelectContour2, double SelectContour3,
	int UnionContour1, int UnionContour2,
	int FitCircleContour1, int FitCircleContour2, int FitCircleContour3, int FitCircleContour4, int FitCircleContour5)
{

	// Local iconic variables
	HObject  ho_ModelContours, ho_ContoursAffinTrans;
	HObject  ho_Rectangle, ho_RectangleTrans, ho_ImageReduced;
	HObject  ho_Edges, ho_ContoursSplit, ho_SelectedContours;
	HObject  ho_UnionContours;

	// Local control variables
	HTuple  hv_Width, hv_Height, hv_ReusedModelID;
	HTuple  hv_ReusedRefPointRow, hv_ReusedRefPointCol, hv_NumLevels;
	HTuple  hv_AngleStart, hv_AngleExtent, hv_AngleStep, hv_ScaleMin;
	HTuple  hv_ScaleMax, hv_ScaleStep, hv_Metric, hv_MinContrast;
	HTuple  hv_Row3, hv_Column3, hv_Angle, hv_Score, hv_HomMat2D;
	HTuple  hv_HomMat2DIdentity, hv_HomMat2DTranslate, hv_Row;
	HTuple  hv_Column, hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;

	//读取模板和图像进行模板匹配*
	GetImageSize(ho_Image, &hv_Width, &hv_Height);
	//hv_ModelFile = "./programParmeter/program0_1080new/cam0_4-220-72.sbm";
	ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
	//获取读取的模板的轮廓，区域坐标等信息
	GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
	GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
	GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
		&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
	//进行模板匹配
	//FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.4,
	//	1, 0.3, "least_squares", 0, 0.6, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
	FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, ShapeModel1,
		ShapeModel2, ShapeModel3, "least_squares", ShapeModel4, ShapeModel5, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
	//显示形状特征模板匹配结果
	//  
		//  
	VectorAngleToRigid(hv_ReusedRefPointRow, hv_ReusedRefPointCol, 0, hv_Row3, hv_Column3,
		hv_Angle, &hv_HomMat2D);
	AffineTransContourXld(ho_ModelContours, &ho_ContoursAffinTrans, hv_HomMat2D);
	if (HDevWindowStack::IsOpen())
		SetColor(HDevWindowStack::GetActive(), "green");
	if (HDevWindowStack::IsOpen())
		DispObj(ho_ContoursAffinTrans, HDevWindowStack::GetActive());
	//
	//Segment a region containing the edges
	//基于全局阈值的图像快速阈值化
	//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin与colend可固定不变


	//GenRectangle1(&ho_Rectangle, 0, 0, 300, 300);
	GenRectangle1(&ho_Rectangle, Rectangle1, Rectangle2, Rectangle3, Rectangle4);
	//设置条状的宽度为70

	HomMat2dIdentity(&hv_HomMat2DIdentity);
	HomMat2dTranslate(hv_HomMat2DIdentity, hv_Column3 - 1000, hv_Row3 + 1000, &hv_HomMat2DTranslate);

	AffineTransRegion(ho_Rectangle, &ho_RectangleTrans, hv_HomMat2DTranslate, "nearest_neighbor");

	ReduceDomain(ho_Image, ho_RectangleTrans, &ho_ImageReduced);
	//In the subdomain of the image containing the edges,
	//extract subpixel precise edges.
	//提取亚像素精密边缘轮廓
	//EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);
	EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", EdgesSubP1, EdgesSubP1, EdgesSubP1);
	if (Segment) {
		//SegmentContoursXld(ho_Edges, &ho_ContoursSplit, "lines_circles", 5, 4, 2);
		SegmentContoursXld(ho_Edges, &ho_ContoursSplit, "lines_circles", SegmentContour1, SegmentContour2, SegmentContour3);
		//提取出轮廓中较长的部分线段
		//SelectContoursXld(ho_ContoursSplit, &ho_SelectedContours, "contour_length", 20,
		//	hv_Width / 2, -0.5, 0.5);
		SelectContoursXld(ho_ContoursSplit, &ho_SelectedContours, "contour_length", SelectContour1,
			hv_Width / 2, SelectContour2, SelectContour3);
		//对相邻的轮廓段进行连接
		//UnionAdjacentContoursXld(ho_SelectedContours, &ho_UnionContours, 90, 1, "attr_keep");
		UnionAdjacentContoursXld(ho_SelectedContours, &ho_UnionContours, UnionContour1, UnionContour2, "attr_keep");
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		// 
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		//FitCircleContourXld(ho_UnionContours, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
		//	&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		FitCircleContourXld(ho_UnionContours, "algebraic", FitCircleContour1, FitCircleContour2, FitCircleContour3, FitCircleContour4, FitCircleContour5, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
	}
	else {
		//FitCircleContourXld(ho_Edges, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
		//	&hv_Radius, & hv_StartPhi, & hv_EndPhi, & hv_PointOrder);
		FitCircleContourXld(ho_Edges, "algebraic", FitCircleContour1, FitCircleContour2, FitCircleContour3, FitCircleContour4, FitCircleContour5, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
	}

	double Chamfer_Radius = hv_Radius.D();
	cout << "Chamfer_Radius():" << Chamfer_Radius << " " << Chamfer_Radius * 0.01216 << endl;
	cout << hv_ModelFile << endl;
	//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
	//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
	if (HDevWindowStack::IsOpen())
		SetColor(HDevWindowStack::GetActive(), "green");
	if (HDevWindowStack::IsOpen())
		DispObj(ho_Edges, HDevWindowStack::GetActive());
	return Chamfer_Radius;
};

//根据起始点与终止点求倒角
double program_1::TLineFP_Chamfer_P(HObject ho_Image, double GenRect1, double GenRect2, double GenRect3, double GenRect4,
	int EdgesSubP1, int EdgesSubP2, int EdgesSubP3,
	bool Segment, int SegmentContour1, int SegmentContour2, int SegmentContour3,
	double SelectContour1, double SelectContour2, double SelectContour3,
	int UnionContour1, int UnionContour2,
	int FitCircleContour1, int FitCircleContour2, int FitCircleContour3, int FitCircleContour4, int FitCircleContour5)
{

	// Local iconic variables
	HObject  ho_Rectangle, ho_ImageReduced;
	HObject  ho_Edges, ho_ContoursSplit, ho_SelectedContours;
	HObject  ho_UnionContours;

	// Local control variables
	HTuple  hv_Width, hv_Height, hv_Row, hv_Column;
	HTuple  hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;
	GetImageSize(ho_Image, &hv_Width, &hv_Height);

	//dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
	//
	//Segment a region containing the edges
	//基于全局阈值的图像快速阈值化
	//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin与colend可固定不变
	//GenRectangle1(&ho_Rectangle, cam0_4_220_72shangbian_TLineFP[2], 500, cam0_4_220_72youbiann_TLineFP[0] - 20, 3500);
	GenRectangle1(&ho_Rectangle, GenRect1, GenRect2, GenRect3, GenRect4);
	//提取roi区域，roi以外的变黑，不改变整个图像的尺寸大小。corp_domain是只提取roi区域
	ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);

	//In the subdomain of the image containing the edges,
	//extract subpixel precise edges.
	//提取亚像素精密边缘轮廓
	//EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", EdgesSubP1, EdgesSubP1, EdgesSubP1);
	EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);

	if (Segment) {
		SegmentContoursXld(ho_Edges, &ho_ContoursSplit, "lines_circles", 5, 4, 2);
		//SegmentContoursXld(ho_Edges, &ho_ContoursSplit, "lines_circles", SegmentContour1, SegmentContour2, SegmentContour3);
		//提取出轮廓中较长的部分线段
		SelectContoursXld(ho_ContoursSplit, &ho_SelectedContours, "contour_length", 20,
			hv_Width / 2, -0.5, 0.5);
		//SelectContoursXld(ho_ContoursSplit, &ho_SelectedContours, "contour_length", SelectContour1,
		//	hv_Width / 2, SelectContour2, SelectContour3);
		//对相邻的轮廓段进行连接
		UnionAdjacentContoursXld(ho_SelectedContours, &ho_UnionContours, 90, 1, "attr_keep");
		//UnionAdjacentContoursXld(ho_SelectedContours, &ho_UnionContours, UnionContour1, UnionContour2, "attr_keep");
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		// 
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		FitCircleContourXld(ho_UnionContours, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		//FitCircleContourXld(ho_UnionContours, "algebraic", FitCircleContour1, FitCircleContour2, FitCircleContour3, FitCircleContour4, FitCircleContour5, &hv_Row, &hv_Column,
		//	&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		cout << "Segment is true!" << endl;
	}
	else {
		//FitCircleContourXld(ho_Edges, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
		//	&hv_Radius, & hv_StartPhi, & hv_EndPhi, & hv_PointOrder);
		FitCircleContourXld(ho_Edges, "algebraic", FitCircleContour1, FitCircleContour2, FitCircleContour3, FitCircleContour4, FitCircleContour5, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
	}

	double Chamfer_Radius = hv_Radius.D();
	cout << "Chamfer_Radius():" << Chamfer_Radius << " " << Chamfer_Radius * 0.01216 << endl;
	//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
	//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
	if (HDevWindowStack::IsOpen())
		SetColor(HDevWindowStack::GetActive(), "green");
	if (HDevWindowStack::IsOpen())
		DispObj(ho_Edges, HDevWindowStack::GetActive());
	return Chamfer_Radius;
};

/**/
double program_1::cam0Picture1_L_K_b(HObject ho_Image, double* hv_Box)
{

	// Local iconic variables
	HObject  ho_ModelContours, ho_LineContours;
	HObject  ho_LineContour;

	// Local control variables
	HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
	HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
	HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
	HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
	HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
	HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
	HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
	HTuple  hv_ColEnd;
	HTuple  hv_Nr_new, hv_Nc_new, hv_Dist_new;
	

	//读取模板和图像进行模板匹配*
	GetImageSize(ho_Image, &hv_Width, &hv_Height);
	cout << "GetImageSize:OK" << endl;
	hv_ModelFile = "./programParmeter/WZ20-15-001-80/cam0_1-80-7right.sbm";
	ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
	//创建测量模型	
	CreateMetrologyModel(&hv_MetrologyHandle);
	//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
	hv_Line.Clear();
	hv_Line[0] = 20;
	hv_Line[1] = 30;
	hv_Line[2] = 1950;
	hv_Line[3] = 30;
	AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 40, 12, 1, 70, HTuple(),
		HTuple(), &hv_LineIndices);
	SetMetrologyObjectParam(hv_MetrologyHandle, "all", "measure_distance", 0.3);

	//获取读取的模板的轮廓，区域坐标等信息
	GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
	GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
	GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
		&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
	//进行模板匹配
	cout << "FindShapeModel:READY" << endl;
	FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.3,
		1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
	cout << "FindShapeModel:OK" << endl;
	VectorAngleToRigid(hv_ReusedRefPointRow, hv_ReusedRefPointCol, 0, hv_Row3, hv_Column3,
		hv_Angle, &hv_HomMat2D);
	//affine_trans_contour_xld (ModelContours, ContoursAffinTrans, HomMat2D)
	//dev_set_color ('green')
	//dev_display (ContoursAffinTrans)
	//对测量模型执行变换
	AlignMetrologyModel(hv_MetrologyHandle, hv_Row3, hv_Column3, hv_Angle);
	//获取变换后的测量模型的轮廓
	GetMetrologyObjectMeasures(&ho_LineContours, hv_MetrologyHandle, "all", "all",
		&hv_RowLine, &hv_ColumnLine);
	//卡尺找直线
	ApplyMetrologyModel(ho_Image, hv_MetrologyHandle);

	if (HDevWindowStack::IsOpen())
		SetColor(HDevWindowStack::GetActive(), "yellow");
	if (HDevWindowStack::IsOpen())
		SetLineWidth(HDevWindowStack::GetActive(), 2);
	if (HDevWindowStack::IsOpen())
		DispObj(ho_LineContours, HDevWindowStack::GetActive());
	GetMetrologyObjectResultContour(&ho_LineContour, hv_MetrologyHandle, 0, "all",
		1.5);
	//get_metrology_object_model_contour (ModelContour, MetrologyHandle, 'all', 1.5)
	if (HDevWindowStack::IsOpen())
		SetColor(HDevWindowStack::GetActive(), "blue");
	if (HDevWindowStack::IsOpen())
		SetLineWidth(HDevWindowStack::GetActive(), 2);
	if (HDevWindowStack::IsOpen())
		DispObj(ho_LineContour, HDevWindowStack::GetActive());
	FitLineContourXld(ho_LineContour, "tukey", -1, 0, 5, 2, &hv_RowBegin, &hv_ColBegin,
		&hv_RowEnd, &hv_ColEnd, &hv_Nr_new, &hv_Nc_new, &hv_Dist_new);
	cout << "FitLineContourXld:OK" << endl;
	cout << " hv_Nr_new.D(): " << hv_Nr_new.D() << endl;
	if ( -0.00000001 < hv_Nr_new.D() && hv_Nr_new.D() <=0 ) {
		hv_Box[0] = -0.00000001;
	}
	else if(0 < hv_Nr_new.D() && hv_Nr_new.D() < 0.00000001){
		hv_Box[0] = 0.00000001;
	}
	else {
		hv_Box[0] = hv_Nr_new.D();
	}
	cout << " hv_Box[0]: " << hv_Box[0] << endl;
	hv_Box[1] =hv_Nc_new.D();
	cout << " hv_Box[1]: " << hv_Box[1] << endl;
	double new_Dist = (hv_ColEnd.D() * hv_Nc_new.D() + hv_RowEnd.D() * hv_Nr_new.D()) / hv_Box[0];
	hv_Box[2] = new_Dist;
	cout << "hv_ColEnd[0].D() X: " << hv_ColEnd[0].D() << endl;
	cout << "hv_RowEnd[0].D() Y: " << hv_RowEnd[0].D() << endl;
	return hv_ColEnd[0].D();
	//输出模板匹配后检测直线两端点的位置，即特征点位置。

}
//螺纹：
pair<double,double> program_1::cam0Picture1_algorithmLuowen(HObject ho_Image, HObject ho_Image_L, double L,float OUT2, int m, double Pix_zhoujing_chuanru)//测螺纹
{
	// Local iconic variables
	HObject  ho_ModelContours, ho_LineContours;
	HObject  ho_LineContour;

	// Local control variables
	HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
	HTuple  hv_MetrologyHandle, hv_Line1, hv_Line2, hv_Line3;
	HTuple  hv_Line4, hv_Line5, hv_Line6, hv_Line7, hv_Line8;
	HTuple  hv_LineIndices, hv_ReusedRefPointRow, hv_ReusedRefPointCol;
	HTuple  hv_NumLevels, hv_AngleStart, hv_AngleExtent, hv_AngleStep;
	HTuple  hv_ScaleMin, hv_ScaleMax, hv_ScaleStep, hv_Metric;
	HTuple  hv_MinContrast, hv_Row3, hv_Column3, hv_Angle, hv_Score;
	HTuple  hv_HomMat2D, hv_RowLine, hv_ColumnLine, hv_RowBegin;
	HTuple  hv_ColBegin, hv_RowEnd, hv_ColEnd, hv_Nr, hv_Nc;
	HTuple  hv_Dist;

	//读取模板和图像进行模板匹配*
	GetImageSize(ho_Image, &hv_Width, &hv_Height);
	hv_ModelFile = "./programParmeter/WZ20-15-001-80/cam0_2-luowen.sbm";
	ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
	//创建测量模型
	CreateMetrologyModel(&hv_MetrologyHandle);
	//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
	//求P下方

	hv_Line1.Clear();
	hv_Line1.Append(510 - 350);
	hv_Line1.Append(-10);
	hv_Line1.Append(510 - 380);
	hv_Line1.Append(40);
	hv_Line2.Clear();
	hv_Line2.Append(510 - 475);
	hv_Line2.Append(-10);
	hv_Line2.Append(510 - 510);
	hv_Line2.Append(40);
	hv_Line3.Clear();
	hv_Line3.Append(510 - 595);
	hv_Line3.Append(-10);
	hv_Line3.Append(510 - 625);
	hv_Line3.Append(40);
	//求P上方
	hv_Line4.Clear();
	hv_Line4.Append(518 - 450);
	hv_Line4.Append(-10);
	hv_Line4.Append(518 - 415);
	hv_Line4.Append(40);
	hv_Line5.Clear();
	hv_Line5.Append(518 - 570);
	hv_Line5.Append(-10);
	hv_Line5.Append(518 - 540);
	hv_Line5.Append(40);
	//求大径
	hv_Line6.Clear();
	hv_Line6.Append(518 - 375);
	hv_Line6.Append(50);
	hv_Line6.Append(518 - 435);
	hv_Line6.Append(50);
	hv_Line7.Clear();
	hv_Line7.Append(518 - 490);
	hv_Line7.Append(50);
	hv_Line7.Append(518 - 550);
	hv_Line7.Append(50);
	hv_Line8.Clear();
	hv_Line8.Append(500 - 600);
	hv_Line8.Append(50);
	hv_Line8.Append(500 - 660);
	hv_Line8.Append(50);

	AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", ((((((hv_Line1.TupleConcat(hv_Line2)).TupleConcat(hv_Line3)).TupleConcat(hv_Line4)).TupleConcat(hv_Line5)).TupleConcat(hv_Line6)).TupleConcat(hv_Line7)).TupleConcat(hv_Line8),
		20, 12, 1, 18, HTuple(), HTuple(), &hv_LineIndices);

	//获取读取的模板的轮廓，区域坐标等信息
	GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
	GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
	GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
		&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
	//进行模板匹配
	FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.5,
		1, 0.3, "least_squares", 0, 0.5, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
	//显示形状特征模板匹配结果

	VectorAngleToRigid(hv_ReusedRefPointRow, hv_ReusedRefPointCol, 0, hv_Row3, hv_Column3,
		hv_Angle, &hv_HomMat2D);
	//affine_trans_contour_xld (ModelContours, ContoursAffinTrans, HomMat2D)
	//dev_set_color ('green')
	//dev_display (ContoursAffinTrans)
	//对测量模型执行变换
	AlignMetrologyModel(hv_MetrologyHandle, hv_Row3, hv_Column3, hv_Angle);
	//获取变换后的测量模型的轮廓
	GetMetrologyObjectMeasures(&ho_LineContours, hv_MetrologyHandle, "all", "all",
		&hv_RowLine, &hv_ColumnLine);

	//卡尺找直线
	ApplyMetrologyModel(ho_Image, hv_MetrologyHandle);

	if (HDevWindowStack::IsOpen())
		SetColor(HDevWindowStack::GetActive(), "yellow");
	if (HDevWindowStack::IsOpen())
		SetLineWidth(HDevWindowStack::GetActive(), 2);
	if (HDevWindowStack::IsOpen())
		DispObj(ho_LineContours, HDevWindowStack::GetActive());
	GetMetrologyObjectResultContour(&ho_LineContour, hv_MetrologyHandle, "all", "all",
		1.5);
	//get_metrology_object_model_contour (ModelContour, MetrologyHandle, 'all', 1.5)
	if (HDevWindowStack::IsOpen())
		SetColor(HDevWindowStack::GetActive(), "blue");
	if (HDevWindowStack::IsOpen())
		SetLineWidth(HDevWindowStack::GetActive(), 2);
	if (HDevWindowStack::IsOpen())
		DispObj(ho_LineContour, HDevWindowStack::GetActive());
	FitLineContourXld(ho_LineContour, "tukey", -1, 0, 5, 2, &hv_RowBegin, &hv_ColBegin,
		&hv_RowEnd, &hv_ColEnd, &hv_Nr, &hv_Nc, &hv_Dist);

	vector<double> cam0_1_460_luowenP_left_x_TLineFP;
	vector<double> cam0_1_460_luowenP_left_y_TLineFP;
	vector<double> cam0_1_460_luowenP_right_x_TLineFP;
	vector<double> cam0_1_460_luowenP_right_y_TLineFP;

	vector<double> cam0_1_460_luowenD_up_x_TLineFP;
	vector<double> cam0_1_460_luowenD_up_y_TLineFP;
	vector<double> cam0_1_460_luowenD_low_x_TLineFP;
	vector<double> cam0_1_460_luowenD_low_y_TLineFP;
	//cout << "hv_RowBegin.Length():" << hv_RowBegin.Length() << endl;

	
	
	/*****************************/

	//中线位置 x00,用于转换坐标系
	// 利用光幕传递临近轴颈位置的读数，光幕直径读数为L，轴颈像素格数为 Pix_zhoujing
	double Pix_zhoujing = Pix_zhoujing_chuanru;
	if (m == 0) {
		OUT2_1 = OUT2;//如果是第一次传入，就把该值记录下，并保存到类外；
	}
	else {
		Pix_zhoujing = Pix_zhoujing;//补偿圆心偏移
	}
	double x00 = -(L / 2) / Calik + Pix_zhoujing;
	//cout << "中线位置:" << x00 << endl;
	//cout << "光幕:" << L<< endl;
	screwX00.push_back(x00);
	


	
	for (int t = 0; t < hv_RowBegin.Length(); t++)
	{
		// cout << t << endl;
		if (t < 5) {
			//cout << "hv_ColBegin[t].D():" << hv_ColBegin[t].D() << endl;
			//cout << "hv_RowBegin[t].D():" << hv_RowBegin[t].D() << endl;
			//cout << "hv_ColEnd[t].D():" << hv_ColEnd[t].D() << endl;
			//cout << "hv_RowEnd[t].D():" << hv_RowEnd[t].D() << endl;
			cam0_1_460_luowenP_left_x_TLineFP.push_back(hv_ColBegin[t].D());
			cam0_1_460_luowenP_left_y_TLineFP.push_back(5120 - hv_RowBegin[t].D());
			cam0_1_460_luowenP_right_x_TLineFP.push_back(hv_ColEnd[t].D());
			cam0_1_460_luowenP_right_y_TLineFP.push_back(5120 - hv_RowEnd[t].D());
		}
		else {
			cam0_1_460_luowenD_up_x_TLineFP.push_back(hv_ColEnd[t].D());
			cam0_1_460_luowenD_up_y_TLineFP.push_back(5120 - hv_RowEnd[t].D());
			cam0_1_460_luowenD_low_x_TLineFP.push_back(hv_ColBegin[t].D());
			cam0_1_460_luowenD_low_y_TLineFP.push_back(5120 - hv_RowBegin[t].D());
		}
		//cout << "right_x:" << cam0_1_460_luowenP_right_x_TLineFP[t] << "right_y:" << cam0_1_460_luowenP_right_y_TLineFP[t] << endl;
	}
	//cout << "cam0_1_460_luowenP_left_x_TLineFP.size():" << cam0_1_460_luowenP_left_x_TLineFP.size() << "cam0_1_460_luowenD_low_y_TLineFP.size():" << cam0_1_460_luowenD_low_y_TLineFP.size() << endl;
	

	//-----------------------计算外径D值------------------------//
	
	//(cam0_1_460_32TLineFP[1] + cam0_1_460_32TLineFP[3]) / 2;
	cout << "Pix_zhoujing:" << Pix_zhoujing << endl;
	//认为螺纹垂直，直接使用x轴的平均值作为螺纹大径的像素格数
	double Pix_luowen = (cam0_1_460_luowenD_up_x_TLineFP[0] + cam0_1_460_luowenD_up_x_TLineFP[1] +
		cam0_1_460_luowenD_up_x_TLineFP[2] + cam0_1_460_luowenD_low_x_TLineFP[0] + cam0_1_460_luowenD_low_x_TLineFP[1] + cam0_1_460_luowenD_low_x_TLineFP[2]) / 6;
	//cout << "1X值:" << cam0_1_460_luowenD_up_x_TLineFP[0] << endl;
	//cout << "2X值:" << cam0_1_460_luowenD_up_x_TLineFP[1] << endl;
	//cout << "3X值:" << cam0_1_460_luowenD_up_x_TLineFP[2] << endl;
	//cout << "4X值:" << cam0_1_460_luowenD_low_x_TLineFP[0] << endl;
	//cout << "5X值:" << cam0_1_460_luowenD_low_x_TLineFP[1] << endl;
	//cout << "6X值:" << cam0_1_460_luowenD_low_x_TLineFP[2] << endl;
	//cout << "Pix_luowen:" << Pix_luowen << endl;
	double D =  (Pix_luowen - x00 )* Calik * 2;
	//double D = 42;
	//cout << "D:" << D << endl;
	

	

	//-------------------计算螺距----------------------//
	//计算补偿前五条牙边直线的方程(由每条直线的两端点拟合)；利用k=(y2-y1)/(x2-x1)；k1,k2,k3为斜向右上直线，k4，k5为斜向右下直线
	double k1 = (cam0_1_460_luowenP_right_y_TLineFP[0] - cam0_1_460_luowenP_left_y_TLineFP[0]) / (cam0_1_460_luowenP_right_x_TLineFP[0] - cam0_1_460_luowenP_left_x_TLineFP[0]);
	double b1 = cam0_1_460_luowenP_right_y_TLineFP[0] - k1 * cam0_1_460_luowenP_right_x_TLineFP[0];

	double k2 = (cam0_1_460_luowenP_right_y_TLineFP[1] - cam0_1_460_luowenP_left_y_TLineFP[1]) / (cam0_1_460_luowenP_right_x_TLineFP[1] - cam0_1_460_luowenP_left_x_TLineFP[1]);
	double b2 = cam0_1_460_luowenP_right_y_TLineFP[1] - k2 * cam0_1_460_luowenP_right_x_TLineFP[1];

	double k3 = (cam0_1_460_luowenP_right_y_TLineFP[2] - cam0_1_460_luowenP_left_y_TLineFP[2]) / (cam0_1_460_luowenP_right_x_TLineFP[2] - cam0_1_460_luowenP_left_x_TLineFP[2]);
	double b3 = cam0_1_460_luowenP_right_y_TLineFP[2] - k3 * cam0_1_460_luowenP_right_x_TLineFP[2];

	double k4 = (cam0_1_460_luowenP_right_y_TLineFP[3] - cam0_1_460_luowenP_left_y_TLineFP[3]) / (cam0_1_460_luowenP_right_x_TLineFP[3] - cam0_1_460_luowenP_left_x_TLineFP[3]);
	double b4 = cam0_1_460_luowenP_right_y_TLineFP[3] - k4 * cam0_1_460_luowenP_right_x_TLineFP[3];

	double k5 = (cam0_1_460_luowenP_right_y_TLineFP[4] - cam0_1_460_luowenP_left_y_TLineFP[4]) / (cam0_1_460_luowenP_right_x_TLineFP[4] - cam0_1_460_luowenP_left_x_TLineFP[4]);
	double b5 = cam0_1_460_luowenP_right_y_TLineFP[4] - k5 * cam0_1_460_luowenP_right_x_TLineFP[4];
	//cout << "b1:" << b1 << " b2:" << b2 << " b4:" << b4 << endl;
	//cout << "k1:" << k1 << " k2:" << k2 << " k4:" << k4 << endl;

	

	//--------------------计算补偿值-----------------------------//
	//图像垂直放置，利用（外径-拟合第一条直线的最左x值），对所有牙边直线进行十段分割，为补偿z值做准备
	double c = (Pix_luowen - cam0_1_460_luowenP_left_x_TLineFP[0]) / 10;
	//cout << "cam0_1_460_luowenP_left_x_TLineFP[0]:" << cam0_1_460_luowenP_left_x_TLineFP[0] << endl;
	//cout << "double c：" << c << endl;
	double a = 3.1415926535 / 3;//牙型角为弧度值；每一种轴的牙型角可能不一样，要注意修改牙型角；此处直接使用设计值，因为牙型角偏差几乎不会影响整体测量。(-----------以后如果换轴，此位置需要改动---------------------)
	//double PI = 3.1415926535;//圆周率
	double C[10] = { 0 }; //保存十分段的每个段点所在实际轴径
	double A[10] = { 0 }; //保存每个段点的y补偿值
	double cc[10] = { 0 }; //保存每个段点的x值
	double YF1[10] = { 0 };//第一条直线补偿后的段点y值
	double YF2[10] = { 0 };//第二条直线补偿后的段点y值
	double YF4[10] = { 0 };//第四条直线补偿后的段点y值
	double YB1[10] = { 0 };//第一条直线补偿前的段点y值，用于计算P
	double YB2[10] = { 0 };//第二条直线补偿前的段点y值，用于计算P
	double YB3[10] = { 0 };//第三条直线补偿前的段点y值，用于计算P
	double YB4[10] = { 0 };//第三条直线补偿前的段点y值，用于计算P
	double YB21[10] = { 0 };//存放用于计算平均P的2-1
	double YB32[10] = { 0 };//存放用于计算平均P的3-2
	//由两平行线距离公式计算螺距P，三条直线可能k值会有不同，默认为K1和K2
	double P_sum = 0.0;
	double P = 0.0;
	for (int i = 0; i < 10; i++) {
		cc[i] = double(Pix_luowen - i * c);//计算每段的x值，确定从右往左算x值
		YB1[i] = double(k1 * cc[i] + b1);
		YB2[i] = double(k2 * cc[i] + b2);
		YB3[i] = double(k3 * cc[i] + b3);
		YB4[i] = double(k4 * cc[i] + b4);
		//cout << "补偿前直线1上的点：" << i << "," << YB1[i] << ")" << endl;
		//cout << "补偿前直线2上的点：" << i << "," << YB2[i] << ")" << endl;
		//cout << "补偿前直线4上的点：" << i << "," << YB4[i] << ")" << endl;
		P_sum = P_sum + double(YB2[i] - YB1[i]) + double(YB3[i] - YB2[i]);
		if (i == 9) {
			P = P_sum / 20 * Calik;
		}
	}
	//cout << "P:" << P << endl;



	for (int i = 0; i < 10; i++) {
		C[i] =( D - 2 * c * i * Calik)/2;//计算分段后，从大径向内侧的每一段的轴径
		//cout << "C[" << i << "]:" << C[i] << endl;

		//计算补偿值，周策策
		A[i] = (D / 2 - C[i]) * tan(a / 2) + P / 4 - (D / 2 - P * C[i] / sqrt(2 * PI * C[i] * tan(a / 2) * sqrt(PI * PI * C[i] * C[i] * tan(a / 2) * tan(a / 2) + P * P) - 2 * PI * PI * C[i] * C[i] * tan(a / 2) * tan(a / 2))) * tan(a / 2) - P / (2 * PI) * acos(PI * C[i] * tan(a / 2) / P - sqrt(1 + PI * PI * C[i] * C[i] * tan(a / 2) * tan(a / 2) / P / P));


		//补偿后的z值对应图像中的y值
		//YF1,2为向右上倾斜直线，故补偿值应添加
		//YF4为向下倾斜直线，故补偿值应减小

		/*//两上一下
		cout << "A[i]：" << A[i] << endl;
		YF1[i] = YB1[i] + A[i] / Calik;
		YF2[i] = YB2[i] + A[i] / Calik;
		YF4[i] = YB4[i] - A[i] / Calik;
		cout << "补偿后直线1上的点：" << i << "," << YF1[i] << endl;
		cout << "补偿后直线2上的点：" << i << "," << YF2[i] << endl;
		//cout << "补偿后直线2上的点：" << "(" << cc[i] << "," << YF2[i] << ")" << endl;
		cout << "补偿后直线4上的点：" << i << "," << YF4[i] << endl;*/


	}

	/*//--------------------计算补偿后直线-----------------------------//
	//计算第一条线的斜率和截距
	cv::Vec4f lines1;//存放拟合后的直线
	vector <cv::Point2f> point1;//待检测是否存在直线的所有点
	const static double Points1[10][2] = {
			{cc[0],YF1[0]},{cc[1],YF1[1]},{cc[2],YF1[2]},{cc[3],YF1[3]},
			{cc[4],YF1[4]},{cc[5],YF1[5]},{cc[6],YF1[6]},{cc[7],YF1[7]},
			{cc[8],YF1[8]},{cc[9],YF1[9]}
	};

	//将所有点存放在vector中，用于输入函数中
	for (int i = 0; i < 10; ++i) {
		point1.push_back(cv::Point2f(Points1[i][0], Points1[i][1]));
	}

	cv::fitLine(point1, lines1, cv::DIST_L1, 0, 0.01, 0.01);//opencv拟合库函数求取斜率和截距
	double NK1 = lines1[1] / lines1[0];//直线斜率
	double Nb1 = lines1[3] - NK1 * lines1[2];//截距


	//计算第二条线的斜率和截距
	cv::Vec4f lines2;//存放拟合后的直线
	vector <cv::Point2f> point2;//待检测是否存在直线的所有点
	const static double Points2[10][2] = {
			{cc[0],YF2[0]},{cc[1],YF2[1]},{cc[2],YF2[2]},{cc[3],YF2[3]},
			{cc[4],YF2[4]},{cc[5],YF2[5]},{cc[6],YF2[6]},{cc[7],YF2[7]},
			{cc[8],YF2[8]},{cc[9],YF2[9]}
	};

	//将所有点存放在vector中，用于输入函数中
	for (int i = 0; i < 10; ++i) {
		point2.push_back(cv::Point2f(Points2[i][0], Points2[i][1]));
	}

	fitLine(point2, lines2, cv::DIST_L1, 0, 0.01, 0.01);
	double NK2 = lines2[1] / lines2[0];//直线斜率
	double Nb2 = lines2[3] - NK2 * lines2[2];//截距

	//计算第四条线的斜率和截距
	cv::Vec4f lines4;//存放拟合后的直线
	vector <cv::Point2f> point4;//待检测是否存在直线的所有点
	const static double Points4[10][2] = {
			{cc[0],YF4[0]},{cc[1],YF4[1]},{cc[2],YF4[2]},{cc[3],YF4[3]},
			{cc[4],YF4[4]},{cc[5],YF4[5]},{cc[6],YF4[6]},{cc[7],YF4[7]},
			{cc[8],YF4[8]},{cc[9],YF4[9]}
	};

	//将所有点存放在vector中，用于输入函数中
	for (int i = 0; i < 10; ++i) {
		point4.push_back(cv::Point2f(Points4[i][0], Points4[i][1]));
	}

	fitLine(point4, lines4, cv::DIST_L1, 0, 0.01, 0.01);
	double NK4 = lines4[1] / lines4[0];//直线斜率
	double Nb4 = lines4[3] - NK4 * lines4[2];//截距*/
	

	//--------------计算中径x轴的纵坐标------------------------//
	//int X_jiexian = 100;//选择一条垂直线截取螺纹，这条线的大小必须小于大径，大于小径(---------------以后如果换轴，此位置需要改动-------------------)
	double X_jiexian = (Pix_luowen - cam0_1_460_luowenP_left_x_TLineFP[0]) / 2 + cam0_1_460_luowenP_left_x_TLineFP[0];//选择一条垂直线截取螺纹,固定
	//cout << "X_jiexian:" << X_jiexian << endl;
	//cout << "NK1:" << NK1 << " NK2:" << NK2 << " NK4:" << NK4 << endl;
	//cout << "Nb1:" << Nb1 << " Nb2:" << Nb2 << " Nb4:" << Nb4 << endl;
	//double Y_jie1 = (NK1 - 2*NK4 + NK2) / 4 * X_jiexian + Nb1;//(NK1 - 2 * NK4 + NK2) / 4 旨在中和三线斜率的误差
	//double Y_jie2 = (NK1 -  2*NK4 + NK2) / 4 * X_jiexian + Nb2;
	//double Y_jie4 = -(NK1 -  2*NK4 + NK2) / 4 * X_jiexian + Nb4;
	double Y_jie1 = k1 * X_jiexian + b1;//(NK1 - 2 * NK4 + NK2) / 4 旨在中和三线斜率的误差
	double Y_jie2 = k2 * X_jiexian + b2;
	double Y_jie4 = k4 * X_jiexian + b4;
	//cout << "Y_jie1:" << Y_jie1 << "Y_jie2:" << Y_jie2 << "Y_jie4:" << Y_jie4 << endl;
	//计算垂直平分线的中点
	double Y_zhongjing = ((Y_jie4 + (Y_jie2 - Y_jie4) / 2) - ((Y_jie4 - Y_jie1) / 2 + Y_jie1)) / 2 + ((Y_jie4 - Y_jie1) / 2 + Y_jie1);
	cout << "Y_zhongjing:" << Y_zhongjing << endl;
	double Pix_d = (Y_zhongjing - b4) / k4;//中点落在第四条直线上
	cout << " Pix_d:" << Pix_d << endl;
	screwZhongjingCOl.push_back(Pix_d);//
	/*****************************/
	//传入基准A的拟合直线
	double hv_Box[3];
	//cout << "program_1::cam0Picture1_L_K_b开始执行" << endl;
	double end_col_before = cam0Picture1_L_K_b(ho_Image_L, hv_Box);
	//cout << "program_1::cam0Picture1_L_K_b执行完毕,end_col_before=" << end_col_before << endl;
	//cout << "Nr=" << hv_Box[0] << "Nc=" << hv_Box[1] <<"Dist="<< hv_Box[2] << endl;
	double Pianyi_b =end_col_before - Pix_zhoujing_chuanru;
	double axisA_k = -(hv_Box[1] / hv_Box[0]);
	//cout << " axisA_k=" << atan(axisA_k)*180/3.1415926  << endl;
	cout << " axisA_k=" << axisA_k << endl;
	cout << " Pianyi_b=" << Pianyi_b << endl;
	cout << " Pix_zhoujing_chuanru=" << Pix_zhoujing_chuanru << endl;
	double axisA_b = hv_Box[2];
	cout << " 直线" << " y=" << axisA_k << " x +" << axisA_b << endl;
	//-----------------计算中径--------------//
	//中径实际尺寸d =大径实际尺寸-（螺纹大径的x轴像素格数-中径所在像素格数）*像素尺寸

	//double AdL = (abs(hv_Box[1] *(Pix_d+A[4]/Calik/tan(a/2))- hv_Box[0] *Y_zhongjing +hv_Box[1]* Pianyi_b+5120* hv_Box[0]+hv_Box[2]) /(sqrt(hv_Box[1] * hv_Box[1] + hv_Box[0]* hv_Box[0])))* Calik;
	double AdL = (abs(-(hv_Box[1]) * (Pix_d + A[4] / Calik / tan(a / 2)) +  Y_zhongjing* hv_Box[0] -(hv_Box[1]) * Pianyi_b - 5120* hv_Box[0] -56.5* hv_Box[0] /0.01218+ axisA_b* hv_Box[0]) / (sqrt(hv_Box[1]* hv_Box[1]+ hv_Box[0]* hv_Box[0]))) * Calik;
	//double AdL = (abs(hv_Box[1] * Pix_d  - hv_Box[0] * Y_zhongjing - hv_Box[2]) / (sqrt(hv_Box[1] * hv_Box[1] + hv_Box[0] * hv_Box[0]))) * Calik;
	cout << "AdL=" << AdL << endl;
	double d = -2 * AdL + L;
	//double d = (Pix_d - x00) * Calik * 2;
	cout << "中径d:" << d  << endl;
	
	//返回中径
	return make_pair(d,AdL);
}

HObject program_1::imgAug(string imgPath) {
	HObject  ho_Image, ho_LineContours, ho_LineContour;
	// Local control variables
	HTuple  hv_Width, hv_Height;
	HTuple  halconPath = imgPath.c_str();
	ReadImage(&ho_Image, halconPath);
	GetImageSize(ho_Image, &hv_Width, &hv_Height);
	//图像增强
	HObject   ho_GammaImage, ho_ImageEmphasize;
	GammaImage(ho_Image, &ho_GammaImage, 0.416667, 0.055, 0.018, 255, "true");
	Emphasize(ho_GammaImage, &ho_ImageEmphasize, hv_Width, hv_Height, 1.4);
	//图像增强结束
	return ho_ImageEmphasize;
}
/*
函数作用：远心相机视觉算法
*/
void  program_1::cam0Picture_algorithm()
{
	string Img1path = imgSavePath[0];
	string Img2path = imgSavePath[1];
	string Img3path = imgSavePath[2];
	string Img4path = imgSavePath[3];
	string Img5path = imgSavePath[4];

	HObject Img1Aug = imgAug(Img1path);
	HObject Img2Aug = imgAug(Img2path);
	HObject Img3Aug = imgAug(Img3path);
	HObject Img4Aug = imgAug(Img4path);
	HObject Img5Aug = imgAug(Img5path);

	HTuple halconPath = Img1path.c_str();
	HObject Img1_raw;
	ReadImage(&Img1_raw, halconPath);

	halconPath = Img2path.c_str();
	HObject Img2_raw;
	ReadImage(&Img2_raw, halconPath);

	halconPath = Img3path.c_str();
	HObject Img3_raw;
	ReadImage(&Img3_raw, halconPath);

	halconPath = Img4path.c_str();
	HObject Img4_raw;
	ReadImage(&Img4_raw, halconPath);

	halconPath = Img5path.c_str();
	HObject Img5_raw;
	ReadImage(&Img5_raw, halconPath);


	int fault_detect = 0;

	double cam0_1_0_TLineFP[4];
	Straight_TLineFP_P(cam0_1_0_TLineFP, Img1Aug,
		244, 900, 244, 1000,
		50, 12, 1, 1);

	double cam0_1_15_TLineFP[4];
	Straight_TLineFP_P(cam0_1_15_TLineFP, Img1Aug,
		1481, 900, 1481, 1000,
		50, 12, 1, 1);

	double cam0_1_25_TLineFP[4];
	Straight_TLineFP_P(cam0_1_25_TLineFP, Img1Aug,
		2295, 900, 2295, 1000,
		50, 12, 1, 1);

	double cam0_1_35_TLineFP[4];
	Straight_TLineFP_P(cam0_1_35_TLineFP, Img1Aug,
		3122, 900, 3122, 1000,
		50, 12, 1, 1);

	double cam0_1_45_TLineFP[4];
	Straight_TLineFP_P(cam0_1_45_TLineFP, Img1Aug,
		3936, 900, 3936, 1000,
		50, 12, 1, 1);

	double cam0_1_55_TLineFP[4];
	Straight_TLineFP_P(cam0_1_55_TLineFP, Img1Aug,
		4763, 900, 4763, 1000,
		50, 12, 1, 1);

	//图像2
	double cam0_2_65_TLineFP[4];
	Straight_TLineFP_P(cam0_2_65_TLineFP, Img2Aug,
		573, 900, 573, 1000,
		50, 12, 1, 1);

	double cam0_2_75_TLineFP[4];
	Straight_TLineFP_P(cam0_2_75_TLineFP, Img2Aug,
		1393, 1000, 1393, 1070,
		50, 12, 1, 1);

	double cam0_2_85_TLineFP[4];
	Straight_TLineFP_P(cam0_2_85_TLineFP, Img2Aug,
		2213, 800, 2213, 900,
		50, 12, 1, 1);

	double cam0_2_95_TLineFP[4];
	Straight_TLineFP_P(cam0_2_95_TLineFP, Img2Aug,
		3033, 900, 3033, 1000,
		50, 12, 1, 1);

	double cam0_2_105_TLineFP[4];
	Straight_TLineFP_P(cam0_2_105_TLineFP, Img2Aug,
		3853, 900, 3853, 1000,
		50, 12, 1, 1);

	double cam0_2_115_TLineFP[4];
	Straight_TLineFP_P(cam0_2_115_TLineFP, Img2Aug,
		4673, 900, 4673, 1000,
		50, 12, 1, 1);
	//图像3
	double cam0_3_125_TLineFP[4];
	Straight_TLineFP_P(cam0_3_125_TLineFP, Img3Aug,
		573, 990, 573, 1070,
		50, 12, 1, 1);

	double cam0_3_135_TLineFP[4];
	Straight_TLineFP_P(cam0_3_135_TLineFP, Img3Aug,
		1393, 900, 1393, 1000,
		50, 12, 1, 1);

	double cam0_3_145_TLineFP[4];
	Straight_TLineFP_P(cam0_3_145_TLineFP, Img3Aug,
		2213, 990, 2213, 1070,
		50, 12, 1, 1);

	double cam0_3_155_TLineFP[4];
	Straight_TLineFP_P(cam0_3_155_TLineFP, Img3Aug,
		3033, 900, 3033, 1000,
		50, 12, 1, 1);

	double cam0_3_165_TLineFP[4];
	Straight_TLineFP_P(cam0_3_165_TLineFP, Img3Aug,
		3853, 990, 3853, 1070,
		50, 12, 1, 1);

	double cam0_3_175_TLineFP[4];
	Straight_TLineFP_P(cam0_3_175_TLineFP, Img3Aug,
		4673, 900, 4673, 1000,
		50, 12, 1, 1);

	//图像4
	double cam0_4_185_TLineFP[4];
	Straight_TLineFP_P(cam0_4_185_TLineFP, Img4Aug,
		573, 990, 573, 1070,
		50, 12, 1, 1);

	double cam0_4_195_TLineFP[4];
	Straight_TLineFP_P(cam0_4_195_TLineFP, Img4Aug,
		1393, 900, 1393, 1000,
		50, 12, 1, 1);

	double cam0_4_205_TLineFP[4];
	Straight_TLineFP_P(cam0_4_205_TLineFP, Img4Aug,
		2213, 990, 2213, 1070,
		50, 12, 1, 1);

	double cam0_4_215_TLineFP[4];
	Straight_TLineFP_P(cam0_4_215_TLineFP, Img4Aug,
		3033, 900, 3033, 1000,
		50, 12, 1, 1);

	double cam0_4_225_TLineFP[4];
	Straight_TLineFP_P(cam0_4_225_TLineFP, Img4Aug,
		3853, 990, 3853, 1070,
		50, 12, 1, 1);

	double cam0_4_235_TLineFP[4];
	Straight_TLineFP_P(cam0_4_235_TLineFP, Img4Aug,
		4673, 900, 4673, 1000,
		50, 12, 1, 1);

	//图像5
	double cam0_5_245_TLineFP[4];
	Straight_TLineFP_P(cam0_5_245_TLineFP, Img5Aug,
		410, 990, 410, 1070,
		50, 12, 1, 1);

	double cam0_5_255_TLineFP[4];
	Straight_TLineFP_P(cam0_5_255_TLineFP, Img5Aug,
		1233, 900, 1233, 1000,
		50, 12, 1, 1);

	double cam0_5_265_TLineFP[4];
	Straight_TLineFP_P(cam0_5_265_TLineFP, Img5Aug,
		2053, 990, 2053, 1070,
		50, 12, 1, 1);

	double cam0_5_275_TLineFP[4];
	Straight_TLineFP_P(cam0_5_275_TLineFP, Img5Aug,
		2873, 900, 2873, 1000,
		50, 12, 1, 1);

	double cam0_5_285_TLineFP[4];
	Straight_TLineFP_P(cam0_5_285_TLineFP, Img5Aug,
		3693, 990, 3693, 1070,
		50, 12, 1, 1);

	double cam0_5_300_TLineFP[4];
	Straight_TLineFP_P(cam0_5_300_TLineFP, Img5Aug,
		4927, 980, 4927, 1050,
		50, 12, 1, 1);


	//以下程序对特征集中处理，不涉及特征点检测

	double AxialD_biaozhun_15 = (cam0_1_15_TLineFP[0] + cam0_1_15_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_15 * Calik);
	pushback_vectors(resultVectorList, 4, 15, 15, -1, +1);

	double AxialD_biaozhun_25 = (cam0_1_25_TLineFP[0] + cam0_1_25_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_25 * Calik);
	pushback_vectors(resultVectorList, 4, 25, 25, -1, +1);

	double AxialD_biaozhun_35 = (cam0_1_35_TLineFP[0] + cam0_1_35_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_35 * Calik);
	pushback_vectors(resultVectorList, 4, 35, 35, -1, +1);

	double AxialD_biaozhun_45 = (cam0_1_45_TLineFP[0] + cam0_1_45_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_45 * Calik);
	pushback_vectors(resultVectorList, 4, 45, 45, -1, +1);

	double AxialD_biaozhun_55 = (cam0_1_55_TLineFP[0] + cam0_1_55_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_55 * Calik);
	pushback_vectors(resultVectorList, 4, 55, 55, -1, +1);

	//图片2
	double AxialD_biaozhun_65 = (cam0_2_65_TLineFP[0] + cam0_2_65_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_65 * Calik + pic1To2_moveDistance);
	pushback_vectors(resultVectorList, 4, 65, 65, -1, +1);

	double AxialD_biaozhun_75 = (cam0_2_75_TLineFP[0] + cam0_2_75_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_75 * Calik + pic1To2_moveDistance);
	pushback_vectors(resultVectorList, 4, 75, 75, -1, +1);

	double AxialD_biaozhun_85 = (cam0_2_85_TLineFP[0] + cam0_2_85_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_85 * Calik + pic1To2_moveDistance);
	pushback_vectors(resultVectorList, 4, 85, 85, -1, +1);

	double AxialD_biaozhun_95 = (cam0_2_95_TLineFP[0] + cam0_2_95_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_95 * Calik + pic1To2_moveDistance);
	pushback_vectors(resultVectorList, 4, 95, 95, -1, +1);

	double AxialD_biaozhun_105 = (cam0_2_105_TLineFP[0] + cam0_2_105_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_105 * Calik + pic1To2_moveDistance);
	pushback_vectors(resultVectorList, 4, 105, 105, -1, +1);

	double AxialD_biaozhun_115 = (cam0_2_115_TLineFP[0] + cam0_2_115_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_115 * Calik + pic1To2_moveDistance);
	pushback_vectors(resultVectorList, 4, 115, 115, -1, +1);

	//图片3
	double AxialD_biaozhun_125 = (cam0_3_125_TLineFP[0] + cam0_3_125_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_125 * Calik + pic1To2_moveDistance + pic2To3_moveDistance);
	pushback_vectors(resultVectorList, 4, 125, 125, -1, +1);

	double AxialD_biaozhun_135 = (cam0_3_135_TLineFP[0] + cam0_3_135_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_135 * Calik + pic1To2_moveDistance + pic2To3_moveDistance);
	pushback_vectors(resultVectorList, 4, 135, 135, -1, +1);

	double AxialD_biaozhun_145 = (cam0_3_145_TLineFP[0] + cam0_3_145_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_145 * Calik + pic1To2_moveDistance + pic2To3_moveDistance);
	pushback_vectors(resultVectorList, 4, 145, 145, -1, +1);

	double AxialD_biaozhun_155 = (cam0_3_155_TLineFP[0] + cam0_3_155_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_155 * Calik + pic1To2_moveDistance + pic2To3_moveDistance);
	pushback_vectors(resultVectorList, 4, 155, 155, -1, +1);

	double AxialD_biaozhun_165 = (cam0_3_165_TLineFP[0] + cam0_3_165_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_165 * Calik + pic1To2_moveDistance + pic2To3_moveDistance);
	pushback_vectors(resultVectorList, 4, 165, 165, -1, +1);

	double AxialD_biaozhun_175 = (cam0_3_175_TLineFP[0] + cam0_3_175_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_175 * Calik + pic1To2_moveDistance + pic2To3_moveDistance);
	pushback_vectors(resultVectorList, 4, 175, 175, -1, +1);

	//图片4
	double AxialD_biaozhun_185 = (cam0_4_185_TLineFP[0] + cam0_4_185_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_185 * Calik + pic1To2_moveDistance + pic2To3_moveDistance + pic3To4_moveDistance);
	pushback_vectors(resultVectorList, 4, 185, 185, -1, +1);

	double AxialD_biaozhun_195 = (cam0_4_195_TLineFP[0] + cam0_4_195_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_195 * Calik + pic1To2_moveDistance + pic2To3_moveDistance + pic3To4_moveDistance);
	pushback_vectors(resultVectorList, 4, 195, 195, -1, +1);

	double AxialD_biaozhun_205 = (cam0_4_205_TLineFP[0] + cam0_4_205_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_205 * Calik + pic1To2_moveDistance + pic2To3_moveDistance + pic3To4_moveDistance);
	pushback_vectors(resultVectorList, 4, 205, 205, -1, +1);

	double AxialD_biaozhun_215 = (cam0_4_215_TLineFP[0] + cam0_4_215_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_215 * Calik + pic1To2_moveDistance + pic2To3_moveDistance + pic3To4_moveDistance);
	pushback_vectors(resultVectorList, 4, 215, 215, -1, +1);

	double AxialD_biaozhun_225 = (cam0_4_225_TLineFP[0] + cam0_4_225_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_225 * Calik + pic1To2_moveDistance + pic2To3_moveDistance + pic3To4_moveDistance);
	pushback_vectors(resultVectorList, 4, 225, 225, -1, +1);

	double AxialD_biaozhun_235 = (cam0_4_235_TLineFP[0] + cam0_4_235_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_235 * Calik + pic1To2_moveDistance + pic2To3_moveDistance + pic3To4_moveDistance);
	pushback_vectors(resultVectorList, 4, 235, 235, -1, +1);

	//图片5
	double AxialD_biaozhun_245 = (cam0_5_245_TLineFP[0] + cam0_5_245_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_245 * Calik + pic1To2_moveDistance + pic2To3_moveDistance + pic3To4_moveDistance + pic4To5_moveDistance);
	pushback_vectors(resultVectorList, 4, 245, 245, -1, +1);

	double AxialD_biaozhun_255 = (cam0_5_255_TLineFP[0] + cam0_5_255_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_255 * Calik + pic1To2_moveDistance + pic2To3_moveDistance + pic3To4_moveDistance + pic4To5_moveDistance);
	pushback_vectors(resultVectorList, 4, 255, 255, -1, +1);

	double AxialD_biaozhun_265 = (cam0_5_265_TLineFP[0] + cam0_5_265_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_265 * Calik + pic1To2_moveDistance + pic2To3_moveDistance + pic3To4_moveDistance + pic4To5_moveDistance);
	pushback_vectors(resultVectorList, 4, 265, 265, -1, +1);

	double AxialD_biaozhun_275 = (cam0_5_275_TLineFP[0] + cam0_5_275_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_275 * Calik + pic1To2_moveDistance + pic2To3_moveDistance + pic3To4_moveDistance + pic4To5_moveDistance);
	pushback_vectors(resultVectorList, 4, 275, 275, -1, +1);

	double AxialD_biaozhun_285 = (cam0_5_285_TLineFP[0] + cam0_5_285_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_285 * Calik + pic1To2_moveDistance + pic2To3_moveDistance + pic3To4_moveDistance + pic4To5_moveDistance);
	pushback_vectors(resultVectorList, 4, 285, 285, -1, +1);

	double AxialD_biaozhun_300 = (cam0_5_300_TLineFP[0] + cam0_5_300_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_300 * Calik + pic1To2_moveDistance + pic2To3_moveDistance + pic3To4_moveDistance + pic4To5_moveDistance);
	pushback_vectors(resultVectorList, 4, 300, 300, -1, +1);
};


//用于测试的函数************



//用于测试的函数************


void program_1::run()
{
	//检测运行提示（设备自检）*****************************************************************************************************
	

	programConfirm = "";//检查拨叉位置是否安装正确
	emit programTips("请检查工件种类及安装是否正确？零件编号是否输入正确？确保拨叉位于相机侧后，点击'程序确认按钮'进行检测！",1);
	do
	{
		msleep(500);
	} while (programConfirm != "continue");
	emit updateDeviceInf("正在自动测量，请等待测量完成且勿执行其他操作");
	

	//测量初始化*******************************************************************************************************


	//清空程序所用的动态数组和主界面表格控件
	//由于不需要跳动和孔径测量，所以将相关变量屏蔽掉
	
	//cam1PathList.clear();
	//cam1PathList.shrink_to_fit();
	//clearRoundoutData();
	//clearScrewRoundOutData();
	int tableRowCount = measureTablePtr->rowCount();
	for (int i = 0; i < tableRowCount; i++)
	{
		measureTablePtr->removeRow(0);
	};
	emit partsNumber(partNub);//向主页面发送程序号
	std::string stime;//获取当前时间
	std::stringstream strtime;
	std::time_t currenttime = std::time(0);
	char tAll[255];
	std::strftime(tAll, sizeof(tAll), "%Y%m%d_%H%M%S", std::localtime(&currenttime));
	strtime << tAll;
	stime = strtime.str();
	char cwd[256];//获取当前检测程序目录
	_getcwd(cwd, 256);
	string defaultPath = cwd;
	//cout << "运行目录" << defaultPath << endl;
	defaultPath = defaultPath+"\\measureData\\p10";
	dataSavePath = defaultPath + "\\" + stime;//新建文件夹
	//cout << "保存目录" << dataSavePath << endl;
	string command;
	command = "mkdir -p " + dataSavePath;
	system(command.c_str());
	cout << dataSavePath << endl;
	allFeatureFlag = "OK";//全局特征结果重置
	ngFeatureNum = 0;//NG特征数量重置
	emit measureStatistics("--", ngFeatureNum, measurePartsNum_all, yield);


	//使用光幕传感器进行直径测量检测（上下来回求平均）***************************************************************************
	
	
	
	for (int i = 0; i < 5; i++)//远心1-2号采集位置采集图像
	{
		camPtrList[0]->setExposeTime(cam0_exposeTime);
		camPtrList[0]->m_captureMode = "continuous";
		originalImgPtr = &(camPtrList[0]->capturedImg);
		emit programProcess(QString("远心相机正在对 %1 号位置进行轮廓测量！").arg(i+1), 48+(i+1)*15);
		imgSavePath[i] = cam0_Measure_prepare(i);
	};
	pic1To2_moveDistance = axis5_compensation(cam0_arriveOrgEncode_axis5[1]) - axis5_compensation(cam0_arriveOrgEncode_axis5[0]);
	pic2To3_moveDistance = axis5_compensation(cam0_arriveOrgEncode_axis5[2]) - axis5_compensation(cam0_arriveOrgEncode_axis5[1]);
	pic3To4_moveDistance = axis5_compensation(cam0_arriveOrgEncode_axis5[3]) - axis5_compensation(cam0_arriveOrgEncode_axis5[2]);
	pic4To5_moveDistance = axis5_compensation(cam0_arriveOrgEncode_axis5[4]) - axis5_compensation(cam0_arriveOrgEncode_axis5[3]);
	cout << "移动距离" << pic1To2_moveDistance << endl;
	cout << "移动距离" << pic2To3_moveDistance << endl;
	cout << "移动距离" << pic3To4_moveDistance << endl;
	cout << "移动距离" << pic4To5_moveDistance << endl;

	try {
		cam0Picture_algorithm();
	}
	catch(...){
		emit updateDeviceInf(QString("远心相机图像%1处理错误").arg(fault_detect));
	};
	
	//五轴回合适位置
	moveControlPtr->setCurrentAxis(5);
	moveControlPtr->setTrapPrm(moveControlPtr->axisCore[moveControlPtr->currentAxisIndex], moveControlPtr->currentAxisNumber, moveControlPtr->trapAuto[moveControlPtr->currentAxisIndex], 0, moveControlPtr->trapHighVelAuto[moveControlPtr->currentAxisIndex]);
	moveControlPtr->startTrap();
	//clearRoundoutData();
	emit programProcess("各轴正在回到合适位置,并整理测量结果，请等待！", 97);//检测统计值计算以及显示
	measureTablePtr->sortItems(0, Qt::AscendingOrder);
	if (allFeatureFlag == "OK")
	{
		measurePartsNum_ok++;
	}
	else
	{
		measurePartsNum_ng++;
	};
	measurePartsNum_all++;
	yield = (measurePartsNum_ok / measurePartsNum_all) * 100;
	emit measureStatistics("--", ngFeatureNum, measurePartsNum_all, yield);


	//删除照片文件
	/*
	replace(dataSavePath.begin(), dataSavePath.end(), '\\', '/');
	std::vector<std::string> allowedExtensions = { ".bmp" };
	fs::path folderPath(dataSavePath);

	if (!fs::exists(folderPath)) {
		std::cout << "Folder does not exist." << std::endl;
	}
	else {
		std::cout << "文件夹存在！" << std::endl;
	}

	deleteFolderContents(folderPath, allowedExtensions);
	*/
	emit programProcess("数据整理完成，请确认检测结果！", 98);//检测统计值计算以及显示
	programConfirm = "";
	do
	{
		msleep(300);
	} while (programConfirm != "save" && programConfirm != "delete");
	if (programConfirm == "save")
	{
		cout << "写入" << endl;
		writeToDatabase();
		saveAsExcel();//检测结果储存为excel文件
	};
	do
	{
		moveControlPtr->updateAxisStatus(moveControlPtr->currentAxisNumber);
		msleep(200);
	} while (!moveControlPtr->bFlagArrive[moveControlPtr->currentAxisIndex] && moveControlPtr->bFlagMotion[moveControlPtr->currentAxisIndex]);
	emit programProcess("全部测量完成 ！", 100);//检测全部完成提示
	if (allFeatureFlag == "OK")
	{
		emit Finished(true);
	}
	else
	{
		emit Finished(false);
	};
	emit updateDeviceInf("当前测量已完成，请取走零件");
};

/*
sort(fGlobal.begin(), fGlobal.end());
*/