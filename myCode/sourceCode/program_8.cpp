//KF71-53-009-140****************************************************************************


#include "program_8.h"

program_8::program_8(cam_device* cam0, cam_device* cam1, cam_device* cam2, moveControl* cradDevicePtr, ls_device* lsDevicePtr, QTableWidget* tablePtr, QSqlDatabase* DbPtr, roughnessFun* roughnessObjPtr)
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
	
	for (int i = 0; i < 2; i++)
	{
		roundoutDataPtr[i]= new roundoutData();
	};
	clearRoundoutData();
	
};
program_8::~program_8()
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

void program_8::lsSensorMeasure_diameter(int positionNumber, int time)
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

void program_8::lsSensorMeasure_roundOutPrepare(int startLocation, int endLocation, int positionNumber, int intervalTime = 10)//使用光幕传感器进行跳动测量;startLocation特征测量开始采样位置的索引；endLocation采样结束时的位置索引
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
		cout << "五轴位置" << lsRoundoutSteplength_axis5[positionNumber][i]<< endl;
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
void program_8::axisLineCalculate(double pointsData[][3], int pointsNum, double* directionVector, double* aixsReferancePoint)//计算拟合轴线
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
void program_8::centerCosFit(int positionNumber, int positionIndex)//拟合对应跳动位置的余弦曲线
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
void program_8::axisLoaction(int positionNumber, int positionIndex, double* directionVector, double* aixsReferancePoint)//计算各跳动位置实际回转轴心
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
void program_8::roundoutCalculate(int positionNumber, int positionIndex, int calculateMode = 0)
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

void program_8::clearRoundoutData()
{
	for (int i = 0; i < 2; i++)
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
	fill(&directionVector[0], &directionVector[0] + sizeof(directionVector) / sizeof(double), 0);
	fill(&aixsReferancePoint[0], &aixsReferancePoint[0] + sizeof(aixsReferancePoint) / sizeof(double), 0);
};


//圆度测量相关函数******************************************************************************************************************************************************************************************************************************

/*
//该零件不需要测圆度
void program_8::lsSensorMeasure_roundness(int positionNumber, float intervalTime=50)//使用光幕传感器进行圆度测量
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




//该工序不需要测孔径
/*
void program_8::cam1_Measure_prepare(int positionNumber)
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
string program_8::cam0_Measure_prepare(int positionNumber)
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
	cout << "图像地址" << imgPath << endl;
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
void program_8::cam2_Measure_prepare(int positionNumber)
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
	long axis1_steplength = cam2_parpare[positionNumber][1] + 5000 * (cam2_radius[positionNumber][1] - cam2_radius[positionNumber][0]) - 440;//5个脉冲为1微米
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
		cout << "S_F" << cam2_parpare[positionNumber][1] << "S_C" << axis1_steplength << "D_F" << cam2_radius[positionNumber][0] << "D_A" << cam2_radius[positionNumber][1] << endl;
		moveControlPtr->setTrapPrm(moveControlPtr->axisCore[moveControlPtr->currentAxisIndex], moveControlPtr->currentAxisNumber, moveControlPtr->trapAuto[moveControlPtr->currentAxisIndex], axis1_steplength, moveControlPtr->trapHighVelAuto[moveControlPtr->currentAxisIndex]);
		moveControlPtr->startTrap();
		do
		{
			moveControlPtr->updateAxisStatus(moveControlPtr->currentAxisNumber);
			msleep(200);
		} while (!moveControlPtr->bFlagArrive[moveControlPtr->currentAxisIndex] && moveControlPtr->bFlagMotion[moveControlPtr->currentAxisIndex]);
		//cout << "予以动" << endl;
		camPtrList[2]->startCapture();//开始次移动和拍照
		for (int i = 0; i < 6; i++)
		{
			axis1_steplength = cam2_parpare[positionNumber][1] + 5000 * (cam2_radius[positionNumber][1] - cam2_radius[positionNumber][0]) - 45 * (i - 1) - 50;
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

void program_8::saveAsExcel()//将主界面上的表格保存为excel文件
{
	string excelPath = dataSavePath + "\\" + stime + "_" + partNub.toStdString() + ".xlsx";

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
bool program_8::mergeCells(QString start, QString end, QString value)
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

void program_8::creatDatabaseTable()
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
void program_8::writeToDatabase()
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
void program_8::zeroMeasureNub()
{
	allFeatureFlag = "--";
	ngFeatureNum = 0;//NG特征数量
	measurePartsNum_all = 0;//该型号轴测量总数
	measurePartsNum_ok = 0;//该型号轴合格数量
	measurePartsNum_ng = 0;//该型号轴NG数量
	yield = 0;//该型号轴合格率
	emit measureStatistics(allFeatureFlag, ngFeatureNum, measurePartsNum_all, yield);
};
void program_8::pushback_vectors(vector<double>& fResult, int fType, double fIndex, double fNominalsize, double fLowerTolerance, double fUpperTolerance)
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
		item[0]->setData(Qt::DisplayRole, fIndex);//第一列特征编号
		measureTablePtr->setItem(row, 0, item[0]);
		item[1]->setText(featureMap[fType]);//显示特征名称
		measureTablePtr->setItem(row, 1, item[1]);
		item[2]->setText(featureQualityMap[fQualify[2]]);//显示测量判断结果
		if (fQualify[2])
		{
			item[2]->setBackground(QColor(230, 0, 0));
		}
		else
		{
			item[2]->setBackground(QColor(0, 200, 0));
		};
		measureTablePtr->setItem(row, 2, item[2]);
		item[3]->setText(QString::number(minResult, 'f', 4));//显示测量最小值
		if (fQualify[0])
		{
			if ((fLowerSize - minResult) / tolerance > 0.1 || (minResult - fUpperSize) / tolerance > 0.1)
			{
				item[3]->setBackground(QColor(230, 0, 0));
			}
			else
			{
				item[3]->setBackground(QColor(230, 230, 0));
			}
		}
		else
		{
			item[3]->setBackground(QColor(0, 200, 0));
		};
		measureTablePtr->setItem(row, 3, item[3]);
		item[4]->setText(QString::number(maxResult, 'f', 4));//显示测量最大值
		if (fQualify[1])
		{
			if ((fLowerSize - maxResult) / tolerance > 0.1 || (maxResult - fUpperSize) / tolerance > 0.1)
			{
				item[4]->setBackground(QColor(230, 0, 0));
			}
			else
			{
				item[4]->setBackground(QColor(230, 230, 0));
			};
		}
		else
		{
			item[4]->setBackground(QColor(0, 200, 0));
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

void program_8::clearVector(vector <float>& list)
{
	list.clear();
	list.shrink_to_fit();
};


//远心相机图像检测算法******************************************************************************************************************************************************************************************************************************

/*
//以下是调用测试
//卡尺直接测量
void program_8::Straight_TLineFP(double* TLineFP_result, string imgPath, int Line1, int Line2, int Line3, int Line4,
	int Metrology1, int Metrology2, int Metrology3, int Metrology4)
{
	// Local iconic variables
	HObject  ho_Image, ho_LineContours, ho_LineContour;
	// Local control variables
	HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices;
	HTuple  hv_RowLine, hv_ColumnLine, hv_RowBegin, hv_ColBegin;
	HTuple  hv_RowEnd, hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;
	HTuple  hv_Width, hv_Height;
	HTuple  halconPath = imgPath.c_str();
	ReadImage(&ho_Image, halconPath);
	GetImageSize(ho_Image, &hv_Width, &hv_Height);
	//图像增强
	HObject   ho_GammaImage, ho_ImageEmphasize;
	GammaImage(ho_Image, &ho_GammaImage, 0.416667, 0.055, 0.018, 255, "true");
	Emphasize(ho_GammaImage, &ho_ImageEmphasize, hv_Width, hv_Height, 1.4);
	//图像增强结束

	CreateMetrologyModel(&hv_MetrologyHandle);

	hv_Line.Clear();
	hv_Line[0] = Line1;
	hv_Line[1] = Line2;
	hv_Line[2] = Line3;
	hv_Line[3] = Line4;
	AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, Metrology1, Metrology2, Metrology3, Metrology4,
		HTuple(), HTuple(), &hv_LineIndices);
	SetMetrologyObjectParam(hv_MetrologyHandle, "all", "measure_distance", 0.3);
	ApplyMetrologyModel(ho_ImageEmphasize, hv_MetrologyHandle);

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
*/

/*
函数作用：模板匹配找特征点
*/
/*
void program_8::TemplateMatching_TLineFP(double* TLineFP_result, string imgPath, HTuple hv_ModelFile, int MatchingLine1, int MatchingLine2, int MatchingLine3, int MatchingLine4,
	int Metrology1, int Metrology2, int Metrology3, int Metrology4,
	double ShapeModel1, double ShapeModel2, double ShapeModel3, double ShapeModel4, double ShapeModel5) {
	// Local iconic variables
	HObject  ho_Image, ho_ModelContours, ho_LineContours;
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

	HTuple halconPath = imgPath.c_str();
	//读取模板和图像进行模板匹配*
	ReadImage(&ho_Image, halconPath);
	GetImageSize(ho_Image, &hv_Width, &hv_Height);
	//图像增强
	HObject   ho_GammaImage, ho_ImageEmphasize;
	GammaImage(ho_Image, &ho_GammaImage, 0.416667, 0.055, 0.018, 255, "true");
	Emphasize(ho_GammaImage, &ho_ImageEmphasize, hv_Width, hv_Height, 1.4);
	//图像增强结束
	// 
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
	FindShapeModel(ho_ImageEmphasize, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, ShapeModel1,
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
	ApplyMetrologyModel(ho_ImageEmphasize, hv_MetrologyHandle);

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
*/

/*
函数作用：模板匹配求倒角
*/
/*
double program_8::TemplateMatching_Chamfer(string imgPath, HTuple hv_ModelFile, double ShapeModel1, double ShapeModel2, double ShapeModel3, double ShapeModel4, double ShapeModel5,
	int Rectangle1, int Rectangle2, int Rectangle3, int Rectangle4,
	int EdgesSubP1, int EdgesSubP2, int EdgesSubP3,
	bool Segment, int SegmentContour1, int SegmentContour2, int SegmentContour3,
	double SelectContour1, double SelectContour2, double SelectContour3,
	int UnionContour1, int UnionContour2,
	int FitCircleContour1, int FitCircleContour2, int FitCircleContour3, int FitCircleContour4, int FitCircleContour5)
{

	// Local iconic variables
	HObject  ho_Image, ho_ModelContours, ho_ContoursAffinTrans;
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
	HTuple halconPath = imgPath.c_str();
	ReadImage(&ho_Image, halconPath);
	GetImageSize(ho_Image, &hv_Width, &hv_Height);
	//图像增强
	HObject   ho_GammaImage, ho_ImageEmphasize;
	GammaImage(ho_Image, &ho_GammaImage, 0.416667, 0.055, 0.018, 255, "true");
	Emphasize(ho_GammaImage, &ho_ImageEmphasize, hv_Width, hv_Height, 1.4);
	//图像增强结束
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
	FindShapeModel(ho_ImageEmphasize, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, ShapeModel1,
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

	ReduceDomain(ho_ImageEmphasize, ho_RectangleTrans, &ho_ImageReduced);
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


/*
函数作用：根据起始点与终止点求倒角
*/

/*
double program_8::TLineFP_Chamfer(string imgPath, double GenRect1, double GenRect2, double GenRect3, double GenRect4,
	int EdgesSubP1, int EdgesSubP2, int EdgesSubP3,
	bool Segment, int SegmentContour1, int SegmentContour2, int SegmentContour3,
	double SelectContour1, double SelectContour2, double SelectContour3,
	int UnionContour1, int UnionContour2,
	int FitCircleContour1, int FitCircleContour2, int FitCircleContour3, int FitCircleContour4, int FitCircleContour5)
{

	// Local iconic variables
	HObject  ho_Image, ho_Rectangle, ho_ImageReduced;
	HObject  ho_Edges, ho_ContoursSplit, ho_SelectedContours;
	HObject  ho_UnionContours;

	// Local control variables
	HTuple  hv_Width, hv_Height, hv_Row, hv_Column;
	HTuple  hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;
	HTuple halconPath = imgPath.c_str();
	ReadImage(&ho_Image, halconPath);
	GetImageSize(ho_Image, &hv_Width, &hv_Height);
	//图像增强
	HObject   ho_GammaImage, ho_ImageEmphasize;
	GammaImage(ho_Image, &ho_GammaImage, 0.416667, 0.055, 0.018, 255, "true");
	Emphasize(ho_GammaImage, &ho_ImageEmphasize, hv_Width, hv_Height, 1.4);
	//图像增强结束
	//dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
	//
	//Segment a region containing the edges
	//基于全局阈值的图像快速阈值化
	//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin与colend可固定不变
	//GenRectangle1(&ho_Rectangle, cam0_4_220_72shangbian_TLineFP[2], 500, cam0_4_220_72youbiann_TLineFP[0] - 20, 3500);
	GenRectangle1(&ho_Rectangle, GenRect1, GenRect2, GenRect3, GenRect4);
	//提取roi区域，roi以外的变黑，不改变整个图像的尺寸大小。corp_domain是只提取roi区域
	ReduceDomain(ho_ImageEmphasize, ho_Rectangle, &ho_ImageReduced);

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
*/

/*
函数作用：远心相机视觉算法
*/

void program_8::Straight_TLineFP_P(double* TLineFP_result, HObject ho_Image, int Line1, int Line2, int Line3, int Line4,
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
void program_8::TemplateMatching_TLineFP_P(double* TLineFP_result, HObject ho_Image, HTuple hv_ModelFile, int MatchingLine1, int MatchingLine2, int MatchingLine3, int MatchingLine4,
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
double program_8::TemplateMatching_Chamfer_P(HObject ho_Image, HTuple hv_ModelFile, double ShapeModel1, double ShapeModel2, double ShapeModel3, double ShapeModel4, double ShapeModel5,
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
double program_8::TLineFP_Chamfer_P(HObject ho_Image, double GenRect1, double GenRect2, double GenRect3, double GenRect4,
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
HObject program_8::imgAug(string imgPath) {
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
};



void  program_8::cam0Picture_algorithm()
{
	//newProgram0_4.cpp begin

	string Img1path = imgSavePath[0];
	string Img2path = imgSavePath[1];
	string Img3path = imgSavePath[2];

	HObject Img1Aug = imgAug(Img1path);


	HObject Img2Aug = imgAug(Img2path);


	HObject Img3Aug = imgAug(Img3path);



	int fault_detect = 0;
	//准备开始图像采集


	fault_detect = 1;
	//以下为图像1
	HTuple Pattern1path = "./programParmeter/KF71-53-009-140/cam0_1-140-44.sbm";
	double cam0_1_40right_TLineFP[4];
	TemplateMatching_TLineFP_P(cam0_1_40right_TLineFP, Img1Aug,
		Pattern1path,
		30, 60, 200, 60,
		40, 12, 1, 70,
		0.5, 1, 0.3, 0, 0.7);

	double cam0_1_44_TLineFP[4];
	TemplateMatching_TLineFP_P(cam0_1_44_TLineFP, Img1Aug,
		Pattern1path,
		-40, -20, 20, 40,
		40, 12, 1, 70,
		0.5, 1, 0.3, 0, 0.7);

	double cam0_1_45_TLineFP[4];
	TemplateMatching_TLineFP_P(cam0_1_45_TLineFP, Img1Aug,
		Pattern1path,
		-210, -105, -160, -50,
		40, 12, 1, 40,
		0.5, 1, 0.3, 0, 0.7);

	double cam0_1_38right_TLineFP[4];
	TemplateMatching_TLineFP_P(cam0_1_38right_TLineFP, Img1Aug,
		Pattern1path,
		890, 60, 1070, 60,
		40, 12, 1, 70,
		0.5, 1, 0.3, 0, 0.7);

	double cam0_1_38left_TLineFP[4];
	TemplateMatching_TLineFP_P(cam0_1_38left_TLineFP, Img1Aug,
		Pattern1path,
		1300, 30, 1300, 80,
		40, 12, 1, 10,
		0.5, 1, 0.3, 0, 0.7);

	double cam0_1_42_TLineFP[4];
	TemplateMatching_TLineFP_P(cam0_1_42_TLineFP, Img1Aug,
		Pattern1path,
		1100, 10, 1060, 50,
		40, 12, 1, 70,
		0.5, 1, 0.3, 0, 0.7);

	double cam0_1_37left_TLineFP[4];
	TemplateMatching_TLineFP_P(cam0_1_37left_TLineFP, Img1Aug,
		Pattern1path,
		1320, 100, 1500, 100,
		40, 12, 1, 70,
		0.5, 1, 0.3, 0, 0.7);

	double cam0_1_39_TLineFP[4];
	TemplateMatching_TLineFP_P(cam0_1_39_TLineFP, Img1Aug,
		Pattern1path,
		1292, 62, 1340, 105,
		40, 12, 1, 10,
		0.5, 1, 0.3, 0, 0.7);

	double cam0_1_36left_TLineFP[4];
	TemplateMatching_TLineFP_P(cam0_1_36left_TLineFP, Img1Aug,
		Pattern1path,
		1980, 110, 2090, 110,
		40, 12, 1, 70,
		0.5, 1, 0.3, 0, 0.7);
	//cam0_1_36left_TLineFP[2] = cam0_1_36left_TLineFP[2] - 3;

	fault_detect = 2;
	//以下为图像2
	HTuple Pattern2_1path = "./programParmeter/KF71-53-009-140/cam0_2-140-30.sbm";
	double cam0_2_25left_TLineFP[4];
	TemplateMatching_TLineFP_P(cam0_2_25left_TLineFP, Img2Aug,
		Pattern2_1path,
		750, 60, 900, 60,
		40, 12, 1, 70,
		0.5, 1, 0.3, 0, 0.7);

	double cam0_2_26left_TLineFP[4];
	TemplateMatching_TLineFP_P(cam0_2_26left_TLineFP, Img2Aug,
		Pattern2_1path,
		30, 60, 200, 60,
		40, 12, 1, 70,
		0.5, 1, 0.3, 0, 0.7);

	double cam0_2_25right_TLineFP[4];
	TemplateMatching_TLineFP_P(cam0_2_25right_TLineFP, Img2Aug,
		Pattern2_1path,
		10, -20, 10, 40,
		40, 12, 1, 10,
		0.5, 1, 0.3, 0, 0.7);

	double cam0_2_33_TLineFP[4];
	TemplateMatching_TLineFP_P(cam0_2_33_TLineFP, Img2Aug,
		Pattern2_1path,
		10, 20, 50, 70,
		40, 12, 1, 40,
		0.5, 1, 0.3, 0, 0.7);

	double cam0_2_30right_TLineFP[4];
	TemplateMatching_TLineFP_P(cam0_2_30right_TLineFP, Img2Aug,
		Pattern2_1path,
		-440, 50, -300, 50,
		40, 12, 1, 70,
		0.5, 1, 0.3, 0, 0.7);
	cam0_2_30right_TLineFP[2] = cam0_2_30right_TLineFP[2] - 2;

	double cam0_2_32_TLineFP[4];
	TemplateMatching_TLineFP_P(cam0_2_32_TLineFP, Img2Aug,
		Pattern2_1path,
		-230, -30, -300, 30,
		40, 12, 1, 40,
		0.5, 1, 0.3, 0, 0.7);

	double cam0_2_27right_TLineFP[4];
	TemplateMatching_TLineFP_P(cam0_2_27right_TLineFP, Img2Aug,
		Pattern2_1path,
		-2050, 50, -1800, 50,
		40, 12, 1, 70,
		0.5, 1, 0.3, 0, 0.7);

	double cam0_2_28_TLineFP[4];
	TemplateMatching_TLineFP_P(cam0_2_28_TLineFP, Img2Aug,
		Pattern2_1path,
		-2160, -70, -2070, 20,
		40, 12, 1, 40,
		0.5, 1, 0.3, 0, 0.7);


	fault_detect = 3;
	//以下为图像3
	HTuple Pattern3_1path = "./programParmeter/KF71-53-009-140/cam0_3-140-14.sbm";
	double cam0_3_20left_TLineFP[4];
	TemplateMatching_TLineFP_P(cam0_3_20left_TLineFP, Img3Aug,
		Pattern3_1path,
		460, -130, 460, -60,
		40, 12, 1, 50,
		0.5, 1, 0.3, 0, 0.7);

	double cam0_3_20right_TLineFP[4];
	TemplateMatching_TLineFP_P(cam0_3_20right_TLineFP, Img3Aug,
		Pattern3_1path,
		240, -40, 240, 20,
		40, 12, 1, 10,
		0.5, 1, 0.3, 0, 0.7);

	double cam0_3_21_1_TLineFP[4];
	TemplateMatching_TLineFP_P(cam0_3_21_1_TLineFP, Img3Aug,
		Pattern3_1path,
		260, 40, 330, 40,
		40, 12, 1, 10,
		0.5, 1, 0.3, 0, 0.7);

	double cam0_3_13_TLineFP[4];
	TemplateMatching_TLineFP_P(cam0_3_13_TLineFP, Img3Aug,
		Pattern3_1path,
		465, -50, 370, 55,
		40, 12, 1, 10,
		0.5, 1, 0.3, 0, 0.7);

	double cam0_3_14right_TLineFP[4];
	TemplateMatching_TLineFP_P(cam0_3_14right_TLineFP, Img3Aug,
		Pattern3_1path,
		20, -50, 20, 20,
		40, 12, 1, 10,
		0.5, 1, 0.3, 0, 0.7);

	double cam0_3_21right_TLineFP[4];
	TemplateMatching_TLineFP_P(cam0_3_21right_TLineFP, Img3Aug,
		Pattern3_1path,
		-540, 120, -310, 120,
		40, 12, 1, 70,
		0.5, 1, 0.3, 0, 0.7);

	double cam0_3_17_TLineFP[4];
	TemplateMatching_TLineFP_P(cam0_3_17_TLineFP, Img3Aug,
		Pattern3_1path,
		-270, 65, -310, 115,
		40, 12, 1, 40,
		0.5, 1, 0.3, 0, 0.7);

	double cam0_3_16right_TLineFP[4];
	TemplateMatching_TLineFP_P(cam0_3_16right_TLineFP, Img3Aug,
		Pattern3_1path,
		-1290, 120, -1010, 120,
		40, 12, 1, 70,
		0.5, 1, 0.3, 0, 0.7);

	double cam0_3_19_TLineFP[4];
	TemplateMatching_TLineFP_P(cam0_3_19_TLineFP, Img3Aug,
		Pattern3_1path,
		-1350, 40, -1280, 110,
		40, 12, 1, 70,
		0.5, 1, 0.3, 0, 0.7);




	//图像3结束
	fault_detect = 0;

	//以下程序对特征集中处理，不涉及特征点检测
	double AxialD_140_20 = (cam0_3_20left_TLineFP[0] + cam0_3_20left_TLineFP[2]) / 2 - (cam0_3_20right_TLineFP[0] + cam0_3_20right_TLineFP[2]) / 2 - 2;
	resultVectorList.push_back(AxialD_140_20 * Calik);
	pushback_vectors(resultVectorList, 4, 20, 2.6, -0.05, +0.05);


	double AxialD_140_25 = cam0_2_25left_TLineFP[2] - cam0_2_25right_TLineFP[2];
	resultVectorList.push_back(AxialD_140_25 * Calik);
	pushback_vectors(resultVectorList, 4, 25, 10.5, -0.05, +0.05);

	double AxialD_140_26 = cam0_2_26left_TLineFP[0] - cam0_2_25right_TLineFP[2];
	resultVectorList.push_back(AxialD_140_26 * Calik);
	pushback_vectors(resultVectorList, 4, 26, 0.58, -0.16, +0.16);

	double AxialD_140_30 = cam0_2_25right_TLineFP[2] - cam0_2_30right_TLineFP[2];
	resultVectorList.push_back(AxialD_140_30 * Calik);
	pushback_vectors(resultVectorList, 4, 30, 4.1, -0.05, +0.05);

	double AxialD_140_27 = cam0_2_30right_TLineFP[2] - cam0_2_27right_TLineFP[0];
	resultVectorList.push_back(AxialD_140_27 * Calik);
	pushback_vectors(resultVectorList, 4, 27, 20.8, -0.05, +0.05);

	double AxialD_140_9 = (cam0_3_20left_TLineFP[0] + cam0_3_20left_TLineFP[2]) / 2 - cam0_2_25right_TLineFP[2];
	resultVectorList.push_back(AxialD_140_9 * Calik + pic2To3_moveDistance);
	pushback_vectors(resultVectorList, 4, 9, 369.3, -0.1, +0.1);

	

	double Chamfer_140_20_1 = TLineFP_Chamfer_P(Img3Aug,
		cam0_3_20right_TLineFP[0] - 40, cam0_3_20right_TLineFP[1] - 100, cam0_3_21_1_TLineFP[0] - 5, cam0_3_20right_TLineFP[1] - 10,
		2, 20, 60,
		true, 5, 4, 2,
		20, -0.5, 0.5,
		90, 1,
		-1, 0, 0, 3, 2);
	resultVectorList.push_back(Chamfer_140_20_1 * Calik);
	pushback_vectors(resultVectorList, 2, 20, 0.4, -0.2, +0.2);

	double Chamfer_140_20_2 = TLineFP_Chamfer_P(Img3Aug,
		cam0_3_14right_TLineFP[0] - 25, cam0_3_14right_TLineFP[1] - 100, cam0_3_14right_TLineFP[0] + 40, cam0_3_14right_TLineFP[1],
		2, 20, 60,
		true, 5, 4, 2,
		20, -0.5, 0.5,
		90, 1,
		-1, 0, 0, 3, 2);
	resultVectorList.push_back(Chamfer_140_20_2* Calik);
	pushback_vectors(resultVectorList, 2, 20, 0.4, -0.2, +0.2);

	double AxialD_140_13_1 = atan((cam0_3_13_TLineFP[0] - cam0_3_13_TLineFP[2]) / (cam0_3_13_TLineFP[3] - cam0_3_13_TLineFP[1])) * 180 / PI;
	resultVectorList.push_back(AxialD_140_13_1);
	pushback_vectors(resultVectorList, 3, 13.1, 45, -45, +45);
	/// 下面的写入表格需要单独处理
	double AxialD_140_13_2 = ((cam0_3_13_TLineFP[0] - cam0_3_13_TLineFP[2]) + (cam0_3_13_TLineFP[3] - cam0_3_13_TLineFP[1])) / 2;
	resultVectorList.push_back(AxialD_140_13_2 * Calik);
	pushback_vectors(resultVectorList, 4, 13, 1, -0.3, +0.3);

	double AxialD_140_14 = (cam0_3_20right_TLineFP[0] + cam0_3_20right_TLineFP[2]) / 2 + 2 - (cam0_3_14right_TLineFP[0] + cam0_3_14right_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_140_14 * Calik);
	pushback_vectors(resultVectorList, 4, 14, 2.8, -0, +0.2);

	double AxialD_140_21 = (cam0_3_20left_TLineFP[0] + cam0_3_20left_TLineFP[2]) / 2 - cam0_3_21right_TLineFP[2] + 2;
	resultVectorList.push_back(AxialD_140_21 * Calik);
	pushback_vectors(resultVectorList, 4, 21, 9.6, -0.05, +0.05);

	double AxialD_140_17 = atan((cam0_3_17_TLineFP[0] - cam0_3_17_TLineFP[2]) / (cam0_3_17_TLineFP[3] - cam0_3_17_TLineFP[1])) * 180 / PI;
	resultVectorList.push_back(AxialD_140_17);
	pushback_vectors(resultVectorList, 3, 17, 45, -1, +1);

	double AxialD_140_33 = atan((cam0_2_33_TLineFP[2] - cam0_2_33_TLineFP[0]) / (cam0_2_33_TLineFP[3] - cam0_2_33_TLineFP[1])) * 180 / PI;
	resultVectorList.push_back(AxialD_140_33);
	pushback_vectors(resultVectorList, 3, 33, 45, -1, +1);

	double AxialD_140_32 = atan((cam0_2_32_TLineFP[0] - cam0_2_32_TLineFP[2]) / (cam0_2_32_TLineFP[3] - cam0_2_32_TLineFP[1])) * 180 / PI;
	resultVectorList.push_back(AxialD_140_32);
	pushback_vectors(resultVectorList, 3, 32, 45, -1, +1);

	double AxialD_140_28 = atan((cam0_2_28_TLineFP[2] - cam0_2_28_TLineFP[0]) / (cam0_2_28_TLineFP[3] - cam0_2_28_TLineFP[1])) * 180 / PI;
	resultVectorList.push_back(AxialD_140_28);
	pushback_vectors(resultVectorList, 3, 28, 45, -1, +1);

	double Chamfer_140_29 = TLineFP_Chamfer_P(Img2Aug,
		cam0_2_28_TLineFP[0] - 330, 1400, cam0_2_28_TLineFP[0], cam0_2_28_TLineFP[1],
		2, 20, 60,
		true, 5, 4, 2,
		20, -0.5, 0.5,
		90, 1,
		-1, 0, 0, 3, 2);
	resultVectorList.push_back(Chamfer_140_29 * Calik);
	pushback_vectors(resultVectorList, 2, 29, 5, -1, +1);

	double Chamfer_140_15 = TLineFP_Chamfer_P(Img3Aug,
		cam0_3_17_TLineFP[0], 1800, cam0_3_17_TLineFP[0] + 75, 2200,
		2, 20, 60,
		true, 5, 4, 2,
		20, -0.5, 0.5,
		90, 1,
		-1, 0, 0, 3, 2);
	resultVectorList.push_back(Chamfer_140_15 * Calik);
	pushback_vectors(resultVectorList, 2, 15, 1, -0.3, +0.3);

	double Chamfer_140_21_1 = TLineFP_Chamfer_P(Img3Aug,
		cam0_3_21_1_TLineFP[0] - 60, cam0_3_21_1_TLineFP[1] - 30, cam0_3_21_1_TLineFP[0], cam0_3_21_1_TLineFP[1] + 50,
		2, 20, 60,
		true, 5, 4, 2,
		20, -0.5, 0.5,
		90, 1,
		-1, 0, 0, 3, 2);
	resultVectorList.push_back(Chamfer_140_21_1* Calik);

	double Chamfer_140_21_2 = TLineFP_Chamfer_P(Img3Aug,
		cam0_3_14right_TLineFP[0] - 25, cam0_3_14right_TLineFP[3], cam0_3_14right_TLineFP[0] + 30, cam0_3_14right_TLineFP[3] + 150,
		2, 20, 60,
		true, 5, 4, 2,
		20, -0.5, 0.5,
		90, 1,
		-1, 0, 0, 3, 2);
	resultVectorList.push_back(Chamfer_140_21_2 * Calik);
	pushback_vectors(resultVectorList, 2, 21, 0.2, -0.1, +0.1);

	

	double AxialD_140_16 = cam0_3_21right_TLineFP[2] - cam0_3_16right_TLineFP[0];
	resultVectorList.push_back(AxialD_140_16 * Calik);
	pushback_vectors(resultVectorList, 4, 16, 11.4, -0.05, +0.05);

	double AxialD_140_19 = atan((cam0_3_19_TLineFP[2] - cam0_3_19_TLineFP[0]) / (cam0_3_19_TLineFP[3] - cam0_3_19_TLineFP[1])) * 180 / PI;
	resultVectorList.push_back(AxialD_140_19);
	pushback_vectors(resultVectorList, 3, 19, 45, -1, +1);

	double Chamfer_140_18 = TLineFP_Chamfer_P(Img3Aug,
		cam0_3_19_TLineFP[0] - 350, 1600, cam0_3_19_TLineFP[0], cam0_3_19_TLineFP[1],
		2, 20, 60,
		true, 5, 4, 2,
		20, -0.5, 0.5,
		90, 1,
		-1, 0, 0, 3, 2);
	resultVectorList.push_back(Chamfer_140_18 * Calik);
	pushback_vectors(resultVectorList, 2, 18, 6, -1, +1);

	double Chamfer_140_24 = TLineFP_Chamfer_P(Img2Aug,
		cam0_2_25left_TLineFP[2] + 20, 1600, cam0_2_25left_TLineFP[2] + 800, 2300,
		2, 20, 60,
		true, 5, 4, 2,
		20, -0.5, 0.5,
		90, 1,
		-1, 0, 0, 3, 2);
	resultVectorList.push_back(Chamfer_140_24 * Calik);
	pushback_vectors(resultVectorList, 2, 24, 13, -2, +2);

	double Chamfer_140_31_1 = TLineFP_Chamfer_P(Img2Aug,
		cam0_2_25right_TLineFP[0] - 80, 1800, cam0_2_25right_TLineFP[0], cam0_2_25right_TLineFP[1],
		2, 20, 60,
		true, 5, 4, 2,
		20, -0.5, 0.5,
		90, 1,
		-1, 0, 0, 3, 2);
	resultVectorList.push_back(Chamfer_140_31_1 * Calik);

	double Chamfer_140_31_2 = TLineFP_Chamfer_P(Img2Aug,
		cam0_2_32_TLineFP[0], 1800, cam0_2_32_TLineFP[0] + 90, cam0_2_32_TLineFP[1],
		2, 20, 60,
		true, 5, 4, 2,
		20, -0.5, 0.5,
		90, 1,
		-1, 0, 0, 3, 2);
	resultVectorList.push_back(Chamfer_140_31_2 * Calik);
	pushback_vectors(resultVectorList, 2, 31, 1, -0.3, +0.3);
	// 
	// 

	double AxialD_140_44 = atan((cam0_1_44_TLineFP[2] - cam0_1_44_TLineFP[0]) / (cam0_1_44_TLineFP[3] - cam0_1_44_TLineFP[1])) * 180 / PI;
	resultVectorList.push_back(AxialD_140_44);
	pushback_vectors(resultVectorList, 3, 44, 45, -1, +1);

	double Chamfer_140_43 = TLineFP_Chamfer_P(Img1Aug,
		cam0_1_44_TLineFP[0] - 80, 1700, cam0_1_44_TLineFP[0] - 15, cam0_1_44_TLineFP[1],
		2, 20, 60,
		true, 5, 4, 2,
		20, -0.5, 0.5,
		90, 1,
		-1, 0, 0, 3, 2);
	resultVectorList.push_back(Chamfer_140_43 * Calik);
	pushback_vectors(resultVectorList, 2, 43, 0.8, -0.3, +0.3);

	double AxialD_140_45_1 = atan((cam0_1_45_TLineFP[2] - cam0_1_45_TLineFP[0]) / (cam0_1_45_TLineFP[3] - cam0_1_45_TLineFP[1])) * 180 / PI;
	resultVectorList.push_back(AxialD_140_45_1);
	pushback_vectors(resultVectorList, 3, 45.1, 45, -45, +45);
	/// 下面的写入表格需要单独处理
	double AxialD_140_45_2 = ((cam0_1_45_TLineFP[2] - cam0_1_45_TLineFP[0]) + (cam0_1_45_TLineFP[3] - cam0_1_45_TLineFP[1])) / 2;
	resultVectorList.push_back(AxialD_140_45_2 * Calik);
	pushback_vectors(resultVectorList, 4, 45, 0.4, -0.2, +0.2);

	double AxialD_140_42 = atan((cam0_1_42_TLineFP[0] - cam0_1_42_TLineFP[2]) / (cam0_1_42_TLineFP[3] - cam0_1_42_TLineFP[1])) * 180 / PI;
	resultVectorList.push_back(AxialD_140_42);
	pushback_vectors(resultVectorList, 3, 42, 45, -1, +1);

	double Chamfer_140_41_2 = TLineFP_Chamfer_P(Img1Aug,
		cam0_1_42_TLineFP[0], 1700, cam0_1_42_TLineFP[0] + 55, 2000,
		2, 20, 60,
		true, 5, 4, 2,
		20, -0.5, 0.5,
		90, 1,
		-1, 0, 0, 3, 2);
	resultVectorList.push_back(Chamfer_140_41_2 * Calik);

	double Chamfer_140_41_1 = TLineFP_Chamfer_P(Img1Aug,
		cam0_1_38left_TLineFP[0] - 75, 1700, cam0_1_38left_TLineFP[0], cam0_1_38left_TLineFP[1],
		2, 20, 60,
		true, 5, 4, 2,
		20, -0.5, 0.5,
		90, 1,
		-1, 0, 0, 3, 2);
	resultVectorList.push_back(Chamfer_140_41_1 * Calik);
	pushback_vectors(resultVectorList, 2, 41, 1, -0.3, +0.3);

	double AxialD_140_38 = (cam0_1_38left_TLineFP[0] + cam0_1_38left_TLineFP[2]) / 2 - cam0_1_38right_TLineFP[2];
	resultVectorList.push_back(AxialD_140_38 * Calik);
	pushback_vectors(resultVectorList, 4, 38, 3.1, -0.05, +0.05);

	double AxialD_140_40 = cam0_1_38left_TLineFP[2] - cam0_1_40right_TLineFP[0];
	resultVectorList.push_back(AxialD_140_40 * Calik);
	pushback_vectors(resultVectorList, 4, 40, 15.2, -0.05, +0.05);

	double AxialD_140_37 = cam0_1_37left_TLineFP[0] - (cam0_1_38left_TLineFP[0] + cam0_1_38left_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_140_37 * Calik);
	pushback_vectors(resultVectorList, 4, 37, 0.5, -0.18, +0.18);

	double AxialD_140_39 = atan((cam0_1_39_TLineFP[2] - cam0_1_39_TLineFP[0]) / (cam0_1_39_TLineFP[3] - cam0_1_39_TLineFP[1])) * 180 / PI;
	resultVectorList.push_back(AxialD_140_39);
	pushback_vectors(resultVectorList, 3, 39, 45, -1, +1);
	// 	 cam0_1_39_TLineFP

	double AxialD_140_36 = cam0_1_36left_TLineFP[2] - (cam0_1_38left_TLineFP[0] + cam0_1_38left_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_140_36 * Calik);
	pushback_vectors(resultVectorList, 4, 36, 9.39, -0.05, +0.05);

	double Chamfer_140_35 = TLineFP_Chamfer_P(Img1Aug,
		cam0_1_36left_TLineFP[2] + 8, 1600, cam0_1_36left_TLineFP[2] + 420, 2100,
		2, 20, 60,
		true, 5, 4, 2,
		20, -0.5, 0.5,
		90, 1,
		-1, 0, 0, 3, 2);
	resultVectorList.push_back(Chamfer_140_35 * Calik);
	pushback_vectors(resultVectorList, 2, 35, 7.5, -2, +2);

	double AxialD_140_10 = (cam0_3_20left_TLineFP[0] + cam0_3_20left_TLineFP[2]) / 2 - (cam0_1_38left_TLineFP[0] + cam0_1_38left_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_140_10 * Calik + pic1To2_moveDistance + pic2To3_moveDistance);
	pushback_vectors(resultVectorList, 4, 10, 570.1, -0.2, +0.2);
};
	


//孔和花键的测量函数*************************************************************************************************************************************************************
//本工序不测量


//用于测试的函数************



//用于测试的函数************


void program_8::run()
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
	//由于不需要跳动测量，所以将相关变量屏蔽掉
	//cam1PathList.clear();
	//cam1PathList.shrink_to_fit();
	clearRoundoutData();
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
	cout << "运行目录" << defaultPath << endl;
	defaultPath = defaultPath+"\\measureData\\KF71-53-009-140";
	dataSavePath = defaultPath + "\\" + stime;//新建文件夹
	cout << "保存目录" << dataSavePath << endl;
	string command;
	command = "mkdir -p " + dataSavePath;
	system(command.c_str());
	cout << dataSavePath << endl;
	allFeatureFlag = "OK";//全局特征结果重置
	ngFeatureNum = 0;//NG特征数量重置
	emit measureStatistics("--", ngFeatureNum, measurePartsNum_all, yield);


	//使用光幕传感器进行直径测量检测（上下来回求平均）***************************************************************************
	
	
	moveControlPtr->setCurrentAxis(5);
	moveControlPtr->setMoveMode("Trap");
	
	for (int i = 0; i < 13; i++)//第一次测量（向上移动）
	{
		emit programProcess(QString("正在对 %1 号特征（%2）进行第1次测量！").arg(lsDiameterResult[i][3]).arg(featureMap[1]), i * 4);
		lsSensorMeasure_diameter(i, 1);
	};
	programConfirm = "";
	emit programTips("光幕即将下行检测，请手动将轴旋转90°后，点击'程序确认'按钮，！",2);
	do
	{
		msleep(500);
	} while (programConfirm != "continue");
	for (int i = 12; i >= 0; i--)//第二次测量（向下移动）
	{
		emit programProcess(QString("正在对 %1 号特征（%2）进行第2次测量！").arg(lsDiameterResult[i][3]).arg(featureMap[1]), 48 - i * 4 );
		lsSensorMeasure_diameter(i, 2);
		lsDiameterResult[i][2] = (lsDiameterResult[i][0] + lsDiameterResult[i][1]) / 2;//计算平均直径
		lsDiameterResult[i][9] = lsDiameterResult[i][2] / 2;
		switch (i)
		{
		case 12:
			break;
		case 10:
			break;
		default:
			resultVectorList.push_back(lsDiameterResult[i][0]);
			resultVectorList.push_back(lsDiameterResult[i][1]);
			pushback_vectors(resultVectorList, 1, lsDiameterResult[i][3], lsDiameterResult[i][4], lsDiameterResult[i][6], lsDiameterResult[i][5]);
			//pushback_vectors(lsDiameterResult[i][2], 1, lsDiameterResult[i][3], lsDiameterResult[i][4], lsDiameterResult[i][6], lsDiameterResult[i][5]);//将一条完整的测量数据储存并显示
			//lsDiameterResult[i][9] = (lsDiameterResult[i][7] + lsDiameterResult[i][8]) / 2;//计算平均旋转半径
		};
	};
	resultVectorList.push_back(lsDiameterResult[12][0]);
	resultVectorList.push_back(lsDiameterResult[12][1]);
	resultVectorList.push_back(lsDiameterResult[10][0]);
	resultVectorList.push_back(lsDiameterResult[10][1]);
	pushback_vectors(resultVectorList, 1, lsDiameterResult[10][3], lsDiameterResult[10][4], lsDiameterResult[10][6], lsDiameterResult[10][5]);

	
	
	cam2_radius[0][1] = lsDiameterResult[4][9];//传递旋转半径给粗糙度测量参数
	//cam2_radius[1][1] = lsDiameterResult[5][9];
	
	
	//测试代码*********
	
	//测试代码*********


	//向上进行远心和粗糙度测量（上行过程进行检测）********************************************************************************
	

	
	//测量粗糙度
	for (int i = 0; i < 1; i++)
	{
		camPtrList[2]->setExposeTime(cam2_exposeTime);//粗糙度采集测量1号位置
		camPtrList[2]->m_captureMode = "continuous";
		originalImgPtr = &(camPtrList[2]->capturedImg);
		emit programProcess(QString("正在对 %1 号特征（粗糙度） 进行测量 ！").arg(roughnessResult[i][0]), 36 + i * 12);
		cam2_Measure_prepare(i);
	};
	for (int i = 0; i < 3; i++)//远心1-2号采集位置采集图像
	{
		camPtrList[0]->setExposeTime(cam0_exposeTime);
		camPtrList[0]->m_captureMode = "continuous";
		originalImgPtr = &(camPtrList[0]->capturedImg);
		emit programProcess(QString("远心相机正在对 %1 号位置进行轮廓测量！").arg(i+1), 48+(i+1)*15);
		imgSavePath[i] = cam0_Measure_prepare(i);
	};
	pic1To2_moveDistance = axis5_compensation(cam0_arriveOrgEncode_axis5[1]) - axis5_compensation(cam0_arriveOrgEncode_axis5[0]);
	pic2To3_moveDistance = axis5_compensation(cam0_arriveOrgEncode_axis5[2]) - axis5_compensation(cam0_arriveOrgEncode_axis5[1]);
	try {
		cam0Picture_algorithm();
	}
	catch(...){
		emit updateDeviceInf(QString("远心相机图像%1处理错误").arg(fault_detect));
	};
	
	
	
	


	//跳动&圆柱度&孔径的测量（再次下行）***************************************************************************************

	
	
	emit programTips("即将进行跳动检测，请参照图示正确安装机心夹！",3);
	msleep(200);
	emit fixtureTips(4);
	programConfirm = "doNotContinue";
	do
	{
		msleep(200);
	} while (programConfirm == "doNotContinue");
	//本工序无孔测量需求
	/*
	emit programProcess("正在对 孔径 进行测量！", 90);//孔径相机1号位置检测
	camPtrList[1]->setExposeTime(cam1_exposeTime);
	camPtrList[1]->m_captureMode = "continuous";
	originalImgPtr = &(camPtrList[1]->capturedImg);
	cam1_Measure_prepare(0);
	try {
		for (int i = 0; i < cam1PathList.size(); i++)
		{
		   double diameter = cam1Picture_holeAlgorithm(cam1PathList[i]);
		   resultVectorList.push_back(diameter);	
		};
		pushback_vectors(resultVectorList, 8, 70, 5.4, 0, +0.15);
	}
	catch (...) {
		resultVectorList.clear();
		resultVectorList.shrink_to_fit();
		emit updateDeviceInf("孔径拍摄图像错误,孔可能未对正或不洁净，请检查确认");
	};
	*/

	//两个截面的跳动测量
	for (int i = 1; i >= 0; i--)
	{
		cout << "第一个跳动位置" << endl;
		emit programProcess(QString("正在对 %1 号特征（跳动） 进行数据采集！").arg(lsRoundoutResult[3][0]), 68);//跳动基准B—跳动测序号4跳动数据采集
		lsSensorMeasure_roundOutPrepare(2, 0, i);
	};

	//检测完成各轴回到合适的位置&跳动数据分析&最终结果存储***************************************************

	//五轴回合适位置
	
	moveControlPtr->setCurrentAxis(5);
	moveControlPtr->setMoveMode("Trap");
	moveControlPtr->setTrapPrm(moveControlPtr->axisCore[moveControlPtr->currentAxisIndex], moveControlPtr->currentAxisNumber, moveControlPtr->trapAuto[moveControlPtr->currentAxisIndex], 0, moveControlPtr->trapHighVelAuto[moveControlPtr->currentAxisIndex]);
	moveControlPtr->startTrap();
	
	//跳动数据处理以及计算()
	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			roundoutCalculate(i, j, 1);//对跳动数据采集的截面不按照基准拟合的方式进行轴线计算
		};
		/*
			//原程序
			int listsize = sizeof(roundoutDataPtr[i]->roundoutResult) / sizeof(roundoutDataPtr[i]->roundoutResult[0]);
			double maxELement = *max_element(roundoutDataPtr[i]->roundoutResult, roundoutDataPtr[i]->roundoutResult + listsize);
			lsRoundoutResult[i][1] = maxELement;
			*/
			//新程序
		vector <double> lsRoundout_forRank;
		lsRoundout_forRank.push_back(roundoutDataPtr[i]->roundoutResult[0]);
		lsRoundout_forRank.push_back(roundoutDataPtr[i]->roundoutResult[1]);
		lsRoundout_forRank.push_back(roundoutDataPtr[i]->roundoutResult[2]);
		sort(lsRoundout_forRank.begin(), lsRoundout_forRank.end());
		lsRoundoutResult[i][1] = lsRoundout_forRank[1];
		resultVectorList.push_back(lsRoundoutResult[i][1]);
		pushback_vectors(resultVectorList, 5, lsRoundoutResult[i][0], 0, 0, lsRoundoutResult[i][2]);
		break;
	}
	clearRoundoutData();
	emit programProcess("各轴正在回到合适位置,请确认测量结果！", 95);//检测统计值计算以及显示
	
	emit programProcess("各轴正在回到合适位置,并整理测量结果，请等待！", 98);//检测统计值计算以及显示
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

	//测试输出变量
	cout << "1-2" << pic1To2_moveDistance << endl;

};

/*
sort(fGlobal.begin(), fGlobal.end());
*/