//参考代码****************************************************************************


#include "program_29.h"

program_29::program_29(cam_device* cam0, cam_device* cam1, cam_device* cam2, moveControl* cradDevicePtr, ls_device* lsDevicePtr, QTableWidget* tablePtr, QSqlDatabase* DbPtr, roughnessFun* roughnessObjPtr)
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
	partsID = "未输入";
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
	for (int i = 0; i < 7; i++)//如有跳动测量需求将i的循环上限修改为需要进行跳动采集的轴号（!!!!只是为了测试新建的子程序是否添加到主程序中时可以暂时不用修改）
	{
		roundoutDataPtr[i]= new roundoutData();
	};
	clearRoundoutData();//如有跳动测量需求按ctrl点击该函数跳转到该函数定义进行修改（!!!!只是为了测试新建的子程序是否添加到主程序中时可以暂时不用修改）

	//螺纹跳动数据结构体初始化
	
	for (int i = 0; i < 2; i++)
	{
		myScrewRoundoutDataPtr[i] = new screwRoundoutData();
	};
	clearScrewRoundOutData();
	
};
program_29::~program_29()
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
bool program_29::stepLengthCompensationCaculation_axis5()
{
	moveControlPtr->setCurrentAxis(5);
	moveControlPtr->setMoveMode("Trap");
	moveControlPtr->setTrapPrm(moveControlPtr->axisCore[moveControlPtr->currentAxisIndex], moveControlPtr->currentAxisNumber, moveControlPtr->trapAuto[moveControlPtr->currentAxisIndex], stepLengthCompensation_cam0Place, moveControlPtr->trapHighVelAuto[moveControlPtr->currentAxisIndex]);
	moveControlPtr->startTrap();
	do
	{
		moveControlPtr->updateAxisStatus(moveControlPtr->currentAxisNumber);
		msleep(200);
	} while (!moveControlPtr->bFlagArrive[moveControlPtr->currentAxisIndex] && moveControlPtr->bFlagMotion[moveControlPtr->currentAxisIndex]);
	msleep(300);
	camPtrList[0]->startCapture();
	string imgPath = dataSavePath + "\\cam0_forCompensation.bmp";
	msleep(2000);
	moveControlPtr->updateAxisStatus(moveControlPtr->currentAxisNumber);
	camPtrList[0]->stopCapture();
	emit imgInf(originalImgPtr, "org", 0);
	camPtrList[0]->saveImg(imgPath, 0, 0, 0);
	replace(imgPath.begin(), imgPath.end(), '\\', '/');
	try {
		/*匹配轴向移动点位开始*/
		string Img1path_pattern = "./programParmeter/WZ10-11-2006-430/DQ.bmp";
		HObject Img1Aug_pattern = imgAug(Img1path_pattern);
		HTuple Pattern_path = "./programParmeter/WZ10-11-2006-430/cam0_1-430-5.sbm";
		HObject Img1Aug_dianwei = imgAug(imgPath);

		double cam0_1_6right_muban_TLineFP[4];
		TemplateMatching_TLineFP_P(cam0_1_6right_muban_TLineFP, Img1Aug_pattern,
			Pattern_path,
			30, 50, 250, 50,
			50, 12, 1, 70,
			0.5, 1, 0.3, 0, 0.7);

		double cam0_1_6right_new_TLineFP[4];
		TemplateMatching_TLineFP_P(cam0_1_6right_new_TLineFP, Img1Aug_dianwei,
			Pattern_path,
			30, 50, 250, 50,
			50, 12, 1, 70,
			0.5, 1, 0.3, 0, 0.7);
		//需要移动的光栅读数（相对于点位367960） 367960+dianwei_moveDistance=现在应该达到的点位
		stepLengthCompensation_axis5 = long((cam0_1_6right_new_TLineFP[0] - cam0_1_6right_muban_TLineFP[0]) * 0.01218 / 0.0005);
		/*匹配轴向移动点位结束*/

		return true;
	}
	catch (...)
	{
		return false;
	}
};
void program_29::lsSensorMeasure_diameter(int positionNumber, int time)
{
	//使用光幕传感器进行直径测量
	moveControlPtr->setTrapPrm(moveControlPtr->axisCore[moveControlPtr->currentAxisIndex], moveControlPtr->currentAxisNumber, moveControlPtr->trapAuto[moveControlPtr->currentAxisIndex], ls_StepLength_axis5[positionNumber]+ stepLengthCompensation_axis5, moveControlPtr->trapHighVelAuto[moveControlPtr->currentAxisIndex]);
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
void program_29::lsSensorMeasure_cylindricity(int positionNumber, float intervalTime = 50)
{
	//初始化变量
	vector<float> diameter;

	for (int i = 0; i < 3; i++)
	{
		moveControlPtr->setCurrentAxis(5);
		moveControlPtr->setMoveMode("Trap");
		moveControlPtr->setTrapPrm(moveControlPtr->axisCore[moveControlPtr->currentAxisIndex], moveControlPtr->currentAxisNumber, moveControlPtr->trapAuto[moveControlPtr->currentAxisIndex], lsCylindricitySteplength_axis5[positionNumber][i] + stepLengthCompensation_axis5, moveControlPtr->trapHighVelAuto[moveControlPtr->currentAxisIndex]);
		moveControlPtr->startTrap();
		do
		{
			moveControlPtr->updateAxisStatus(moveControlPtr->currentAxisNumber);
			msleep(200);
		} while (!moveControlPtr->bFlagArrive[moveControlPtr->currentAxisIndex] && moveControlPtr->bFlagMotion[moveControlPtr->currentAxisIndex]);
		cout << "圆柱度位置" << i + 1 << endl;
		long axis7_steplength = 360 * 500;//一个脉冲0.002度
		moveControlPtr->setCurrentAxis(7);
		moveControlPtr->zeroPosition();
		moveControlPtr->setMoveMode("Trap");
		moveControlPtr->setTrapPrm(moveControlPtr->axisCore[moveControlPtr->currentAxisIndex], moveControlPtr->currentAxisNumber, moveControlPtr->trapAuto[moveControlPtr->currentAxisIndex], axis7_steplength, moveControlPtr->trapHighVelAuto[moveControlPtr->currentAxisIndex]);
		moveControlPtr->startTrap();
		do
		{
			diameter.push_back(diameter_compensation(ls_devicePtr->getLsMeasurementValue(1)));
			moveControlPtr->updateAxisStatus(moveControlPtr->currentAxisNumber);
			msleep(intervalTime);
		} while (moveControlPtr->bFlagMotion[moveControlPtr->currentAxisIndex]);
	};
	//一下是新追加的内容
	sort(diameter.begin(), diameter.end());
	int dataSize = diameter.size();
	lsCylindricityResult[positionNumber][1] = (diameter[dataSize - 15] - diameter[0]) / 2;
	cout << "最大直径数据：" << diameter[dataSize - 1] << "10直径数据：" << diameter[dataSize - 10] << "20直径数据：" << diameter[dataSize - 20] << "30直径数据" << diameter[dataSize - 30] << "   最小直径数据：" << diameter[0] << endl;
	//原有内容
	/*
	float maxDiameter, minDiameter;
	maxDiameter = *max_element(diameter.begin(), diameter.end());
	minDiameter = *min_element(diameter.begin(), diameter.end());
	lsCylindricityResult[positionNumber][1] = (maxDiameter - minDiameter) / 2;
	*/
	resultVectorList.push_back(lsCylindricityResult[positionNumber][1]);
	pushback_vectors(resultVectorList, 6, lsCylindricityResult[positionNumber][0], 0, 0, lsCylindricityResult[positionNumber][2]);
	//便于调试观察
	/*
	cout << "圆柱度测量数据:" << maxDiameter << "  " << minDiameter << "    " << lsCylindricityResult[positionNumber][1];
	for (int i = 0; i < diameter.size(); i++)
	{
		cout << "直径" << i + 1 << ":" << diameter[i] << "  ";
	};
	cout << endl;
	*/
	//手动释放
	diameter.clear();
	diameter.shrink_to_fit();
};


//跳动测量相关函数******************************************************************************************************************************************************************************************************************************


void program_29::lsSensorMeasure_roundOutPrepare(int startLocation, int endLocation, int positionNumber, int intervalTime = 10)//使用光幕传感器进行跳动测量;startLocation特征测量开始采样位置的索引；endLocation采样结束时的位置索引
{
	//初始化变量
	/*
	vector<float> upperDistance;
	vector<float> bottomDistance;
	*/
	//移动到位置读取光幕轴到边缘距离
	for (int i = startLocation; i >= endLocation; i--)
	{
		cout << "跳动索引" << i << endl;
		//光幕传感器移动到指定位置
		moveControlPtr->setCurrentAxis(5);
		moveControlPtr->setMoveMode("Trap");
		moveControlPtr->setTrapPrm(moveControlPtr->axisCore[moveControlPtr->currentAxisIndex], moveControlPtr->currentAxisNumber, moveControlPtr->trapAuto[moveControlPtr->currentAxisIndex], lsRoundoutSteplength_axis5[positionNumber][i], moveControlPtr->trapHighVelAuto[moveControlPtr->currentAxisIndex]);
		moveControlPtr->startTrap();
		do
		{
			moveControlPtr->updateAxisStatus(moveControlPtr->currentAxisNumber);
			msleep(200);
		} while (!moveControlPtr->bFlagArrive[moveControlPtr->currentAxisIndex] && moveControlPtr->bFlagMotion[moveControlPtr->currentAxisIndex]);
		roundoutDataPtr[positionNumber]->axisPoint[i][2] = axis5_compensation(moveControlPtr->dEncodePos[4]);
		//7轴旋转
		long axis7_steplength = 360 * 500;//一个脉冲是0.002度;
		moveControlPtr->setCurrentAxis(7);
		moveControlPtr->zeroPosition();
		moveControlPtr->setMoveMode("Trap");
		moveControlPtr->setTrapPrm(moveControlPtr->axisCore[moveControlPtr->currentAxisIndex], moveControlPtr->currentAxisNumber, moveControlPtr->trapAuto[moveControlPtr->currentAxisIndex], axis7_steplength, moveControlPtr->trapHighVelAuto[moveControlPtr->currentAxisIndex]);
		moveControlPtr->startTrap();
		do
		{
			//光幕采样
			ls_devicePtr->getLsMeasurementValue(5);
			switch (i) {
			case 0:
				roundoutDataPtr[positionNumber]->radius0.push_back((ls_devicePtr->lsMeasureResult[0]) / 2);
				roundoutDataPtr[positionNumber]->center0.push_back((ls_devicePtr->lsMeasureResult[1] - ls_devicePtr->lsMeasureResult[2]) / 2);
				break;
			case 1:
				roundoutDataPtr[positionNumber]->radius1.push_back((ls_devicePtr->lsMeasureResult[0]) / 2);
				roundoutDataPtr[positionNumber]->center1.push_back((ls_devicePtr->lsMeasureResult[1] - ls_devicePtr->lsMeasureResult[2]) / 2);
				break;
			case 2:
				roundoutDataPtr[positionNumber]->radius2.push_back((ls_devicePtr->lsMeasureResult[0]) / 2);
				roundoutDataPtr[positionNumber]->center2.push_back((ls_devicePtr->lsMeasureResult[1] - ls_devicePtr->lsMeasureResult[2]) / 2);
				break;
			default:
				break;
			};
			moveControlPtr->updateAxisStatus(moveControlPtr->currentAxisNumber);
			msleep(intervalTime);
		} while (moveControlPtr->bFlagMotion[moveControlPtr->currentAxisIndex]);
	};
	/*
	//将采集到的数据输出到控制台
	for (int i = 0; i < 3; i++)
	{
		cout << "测量位置：" << positionNumber << "    跳动数据：" << i << "z轴："<< roundoutDataPtr[positionNumber]->axisPoint[i][2]<<endl;
		int x;
		if (i == 0)
		{
			x = roundoutDataPtr[positionNumber]->radius0.size();
			cout << "数据大小：" << x << endl;
			for (int j = 0; j < x; j++)
			{
				cout << "半径：" << roundoutDataPtr[positionNumber]->radius0[j] << "中心位置：" << roundoutDataPtr[positionNumber]->center0[j] << endl;;
			}
		}
		else if (i == 1)
		{
				x = roundoutDataPtr[positionNumber]->radius1.size();
				cout << "数据大小：" << x << endl;
				for (int j = 0; j < x; j++)
				{
					cout << "半径：" << roundoutDataPtr[positionNumber]->radius1[j] << "中心位置：" << roundoutDataPtr[positionNumber]->center1[j] << endl;;
				}
		}
		else
		{
				x = roundoutDataPtr[positionNumber]->radius2.size();
				cout << "数据大小：" << x << endl;
				for (int j = 0; j < x;j++)
				{
					cout << "半径：" << roundoutDataPtr[positionNumber]->radius2[j] << "中心位置：" << roundoutDataPtr[positionNumber]->center2[j] << endl;;
				};
		};
	}
	*/
};
void program_29::axisLineCalculate(double pointsData[][3], int pointsNum, double* directionVector, double* aixsReferancePoint)//计算拟合轴线
{
	coder::array<double, 2U> xdata;
	xdata.set_size(1, pointsNum);
	coder::array<double, 2U> ydata;
	ydata.set_size(1, pointsNum);
	coder::array<double, 2U> zdata;
	zdata.set_size(1, pointsNum);
	for (int i = 0; i < pointsNum; i++)
	{
		xdata[i] = pointsData[i][0];
		ydata[i] = pointsData[i][1];
		zdata[i] = pointsData[i][2];
	};
	funFitLine(xdata, ydata, zdata, &aixsReferancePoint[0], &aixsReferancePoint[1], &aixsReferancePoint[2], &directionVector[0], &directionVector[1], &directionVector[2]);
	funFitLine_terminate();
};
void program_29::centerCosFit(int positionNumber, int positionIndex)//拟合对应跳动位置的余弦曲线
{
	coder::array<double, 2U> center;
	int dataLength;
	if (positionIndex == 0)
	{
		dataLength = roundoutDataPtr[positionNumber]->center0.size();
		center.set_size(1, dataLength);
		for (int i = 0; i < dataLength; i++)
		{
			center[i] = roundoutDataPtr[positionNumber]->center0[i];
		}
	}
	else if (positionIndex == 1)
	{
		dataLength = roundoutDataPtr[positionNumber]->center1.size();
		center.set_size(1, dataLength);
		for (int i = 0; i < dataLength; i++)
		{
			center[i] = roundoutDataPtr[positionNumber]->center1[i];
		}
	}
	else
	{
		dataLength = roundoutDataPtr[positionNumber]->center2.size();
		center.set_size(1, dataLength);
		for (int i = 0; i < dataLength; i++)
		{
			center[i] = roundoutDataPtr[positionNumber]->center2[i];
		}
	}
	funFitCos(center, &roundoutDataPtr[positionNumber]->cosFit[positionIndex][0], &roundoutDataPtr[positionNumber]->cosFit[positionIndex][1], &roundoutDataPtr[positionNumber]->cosFit[positionIndex][2]);
	funFitCos_terminate();
};
void program_29::axisLoaction(int positionNumber, int positionIndex, double* directionVector, double* aixsReferancePoint)//计算各跳动位置实际回转轴心
{
	double t = (roundoutDataPtr[positionNumber]->axisPoint[positionIndex][2] - aixsReferancePoint[2]) / directionVector[2];
	roundoutDataPtr[positionNumber]->axisPoint[positionIndex][0] = aixsReferancePoint[0] + t * directionVector[0];
	roundoutDataPtr[positionNumber]->axisPoint[positionIndex][1] = aixsReferancePoint[1] + t * directionVector[1];
	//cout<< roundoutDataPtr[positionNumber]->axisPoint[positionIndex][1]
};
/*
函数作用：对单一截面的数据进行跳动分析计算
参数说明：positionNumber（跳动序号对应的索引）；positionIndex（跳动对应的子截面对应的索引）；calculateMode（跳动测量模式，0按照拟合的方式进行计算，1按照上下顶尖定位作为基准计算）
其他说明：计算结果储存在roundoutDataPtr[positionNumber]->roundoutResult[positionIndex]中
*/
void program_29::roundoutCalculate(int positionNumber, int positionIndex, int calculateMode = 0)
{
	int dataLength;
	//按照拟合的方式进行跳动计算
	if (calculateMode == 0)
	{
		double angle = PI;
		double angleStepLength;
		double x, y, x0, y0;
		double distance;
		x0 = roundoutDataPtr[positionNumber]->cosFit[positionIndex][0] * cos(roundoutDataPtr[positionNumber]->cosFit[positionIndex][1]);
		y0 = roundoutDataPtr[positionNumber]->cosFit[positionIndex][0] * sin(roundoutDataPtr[positionNumber]->cosFit[positionIndex][1]);
		cout << "跳动位置：" << positionNumber << "索引" << positionIndex << "  旋转x：" << roundoutDataPtr[positionNumber]->axisPoint[positionIndex][0] << "旋转y:" << roundoutDataPtr[positionNumber]->axisPoint[positionIndex][1] << endl;
		if (positionIndex == 0)
		{
			dataLength = roundoutDataPtr[positionNumber]->radius0.size();
			angleStepLength = 2 * PI / (dataLength - 1);
			for (int i = 0; i < dataLength; i++)
			{
				x = x0 + (roundoutDataPtr[positionNumber]->radius0[i]) * cos(angle);
				y = y0 + (roundoutDataPtr[positionNumber]->radius0[i]) * sin(angle);
				distance = sqrt(pow(x - roundoutDataPtr[positionNumber]->axisPoint[positionIndex][0], 2) + pow(y - roundoutDataPtr[positionNumber]->axisPoint[positionIndex][1], 2));
				//cout << distance << endl;
				roundoutDataPtr[positionNumber]->distance0.push_back(distance);
				angle += angleStepLength;
			};
			//新代码
			sort(roundoutDataPtr[positionNumber]->distance0.begin(), roundoutDataPtr[positionNumber]->distance0.end());
			cout << "跳动截面数据长度=" << dataLength << endl;
			cout << "最大距离：" << roundoutDataPtr[positionNumber]->distance0[dataLength - 1] << "第5号距离：" << roundoutDataPtr[positionNumber]->distance0[dataLength - 5] << "第10号距离" << roundoutDataPtr[positionNumber]->distance0[dataLength - 10] << endl;
			cout << "最小距离：" << roundoutDataPtr[positionNumber]->distance0[0] << "第5号距离：" << roundoutDataPtr[positionNumber]->distance0[5] << "第10号距离" << roundoutDataPtr[positionNumber]->distance0[10] << endl;

			roundoutDataPtr[positionNumber]->roundoutResult[positionIndex] = roundoutDataPtr[positionNumber]->distance0[dataLength - 13] - roundoutDataPtr[positionNumber]->distance0[12];
			/*
			//原代码
			cout << "最大距离：" << *max_element(roundoutDataPtr[positionNumber]->distance0.begin(), roundoutDataPtr[positionNumber]->distance0.end()) << "  最小距离： " << *min_element(roundoutDataPtr[positionNumber]->distance0.begin(), roundoutDataPtr[positionNumber]->distance0.end()) << endl;
			roundoutDataPtr[positionNumber]->roundoutResult[positionIndex] = *max_element(roundoutDataPtr[positionNumber]->distance0.begin(), roundoutDataPtr[positionNumber]->distance0.end()) - *min_element(roundoutDataPtr[positionNumber]->distance0.begin(), roundoutDataPtr[positionNumber]->distance0.end());
			*/
		}
		else if (positionIndex == 1)
		{
			dataLength = roundoutDataPtr[positionNumber]->radius1.size();
			angleStepLength = 2 * PI / dataLength;
			for (int i = 0; i < dataLength; i++)
			{
				x = x0 + (roundoutDataPtr[positionNumber]->radius1[i]) * cos(angle);
				y = y0 + (roundoutDataPtr[positionNumber]->radius1[i]) * sin(angle);
				distance = sqrt(pow(x - roundoutDataPtr[positionNumber]->axisPoint[positionIndex][0], 2) + pow(y - roundoutDataPtr[positionNumber]->axisPoint[positionIndex][1], 2));
				roundoutDataPtr[positionNumber]->distance1.push_back(distance);
				//cout << "   半径" << roundoutDataPtr[positionNumber]->radius1[i] << "  距离：" << distance << endl;
				angle += angleStepLength;
			}
			//新代码
			sort(roundoutDataPtr[positionNumber]->distance1.begin(), roundoutDataPtr[positionNumber]->distance1.end());
			cout << "跳动截面数据长度=" << dataLength << endl;
			cout << "最大距离：" << roundoutDataPtr[positionNumber]->distance1[dataLength - 1] << "第5号距离：" << roundoutDataPtr[positionNumber]->distance1[dataLength - 5] << "第10号距离" << roundoutDataPtr[positionNumber]->distance1[dataLength - 10] << endl;
			cout << "最小距离：" << roundoutDataPtr[positionNumber]->distance1[0] << "第5号距离：" << roundoutDataPtr[positionNumber]->distance1[5] << "第10号距离" << roundoutDataPtr[positionNumber]->distance1[10] << endl;
			roundoutDataPtr[positionNumber]->roundoutResult[positionIndex] = roundoutDataPtr[positionNumber]->distance1[dataLength - 13] - roundoutDataPtr[positionNumber]->distance1[12];
			/*
			//源代码
			cout << "最大距离：" << *max_element(roundoutDataPtr[positionNumber]->distance1.begin(), roundoutDataPtr[positionNumber]->distance1.end()) << "  最小距离： " << *min_element(roundoutDataPtr[positionNumber]->distance1.begin(), roundoutDataPtr[positionNumber]->distance1.end()) << endl;
			roundoutDataPtr[positionNumber]->roundoutResult[positionIndex] = *max_element(roundoutDataPtr[positionNumber]->distance1.begin(), roundoutDataPtr[positionNumber]->distance1.end()) - *min_element(roundoutDataPtr[positionNumber]->distance1.begin(), roundoutDataPtr[positionNumber]->distance1.end());
			*/

		}
		else
		{
			dataLength = roundoutDataPtr[positionNumber]->radius2.size();
			angleStepLength = 2 * PI / dataLength;
			for (int i = 0; i < dataLength; i++)
			{
				x = x0 + (roundoutDataPtr[positionNumber]->radius2[i]) * cos(angle);
				y = y0 + (roundoutDataPtr[positionNumber]->radius2[i]) * sin(angle);
				distance = sqrt(pow(x - roundoutDataPtr[positionNumber]->axisPoint[positionIndex][0], 2) + pow(y - roundoutDataPtr[positionNumber]->axisPoint[positionIndex][1], 2));
				roundoutDataPtr[positionNumber]->distance2.push_back(distance);
				//out << "   半径" << roundoutDataPtr[positionNumber]->radius1[i] << "  距离：" << distance << endl;
				angle += angleStepLength;

			}
			//新代码
			sort(roundoutDataPtr[positionNumber]->distance2.begin(), roundoutDataPtr[positionNumber]->distance2.end());
			cout << "跳动截面数据长度=" << dataLength << endl;
			cout << "最大距离：" << roundoutDataPtr[positionNumber]->distance2[dataLength - 1] << "第5号距离：" << roundoutDataPtr[positionNumber]->distance2[dataLength - 5] << "第10号距离" << roundoutDataPtr[positionNumber]->distance2[dataLength - 10] << endl;
			cout << "最小距离：" << roundoutDataPtr[positionNumber]->distance2[0] << "第5号距离：" << roundoutDataPtr[positionNumber]->distance2[5] << "第10号距离" << roundoutDataPtr[positionNumber]->distance2[10] << endl;
			roundoutDataPtr[positionNumber]->roundoutResult[positionIndex] = roundoutDataPtr[positionNumber]->distance2[dataLength - 13] - roundoutDataPtr[positionNumber]->distance2[12];

			/*
			//源代码
			cout << "最大距离：" << *max_element(roundoutDataPtr[positionNumber]->distance2.begin(), roundoutDataPtr[positionNumber]->distance2.end()) << "  最小距离： " << *min_element(roundoutDataPtr[positionNumber]->distance2.begin(), roundoutDataPtr[positionNumber]->distance2.end()) << endl;
			roundoutDataPtr[positionNumber]->roundoutResult[positionIndex] = *max_element(roundoutDataPtr[positionNumber]->distance2.begin(), roundoutDataPtr[positionNumber]->distance2.end()) - *min_element(roundoutDataPtr[positionNumber]->distance2.begin(), roundoutDataPtr[positionNumber]->distance2.end());
			*/
		};
	}
	//按照上下顶尖为基准进行跳动计算
	else
	{
		vector<float> upperDistance;
		vector<float> bottomDistance;
		//计算光幕上下边缘的原始读数
		if (positionIndex == 0)
		{
			dataLength = roundoutDataPtr[positionNumber]->radius0.size();
			for (int i = 0; i < dataLength; i++)
			{
				upperDistance.push_back(roundoutDataPtr[positionNumber]->center0[i] + roundoutDataPtr[positionNumber]->radius0[i]);
				bottomDistance.push_back(roundoutDataPtr[positionNumber]->center0[i] - roundoutDataPtr[positionNumber]->radius0[i]);
			};
		}
		else if (positionIndex == 1)
		{
			dataLength = roundoutDataPtr[positionNumber]->radius1.size();
			for (int i = 0; i < dataLength; i++)
			{
				upperDistance.push_back(roundoutDataPtr[positionNumber]->center1[i] + roundoutDataPtr[positionNumber]->radius1[i]);
				bottomDistance.push_back(roundoutDataPtr[positionNumber]->center1[i] - roundoutDataPtr[positionNumber]->radius1[i]);
			};
		}
		else
		{
			dataLength = roundoutDataPtr[positionNumber]->radius2.size();
			for (int i = 0; i < dataLength; i++)
			{
				upperDistance.push_back(roundoutDataPtr[positionNumber]->center2[i] + roundoutDataPtr[positionNumber]->radius2[i]);
				bottomDistance.push_back(roundoutDataPtr[positionNumber]->center2[i] - roundoutDataPtr[positionNumber]->radius2[i]);
			};
		}
		/*
		//对上述原始数据使用3西格玛准则进行滤波
		double mean_upper = (accumulate(upperDistance.begin(), upperDistance.end(), 0) / dataLength);
		double mean_bottom = (accumulate(bottomDistance.begin(), bottomDistance.end(), 0) / dataLength);
		double variance_upper = 0;
		double variance_bottom = 0;
		for (int i = 0; i < dataLength; i++)
		{
			variance_upper = variance_upper + pow(upperDistance[i] - mean_upper, 2);
			variance_bottom = variance_bottom + pow(bottomDistance[i] - mean_bottom, 2);
			//cout << i + 1 << "  上边缘距离:" << upperDistance[i] << "  下边缘距离:" << bottomDistance[i] << endl;
		};
		variance_upper = variance_upper / dataLength;
		variance_bottom = variance_bottom / dataLength;
		double standardDeviation_upper = sqrt(variance_upper);
		double standardDeviation_bottom = sqrt(variance_bottom);
		double lowerBound_upper = mean_upper - 3 * standardDeviation_upper;
		double upperBound_upper = mean_upper + 3 * standardDeviation_upper;
		double lowerBound_bottom = mean_bottom - 3 * standardDeviation_bottom;
		double upperBound_bottom = mean_bottom + 3 * standardDeviation_bottom;
		//cout << "均值" << mean_upper << " 方差" << variance_upper << " 标准差" << standardDeviation_upper << " 上限" << upperBound_upper << " 下限" << lowerBound_upper << endl;
		//cout << "均值" << mean_bottom << " 方差" << variance_bottom << " 标准差" << standardDeviation_bottom << " 下限" << lowerBound_bottom << " 上限" << upperBound_bottom << endl;
		vector<float>::iterator it;//剔除超范围数据
		for (it = upperDistance.begin(); it != upperDistance.end();)
		{
			if (*it >= upperBound_upper || *it <= lowerBound_upper)
			{
				it = upperDistance.erase(it);
			}
			else
			{
				++it;
			}
		};
		for (it = bottomDistance.begin(); it != bottomDistance.end();)
		{
			if (*it >= upperBound_bottom || *it <= lowerBound_bottom)
			{
				it = bottomDistance.erase(it);
			}
			else
			{
				++it;
			}
		};
		//跳动数据计算
		float maxUpperDistance, minUpperDistance, upperRoundout, maxBottomDistance, minBottomDistance, bottomRoundout;
		maxUpperDistance = *max_element(upperDistance.begin(), upperDistance.end());
		minUpperDistance = *min_element(upperDistance.begin(), upperDistance.end());
		maxBottomDistance = *max_element(bottomDistance.begin(), bottomDistance.end());
		minBottomDistance = *min_element(bottomDistance.begin(), bottomDistance.end());
		upperRoundout = maxUpperDistance - minUpperDistance;
		bottomRoundout = maxBottomDistance - minBottomDistance;
		roundoutDataPtr[positionNumber]->roundoutResult[positionIndex] = max(upperRoundout, bottomRoundout);
		*/
		//新代码
		float upperRoundout, bottomRoundout;
		sort(upperDistance.begin(), upperDistance.end());
		sort(bottomDistance.begin(), bottomDistance.end());
		upperRoundout = upperDistance[dataLength - 10] - upperDistance[5];
		bottomRoundout = bottomDistance[dataLength - 10] - bottomDistance[5];
		roundoutDataPtr[positionNumber]->roundoutResult[positionIndex] = max(upperRoundout, bottomRoundout);
		//手动释放动态数组
		upperDistance.clear();
		upperDistance.shrink_to_fit();
		bottomDistance.clear();
		bottomDistance.shrink_to_fit();
	}
	cout << "跳动结果" << roundoutDataPtr[positionNumber]->roundoutResult[positionIndex] << endl;
};

void program_29::clearRoundoutData()
{
	for (int i = 0; i < 0; i++)//需要将循环体的i变量修改为跳动采集的轴段总数
	{
		roundoutDataPtr[i]->radius0.clear();
		roundoutDataPtr[i]->radius0.shrink_to_fit();
		roundoutDataPtr[i]->radius1.clear();
		roundoutDataPtr[i]->radius1.shrink_to_fit();
		roundoutDataPtr[i]->radius2.clear();
		roundoutDataPtr[i]->radius2.shrink_to_fit();
		roundoutDataPtr[i]->center0.clear();
		roundoutDataPtr[i]->center0.shrink_to_fit();
		roundoutDataPtr[i]->center1.clear();
		roundoutDataPtr[i]->center1.shrink_to_fit();
		roundoutDataPtr[i]->center2.clear();
		roundoutDataPtr[i]->center2.shrink_to_fit();
		fill(&roundoutDataPtr[i]->axisPoint[0][0], &roundoutDataPtr[i]->axisPoint[0][0] + sizeof(roundoutDataPtr[i]->axisPoint) / sizeof(double), 0);
		fill(&roundoutDataPtr[i]->cosFit[0][0], &roundoutDataPtr[i]->cosFit[0][0] + sizeof(roundoutDataPtr[i]->cosFit) / sizeof(double), 0);
		fill(&roundoutDataPtr[i]->roundoutResult[0], &roundoutDataPtr[i]->roundoutResult[0] + sizeof(roundoutDataPtr[i]->roundoutResult) / sizeof(double), 0);
	};
};


//***********************************************************************************************************************************************************************************************************************************************
string program_29::cam0_Measure_prepare(int positionNumber)
{
	moveControlPtr->setCurrentAxis(5);
	moveControlPtr->setMoveMode("Trap");
	moveControlPtr->setTrapPrm(moveControlPtr->axisCore[moveControlPtr->currentAxisIndex], moveControlPtr->currentAxisNumber, moveControlPtr->trapAuto[moveControlPtr->currentAxisIndex], cam0_StepLength_axis5[positionNumber]+ stepLengthCompensation_axis5, moveControlPtr->trapHighVelAuto[moveControlPtr->currentAxisIndex]);
	moveControlPtr->startTrap();
	do
	{
		moveControlPtr->updateAxisStatus(moveControlPtr->currentAxisNumber);
		msleep(200);
	} while (!moveControlPtr->bFlagArrive[moveControlPtr->currentAxisIndex] && moveControlPtr->bFlagMotion[moveControlPtr->currentAxisIndex]);
	camPtrList[0]->startCapture();
	string imgPath = dataSavePath + "\\cam0_" + to_string(positionNumber + 1) + ".bmp";
	msleep(2000);
	moveControlPtr->updateAxisStatus(moveControlPtr->currentAxisNumber);
	camPtrList[0]->stopCapture();
	cam0_arriveOrgEncode_axis5[positionNumber] = moveControlPtr->dEncodePos[4];//将远心相机拍摄点位的实际编码器位置保存下来
	emit imgInf(originalImgPtr, "org",0);
	camPtrList[0]->saveImg(imgPath,0,0,0);
	cout << " cam0_Measure_prepare点位 " << positionNumber + 1 << " 执行完毕 " << endl;
	replace(imgPath.begin(), imgPath.end(), '\\', '/');
	return imgPath;
};
void program_29::cam1_Measure_prepare(int positionNumber)
{
	cam1PathList.clear();
	cam1PathList.shrink_to_fit();
	//trap5轴
	moveControlPtr->setCurrentAxis(5);
	moveControlPtr->setMoveMode("Trap");
	moveControlPtr->setTrapPrm(moveControlPtr->axisCore[moveControlPtr->currentAxisIndex], moveControlPtr->currentAxisNumber, moveControlPtr->trapAuto[moveControlPtr->currentAxisIndex], cam1_prepare[positionNumber][0]+ stepLengthCompensation_axis5, moveControlPtr->trapHighVelAuto[moveControlPtr->currentAxisIndex]);
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
	emit programTips("相机已经到达采集位置，请安装机心夹并确保零件对正！",4);
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
		axis7_steplength = (i + 1) * axis7_stepIncrease;
		moveControlPtr->setTrapPrm(moveControlPtr->axisCore[moveControlPtr->currentAxisIndex], moveControlPtr->currentAxisNumber, moveControlPtr->trapAuto[moveControlPtr->currentAxisIndex], axis7_steplength, moveControlPtr->trapHighVelAuto[moveControlPtr->currentAxisIndex]);
		moveControlPtr->startTrap();
		do
		{
			moveControlPtr->updateAxisStatus(moveControlPtr->currentAxisNumber);
			msleep(200);
		} while (moveControlPtr->bFlagMotion[moveControlPtr->currentAxisIndex]);
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
	moveControlPtr->updateAxisStatus(moveControlPtr->currentAxisNumber);
	cam1_axis5Place[positionNumber] = axis5_compensation(moveControlPtr->dEncodePos[4]);
	cout << " cam1_Measure_prepare准备 " << positionNumber + 1 << " 执行完毕 " << "光幕位置" << moveControlPtr->dEncodePos[4]<<endl;
};
void program_29::cam2_Measure_prepare(int positionNumber)
{
	string roughnessFolder;//新建粗糙度文件夹命名为序号
	roughnessFolder = dataSavePath + "\\" + to_string(positionNumber + 1);
	string command;
	command = "mkdir -p " + roughnessFolder;
	system(command.c_str());
	moveControlPtr->setCurrentAxis(5);//移动5轴
	moveControlPtr->setMoveMode("Trap");
	moveControlPtr->setTrapPrm(moveControlPtr->axisCore[moveControlPtr->currentAxisIndex], moveControlPtr->currentAxisNumber, moveControlPtr->trapAuto[moveControlPtr->currentAxisIndex], cam2_parpare[positionNumber][0]+ stepLengthCompensation_axis5, moveControlPtr->trapHighVelAuto[moveControlPtr->currentAxisIndex]);
	moveControlPtr->startTrap();
	do
	{
		moveControlPtr->updateAxisStatus(moveControlPtr->currentAxisNumber);
		msleep(200);
	} while (!moveControlPtr->bFlagArrive[moveControlPtr->currentAxisIndex] && moveControlPtr->bFlagMotion[moveControlPtr->currentAxisIndex]);
	moveControlPtr->setCurrentAxis(1);//1轴第一次trap移动(预先移动)
	moveControlPtr->setMoveMode("Trap");
	long axis1_steplength = cam2_parpare[positionNumber][1] + 5000 * (cam2_radius[positionNumber][1] - cam2_radius[positionNumber][0]) - 50;//5个脉冲为1微米
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
		for (int i = 0; i < 5; i++)
		{
			axis1_steplength = cam2_parpare[positionNumber][1] + 5000 * (cam2_radius[positionNumber][1] - cam2_radius[positionNumber][0]) - 45 * (i - 1) - 10;
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

void program_29::saveAsExcel()//将主界面上的表格保存为excel文件
{
	string excelPath = "D:\\temporary_sun\\AxisMeasurement\\x64\\release\\measureData\\Report\\"  + stime + "_" + partNub.toStdString() + ".xlsx";

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
bool program_29::mergeCells(QString start, QString end, QString value)
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

void program_29::creatDatabaseTable()
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
void program_29::writeToDatabase()
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
void program_29::zeroMeasureNub()
{
	allFeatureFlag = "--";
	ngFeatureNum = 0;//NG特征数量
	measurePartsNum_all = 0;//该型号轴测量总数
	measurePartsNum_ok = 0;//该型号轴合格数量
	measurePartsNum_ng = 0;//该型号轴NG数量
	yield = 0;//该型号轴合格率
	emit measureStatistics(allFeatureFlag, ngFeatureNum, measurePartsNum_all, yield);
};
void program_29::pushback_vectors(vector<double>& fResult, int fType, double fIndex, double fNominalsize, double fLowerTolerance, double fUpperTolerance)
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
		item[0]->setData(Qt::DisplayRole,fIndex);//第一列特征编号
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
			if ((fLowerSize - minResult) / tolerance > 0.1 || (minResult - fUpperSize) / tolerance > 0.1)
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
			if ((fLowerSize - maxResult) / tolerance > 0.1 || (maxResult - fUpperSize) / tolerance > 0.1)
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



//远心相机图像检测算法
void program_29::cam0Picture_algorithm()//远心相机1图像算法
{
	

	//以下部分贴入远心相机的图像处理代码
	//需要将原程序的图像地址按照下面格式修改
	/*
	//参考地址修改方式
	string Img0path = imgSavePath[0];
	string Img1path = imgSavePath[1];
	string Img2path = imgSavePath[2];
	string Img3path = imgSavePath[3];
	*/

	//需要3.9 需要将远心相机的图像补偿距离复制到以下
	//需要从以下复制
	//大远心相机采集图像路径
	/*大远心相机采集图像路径--调用示例
	string Img1path = "C:/Users/123/Desktop/XD/0.bmp";//需要
	*/
	string Img1path = "D:\\temporary_sun\\AxisMeasurement\\programParmeter\\WZ10442111-90\\cam0_1.bmp";


	//孔径相机采集图像路径
	/*孔径相机采集图像路径--调用示例
	string Imgh1path = "C:/Users/123/Desktop/image/H0.bmp";
	*/
//	string Imgh1path = "C:/Users/123/Desktop/XD/12.7//H0.bmp";

	//预处理后的大远心相机采集图像1、2：Img1Aug、Img2Aug
	/*大远心相机采集图像预处理--调用示例*/
	HObject Img1Aug = imgAug(Img1path);
	HTuple halconPath = Img1path.c_str();
	HObject Img1_raw;
	ReadImage(&Img1_raw, halconPath);
	

	//可能需要，增强对比度 imgAug为增强后，Img3_raw 为原图
	//HObject Img1Aug = imgAug(Img1path);
	//HTuple halconPath = Img1path.c_str();
	//HObject Img1_raw;
	//ReadImage(&Img1_raw, halconPath);//未进行预处理的大远心相机采集图像1：Img1_raw


	int fault_detect = 0;
	fault_detect = 1;//图像1处理函数开始

	double cam0_1_Chamfer_3 = TLineFP_Chamfer_P(Img1_raw,
		2205, 395, 2263, 434,
		2, 20, 60,
		true, 5, 4, 2,
		20, -0.5, 0.5,
		90, 1,
		-1, 0, 0, 3, 2);
	resultVectorList.push_back(cam0_1_Chamfer_3 * Calik);
	pushback_vectors(resultVectorList, 2, 3, 1, 0, 0.2);


	


	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////特征集中处理完毕////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	/*1、轴向距离测量输出示例
	double AxialD_45dudaojiao = ((cam0_1_175left4_TLineFP[3] - cam0_1_175left4_TLineFP[1])+ (cam0_1_175left4_TLineFP[0] - cam0_1_175left4_TLineFP[2]))/2;
	resultVectorList.push_back(AxialD_45dudaojiao* Calik);
	pushback_vectors(resultVectorList, 4, 176, 1.5, 0, +0.3);
	*/


	/*
	2、轴向距离测量输出示例（跨越图像计算示例）
	double AxialD_84_XD = cam0_1_84_XD2[3] - cam0_1_84_XD[3];
	resultVectorList.push_back(AxialD_84_XD * Calik + pic1To2_moveDistance );
	pushback_vectors(resultVectorList, 4, 84, 114, -0.5, +0.5);
	*/

	/*
	3、倒圆角计算输出示例
	double Chamfer_73 = TLineFP_Chamfer_P(Img1_raw,
		cam0_1_73daojiao_TLineFP[0] - 500, cam0_1_73daojiao_TLineFP[1] - 900, cam0_1_73daojiao_TLineFP[0] + 60, cam0_1_73daojiao_TLineFP[1] - 300,
		2, 20, 60,
		true, 5, 4, 2,
		20, -0.5, 0.5,
		90, 1,
		-1, 0, 0, 3, 2);
	resultVectorList.push_back(Chamfer_73  * Calik);
	pushback_vectors(resultVectorList, 2, 6, 1.75, -0.2, 0);
	*/

	/*
	4、角度计算示例
	double AxialD_45dudaojiao_1 = atan((cam0_1_45dudaojiao_TLineFP[0] - cam0_1_45dudaojiao_TLineFP[2]) / (cam0_1_45dudaojiao_TLineFP[3] - cam0_1_45dudaojiao_TLineFP[1])) * 180 / PI;
	resultVectorList.push_back(AxialD_45dudaojiao_1);
	pushback_vectors(resultVectorList, 3, 176, 45, -5, +5);
	*/


	/*
	5、同时带尺寸和角度的计算示例
	double AxialD_45dudaojiao = ((cam0_1_45dudaojiao_TLineFP[3] - cam0_1_45dudaojiao_TLineFP[1])+ (cam0_1_45dudaojiao_TLineFP[0] - cam0_1_45dudaojiao_TLineFP[2]))/2;
	resultVectorList.push_back(AxialD_45dudaojiao* Calik);
	pushback_vectors(resultVectorList, 4, 176, 1.5, -0, +0.3);

	double AxialD_45dudaojiao_1 = atan((cam0_1_45dudaojiao_TLineFP[0] - cam0_1_45dudaojiao_TLineFP[2]) / (cam0_1_45dudaojiao_TLineFP[3] - cam0_1_45dudaojiao_TLineFP[1])) * 180 / PI;
	resultVectorList.push_back(AxialD_45dudaojiao_1);
	pushback_vectors(resultVectorList, 3, 176.1, 45, -5, +5);
	*/

	/*
	6、孔径计算示例（要调整cam1Picture2_algorithm函数体里的参数，使与Halcon算子一致）
	cam1Picture2_algorithm(Imgh1path);
	*/

	//需要，复制以上
};


double program_29::cam0Picture1_algorithmLuowenDown(HObject ho_Image,  HObject ho_Image_L,double L, float OUT2, int m, double Pix_zhoujing_chuanru)//测螺纹
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
	HTuple  hv_Nr_new, hv_Nc_new, hv_Dist_new;

	//读取模板和图像进行模板匹配*
	GetImageSize(ho_Image, &hv_Width, &hv_Height);
	hv_ModelFile = "./programParmeter/WZ10-11-2006-430/cam0_1-430-luowenDown.sbm";
	ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
	//创建测量模型
	CreateMetrologyModel(&hv_MetrologyHandle);
	//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
	//求P下方

	hv_Line1.Clear();
	hv_Line1.Append(510 - 340);
	hv_Line1.Append(-10);
	hv_Line1.Append(510 - 370);
	hv_Line1.Append(40);
	hv_Line2.Clear();
	hv_Line2.Append(510 - 465);
	hv_Line2.Append(-10);
	hv_Line2.Append(510 - 500);
	hv_Line2.Append(40);
	hv_Line3.Clear();
	hv_Line3.Append(510 - 605);
	hv_Line3.Append(-10);
	hv_Line3.Append(510 - 635);
	hv_Line3.Append(40);
	//求P上方
	hv_Line4.Clear();
	hv_Line4.Append(518 - 430);
	hv_Line4.Append(-10);
	hv_Line4.Append(518 - 395);
	hv_Line4.Append(40);
	hv_Line5.Clear();
	hv_Line5.Append(518 - 560);
	hv_Line5.Append(-10);
	hv_Line5.Append(518 - 530);
	hv_Line5.Append(40);
	//求大径
	hv_Line6.Clear();
	hv_Line6.Append(518 - 370);
	hv_Line6.Append(50);
	hv_Line6.Append(518 - 425);
	hv_Line6.Append(50);
	hv_Line7.Clear();
	hv_Line7.Append(518 - 490);
	hv_Line7.Append(50);
	hv_Line7.Append(518 - 550);
	hv_Line7.Append(50);
	hv_Line8.Clear();
	hv_Line8.Append(500 - 605);
	hv_Line8.Append(50);
	hv_Line8.Append(500 - 665);
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
	double x00 = -(L / 2) / Calik + Pix_zhoujing;
	//cout << "中线位置:" << x00 << endl;
	//cout << "光幕:" << L<< endl;
	//screwX00.push_back(x00);




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
	double D = (Pix_luowen - x00) * Calik * 2;
	//double D = 42;
	cout << "D:" << D << endl;




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
		C[i] = (D - 2 * c * i * Calik) / 2;//计算分段后，从大径向内侧的每一段的轴径
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
	//screwZhongjingCOl.push_back(Pix_d);//
	/*****************************/
	//传入基准A的拟合直线
	double hv_Box[3];
	double end_col_before = cam0Picture1_L_K_b_Down(ho_Image_L, hv_Box);
	double axisA_k = -(hv_Box[1] / hv_Box[0]);
	//cout << " axisA_k=" << atan(axisA_k)*180/3.1415926  << endl;
	cout << " axisA_k=" << axisA_k << endl;
	double axisA_b = hv_Box[2];
	cout << " 直线" << " y=" << axisA_k << " x +" << axisA_b << endl;
	//-----------------计算中径--------------//
	//中径实际尺寸d =大径实际尺寸-（螺纹大径的x轴像素格数-中径所在像素格数）*像素尺寸

	//double AdL = (abs(hv_Box[1] *(Pix_d+A[4]/Calik/tan(a/2))- hv_Box[0] *Y_zhongjing +hv_Box[1]* Pianyi_b+5120* hv_Box[0]+hv_Box[2]) /(sqrt(hv_Box[1] * hv_Box[1] + hv_Box[0]* hv_Box[0])))* Calik;
	double AdL = (abs(-(hv_Box[1]) * (Pix_d + A[4] / Calik / tan(a / 2)) + Y_zhongjing * hv_Box[0] - 5120 * hv_Box[0] + axisA_b * hv_Box[0]) / (sqrt(hv_Box[1] * hv_Box[1] + hv_Box[0] * hv_Box[0]))) * Calik;
	//double AdL = (abs(hv_Box[1] * Pix_d  - hv_Box[0] * Y_zhongjing - hv_Box[2]) / (sqrt(hv_Box[1] * hv_Box[1] + hv_Box[0] * hv_Box[0]))) * Calik;
	cout << "AdL=" << AdL << endl;
	double d = -2 * AdL + L;
	//double d = (Pix_d - x00) * Calik * 2;
	cout << "中径d:" << d << endl;

	//返回中径
	return d;
}

double program_29::cam0Picture1_algorithmLuowenUp(HObject ho_Image, HObject ho_Image_L, double L, float OUT2, int m, double Pix_zhoujing_chuanru)//测螺纹
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
	HTuple  hv_Nr_new, hv_Nc_new, hv_Dist_new;

	//读取模板和图像进行模板匹配*
	GetImageSize(ho_Image, &hv_Width, &hv_Height);
	hv_ModelFile = "./programParmeter/WZ10-11-2006-430/cam0_1-430-luowenUp.sbm";
	ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
	//创建测量模型
	CreateMetrologyModel(&hv_MetrologyHandle);
	//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
	//求P下方

	hv_Line1.Clear();
	hv_Line1.Append(510 - 340);
	hv_Line1.Append(-10);
	hv_Line1.Append(510 - 370);
	hv_Line1.Append(40);
	hv_Line2.Clear();
	hv_Line2.Append(510 - 465);
	hv_Line2.Append(-10);
	hv_Line2.Append(510 - 500);
	hv_Line2.Append(40);
	hv_Line3.Clear();
	hv_Line3.Append(510 - 560);
	hv_Line3.Append(-10);
	hv_Line3.Append(510 - 530);
	hv_Line3.Append(40);
	//求P上方
	hv_Line4.Clear();
	hv_Line4.Append(518 - 430);
	hv_Line4.Append(-10);
	hv_Line4.Append(518 - 395);
	hv_Line4.Append(40);
	hv_Line5.Clear();
	hv_Line5.Append(518 - 560);
	hv_Line5.Append(-10);
	hv_Line5.Append(518 - 530);
	hv_Line5.Append(40);
	//求大径
	hv_Line6.Clear();
	hv_Line6.Append(518 - 370);
	hv_Line6.Append(50);
	hv_Line6.Append(518 - 425);
	hv_Line6.Append(50);
	hv_Line7.Clear();
	hv_Line7.Append(518 - 490);
	hv_Line7.Append(50);
	hv_Line7.Append(518 - 550);
	hv_Line7.Append(50);
	hv_Line8.Clear();
	hv_Line8.Append(500 - 585);
	hv_Line8.Append(50);
	hv_Line8.Append(500 - 645);
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
	double x00 = -(L / 2) / Calik + Pix_zhoujing;
	//cout << "中线位置:" << x00 << endl;
	//cout << "光幕:" << L<< endl;
	//screwX00.push_back(x00);




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
	double D = (Pix_luowen - x00) * Calik * 2;
	//double D = 42;
	cout << "D:" << D << endl;




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
		C[i] = (D - 2 * c * i * Calik) / 2;//计算分段后，从大径向内侧的每一段的轴径
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
	//screwZhongjingCOl.push_back(Pix_d);//
	/*****************************/
	//传入基准A的拟合直线
	double hv_Box[3];
	double end_col_before = cam0Picture1_L_K_b_Up(ho_Image_L, hv_Box);
	double axisA_k = -(hv_Box[1] / hv_Box[0]);
	//cout << " axisA_k=" << atan(axisA_k)*180/3.1415926  << endl;
	cout << " axisA_k=" << axisA_k << endl;
	double axisA_b = hv_Box[2];
	cout << " 直线" << " y=" << axisA_k << " x +" << axisA_b << endl;
	//-----------------计算中径--------------//
	//中径实际尺寸d =大径实际尺寸-（螺纹大径的x轴像素格数-中径所在像素格数）*像素尺寸

	//double AdL = (abs(hv_Box[1] *(Pix_d+A[4]/Calik/tan(a/2))- hv_Box[0] *Y_zhongjing +hv_Box[1]* Pianyi_b+5120* hv_Box[0]+hv_Box[2]) /(sqrt(hv_Box[1] * hv_Box[1] + hv_Box[0]* hv_Box[0])))* Calik;
	double AdL = (abs(-(hv_Box[1]) * (Pix_d + A[4] / Calik / tan(a / 2)) + Y_zhongjing * hv_Box[0] - 5120 * hv_Box[0] + axisA_b * hv_Box[0]) / (sqrt(hv_Box[1] * hv_Box[1] + hv_Box[0] * hv_Box[0]))) * Calik;
	//double AdL = (abs(hv_Box[1] * Pix_d  - hv_Box[0] * Y_zhongjing - hv_Box[2]) / (sqrt(hv_Box[1] * hv_Box[1] + hv_Box[0] * hv_Box[0]))) * Calik;
	cout << "AdL=" << AdL << endl;
	double d = -2 * AdL + L;
	//double d = (Pix_d - x00) * Calik * 2;
	cout << "中径d:" << d << endl;

	//返回中径
	return d;
}

double program_29::cam0Picture1_L_K_b_Down(HObject ho_Image, double* hv_Box)
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
	hv_ModelFile = "./programParmeter/WZ10-11-2006-430/cam0_1-430-5.sbm";
	ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
	//创建测量模型	
	CreateMetrologyModel(&hv_MetrologyHandle);
	//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
	hv_Line.Clear();
	hv_Line[0] = 380;
	hv_Line[1] = 50;
	hv_Line[2] = 550;
	hv_Line[3] = 50;
	AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 70, HTuple(),
		HTuple(), &hv_LineIndices);
	SetMetrologyObjectParam(hv_MetrologyHandle, "all", "measure_distance", 0.3);

	//获取读取的模板的轮廓，区域坐标等信息
	GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
	GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
	GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
		&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
	//进行模板匹配
	cout << "FindShapeModel:READY" << endl;
	FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.5,
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
	if (-0.00000001 < hv_Nr_new.D() && hv_Nr_new.D() <= 0) {
		hv_Box[0] = -0.00000001;
	}
	else if (0 < hv_Nr_new.D() && hv_Nr_new.D() < 0.00000001) {
		hv_Box[0] = 0.00000001;
	}
	else {
		hv_Box[0] = hv_Nr_new.D();
	}
	cout << " hv_Box[0]: " << hv_Box[0] << endl;
	hv_Box[1] = hv_Nc_new.D();
	cout << " hv_Box[1]: " << hv_Box[1] << endl;
	double new_Dist = (hv_ColEnd.D() * hv_Nc_new.D() + hv_RowEnd.D() * hv_Nr_new.D()) / hv_Box[0];
	hv_Box[2] = new_Dist;
	cout << "hv_ColEnd[0].D() X: " << hv_ColEnd[0].D() << endl;
	cout << "hv_RowEnd[0].D() Y: " << hv_RowEnd[0].D() << endl;
	return hv_ColEnd[0].D();
	//输出模板匹配后检测直线两端点的位置，即特征点位置。

}
double program_29::cam0Picture1_L_K_b_Up(HObject ho_Image, double* hv_Box)
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
	hv_ModelFile = "./programParmeter/WZ10-11-2006-430/cam0_4-430-131.sbm";
	ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
	//创建测量模型	
	CreateMetrologyModel(&hv_MetrologyHandle);
	//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
	hv_Line.Clear();
	hv_Line[0] = -175;
	hv_Line[1] = 50;
	hv_Line[2] = -55;
	hv_Line[3] = 50;
	AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50, HTuple(),
		HTuple(), &hv_LineIndices);
	SetMetrologyObjectParam(hv_MetrologyHandle, "all", "measure_distance", 0.3);

	//获取读取的模板的轮廓，区域坐标等信息
	GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
	GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
	GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
		&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
	//进行模板匹配
	cout << "FindShapeModel:READY" << endl;
	FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.5,
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
	if (-0.00000001 < hv_Nr_new.D() && hv_Nr_new.D() <= 0) {
		hv_Box[0] = -0.00000001;
	}
	else if (0 < hv_Nr_new.D() && hv_Nr_new.D() < 0.00000001) {
		hv_Box[0] = 0.00000001;
	}
	else {
		hv_Box[0] = hv_Nr_new.D();
	}
	cout << " hv_Box[0]: " << hv_Box[0] << endl;
	hv_Box[1] = hv_Nc_new.D();
	cout << " hv_Box[1]: " << hv_Box[1] << endl;
	double new_Dist = (hv_ColEnd.D() * hv_Nc_new.D() + hv_RowEnd.D() * hv_Nr_new.D()) / hv_Box[0];
	hv_Box[2] = new_Dist;
	cout << "hv_ColEnd[0].D() X: " << hv_ColEnd[0].D() << endl;
	cout << "hv_RowEnd[0].D() Y: " << hv_RowEnd[0].D() << endl;
	return hv_ColEnd[0].D();
	//输出模板匹配后检测直线两端点的位置，即特征点位置。

}

HObject program_29::imgAug(string imgPath) {
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
//模板匹配找特征点
void program_29::TemplateMatching_TLineFP_P(double* TLineFP_result, HObject ho_Image, HTuple hv_ModelFile, int MatchingLine1, int MatchingLine2, int MatchingLine3, int MatchingLine4,
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

//根据起始点与终止点求倒角
double program_29::TLineFP_Chamfer_P(HObject ho_Image, double GenRect1, double GenRect2, double GenRect3, double GenRect4,
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

//根据起始点与终止点求倒角2 添加TupleMax(hv_Radius, &hv_Max); 选取最大值函数
double program_29::TLineFP_Chamfer_H2(HObject ho_Image, double GenRect1, double GenRect2, double GenRect3, double GenRect4,
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
	HTuple  hv_Max;
	// Local control variables
	HTuple  hv_Width, hv_Height, hv_Row, hv_Column;
	HTuple  hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;
	HObject ho_GammaImage, ho_ImageEmphasize, ho_ImageGauss;
	GetImageSize(ho_Image, &hv_Width, &hv_Height);
	GammaImage(ho_Image, &ho_GammaImage, 0.616667, 0.055, 0.018, 255, "true");
	Emphasize(ho_GammaImage, &ho_ImageEmphasize, hv_Width, hv_Height, 1.9);
	GaussFilter(ho_ImageEmphasize, &ho_ImageGauss, 7);
	//dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
	//
	//Segment a region containing the edges
	//基于全局阈值的图像快速阈值化
	//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin与colend可固定不变
	//GenRectangle1(&ho_Rectangle, cam0_4_220_72shangbian_TLineFP[2], 500, cam0_4_220_72youbiann_TLineFP[0] - 20, 3500);
	GenRectangle1(&ho_Rectangle, GenRect1, GenRect2, GenRect3, GenRect4);
	//提取roi区域，roi以外的变黑，不改变整个图像的尺寸大小。corp_domain是只提取roi区域
	ReduceDomain(ho_ImageGauss, ho_Rectangle, &ho_ImageReduced);

	//In the subdomain of the image containing the edges,
	//extract subpixel precise edges.
	//提取亚像素精密边缘轮廓
	//EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", EdgesSubP1, EdgesSubP1, EdgesSubP1);
	EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 3, 20, 60);

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
	TupleMax(hv_Radius, &hv_Max);
	double Chamfer_Radius = hv_Max.D();
	cout << "Chamfer_Radius():" << Chamfer_Radius << " " << Chamfer_Radius * 0.00693 << endl;
	//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
	//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
	if (HDevWindowStack::IsOpen())
		SetColor(HDevWindowStack::GetActive(), "green");
	if (HDevWindowStack::IsOpen())
		DispObj(ho_Edges, HDevWindowStack::GetActive());
	return Chamfer_Radius;
};


double program_29::TLineFP_Chamfer_H2_117up(HObject ho_Image, double GenRect1, double GenRect2, double GenRect3, double GenRect4,
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
	HTuple  hv_Max;
	// Local control variables
	HTuple  hv_Width, hv_Height, hv_Row, hv_Column;
	HTuple  hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;
	HObject ho_ImageMean1, ho_ImageEmphasize1, ho_ImageIlluminate, ho_ImageEquHisto;
	GetImageSize(ho_Image, &hv_Width, &hv_Height);
	MeanImage(ho_Image, &ho_ImageMean1, 9, 9);
	Emphasize(ho_ImageMean1, &ho_ImageEmphasize1, 3, 3, 0.9);
	Illuminate(ho_ImageEmphasize1, &ho_ImageIlluminate, 10, 160, 0.83);
	EquHistoImage(ho_ImageIlluminate, &ho_ImageEquHisto);
	//dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
	//
	//Segment a region containing the edges
	//基于全局阈值的图像快速阈值化
	//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin与colend可固定不变
	//GenRectangle1(&ho_Rectangle, cam0_4_220_72shangbian_TLineFP[2], 500, cam0_4_220_72youbiann_TLineFP[0] - 20, 3500);
	GenRectangle1(&ho_Rectangle, GenRect1, GenRect2, GenRect3, GenRect4);
	//提取roi区域，roi以外的变黑，不改变整个图像的尺寸大小。corp_domain是只提取roi区域
	ReduceDomain(ho_ImageEquHisto, ho_Rectangle, &ho_ImageReduced);

	//In the subdomain of the image containing the edges,
	//extract subpixel precise edges.
	//提取亚像素精密边缘轮廓
	//EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", EdgesSubP1, EdgesSubP1, EdgesSubP1);
	EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 3, 80, 120);

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
	TupleMax(hv_Radius, &hv_Max);
	double Chamfer_Radius = hv_Max.D();
	cout << "Chamfer_Radius():" << Chamfer_Radius << " " << Chamfer_Radius * 0.00693 << endl;
	//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
	//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
	if (HDevWindowStack::IsOpen())
		SetColor(HDevWindowStack::GetActive(), "green");
	if (HDevWindowStack::IsOpen())
		DispObj(ho_Edges, HDevWindowStack::GetActive());
	return Chamfer_Radius;
};


void program_29::Straight_TLineFP_P(double* TLineFP_result, HObject ho_Image, int Line1, int Line2, int Line3, int Line4,
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

void program_29::Straight_TLineFP_P137(double* TLineFP_result, HObject ho_Image, int Line1, int Line2, int Line3, int Line4,
	int Metrology1, int Metrology2, int Metrology3, int Metrology4)
{
	// Local iconic variables
	HObject  ho_LineContours, ho_LineContour;
	// Local control variables
	HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices;
	HTuple  hv_RowLine, hv_ColumnLine, hv_RowBegin, hv_ColBegin;
	HTuple  hv_RowEnd, hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;
	HTuple  hv_Width, hv_Height;
	HObject  ho_ImageMean1, ho_ImageEmphasize1, ho_ImageIlluminate, ho_ImageEquHisto;
	GetImageSize(ho_Image, &hv_Width, &hv_Height);


	CreateMetrologyModel(&hv_MetrologyHandle);

	MeanImage(ho_Image, &ho_ImageMean1, 9, 9);
	//图像增强
	Emphasize(ho_ImageMean1, &ho_ImageEmphasize1, 3, 3, 0.9);
	//照射增强对比。图像中非常暗的部分被强烈“照亮”，非常亮的部分被“暗化”
	Illuminate(ho_ImageEmphasize1, &ho_ImageIlluminate, 10, 190, 0.75);
	EquHistoImage(ho_ImageIlluminate, &ho_ImageEquHisto);

	hv_Line.Clear();
	hv_Line[0] = Line1;
	hv_Line[1] = Line2;
	hv_Line[2] = Line3;
	hv_Line[3] = Line4;
	AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, Metrology1, Metrology2, Metrology3, Metrology4,
		HTuple(), HTuple(), &hv_LineIndices);
	SetMetrologyObjectParam(hv_MetrologyHandle, "all", "measure_distance", 0.3);
	ApplyMetrologyModel(ho_ImageEquHisto, hv_MetrologyHandle);

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

void program_29::Straight_TLineFP_P116(double* TLineFP_result, HObject ho_Image, int Line1, int Line2, int Line3, int Line4,
	int Metrology1, int Metrology2, int Metrology3, int Metrology4)
{
	// Local iconic variables
	HObject  ho_LineContours, ho_LineContour, ho_ImageMean1, ho_ImageEmphasize1, ho_ImageIlluminate, ho_ImageEquHisto;
	// Local control variables
	HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices;
	HTuple  hv_RowLine, hv_ColumnLine, hv_RowBegin, hv_ColBegin;
	HTuple  hv_RowEnd, hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;
	HTuple  hv_Width, hv_Height;
	GetImageSize(ho_Image, &hv_Width, &hv_Height);


	CreateMetrologyModel(&hv_MetrologyHandle);
	MeanImage(ho_Image, &ho_ImageMean1, 9, 9);
	Emphasize(ho_ImageMean1, &ho_ImageEmphasize1, 3, 3, 0.9);
	Illuminate(ho_ImageEmphasize1, &ho_ImageIlluminate, 10, 190, 0.99);
	EquHistoImage(ho_ImageIlluminate, &ho_ImageEquHisto);
	hv_Line.Clear();
	hv_Line[0] = Line1;
	hv_Line[1] = Line2;
	hv_Line[2] = Line3;
	hv_Line[3] = Line4;
	AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, Metrology1, Metrology2, Metrology3, Metrology4,
		HTuple(), HTuple(), &hv_LineIndices);
	SetMetrologyObjectParam(hv_MetrologyHandle, "all", "measure_distance", 0.3);
	ApplyMetrologyModel(ho_ImageEquHisto, hv_MetrologyHandle);

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

void program_29::Straight_TLineFP_P170(double* TLineFP_result, HObject ho_Image, int Line1, int Line2, int Line3, int Line4,
	int Metrology1, int Metrology2, int Metrology3, int Metrology4)
{
	// Local iconic variables
	HObject  ho_LineContours, ho_LineContour, ho_ImageMean1, ho_ImageEmphasize1, ho_ImageIlluminate, ho_ImageEquHisto;
	// Local control variables
	HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices;
	HTuple  hv_RowLine, hv_ColumnLine, hv_RowBegin, hv_ColBegin;
	HTuple  hv_RowEnd, hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;
	HTuple  hv_Width, hv_Height;
	GetImageSize(ho_Image, &hv_Width, &hv_Height);


	CreateMetrologyModel(&hv_MetrologyHandle);
	MeanImage(ho_Image, &ho_ImageMean1, 9, 9);
	Emphasize(ho_ImageMean1, &ho_ImageEmphasize1, 3, 3, 0.9);
	Illuminate(ho_ImageEmphasize1, &ho_ImageIlluminate, 10, 190, 1.39);
	EquHistoImage(ho_ImageIlluminate, &ho_ImageEquHisto);
	hv_Line.Clear();
	hv_Line[0] = Line1;
	hv_Line[1] = Line2;
	hv_Line[2] = Line3;
	hv_Line[3] = Line4;
	AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, Metrology1, Metrology2, Metrology3, Metrology4,
		HTuple(), HTuple(), &hv_LineIndices);
	SetMetrologyObjectParam(hv_MetrologyHandle, "all", "measure_distance", 0.3);
	ApplyMetrologyModel(ho_ImageEquHisto, hv_MetrologyHandle);

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

void program_29::Straight_TLineFP_P102(double* TLineFP_result, HObject ho_Image, int Line1, int Line2, int Line3, int Line4,
	int Metrology1, int Metrology2, int Metrology3, int Metrology4)
{
	// Local iconic variables
	HObject  ho_LineContours, ho_LineContour, ho_ImageMean1, ho_ImageEmphasize1, ho_ImageIlluminate, ho_ImageEquHisto;
	// Local control variables
	HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices;
	HTuple  hv_RowLine, hv_ColumnLine, hv_RowBegin, hv_ColBegin;
	HTuple  hv_RowEnd, hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;
	HTuple  hv_Width, hv_Height;
	GetImageSize(ho_Image, &hv_Width, &hv_Height);


	CreateMetrologyModel(&hv_MetrologyHandle);
	MeanImage(ho_Image, &ho_ImageMean1, 9, 9);
	Emphasize(ho_ImageMean1, &ho_ImageEmphasize1, 3, 3, 0.9);
	Illuminate(ho_ImageEmphasize1, &ho_ImageIlluminate, 10, 260, 0.75);
	EquHistoImage(ho_ImageIlluminate, &ho_ImageEquHisto);
	hv_Line.Clear();
	hv_Line[0] = Line1;
	hv_Line[1] = Line2;
	hv_Line[2] = Line3;
	hv_Line[3] = Line4;
	AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, Metrology1, Metrology2, Metrology3, Metrology4,
		HTuple(), HTuple(), &hv_LineIndices);
	SetMetrologyObjectParam(hv_MetrologyHandle, "all", "measure_distance", 0.3);
	ApplyMetrologyModel(ho_ImageEquHisto, hv_MetrologyHandle);

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


void program_29::cam0_roundoutMeasure_prepare(int positionNumber)
{
	//trap5轴
	moveControlPtr->setCurrentAxis(5);
	moveControlPtr->setMoveMode("Trap");
	moveControlPtr->setTrapPrm(moveControlPtr->axisCore[moveControlPtr->currentAxisIndex], moveControlPtr->currentAxisNumber, moveControlPtr->trapAuto[moveControlPtr->currentAxisIndex], screwPrepare[positionNumber][0]+ stepLengthCompensation_axis5, moveControlPtr->trapHighVelAuto[moveControlPtr->currentAxisIndex]);
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

	long axis7_stepIncrease = 360 / screwPrepare[positionNumber][2] * 500;
	long axis7_steplength = 0;
	moveControlPtr->setCurrentAxis(7);
	moveControlPtr->zeroPosition();
	moveControlPtr->setMoveMode("Trap");
	for (int i = 0; i < screwPrepare[positionNumber][2]; i++)
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
		string imgPath = dataSavePath + "\\cam1_screw" + to_string(positionNumber + 1) + "_" + to_string(i + 1) + ".bmp";
		cout << "螺纹地址：" << imgPath << endl;
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
	moveControlPtr->setTrapPrm(moveControlPtr->axisCore[moveControlPtr->currentAxisIndex], moveControlPtr->currentAxisNumber, moveControlPtr->trapAuto[moveControlPtr->currentAxisIndex], screwPrepare[positionNumber][1]+ stepLengthCompensation_axis5, moveControlPtr->trapHighVelAuto[moveControlPtr->currentAxisIndex]);
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
	for (int i = 0; i < screwPrepare[positionNumber][2]; i++)
	{
		axis7_steplength = (i + 1) * axis7_stepIncrease;
		moveControlPtr->setTrapPrm(moveControlPtr->axisCore[moveControlPtr->currentAxisIndex], moveControlPtr->currentAxisNumber, moveControlPtr->trapAuto[moveControlPtr->currentAxisIndex], axis7_steplength, moveControlPtr->trapHighVelAuto[moveControlPtr->currentAxisIndex]);
		moveControlPtr->startTrap();
		do
		{
			moveControlPtr->updateAxisStatus(moveControlPtr->currentAxisNumber);
			msleep(200);
		} while (moveControlPtr->bFlagMotion[moveControlPtr->currentAxisIndex]);

		cout << "光幕旋转7轴步长" << axis7_steplength << endl;

		float a = ls_devicePtr->getLsMeasurementValue(5);
		cout << ls_devicePtr->lsMeasureResult[0] << endl;
		cout << ls_devicePtr->lsMeasureResult[1] << endl;
		cout << ls_devicePtr->lsMeasureResult[2] << endl;
		cout << ls_devicePtr->lsMeasureResult[3] << endl;

		myScrewRoundoutDataPtr[positionNumber]->diameter.push_back(ls_devicePtr->lsMeasureResult[0]);
		myScrewRoundoutDataPtr[positionNumber]->out2.push_back(ls_devicePtr->lsMeasureResult[1]);
		myScrewRoundoutDataPtr[positionNumber]->out3.push_back(ls_devicePtr->lsMeasureResult[2]);
		myScrewRoundoutDataPtr[positionNumber]->out4.push_back((ls_devicePtr->lsMeasureResult[1] - ls_devicePtr->lsMeasureResult[2]) / 2);
	};
};
void program_29::clearScrewRoundOutData()
{
	for (int i = 0; i < 2; i++)
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

//孔测量函数



double program_29::cam1Picture_holeAlgorithm0(string imgPath)//H0孔图像算法
{
	//需要3.1在下方贴入孔径图像处理代码
	HTuple halconPath = imgPath.c_str();
	//"cam0_11-460-152左倒角.sbm"
	//cam0_11_460_152LeftChamferBegin_TLineFP()
	double cam0_11_460_152LeftChamferBegin_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ImageMean1, ho_ImageEmphasize1;
		HObject  ho_ImageIlluminate, ho_ImageEquHisto, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_MetrologyHandle, hv_Line1, hv_Line2;
		HTuple  hv_LineIndices, hv_RowLine, hv_ColumnLine, hv_UsedRow;
		HTuple  hv_UsedColumn, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		ReadImage(&ho_Image, halconPath);
		//7.光照不均匀处理
		//这里如果是灰度图像可以通过图像增强和直方图均衡化进行处理
		//彩色图像的处理可以通过颜色三通道进行处理，这里主要是彩色处理
		//把分离出的三张照片分别通过滤波和均衡化进行处理，然后子合成彩色图片
		MeanImage(ho_Image, &ho_ImageMean1, 9, 9);
		//图像增强
		Emphasize(ho_ImageMean1, &ho_ImageEmphasize1, 3, 3, 0.9);
		//照射增强对比。图像中非常暗的部分被强烈“照亮”，非常亮的部分被“暗化”
		Illuminate(ho_ImageEmphasize1, &ho_ImageIlluminate, 10, 85, 0.88);
		EquHistoImage(ho_ImageIlluminate, &ho_ImageEquHisto);
		if (HDevWindowStack::IsOpen())
			DispObj(ho_ImageEquHisto, HDevWindowStack::GetActive());

		CreateMetrologyModel(&hv_MetrologyHandle);//需要3.4.3.1 修改HAC调试坐标
		hv_Line1.Clear();
		hv_Line1[0] = 330;
		hv_Line1[1] = 1231;
		hv_Line1[2] = 337;
		hv_Line1[3] = 1316;
		hv_Line2.Clear();
		hv_Line2[0] = 1642;
		hv_Line2[1] = 1187;
		hv_Line2[2] = 1635;
		hv_Line2[3] = 1321;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line1.TupleConcat(hv_Line2),
			20, 12, 10, 1, HTuple(), HTuple(), &hv_LineIndices);
		GetMetrologyObjectMeasures(&ho_LineContours, hv_MetrologyHandle, "all", "all",
			&hv_RowLine, &hv_ColumnLine);
		ApplyMetrologyModel(ho_ImageEquHisto, hv_MetrologyHandle);
		if (HDevWindowStack::IsOpen())
			SetColor(HDevWindowStack::GetActive(), "yellow");
		if (HDevWindowStack::IsOpen())
			SetLineWidth(HDevWindowStack::GetActive(), 2);
		if (HDevWindowStack::IsOpen())
			DispObj(ho_LineContours, HDevWindowStack::GetActive());
		GetMetrologyObjectResultContour(&ho_LineContour, hv_MetrologyHandle, "all", "all",
			1.5);
		GetMetrologyObjectResult(hv_MetrologyHandle, "all", "all", "used_edges", "row",
			&hv_UsedRow);
		GetMetrologyObjectResult(hv_MetrologyHandle, "all", "all", "used_edges", "column",
			&hv_UsedColumn);
		//get_metrology_object_model_contour (ModelContour, MetrologyHandle, 'all', 1.5)
		FitLineContourXld(ho_LineContour, "tukey", -1, 0, 5, 2, &hv_RowBegin, &hv_ColBegin,
			&hv_RowEnd, &hv_ColEnd, &hv_Nr, &hv_Nc, &hv_Dist);
		if (HDevWindowStack::IsOpen())
			SetColor(HDevWindowStack::GetActive(), "blue");
		if (HDevWindowStack::IsOpen())
			SetLineWidth(HDevWindowStack::GetActive(), 2);
		if (HDevWindowStack::IsOpen())
			DispObj(ho_LineContour, HDevWindowStack::GetActive());
		vector<double> cam000__hole_ColBegin;
		vector<double> cam000__hole_ColEnd;
		vector<double> cam000__hole_RowBegin;
		vector<double> cam000__hole_RowEnd;
		//计算孔径
		for (int i = 0; i < hv_RowBegin.Length(); i++) {
			cam000__hole_RowBegin.push_back(hv_RowBegin[i]);
			cam000__hole_ColBegin.push_back(hv_ColBegin[i]);
			cam000__hole_RowEnd.push_back(hv_RowEnd[i]);
			cam000__hole_ColEnd.push_back(hv_ColEnd[i]);
		}
		/*
		vector<double> midle_point_row;
		vector<double> midle_point_col;
		double midPointCol;
		double midPointRow;
		for (int n = 0; n < 2; n++) {
			midPointRow = (hv_RowBegin[n] + hv_RowEnd[n]) / 2;
			midPointCol = (hv_ColBegin[n] + hv_ColEnd[n]) / 2;
			midle_point_row.push_back(midPointRow);
			midle_point_col.push_back(midPointCol);
		}*/
		double diameter1 = sqrt(pow(cam000__hole_RowBegin[0] - cam000__hole_RowEnd[1], 2) + pow(cam000__hole_ColBegin[0] - cam000__hole_ColEnd[1], 2)) * CalikKong;
		double diameter2 = sqrt(pow(cam000__hole_RowBegin[1] - cam000__hole_RowEnd[0], 2) + pow(cam000__hole_ColBegin[1] - cam000__hole_ColEnd[0], 2)) * CalikKong;
		double diameter3 = sqrt(pow(cam000__hole_RowBegin[0] - cam000__hole_RowBegin[1], 2) + pow(cam000__hole_ColBegin[0] - cam000__hole_ColBegin[1], 2)) * CalikKong;
		double diameter4 = sqrt(pow(cam000__hole_RowEnd[0] - cam000__hole_RowEnd[1], 2) + pow(cam000__hole_ColEnd[0] - cam000__hole_ColEnd[1], 2)) * CalikKong;
		cout << "diameter1:" << diameter1 << " " << "diameter2:" << diameter2 << " " << "diameter3:" << diameter3 << " " << "diameter4:" << diameter4 << endl;
		diameter1 = diameter1 > diameter2 ? diameter1 : diameter2;
		diameter1 = diameter1 > diameter3 ? diameter1 : diameter3;
		diameter1 = diameter1 > diameter4 ? diameter1 : diameter4;
		cout << "final diameter:" << diameter1 << endl;

		return diameter1;
		//需要3.4.3.2 在此剪切return diameter1;
		//resultVectorList.push_back(diameter1);//粘贴完成后删除或者注释掉resultVectorList.push_back(diameter1)
		//pushback_vectors(resultVectorList, 8, 111, 8.9, -0, +0.2);//粘贴完成后删除或者注释掉pushback_vectors(resultVectorList, 8, 111, 8.9, -0, +0.2)
	}//需要复制以上	//3.0 复制以上


	//需要3.1粘贴完成后删除或者注释掉resultVectorList.push_back(diameter1)和pushback_vectors(resultVectorList, 8, 111, 8.9, -0, +0.2); ，并将return 修改为 return diameter1放入大括号内;
	
	
};


double program_29::cam1Picture_holeAlgorithm1(string imgPath)//H1孔图像处理算法
{
	//在下方贴入孔径图像处理代码



	//粘贴完成后删除或者注释掉resultVectorList.push_back(diameter1)和pushback_vectors(resultVectorList, 8, 111, 8.9, -0, +0.2); ，并将return 修改为 return diameter1放入大括号内;
	return 0;
};

double program_29::cam1Picture_holeAlgorithm2(string imgPath)//H2孔图像处理算法
{
	//在下方贴入孔径图像处理代码



	//粘贴完成后删除或者注释掉resultVectorList.push_back(diameter1)和pushback_vectors(resultVectorList, 8, 111, 8.9, -0, +0.2); ，并将return 修改为 return diameter1放入大括号内;
	return 0;
};
double program_29::cam1Picture_holeAlgorithm3(string imgPath)//H3孔图像处理算法
{
	//在下方贴入孔径图像处理代码



	//粘贴完成后删除或者注释掉resultVectorList.push_back(diameter1)和pushback_vectors(resultVectorList, 8, 111, 8.9, -0, +0.2); ，并将return 修改为 return diameter1放入大括号内;
	return 0;
};


//用于测试的函数************



//用于测试的函数************


void program_29::run()
{
	//检测运行提示（设备自检）*****************************************************************************************************

	programConfirm = "";//检查拨叉位置是否安装正确
	emit programTips("小端朝下", 1);
	do
	{
		msleep(500);
	} while (programConfirm != "continue");
	emit updateDeviceInf("正在自动测量，请等待测量完成且勿执行其他操作");


	//测量初始化*******************************************************************************************************

	cam1PathList.clear();
	cam1PathList.shrink_to_fit();
	clearRoundoutData();//清除跳动测量数据
	clearScrewRoundOutData();
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
	defaultPath = defaultPath + "\\measureData\\WZ10442111-90";//！需要将文件后夹后缀名改为对应的零件号
	dataSavePath = defaultPath + "\\" + stime;//新建文件夹
	string command;
	command = "mkdir -p " + dataSavePath;
	system(command.c_str());
	cout << dataSavePath << endl;
	allFeatureFlag = "OK";//全局特征结果重置
	ngFeatureNum = 0;//NG特征数量重置
	emit measureStatistics("--", ngFeatureNum, measurePartsNum_all, yield);
	
	stepLengthCompensation_axis5 = 0;
	//五轴补偿位置计算*******************************************************************************************************
	
	/*
	//光幕轴位置补偿值函数修改完成后取消掉此部分的注释内容
	emit programProcess("正在检测零件安装位置并进行补偿计算", 1);
	stepLengthCompensation_axis5 = 0;
	cam0_exposeTime = 14;
	camPtrList[0]->setExposeTime(cam0_exposeTime);
	camPtrList[0]->m_captureMode = "continuous";
	originalImgPtr = &(camPtrList[0]->capturedImg);
	bool caculationFlag=stepLengthCompensationCaculation_axis5();
	if (!caculationFlag)
	{
		emit updateDeviceInf("零件补偿位置计算出错，请清洁零件后重试");
		emit Finished(false);
		return;
	}
	cout << "5轴补偿的距离是(脉冲)" << stepLengthCompensation_axis5 << endl;
	*/

	//使用光幕传感器进行直径测量检测（上下来回求平均）***************************************************************************
	
	moveControlPtr->setCurrentAxis(5);
	moveControlPtr->setMoveMode("Trap");
	for (int i = 0; i < 2; i++)//第一次测量（向上移动）（需要将i修改为直径采集的总次数）
	{
		emit programProcess(QString("正在对 %1 号特征（%2）进行第1次测量！").arg(lsDiameterResult[i][3]).arg(featureMap[1]), int(i / 4));
		lsSensorMeasure_diameter(i, 1);
	};
	programConfirm = "";
	emit programTips("光幕即将下行检测，请手动将轴旋转90°后，点击'程序确认'按钮，！", 2);
	do
	{
		msleep(500);
	} while (programConfirm != "continue");
	for (int i = 1; i >= 0; i--)//第二次测量（向下移动）（需要将i修改为直径采集的总次数-1）
	{
		emit programProcess(QString("正在对 %1 号特征（%2）进行第2次测量！").arg(lsDiameterResult[i][3]).arg(featureMap[1]), int(19 - (i / 4)));
		lsSensorMeasure_diameter(i, 2);
		lsDiameterResult[i][2] = (lsDiameterResult[i][0] + lsDiameterResult[i][1]) / 2;//计算平均直径
		lsDiameterResult[i][9] = lsDiameterResult[i][2] / 2;//计算平均半径主要用于传递参数给粗糙度测量函数
		switch (i)
		{
		//如有部分直径测量结果不需要显示立即显示的重新编写需要重新编写case 语句，下面示例为d0,d1,d2不直接显示（后面需要取最大值作为输出），d5,d6,d7不直接显示（后面需要取最小值作为输出）
			//如所有直径都想立即显示则删除所有的case 语句
		case 3://88号直径特征（后续取最大值）
			break;
		case 4:
			break;
		case 5:
			break;
		default:
			resultVectorList.push_back(lsDiameterResult[i][2]);
			pushback_vectors(resultVectorList, 1, lsDiameterResult[i][3], lsDiameterResult[i][4], lsDiameterResult[i][6], lsDiameterResult[i][5]);
			break;
		}
	};
	//如不需要对直径数据进行处理需要将下面直径处理的部分语句直接删除********************************************************************************
	//88号直径数据处理取最大值输出
	
	dataScreenOrg.push_back(lsDiameterResult[3][0]);
	dataScreenOrg.push_back(lsDiameterResult[3][1]);
	dataScreenOrg.push_back(lsDiameterResult[4][0]);
	dataScreenOrg.push_back(lsDiameterResult[4][1]);
	dataScreenOrg.push_back(lsDiameterResult[5][0]);
	dataScreenOrg.push_back(lsDiameterResult[5][1]);
	dataScreening_maxTwo(dataScreenOrg, resultVectorList);
	pushback_vectors(resultVectorList, 1, lsDiameterResult[3][3], lsDiameterResult[3][4], lsDiameterResult[3][6], lsDiameterResult[3][5]);
	dataScreenOrg.clear();
	dataScreenOrg.shrink_to_fit();
	

	
	//需要将直径实际测量结果传递给粗糙度用于补偿粗糙度轴移动位置
	cam2_radius[0][1] = lsDiameterResult[1][9];//传递旋转半径d4给粗糙度0号采集位置（如有新增复制本句代码）进行相应修改即可
	cam2_radius[1][1] = lsDiameterResult[2][9];
	cam2_radius[2][1] = lsDiameterResult[6][9];
	/*
	新增参考代码
	cam2_radius[1][1] = lsDiameterResult[11][9];//传递旋转半径d11给粗糙度1号采集位置
	cam2_radius[2][1] = lsDiameterResult[14][9];//传递旋转半径d14给粗糙度2号采集位置
	cam2_radius[3][1] = lsDiameterResult[30][9];
	*/
	


	//向上进行远心和粗糙度测量（上行过程进行检测）********************************************************************************
	
		

	//远心相机拍照控制

	cam0_exposeTime = 14;
	camPtrList[0]->setExposeTime(cam0_exposeTime);//远心0号点
	camPtrList[0]->m_captureMode = "continuous";
	originalImgPtr = &(camPtrList[0]->capturedImg);
	for (int i = 0; i < 1; i++)//远心0-1号采集位置采集图像（将i修改为远心相机的拍照位置总数，循环中的其他语句不需要修改）
	{
		camPtrList[0]->setExposeTime(cam0_exposeTime);
		camPtrList[0]->m_captureMode = "continuous";
		originalImgPtr = &(camPtrList[0]->capturedImg);
		emit programProcess(QString("远心相机正在对 %1 号位置进行轮廓测量！").arg(i + 1), 48 + (i + 1) * 15);
		imgSavePath[i] = cam0_Measure_prepare(i);
	};
	//计算拍照之间的距离 需要3.8 复制以下
	
	//复制以上
	/*
	//如需计算拍照位置1和拍照位置2的距离参照如下代码
	double pic1To2_moveDistance;
	pic1To2_moveDistance = axis5_compensation(cam0_arriveOrgEncode_axis5[2]) - axis5_compensation(cam0_arriveOrgEncode_axis5[1]);//需要修改变量的名称
	*/
	try {
		cam0Picture_algorithm();//远心相机的图像处理函数（视觉处理函数调试完成后需要将代码贴入该函数）需要3.8粘贴进去
	}
	catch (...) {
		emit updateDeviceInf(QString("远心相机图像%1处理错误").arg(fault_detect));
	};
	
	//零件翻边，可能需要，程序暂停,复制以下
	//emit programTips("点击继续！", 3);
	//暂停部分
	/*programConfirm = "doNotContinue";
	do
	{
		msleep(200);
	} while (programConfirm == "doNotContinue");*/
	
	
	
	
	//检测完成各轴回到合适的位置&最终结果存储***************************************************

	//五轴回合适位置
	moveControlPtr->setCurrentAxis(5);
	moveControlPtr->setTrapPrm(moveControlPtr->axisCore[moveControlPtr->currentAxisIndex], moveControlPtr->currentAxisNumber, moveControlPtr->trapAuto[moveControlPtr->currentAxisIndex], 0, moveControlPtr->trapHighVelAuto[moveControlPtr->currentAxisIndex]);
	moveControlPtr->startTrap();
	clearRoundoutData();
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


	//删除照片文件******************************************************************************************************************

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
	//emit programProcess("数据整理完成，请确认检测结果！", 98);//检测统计值计算以及显示
	programConfirm = "";
	/*do
	{
		msleep(300);
	} while (programConfirm != "save" && programConfirm != "delete"); */
	/*if (programConfirm == "save")
	{*/
		cout << "写入" << endl;
		//writeToDatabase();
		//saveAsExcel();//检测结果储存为excel文件
		msleep(1200);
		//ShellExecuteW(NULL, L"open", L"D:\\temporary_sun\\AxisMeasurement\\x64\\Release\\measureData\\upload.bat", NULL, NULL, SW_SHOW);
	//};
	do
	{
		moveControlPtr->updateAxisStatus(moveControlPtr->currentAxisNumber);
		msleep(200);
	} while (!moveControlPtr->bFlagArrive[moveControlPtr->currentAxisIndex] && moveControlPtr->bFlagMotion[moveControlPtr->currentAxisIndex]);
	//emit programProcess("全部测量完成 ！", 100);//检测全部完成提示

	if (allFeatureFlag == "OK")
	{
		emit Finished(true);
	}
	else
	{
		emit Finished(false);
	};
	emit updateDeviceInf("当前测量已完成，请取走零件");
	
	
}
/*
sort(fGlobal.begin(), fGlobal.end());
*/