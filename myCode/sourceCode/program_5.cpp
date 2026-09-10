//WZ10-45-1080-220****************************************************************************


#include "program_5.h"

program_5::program_5(cam_device* cam0, cam_device* cam1, cam_device* cam2, moveControl* cradDevicePtr, ls_device* lsDevicePtr, QTableWidget* tablePtr, QSqlDatabase* DbPtr, roughnessFun* roughnessObjPtr)
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
	
	for (int i = 0; i < 18; i++)
	{
		roundoutDataPtr[i]= new roundoutData();
	};
	//clearRoundoutData();
	
};
program_5::~program_5()
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

void program_5::lsSensorMeasure_diameter(int positionNumber, int time)
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
void program_5::lsSensorMeasure_roundness(int positionNumber, float intervalTime=50)//使用光幕传感器进行圆度测量
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


string program_5::cam0_Measure_prepare(int positionNumber)
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

/*
void program_5::cam1_Measure_prepare(int positionNumber)
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
/*
void program_5::cam2_Measure_prepare(int positionNumber)
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
	long axis1_steplength = cam2_parpare[positionNumber][1] + 5000 * (cam2_radius[positionNumber][1] - cam2_radius[positionNumber][0]) - 500;//5个脉冲为1微米
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
		for (int i = 0; i < 4; i++)
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
			pushback_vectors(roughnessResult[positionNumber][2], 9, roughnessResult[positionNumber][0], 0, 0, roughnessResult[positionNumber][1]);
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
};*/

void program_5::saveAsExcel()//将主界面上的表格保存为excel文件
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
bool program_5::mergeCells(QString start, QString end, QString value)
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

void program_5::creatDatabaseTable()
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
void program_5::writeToDatabase()
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
void program_5::zeroMeasureNub()
{
	allFeatureFlag = "--";
	ngFeatureNum = 0;//NG特征数量
	measurePartsNum_all = 0;//该型号轴测量总数
	measurePartsNum_ok = 0;//该型号轴合格数量
	measurePartsNum_ng = 0;//该型号轴NG数量
	yield = 0;//该型号轴合格率
	emit measureStatistics(allFeatureFlag, ngFeatureNum, measurePartsNum_all, yield);
};
void program_5::pushback_vectors(vector<double>& fResult, int fType, double fIndex, double fNominalsize, double fLowerTolerance, double fUpperTolerance)
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

void program_5::clearVector(vector <float>& list)
{
	list.clear();
	list.shrink_to_fit();
};


//远心相机图像检测算法******************************************************************************************************************************************************************************************************************************



void program_5::Straight_TLineFP(double* TLineFP_result, string imgPath, int Line1, int Line2, int Line3, int Line4,
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


//模板匹配找特征点
void program_5::TemplateMatching_TLineFP(double* TLineFP_result, string imgPath, HTuple hv_ModelFile, int MatchingLine1, int MatchingLine2, int MatchingLine3, int MatchingLine4,
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
	//hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_10-460-209.sbm";
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
double program_5::TemplateMatching_Chamfer(string imgPath, HTuple hv_ModelFile, double ShapeModel1, double ShapeModel2, double ShapeModel3, double ShapeModel4, double ShapeModel5,
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
	//hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_4-220-72.sbm";
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
double program_5::TLineFP_Chamfer(string imgPath, double GenRect1, double GenRect2, double GenRect3, double GenRect4,
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
/*
函数作用：远心相机视觉算法
*/

void program_5::cam0Picture4_algorithm(string imgPath)//远心相机4图像算法
{
	cout << "" << endl;
	HTuple halconPath = imgPath.c_str();

	//cam0_4_220_13Chamfer()
	//double cam0_4_220_13Chamfer;
	//{

	//	// Local iconic variables
	//	HObject  ho_Image, ho_ModelContours, ho_ContoursAffinTrans;
	//	HObject  ho_Rectangle, ho_RectangleTrans, ho_ImageReduced;
	//	HObject  ho_Edges;

	//	// Local control variables
	//	HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
	//	HTuple  hv_ReusedRefPointRow, hv_ReusedRefPointCol, hv_NumLevels;
	//	HTuple  hv_AngleStart, hv_AngleExtent, hv_AngleStep, hv_ScaleMin;
	//	HTuple  hv_ScaleMax, hv_ScaleStep, hv_Metric, hv_MinContrast;
	//	HTuple  hv_Row3, hv_Column3, hv_Angle, hv_Score, hv_HomMat2D;
	//	HTuple  hv_HomMat2DIdentity, hv_HomMat2DTranslate, hv_Row;
	//	HTuple  hv_Column, hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;

	//	//读取模板和图像进行模板匹配*
	//	ReadImage(&ho_Image, halconPath);
	//	GetImageSize(ho_Image, &hv_Width, &hv_Height);
	//	hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_4-220-72.sbm";
	//	ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
	//	//获取读取的模板的轮廓，区域坐标等信息
	//	GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
	//	GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
	//	GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
	//		&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
	//	//进行模板匹配
	//	FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.4,
	//		1, 0.3, "least_squares", 0, 0.6, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
	//	//显示形状特征模板匹配结果
	//	//  
	//		//  
	//	VectorAngleToRigid(hv_ReusedRefPointRow, hv_ReusedRefPointCol, 0, hv_Row3, hv_Column3,
	//		hv_Angle, &hv_HomMat2D);
	//	AffineTransContourXld(ho_ModelContours, &ho_ContoursAffinTrans, hv_HomMat2D);
	//	if (HDevWindowStack::IsOpen())
	//		SetColor(HDevWindowStack::GetActive(), "green");
	//	if (HDevWindowStack::IsOpen())
	//		DispObj(ho_ContoursAffinTrans, HDevWindowStack::GetActive());
	//	//
	//	//Segment a region containing the edges
	//	//基于全局阈值的图像快速阈值化
	//	//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin与colend可固定不变
	//	GenRectangle1(&ho_Rectangle, 0, 0, 300, 300);
	//	//设置条状的宽度为70

	//	HomMat2dIdentity(&hv_HomMat2DIdentity);
	//	HomMat2dTranslate(hv_HomMat2DIdentity, hv_Column3 - 1000, hv_Row3 + 1000, &hv_HomMat2DTranslate);

	//	AffineTransRegion(ho_Rectangle, &ho_RectangleTrans, hv_HomMat2DTranslate, "nearest_neighbor");

	//	ReduceDomain(ho_Image, ho_RectangleTrans, &ho_ImageReduced);
	//	//In the subdomain of the image containing the edges,
	//	//extract subpixel precise edges.
	//	//提取亚像素精密边缘轮廓
	//	EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);
	//	//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
	//	FitCircleContourXld(ho_Edges, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
	//		&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
	//	cam0_4_220_13Chamfer = hv_Radius.D();
	//	cout << "cam0_4_220_13Chamfer():" << cam0_4_220_13Chamfer << " " << cam0_4_220_13Chamfer * 0.01216 << endl;
	//	resultVectorList.push_back(cam0_4_220_13Chamfer * Calik);
	//	pushback_vectors(resultVectorList, 2, 82, 3, 0, +0.5);
	//	//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
	//	//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
	//	if (HDevWindowStack::IsOpen())
	//		SetColor(HDevWindowStack::GetActive(), "green");
	//	if (HDevWindowStack::IsOpen())
	//		DispObj(ho_Edges, HDevWindowStack::GetActive());

	//};

	double cam0_4_220_13Chamfer = TLineFP_Chamfer(imgPath,
		1100, 1900, 1500, 2350,
		2, 20, 60,
		true, 5, 4, 2,
		20, -0.5, 0.5,
		90, 1,
		-1, 0, 0, 3, 2);
	resultVectorList.push_back(cam0_4_220_13Chamfer * Calik);
	pushback_vectors(resultVectorList, 2, 82, 3, 0, +0.5);


	//cam0_4_220_14Chamfer()
	double cam0_4_220_14Chamfer;
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_ContoursAffinTrans;
		HObject  ho_Rectangle, ho_RectangleTrans, ho_ImageReduced;
		HObject  ho_Edges, ho_ContoursSplit, ho_SelectedContours;
		HObject  ho_UnionContours;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_ReusedRefPointRow, hv_ReusedRefPointCol, hv_NumLevels;
		HTuple  hv_AngleStart, hv_AngleExtent, hv_AngleStep, hv_ScaleMin;
		HTuple  hv_ScaleMax, hv_ScaleStep, hv_Metric, hv_MinContrast;
		HTuple  hv_Row3, hv_Column3, hv_Angle, hv_Score, hv_HomMat2D;
		HTuple  hv_HomMat2DIdentity, hv_HomMat2DTranslate, hv_Row;
		HTuple  hv_Column, hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_4-220-72.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.4,
			1, 0.3, "least_squares", 0, 0.6, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		GenRectangle1(&ho_Rectangle, 0, 0, 330, 330);
		//设置条状的宽度为70

		HomMat2dIdentity(&hv_HomMat2DIdentity);
		HomMat2dTranslate(hv_HomMat2DIdentity, hv_Column3 - 430, hv_Row3 + 220, &hv_HomMat2DTranslate);

		AffineTransRegion(ho_Rectangle, &ho_RectangleTrans, hv_HomMat2DTranslate, "nearest_neighbor");

		ReduceDomain(ho_Image, ho_RectangleTrans, &ho_ImageReduced);
		//In the subdomain of the image containing the edges,
		//extract subpixel precise edges.
		//提取亚像素精密边缘轮廓
		EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);
		SegmentContoursXld(ho_Edges, &ho_ContoursSplit, "lines_circles", 5, 4, 2);
		//提取出轮廓中较长的部分线段
		SelectContoursXld(ho_ContoursSplit, &ho_SelectedContours, "contour_length", 20,
			hv_Width / 2, -0.5, 0.5);
		//对相邻的轮廓段进行连接
		UnionAdjacentContoursXld(ho_SelectedContours, &ho_UnionContours, 90, 1, "attr_keep");
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		FitCircleContourXld(ho_UnionContours, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		cam0_4_220_14Chamfer = hv_Radius.D();
		cout << "cam0_4_220_14Chamfer():" << cam0_4_220_14Chamfer << " " << cam0_4_220_14Chamfer * 0.01216 << endl;
		resultVectorList.push_back(cam0_4_220_14Chamfer * Calik);
		pushback_vectors(resultVectorList, 2, 83, 3, 0, +0.5);
		//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
		//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
		if (HDevWindowStack::IsOpen())
			SetColor(HDevWindowStack::GetActive(), "green");
		if (HDevWindowStack::IsOpen())
			DispObj(ho_Edges, HDevWindowStack::GetActive());

	};


	//cam0_4_220_43WidthLow_TLineFP()
	double cam0_4_220_43WidthLow_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line1, hv_LineIndices, hv_Line2;
		HTuple  hv_ReusedRefPointRow, hv_ReusedRefPointCol, hv_NumLevels;
		HTuple  hv_AngleStart, hv_AngleExtent, hv_AngleStep, hv_ScaleMin;
		HTuple  hv_ScaleMax, hv_ScaleStep, hv_Metric, hv_MinContrast;
		HTuple  hv_Row3, hv_Column3, hv_Angle, hv_Score, hv_HomMat2D;
		HTuple  hv_RowLine, hv_ColumnLine, hv_RowBegin, hv_ColBegin;
		HTuple  hv_RowEnd, hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_4-220-72.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line2.Clear();
		hv_Line2[0] = 200;
		hv_Line2[1] = 600;
		hv_Line2[2] = 200;
		hv_Line2[3] = 1450;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line2, 50, 12, 1, 40,
			HTuple(), HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.4,
			1, 0.3, "least_squares", 0, 0.6, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
		//显示形状特征模板匹配结果
		//  
			//  
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
		//输出模板匹配后检测直线两端点的位置，即特征点位置。
		//暂时不用上面这个函数来计算 ，算的内容不是很对，目前就用检测出的直线两端点的距离求平均
		//distance_ss (RowBegin[0], ColBegin[0], RowEnd[0], ColEnd[0], RowBegin[1], ColBegin[1], RowEnd[1], ColEnd[1], DistanceMin, DistanceMax)
		cam0_4_220_43WidthLow_TLineFP[0] = hv_RowBegin[0].D();
		cam0_4_220_43WidthLow_TLineFP[1] = hv_ColBegin[0].D();
		cam0_4_220_43WidthLow_TLineFP[2] = hv_RowEnd[0].D();
		cam0_4_220_43WidthLow_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_4_220_43WidthLow_TLineFP():" << cam0_4_220_43WidthLow_TLineFP[0] << "  " << cam0_4_220_43WidthLow_TLineFP[1] << "  " << cam0_4_220_43WidthLow_TLineFP[2] << "  " << cam0_4_220_43WidthLow_TLineFP[3] << endl;

	};

	//cam0_4_220_73Right_TLineFP()
	double cam0_4_220_73Right_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_4-220-72.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = 750;
		hv_Line[1] = -340;
		hv_Line[2] = 750;
		hv_Line[3] = -140;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 40, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.4,
			1, 0.3, "least_squares", 0, 0.6, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
		//显示形状特征模板匹配结果
		//  
			//  
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
		cam0_4_220_73Right_TLineFP[0] = hv_RowBegin[0].D();
		cam0_4_220_73Right_TLineFP[1] = hv_ColBegin[0].D();
		cam0_4_220_73Right_TLineFP[2] = hv_RowEnd[0].D();
		cam0_4_220_73Right_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_4_220_73Right_TLineFP():" << cam0_4_220_73Right_TLineFP[0] << "  " << cam0_4_220_73Right_TLineFP[1] << "  " << cam0_4_220_73Right_TLineFP[2] << "  " << cam0_4_220_73Right_TLineFP[3] << endl;

	};

	//"cam0_4-460-84.sbm"
	//cam0_4_220_84Right_95Left_TLineFP()
	double cam0_4_220_84Right_95Left_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_4-220-84.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -180;
		hv_Line[1] = 20;
		hv_Line[2] = 20;
		hv_Line[3] = 20;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.6,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
		//显示形状特征模板匹配结果
		//  
			///  
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
		cam0_4_220_84Right_95Left_TLineFP[0] = hv_RowBegin[0].D();
		cam0_4_220_84Right_95Left_TLineFP[1] = hv_ColBegin[0].D();
		cam0_4_220_84Right_95Left_TLineFP[2] = hv_RowEnd[0].D();
		cam0_4_220_84Right_95Left_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_4_220_84Right_95Left_TLineFP():" << cam0_4_220_84Right_95Left_TLineFP[0] << "  " << cam0_4_220_84Right_95Left_TLineFP[1] << "  " << cam0_4_220_84Right_95Left_TLineFP[2] << "  " << cam0_4_220_84Right_95Left_TLineFP[3] << endl;

	};

	//cam0_4_220_84Left_TLineFP()

	double cam0_4_220_84Left_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_4-220-84.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -450;
		hv_Line[1] = 500;
		hv_Line[2] = -450;
		hv_Line[3] = 650;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.6,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
		//显示形状特征模板匹配结果
		///  
			//  
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
		cam0_4_220_84Left_TLineFP[0] = hv_RowBegin[0].D();
		cam0_4_220_84Left_TLineFP[1] = hv_ColBegin[0].D();
		cam0_4_220_84Left_TLineFP[2] = hv_RowEnd[0].D();
		cam0_4_220_84Left_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_4_220_84Left_TLineFP():" << cam0_4_220_84Left_TLineFP[0] << "  " << cam0_4_220_84Left_TLineFP[1] << "  " << cam0_4_220_84Left_TLineFP[2] << "  " << cam0_4_220_84Left_TLineFP[3] << endl;

	};

	//cam0_4_220_95Right_TLineFP()
	double cam0_4_220_95Right_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_4-220-84.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = 50;
		hv_Line[1] = 0;
		hv_Line[2] = 170;
		hv_Line[3] = 0;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 40, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.6,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
		//显示形状特征模板匹配结果
		///  
			//  
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
		cam0_4_220_95Right_TLineFP[0] = hv_RowBegin[0].D();
		cam0_4_220_95Right_TLineFP[1] = hv_ColBegin[0].D();
		cam0_4_220_95Right_TLineFP[2] = hv_RowEnd[0].D();
		cam0_4_220_95Right_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_4_220_95Right_TLineFP():" << cam0_4_220_95Right_TLineFP[0] << "  " << cam0_4_220_95Right_TLineFP[1] << "  " << cam0_4_220_95Right_TLineFP[2] << "  " << cam0_4_220_95Right_TLineFP[3] << endl;

	};

	//下方函数传递
	//cam0_4_220_95Chamfer()
	double cam0_4_220_95Chamfer;
	{

		// Local iconic variables
		HObject  ho_Image, ho_Rectangle, ho_ImageReduced;
		HObject  ho_Edges;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_Row, hv_Column;
		HTuple  hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;

		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		//dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
		//
		//Segment a region containing the edges
		//基于全局阈值的图像快速阈值化
		//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin 500与colend 3500可固定不变
		GenRectangle1(&ho_Rectangle, cam0_4_220_84Right_95Left_TLineFP[2] + 5, 500, cam0_4_220_95Right_TLineFP[0], 3500);
		//提取roi区域，roi以外的变黑，不改变整个图像的尺寸大小。corp_domain是只提取roi区域
		ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);

		//In the subdomain of the image containing the edges,
		//extract subpixel precise edges.
		//提取亚像素精密边缘轮廓
		EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		FitCircleContourXld(ho_Edges, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		cam0_4_220_95Chamfer = hv_Radius.D();
		cout << "cam0_4_220_95Chamfer():" << cam0_4_220_95Chamfer << " " << cam0_4_220_95Chamfer * 0.01216 << endl;
		resultVectorList.push_back(cam0_4_220_95Chamfer * Calik);
		pushback_vectors(resultVectorList, 2, 48, 4, -3, +3);
		//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
		//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
		if (HDevWindowStack::IsOpen())
			SetColor(HDevWindowStack::GetActive(), "green");
		if (HDevWindowStack::IsOpen())
			DispObj(ho_Edges, HDevWindowStack::GetActive());

	};


	//"cam0_4-460-177.sbm"
	//cam0_4_220_177_TLineFP_SizeAngle()
	double cam0_4_220_177_TLineFP_SizeAngle[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/program0/cam0_4-460-177.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = 110;
		hv_Line[1] = -94;
		hv_Line[2] = -69;
		hv_Line[3] = 84;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 40, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.5,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
		//显示形状特征模板匹配结果
		//  
			//  
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

		//(colEnd-colBegin)*0.01216= (3595.47-3462.84)*0.01216= 132.63*0.01216 =1.61278 (1.5+0.3-0)
		//(rowBegin-rowEnd)*0.01216= (1172.84-1040.47)*0.01216 = 132.37*0.01216 =1.609619 (1.5+0.3-0)
		//求角度
		//arctan((rowBegin-rowEnd)/(colEnd-colBegin))=arctan(132.37/132.63)=arctan(0.9980396592)= 44.94378529 (45+-5)
		cam0_4_220_177_TLineFP_SizeAngle[0] = hv_RowBegin[0].D();
		cam0_4_220_177_TLineFP_SizeAngle[1] = hv_ColBegin[0].D();
		cam0_4_220_177_TLineFP_SizeAngle[2] = hv_RowEnd[0].D();
		cam0_4_220_177_TLineFP_SizeAngle[3] = hv_ColEnd[0].D();
		cout << " cam0_4_220_177_TLineFP_SizeAngle():" << cam0_4_220_177_TLineFP_SizeAngle[0] << "  " << cam0_4_220_177_TLineFP_SizeAngle[1] << "  " << cam0_4_220_177_TLineFP_SizeAngle[2] << "  " << cam0_4_220_177_TLineFP_SizeAngle[3] << endl;

	};

	//cam0_4_220_177_12_TLineFP()
	double cam0_4_220_177_12_TLineFP[4];
	Straight_TLineFP(cam0_4_220_177_12_TLineFP, imgPath,
		828, 3615, 1041, 3615,
		50, 12, 1, 50);

	//以下为各个特征测量结果处理算法

	Row_61right = (cam0_4_220_43WidthLow_TLineFP[2] + cam0_4_220_43WidthLow_TLineFP[0]) / 2;

	double AxialD_73 = (cam0_4_220_84Left_TLineFP[0] + cam0_4_220_84Left_TLineFP[2]) / 2 - Row_61right;
	AxialD_73 = AxialD_73 * Calik;
	resultVectorList.push_back(AxialD_73);
	pushback_vectors(resultVectorList, 4, 14, 6.92, -0.1, +0.1);

	double AxialD_84 = cam0_4_220_84Right_95Left_TLineFP[2] - (cam0_4_220_84Left_TLineFP[0] + cam0_4_220_84Left_TLineFP[2]) / 2;
	AxialD_84 = AxialD_84 * Calik;
	resultVectorList.push_back(AxialD_84);
	pushback_vectors(resultVectorList, 4, 13, 5, -0.1, +0.1);

	double AxialD_177_1 = cam0_4_220_177_12_TLineFP[1] - cam0_4_220_43WidthLow_TLineFP[3];
	AxialD_177_1 = AxialD_177_1 * Calik;
	double AxialD_177_2 = cam0_4_220_43WidthLow_TLineFP[2] - cam0_4_220_177_12_TLineFP[2];
	AxialD_177_2 = AxialD_177_2 * Calik;
	double AxialD_177 = (AxialD_177_1 + AxialD_177_1) / 2;
	resultVectorList.push_back(AxialD_177);
	pushback_vectors(resultVectorList, 2, 25, 1.8, 0, +0.2);

	//cam0_4_220_177_12_TLineFP()
	double cam0_4_220_177_12angle_TLineFP[4];
	Straight_TLineFP(cam0_4_220_177_12angle_TLineFP, imgPath,
		1150, 3497, 1065, 3577,
		50, 12, 1, 50);

	double cam0_4_220_25angle_TLineFP[4];
	Straight_TLineFP(cam0_4_220_25angle_TLineFP, imgPath,
		1146, 3495, 1058, 3582,
		50, 12, 1, 20);
	double AxialD_177_3 = atan((cam0_4_220_25angle_TLineFP[3] - cam0_4_220_25angle_TLineFP[1]) / (cam0_4_220_25angle_TLineFP[0] - cam0_4_220_25angle_TLineFP[2])) * 180 / PI;//col/row
	resultVectorList.push_back(AxialD_177_3);
	pushback_vectors(resultVectorList, 3, 25.1, 45, -45, +45);
};
void program_5::cam0Picture5_algorithm(string imgPath)//远心相机5图像算法
{
	HTuple halconPath = imgPath.c_str();

	//"cam0_5-460-83.sbm"
	//cam0_5_220_83Right_183ChamferEnd_TLineFP()
	double cam0_5_220_83Right_183ChamferEnd_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_5-220-83.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = 50;
		hv_Line[1] = 50;
		hv_Line[2] = 300;
		hv_Line[3] = 50;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.6,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_5_220_83Right_183ChamferEnd_TLineFP[0] = hv_RowBegin[0].D();
		cam0_5_220_83Right_183ChamferEnd_TLineFP[1] = hv_ColBegin[0].D();
		cam0_5_220_83Right_183ChamferEnd_TLineFP[2] = hv_RowEnd[0].D();
		cam0_5_220_83Right_183ChamferEnd_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_5_220_83Right_183ChamferEnd_TLineFP():" << cam0_5_220_83Right_183ChamferEnd_TLineFP[0] << "  " << cam0_5_220_83Right_183ChamferEnd_TLineFP[1] << "  " << cam0_5_220_83Right_183ChamferEnd_TLineFP[2] << "  " << cam0_5_220_83Right_183ChamferEnd_TLineFP[3] << endl;

	};

	//cam0_5_220_181ChamferBegin_TLineFP()
	double cam0_5_220_181ChamferBegin_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_5-220-83.sbm";

		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -380;
		hv_Line[1] = -70;
		hv_Line[2] = -165;
		hv_Line[3] = -70;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.5,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
		//显示形状特征模板匹配结果
		//  
			//  
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
		cam0_5_220_181ChamferBegin_TLineFP[0] = hv_RowBegin[0].D();
		cam0_5_220_181ChamferBegin_TLineFP[1] = hv_ColBegin[0].D();
		cam0_5_220_181ChamferBegin_TLineFP[2] = hv_RowEnd[0].D();
		cam0_5_220_181ChamferBegin_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_5_220_181ChamferBegin_TLineFP():" << cam0_5_220_181ChamferBegin_TLineFP[0] << "  " << cam0_5_220_181ChamferBegin_TLineFP[1] << "  " << cam0_5_220_181ChamferBegin_TLineFP[2] << "  " << cam0_5_220_181ChamferBegin_TLineFP[3] << endl;

	};

	//cam0_5_220_181ChamferEnd_180ChamferBegin_TLineFP()
	double cam0_5_220_181ChamferEnd_180ChamferBegin_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_5-220-83.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -135;
		hv_Line[1] = -50;
		hv_Line[2] = -55;
		hv_Line[3] = -50;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 40, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.6,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_5_220_181ChamferEnd_180ChamferBegin_TLineFP[0] = hv_RowBegin[0].D();
		cam0_5_220_181ChamferEnd_180ChamferBegin_TLineFP[1] = hv_ColBegin[0].D();
		cam0_5_220_181ChamferEnd_180ChamferBegin_TLineFP[2] = hv_RowEnd[0].D();
		cam0_5_220_181ChamferEnd_180ChamferBegin_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_5_220_181ChamferEnd_180ChamferBegin_TLineFP():" << cam0_5_220_181ChamferEnd_180ChamferBegin_TLineFP[0] << "  " << cam0_5_220_181ChamferEnd_180ChamferBegin_TLineFP[1] << "  " << cam0_5_220_181ChamferEnd_180ChamferBegin_TLineFP[2] << "  " << cam0_5_220_181ChamferEnd_180ChamferBegin_TLineFP[3] << endl;

	};

	//下方函数传递
	//cam0_5_220_181Chamfer()
	double cam0_5_220_181Chamfer;
	{

		// Local iconic variables
		HObject  ho_Image, ho_Rectangle, ho_ImageReduced;
		HObject  ho_Edges;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_Row, hv_Column;
		HTuple  hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;

		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		//dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
		//
		//Segment a region containing the edges
		//基于全局阈值的图像快速阈值化
		//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin 500与colend 3500可固定不变
		GenRectangle1(&ho_Rectangle, cam0_5_220_181ChamferBegin_TLineFP[2], 500, cam0_5_220_181ChamferEnd_180ChamferBegin_TLineFP[0], 3500);
		//提取roi区域，roi以外的变黑，不改变整个图像的尺寸大小。corp_domain是只提取roi区域
		ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);

		//In the subdomain of the image containing the edges,
		//extract subpixel precise edges.
		//提取亚像素精密边缘轮廓
		EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		FitCircleContourXld(ho_Edges, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		cam0_5_220_181Chamfer = hv_Radius.D();
		cout << "cam0_5_220_181Chamfer():" << cam0_5_220_181Chamfer << " " << cam0_5_220_181Chamfer * 0.01216 << endl;
		resultVectorList.push_back(cam0_5_220_181Chamfer * Calik);
		pushback_vectors(resultVectorList, 2, 52, 4, -3, +3);
		//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
		//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
		if (HDevWindowStack::IsOpen())
			SetColor(HDevWindowStack::GetActive(), "green");
		if (HDevWindowStack::IsOpen())
			DispObj(ho_Edges, HDevWindowStack::GetActive());

	};

	//cam0_5_220_180ChamferEnd_183ChamferBegin_TLineFP()
	double cam0_5_220_180ChamferEnd_183ChamferBegin_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_5-220-83.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -30;
		hv_Line[1] = -30;
		hv_Line[2] = 74;
		hv_Line[3] = 50;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 40, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.5,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_5_220_180ChamferEnd_183ChamferBegin_TLineFP[0] = hv_RowBegin[0].D();
		cam0_5_220_180ChamferEnd_183ChamferBegin_TLineFP[1] = hv_ColBegin[0].D();
		cam0_5_220_180ChamferEnd_183ChamferBegin_TLineFP[2] = hv_RowEnd[0].D();
		cam0_5_220_180ChamferEnd_183ChamferBegin_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_5_220_180ChamferEnd_183ChamferBegin_TLineFP():" << cam0_5_220_180ChamferEnd_183ChamferBegin_TLineFP[0] << "  " << cam0_5_220_180ChamferEnd_183ChamferBegin_TLineFP[1] << "  " << cam0_5_220_180ChamferEnd_183ChamferBegin_TLineFP[2] << "  " << cam0_5_220_180ChamferEnd_183ChamferBegin_TLineFP[3] << endl;

	};

	//下方函数传递
	//cam0_5_220_180Chamfer()
	double cam0_5_220_180Chamfer;
	{

		// Local iconic variables
		HObject  ho_Image, ho_Rectangle, ho_ImageReduced;
		HObject  ho_Edges;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_Row, hv_Column;
		HTuple  hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;

		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		//dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
		//
		//Segment a region containing the edges
		//基于全局阈值的图像快速阈值化
		//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin 500与colend 3500可固定不变
		GenRectangle1(&ho_Rectangle, cam0_5_220_181ChamferEnd_180ChamferBegin_TLineFP[2], 500, cam0_5_220_180ChamferEnd_183ChamferBegin_TLineFP[0], 3500);
		//提取roi区域，roi以外的变黑，不改变整个图像的尺寸大小。corp_domain是只提取roi区域
		ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);

		//In the subdomain of the image containing the edges,
		//extract subpixel precise edges.
		//提取亚像素精密边缘轮廓
		EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		FitCircleContourXld(ho_Edges, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		cam0_5_220_180Chamfer = hv_Radius.D();
		cout << "cam0_5_220_180Chamfer():" << cam0_5_220_180Chamfer << " " << cam0_5_220_180Chamfer * 0.01216 << endl;
		resultVectorList.push_back(cam0_5_220_180Chamfer * Calik);
		pushback_vectors(resultVectorList, 2, 56, 2, -0.5, +0.5);
		//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
		//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
		if (HDevWindowStack::IsOpen())
			SetColor(HDevWindowStack::GetActive(), "green");
		if (HDevWindowStack::IsOpen())
			DispObj(ho_Edges, HDevWindowStack::GetActive());

	};

	//下方函数传递
//cam0_5_220_183Chamfer()
	double cam0_5_220_183Chamfer;
	{

		// Local iconic variables
		HObject  ho_Image, ho_Rectangle, ho_ImageReduced;
		HObject  ho_Edges;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_Row, hv_Column;
		HTuple  hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;

		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		//dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
		//
		//Segment a region containing the edges
		//基于全局阈值的图像快速阈值化
		//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin 500与colend 3500可固定不变
		GenRectangle1(&ho_Rectangle, cam0_5_220_180ChamferEnd_183ChamferBegin_TLineFP[2], 500, cam0_5_220_83Right_183ChamferEnd_TLineFP[0], 3500);
		//提取roi区域，roi以外的变黑，不改变整个图像的尺寸大小。corp_domain是只提取roi区域
		ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);

		//In the subdomain of the image containing the edges,
		//extract subpixel precise edges.
		//提取亚像素精密边缘轮廓
		EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		FitCircleContourXld(ho_Edges, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		cam0_5_220_183Chamfer = hv_Radius.D();
		cout << "cam0_5_220_183Chamfer():" << cam0_5_220_183Chamfer << " " << cam0_5_220_183Chamfer * 0.01216 << endl;
		resultVectorList.push_back(cam0_5_220_183Chamfer * Calik);
		pushback_vectors(resultVectorList, 2, 2, 0.6, -0.3, +0.3);
		//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
		//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
		if (HDevWindowStack::IsOpen())
			SetColor(HDevWindowStack::GetActive(), "green");
		if (HDevWindowStack::IsOpen())
			DispObj(ho_Edges, HDevWindowStack::GetActive());

	};

	//"cam0_5-460-185.sbm"
	//cam0_5_220_186Right_185LeftChamferBegin_TLineFP()
	double cam0_5_220_186Right_185LeftChamferBegin_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_5-220-185.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -400;
		hv_Line[1] = 70;
		hv_Line[2] = -100;
		hv_Line[3] = 70;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 60,
			HTuple(), HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.6,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_5_220_186Right_185LeftChamferBegin_TLineFP[0] = hv_RowBegin[0].D();
		cam0_5_220_186Right_185LeftChamferBegin_TLineFP[1] = hv_ColBegin[0].D();
		cam0_5_220_186Right_185LeftChamferBegin_TLineFP[2] = hv_RowEnd[0].D();
		cam0_5_220_186Right_185LeftChamferBegin_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_5_220_186Right_185LeftChamferBegin_TLineFP():" << cam0_5_220_186Right_185LeftChamferBegin_TLineFP[0] << "  " << cam0_5_220_186Right_185LeftChamferBegin_TLineFP[1] << "  " << cam0_5_220_186Right_185LeftChamferBegin_TLineFP[2] << "  " << cam0_5_220_186Right_185LeftChamferBegin_TLineFP[3] << endl;

	};

	//cam0_5_220_185LeftChamferEnd_RightChamferBegin_TLineFP()
	double cam0_5_220_185LeftChamferEnd_RightChamferBegin_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_5-220-185.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -50;
		hv_Line[1] = 50;
		hv_Line[2] = 50;
		hv_Line[3] = -30;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 40, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.6,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_5_220_185LeftChamferEnd_RightChamferBegin_TLineFP[0] = hv_RowBegin[0].D();
		cam0_5_220_185LeftChamferEnd_RightChamferBegin_TLineFP[1] = hv_ColBegin[0].D();
		cam0_5_220_185LeftChamferEnd_RightChamferBegin_TLineFP[2] = hv_RowEnd[0].D();
		cam0_5_220_185LeftChamferEnd_RightChamferBegin_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_5_220_185LeftChamferEnd_RightChamferBegin_TLineFP():" << cam0_5_220_185LeftChamferEnd_RightChamferBegin_TLineFP[0] << "  " << cam0_5_220_185LeftChamferEnd_RightChamferBegin_TLineFP[1] << "  " << cam0_5_220_185LeftChamferEnd_RightChamferBegin_TLineFP[2] << "  " << cam0_5_220_185LeftChamferEnd_RightChamferBegin_TLineFP[3] << endl;

	};

	//下方函数传递
	//cam0_5_220_185LeftChamfer()
	double cam0_5_220_185LeftChamfer;
	{

		// Local iconic variables
		HObject  ho_Image, ho_Rectangle, ho_ImageReduced;
		HObject  ho_Edges;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_Row, hv_Column;
		HTuple  hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;

		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		//dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
		//
		//Segment a region containing the edges
		//基于全局阈值的图像快速阈值化
		//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin 500与colend 3500可固定不变
		GenRectangle1(&ho_Rectangle, cam0_5_220_186Right_185LeftChamferBegin_TLineFP[2], 500, cam0_5_220_185LeftChamferEnd_RightChamferBegin_TLineFP[0], 3500);
		//提取roi区域，roi以外的变黑，不改变整个图像的尺寸大小。corp_domain是只提取roi区域
		ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);

		//In the subdomain of the image containing the edges,
		//extract subpixel precise edges.
		//提取亚像素精密边缘轮廓
		EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		FitCircleContourXld(ho_Edges, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		cam0_5_220_185LeftChamfer = hv_Radius.D();
		cout << "cam0_5_220_185LeftChamfer():" << cam0_5_220_185LeftChamfer << " " << cam0_5_220_185LeftChamfer * 0.01216 << endl;
		resultVectorList.push_back(cam0_5_220_185LeftChamfer * Calik);
		pushback_vectors(resultVectorList, 2, 3, 4, -3, +3);
		//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
		//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
		if (HDevWindowStack::IsOpen())
			SetColor(HDevWindowStack::GetActive(), "green");
		if (HDevWindowStack::IsOpen())
			DispObj(ho_Edges, HDevWindowStack::GetActive());

	};

	//cam0_5_220_185RightChamferEnd_TLineFP()
	double cam0_5_220_185RightChamferEnd_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_5-220-185.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = 140;
		hv_Line[1] = -50;
		hv_Line[2] = 350;
		hv_Line[3] = -50;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 60, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.6,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_5_220_185RightChamferEnd_TLineFP[0] = hv_RowBegin[0].D();
		cam0_5_220_185RightChamferEnd_TLineFP[1] = hv_ColBegin[0].D();
		cam0_5_220_185RightChamferEnd_TLineFP[2] = hv_RowEnd[0].D();
		cam0_5_220_185RightChamferEnd_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_5_220_185RightChamferEnd_TLineFP():" << cam0_5_220_185RightChamferEnd_TLineFP[0] << "  " << cam0_5_220_185RightChamferEnd_TLineFP[1] << "  " << cam0_5_220_185RightChamferEnd_TLineFP[2] << "  " << cam0_5_220_185RightChamferEnd_TLineFP[3] << endl;
	};

	//下方函数传递
	//cam0_5_220_185RightChamfer()
	double cam0_5_220_185RightChamfer;
	{

		// Local iconic variables
		HObject  ho_Image, ho_Rectangle, ho_ImageReduced;
		HObject  ho_Edges;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_Row, hv_Column;
		HTuple  hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;

		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		//dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
		//
		//Segment a region containing the edges
		//基于全局阈值的图像快速阈值化
		//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin 500与colend 3500可固定不变
		GenRectangle1(&ho_Rectangle, cam0_5_220_185LeftChamferEnd_RightChamferBegin_TLineFP[2], 500, cam0_5_220_185RightChamferEnd_TLineFP[0], 3500);
		//提取roi区域，roi以外的变黑，不改变整个图像的尺寸大小。corp_domain是只提取roi区域
		ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);

		//In the subdomain of the image containing the edges,
		//extract subpixel precise edges.
		//提取亚像素精密边缘轮廓
		EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		FitCircleContourXld(ho_Edges, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		cam0_5_220_185RightChamfer = hv_Radius.D();
		cout << "cam0_5_220_185RightChamfer():" << cam0_5_220_185RightChamfer << " " << cam0_5_220_185RightChamfer * 0.01216 << endl;
		resultVectorList.push_back(cam0_5_220_185RightChamfer * Calik);
		pushback_vectors(resultVectorList, 2, 57, 4, -3, +3);
		//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
		//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
		if (HDevWindowStack::IsOpen())
			SetColor(HDevWindowStack::GetActive(), "green");
		if (HDevWindowStack::IsOpen())
			DispObj(ho_Edges, HDevWindowStack::GetActive());

	};

	Row_186left = cam0_5_220_83Right_183ChamferEnd_TLineFP[0];
	double AxialD_83 = Row_186left - Row_61right;
	AxialD_83 = AxialD_83 * Calik + pic4To5_moveDistance;
	//resultVectorList.push_back(AxialD_83);
	//pushback_vectors(resultVectorList, 4, 4, 114.4, -0.3, +0.3);

	double AxialD_186 = cam0_5_220_185RightChamferEnd_TLineFP[0] - cam0_5_220_181ChamferEnd_180ChamferBegin_TLineFP[0];
	AxialD_186 = AxialD_186 * Calik;
	//resultVectorList.push_back(AxialD_186);
	//pushback_vectors(resultVectorList, 4, 50, 18.07, -0.5, +0.5);

	double AxialD_179 = cam0_5_220_180ChamferEnd_183ChamferBegin_TLineFP[0] - 27 - cam0_5_220_181ChamferEnd_180ChamferBegin_TLineFP[0];
	AxialD_179 = AxialD_179 * Calik;
	//resultVectorList.push_back(AxialD_179);
	//pushback_vectors(resultVectorList, 4, 51, 0.91, -0.1, +0.1);

	double cam0_5_54angle_TLineFP[4];
	Straight_TLineFP(cam0_5_54angle_TLineFP, imgPath,
		925, 1327, 972, 1370,
		50, 12, 1, 20);
	double AxialD_184_1 = atan((cam0_5_54angle_TLineFP[3] - cam0_5_54angle_TLineFP[1]) / (cam0_5_54angle_TLineFP[2] - cam0_5_54angle_TLineFP[0])) * 180 / PI;
	resultVectorList.push_back(AxialD_184_1);
	pushback_vectors(resultVectorList, 3, 54, 40, -1, +1);

	double cam0_5_55angle_TLineFP[4];
	Straight_TLineFP(cam0_5_55angle_TLineFP, imgPath,
		2265, 1305, 2196, 1350,
		30, 12, 1, 10);
	double AxialD_184_2 = atan((cam0_5_55angle_TLineFP[3] - cam0_5_55angle_TLineFP[1]) / (cam0_5_55angle_TLineFP[0] - cam0_5_55angle_TLineFP[2])) * 180 / PI;
	resultVectorList.push_back(AxialD_184_2);
	pushback_vectors(resultVectorList, 3, 55, 40, -1, +1);
};
void program_5::cam0Picture6_algorithm(string imgPath)//远心相机6图像算法
{
	HTuple halconPath = imgPath.c_str();

	//"cam0_6-460-82.sbm"
	//cam0_6_220_82Right_187Left_189Right_194LeftChamferEnd_TLineFP()
	double cam0_6_220_82Right_187Left_189Right_194LeftChamferEnd_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_6-220-82.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = 50;
		hv_Line[1] = 50;
		hv_Line[2] = 300;
		hv_Line[3] = 50;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 40, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.6,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_6_220_82Right_187Left_189Right_194LeftChamferEnd_TLineFP[0] = hv_RowBegin[0].D();
		cam0_6_220_82Right_187Left_189Right_194LeftChamferEnd_TLineFP[1] = hv_ColBegin[0].D();
		cam0_6_220_82Right_187Left_189Right_194LeftChamferEnd_TLineFP[2] = hv_RowEnd[0].D();
		cam0_6_220_82Right_187Left_189Right_194LeftChamferEnd_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_6_220_82Right_187Left_189Right_194LeftChamferEnd_TLineFP():" << cam0_6_220_82Right_187Left_189Right_194LeftChamferEnd_TLineFP[0] << "  " << cam0_6_220_82Right_187Left_189Right_194LeftChamferEnd_TLineFP[1] << "  " << cam0_6_220_82Right_187Left_189Right_194LeftChamferEnd_TLineFP[2] << "  " << cam0_6_220_82Right_187Left_189Right_194LeftChamferEnd_TLineFP[3] << endl;

	};

	//cam0_6_220_190ChamferBegin_TLineFP()
	double cam0_6_220_190ChamferBegin_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_6-220-82.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -300;
		hv_Line[1] = -20;
		hv_Line[2] = -110;
		hv_Line[3] = -20;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50,
			HTuple(), HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.6,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_6_220_190ChamferBegin_TLineFP[0] = hv_RowBegin[0].D();
		cam0_6_220_190ChamferBegin_TLineFP[1] = hv_ColBegin[0].D();
		cam0_6_220_190ChamferBegin_TLineFP[2] = hv_RowEnd[0].D();
		cam0_6_220_190ChamferBegin_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_6_220_190ChamferBegin_TLineFP():" << cam0_6_220_190ChamferBegin_TLineFP[0] << "  " << cam0_6_220_190ChamferBegin_TLineFP[1] << "  " << cam0_6_220_190ChamferBegin_TLineFP[2] << "  " << cam0_6_220_190ChamferBegin_TLineFP[3] << endl;

	};

	//cam0_6_220_190ChamferEnd_188ChamferBegin_TLineFP()
	double cam0_6_220_190ChamferEnd_188ChamferBegin_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_6-220-82.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -50;
		hv_Line[1] = 0;
		hv_Line[2] = 40;
		hv_Line[3] = 0;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 30, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.6,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_6_220_190ChamferEnd_188ChamferBegin_TLineFP[0] = hv_RowBegin[0].D();
		cam0_6_220_190ChamferEnd_188ChamferBegin_TLineFP[1] = hv_ColBegin[0].D();
		cam0_6_220_190ChamferEnd_188ChamferBegin_TLineFP[2] = hv_RowEnd[0].D();
		cam0_6_220_190ChamferEnd_188ChamferBegin_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_6_220_190ChamferEnd_188ChamferBegin_TLineFP():" << cam0_6_220_190ChamferEnd_188ChamferBegin_TLineFP[0] << "  " << cam0_6_220_190ChamferEnd_188ChamferBegin_TLineFP[1] << "  " << cam0_6_220_190ChamferEnd_188ChamferBegin_TLineFP[2] << "  " << cam0_6_220_190ChamferEnd_188ChamferBegin_TLineFP[3] << endl;

	};

	//以下函数传递
	//cam0_6_220_190Chamfer()
	double cam0_6_220_190Chamfer;
	{

		// Local iconic variables
		HObject  ho_Image, ho_Rectangle, ho_ImageReduced;
		HObject  ho_Edges;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_Row, hv_Column;
		HTuple  hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;

		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		//dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
		//
		//Segment a region containing the edges
		//基于全局阈值的图像快速阈值化
		//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin 500与colend 3500可固定不变
		GenRectangle1(&ho_Rectangle, cam0_6_220_190ChamferBegin_TLineFP[2], 500, cam0_6_220_190ChamferEnd_188ChamferBegin_TLineFP[0] - 15, 3500);
		//提取roi区域，roi以外的变黑，不改变整个图像的尺寸大小。corp_domain是只提取roi区域
		ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);

		//In the subdomain of the image containing the edges,
		//extract subpixel precise edges.
		//提取亚像素精密边缘轮廓
		EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		FitCircleContourXld(ho_Edges, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		cam0_6_220_190Chamfer = hv_Radius.D();
		cout << "cam0_6_220_190Chamfer():" << cam0_6_220_190Chamfer << " " << cam0_6_220_190Chamfer * 0.01216 << endl;
		resultVectorList.push_back(cam0_6_220_190Chamfer * Calik);
		pushback_vectors(resultVectorList, 2, 62, 4, -3, +3);
		//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
		//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
		if (HDevWindowStack::IsOpen())
			SetColor(HDevWindowStack::GetActive(), "green");
		if (HDevWindowStack::IsOpen())
			DispObj(ho_Edges, HDevWindowStack::GetActive());

	};

	//cam0_6_220_188ChamferEnd_194LeftChamferBegin_TLineFP()
	double cam0_6_220_188ChamferEnd_194LeftChamferBegin_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_6-220-82.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = 40;
		hv_Line[1] = 0;
		hv_Line[2] = 80;
		hv_Line[3] = 35;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 40, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.6,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_6_220_188ChamferEnd_194LeftChamferBegin_TLineFP[0] = hv_RowBegin[0].D();
		cam0_6_220_188ChamferEnd_194LeftChamferBegin_TLineFP[1] = hv_ColBegin[0].D();
		cam0_6_220_188ChamferEnd_194LeftChamferBegin_TLineFP[2] = hv_RowEnd[0].D();
		cam0_6_220_188ChamferEnd_194LeftChamferBegin_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_6_220_188ChamferEnd_194LeftChamferBegin_TLineFP():" << cam0_6_220_188ChamferEnd_194LeftChamferBegin_TLineFP[0] << "  " << cam0_6_220_188ChamferEnd_194LeftChamferBegin_TLineFP[1] << "  " << cam0_6_220_188ChamferEnd_194LeftChamferBegin_TLineFP[2] << "  " << cam0_6_220_188ChamferEnd_194LeftChamferBegin_TLineFP[3] << endl;

	};

	//以下函数传递
	//cam0_6_220_188Chamfer()
	double cam0_6_220_188Chamfer;
	{

		// Local iconic variables
		HObject  ho_Image, ho_Rectangle, ho_ImageReduced;
		HObject  ho_Edges;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_Row, hv_Column;
		HTuple  hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;

		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		//dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
		//
		//Segment a region containing the edges
		//基于全局阈值的图像快速阈值化
		//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin 500与colend 3500可固定不变
		GenRectangle1(&ho_Rectangle, cam0_6_220_190ChamferEnd_188ChamferBegin_TLineFP[2], 500, cam0_6_220_188ChamferEnd_194LeftChamferBegin_TLineFP[2], 3500);
		//提取roi区域，roi以外的变黑，不改变整个图像的尺寸大小。corp_domain是只提取roi区域
		ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);

		//In the subdomain of the image containing the edges,
		//extract subpixel precise edges.
		//提取亚像素精密边缘轮廓
		EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		FitCircleContourXld(ho_Edges, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		cam0_6_220_188Chamfer = hv_Radius.D();
		cout << "cam0_6_220_188Chamfer():" << cam0_6_220_188Chamfer << " " << cam0_6_220_188Chamfer * 0.01216 << endl;
		resultVectorList.push_back(cam0_6_220_188Chamfer * Calik);
		pushback_vectors(resultVectorList, 2, 59, 1.3, -0.5, +0.5);
		//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
		//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
		if (HDevWindowStack::IsOpen())
			SetColor(HDevWindowStack::GetActive(), "green");
		if (HDevWindowStack::IsOpen())
			DispObj(ho_Edges, HDevWindowStack::GetActive());

	};

	//cam0_6_220_194LeftChamferEnd_187Left_TLineFP()
	double cam0_6_220_194LeftChamferEnd_187Left_TLineFP[4];
	Straight_TLineFP(cam0_6_220_194LeftChamferEnd_187Left_TLineFP, imgPath,
		949, 1332, 1100, 1332,
		50, 12, 1, 20);

	//"cam0_6-460-194右倒角.sbm"
	//cam0_6_220_194RightChamferBegin_187Right_TLineFP()
	double cam0_6_220_194RightChamferBegin_187Right_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_6-220-194右倒角.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -400;
		hv_Line[1] = 30;
		hv_Line[2] = -80;
		hv_Line[3] = 30;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 40, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.6,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_6_220_194RightChamferBegin_187Right_TLineFP[0] = hv_RowBegin[0].D();
		cam0_6_220_194RightChamferBegin_187Right_TLineFP[1] = hv_ColBegin[0].D();
		cam0_6_220_194RightChamferBegin_187Right_TLineFP[2] = hv_RowEnd[0].D();
		cam0_6_220_194RightChamferBegin_187Right_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_6_220_194RightChamferBegin_187Right_TLineFP():" << cam0_6_220_194RightChamferBegin_187Right_TLineFP[0] << "  " << cam0_6_220_194RightChamferBegin_187Right_TLineFP[1] << "  " << cam0_6_220_194RightChamferBegin_187Right_TLineFP[2] << "  " << cam0_6_220_194RightChamferBegin_187Right_TLineFP[3] << endl;

	};

	//cam0_6_220_194RightChamferEnd_191ChamferBegin_TLineFP()
	double cam0_6_220_194RightChamferEnd_191ChamferBegin_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_6-220-194右倒角.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -110;
		hv_Line[1] = 40;
		hv_Line[2] = -60;
		hv_Line[3] = -10;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 10, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.8,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_6_220_194RightChamferEnd_191ChamferBegin_TLineFP[0] = hv_RowBegin[0].D();
		cam0_6_220_194RightChamferEnd_191ChamferBegin_TLineFP[1] = hv_ColBegin[0].D();
		cam0_6_220_194RightChamferEnd_191ChamferBegin_TLineFP[2] = hv_RowEnd[0].D();
		cam0_6_220_194RightChamferEnd_191ChamferBegin_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_6_220_194RightChamferEnd_191ChamferBegin_TLineFP():" << cam0_6_220_194RightChamferEnd_191ChamferBegin_TLineFP[0] << "  " << cam0_6_220_194RightChamferEnd_191ChamferBegin_TLineFP[1] << "  " << cam0_6_220_194RightChamferEnd_191ChamferBegin_TLineFP[2] << "  " << cam0_6_220_194RightChamferEnd_191ChamferBegin_TLineFP[3] << endl;

	};


	//cam0_6_220_191ChamferEnd_192ChamferBegin_TLineFP()
	double cam0_6_220_191ChamferEnd_192ChamferBegin_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_6-220-194右倒角.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -60;
		hv_Line[1] = 0;
		hv_Line[2] = 50;
		hv_Line[3] = 0;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 40, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.6,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_6_220_191ChamferEnd_192ChamferBegin_TLineFP[0] = hv_RowBegin[0].D();
		cam0_6_220_191ChamferEnd_192ChamferBegin_TLineFP[1] = hv_ColBegin[0].D();
		cam0_6_220_191ChamferEnd_192ChamferBegin_TLineFP[2] = hv_RowEnd[0].D();
		cam0_6_220_191ChamferEnd_192ChamferBegin_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_6_220_191ChamferEnd_192ChamferBegin_TLineFP():" << cam0_6_220_191ChamferEnd_192ChamferBegin_TLineFP[0] << "  " << cam0_6_220_191ChamferEnd_192ChamferBegin_TLineFP[1] << "  " << cam0_6_220_191ChamferEnd_192ChamferBegin_TLineFP[2] << "  " << cam0_6_220_191ChamferEnd_192ChamferBegin_TLineFP[3] << endl;

	};

	//以下函数传递
	//cam0_6_220_191Chamfer()
	double cam0_6_220_191Chamfer;
	{

		// Local iconic variables
		HObject  ho_Image, ho_Rectangle, ho_ImageReduced;
		HObject  ho_Edges;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_Row, hv_Column;
		HTuple  hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;

		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		//dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
		//
		//Segment a region containing the edges
		//基于全局阈值的图像快速阈值化
		//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin 500与colend 3500可固定不变
		GenRectangle1(&ho_Rectangle, cam0_6_220_194RightChamferEnd_191ChamferBegin_TLineFP[0] + 10, 500, cam0_6_220_191ChamferEnd_192ChamferBegin_TLineFP[0], 3500);
		//提取roi区域，roi以外的变黑，不改变整个图像的尺寸大小。corp_domain是只提取roi区域
		ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);

		//In the subdomain of the image containing the edges,
		//extract subpixel precise edges.
		//提取亚像素精密边缘轮廓
		EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		FitCircleContourXld(ho_Edges, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		cam0_6_220_191Chamfer = hv_Radius.D();
		cout << "cam0_6_220_191Chamfer():" << cam0_6_220_191Chamfer << " " << cam0_6_220_191Chamfer * 0.01216 << endl;
		resultVectorList.push_back(cam0_6_220_191Chamfer * Calik);
		pushback_vectors(resultVectorList, 2, 63, 1.2, -0.5, +0.5);
		//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
		//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
		if (HDevWindowStack::IsOpen())
			SetColor(HDevWindowStack::GetActive(), "green");
		if (HDevWindowStack::IsOpen())
			DispObj(ho_Edges, HDevWindowStack::GetActive());

	};

	//cam0_6_220_192ChamferEnd_TLineFP()
	double cam0_6_220_192ChamferEnd_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_6-220-194右倒角.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = 100;
		hv_Line[1] = -10;
		hv_Line[2] = 320;
		hv_Line[3] = -10;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50,
			HTuple(), HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.6,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
		//显示形状特征模板匹配结果
		//  
			//  
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
		cam0_6_220_192ChamferEnd_TLineFP[0] = hv_RowBegin[0].D();
		cam0_6_220_192ChamferEnd_TLineFP[1] = hv_ColBegin[0].D();
		cam0_6_220_192ChamferEnd_TLineFP[2] = hv_RowEnd[0].D();
		cam0_6_220_192ChamferEnd_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_6_220_192ChamferEnd_TLineFP():" << cam0_6_220_192ChamferEnd_TLineFP[0] << "  " << cam0_6_220_192ChamferEnd_TLineFP[1] << "  " << cam0_6_220_192ChamferEnd_TLineFP[2] << "  " << cam0_6_220_192ChamferEnd_TLineFP[3] << endl;

	};

	//以下函数传递
	//cam0_6_220_192Chamfer()
	double cam0_6_220_192Chamfer;
	{

		// Local iconic variables
		HObject  ho_Image, ho_Rectangle, ho_ImageReduced;
		HObject  ho_Edges;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_Row, hv_Column;
		HTuple  hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;

		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		//dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
		//
		//Segment a region containing the edges
		//基于全局阈值的图像快速阈值化
		//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin 500与colend 3500可固定不变
		GenRectangle1(&ho_Rectangle, cam0_6_220_191ChamferEnd_192ChamferBegin_TLineFP[2], 500, cam0_6_220_192ChamferEnd_TLineFP[0], 3500);
		//提取roi区域，roi以外的变黑，不改变整个图像的尺寸大小。corp_domain是只提取roi区域
		ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);

		//In the subdomain of the image containing the edges,
		//extract subpixel precise edges.
		//提取亚像素精密边缘轮廓
		EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		FitCircleContourXld(ho_Edges, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		cam0_6_220_192Chamfer = hv_Radius.D();
		cout << "cam0_6_220_192Chamfer():" << cam0_6_220_192Chamfer << " " << cam0_6_220_192Chamfer * 0.01216 << endl;
		resultVectorList.push_back(cam0_6_220_192Chamfer * Calik);
		pushback_vectors(resultVectorList, 2, 64, 4, -3, +3);
		//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
		//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
		if (HDevWindowStack::IsOpen())
			SetColor(HDevWindowStack::GetActive(), "green");
		if (HDevWindowStack::IsOpen())
			DispObj(ho_Edges, HDevWindowStack::GetActive());

	};

	//以下为各个特征测量结果处理算法
	double AxialD_187 = cam0_6_220_194RightChamferBegin_187Right_TLineFP[2] - cam0_6_220_194LeftChamferEnd_187Left_TLineFP[0];
	AxialD_187 = AxialD_187 * Calik;
	resultVectorList.push_back(AxialD_187);
	pushback_vectors(resultVectorList, 4, 58, 13.45, -0.1, +0.1);

	double AxialD_82 = cam0_6_220_194LeftChamferEnd_187Left_TLineFP[0] - Row_61right;
	AxialD_82 = AxialD_82 * Calik + pic4To5_moveDistance + pic5To6_moveDistance;
	resultVectorList.push_back(AxialD_82);
	pushback_vectors(resultVectorList, 4, 72, 240.9, -0.1, +0.1);

	double AxialD_189_1 = cam0_6_220_194LeftChamferEnd_187Left_TLineFP[0] - cam0_6_220_190ChamferEnd_188ChamferBegin_TLineFP[0];
	AxialD_189_1 = AxialD_189_1 * Calik;
	resultVectorList.push_back(AxialD_189_1);

	double AxialD_189_2 = cam0_6_220_191ChamferEnd_192ChamferBegin_TLineFP[2] - 8 - cam0_6_220_194RightChamferBegin_187Right_TLineFP[2];
	AxialD_189_2 = AxialD_189_2 * Calik;
	resultVectorList.push_back(AxialD_189_2);
	pushback_vectors(resultVectorList, 4, 61, 1.55, -0.21, +0.21);

};
void program_5::cam0Picture7_algorithm(string imgPath)//远心相机7图像算法
{
	HTuple halconPath = imgPath.c_str();

	//"cam0_7-460-81.sbm"
	//cam0_7_220_81Right_TLineFP()
	double cam0_7_220_81Right_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_7-220-81.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = 6;
		hv_Line[1] = 20;
		hv_Line[2] = 150;
		hv_Line[3] = 20;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.6,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
		//显示形状特征模板匹配结果
		//  
			//  
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
		cam0_7_220_81Right_TLineFP[0] = hv_RowBegin[0].D();
		cam0_7_220_81Right_TLineFP[1] = hv_ColBegin[0].D();
		cam0_7_220_81Right_TLineFP[2] = hv_RowEnd[0].D();
		cam0_7_220_81Right_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_7_220_81Right_TLineFP():" << cam0_7_220_81Right_TLineFP[0] << "  " << cam0_7_220_81Right_TLineFP[1] << "  " << cam0_7_220_81Right_TLineFP[2] << "  " << cam0_7_220_81Right_TLineFP[3] << endl;

	};

	//cam0_7_220_87Left_TLineFP()
	double cam0_7_220_87Left_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_7-220-81.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -200;
		hv_Line[1] = 0;
		hv_Line[2] = -60;
		hv_Line[3] = 0;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 60,
			HTuple(), HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.6,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
		//显示形状特征模板匹配结果
		//  
			//  
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
		cam0_7_220_87Left_TLineFP[0] = hv_RowBegin[0].D();
		cam0_7_220_87Left_TLineFP[1] = hv_ColBegin[0].D();
		cam0_7_220_87Left_TLineFP[2] = hv_RowEnd[0].D();
		cam0_7_220_87Left_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_7_220_87Left_TLineFP():" << cam0_7_220_87Left_TLineFP[0] << "  " << cam0_7_220_87Left_TLineFP[1] << "  " << cam0_7_220_87Left_TLineFP[2] << "  " << cam0_7_220_87Left_TLineFP[3] << endl;

	};

	//以下函数传递
	//cam0_7_220_87Chamfer()
	double cam0_7_220_87Chamfer;
	{

		// Local iconic variables
		HObject  ho_Image, ho_Rectangle, ho_ImageReduced;
		HObject  ho_Edges;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_Row, hv_Column;
		HTuple  hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;

		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		//dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
		//
		//Segment a region containing the edges
		//基于全局阈值的图像快速阈值化
		//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin 500与colend 3500可固定不变
		GenRectangle1(&ho_Rectangle, cam0_7_220_87Left_TLineFP[2], 500, cam0_7_220_81Right_TLineFP[0] - 5, 3500);
		//提取roi区域，roi以外的变黑，不改变整个图像的尺寸大小。corp_domain是只提取roi区域
		ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);

		//In the subdomain of the image containing the edges,
		//extract subpixel precise edges.
		//提取亚像素精密边缘轮廓
		EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		FitCircleContourXld(ho_Edges, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		cam0_7_220_87Chamfer = hv_Radius.D();
		cout << "cam0_7_220_87Chamfer():" << cam0_7_220_87Chamfer << " " << cam0_7_220_87Chamfer * 0.01216 << endl;
		//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
		//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
		if (HDevWindowStack::IsOpen())
			SetColor(HDevWindowStack::GetActive(), "green");
		if (HDevWindowStack::IsOpen())
			DispObj(ho_Edges, HDevWindowStack::GetActive());

	};

	//cam0_7_220_88_TLineFP()
	double cam0_7_220_88_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_7-220-81.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = 850;
		hv_Line[1] = 20;
		hv_Line[2] = 1012;
		hv_Line[3] = 20;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.6,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
		//显示形状特征模板匹配结果
		//  
			//  
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
		cam0_7_220_88_TLineFP[0] = hv_RowBegin[0].D();
		cam0_7_220_88_TLineFP[1] = hv_ColBegin[0].D();
		cam0_7_220_88_TLineFP[2] = hv_RowEnd[0].D();
		cam0_7_220_88_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_7_220_88_TLineFP():" << cam0_7_220_88_TLineFP[0] << "  " << cam0_7_220_88_TLineFP[1] << "  " << cam0_7_220_88_TLineFP[2] << "  " << cam0_7_220_88_TLineFP[3] << endl;

	};

	//"cam0_7-460-89.sbm"
	//cam0_7_220_89Right_TLineFP()
	double cam0_7_220_89Right_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_7-220-89.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = 20;
		hv_Line[1] = -10;
		hv_Line[2] = 200;
		hv_Line[3] = -10;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50,
			HTuple(), HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.6,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_7_220_89Right_TLineFP[0] = hv_RowBegin[0].D();
		cam0_7_220_89Right_TLineFP[1] = hv_ColBegin[0].D();
		cam0_7_220_89Right_TLineFP[2] = hv_RowEnd[0].D();
		cam0_7_220_89Right_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_7_220_89Right_TLineFP():" << cam0_7_220_89Right_TLineFP[0] << "  " << cam0_7_220_89Right_TLineFP[1] << "  " << cam0_7_220_89Right_TLineFP[2] << "  " << cam0_7_220_89Right_TLineFP[3] << endl;

	};

	//以下函数传递
	//cam0_7_220_89Chamfer()
	double cam0_7_220_89Chamfer;
	{

		// Local iconic variables
		HObject  ho_Image, ho_Rectangle, ho_ImageReduced;
		HObject  ho_Edges;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_Row, hv_Column;
		HTuple  hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;

		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		//dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
		//
		//Segment a region containing the edges
		//基于全局阈值的图像快速阈值化
		//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin 500与colend 3500可固定不变
		GenRectangle1(&ho_Rectangle, cam0_7_220_88_TLineFP[2], 500, cam0_7_220_89Right_TLineFP[0], 3500);
		//提取roi区域，roi以外的变黑，不改变整个图像的尺寸大小。corp_domain是只提取roi区域
		ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);

		//In the subdomain of the image containing the edges,
		//extract subpixel precise edges.
		//提取亚像素精密边缘轮廓
		EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		FitCircleContourXld(ho_Edges, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		cam0_7_220_89Chamfer = hv_Radius.D();
		cout << "cam0_7_220_89Chamfer():" << cam0_7_220_89Chamfer << " " << cam0_7_220_89Chamfer * 0.01216 << endl;
		resultVectorList.push_back(cam0_7_220_89Chamfer * Calik);
		resultVectorList.push_back(cam0_7_220_87Chamfer * Calik);
		pushback_vectors(resultVectorList, 2, 84, 4, -3, +3);
		//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
		//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
		if (HDevWindowStack::IsOpen())
			SetColor(HDevWindowStack::GetActive(), "green");
		if (HDevWindowStack::IsOpen())
			DispObj(ho_Edges, HDevWindowStack::GetActive());

	};

	//以下为各个特征测量结果处理算法
	double AxialD_81 = cam0_7_220_81Right_TLineFP[0] - Row_61right;
	AxialD_81 = AxialD_81 * Calik + pic4To5_moveDistance + pic5To6_moveDistance + pic6To7_moveDistance;
	resultVectorList.push_back(AxialD_81);
	pushback_vectors(resultVectorList, 4, 100, 414.9, -0.1, +0.1);

	double AxialD_88 = cam0_7_220_88_TLineFP[2] - cam0_7_220_81Right_TLineFP[0];
	AxialD_88 = AxialD_88 * Calik;
	resultVectorList.push_back(AxialD_88);
	pushback_vectors(resultVectorList, 4, 5, 12, -0.21, +0.21);
};
void program_5::cam0Picture8_algorithm(string imgPath)//远心相机8图像算法
{
	HTuple halconPath = imgPath.c_str();

	//"cam0_8-460-107.sbm"
	//cam0_8_220_130ChamferBegin_TLineFP()
	double cam0_8_220_130ChamferBegin_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_8-220-107.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -900;
		hv_Line[1] = -50;
		hv_Line[2] = -730;
		hv_Line[3] = -50;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50,
			HTuple(), HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.6,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_8_220_130ChamferBegin_TLineFP[0] = hv_RowBegin[0].D();
		cam0_8_220_130ChamferBegin_TLineFP[1] = hv_ColBegin[0].D();
		cam0_8_220_130ChamferBegin_TLineFP[2] = hv_RowEnd[0].D();
		cam0_8_220_130ChamferBegin_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_8_220_130ChamferBegin_TLineFP():" << cam0_8_220_130ChamferBegin_TLineFP[0] << "  " << cam0_8_220_130ChamferBegin_TLineFP[1] << "  " << cam0_8_220_130ChamferBegin_TLineFP[2] << "  " << cam0_8_220_130ChamferBegin_TLineFP[3] << endl;

	};

	//cam0_8_220_107_130ChamferEnd_108left_131ChamferBegin_TLineFP()
	double cam0_8_220_107_130ChamferEnd_108left_131ChamferBegin_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_8-220-107.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -550;
		hv_Line[1] = 80;
		hv_Line[2] = 700;
		hv_Line[3] = 80;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 30, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.6,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_8_220_107_130ChamferEnd_108left_131ChamferBegin_TLineFP[0] = hv_RowBegin[0].D();
		cam0_8_220_107_130ChamferEnd_108left_131ChamferBegin_TLineFP[1] = hv_ColBegin[0].D();
		cam0_8_220_107_130ChamferEnd_108left_131ChamferBegin_TLineFP[2] = hv_RowEnd[0].D();
		cam0_8_220_107_130ChamferEnd_108left_131ChamferBegin_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_8_220_107_130ChamferEnd_108left_131ChamferBegin_TLineFP():" << cam0_8_220_107_130ChamferEnd_108left_131ChamferBegin_TLineFP[0] << "  " << cam0_8_220_107_130ChamferEnd_108left_131ChamferBegin_TLineFP[1] << "  " << cam0_8_220_107_130ChamferEnd_108left_131ChamferBegin_TLineFP[2] << "  " << cam0_8_220_107_130ChamferEnd_108left_131ChamferBegin_TLineFP[3] << endl;

	}

	//以下函数传递
	//cam0_8_220_130Chamfer()
	double cam0_8_220_130Chamfer;
	{

		// Local iconic variables
		HObject  ho_Image, ho_Rectangle, ho_ImageReduced;
		HObject  ho_Edges;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_Row, hv_Column;
		HTuple  hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;

		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		//dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
		//
		//Segment a region containing the edges
		//基于全局阈值的图像快速阈值化
		//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin 500与colend 3500可固定不变
		GenRectangle1(&ho_Rectangle, cam0_8_220_130ChamferBegin_TLineFP[2], 500, cam0_8_220_107_130ChamferEnd_108left_131ChamferBegin_TLineFP[0] - 5, 3500);
		//提取roi区域，roi以外的变黑，不改变整个图像的尺寸大小。corp_domain是只提取roi区域
		ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);

		//In the subdomain of the image containing the edges,
		//extract subpixel precise edges.
		//提取亚像素精密边缘轮廓
		EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		FitCircleContourXld(ho_Edges, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		cam0_8_220_130Chamfer = hv_Radius.D();
		cout << "cam0_8_220_130Chamfer():" << cam0_8_220_130Chamfer << " " << cam0_8_220_130Chamfer * 0.01216 << endl;
		resultVectorList.push_back(cam0_8_220_130Chamfer * Calik);
		pushback_vectors(resultVectorList, 2, 85, 4, -3, +3);
		//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
		//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
		if (HDevWindowStack::IsOpen())
			SetColor(HDevWindowStack::GetActive(), "green");
		if (HDevWindowStack::IsOpen())
			DispObj(ho_Edges, HDevWindowStack::GetActive());

	}

	//cam0_8_220_131ChamferEnd_198ChamferBegin_TLineFP()
	double cam0_8_220_131ChamferEnd_198ChamferBegin_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_8-220-107.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = 680;
		hv_Line[1] = -50;
		hv_Line[2] = 1100;
		hv_Line[3] = -50;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50,
			HTuple(), HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.5,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_8_220_131ChamferEnd_198ChamferBegin_TLineFP[0] = hv_RowBegin[0].D();
		cam0_8_220_131ChamferEnd_198ChamferBegin_TLineFP[1] = hv_ColBegin[0].D();
		cam0_8_220_131ChamferEnd_198ChamferBegin_TLineFP[2] = hv_RowEnd[0].D();
		cam0_8_220_131ChamferEnd_198ChamferBegin_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_8_220_131ChamferEnd_198ChamferBegin_TLineFP():" << cam0_8_220_131ChamferEnd_198ChamferBegin_TLineFP[0] << "  " << cam0_8_220_131ChamferEnd_198ChamferBegin_TLineFP[1] << "  " << cam0_8_220_131ChamferEnd_198ChamferBegin_TLineFP[2] << "  " << cam0_8_220_131ChamferEnd_198ChamferBegin_TLineFP[3] << endl;

	}

	//以下函数传递
	//cam0_8_220_131Chamfer()
	double cam0_8_220_131Chamfer;
	{

		// Local iconic variables
		HObject  ho_Image, ho_Rectangle, ho_ImageReduced;
		HObject  ho_Edges;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_Row, hv_Column;
		HTuple  hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;

		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		//dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
		//
		//Segment a region containing the edges
		//基于全局阈值的图像快速阈值化
		//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin 500与colend 3500可固定不变
		GenRectangle1(&ho_Rectangle, cam0_8_220_107_130ChamferEnd_108left_131ChamferBegin_TLineFP[2], 500, cam0_8_220_131ChamferEnd_198ChamferBegin_TLineFP[0], 1370);
		//提取roi区域，roi以外的变黑，不改变整个图像的尺寸大小。corp_domain是只提取roi区域
		ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);

		//In the subdomain of the image containing the edges,
		//extract subpixel precise edges.
		//提取亚像素精密边缘轮廓
		EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		FitCircleContourXld(ho_Edges, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		cam0_8_220_131Chamfer = hv_Radius.D();
		cout << "cam0_8_220_131Chamfer():" << cam0_8_220_131Chamfer << " " << cam0_8_220_131Chamfer * 0.01216 << endl;
		resultVectorList.push_back(cam0_8_220_131Chamfer * Calik);
		pushback_vectors(resultVectorList, 2, 86, 1, -0.3, +0.3);
		//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
		//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
		if (HDevWindowStack::IsOpen())
			SetColor(HDevWindowStack::GetActive(), "green");
		if (HDevWindowStack::IsOpen())
			DispObj(ho_Edges, HDevWindowStack::GetActive());

	}

	//"cam0_8-460-109左.sbm"
	//cam0_8_220_109left_244ChamferBegin_TLineFP()
	double cam0_8_220_109left_244ChamferBegin_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_8-220-109左.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -50;
		hv_Line[1] = -50;
		hv_Line[2] = -50;
		hv_Line[3] = 100;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 40,
			HTuple(), HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.4,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_8_220_109left_244ChamferBegin_TLineFP[0] = hv_RowBegin[0].D();
		cam0_8_220_109left_244ChamferBegin_TLineFP[1] = hv_ColBegin[0].D();
		cam0_8_220_109left_244ChamferBegin_TLineFP[2] = hv_RowEnd[0].D();
		cam0_8_220_109left_244ChamferBegin_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_8_220_109left_244ChamferBegin_TLineFP():" << cam0_8_220_109left_244ChamferBegin_TLineFP[0] << "  " << cam0_8_220_109left_244ChamferBegin_TLineFP[1] << "  " << cam0_8_220_109left_244ChamferBegin_TLineFP[2] << "  " << cam0_8_220_109left_244ChamferBegin_TLineFP[3] << endl;

	}

	//cam0_8_220_200Gear7LeftChamferEnd_204Gear7_TLineFP()
	double cam0_8_220_200Gear7LeftChamferEnd_204Gear7_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_8-220-109左.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -110;
		hv_Line[1] = -20;
		hv_Line[2] = -60;
		hv_Line[3] = 90;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 40, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.5,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
		//显示形状特征模板匹配结果
		//  
			//  
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
		cam0_8_220_200Gear7LeftChamferEnd_204Gear7_TLineFP[0] = hv_RowBegin[0].D();
		cam0_8_220_200Gear7LeftChamferEnd_204Gear7_TLineFP[1] = hv_ColBegin[0].D();
		cam0_8_220_200Gear7LeftChamferEnd_204Gear7_TLineFP[2] = hv_RowEnd[0].D();
		cam0_8_220_200Gear7LeftChamferEnd_204Gear7_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_8_220_200Gear7LeftChamferEnd_204Gear7_TLineFP():" << cam0_8_220_200Gear7LeftChamferEnd_204Gear7_TLineFP[0] << "  " << cam0_8_220_200Gear7LeftChamferEnd_204Gear7_TLineFP[1] << "  " << cam0_8_220_200Gear7LeftChamferEnd_204Gear7_TLineFP[2] << "  " << cam0_8_220_200Gear7LeftChamferEnd_204Gear7_TLineFP[3] << endl;

	};

	//cam0_8_220_200Gear7LeftChamferBegin_200Gear6RightChamferEnd_TLineFP()
	double cam0_8_220_200Gear7LeftChamferBegin_200Gear6RightChamferEnd_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_8-220-109左.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -260;
		hv_Line[1] = -90;
		hv_Line[2] = -150;
		hv_Line[3] = -90;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.4,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
		//显示形状特征模板匹配结果
		//  
			//  
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
		cam0_8_220_200Gear7LeftChamferBegin_200Gear6RightChamferEnd_TLineFP[0] = hv_RowBegin[0].D();
		cam0_8_220_200Gear7LeftChamferBegin_200Gear6RightChamferEnd_TLineFP[1] = hv_ColBegin[0].D();
		cam0_8_220_200Gear7LeftChamferBegin_200Gear6RightChamferEnd_TLineFP[2] = hv_RowEnd[0].D();
		cam0_8_220_200Gear7LeftChamferBegin_200Gear6RightChamferEnd_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_8_220_200Gear7LeftChamferBegin_200Gear6RightChamferEnd_TLineFP():" << cam0_8_220_200Gear7LeftChamferBegin_200Gear6RightChamferEnd_TLineFP[0] << "  " << cam0_8_220_200Gear7LeftChamferBegin_200Gear6RightChamferEnd_TLineFP[1] << "  " << cam0_8_220_200Gear7LeftChamferBegin_200Gear6RightChamferEnd_TLineFP[2] << "  " << cam0_8_220_200Gear7LeftChamferBegin_200Gear6RightChamferEnd_TLineFP[3] << endl;

	};

	//以下函数传递
	//cam0_8_220_200Gear7LeftChamfer()
	double cam0_8_220_200Gear7LeftChamfer = TLineFP_Chamfer(imgPath,
		3545, 1200, 3600, 1340,
		2, 20, 60,
		true, 5, 4, 2,
		20, -0.5, 0.5,
		90, 1,
		-1, 0, 0, 3, 2);
	//resultVectorList.push_back(cam0_8_220_200Gear7LeftChamfer* Calik);
	//pushback_vectors(resultVectorList, 2, 67.7, 1, -0.25, +0);

	//cam0_8_220_244ChamferEnd_TLineFP()
	double cam0_8_220_244ChamferEnd_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_8-220-109左.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = 10;
		hv_Line[1] = -100;
		hv_Line[2] = 300;
		hv_Line[3] = -100;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50,
			HTuple(), HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.4,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_8_220_244ChamferEnd_TLineFP[0] = hv_RowBegin[0].D();
		cam0_8_220_244ChamferEnd_TLineFP[1] = hv_ColBegin[0].D();
		cam0_8_220_244ChamferEnd_TLineFP[2] = hv_RowEnd[0].D();
		cam0_8_220_244ChamferEnd_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_8_220_244ChamferEnd_TLineFP():" << cam0_8_220_244ChamferEnd_TLineFP[0] << "  " << cam0_8_220_244ChamferEnd_TLineFP[1] << "  " << cam0_8_220_244ChamferEnd_TLineFP[2] << "  " << cam0_8_220_244ChamferEnd_TLineFP[3] << endl;

	};

	//以下函数传递
	//cam0_8_220_244Chamfer()
	double cam0_8_220_244Chamfer = TLineFP_Chamfer(imgPath,
		3650, 1200, 3740, 1345,
		2, 20, 60,
		true, 5, 4, 2,
		20, -0.5, 0.5,
		90, 1,
		-1, 0, 0, 3, 2);
	//resultVectorList.push_back(cam0_8_220_244Chamfer* Calik);
	//pushback_vectors(resultVectorList, 2, 68.7, 0.8, -0.3, +0.3);

	//cam0_8_220_208Left_TLineFP()
	double cam0_8_220_208Left_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_8-220-109左.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -280;
		hv_Line[1] = -50;
		hv_Line[2] = -280;
		hv_Line[3] = 140;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.4,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_8_220_208Left_TLineFP[0] = hv_RowBegin[0].D();
		cam0_8_220_208Left_TLineFP[1] = hv_ColBegin[0].D();
		cam0_8_220_208Left_TLineFP[2] = hv_RowEnd[0].D();
		cam0_8_220_208Left_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_8_220_208Left_TLineFP():" << cam0_8_220_208Left_TLineFP[0] << "  " << cam0_8_220_208Left_TLineFP[1] << "  " << cam0_8_220_208Left_TLineFP[2] << "  " << cam0_8_220_208Left_TLineFP[3] << endl;

	};

	//以下函数传递
	//cam0_8_220_200Gear6RightChamfe()
	double cam0_8_220_200Gear6RightChamfe = TLineFP_Chamfer(imgPath,
		3370, 1200, 3486, 1355,
		2, 20, 60,
		true, 5, 4, 2,
		20, -0.5, 0.5,
		90, 1,
		-1, 0, 0, 3, 2);
	//resultVectorList.push_back(cam0_8_220_200Gear6RightChamfe* Calik);
	//pushback_vectors(resultVectorList, 2, 68.6, 0.8, -0.3, +0.3);

	//cam0_8_220_200Gear6LeftChamferEnd_204Gear6_TLineFP()
	double cam0_8_220_200Gear6LeftChamferEnd_204Gear6_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_8-220-109左.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -370;
		hv_Line[1] = -20;
		hv_Line[2] = -330;
		hv_Line[3] = 90;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.4,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_8_220_200Gear6LeftChamferEnd_204Gear6_TLineFP[0] = hv_RowBegin[0].D();
		cam0_8_220_200Gear6LeftChamferEnd_204Gear6_TLineFP[1] = hv_ColBegin[0].D();
		cam0_8_220_200Gear6LeftChamferEnd_204Gear6_TLineFP[2] = hv_RowEnd[0].D();
		cam0_8_220_200Gear6LeftChamferEnd_204Gear6_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_8_220_200Gear6LeftChamferEnd_204Gear6_TLineFP():" << cam0_8_220_200Gear6LeftChamferEnd_204Gear6_TLineFP[0] << "  " << cam0_8_220_200Gear6LeftChamferEnd_204Gear6_TLineFP[1] << "  " << cam0_8_220_200Gear6LeftChamferEnd_204Gear6_TLineFP[2] << "  " << cam0_8_220_200Gear6LeftChamferEnd_204Gear6_TLineFP[3] << endl;

	};

	//cam0_8_220_200Gear6LeftChamferBegin_200Gear5RightChamferEnd_TLineFP()
	double cam0_8_220_200Gear6LeftChamferBegin_200Gear5RightChamferEnd_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_8-220-109左.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -520;
		hv_Line[1] = -90;
		hv_Line[2] = -410;
		hv_Line[3] = -90;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.4,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_8_220_200Gear6LeftChamferBegin_200Gear5RightChamferEnd_TLineFP[0] = hv_RowBegin[0].D();
		cam0_8_220_200Gear6LeftChamferBegin_200Gear5RightChamferEnd_TLineFP[1] = hv_ColBegin[0].D();
		cam0_8_220_200Gear6LeftChamferBegin_200Gear5RightChamferEnd_TLineFP[2] = hv_RowEnd[0].D();
		cam0_8_220_200Gear6LeftChamferBegin_200Gear5RightChamferEnd_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_8_220_200Gear6LeftChamferBegin_200Gear5RightChamferEnd_TLineFP():" << cam0_8_220_200Gear6LeftChamferBegin_200Gear5RightChamferEnd_TLineFP[0] << "  " << cam0_8_220_200Gear6LeftChamferBegin_200Gear5RightChamferEnd_TLineFP[1] << "  " << cam0_8_220_200Gear6LeftChamferBegin_200Gear5RightChamferEnd_TLineFP[2] << "  " << cam0_8_220_200Gear6LeftChamferBegin_200Gear5RightChamferEnd_TLineFP[3] << endl;

	};

	//以下函数传递
	//cam0_8_220_200Gear6LeftChamfer()
	double cam0_8_220_200Gear6LeftChamfer = TLineFP_Chamfer(imgPath,
		3260, 1200, 3350, 1355,
		2, 20, 60,
		true, 5, 4, 2,
		20, -0.5, 0.5,
		90, 1,
		-1, 0, 0, 3, 2);
	//resultVectorList.push_back(cam0_8_220_200Gear6LeftChamfer* Calik);
	//pushback_vectors(resultVectorList, 2, 67.6, 1, -0.25, +0);

	//cam0_8_220_207_TLineFP()
	double cam0_8_220_207_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_8-220-109左.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -560;
		hv_Line[1] = -50;
		hv_Line[2] = -560;
		hv_Line[3] = 140;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.4,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_8_220_207_TLineFP[0] = hv_RowBegin[0].D();
		cam0_8_220_207_TLineFP[1] = hv_ColBegin[0].D();
		cam0_8_220_207_TLineFP[2] = hv_RowEnd[0].D();
		cam0_8_220_207_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_8_220_207_TLineFP():" << cam0_8_220_207_TLineFP[0] << "  " << cam0_8_220_207_TLineFP[1] << "  " << cam0_8_220_207_TLineFP[2] << "  " << cam0_8_220_207_TLineFP[3] << endl;

	};

	//以下函数传递
	//cam0_8_220_200Gear5RightChamfer()
	double cam0_8_220_200Gear5RightChamfer = TLineFP_Chamfer(imgPath,
		3115, 1200, 3210, 1370,
		2, 20, 60,
		true, 5, 4, 2,
		20, -0.5, 0.5,
		90, 1,
		-1, 0, 0, 3, 2);
	//resultVectorList.push_back(cam0_8_220_200Gear5RightChamfer* Calik);
	//pushback_vectors(resultVectorList, 2, 68.5, 0.8, -0.3, +0.3);

	//cam0_8_220_200Gear5LeftChamferEnd_204Gear5_TLineFP()
	double cam0_8_220_200Gear5LeftChamferEnd_204Gear5_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_8-220-109左.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -630;
		hv_Line[1] = 0;
		hv_Line[2] = -590;
		hv_Line[3] = 90;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.4,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
		//显示形状特征模板匹配结果
		//  
			//  
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
		cam0_8_220_200Gear5LeftChamferEnd_204Gear5_TLineFP[0] = hv_RowBegin[0].D();
		cam0_8_220_200Gear5LeftChamferEnd_204Gear5_TLineFP[1] = hv_ColBegin[0].D();
		cam0_8_220_200Gear5LeftChamferEnd_204Gear5_TLineFP[2] = hv_RowEnd[0].D();
		cam0_8_220_200Gear5LeftChamferEnd_204Gear5_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_8_220_200Gear5LeftChamferEnd_204Gear5_TLineFP():" << cam0_8_220_200Gear5LeftChamferEnd_204Gear5_TLineFP[0] << "  " << cam0_8_220_200Gear5LeftChamferEnd_204Gear5_TLineFP[1] << "  " << cam0_8_220_200Gear5LeftChamferEnd_204Gear5_TLineFP[2] << "  " << cam0_8_220_200Gear5LeftChamferEnd_204Gear5_TLineFP[3] << endl;

	};

	//cam0_8_220_200Gear5LeftChamferBegin_200Gear4RightChamferEnd_TLineFP()
	double cam0_8_220_200Gear5LeftChamferBegin_200Gear4RightChamferEnd_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_8-220-109左.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -780;
		hv_Line[1] = -90;
		hv_Line[2] = -680;
		hv_Line[3] = -90;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.4,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
		//显示形状特征模板匹配结果
		//  
			//  
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
		cam0_8_220_200Gear5LeftChamferBegin_200Gear4RightChamferEnd_TLineFP[0] = hv_RowBegin[0].D();
		cam0_8_220_200Gear5LeftChamferBegin_200Gear4RightChamferEnd_TLineFP[1] = hv_ColBegin[0].D();
		cam0_8_220_200Gear5LeftChamferBegin_200Gear4RightChamferEnd_TLineFP[2] = hv_RowEnd[0].D();
		cam0_8_220_200Gear5LeftChamferBegin_200Gear4RightChamferEnd_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_8_220_200Gear5LeftChamferBegin_200Gear4RightChamferEnd_TLineFP():" << cam0_8_220_200Gear5LeftChamferBegin_200Gear4RightChamferEnd_TLineFP[0] << "  " << cam0_8_220_200Gear5LeftChamferBegin_200Gear4RightChamferEnd_TLineFP[1] << "  " << cam0_8_220_200Gear5LeftChamferBegin_200Gear4RightChamferEnd_TLineFP[2] << "  " << cam0_8_220_200Gear5LeftChamferBegin_200Gear4RightChamferEnd_TLineFP[3] << endl;

	};

	//以下函数传递
	//cam0_8_220_200Gear5LeftChamfer()
	double cam0_8_220_200Gear5LeftChamfer = TLineFP_Chamfer(imgPath,
		2990, 1200, 3090, 1370,
		2, 20, 60,
		true, 5, 4, 2,
		20, -0.5, 0.5,
		90, 1,
		-1, 0, 0, 3, 2);
	//resultVectorList.push_back(cam0_8_220_200Gear5LeftChamfer* Calik);
	//pushback_vectors(resultVectorList, 2, 67.5, 1, -0.25, +0);

	//cam0_8_220_201Left_TLineFP()
	double cam0_8_220_201Left_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_8-220-109左.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -830;
		hv_Line[1] = -50;
		hv_Line[2] = -830;
		hv_Line[3] = 140;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.4,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
		//显示形状特征模板匹配结果
		//  
			//  
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
		cam0_8_220_201Left_TLineFP[0] = hv_RowBegin[0].D();
		cam0_8_220_201Left_TLineFP[1] = hv_ColBegin[0].D();
		cam0_8_220_201Left_TLineFP[2] = hv_RowEnd[0].D();
		cam0_8_220_201Left_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_8_220_201Left_TLineFP():" << cam0_8_220_201Left_TLineFP[0] << "  " << cam0_8_220_201Left_TLineFP[1] << "  " << cam0_8_220_201Left_TLineFP[2] << "  " << cam0_8_220_201Left_TLineFP[3] << endl;

	};

	//以下函数传递
	//cam0_8_220_200Gear4RightChamfer()
	double cam0_8_220_200Gear4RightChamfer = TLineFP_Chamfer(imgPath,
		2860, 1200, 2955, 1370,
		2, 20, 60,
		true, 5, 4, 2,
		20, -0.5, 0.5,
		90, 1,
		-1, 0, 0, 3, 2);
	//resultVectorList.push_back(cam0_8_220_200Gear4RightChamfer* Calik);
	//pushback_vectors(resultVectorList, 2, 68.4, 0.8, -0.3, +0.3);

	//cam0_8_220_200Gear4LeftChamferEnd_204Gear4_TLineFP()
	double cam0_8_220_200Gear4LeftChamferEnd_204Gear4_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_8-220-109左.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -890;
		hv_Line[1] = 0;
		hv_Line[2] = -860;
		hv_Line[3] = 90;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 40, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.4,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_8_220_200Gear4LeftChamferEnd_204Gear4_TLineFP[0] = hv_RowBegin[0].D();
		cam0_8_220_200Gear4LeftChamferEnd_204Gear4_TLineFP[1] = hv_ColBegin[0].D();
		cam0_8_220_200Gear4LeftChamferEnd_204Gear4_TLineFP[2] = hv_RowEnd[0].D();
		cam0_8_220_200Gear4LeftChamferEnd_204Gear4_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_8_220_200Gear4LeftChamferEnd_204Gear4_TLineFP():" << cam0_8_220_200Gear4LeftChamferEnd_204Gear4_TLineFP[0] << "  " << cam0_8_220_200Gear4LeftChamferEnd_204Gear4_TLineFP[1] << "  " << cam0_8_220_200Gear4LeftChamferEnd_204Gear4_TLineFP[2] << "  " << cam0_8_220_200Gear4LeftChamferEnd_204Gear4_TLineFP[3] << endl;

	};

	//cam0_8_220_200Gear4LeftChamferBegin_200Gear3RightChamferEnd_TLineFP()
	double cam0_8_220_200Gear4LeftChamferBegin_200Gear3RightChamferEnd_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_8-220-109左.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -1050;
		hv_Line[1] = -90;
		hv_Line[2] = -930;
		hv_Line[3] = -90;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.4,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_8_220_200Gear4LeftChamferBegin_200Gear3RightChamferEnd_TLineFP[0] = hv_RowBegin[0].D();
		cam0_8_220_200Gear4LeftChamferBegin_200Gear3RightChamferEnd_TLineFP[1] = hv_ColBegin[0].D();
		cam0_8_220_200Gear4LeftChamferBegin_200Gear3RightChamferEnd_TLineFP[2] = hv_RowEnd[0].D();
		cam0_8_220_200Gear4LeftChamferBegin_200Gear3RightChamferEnd_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_8_220_200Gear4LeftChamferBegin_200Gear3RightChamferEnd_TLineFP():" << cam0_8_220_200Gear4LeftChamferBegin_200Gear3RightChamferEnd_TLineFP[0] << "  " << cam0_8_220_200Gear4LeftChamferBegin_200Gear3RightChamferEnd_TLineFP[1] << "  " << cam0_8_220_200Gear4LeftChamferBegin_200Gear3RightChamferEnd_TLineFP[2] << "  " << cam0_8_220_200Gear4LeftChamferBegin_200Gear3RightChamferEnd_TLineFP[3] << endl;

	};

	//以下函数传递
	//cam0_8_220_200Gear4LeftChamfer()
	double cam0_8_220_200Gear4LeftChamfer = TLineFP_Chamfer(imgPath,
		2735, 1200, 2822, 1370,
		2, 20, 60,
		true, 5, 4, 2,
		20, -0.5, 0.5,
		90, 1,
		-1, 0, 0, 3, 2);
	//resultVectorList.push_back(cam0_8_220_200Gear4LeftChamfer* Calik);
	//pushback_vectors(resultVectorList, 2, 67.4, 1, -0.1, +0.1);

	//cam0_8_220_199Left_TLineFP()
	double cam0_8_220_199Left_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_8-220-109左.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -1100;
		hv_Line[1] = -50;
		hv_Line[2] = -1100;
		hv_Line[3] = 140;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.4,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_8_220_199Left_TLineFP[0] = hv_RowBegin[0].D();
		cam0_8_220_199Left_TLineFP[1] = hv_ColBegin[0].D();
		cam0_8_220_199Left_TLineFP[2] = hv_RowEnd[0].D();
		cam0_8_220_199Left_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_8_220_199Left_TLineFP():" << cam0_8_220_199Left_TLineFP[0] << "  " << cam0_8_220_199Left_TLineFP[1] << "  " << cam0_8_220_199Left_TLineFP[2] << "  " << cam0_8_220_199Left_TLineFP[3] << endl;

	};

	//以下函数传递
	//cam0_8_220_200Gear3RightChamfer()
	double cam0_8_220_200Gear3RightChamfer = TLineFP_Chamfer(imgPath,
		2600, 1200, 2690, 1370,
		2, 20, 60,
		true, 5, 4, 2,
		20, -0.5, 0.5,
		90, 1,
		-1, 0, 0, 3, 2);
	//resultVectorList.push_back(cam0_8_220_200Gear3RightChamfer* Calik);
	//pushback_vectors(resultVectorList, 2, 68.3, 0.8, -0.3, +0.3);

	//cam0_8_220_200Gear3LeftChamferEnd_204Gear3_TLineFP()
	double cam0_8_220_200Gear3LeftChamferEnd_204Gear3_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_8-220-109左.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -1150;
		hv_Line[1] = 20;
		hv_Line[2] = -1130;
		hv_Line[3] = 90;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.4,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_8_220_200Gear3LeftChamferEnd_204Gear3_TLineFP[0] = hv_RowBegin[0].D();
		cam0_8_220_200Gear3LeftChamferEnd_204Gear3_TLineFP[1] = hv_ColBegin[0].D();
		cam0_8_220_200Gear3LeftChamferEnd_204Gear3_TLineFP[2] = hv_RowEnd[0].D();
		cam0_8_220_200Gear3LeftChamferEnd_204Gear3_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_8_220_200Gear3LeftChamferEnd_204Gear3_TLineFP():" << cam0_8_220_200Gear3LeftChamferEnd_204Gear3_TLineFP[0] << "  " << cam0_8_220_200Gear3LeftChamferEnd_204Gear3_TLineFP[1] << "  " << cam0_8_220_200Gear3LeftChamferEnd_204Gear3_TLineFP[2] << "  " << cam0_8_220_200Gear3LeftChamferEnd_204Gear3_TLineFP[3] << endl;

	};

	//cam0_8_220_200Gear3LeftChamferBegin_200Gear2RightChamferEnd_TLineFP()
	double cam0_8_220_200Gear3LeftChamferBegin_200Gear2RightChamferEnd_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_8-220-109左.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -1310;
		hv_Line[1] = -90;
		hv_Line[2] = -1190;
		hv_Line[3] = -90;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 20, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.4,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_8_220_200Gear3LeftChamferBegin_200Gear2RightChamferEnd_TLineFP[0] = hv_RowBegin[0].D();
		cam0_8_220_200Gear3LeftChamferBegin_200Gear2RightChamferEnd_TLineFP[1] = hv_ColBegin[0].D();
		cam0_8_220_200Gear3LeftChamferBegin_200Gear2RightChamferEnd_TLineFP[2] = hv_RowEnd[0].D();
		cam0_8_220_200Gear3LeftChamferBegin_200Gear2RightChamferEnd_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_8_220_200Gear3LeftChamferBegin_200Gear2RightChamferEnd_TLineFP():" << cam0_8_220_200Gear3LeftChamferBegin_200Gear2RightChamferEnd_TLineFP[0] << "  " << cam0_8_220_200Gear3LeftChamferBegin_200Gear2RightChamferEnd_TLineFP[1] << "  " << cam0_8_220_200Gear3LeftChamferBegin_200Gear2RightChamferEnd_TLineFP[2] << "  " << cam0_8_220_200Gear3LeftChamferBegin_200Gear2RightChamferEnd_TLineFP[3] << endl;

	};

	//以下函数传递
	//cam0_8_220_200Gear3LeftChamfer()
	double cam0_8_220_200Gear3LeftChamfer = TLineFP_Chamfer(imgPath,
		2475, 1200, 2555, 1370,
		2, 20, 60,
		true, 5, 4, 2,
		20, -0.5, 0.5,
		90, 1,
		-1, 0, 0, 3, 2);
	//resultVectorList.push_back(cam0_8_220_200Gear3LeftChamfer* Calik);
	//pushback_vectors(resultVectorList, 2, 67.3, 1, -0.25, +0);


	//cam0_8_220_206Left_TLineFP()
	double cam0_8_220_206Left_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_8-220-109左.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -1360;
		hv_Line[1] = -50;
		hv_Line[2] = -1360;
		hv_Line[3] = 140;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.4,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_8_220_206Left_TLineFP[0] = hv_RowBegin[0].D();
		cam0_8_220_206Left_TLineFP[1] = hv_ColBegin[0].D();
		cam0_8_220_206Left_TLineFP[2] = hv_RowEnd[0].D();
		cam0_8_220_206Left_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_8_220_206Left_TLineFP():" << cam0_8_220_206Left_TLineFP[0] << "  " << cam0_8_220_206Left_TLineFP[1] << "  " << cam0_8_220_206Left_TLineFP[2] << "  " << cam0_8_220_206Left_TLineFP[3] << endl;

	};

	//以下函数传递
	//cam0_8_220_200Gear2RightChamfer()
	double cam0_8_220_200Gear2RightChamfer = TLineFP_Chamfer(imgPath,
		2340, 1200, 2435, 1370,
		2, 20, 60,
		true, 5, 4, 2,
		20, -0.5, 0.5,
		90, 1,
		-1, 0, 0, 3, 2);
	//resultVectorList.push_back(cam0_8_220_200Gear2RightChamfer* Calik);
	//pushback_vectors(resultVectorList, 2, 68.2, 0.8, -0.3, +0.3);

	//cam0_8_220_200Gear2LeftChamferEnd_204Gear2_TLineFP()
	double cam0_8_220_200Gear2LeftChamferEnd_204Gear2_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_8-220-109左.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -1410;
		hv_Line[1] = 20;
		hv_Line[2] = -1394;
		hv_Line[3] = 90;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 40, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.4,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_8_220_200Gear2LeftChamferEnd_204Gear2_TLineFP[0] = hv_RowBegin[0].D();
		cam0_8_220_200Gear2LeftChamferEnd_204Gear2_TLineFP[1] = hv_ColBegin[0].D();
		cam0_8_220_200Gear2LeftChamferEnd_204Gear2_TLineFP[2] = hv_RowEnd[0].D();
		cam0_8_220_200Gear2LeftChamferEnd_204Gear2_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_8_220_200Gear2LeftChamferEnd_204Gear2_TLineFP():" << cam0_8_220_200Gear2LeftChamferEnd_204Gear2_TLineFP[0] << "  " << cam0_8_220_200Gear2LeftChamferEnd_204Gear2_TLineFP[1] << "  " << cam0_8_220_200Gear2LeftChamferEnd_204Gear2_TLineFP[2] << "  " << cam0_8_220_200Gear2LeftChamferEnd_204Gear2_TLineFP[3] << endl;

	};

	//cam0_8_220_200Gear2LeftChamferBegin_200Gear1RightChamferEnd_TLineFP()
	double cam0_8_220_200Gear2LeftChamferBegin_200Gear1RightChamferEnd_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_8-220-109左.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -1570;
		hv_Line[1] = -70;
		hv_Line[2] = -1450;
		hv_Line[3] = -70;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.4,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_8_220_200Gear2LeftChamferBegin_200Gear1RightChamferEnd_TLineFP[0] = hv_RowBegin[0].D();
		cam0_8_220_200Gear2LeftChamferBegin_200Gear1RightChamferEnd_TLineFP[1] = hv_ColBegin[0].D();
		cam0_8_220_200Gear2LeftChamferBegin_200Gear1RightChamferEnd_TLineFP[2] = hv_RowEnd[0].D();
		cam0_8_220_200Gear2LeftChamferBegin_200Gear1RightChamferEnd_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_8_220_200Gear2LeftChamferBegin_200Gear1RightChamferEnd_TLineFP():" << cam0_8_220_200Gear2LeftChamferBegin_200Gear1RightChamferEnd_TLineFP[0] << "  " << cam0_8_220_200Gear2LeftChamferBegin_200Gear1RightChamferEnd_TLineFP[1] << "  " << cam0_8_220_200Gear2LeftChamferBegin_200Gear1RightChamferEnd_TLineFP[2] << "  " << cam0_8_220_200Gear2LeftChamferBegin_200Gear1RightChamferEnd_TLineFP[3] << endl;

	};

	//以下函数传递
	//cam0_8_220_200Gear2LeftChamfer()
	double cam0_8_220_200Gear2LeftChamfer = TLineFP_Chamfer(imgPath,
		2205, 1200, 2292, 1370,
		2, 20, 60,
		true, 5, 4, 2,
		20, -0.5, 0.5,
		90, 1,
		-1, 0, 0, 3, 2);
	//resultVectorList.push_back(cam0_8_220_200Gear2LeftChamfer* Calik);
	//pushback_vectors(resultVectorList, 2, 67.2, 1, -0.25, +0);


	//cam0_8_220_205Left_TLineFP()
	double cam0_8_220_205Left_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_8-220-109左.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -1620;
		hv_Line[1] = -50;
		hv_Line[2] = -1620;
		hv_Line[3] = 140;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.4,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_8_220_205Left_TLineFP[0] = hv_RowBegin[0].D();
		cam0_8_220_205Left_TLineFP[1] = hv_ColBegin[0].D();
		cam0_8_220_205Left_TLineFP[2] = hv_RowEnd[0].D();
		cam0_8_220_205Left_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_8_220_205Left_TLineFP():" << cam0_8_220_205Left_TLineFP[0] << "  " << cam0_8_220_205Left_TLineFP[1] << "  " << cam0_8_220_205Left_TLineFP[2] << "  " << cam0_8_220_205Left_TLineFP[3] << endl;

	};

	//以下函数传递
	//cam0_8_220_200Gear1RightChamfer()
	double cam0_8_220_200Gear1RightChamfer = TLineFP_Chamfer(imgPath,
		2062, 1200, 2160, 1370,
		2, 20, 60,
		true, 5, 4, 2,
		20, -0.5, 0.5,
		90, 1,
		-1, 0, 0, 3, 2);
	//resultVectorList.push_back(cam0_8_220_200Gear1RightChamfer* Calik);
	//pushback_vectors(resultVectorList, 2, 68.1, 0.8, -0.3, +0.3);

	//cam0_8_220_200Gear1LeftChamfer198End_204Gear1_TLineFP()
	double cam0_8_220_200Gear1LeftChamfer198End_204Gear1_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_8-220-109左.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -1680;
		hv_Line[1] = 20;
		hv_Line[2] = -1650;
		hv_Line[3] = 90;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 20, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.4,
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
		cam0_8_220_200Gear1LeftChamfer198End_204Gear1_TLineFP[0] = hv_RowBegin[0].D();
		cam0_8_220_200Gear1LeftChamfer198End_204Gear1_TLineFP[1] = hv_ColBegin[0].D();
		cam0_8_220_200Gear1LeftChamfer198End_204Gear1_TLineFP[2] = hv_RowEnd[0].D();
		cam0_8_220_200Gear1LeftChamfer198End_204Gear1_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_8_220_200Gear1LeftChamfer198End_204Gear1_TLineFP():" << cam0_8_220_200Gear1LeftChamfer198End_204Gear1_TLineFP[0] << "  " << cam0_8_220_200Gear1LeftChamfer198End_204Gear1_TLineFP[1] << "  " << cam0_8_220_200Gear1LeftChamfer198End_204Gear1_TLineFP[2] << "  " << cam0_8_220_200Gear1LeftChamfer198End_204Gear1_TLineFP[3] << endl;

	};
	//double cam0_8_220_200Gear1LeftChamfer198End_204Gear1_TLineFP[4];
	//Straight_TLineFP(cam0_8_220_200Gear1LeftChamfer198End_204Gear1_TLineFP, imgPath,
	//	3850, 4370, 3961, 4370,
	//	50, 12, 1, 20);

	//cam0_8_220_200Gear1LeftChamfer198Begin()_TLineFP
	double cam0_8_220_200Gear1LeftChamfer198Begin[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_8-220-109左.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -1950;
		hv_Line[1] = -80;
		hv_Line[2] = -1730;
		hv_Line[3] = -80;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.4,
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
		cam0_8_220_200Gear1LeftChamfer198Begin[0] = hv_RowBegin[0].D();
		cam0_8_220_200Gear1LeftChamfer198Begin[1] = hv_ColBegin[0].D();
		cam0_8_220_200Gear1LeftChamfer198Begin[2] = hv_RowEnd[0].D();
		cam0_8_220_200Gear1LeftChamfer198Begin[3] = hv_ColEnd[0].D();
		cout << " cam0_8_220_200Gear1LeftChamfer198Begin():" << cam0_8_220_200Gear1LeftChamfer198Begin[0] << "  " << cam0_8_220_200Gear1LeftChamfer198Begin[1] << "  " << cam0_8_220_200Gear1LeftChamfer198Begin[2] << "  " << cam0_8_220_200Gear1LeftChamfer198Begin[3] << endl;

	};

	//以下函数传递
	//cam0_8_220_200Gear1LeftChamfer198()
	double cam0_8_220_200Gear1LeftChamfer198 = TLineFP_Chamfer(imgPath,
		1940, 1200, 2035, 1370,
		2, 20, 60,
		true, 5, 4, 2,
		20, -0.5, 0.5,
		90, 1,
		-1, 0, 0, 3, 2);
	//resultVectorList.push_back(cam0_8_220_200Gear1LeftChamfer198* Calik);
	//pushback_vectors(resultVectorList, 2, 67.1, 1, -0.25, +0);



	//以下为各个特征测量结果处理算法
	/*************/

	resultVectorList.push_back(cam0_8_220_244Chamfer * Calik);
	resultVectorList.push_back(cam0_8_220_200Gear6RightChamfe * Calik);
	resultVectorList.push_back(cam0_8_220_200Gear5RightChamfer * Calik);
	resultVectorList.push_back(cam0_8_220_200Gear4RightChamfer * Calik);
	resultVectorList.push_back(cam0_8_220_200Gear3RightChamfer * Calik);
	resultVectorList.push_back(cam0_8_220_200Gear2RightChamfer * Calik);
	resultVectorList.push_back(cam0_8_220_200Gear1RightChamfer * Calik);
	pushback_vectors(resultVectorList, 2, 68, 0.8, -0.3, +0.3);

	resultVectorList.push_back(cam0_8_220_200Gear7LeftChamfer * Calik);
	resultVectorList.push_back(cam0_8_220_200Gear6LeftChamfer * Calik);
	resultVectorList.push_back(cam0_8_220_200Gear5LeftChamfer * Calik);
	resultVectorList.push_back(cam0_8_220_200Gear4LeftChamfer * Calik);
	resultVectorList.push_back(cam0_8_220_200Gear3LeftChamfer * Calik);
	resultVectorList.push_back(cam0_8_220_200Gear2LeftChamfer * Calik);
	resultVectorList.push_back(cam0_8_220_200Gear1LeftChamfer198 * Calik);
	pushback_vectors(resultVectorList, 2, 67, 1, -0.25, +0);



	/*************/
	Row_108left = cam0_8_220_107_130ChamferEnd_108left_131ChamferBegin_TLineFP[2];

	double AxialD_107 = cam0_8_220_107_130ChamferEnd_108left_131ChamferBegin_TLineFP[2] - cam0_8_220_107_130ChamferEnd_108left_131ChamferBegin_TLineFP[0];
	AxialD_107 = AxialD_107 * Calik;
	resultVectorList.push_back(AxialD_107);
	pushback_vectors(resultVectorList, 4, 6, 14, -1, +1);

	Row_109left = (cam0_8_220_109left_244ChamferBegin_TLineFP[2] + cam0_8_220_109left_244ChamferBegin_TLineFP[0]) / 2;
	double AxialD_208 = Row_109left - (cam0_8_220_208Left_TLineFP[2] + cam0_8_220_208Left_TLineFP[0]) / 2;
	AxialD_208 = AxialD_208 * Calik;
	resultVectorList.push_back(AxialD_208);
	pushback_vectors(resultVectorList, 4, 73, 3.2, -0.15, +0.15);

	double AxialD_207 = Row_109left - (cam0_8_220_207_TLineFP[2] + cam0_8_220_207_TLineFP[0]) / 2;
	AxialD_207 = AxialD_207 * Calik;
	resultVectorList.push_back(AxialD_207);
	pushback_vectors(resultVectorList, 4, 74, 6.4, -0.15, +0.15);

	double AxialD_201 = Row_109left - (cam0_8_220_201Left_TLineFP[2] + cam0_8_220_201Left_TLineFP[0]) / 2;
	AxialD_201 = AxialD_201 * Calik;
	resultVectorList.push_back(AxialD_201);
	pushback_vectors(resultVectorList, 4, 75, 9.6, -0.15, +0.15);

	double AxialD_199 = Row_109left - (cam0_8_220_199Left_TLineFP[2] + cam0_8_220_199Left_TLineFP[0]) / 2;
	AxialD_199 = AxialD_199 * Calik;
	resultVectorList.push_back(AxialD_199);
	pushback_vectors(resultVectorList, 4, 66, 12.8, -0.15, +0.15);

	double AxialD_206 = Row_109left - (cam0_8_220_206Left_TLineFP[2] + cam0_8_220_206Left_TLineFP[0]) / 2;
	AxialD_206 = AxialD_206 * Calik;
	resultVectorList.push_back(AxialD_206);
	pushback_vectors(resultVectorList, 4, 65, 16, -0.15, +0.15);

	double AxialD_205 = Row_109left - (cam0_8_220_205Left_TLineFP[2] + cam0_8_220_205Left_TLineFP[0]) / 2;
	AxialD_205 = AxialD_205 * Calik;
	resultVectorList.push_back(AxialD_205);
	pushback_vectors(resultVectorList, 4, 70, 19.2, -0.15, +0.15);


	//齿顶宽
	//新找点
	double cam0_8_220_gear7chiding[4];
	Straight_TLineFP(cam0_8_220_gear7chiding, imgPath,
		3632, 1464, 3651, 1521,
		20, 12, 1, 20);
	cam0_8_220_gear7chiding[2] = cam0_8_220_gear7chiding[2] - 3.07;
	double cam0_8_220_gear6chiding[4];
	Straight_TLineFP(cam0_8_220_gear6chiding, imgPath,
		3372, 1464, 3388, 1521,
		20, 12, 1, 20);
	cam0_8_220_gear6chiding[2] = cam0_8_220_gear6chiding[2] + 3.07;
	double cam0_8_220_gear5chiding[4];
	Straight_TLineFP(cam0_8_220_gear5chiding, imgPath,
		3114, 1464, 3125, 1521,
		20, 12, 1, 20);
	cam0_8_220_gear5chiding[2] = cam0_8_220_gear5chiding[2] + 3.07;
	double cam0_8_220_gear4chiding[4];
	Straight_TLineFP(cam0_8_220_gear4chiding, imgPath,
		2850, 1464, 2861, 1521,
		20, 12, 1, 20);
	cam0_8_220_gear4chiding[2] = cam0_8_220_gear4chiding[2] + 3.07;
	double cam0_8_220_gear3chiding[4];
	Straight_TLineFP(cam0_8_220_gear3chiding, imgPath,
		2590, 1464, 2598, 1521,
		20, 12, 1, 20);
	cam0_8_220_gear3chiding[2] = cam0_8_220_gear3chiding[2] + 3.07;
	double cam0_8_220_gear2chiding[4];
	Straight_TLineFP(cam0_8_220_gear2chiding, imgPath,
		2326, 1464, 2337, 1521,
		20, 12, 1, 20);
	cam0_8_220_gear2chiding[2] = cam0_8_220_gear2chiding[2] + 3.07;
	double cam0_8_220_gear1chiding[4];
	Straight_TLineFP(cam0_8_220_gear1chiding, imgPath,
		2065, 1464, 2075, 1521,
		20, 12, 1, 20);
	cam0_8_220_gear1chiding[2] = cam0_8_220_gear1chiding[2] + 3.07;
	//找点结束
	double AxialD_202_7 = cam0_8_220_109left_244ChamferBegin_TLineFP[2] - cam0_8_220_gear7chiding[2];
	AxialD_202_7 = AxialD_202_7 * Calik;
	resultVectorList.push_back(AxialD_202_7);

	double AxialD_202_6 = cam0_8_220_208Left_TLineFP[2] - cam0_8_220_gear6chiding[2];
	AxialD_202_6 = AxialD_202_6 * Calik;
	resultVectorList.push_back(AxialD_202_6);

	double AxialD_202_5 = cam0_8_220_207_TLineFP[2] - cam0_8_220_gear5chiding[2];
	AxialD_202_5 = AxialD_202_5 * Calik;
	resultVectorList.push_back(AxialD_202_5);

	double AxialD_202_4 = cam0_8_220_201Left_TLineFP[2] - cam0_8_220_gear4chiding[2];
	AxialD_202_4 = AxialD_202_4 * Calik;
	resultVectorList.push_back(AxialD_202_4);

	double AxialD_202_3 = cam0_8_220_199Left_TLineFP[2] - cam0_8_220_gear3chiding[2];
	AxialD_202_3 = AxialD_202_3 * Calik;
	resultVectorList.push_back(AxialD_202_3);

	double AxialD_202_2 = cam0_8_220_206Left_TLineFP[2] - cam0_8_220_gear2chiding[2];
	AxialD_202_2 = AxialD_202_2 * Calik;
	resultVectorList.push_back(AxialD_202_2);

	double AxialD_202_1 = cam0_8_220_205Left_TLineFP[2] - cam0_8_220_gear1chiding[2];
	AxialD_202_1 = AxialD_202_1 * Calik;
	resultVectorList.push_back(AxialD_202_1);
	pushback_vectors(resultVectorList, 4, 71, 0.27, -0.05, +0.05);

	//求角度
	double AxialD_204_1 = atan((cam0_8_220_200Gear1LeftChamfer198End_204Gear1_TLineFP[2] - cam0_8_220_200Gear1LeftChamfer198End_204Gear1_TLineFP[0]) / (cam0_8_220_200Gear1LeftChamfer198End_204Gear1_TLineFP[3] - cam0_8_220_200Gear1LeftChamfer198End_204Gear1_TLineFP[1])) * 180 / PI;
	resultVectorList.push_back(AxialD_204_1);

	double AxialD_204_2 = atan((cam0_8_220_200Gear2LeftChamferEnd_204Gear2_TLineFP[2] - cam0_8_220_200Gear2LeftChamferEnd_204Gear2_TLineFP[0]) / (cam0_8_220_200Gear2LeftChamferEnd_204Gear2_TLineFP[3] - cam0_8_220_200Gear2LeftChamferEnd_204Gear2_TLineFP[1])) * 180 / PI;
	resultVectorList.push_back(AxialD_204_2);

	double AxialD_204_3 = atan((cam0_8_220_200Gear3LeftChamferEnd_204Gear3_TLineFP[2] - cam0_8_220_200Gear3LeftChamferEnd_204Gear3_TLineFP[0]) / (cam0_8_220_200Gear3LeftChamferEnd_204Gear3_TLineFP[3] - cam0_8_220_200Gear3LeftChamferEnd_204Gear3_TLineFP[1])) * 180 / PI;
	resultVectorList.push_back(AxialD_204_3);

	double AxialD_204_4 = atan((cam0_8_220_200Gear4LeftChamferEnd_204Gear4_TLineFP[2] - cam0_8_220_200Gear4LeftChamferEnd_204Gear4_TLineFP[0]) / (cam0_8_220_200Gear4LeftChamferEnd_204Gear4_TLineFP[3] - cam0_8_220_200Gear4LeftChamferEnd_204Gear4_TLineFP[1])) * 180 / PI;
	resultVectorList.push_back(AxialD_204_4);

	double AxialD_204_5 = atan((cam0_8_220_200Gear5LeftChamferEnd_204Gear5_TLineFP[2] - cam0_8_220_200Gear5LeftChamferEnd_204Gear5_TLineFP[0]) / (cam0_8_220_200Gear5LeftChamferEnd_204Gear5_TLineFP[3] - cam0_8_220_200Gear5LeftChamferEnd_204Gear5_TLineFP[1])) * 180 / PI;
	resultVectorList.push_back(AxialD_204_5);

	double AxialD_204_6 = atan((cam0_8_220_200Gear6LeftChamferEnd_204Gear6_TLineFP[2] - cam0_8_220_200Gear6LeftChamferEnd_204Gear6_TLineFP[0]) / (cam0_8_220_200Gear6LeftChamferEnd_204Gear6_TLineFP[3] - cam0_8_220_200Gear6LeftChamferEnd_204Gear6_TLineFP[1])) * 180 / PI;
	resultVectorList.push_back(AxialD_204_6);

	double AxialD_204_7 = atan((cam0_8_220_200Gear7LeftChamferEnd_204Gear7_TLineFP[2] - cam0_8_220_200Gear7LeftChamferEnd_204Gear7_TLineFP[0]) / (cam0_8_220_200Gear7LeftChamferEnd_204Gear7_TLineFP[3] - cam0_8_220_200Gear7LeftChamferEnd_204Gear7_TLineFP[1])) * 180 / PI;
	resultVectorList.push_back(AxialD_204_7);
	pushback_vectors(resultVectorList, 3, 69, 14.8333, -1, +1);

};
void program_5::cam0Picture9_algorithm(string imgPath)//远心相机9图像算法
{
	HTuple halconPath = imgPath.c_str();

	//"cam0_9-460-132.sbm"
	//cam0_9_220_132ChamferBegin_TLineFP()
	double cam0_9_220_132ChamferBegin_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_9-220-132.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -500;
		hv_Line[1] = -70;
		hv_Line[2] = -300;
		hv_Line[3] = -70;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50,
			HTuple(), HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.6,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_9_220_132ChamferBegin_TLineFP[0] = hv_RowBegin[0].D();
		cam0_9_220_132ChamferBegin_TLineFP[1] = hv_ColBegin[0].D();
		cam0_9_220_132ChamferBegin_TLineFP[2] = hv_RowEnd[0].D();
		cam0_9_220_132ChamferBegin_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_9_220_132ChamferBegin_TLineFP():" << cam0_9_220_132ChamferBegin_TLineFP[0] << "  " << cam0_9_220_132ChamferBegin_TLineFP[1] << "  " << cam0_9_220_132ChamferBegin_TLineFP[2] << "  " << cam0_9_220_132ChamferBegin_TLineFP[3] << endl;

	};

	//cam0_9_220_133LeftChamferEnd_132ChamferEnd_123ChamferBegin_111Left_TLineFP()
	double cam0_9_220_133LeftChamferEnd_132ChamferEnd_123ChamferBegin_111Left_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_9-220-132.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -50;
		hv_Line[1] = 70;
		hv_Line[2] = 120;
		hv_Line[3] = 70;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.5,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_9_220_133LeftChamferEnd_132ChamferEnd_123ChamferBegin_111Left_TLineFP[0] = hv_RowBegin[0].D();
		cam0_9_220_133LeftChamferEnd_132ChamferEnd_123ChamferBegin_111Left_TLineFP[1] = hv_ColBegin[0].D();
		cam0_9_220_133LeftChamferEnd_132ChamferEnd_123ChamferBegin_111Left_TLineFP[2] = hv_RowEnd[0].D();
		cam0_9_220_133LeftChamferEnd_132ChamferEnd_123ChamferBegin_111Left_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_9_220_133LeftChamferEnd_132ChamferEnd_123ChamferBegin_111Left_TLineFP():" << cam0_9_220_133LeftChamferEnd_132ChamferEnd_123ChamferBegin_111Left_TLineFP[0] << "  " << cam0_9_220_133LeftChamferEnd_132ChamferEnd_123ChamferBegin_111Left_TLineFP[1] << "  " << cam0_9_220_133LeftChamferEnd_132ChamferEnd_123ChamferBegin_111Left_TLineFP[2] << "  " << cam0_9_220_133LeftChamferEnd_132ChamferEnd_123ChamferBegin_111Left_TLineFP[3] << endl;

	};

	//以下函数传递
	//cam0_9_220_133LeftChamfer()
	double cam0_9_220_133LeftChamfer;
	{

		// Local iconic variables
		HObject  ho_Image, ho_Rectangle, ho_ImageReduced;
		HObject  ho_Edges;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_Row, hv_Column;
		HTuple  hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;

		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		//dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
		//
		//Segment a region containing the edges
		//基于全局阈值的图像快速阈值化
		//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin 500与colend 3500可固定不变,-25是固定不变的
		GenRectangle1(&ho_Rectangle, cam0_9_220_133LeftChamferEnd_132ChamferEnd_123ChamferBegin_111Left_TLineFP[0] - 21, 500, cam0_9_220_133LeftChamferEnd_132ChamferEnd_123ChamferBegin_111Left_TLineFP[0] + 5, 3500);
		//提取roi区域，roi以外的变黑，不改变整个图像的尺寸大小。corp_domain是只提取roi区域
		ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);

		//In the subdomain of the image containing the edges,
		//extract subpixel precise edges.
		//提取亚像素精密边缘轮廓
		EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		FitCircleContourXld(ho_Edges, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		cam0_9_220_133LeftChamfer = hv_Radius.D();
		cout << "cam0_9_220_133LeftChamfer():" << cam0_9_220_133LeftChamfer << " " << cam0_9_220_133LeftChamfer * 0.01216 << endl;
		//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
		//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
		if (HDevWindowStack::IsOpen())
			SetColor(HDevWindowStack::GetActive(), "green");
		if (HDevWindowStack::IsOpen())
			DispObj(ho_Edges, HDevWindowStack::GetActive());

	};

	//以下函数传递
	//cam0_9_220_132Chamfer()
	double cam0_9_220_132Chamfer;
	{

		// Local iconic variables
		HObject  ho_Image, ho_Rectangle, ho_ImageReduced;
		HObject  ho_Edges;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_Row, hv_Column;
		HTuple  hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;

		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		//dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
		//
		//Segment a region containing the edges
		//基于全局阈值的图像快速阈值化
		//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin 500与colend 3500可固定不变
		GenRectangle1(&ho_Rectangle, cam0_9_220_132ChamferBegin_TLineFP[2], 500, cam0_9_220_133LeftChamferEnd_132ChamferEnd_123ChamferBegin_111Left_TLineFP[0] - 15, 3500);
		//提取roi区域，roi以外的变黑，不改变整个图像的尺寸大小。corp_domain是只提取roi区域
		ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);

		//In the subdomain of the image containing the edges,
		//extract subpixel precise edges.
		//提取亚像素精密边缘轮廓
		EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		FitCircleContourXld(ho_Edges, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		cam0_9_220_132Chamfer = hv_Radius.D();
		cout << "cam0_9_220_132Chamfer():" << cam0_9_220_132Chamfer << " " << cam0_9_220_132Chamfer * 0.01216 << endl;
		resultVectorList.push_back(cam0_9_220_132Chamfer * Calik);
		pushback_vectors(resultVectorList, 2, 87, 6.5, -1, +1);
		//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
		//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
		if (HDevWindowStack::IsOpen())
			SetColor(HDevWindowStack::GetActive(), "green");
		if (HDevWindowStack::IsOpen())
			DispObj(ho_Edges, HDevWindowStack::GetActive());

	};

	//cam0_9_220_133RightChamferEnd_123ChamferEnd_111Right_TLineFP()
	double cam0_9_220_133RightChamferEnd_123ChamferEnd_111Right_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_9-220-132.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = 180;
		hv_Line[1] = 100;
		hv_Line[2] = 380;
		hv_Line[3] = 100;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.6,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_9_220_133RightChamferEnd_123ChamferEnd_111Right_TLineFP[0] = hv_RowBegin[0].D();
		cam0_9_220_133RightChamferEnd_123ChamferEnd_111Right_TLineFP[1] = hv_ColBegin[0].D();
		cam0_9_220_133RightChamferEnd_123ChamferEnd_111Right_TLineFP[2] = hv_RowEnd[0].D();
		cam0_9_220_133RightChamferEnd_123ChamferEnd_111Right_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_9_220_133RightChamferEnd_123ChamferEnd_111Right_TLineFP():" << cam0_9_220_133RightChamferEnd_123ChamferEnd_111Right_TLineFP[0] << "  " << cam0_9_220_133RightChamferEnd_123ChamferEnd_111Right_TLineFP[1] << "  " << cam0_9_220_133RightChamferEnd_123ChamferEnd_111Right_TLineFP[2] << "  " << cam0_9_220_133RightChamferEnd_123ChamferEnd_111Right_TLineFP[3] << endl;

	};

	//以下函数传递
	//cam0_9_220_123Chamfer()
	double cam0_9_220_123Chamfer;
	{

		// Local iconic variables
		HObject  ho_Image, ho_Rectangle, ho_ImageReduced;
		HObject  ho_Edges;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_Row, hv_Column;
		HTuple  hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;

		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		//dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
		//
		//Segment a region containing the edges
		//基于全局阈值的图像快速阈值化
		//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin 500与colend 3500可固定不变
		GenRectangle1(&ho_Rectangle, cam0_9_220_133LeftChamferEnd_132ChamferEnd_123ChamferBegin_111Left_TLineFP[2], 500, cam0_9_220_133RightChamferEnd_123ChamferEnd_111Right_TLineFP[0] - 15, 3500);
		//提取roi区域，roi以外的变黑，不改变整个图像的尺寸大小。corp_domain是只提取roi区域
		ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);

		//In the subdomain of the image containing the edges,
		//extract subpixel precise edges.
		//提取亚像素精密边缘轮廓
		EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		FitCircleContourXld(ho_Edges, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		cam0_9_220_123Chamfer = hv_Radius.D();
		cout << "cam0_9_220_123Chamfer():" << cam0_9_220_123Chamfer << " " << cam0_9_220_123Chamfer * 0.01216 << endl;
		resultVectorList.push_back(cam0_9_220_123Chamfer * Calik);
		pushback_vectors(resultVectorList, 2, 88, 1.5, -0.3, +0.3);
		//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
		//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
		if (HDevWindowStack::IsOpen())
			SetColor(HDevWindowStack::GetActive(), "green");
		if (HDevWindowStack::IsOpen())
			DispObj(ho_Edges, HDevWindowStack::GetActive());

	};

	//以下函数传递
	//cam0_9_220_133RightChamfer()
	double cam0_9_220_133RightChamfer;
	{

		// Local iconic variables
		HObject  ho_Image, ho_Rectangle, ho_ImageReduced;
		HObject  ho_Edges;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_Row, hv_Column;
		HTuple  hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;

		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		//dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
		//
		//Segment a region containing the edges
		//基于全局阈值的图像快速阈值化
		//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin 500与colend 3500可固定不变
		GenRectangle1(&ho_Rectangle, cam0_9_220_133RightChamferEnd_123ChamferEnd_111Right_TLineFP[0] - 16, 500, cam0_9_220_133RightChamferEnd_123ChamferEnd_111Right_TLineFP[0] + 10, 3500);
		//提取roi区域，roi以外的变黑，不改变整个图像的尺寸大小。corp_domain是只提取roi区域
		ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);

		//In the subdomain of the image containing the edges,
		//extract subpixel precise edges.
		//提取亚像素精密边缘轮廓
		EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		FitCircleContourXld(ho_Edges, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		cam0_9_220_133RightChamfer = hv_Radius.D();
		cout << "cam0_9_220_133RightChamfer():" << cam0_9_220_133RightChamfer << " " << cam0_9_220_133RightChamfer * 0.01216 << endl;
		resultVectorList.push_back(cam0_9_220_133RightChamfer * Calik);
		resultVectorList.push_back(cam0_9_220_133LeftChamfer * Calik);
		pushback_vectors(resultVectorList, 2, 92, 0.2, -0.1, +0.1);
		//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
		//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
		if (HDevWindowStack::IsOpen())
			SetColor(HDevWindowStack::GetActive(), "green");
		if (HDevWindowStack::IsOpen())
			DispObj(ho_Edges, HDevWindowStack::GetActive());

	};

	//"cam0_9-460-124.sbm"
	//cam0_9_220_124ChamferBegin_122Left_125ChamferBegin_TLineFP()
	double cam0_9_220_124ChamferBegin_122Left_125ChamferBegin_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_9-220-124.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -400;
		hv_Line[1] = 100;
		hv_Line[2] = -150;
		hv_Line[3] = 100;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 30, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.5,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_9_220_124ChamferBegin_122Left_125ChamferBegin_TLineFP[0] = hv_RowBegin[0].D();
		cam0_9_220_124ChamferBegin_122Left_125ChamferBegin_TLineFP[1] = hv_ColBegin[0].D();
		cam0_9_220_124ChamferBegin_122Left_125ChamferBegin_TLineFP[2] = hv_RowEnd[0].D();
		cam0_9_220_124ChamferBegin_122Left_125ChamferBegin_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_9_220_124ChamferBegin_122Left_125ChamferBegin_TLineFP():" << cam0_9_220_124ChamferBegin_122Left_125ChamferBegin_TLineFP[0] << "  " << cam0_9_220_124ChamferBegin_122Left_125ChamferBegin_TLineFP[1] << "  " << cam0_9_220_124ChamferBegin_122Left_125ChamferBegin_TLineFP[2] << "  " << cam0_9_220_124ChamferBegin_122Left_125ChamferBegin_TLineFP[3] << endl;

	};


	//cam0_9_220_125ChamferEnd_TLineFP()
	double cam0_9_220_125ChamferEnd_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_9-220-124.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = 140;
		hv_Line[1] = -50;
		hv_Line[2] = 450;
		hv_Line[3] = -50;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50,
			HTuple(), HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.5,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_9_220_125ChamferEnd_TLineFP[0] = hv_RowBegin[0].D();
		cam0_9_220_125ChamferEnd_TLineFP[1] = hv_ColBegin[0].D();
		cam0_9_220_125ChamferEnd_TLineFP[2] = hv_RowEnd[0].D();
		cam0_9_220_125ChamferEnd_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_9_220_125ChamferEnd_TLineFP():" << cam0_9_220_125ChamferEnd_TLineFP[0] << "  " << cam0_9_220_125ChamferEnd_TLineFP[1] << "  " << cam0_9_220_125ChamferEnd_TLineFP[2] << "  " << cam0_9_220_125ChamferEnd_TLineFP[3] << endl;

	};

	//以下函数传递
	//cam0_9_220_125Chamfer()
	double cam0_9_220_125Chamfer;
	{

		// Local iconic variables
		HObject  ho_Image, ho_Rectangle, ho_ImageReduced;
		HObject  ho_Edges;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_Row, hv_Column;
		HTuple  hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;

		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		//dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
		//
		//Segment a region containing the edges
		//基于全局阈值的图像快速阈值化
		//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin 500与colend 3500可固定不变
		GenRectangle1(&ho_Rectangle, cam0_9_220_124ChamferBegin_122Left_125ChamferBegin_TLineFP[2] + 8, 500, cam0_9_220_125ChamferEnd_TLineFP[0], 3500);
		//提取roi区域，roi以外的变黑，不改变整个图像的尺寸大小。corp_domain是只提取roi区域
		ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);

		//In the subdomain of the image containing the edges,
		//extract subpixel precise edges.
		//提取亚像素精密边缘轮廓
		EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		FitCircleContourXld(ho_Edges, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		cam0_9_220_125Chamfer = hv_Radius.D();
		cout << "cam0_9_220_125Chamfer():" << cam0_9_220_125Chamfer << " " << cam0_9_220_125Chamfer * 0.01216 << endl;
		resultVectorList.push_back(cam0_9_220_125Chamfer * Calik);
		pushback_vectors(resultVectorList, 2, 89, 6.5, -0.5, +0.5);
		//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
		//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
		if (HDevWindowStack::IsOpen())
			SetColor(HDevWindowStack::GetActive(), "green");
		if (HDevWindowStack::IsOpen())
			DispObj(ho_Edges, HDevWindowStack::GetActive());

	};

	//"cam0_9-460-113.sbm"
	//cam0_9_220_136ChamferBegin_TLineFP()
	double cam0_9_220_136ChamferBegin_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_9-220-113.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -600;
		hv_Line[1] = -100;
		hv_Line[2] = -370;
		hv_Line[3] = -100;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50,
			HTuple(), HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.5,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_9_220_136ChamferBegin_TLineFP[0] = hv_RowBegin[0].D();
		cam0_9_220_136ChamferBegin_TLineFP[1] = hv_ColBegin[0].D();
		cam0_9_220_136ChamferBegin_TLineFP[2] = hv_RowEnd[0].D();
		cam0_9_220_136ChamferBegin_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_9_220_136ChamferBegin_TLineFP():" << cam0_9_220_136ChamferBegin_TLineFP[0] << "  " << cam0_9_220_136ChamferBegin_TLineFP[1] << "  " << cam0_9_220_136ChamferBegin_TLineFP[2] << "  " << cam0_9_220_136ChamferBegin_TLineFP[3] << endl;

	};

	//cam0_9_220_136ChamferEnd_TLineFP()
	double cam0_9_220_136ChamferEnd_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_9-220-113.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -280;
		hv_Line[1] = -40;
		hv_Line[2] = -280;
		hv_Line[3] = 90;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 30, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.5,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_9_220_136ChamferEnd_TLineFP[0] = hv_RowBegin[0].D();
		cam0_9_220_136ChamferEnd_TLineFP[1] = hv_ColBegin[0].D();
		cam0_9_220_136ChamferEnd_TLineFP[2] = hv_RowEnd[0].D();
		cam0_9_220_136ChamferEnd_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_9_220_136ChamferEnd_TLineFP():" << cam0_9_220_136ChamferEnd_TLineFP[0] << "  " << cam0_9_220_136ChamferEnd_TLineFP[1] << "  " << cam0_9_220_136ChamferEnd_TLineFP[2] << "  " << cam0_9_220_136ChamferEnd_TLineFP[3] << endl;

	};

	//以下函数传递
	//cam0_9_220_136Chamfer()
	double cam0_9_220_136Chamfer;
	{

		// Local iconic variables
		HObject  ho_Image, ho_Rectangle, ho_ImageReduced;
		HObject  ho_Edges;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_Row, hv_Column;
		HTuple  hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;

		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		//dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
		//
		//Segment a region containing the edges
		//基于全局阈值的图像快速阈值化
		//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin 500与colend 3500可固定不变
		GenRectangle1(&ho_Rectangle, cam0_9_220_136ChamferBegin_TLineFP[2], 500, cam0_9_220_136ChamferEnd_TLineFP[0], 3500);
		//提取roi区域，roi以外的变黑，不改变整个图像的尺寸大小。corp_domain是只提取roi区域
		ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);

		//In the subdomain of the image containing the edges,
		//extract subpixel precise edges.
		//提取亚像素精密边缘轮廓
		EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		FitCircleContourXld(ho_Edges, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		cam0_9_220_136Chamfer = hv_Radius.D();
		cout << "cam0_9_220_136Chamfer():" << cam0_9_220_136Chamfer << " " << cam0_9_220_136Chamfer * 0.01216 << endl;

		//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
		//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
		if (HDevWindowStack::IsOpen())
			SetColor(HDevWindowStack::GetActive(), "green");
		if (HDevWindowStack::IsOpen())
			DispObj(ho_Edges, HDevWindowStack::GetActive());

	};

	//cam0_9_220_113Right_138RightChamfer_116Left_TLineFP()
	double cam0_9_220_113Right_138RightChamfer_116Left_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_9-220-113.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = 260;
		hv_Line[1] = 40;
		hv_Line[2] = 140;
		hv_Line[3] = 150;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 40, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.5,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_9_220_113Right_138RightChamfer_116Left_TLineFP[0] = hv_RowBegin[0].D();
		cam0_9_220_113Right_138RightChamfer_116Left_TLineFP[1] = hv_ColBegin[0].D();
		cam0_9_220_113Right_138RightChamfer_116Left_TLineFP[2] = hv_RowEnd[0].D();
		cam0_9_220_113Right_138RightChamfer_116Left_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_9_220_113Right_138RightChamfer_116Left_TLineFP():" << cam0_9_220_113Right_138RightChamfer_116Left_TLineFP[0] << "  " << cam0_9_220_113Right_138RightChamfer_116Left_TLineFP[1] << "  " << cam0_9_220_113Right_138RightChamfer_116Left_TLineFP[2] << "  " << cam0_9_220_113Right_138RightChamfer_116Left_TLineFP[3] << endl;

	};

	//cam0_9_220_138LeftChamfer_TLineFP()
	double cam0_9_220_138LeftChamfer_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_9-220-113.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -300;
		hv_Line[1] = 50;
		hv_Line[2] = -190;
		hv_Line[3] = 150;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 40, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.5,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_9_220_138LeftChamfer_TLineFP[0] = hv_RowBegin[0].D();
		cam0_9_220_138LeftChamfer_TLineFP[1] = hv_ColBegin[0].D();
		cam0_9_220_138LeftChamfer_TLineFP[2] = hv_RowEnd[0].D();
		cam0_9_220_138LeftChamfer_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_9_220_138LeftChamfer_TLineFP():" << cam0_9_220_138LeftChamfer_TLineFP[0] << "  " << cam0_9_220_138LeftChamfer_TLineFP[1] << "  " << cam0_9_220_138LeftChamfer_TLineFP[2] << "  " << cam0_9_220_138LeftChamfer_TLineFP[3] << endl;

	};

	//cam0_9_220_137ChamferBegin_TLineFP()
	double cam0_9_220_137ChamferBegin_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_9-220-113.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = 240;
		hv_Line[1] = -50;
		hv_Line[2] = 240;
		hv_Line[3] = 90;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 30, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.5,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_9_220_137ChamferBegin_TLineFP[0] = hv_RowBegin[0].D();
		cam0_9_220_137ChamferBegin_TLineFP[1] = hv_ColBegin[0].D();
		cam0_9_220_137ChamferBegin_TLineFP[2] = hv_RowEnd[0].D();
		cam0_9_220_137ChamferBegin_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_9_220_137ChamferBegin_TLineFP():" << cam0_9_220_137ChamferBegin_TLineFP[0] << "  " << cam0_9_220_137ChamferBegin_TLineFP[1] << "  " << cam0_9_220_137ChamferBegin_TLineFP[2] << "  " << cam0_9_220_137ChamferBegin_TLineFP[3] << endl;

	};

	//cam0_9_220_137ChamferEnd_TLineFP()
	double cam0_9_220_137ChamferEnd_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_9-220-113.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = 310;
		hv_Line[1] = -120;
		hv_Line[2] = 500;
		hv_Line[3] = -120;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50,
			HTuple(), HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.5,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_9_220_137ChamferEnd_TLineFP[0] = hv_RowBegin[0].D();
		cam0_9_220_137ChamferEnd_TLineFP[1] = hv_ColBegin[0].D();
		cam0_9_220_137ChamferEnd_TLineFP[2] = hv_RowEnd[0].D();
		cam0_9_220_137ChamferEnd_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_9_220_137ChamferEnd_TLineFP():" << cam0_9_220_137ChamferEnd_TLineFP[0] << "  " << cam0_9_220_137ChamferEnd_TLineFP[1] << "  " << cam0_9_220_137ChamferEnd_TLineFP[2] << "  " << cam0_9_220_137ChamferEnd_TLineFP[3] << endl;

	};

	//以下函数传递
	//cam0_9_220_137Chamfer()
	double cam0_9_220_137Chamfer;
	{

		// Local iconic variables
		HObject  ho_Image, ho_Rectangle, ho_ImageReduced;
		HObject  ho_Edges;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_Row, hv_Column;
		HTuple  hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;

		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		//dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
		//
		//Segment a region containing the edges
		//基于全局阈值的图像快速阈值化
		//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin 500与colend 3500可固定不变
		if (cam0_9_220_137ChamferBegin_TLineFP[0] < cam0_9_220_137ChamferBegin_TLineFP[2]) {
			GenRectangle1(&ho_Rectangle, cam0_9_220_137ChamferBegin_TLineFP[2] + 6, 500, cam0_9_220_137ChamferEnd_TLineFP[0], 3500);
		}
		else {
			GenRectangle1(&ho_Rectangle, cam0_9_220_137ChamferBegin_TLineFP[0] + 6, 500, cam0_9_220_137ChamferEnd_TLineFP[0], 3500);
		}
		//提取roi区域，roi以外的变黑，不改变整个图像的尺寸大小。corp_domain是只提取roi区域
		ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);

		//In the subdomain of the image containing the edges,
		//extract subpixel precise edges.
		//提取亚像素精密边缘轮廓
		EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		FitCircleContourXld(ho_Edges, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		cam0_9_220_137Chamfer = hv_Radius.D();
		cout << "cam0_9_220_137Chamfer():" << cam0_9_220_137Chamfer << " " << cam0_9_220_137Chamfer * 0.01216 << endl;
		resultVectorList.push_back(cam0_9_220_137Chamfer * Calik);
		resultVectorList.push_back(cam0_9_220_136Chamfer * Calik);
		pushback_vectors(resultVectorList, 2, 94, 1, 0, +0.5);
		//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
		//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
		if (HDevWindowStack::IsOpen())
			SetColor(HDevWindowStack::GetActive(), "green");
		if (HDevWindowStack::IsOpen())
			DispObj(ho_Edges, HDevWindowStack::GetActive());

	};

	//以下为各个特征测量结果处理算法
	double AxialD_111 = cam0_9_220_133RightChamferEnd_123ChamferEnd_111Right_TLineFP[0] - cam0_9_220_133LeftChamferEnd_132ChamferEnd_123ChamferBegin_111Left_TLineFP[0];
	AxialD_111 = AxialD_111 * Calik;
	resultVectorList.push_back(AxialD_111);
	pushback_vectors(resultVectorList, 4, 7, 2.5, -0.12, +0.12);

	Row_122right = (cam0_9_220_137ChamferBegin_TLineFP[0] + cam0_9_220_137ChamferBegin_TLineFP[2]) / 2;
	double AxialD_122 = Row_122right - cam0_9_220_124ChamferBegin_122Left_125ChamferBegin_TLineFP[2];
	AxialD_122 = AxialD_122 * Calik;
	resultVectorList.push_back(AxialD_122);
	pushback_vectors(resultVectorList, 4, 18, 24.7, -0.26, +0.26);

	double AxialD_121 = cam0_9_220_124ChamferBegin_122Left_125ChamferBegin_TLineFP[2] - cam0_9_220_133RightChamferEnd_123ChamferEnd_111Right_TLineFP[0];
	AxialD_121 = AxialD_121 * Calik;
	resultVectorList.push_back(AxialD_121);
	pushback_vectors(resultVectorList, 4, 8, 15.2, -0.2, +0.2);

	double AxialD_113 = Row_122right - (cam0_9_220_136ChamferEnd_TLineFP[0] + cam0_9_220_136ChamferEnd_TLineFP[2]) / 2;
	AxialD_113 = AxialD_113 * Calik;
	resultVectorList.push_back(AxialD_113);
	pushback_vectors(resultVectorList, 4, 19, 6.35, -0.1, +0.1);

	double AxialD_109 = Row_122right - Row_109left;
	AxialD_109 = AxialD_109 * Calik + pic8To9_moveDistance;
	resultVectorList.push_back(AxialD_109);
	pushback_vectors(resultVectorList, 4, 20, 54.35, -0.5, +0.5);


	double AxialD_108 = Row_122right - Row_108left;
	AxialD_108 = AxialD_108 * Calik + pic8To9_moveDistance;
	resultVectorList.push_back(AxialD_108);
	pushback_vectors(resultVectorList, 4, 21, 80.25, -0.25, +0.25);

	double AxialD_106 = Row_122right - Row_61right;
	AxialD_106 = AxialD_106 * Calik + pic4To5_moveDistance + pic5To6_moveDistance + pic6To7_moveDistance + pic7To8_moveDistance + pic8To9_moveDistance;
	//resultVectorList.push_back(AxialD_106);
	//pushback_vectors(resultVectorList, 4, 15, 697.4, -0.15, +0.15);

	//补充
	double AxialD_new93_1size = ((cam0_9_220_138LeftChamfer_TLineFP[2] - cam0_9_220_138LeftChamfer_TLineFP[0]) +
		(cam0_9_220_138LeftChamfer_TLineFP[3] - cam0_9_220_138LeftChamfer_TLineFP[1])) / 2;
	AxialD_new93_1size = AxialD_new93_1size * Calik;
	resultVectorList.push_back(AxialD_new93_1size);

	double AxialD_new93_2size = ((cam0_9_220_113Right_138RightChamfer_116Left_TLineFP[3] - cam0_9_220_113Right_138RightChamfer_116Left_TLineFP[1]) +
		(cam0_9_220_113Right_138RightChamfer_116Left_TLineFP[0] - cam0_9_220_113Right_138RightChamfer_116Left_TLineFP[2])) / 2;
	AxialD_new93_2size = AxialD_new93_2size * Calik;
	resultVectorList.push_back(AxialD_new93_2size);
	pushback_vectors(resultVectorList, 2, 93, 0.95, -0.1, +0.1);

	double AxialD_new93_1angle = atan((cam0_9_220_138LeftChamfer_TLineFP[3] - cam0_9_220_138LeftChamfer_TLineFP[1]) / (cam0_9_220_138LeftChamfer_TLineFP[2] - cam0_9_220_138LeftChamfer_TLineFP[0])) * 180 / PI;
	AxialD_new93_1angle = AxialD_new93_1angle;
	resultVectorList.push_back(AxialD_new93_1angle);

	double AxialD_new93_2angle = atan((cam0_9_220_113Right_138RightChamfer_116Left_TLineFP[3] - cam0_9_220_113Right_138RightChamfer_116Left_TLineFP[1]) / (cam0_9_220_113Right_138RightChamfer_116Left_TLineFP[0] - cam0_9_220_113Right_138RightChamfer_116Left_TLineFP[2])) * 180 / PI;
	AxialD_new93_2angle = AxialD_new93_2angle;
	resultVectorList.push_back(AxialD_new93_2angle);
	pushback_vectors(resultVectorList, 3, 93.1, 45, -5, +5);

};
void program_5::cam0Picture10_algorithm(string imgPath)//远心相机10图像算法
{
	HTuple halconPath = imgPath.c_str();

	//"cam0_10-460-114.sbm"
	//cam0_10_220_126LeftChamferBegin_TLineFP()
	double cam0_10_220_126LeftChamferBegin_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_10-220-114.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -1200;
		hv_Line[1] = -20;
		hv_Line[2] = -860;
		hv_Line[3] = -20;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50,
			HTuple(), HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.5,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_10_220_126LeftChamferBegin_TLineFP[0] = hv_RowBegin[0].D();
		cam0_10_220_126LeftChamferBegin_TLineFP[1] = hv_ColBegin[0].D();
		cam0_10_220_126LeftChamferBegin_TLineFP[2] = hv_RowEnd[0].D();
		cam0_10_220_126LeftChamferBegin_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_10_220_126LeftChamferBegin_TLineFP():" << cam0_10_220_126LeftChamferBegin_TLineFP[0] << "  " << cam0_10_220_126LeftChamferBegin_TLineFP[1] << "  " << cam0_10_220_126LeftChamferBegin_TLineFP[2] << "  " << cam0_10_220_126LeftChamferBegin_TLineFP[3] << endl;

	};

	//cam0_10_220_126LeftChamferEnd_126RightChamferBegin_114_TLineFP()
	double cam0_10_220_126LeftChamferEnd_126RightChamferBegin_114_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_10-220-114.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -800;
		hv_Line[1] = 100;
		hv_Line[2] = 780;
		hv_Line[3] = 100;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 30, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.6,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_10_220_126LeftChamferEnd_126RightChamferBegin_114_TLineFP[0] = hv_RowBegin[0].D();
		cam0_10_220_126LeftChamferEnd_126RightChamferBegin_114_TLineFP[1] = hv_ColBegin[0].D();
		cam0_10_220_126LeftChamferEnd_126RightChamferBegin_114_TLineFP[2] = hv_RowEnd[0].D();
		cam0_10_220_126LeftChamferEnd_126RightChamferBegin_114_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_10_220_126LeftChamferEnd_126RightChamferBegin_114_TLineFP():" << cam0_10_220_126LeftChamferEnd_126RightChamferBegin_114_TLineFP[0] << "  " << cam0_10_220_126LeftChamferEnd_126RightChamferBegin_114_TLineFP[1] << "  " << cam0_10_220_126LeftChamferEnd_126RightChamferBegin_114_TLineFP[2] << "  " << cam0_10_220_126LeftChamferEnd_126RightChamferBegin_114_TLineFP[3] << endl;

	};

	//以下函数传递
	//cam0_10_220_126LeftChamfer()
	double cam0_10_220_126LeftChamfer;
	{

		// Local iconic variables
		HObject  ho_Image, ho_Rectangle, ho_ImageReduced;
		HObject  ho_Edges;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_Row, hv_Column;
		HTuple  hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;

		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		//dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
		//
		//Segment a region containing the edges
		//基于全局阈值的图像快速阈值化
		//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin 500与colend 3500可固定不变
		GenRectangle1(&ho_Rectangle, cam0_10_220_126LeftChamferBegin_TLineFP[2], 500, cam0_10_220_126LeftChamferEnd_126RightChamferBegin_114_TLineFP[0] - 5, 3500);
		//提取roi区域，roi以外的变黑，不改变整个图像的尺寸大小。corp_domain是只提取roi区域
		ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);

		//In the subdomain of the image containing the edges,
		//extract subpixel precise edges.
		//提取亚像素精密边缘轮廓
		EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		FitCircleContourXld(ho_Edges, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		cam0_10_220_126LeftChamfer = hv_Radius.D();
		cout << "cam0_10_220_126LeftChamfer():" << cam0_10_220_126LeftChamfer << " " << cam0_10_220_126LeftChamfer * 0.01216 << endl;


		//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
		//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
		if (HDevWindowStack::IsOpen())
			SetColor(HDevWindowStack::GetActive(), "green");
		if (HDevWindowStack::IsOpen())
			DispObj(ho_Edges, HDevWindowStack::GetActive());

	};

	//cam0_10_220_126RightChamferEnd_213LeftChamferBegin_TLineFP()
	double cam0_10_220_126RightChamferEnd_213LeftChamferBegin_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_10-220-114.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = 830;
		hv_Line[1] = -20;
		hv_Line[2] = 1420;
		hv_Line[3] = -20;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50,
			HTuple(), HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.6,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_10_220_126RightChamferEnd_213LeftChamferBegin_TLineFP[0] = hv_RowBegin[0].D();
		cam0_10_220_126RightChamferEnd_213LeftChamferBegin_TLineFP[1] = hv_ColBegin[0].D();
		cam0_10_220_126RightChamferEnd_213LeftChamferBegin_TLineFP[2] = hv_RowEnd[0].D();
		cam0_10_220_126RightChamferEnd_213LeftChamferBegin_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_10_220_126RightChamferEnd_213LeftChamferBegin_TLineFP():" << cam0_10_220_126RightChamferEnd_213LeftChamferBegin_TLineFP[0] << "  " << cam0_10_220_126RightChamferEnd_213LeftChamferBegin_TLineFP[1] << "  " << cam0_10_220_126RightChamferEnd_213LeftChamferBegin_TLineFP[2] << "  " << cam0_10_220_126RightChamferEnd_213LeftChamferBegin_TLineFP[3] << endl;

	};

	//以下函数传递
	//cam0_10_220_126RightChamfer()
	double cam0_10_220_126RightChamfer;
	{

		// Local iconic variables
		HObject  ho_Image, ho_Rectangle, ho_ImageReduced;
		HObject  ho_Edges;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_Row, hv_Column;
		HTuple  hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;

		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		//dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
		//
		//Segment a region containing the edges
		//基于全局阈值的图像快速阈值化
		//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin 500与colend 3500可固定不变
		GenRectangle1(&ho_Rectangle, cam0_10_220_126LeftChamferEnd_126RightChamferBegin_114_TLineFP[2] + 10, 500, cam0_10_220_126RightChamferEnd_213LeftChamferBegin_TLineFP[0], 3500);
		//提取roi区域，roi以外的变黑，不改变整个图像的尺寸大小。corp_domain是只提取roi区域
		ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);

		//In the subdomain of the image containing the edges,
		//extract subpixel precise edges.
		//提取亚像素精密边缘轮廓
		EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		FitCircleContourXld(ho_Edges, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		cam0_10_220_126RightChamfer = hv_Radius.D();
		cout << "cam0_10_220_126RightChamfer():" << cam0_10_220_126RightChamfer << " " << cam0_10_220_126RightChamfer * 0.01216 << endl;
		resultVectorList.push_back(cam0_10_220_126LeftChamfer * Calik);
		resultVectorList.push_back(cam0_10_220_126LeftChamfer * Calik);
		pushback_vectors(resultVectorList, 2, 95, 1.75, -0.5, +0);

		//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
		//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
		if (HDevWindowStack::IsOpen())
			SetColor(HDevWindowStack::GetActive(), "green");
		if (HDevWindowStack::IsOpen())
			DispObj(ho_Edges, HDevWindowStack::GetActive());

	};

	//"cam0_10-460-209.sbm"
	//cam0_10_220_209Left_Left_213LeftChamferEnd_TLineFP()
	double cam0_10_220_209Left_Left_213LeftChamferEnd_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_10-220-209.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -190;
		hv_Line[1] = -30;
		hv_Line[2] = -190;
		hv_Line[3] = 120;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 30, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.4,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_10_220_209Left_Left_213LeftChamferEnd_TLineFP[0] = hv_RowBegin[0].D();
		cam0_10_220_209Left_Left_213LeftChamferEnd_TLineFP[1] = hv_ColBegin[0].D();
		cam0_10_220_209Left_Left_213LeftChamferEnd_TLineFP[2] = hv_RowEnd[0].D();
		cam0_10_220_209Left_Left_213LeftChamferEnd_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_10_220_209Left_Left_213LeftChamferEnd_TLineFP():" << cam0_10_220_209Left_Left_213LeftChamferEnd_TLineFP[0] << "  " << cam0_10_220_209Left_Left_213LeftChamferEnd_TLineFP[1] << "  " << cam0_10_220_209Left_Left_213LeftChamferEnd_TLineFP[2] << "  " << cam0_10_220_209Left_Left_213LeftChamferEnd_TLineFP[3] << endl;

	};

	//以下函数传递
	//cam0_10_220_213LeftChamfer()
	double cam0_10_220_213LeftChamfer;
	{

		// Local iconic variables
		HObject  ho_Image, ho_Rectangle, ho_ImageReduced;
		HObject  ho_Edges;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_Row, hv_Column;
		HTuple  hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;

		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		//dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
		//
		//Segment a region containing the edges
		//基于全局阈值的图像快速阈值化
		//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin 500与colend 3500可固定不变
		if (cam0_10_220_209Left_Left_213LeftChamferEnd_TLineFP[2] < cam0_10_220_209Left_Left_213LeftChamferEnd_TLineFP[0]) {
			GenRectangle1(&ho_Rectangle, cam0_10_220_126RightChamferEnd_213LeftChamferBegin_TLineFP[2], 500, cam0_10_220_209Left_Left_213LeftChamferEnd_TLineFP[2] - 8, 3500);
		}
		else {
			GenRectangle1(&ho_Rectangle, cam0_10_220_126RightChamferEnd_213LeftChamferBegin_TLineFP[2], 500, cam0_10_220_209Left_Left_213LeftChamferEnd_TLineFP[0] - 8, 3500);
		}
		//提取roi区域，roi以外的变黑，不改变整个图像的尺寸大小。corp_domain是只提取roi区域
		ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);

		//In the subdomain of the image containing the edges,
		//extract subpixel precise edges.
		//提取亚像素精密边缘轮廓
		EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		FitCircleContourXld(ho_Edges, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);

		//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
		//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
		if (HDevWindowStack::IsOpen())
			SetColor(HDevWindowStack::GetActive(), "green");
		if (HDevWindowStack::IsOpen())
			DispObj(ho_Edges, HDevWindowStack::GetActive());
		cam0_10_220_213LeftChamfer = hv_Radius.D();
		cout << "cam0_10_220_213LeftChamfer():" << cam0_10_220_213LeftChamfer << " " << cam0_10_220_213LeftChamfer * 0.01216 << endl;


	};

	//cam0_10_220_209Left_Right_216ChamferBegin_TLineFP()
	double cam0_10_220_209Left_Right_216ChamferBegin_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_10-220-209.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -110;
		hv_Line[1] = 20;
		hv_Line[2] = -110;
		hv_Line[3] = 130;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 1, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.4,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_10_220_209Left_Right_216ChamferBegin_TLineFP[0] = hv_RowBegin[0].D();
		cam0_10_220_209Left_Right_216ChamferBegin_TLineFP[1] = hv_ColBegin[0].D();
		cam0_10_220_209Left_Right_216ChamferBegin_TLineFP[2] = hv_RowEnd[0].D();
		cam0_10_220_209Left_Right_216ChamferBegin_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_10_220_209Left_Right_216ChamferBegin_TLineFP():" << cam0_10_220_209Left_Right_216ChamferBegin_TLineFP[0] << "  " << cam0_10_220_209Left_Right_216ChamferBegin_TLineFP[1] << "  " << cam0_10_220_209Left_Right_216ChamferBegin_TLineFP[2] << "  " << cam0_10_220_209Left_Right_216ChamferBegin_TLineFP[3] << endl;

	};

	//cam0_10_220_163LeftChamferBegin_TLineFP()
	double cam0_10_220_163LeftChamferBegin_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_10-220-209.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -220;
		hv_Line[1] = 130;
		hv_Line[2] = -80;
		hv_Line[3] = 130;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 20, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.4,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_10_220_163LeftChamferBegin_TLineFP[0] = hv_RowBegin[0].D();
		cam0_10_220_163LeftChamferBegin_TLineFP[1] = hv_ColBegin[0].D();
		cam0_10_220_163LeftChamferBegin_TLineFP[2] = hv_RowEnd[0].D();
		cam0_10_220_163LeftChamferBegin_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_10_220_163LeftChamferBegin_TLineFP():" << cam0_10_220_163LeftChamferBegin_TLineFP[0] << "  " << cam0_10_220_163LeftChamferBegin_TLineFP[1] << "  " << cam0_10_220_163LeftChamferBegin_TLineFP[2] << "  " << cam0_10_220_163LeftChamferBegin_TLineFP[3] << endl;

	};

	//以下函数传递
	//cam0_10_220_163LeftChamfer()
	double cam0_10_220_163LeftChamfer;
	{

		// Local iconic variables
		HObject  ho_Image, ho_Rectangle, ho_ImageReduced;
		HObject  ho_Edges;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_Row, hv_Column;
		HTuple  hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;

		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		//dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
		//
		//Segment a region containing the edges
		//基于全局阈值的图像快速阈值化
		//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin 500与colend 3500可固定不变
		GenRectangle1(&ho_Rectangle, 2785, 1495, 2830, 1528);
		//提取roi区域，roi以外的变黑，不改变整个图像的尺寸大小。corp_domain是只提取roi区域
		ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);

		//In the subdomain of the image containing the edges,
		//extract subpixel precise edges.
		//提取亚像素精密边缘轮廓
		EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		FitCircleContourXld(ho_Edges, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		cam0_10_220_163LeftChamfer = hv_Radius.D();
		cout << "cam0_10_220_163LeftChamfer():" << cam0_10_220_163LeftChamfer << " " << cam0_10_220_163LeftChamfer * 0.01216 << endl;


		//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
		//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
		if (HDevWindowStack::IsOpen())
			SetColor(HDevWindowStack::GetActive(), "green");
		if (HDevWindowStack::IsOpen())
			DispObj(ho_Edges, HDevWindowStack::GetActive());

	};

	//cam0_10_220_209Right_Right_213RightChamferBegin_TLineFP()
	double cam0_10_220_209Right_Right_213RightChamferBegin_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_10-220-209.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = 170;
		hv_Line[1] = -30;
		hv_Line[2] = 170;
		hv_Line[3] = 120;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 30, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.4,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_10_220_209Right_Right_213RightChamferBegin_TLineFP[0] = hv_RowBegin[0].D();
		cam0_10_220_209Right_Right_213RightChamferBegin_TLineFP[1] = hv_ColBegin[0].D();
		cam0_10_220_209Right_Right_213RightChamferBegin_TLineFP[2] = hv_RowEnd[0].D();
		cam0_10_220_209Right_Right_213RightChamferBegin_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_10_220_209Right_Right_213RightChamferBegin_TLineFP():" << cam0_10_220_209Right_Right_213RightChamferBegin_TLineFP[0] << "  " << cam0_10_220_209Right_Right_213RightChamferBegin_TLineFP[1] << "  " << cam0_10_220_209Right_Right_213RightChamferBegin_TLineFP[2] << "  " << cam0_10_220_209Right_Right_213RightChamferBegin_TLineFP[3] << endl;

	};

	//cam0_10_220_209Right_Left_216ChamferEnd_TLineFP()
	double cam0_10_220_209Right_Left_216ChamferEnd_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_10-220-209.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = 50;
		hv_Line[1] = 20;
		hv_Line[2] = 50;
		hv_Line[3] = 110;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 1, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.4,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_10_220_209Right_Left_216ChamferEnd_TLineFP[0] = hv_RowBegin[0].D();
		cam0_10_220_209Right_Left_216ChamferEnd_TLineFP[1] = hv_ColBegin[0].D();
		cam0_10_220_209Right_Left_216ChamferEnd_TLineFP[2] = hv_RowEnd[0].D();
		cam0_10_220_209Right_Left_216ChamferEnd_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_10_220_209Right_Left_216ChamferEnd_TLineFP():" << cam0_10_220_209Right_Left_216ChamferEnd_TLineFP[0] << "  " << cam0_10_220_209Right_Left_216ChamferEnd_TLineFP[1] << "  " << cam0_10_220_209Right_Left_216ChamferEnd_TLineFP[2] << "  " << cam0_10_220_209Right_Left_216ChamferEnd_TLineFP[3] << endl;

	};

	//以下函数传递
	//cam0_10_220_216Chamfer()
	double cam0_10_220_216Chamfer;
	{

		// Local iconic variables
		HObject  ho_Image, ho_Rectangle, ho_ImageReduced;
		HObject  ho_Edges, ho_ContoursSplit, ho_SelectedContours;
		HObject  ho_UnionContours;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_Row, hv_Column;
		HTuple  hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;

		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		//dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
		//
		//Segment a region containing the edges
		//基于全局阈值的图像快速阈值化
		//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin 500与colend 3500可固定不变
		if (cam0_10_220_209Right_Left_216ChamferEnd_TLineFP[2] < cam0_10_220_209Right_Left_216ChamferEnd_TLineFP[0]) {
			if (cam0_10_220_209Left_Right_216ChamferBegin_TLineFP[2] < cam0_10_220_209Left_Right_216ChamferBegin_TLineFP[0]) {
				GenRectangle1(&ho_Rectangle, cam0_10_220_209Left_Right_216ChamferBegin_TLineFP[0], 500, cam0_10_220_209Right_Left_216ChamferEnd_TLineFP[2], 3500);
			}
			else {
				GenRectangle1(&ho_Rectangle, cam0_10_220_209Left_Right_216ChamferBegin_TLineFP[2], 500, cam0_10_220_209Right_Left_216ChamferEnd_TLineFP[2], 3500);
			}
		}
		else {
			if (cam0_10_220_209Left_Right_216ChamferBegin_TLineFP[2] < cam0_10_220_209Left_Right_216ChamferBegin_TLineFP[0]) {
				GenRectangle1(&ho_Rectangle, cam0_10_220_209Left_Right_216ChamferBegin_TLineFP[0], 500, cam0_10_220_209Right_Left_216ChamferEnd_TLineFP[0], 3500);
			}
			else {
				GenRectangle1(&ho_Rectangle, cam0_10_220_209Left_Right_216ChamferBegin_TLineFP[2], 500, cam0_10_220_209Right_Left_216ChamferEnd_TLineFP[0], 3500);
			}
		}

		//提取roi区域，roi以外的变黑，不改变整个图像的尺寸大小。corp_domain是只提取roi区域
		ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);

		//In the subdomain of the image containing the edges,
		//extract subpixel precise edges.
		//提取亚像素精密边缘轮廓
		EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);

		SegmentContoursXld(ho_Edges, &ho_ContoursSplit, "lines_circles", 5, 4, 2);
		//提取出轮廓中较长的部分线段
		SelectContoursXld(ho_ContoursSplit, &ho_SelectedContours, "contour_length", 20,
			hv_Width / 2, -0.5, 0.5);
		//对相邻的轮廓段进行连接
		UnionAdjacentContoursXld(ho_SelectedContours, &ho_UnionContours, 90, 1, "attr_keep");

		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		FitCircleContourXld(ho_UnionContours, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		cam0_10_220_216Chamfer = hv_Radius.D();
		cout << "cam0_10_220_216Chamfer():" << cam0_10_220_216Chamfer << " " << cam0_10_220_216Chamfer * 0.01216 << endl;
		resultVectorList.push_back(cam0_10_220_216Chamfer * Calik);
		pushback_vectors(resultVectorList, 2, 81, 0, -99, +99);

		//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
		//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
		if (HDevWindowStack::IsOpen())
			SetColor(HDevWindowStack::GetActive(), "green");
		if (HDevWindowStack::IsOpen())
			DispObj(ho_Edges, HDevWindowStack::GetActive());

	};

	//cam0_10_220_163RightChamferEnd_TLineFP()
	double cam0_10_220_163RightChamferEnd_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_10-220-209.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = 50;
		hv_Line[1] = 130;
		hv_Line[2] = 180;
		hv_Line[3] = 130;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 20, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.4,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_10_220_163RightChamferEnd_TLineFP[0] = hv_RowBegin[0].D();
		cam0_10_220_163RightChamferEnd_TLineFP[1] = hv_ColBegin[0].D();
		cam0_10_220_163RightChamferEnd_TLineFP[2] = hv_RowEnd[0].D();
		cam0_10_220_163RightChamferEnd_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_10_220_163RightChamferEnd_TLineFP():" << cam0_10_220_163RightChamferEnd_TLineFP[0] << "  " << cam0_10_220_163RightChamferEnd_TLineFP[1] << "  " << cam0_10_220_163RightChamferEnd_TLineFP[2] << "  " << cam0_10_220_163RightChamferEnd_TLineFP[3] << endl;

	};

	//以下函数传递
	//cam0_10_220_163RightChamfer()
	double cam0_10_220_163RightChamfer;
	{

		// Local iconic variables
		HObject  ho_Image, ho_Rectangle, ho_ImageReduced;
		HObject  ho_Edges;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_Row, hv_Column;
		HTuple  hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;

		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		//dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
		//
		//Segment a region containing the edges
		//基于全局阈值的图像快速阈值化
		//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin 500与colend 3500可固定不变

		GenRectangle1(&ho_Rectangle, 2950, 1495, 2990, 1528);
		//提取roi区域，roi以外的变黑，不改变整个图像的尺寸大小。corp_domain是只提取roi区域
		ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);

		//In the subdomain of the image containing the edges,
		//extract subpixel precise edges.
		//提取亚像素精密边缘轮廓
		EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		FitCircleContourXld(ho_Edges, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		cam0_10_220_163RightChamfer = hv_Radius.D();
		cout << "cam0_10_220_163RightChamfer():" << cam0_10_220_163RightChamfer << " " << cam0_10_220_163RightChamfer * 0.01216 << endl;
		resultVectorList.push_back(cam0_10_220_163RightChamfer * Calik);
		resultVectorList.push_back(cam0_10_220_163LeftChamfer * Calik);
		pushback_vectors(resultVectorList, 2, 79, 0, 0, +0.2);

		//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
		//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
		if (HDevWindowStack::IsOpen())
			SetColor(HDevWindowStack::GetActive(), "green");
		if (HDevWindowStack::IsOpen())
			DispObj(ho_Edges, HDevWindowStack::GetActive());

	};

	//cam0_10_220_213RightChamferEnd_TLineFP()
	double cam0_10_220_213RightChamferEnd_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_10-220-209.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = 250;
		hv_Line[1] = -80;
		hv_Line[2] = 440;
		hv_Line[3] = -80;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50,
			HTuple(), HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.2,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_10_220_213RightChamferEnd_TLineFP[0] = hv_RowBegin[0].D();
		cam0_10_220_213RightChamferEnd_TLineFP[1] = hv_ColBegin[0].D();
		cam0_10_220_213RightChamferEnd_TLineFP[2] = hv_RowEnd[0].D();
		cam0_10_220_213RightChamferEnd_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_10_220_213RightChamferEnd_TLineFP():" << cam0_10_220_213RightChamferEnd_TLineFP[0] << "  " << cam0_10_220_213RightChamferEnd_TLineFP[1] << "  " << cam0_10_220_213RightChamferEnd_TLineFP[2] << "  " << cam0_10_220_213RightChamferEnd_TLineFP[3] << endl;

	};

	//以下函数传递
	//cam0_10_220_213RightChamfer()
	double cam0_10_220_213RightChamfer;
	{

		// Local iconic variables
		HObject  ho_Image, ho_Rectangle, ho_ImageReduced;
		HObject  ho_Edges;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_Row, hv_Column;
		HTuple  hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;

		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		//dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
		//
		//Segment a region containing the edges
		//基于全局阈值的图像快速阈值化
		//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin 500与colend 3500可固定不变
		if (cam0_10_220_209Right_Right_213RightChamferBegin_TLineFP[2] < cam0_10_220_209Right_Right_213RightChamferBegin_TLineFP[0]) {
			GenRectangle1(&ho_Rectangle, cam0_10_220_209Right_Right_213RightChamferBegin_TLineFP[0] + 8, 500, cam0_10_220_213RightChamferEnd_TLineFP[0], 3500);
		}
		else {
			GenRectangle1(&ho_Rectangle, cam0_10_220_209Right_Right_213RightChamferBegin_TLineFP[2] + 8, 500, cam0_10_220_213RightChamferEnd_TLineFP[0], 3500);
		}
		//提取roi区域，roi以外的变黑，不改变整个图像的尺寸大小。corp_domain是只提取roi区域
		ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);

		//In the subdomain of the image containing the edges,
		//extract subpixel precise edges.
		//提取亚像素精密边缘轮廓
		EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		FitCircleContourXld(ho_Edges, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		cam0_10_220_213RightChamfer = hv_Radius.D();
		cout << "cam0_10_220_213RightChamfer():" << cam0_10_220_213RightChamfer << " " << cam0_10_220_213RightChamfer * 0.01216 << endl;
		resultVectorList.push_back(cam0_10_220_213RightChamfer * Calik);
		resultVectorList.push_back(cam0_10_220_213LeftChamfer * Calik);
		pushback_vectors(resultVectorList, 2, 80, 1, 0, +0.5);

		//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
		//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
		if (HDevWindowStack::IsOpen())
			SetColor(HDevWindowStack::GetActive(), "green");
		if (HDevWindowStack::IsOpen())
			DispObj(ho_Edges, HDevWindowStack::GetActive());

	};

	//以下为各个特征测量结果处理算法
	double AxialD_114 = cam0_10_220_126LeftChamferEnd_126RightChamferBegin_114_TLineFP[2] - cam0_10_220_126LeftChamferEnd_126RightChamferBegin_114_TLineFP[0];
	AxialD_114 = AxialD_114 * Calik;
	resultVectorList.push_back(AxialD_114);
	pushback_vectors(resultVectorList, 4, 9, 18.3, -0.25, +0.25);

	double AxialD_115 = cam0_10_220_126LeftChamferEnd_126RightChamferBegin_114_TLineFP[2] - Row_122right;
	AxialD_115 = AxialD_115 * Calik + pic9To10_moveDistance;
	resultVectorList.push_back(AxialD_115);
	pushback_vectors(resultVectorList, 4, 23, 27.9, -0.25, +0.25);

	//
	double Row_209_1_left = (cam0_10_220_209Left_Left_213LeftChamferEnd_TLineFP[0] + cam0_10_220_209Left_Left_213LeftChamferEnd_TLineFP[2]) / 2;
	double Row_209_1_right = (cam0_10_220_209Left_Right_216ChamferBegin_TLineFP[0] + cam0_10_220_209Left_Right_216ChamferBegin_TLineFP[2]) / 2;
	double Row_209_2_left = (cam0_10_220_209Right_Left_216ChamferEnd_TLineFP[0] + cam0_10_220_209Right_Left_216ChamferEnd_TLineFP[2]) / 2;
	double Row_209_2_right = (cam0_10_220_209Right_Right_213RightChamferBegin_TLineFP[0] + cam0_10_220_209Right_Right_213RightChamferBegin_TLineFP[2]) / 2;

	double cam0_10_220_76_1_left_TLineFP[4];
	Straight_TLineFP(cam0_10_220_76_1_left_TLineFP, imgPath,
		2709, 1440, 2709, 1498,
		30, 12, 1, 10);

	double cam0_10_220_76_1_right_TLineFP[4];
	Straight_TLineFP(cam0_10_220_76_1_right_TLineFP, imgPath,
		2809, 1470, 2809, 1518,
		30, 12, 1, 10);

	double cam0_10_220_76_2_left_TLineFP[4];
	Straight_TLineFP(cam0_10_220_76_2_left_TLineFP, imgPath,
		2974, 1470, 2974, 1518,
		30, 12, 1, 10);

	double cam0_10_220_76_2_right_TLineFP[4];
	Straight_TLineFP(cam0_10_220_76_2_right_TLineFP, imgPath,
		3076, 1440, 3076, 1498,
		30, 12, 1, 10);



	double Row_209_1 = cam0_10_220_76_1_right_TLineFP[2] - cam0_10_220_76_1_left_TLineFP[2] - 6.97;
	Row_209_1 = Row_209_1 * Calik;
	resultVectorList.push_back(Row_209_1);

	double Row_209_2 = cam0_10_220_76_2_right_TLineFP[2] - cam0_10_220_76_2_left_TLineFP[2] - 6.97;
	Row_209_2 = Row_209_2 * Calik;
	resultVectorList.push_back(Row_209_2);
	pushback_vectors(resultVectorList, 4, 76, 1.25, -0.2, 0);

	double Row_210 = cam0_10_220_76_2_left_TLineFP[2] - cam0_10_220_76_1_right_TLineFP[2] + 3.6;
	Row_210 = Row_210 * Calik;
	resultVectorList.push_back(Row_210);
	pushback_vectors(resultVectorList, 4, 78, 2.05, 0, +0.1);
	//
	double Row_116 = Row_209_1_left - Row_122right;
	Row_116 = Row_116 * Calik + pic9To10_moveDistance;
	resultVectorList.push_back(Row_116);
	pushback_vectors(resultVectorList, 4, 22, 37, -0.3, +0.3);

};
void program_5::cam0Picture11_algorithm(string imgPath)//远心相机11图像算法
{
	HTuple halconPath = imgPath.c_str();
	//"cam0_11-460-152左倒角.sbm"
	//cam0_11_220_152LeftChamferBegin_TLineFP()
	double cam0_11_220_152LeftChamferBegin_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_11-220-152左倒角.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -380;
		hv_Line[1] = -100;
		hv_Line[2] = -130;
		hv_Line[3] = -100;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50,
			HTuple(), HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.6,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
		//显示形状特征模板匹配结果


		VectorAngleToRigid(hv_ReusedRefPointRow, hv_ReusedRefPointCol, 0, hv_Row3, hv_Column3,
			hv_Angle, &hv_HomMat2D);
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
		cam0_11_220_152LeftChamferBegin_TLineFP[0] = hv_RowBegin[0].D();
		cam0_11_220_152LeftChamferBegin_TLineFP[1] = hv_ColBegin[0].D();
		cam0_11_220_152LeftChamferBegin_TLineFP[2] = hv_RowEnd[0].D();
		cam0_11_220_152LeftChamferBegin_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_11_220_152LeftChamferBegin_TLineFP():" << cam0_11_220_152LeftChamferBegin_TLineFP[0] << "  " << cam0_11_220_152LeftChamferBegin_TLineFP[1] << "  " << cam0_11_220_152LeftChamferBegin_TLineFP[2] << "  " << cam0_11_220_152LeftChamferBegin_TLineFP[3] << endl;
	};

	//cam0_11_220_152LeftChamferEnd_153LeftChamfer_164LeftChamferBegin_117Left_TLineFP()
	double cam0_11_220_152LeftChamferEnd_153LeftChamfer_164LeftChamferBegin_117Left_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_11-220-152左倒角.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -100;
		hv_Line[1] = -50;
		hv_Line[2] = 110;
		hv_Line[3] = 150;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 25, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.5,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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

		cam0_11_220_152LeftChamferEnd_153LeftChamfer_164LeftChamferBegin_117Left_TLineFP[0] = hv_RowBegin[0].D();
		cam0_11_220_152LeftChamferEnd_153LeftChamfer_164LeftChamferBegin_117Left_TLineFP[1] = hv_ColBegin[0].D();
		cam0_11_220_152LeftChamferEnd_153LeftChamfer_164LeftChamferBegin_117Left_TLineFP[2] = hv_RowEnd[0].D();
		cam0_11_220_152LeftChamferEnd_153LeftChamfer_164LeftChamferBegin_117Left_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_11_220_152LeftChamferEnd_153LeftChamfer_164LeftChamferBegin_117Left_TLineFP():" << cam0_11_220_152LeftChamferEnd_153LeftChamfer_164LeftChamferBegin_117Left_TLineFP[0] << "  " <<
			cam0_11_220_152LeftChamferEnd_153LeftChamfer_164LeftChamferBegin_117Left_TLineFP[1] <<
			"  " << cam0_11_220_152LeftChamferEnd_153LeftChamfer_164LeftChamferBegin_117Left_TLineFP[2] <<
			"  " << cam0_11_220_152LeftChamferEnd_153LeftChamfer_164LeftChamferBegin_117Left_TLineFP[3] << endl;
	};

	//以下函数传递
	//cam0_11_220_152LeftChamfer()
	double cam0_11_220_152LeftChamfer;
	{

		// Local iconic variables
		HObject  ho_Image, ho_Rectangle, ho_ImageReduced;
		HObject  ho_Edges;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_Row, hv_Column;
		HTuple  hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;

		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		//dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
		//
		//Segment a region containing the edges
		//基于全局阈值的图像快速阈值化
		//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin 500与colend 3500可固定不变
		GenRectangle1(&ho_Rectangle, cam0_11_220_152LeftChamferBegin_TLineFP[2], 500, cam0_11_220_152LeftChamferEnd_153LeftChamfer_164LeftChamferBegin_117Left_TLineFP[0] - 5, 3500);
		//提取roi区域，roi以外的变黑，不改变整个图像的尺寸大小。corp_domain是只提取roi区域
		ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);

		//In the subdomain of the image containing the edges,
		//extract subpixel precise edges.
		//提取亚像素精密边缘轮廓
		EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		FitCircleContourXld(ho_Edges, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		cam0_11_220_152LeftChamfer = hv_Radius.D();
		cout << "cam0_11_220_152LeftChamfer():" << cam0_11_220_152LeftChamfer << " " << cam0_11_220_152LeftChamfer * 0.01216 << endl;

		//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
		//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
		if (HDevWindowStack::IsOpen())
			SetColor(HDevWindowStack::GetActive(), "green");
		if (HDevWindowStack::IsOpen())
			DispObj(ho_Edges, HDevWindowStack::GetActive());

	};

	//cam0_11_220_164LeftChamferEnd_TLineFP()
	double cam0_11_220_164LeftChamferEnd_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_11-220-152左倒角.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = 60;
		hv_Line[1] = 100;
		hv_Line[2] = 360;
		hv_Line[3] = 100;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50,
			HTuple(), HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.5,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_11_220_164LeftChamferEnd_TLineFP[0] = hv_RowBegin[0].D();
		cam0_11_220_164LeftChamferEnd_TLineFP[1] = hv_ColBegin[0].D();
		cam0_11_220_164LeftChamferEnd_TLineFP[2] = hv_RowEnd[0].D();
		cam0_11_220_164LeftChamferEnd_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_11_220_164LeftChamferEnd_TLineFP():" << cam0_11_220_164LeftChamferEnd_TLineFP[0] << "  " << cam0_11_220_164LeftChamferEnd_TLineFP[1] << "  " << cam0_11_220_164LeftChamferEnd_TLineFP[2] << "  " << cam0_11_220_164LeftChamferEnd_TLineFP[3] << endl;
	};

	//以下函数传递
	//cam0_11_220_164LeftChamfer()
	double cam0_11_220_164LeftChamfer;
	{

		// Local iconic variables
		HObject  ho_Image, ho_Rectangle, ho_ImageReduced;
		HObject  ho_Edges;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_Row, hv_Column;
		HTuple  hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;

		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		//dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
		//
		//Segment a region containing the edges
		//基于全局阈值的图像快速阈值化
		//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin 500与colend 3500可固定不变
		GenRectangle1(&ho_Rectangle, cam0_11_220_152LeftChamferEnd_153LeftChamfer_164LeftChamferBegin_117Left_TLineFP[2], 500, cam0_11_220_164LeftChamferEnd_TLineFP[0], 3500);
		//提取roi区域，roi以外的变黑，不改变整个图像的尺寸大小。corp_domain是只提取roi区域
		ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);

		//In the subdomain of the image containing the edges,
		//extract subpixel precise edges.
		//提取亚像素精密边缘轮廓
		EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		FitCircleContourXld(ho_Edges, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		cam0_11_220_164LeftChamfer = hv_Radius.D();
		cout << "cam0_11_220_164LeftChamfer():" << cam0_11_220_164LeftChamfer << " " << cam0_11_220_164LeftChamfer * 0.01216 << endl;
		resultVectorList.push_back(cam0_11_220_164LeftChamfer * Calik);
		pushback_vectors(resultVectorList, 2, 91, 1, -0, +0.5);
		//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
		//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
		if (HDevWindowStack::IsOpen())
			SetColor(HDevWindowStack::GetActive(), "green");
		if (HDevWindowStack::IsOpen())
			DispObj(ho_Edges, HDevWindowStack::GetActive());

	};


	//"cam0_11-460-152右倒角.sbm"
	//cam0_11_220_152RightChamferBegin_153RightChamfer_117Right_TLineFP()
	double cam0_11_220_152RightChamferBegin_153RightChamfer_117Right_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_11-220-152右倒角.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = 100;
		hv_Line[1] = -50;
		hv_Line[2] = -110;
		hv_Line[3] = 150;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 25, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.6,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_11_220_152RightChamferBegin_153RightChamfer_117Right_TLineFP[0] = hv_RowBegin[0].D();
		cam0_11_220_152RightChamferBegin_153RightChamfer_117Right_TLineFP[1] = hv_ColBegin[0].D();
		cam0_11_220_152RightChamferBegin_153RightChamfer_117Right_TLineFP[2] = hv_RowEnd[0].D();
		cam0_11_220_152RightChamferBegin_153RightChamfer_117Right_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_11_220_152RightChamferBegin_153RightChamfer_117Right_TLineFP():" << cam0_11_220_152RightChamferBegin_153RightChamfer_117Right_TLineFP[0] << "  " << cam0_11_220_152RightChamferBegin_153RightChamfer_117Right_TLineFP[1] << "  " << cam0_11_220_152RightChamferBegin_153RightChamfer_117Right_TLineFP[2] << "  " << cam0_11_220_152RightChamferBegin_153RightChamfer_117Right_TLineFP[3] << endl;

	};

	//cam0_11_220_152RightChamferEnd_TLineFP()
	double cam0_11_220_152RightChamferEnd_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_11-220-152右倒角.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = 100;
		hv_Line[1] = -100;
		hv_Line[2] = 350;
		hv_Line[3] = -100;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.6,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_11_220_152RightChamferEnd_TLineFP[0] = hv_RowBegin[0].D();
		cam0_11_220_152RightChamferEnd_TLineFP[1] = hv_ColBegin[0].D();
		cam0_11_220_152RightChamferEnd_TLineFP[2] = hv_RowEnd[0].D();
		cam0_11_220_152RightChamferEnd_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_11_220_152RightChamferEnd_TLineFP():" << cam0_11_220_152RightChamferEnd_TLineFP[0] << "  " << cam0_11_220_152RightChamferEnd_TLineFP[1] << "  " << cam0_11_220_152RightChamferEnd_TLineFP[2] << "  " << cam0_11_220_152RightChamferEnd_TLineFP[3] << endl;

	};

	//以下函数传递
	//cam0_11_220_152RightChamfer()
	double cam0_11_220_152RightChamfer;
	{
		// Local iconic variables
		HObject  ho_Image, ho_Rectangle, ho_ImageReduced;
		HObject  ho_Edges;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_Row, hv_Column;
		HTuple  hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;

		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		//dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
		//
		//Segment a region containing the edges
		//基于全局阈值的图像快速阈值化
		//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin 500与colend 3500可固定不变
		GenRectangle1(&ho_Rectangle, cam0_11_220_152RightChamferBegin_153RightChamfer_117Right_TLineFP[0] + 5, 500, cam0_11_220_152RightChamferEnd_TLineFP[0], 3500);
		//提取roi区域，roi以外的变黑，不改变整个图像的尺寸大小。corp_domain是只提取roi区域
		ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);

		//In the subdomain of the image containing the edges,
		//extract subpixel precise edges.
		//提取亚像素精密边缘轮廓
		EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		FitCircleContourXld(ho_Edges, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		cam0_11_220_152RightChamfer = hv_Radius.D();
		cout << "cam0_11_220_152RightChamfer():" << cam0_11_220_152RightChamfer << " " << cam0_11_220_152RightChamfer * 0.01216 << endl;
		resultVectorList.push_back(cam0_11_220_152RightChamfer * Calik);
		resultVectorList.push_back(cam0_11_220_152LeftChamfer * Calik);
		pushback_vectors(resultVectorList, 2, 90, 0.6, -0.1, +0.1);
		//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
		//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
		if (HDevWindowStack::IsOpen())
			SetColor(HDevWindowStack::GetActive(), "green");
		if (HDevWindowStack::IsOpen())
			DispObj(ho_Edges, HDevWindowStack::GetActive());

	};

	//cam0_11_220_104Right_TLineFP()
	double cam0_11_220_104Right_TLineFP[4];
	{

		// Local iconic variables
		HObject  ho_Image, ho_ModelContours, ho_LineContours;
		HObject  ho_LineContour;

		// Local control variables
		HTuple  hv_Width, hv_Height, hv_ModelFile, hv_ReusedModelID;
		HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
		HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
		HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
		HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
		HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
		HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
		HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

		//读取模板和图像进行模板匹配*
		ReadImage(&ho_Image, halconPath);
		GetImageSize(ho_Image, &hv_Width, &hv_Height);
		hv_ModelFile = "./programParmeter/WZ10-45-1080-220/cam0_11-220-104right.sbm";
		ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
		//创建测量模型
		CreateMetrologyModel(&hv_MetrologyHandle);
		//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
		hv_Line.Clear();
		hv_Line[0] = -30;
		hv_Line[1] = -40;
		hv_Line[2] = -30;
		hv_Line[3] = 100;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 50, HTuple(),
			HTuple(), &hv_LineIndices);
		//获取读取的模板的轮廓，区域坐标等信息
		GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
		GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
		GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
			&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
		//进行模板匹配
		FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.6,
			1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
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
		cam0_11_220_104Right_TLineFP[0] = hv_RowBegin[0].D();
		cam0_11_220_104Right_TLineFP[1] = hv_ColBegin[0].D();
		cam0_11_220_104Right_TLineFP[2] = hv_RowEnd[0].D();
		cam0_11_220_104Right_TLineFP[3] = hv_ColEnd[0].D();
		cout << " cam0_11_220_104Right_TLineFP():" << cam0_11_220_104Right_TLineFP[0] << "  " << cam0_11_220_104Right_TLineFP[1] << "  " << cam0_11_220_104Right_TLineFP[2] << "  " << cam0_11_220_104Right_TLineFP[3] << endl;

	};

	//以下为各个特征测量结果处理算法
	double AxialD_117 = cam0_11_220_152RightChamferBegin_153RightChamfer_117Right_TLineFP[0] - cam0_11_220_152LeftChamferEnd_153LeftChamfer_164LeftChamferBegin_117Left_TLineFP[0];
	AxialD_117 = AxialD_117 * Calik;
	resultVectorList.push_back(AxialD_117);
	pushback_vectors(resultVectorList, 4, 11, 37.5, -0.5, +0.5);

	double AxialD_105 = cam0_11_220_152RightChamferBegin_153RightChamfer_117Right_TLineFP[0] - Row_61right;
	AxialD_105 = AxialD_105 * Calik + pic4To11_moveDistance;
	//resultVectorList.push_back(AxialD_105);
	//pushback_vectors(resultVectorList, 4, 16, 791.8, -0.2, +0.2);


	double AxialD_153_1 = ((cam0_11_220_152LeftChamferEnd_153LeftChamfer_164LeftChamferBegin_117Left_TLineFP[2] - cam0_11_220_152LeftChamferEnd_153LeftChamfer_164LeftChamferBegin_117Left_TLineFP[0]) + (cam0_11_220_152LeftChamferEnd_153LeftChamfer_164LeftChamferBegin_117Left_TLineFP[3] - cam0_11_220_152LeftChamferEnd_153LeftChamfer_164LeftChamferBegin_117Left_TLineFP[1])) / 2;
	AxialD_153_1 = AxialD_153_1 * Calik;
	resultVectorList.push_back(AxialD_153_1);

	double AxialD_153_2 = ((cam0_11_220_152RightChamferBegin_153RightChamfer_117Right_TLineFP[0] - cam0_11_220_152RightChamferBegin_153RightChamfer_117Right_TLineFP[2]) + (cam0_11_220_152RightChamferBegin_153RightChamfer_117Right_TLineFP[3] - cam0_11_220_152RightChamferBegin_153RightChamfer_117Right_TLineFP[1])) / 2;
	AxialD_153_2 = AxialD_153_2 * Calik;
	resultVectorList.push_back(AxialD_153_2);
	pushback_vectors(resultVectorList, 4, 24, 2, -0.3, +0.3);

	//角度
	double AxialD_153_1_angle = atan((cam0_11_220_152LeftChamferEnd_153LeftChamfer_164LeftChamferBegin_117Left_TLineFP[2] - cam0_11_220_152LeftChamferEnd_153LeftChamfer_164LeftChamferBegin_117Left_TLineFP[0]) / (cam0_11_220_152LeftChamferEnd_153LeftChamfer_164LeftChamferBegin_117Left_TLineFP[3] - cam0_11_220_152LeftChamferEnd_153LeftChamfer_164LeftChamferBegin_117Left_TLineFP[1])) * 180 / PI;
	resultVectorList.push_back(AxialD_153_1_angle);

	double AxialD_153_2_angle = atan((cam0_11_220_152RightChamferBegin_153RightChamfer_117Right_TLineFP[0] - cam0_11_220_152RightChamferBegin_153RightChamfer_117Right_TLineFP[2]) / (cam0_11_220_152RightChamferBegin_153RightChamfer_117Right_TLineFP[3] - cam0_11_220_152RightChamferBegin_153RightChamfer_117Right_TLineFP[1])) * 180 / PI;
	resultVectorList.push_back(AxialD_153_2_angle);
	pushback_vectors(resultVectorList, 3, 24.1, 45, -45, +45);
	//

	double AxialD_17_new = (cam0_11_220_104Right_TLineFP[2] + cam0_11_220_104Right_TLineFP[0]) / 2 - Row_61right;
	AxialD_17_new = AxialD_17_new * Calik + pic4To5_moveDistance + pic5To6_moveDistance + pic6To7_moveDistance + pic7To8_moveDistance + pic8To9_moveDistance + pic9To10_moveDistance + pic10To11_moveDistance;
	//resultVectorList.push_back(AxialD_17_new);
	//pushback_vectors(resultVectorList, 4, 17, 804.5, -0.2, +0.2);

};



//孔和花键的测量函数*************************************************************************************************************************************************************



/*
//不要求测量孔径
double program_5::cam1Picture_holeAlgorithm(string imgPath)//远心相机2测孔图像算法
{
	double diameter1;
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
		Emphasize(ho_ImageMean1, &ho_ImageEmphasize1, 3, 3, 0.4);
		//照射增强对比。图像中非常暗的部分被强烈“照亮”，非常亮的部分被“暗化”
		Illuminate(ho_ImageEmphasize1, &ho_ImageIlluminate, 10, 160, 0.75);
		EquHistoImage(ho_ImageIlluminate, &ho_ImageEquHisto);
		if (HDevWindowStack::IsOpen())
			DispObj(ho_ImageEquHisto, HDevWindowStack::GetActive());

		CreateMetrologyModel(&hv_MetrologyHandle);
		hv_Line1.Clear();
		hv_Line1[0] = 300;
		hv_Line1[1] = 1083;
		hv_Line1[2] = 300;
		hv_Line1[3] = 1240;
		hv_Line2.Clear();
		hv_Line2[0] = 1547;
		hv_Line2[1] = 1100;
		hv_Line2[2] = 1547;
		hv_Line2[3] = 1300;
		AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line1.TupleConcat(hv_Line2),
			70, 16, 10, 10, HTuple(), HTuple(), &hv_LineIndices);
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
		
		diameter1 = sqrt(pow(cam000__hole_RowBegin[0] - cam000__hole_RowEnd[1], 2) + pow(cam000__hole_ColBegin[0] - cam000__hole_ColEnd[1], 2)) * CalikKong;
		double diameter2 = sqrt(pow(cam000__hole_RowBegin[1] - cam000__hole_RowEnd[0], 2) + pow(cam000__hole_ColBegin[1] - cam000__hole_ColEnd[0], 2)) * CalikKong;
		double diameter3 = sqrt(pow(cam000__hole_RowBegin[0] - cam000__hole_RowBegin[1], 2) + pow(cam000__hole_ColBegin[0] - cam000__hole_ColBegin[1], 2)) * CalikKong;
		double diameter4 = sqrt(pow(cam000__hole_RowEnd[0] - cam000__hole_RowEnd[1], 2) + pow(cam000__hole_ColEnd[0] - cam000__hole_ColEnd[1], 2)) * CalikKong;
		cout << "diameter1:" << diameter1 << " " << "diameter2:" << diameter2 << " " << "diameter3:" << diameter3 << " " << "diameter4:" << diameter4 << endl;
		diameter1 = diameter1 > diameter2 ? diameter1 : diameter2;
		diameter1 = diameter1 > diameter3 ? diameter1 : diameter3;
		diameter1 = diameter1 > diameter4 ? diameter1 : diameter4;
		cout << "final diameter:" << diameter1 << endl;
		//pushback_vectors(diameter1, 8, 110, 8.9, 0, +0.2);
	}
	return diameter1;
};
*/



//用于测试的函数************



//用于测试的函数************


void program_5::run()
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
	defaultPath = defaultPath+"\\measureData\\WZ10-45-1080-220";
	dataSavePath = defaultPath + "\\" + stime;//新建文件夹
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
	
	for (int i = 0; i < 22; i++)//第一次测量（向上移动）
	{
		emit programProcess(QString("正在对 %1 号特征（%2）进行第1次测量！").arg(lsDiameterResult[i][3]).arg(featureMap[1]), i * 1);
		lsSensorMeasure_diameter(i, 1);
	};
	programConfirm = "";
	emit programTips("光幕即将下行检测，请手动将轴旋转90°后，点击'程序确认'按钮，！",2);
	do
	{
		msleep(500);
	} while (programConfirm != "continue");
	for (int i = 21; i >= 0; i--)//第二次测量（向下移动）
	{
		emit programProcess(QString("正在对 %1 号特征（%2）进行第2次测量！").arg(lsDiameterResult[i][3]).arg(featureMap[1]), 44 - i );
		lsSensorMeasure_diameter(i, 2);
		lsDiameterResult[i][2] = (lsDiameterResult[i][0] + lsDiameterResult[i][1]) / 2;//计算平均直径
		resultVectorList.push_back(lsDiameterResult[i][0]);
		resultVectorList.push_back(lsDiameterResult[i][1]);
		pushback_vectors(resultVectorList, 1, lsDiameterResult[i][3], lsDiameterResult[i][4], lsDiameterResult[i][6], lsDiameterResult[i][5]);		//lsDiameterResult[i][9] = (lsDiameterResult[i][7] + lsDiameterResult[i][8]) / 2;//计算平均旋转半径
		lsDiameterResult[i][9] = lsDiameterResult[i][2] / 2;
	};
	//不需要测量粗糙度，不用对参数初始化
	//cam2_radius[0][1] = lsDiameterResult[1][9];//传递旋转半径给粗糙度测量参数
	//cam2_radius[1][1] = lsDiameterResult[5][9];
	
	
	//测试代码*********
	
	//测试代码*********


	//向上进行远心和粗糙度测量（上行过程进行检测）********************************************************************************
	

	/*
	//不需要测量粗糙度
	for (int i = 0; i < 2; i++)
	{
		camPtrList[2]->setExposeTime(cam2_exposeTime);//粗糙度采集测量1-2号位置
		camPtrList[2]->m_captureMode = "continuous";
		originalImgPtr = &(camPtrList[2]->capturedImg);
		emit programProcess(QString("正在对 %1 号特征（粗糙度） 进行测量 ！").arg(roughnessResult[i][0]), 36 + i * 12);
		cam2_Measure_prepare(i);
	};
	*/


	for (int i = 0; i < 8; i++)//远心1-8号采集位置
	{
		camPtrList[0]->setExposeTime(cam0_exposeTime);
		camPtrList[0]->m_captureMode = "continuous";
		originalImgPtr = &(camPtrList[0]->capturedImg);
		emit programProcess(QString("远心相机正在对 %1 号位置进行轮廓测量！").arg(i+1), 50+i*5);
		imgSavePath[i] = cam0_Measure_prepare(i);
	};
	pic4To5_moveDistance = axis5_compensation(cam0_arriveOrgEncode_axis5[1]) - axis5_compensation(cam0_arriveOrgEncode_axis5[0]);
	pic5To6_moveDistance = axis5_compensation(cam0_arriveOrgEncode_axis5[2]) - axis5_compensation(cam0_arriveOrgEncode_axis5[1]);
	pic6To7_moveDistance = axis5_compensation(cam0_arriveOrgEncode_axis5[3]) - axis5_compensation(cam0_arriveOrgEncode_axis5[2]);
	pic7To8_moveDistance = axis5_compensation(cam0_arriveOrgEncode_axis5[4]) - axis5_compensation(cam0_arriveOrgEncode_axis5[3]);
	pic8To9_moveDistance = axis5_compensation(cam0_arriveOrgEncode_axis5[5]) - axis5_compensation(cam0_arriveOrgEncode_axis5[4]);
	pic9To10_moveDistance = axis5_compensation(cam0_arriveOrgEncode_axis5[6]) - axis5_compensation(cam0_arriveOrgEncode_axis5[5]);
	pic10To11_moveDistance = axis5_compensation(cam0_arriveOrgEncode_axis5[7]) - axis5_compensation(cam0_arriveOrgEncode_axis5[6]);
	pic4To11_moveDistance = axis5_compensation(cam0_arriveOrgEncode_axis5[7]) - axis5_compensation(cam0_arriveOrgEncode_axis5[0]);
	for (int i = 0; i < 8; i++)
	{
		try {
			switch (i) {
			case 0:
				cam0Picture4_algorithm(imgSavePath[0]);
				break;
			case 1:
				cam0Picture5_algorithm(imgSavePath[1]);
				break;
			case 2:
				cam0Picture6_algorithm(imgSavePath[2]);
				break;
			case 3:
				cam0Picture7_algorithm(imgSavePath[3]);
				break;
			case 4:
				cam0Picture8_algorithm(imgSavePath[4]);
				break;
			case 5:
				cam0Picture9_algorithm(imgSavePath[5]);
				break;
			case 6:
				cam0Picture10_algorithm(imgSavePath[6]);
				break;
			case 7:
				cam0Picture11_algorithm(imgSavePath[7]);
				break;
			default :
				break;
			}
		}
		catch (...) {
			emit updateDeviceInf(QString("远心相机图像%1处理错误").arg(i+1));
		};
	}
	
	
	


	//跳动&圆柱度&孔径的测量（再次下行）***************************************************************************************

	/*
	//本零件无该测量需求
	emit programTips("即将进行圆度检测，请安装机心夹！",3);
	msleep(200);
	emit fixtureTips(4);
	//孔径相机暂停测试部分
	programConfirm = "doNotContinue";
	do
	{
		msleep(200);
	} while (programConfirm == "doNotContinue");
	emit programProcess(QString("正在对 %1 号特征（圆度） 进行数据采集！").arg(roundnessResult[0][0]), 74);//圆度1号位置
	lsSensorMeasure_roundness(0);
	camPtrList[1]->setExposeTime(cam1_exposeTime);//孔径相机2号位置键（预留）
	camPtrList[1]->m_captureMode = "continuous";
	originalImgPtr = &(camPtrList[1]->capturedImg);
	emit programProcess("正在对 半圆键槽 进行测量！", 84);
	cam1_Measure_prepare(1);
	for (int i = 0; i < cam1PathList.size(); i++)
	{
		try {
			//double diameter = cam1Picture_keyAlgorithm(cam1PathList[i]);
			//pushback_vectors(diameter, 8, 110.0 + (double(i + 1) / 10), 8.9, 0, +0.2);
		}
		catch (...)
		{
			emit updateDeviceInf("键槽图像处理执行错误");
		};
	}
	
	emit programProcess("正在对 孔径 进行测量！", 94);//孔径相机1号位置检测
	cam1_Measure_prepare(0);
	for (int i = 0; i < cam1PathList.size(); i++)
	{
		try {
			double diameter = cam1Picture_holeAlgorithm(cam1PathList[i]);
			pushback_vectors(diameter, 8, 155.070 + (double(i + 1) / 10000), 5.4, 0, +0.15);
		}
		catch (...)
		{
			emit updateDeviceInf("孔径图像处理执行错误");
		}
	};
	*/


	//检测完成各轴回到合适的位置&跳动数据分析&最终结果存储***************************************************

	//五轴回合适位置
	moveControlPtr->setCurrentAxis(5);
	moveControlPtr->setTrapPrm(moveControlPtr->axisCore[moveControlPtr->currentAxisIndex], moveControlPtr->currentAxisNumber, moveControlPtr->trapAuto[moveControlPtr->currentAxisIndex], 0, moveControlPtr->trapHighVelAuto[moveControlPtr->currentAxisIndex]);
	moveControlPtr->startTrap();
	
	//跳动数据处理以及计算(不需要进行跳动测量)
	
	emit programProcess("各轴正在回到合适位置,请确认测量结果！", 95);//检测统计值计算以及显示
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

	pic4To5_moveDistance = axis5_compensation(cam0_arriveOrgEncode_axis5[1]) - axis5_compensation(cam0_arriveOrgEncode_axis5[0]);
	pic5To6_moveDistance = axis5_compensation(cam0_arriveOrgEncode_axis5[2]) - axis5_compensation(cam0_arriveOrgEncode_axis5[1]);
	pic6To7_moveDistance = axis5_compensation(cam0_arriveOrgEncode_axis5[3]) - axis5_compensation(cam0_arriveOrgEncode_axis5[2]);
	pic7To8_moveDistance = axis5_compensation(cam0_arriveOrgEncode_axis5[4]) - axis5_compensation(cam0_arriveOrgEncode_axis5[3]);
	pic8To9_moveDistance = axis5_compensation(cam0_arriveOrgEncode_axis5[5]) - axis5_compensation(cam0_arriveOrgEncode_axis5[4]);
	pic9To10_moveDistance = axis5_compensation(cam0_arriveOrgEncode_axis5[6]) - axis5_compensation(cam0_arriveOrgEncode_axis5[5]);
	pic10To11_moveDistance = axis5_compensation(cam0_arriveOrgEncode_axis5[7]) - axis5_compensation(cam0_arriveOrgEncode_axis5[6]);
	pic4To11_moveDistance;
	cout <<"4-5:" << pic4To5_moveDistance << endl;
	cout << "5-6:" << pic5To6_moveDistance << endl;
	cout << "6-7:" << pic6To7_moveDistance << endl;
	cout << "7-8:" << pic7To8_moveDistance << endl;
	cout << "8-9:" << pic8To9_moveDistance << endl;
	cout << "9-10:" << pic9To10_moveDistance << endl;
	cout << "10-11:" << pic10To11_moveDistance << endl;

};

/*
sort(fGlobal.begin(), fGlobal.end());
*/