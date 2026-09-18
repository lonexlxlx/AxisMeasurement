#include "AxisMeasurement.h"
#include <QVBoxLayout>//P2：布局重组用
#include <QWidget>//P2：布局重组用

#include <QHBoxLayout>
#include <QScrollArea>
#include <QHeaderView>
#include <QAbstractScrollArea>
#include <QScreen>
#include <QApplication>
#include <QStyle>

#include "graphical_axis_backend.h"

// Temporary UI-only preview requested by the user. Restore false after feedback.
namespace { constexpr bool kManualLayoutPreview = false; }

/// <summary>
/// 构造函数/析构函数
/// </summary>
//
void MyHalconExceptionHandler(const HException& except)
{
	throw except;
}
AxisMeasurement::AxisMeasurement(QWidget* parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	//this->setWindowIcon(QIcon("://AxisMeasurement/config/logo.ico")); 

	//P2-9/10/11：布局重构（分组收纳+QSplitter 自适应+数值仪表盘化），必须在任何控件操作之前执行
	restructureMainLayout();

	//系统相关
	this->setWindowIcon(QIcon(runtimePath("config/logo.ico")));
	m_logIn = new(logIn);
	m_logIn->show();//设定为登陆界面
	connect(m_logIn, SIGNAL(logToSystem()), this, SLOT(show()));
	ui.uiWidget->setCurrentIndex(0);//设置为自动页面
	updateDateTimer = new QTimer(this);
	connect(updateDateTimer, SIGNAL(timeout()), this, SLOT(showTime()));
	updateDateTimer->start(1000);
	m_sdk_assist=new(sdk_assist);
	m_sdk_assist->graphicalEntryError = [this]() -> QString {
		if (programRunFlag || goHomeThread_Ptr->isRunning())
			return QStringLiteral("自动测量或回零正在运行，请先结束当前运动。");
		return QString();
	};
	connect(this, &QObject::destroyed, m_sdk_assist, [assist = m_sdk_assist]() {
		assist->graphicalEntryError = []() { return QStringLiteral("主窗口已关闭，请重新启动软件。"); };
	});
	connect(m_sdk_assist, &sdk_assist::graphicalEditorCreated, this, [this](GraphicalProgramEditor* editor) {
		attachGraphicalAxisBackend(editor, moveControlCardPtr, [this]() {
			return allDeviceOpenFlag && !programRunFlag && !goHomeThread_Ptr->isRunning();
		});
		connect(this, &QObject::destroyed, editor, [editor]() { editor->setAxisBackend({}, {}); });
	});
	//m_sdk_assist->show();
	connect(m_sdk_assist, SIGNAL(diameterPostionRecord()), this, SLOT(diameterPostionRecordExecute()));
	connect(m_sdk_assist, SIGNAL(roughnessPostionRecord()), this, SLOT(roughnessPostionRecordExecute()));
	connect(m_sdk_assist, SIGNAL(cylindricityPostionRecord()), this, SLOT(cylindricityPostionRecordExecute()));
	connect(m_sdk_assist, SIGNAL(roundoutPostionRecord()), this, SLOT(roundoutPostionRecordExecute()));
	connect(m_sdk_assist, SIGNAL(holePostionRecord()), this, SLOT(holePostionRecordExecute()));
	connect(m_sdk_assist, SIGNAL(telecentricPostionRecord()), this, SLOT(telecentricPostionRecordExecute()));
	connect(m_sdk_assist, SIGNAL(tips(QString)), this, SLOT(showTips(QString)));
	

	//用于测试部分
	HException::InstallHHandler(&MyHalconExceptionHandler);

	//全局参数初始化

	currentProgram = 0;
	currentCamNum = 0;
	currentAxisNumber = 1;
	currentAxisIndex = 0;
	programRunFlag = false;
	DbOpenFlag = false;
	m_measurePartsNum_all = 0;//检测的所有零件总数
	m_okPartsNum_all = 0;//检测的所有零件良品数
	m_ngPartsNum_all = 0;//检测的所有零件NG数
	m_yield_all = 0;//检测的所有零件合格率
	currentOperatorName = "";
	currentPartsId = "";
	resultTablePtr = ui.measureTable;

	//P1-8 状态栏分区：右侧永久显示 设备状态灯 + 最近一条提示信息
	m_deviceStatusLabel = new QLabel(this);
	m_statusInfoLabel = new QLabel(this);
	m_statusInfoLabel->setMinimumWidth(420);
	updateDeviceStatus(false);
	m_statusInfoLabel->setText("设备未打开，请先打开设备！");
	statusBar()->addPermanentWidget(m_statusInfoLabel);
	statusBar()->addPermanentWidget(m_deviceStatusLabel);


	//将UI中充当指示灯设置
	ui.axisEnableStatus->setStyleSheet("QLabel{background-color:#16A34A;border-radius:3px;}");
	ui.backwardLimitStatus->setStyleSheet("QLabel{background-color:#16A34A;border-radius:3px;}");
	ui.driveAlarmStatus->setStyleSheet("QLabel{background-color:#16A34A;border-radius:3px;}");
	ui.forwardLimitStatus->setStyleSheet("QLabel{background-color:#16A34A;border-radius:3px;}");
	ui.ioUrgentStopStatus->setStyleSheet("QLabel{background-color:#16A34A;border-radius:3px;}");
	ui.moveErrorStatus->setStyleSheet("QLabel{background-color:#16A34A;border-radius:3px;}");
	ui.moveStatus->setStyleSheet("QLabel{background-color:#16A34A;border-radius:3px;}");

	//自动检测UI控件设置

	ui.ManualControl->setEnabled(kManualLayoutPreview);
	ui.autoMoveAdjust->setEnabled(false);
	ui.programNumber->setEnabled(false);
	ui.closeAllDevice->setEnabled(false);
	ui.allAxisGoHome->setEnabled(false);
	ui.startAutoMearsurement->setEnabled(false);
	ui.measureCancel->setEnabled(false);
	ui.programConfirm->setEnabled(false);
	ui.urgrentStopMearsure->setEnabled(false);
	ui.programProgressBar->setRange(0, 100);//进度条
	showDeviceInf("设备未打开，请先打开设备！");
	showPartNumber("未选择程序");
	showLsResult(999, 0);
	ui.programProcess->setWordWrap(true);//设置qlabel允许多行显示
	showProgramProcess("未进行测量！", 0);

	//显示当前程序装夹图像
	//showClampingPicture(currentProgram);

	//数据表格区UI控件设置
	ui.saveMeasureResult->setEnabled(false);
	ui.clearMeasureResult->setEnabled(false);

	//手动轴控制控件设置
	ui.jogControl->setEnabled(true);
	ui.trapControl->setEnabled(true);
	ui.axisControl->setEnabled(true);
	ui.cameraControl->setEnabled(true);
	/*
	ui.jogControl->setEnabled(false);
	ui.trapControl->setEnabled(false);
	ui.axisControl->setEnabled(false);
	ui.cameraControl->setEnabled(false);
	*/
	//设置手动控制中输入框的范围
	QDoubleValidator* pDoubleValidator = new QDoubleValidator(this);
	//pDoubleValidator->setRange(-360, 360)这条语句可以设置浮点数的输入范围;
	pDoubleValidator->setNotation(QDoubleValidator::StandardNotation);
	ui.trapAcceleratedSpeed->setValidator(pDoubleValidator);
	ui.trapDeclarationSpeed->setValidator(pDoubleValidator);
	ui.trapSmoothTime->setValidator(new QIntValidator(0, 50, this));
	ui.stepLength->setValidator(new QIntValidator(-1073741824, 1073741823, this));
	ui.trapSpeed->setValidator(pDoubleValidator);
	ui.jogSpeed->setValidator(pDoubleValidator);
	ui.jogAcceleratedSpeed->setValidator(pDoubleValidator);
	ui.jogDecelerationSpeed->setValidator(pDoubleValidator);
	QDoubleValidator* pDoubleValidatorLimit = new QDoubleValidator(this);
	pDoubleValidatorLimit->setRange(0, 1);
	ui.jogSmooth->setValidator(pDoubleValidatorLimit);

	//手动控制相机控件设置
	ui.startCamCapture->setEnabled(true);
	ui.stopCamCapture->setEnabled(false);
	ui.selectCamera->setEnabled(true);
	ui.saveImg->setEnabled(false);
	//相机控制中输入框的范围
	ui.gain->setValidator(new QIntValidator(0, 24, this));
	ui.exposeTime->setValidator(new QIntValidator(0, 30000, this));


	//测量数据区设置
	//设置表格控件的表头
	//ui.measureTable->setRowCount(200);    //设置行数
	ui.measureTable->setColumnCount(9); //设置列数
	ui.measureTable->setEditTriggers(QAbstractItemView::NoEditTriggers); //禁止编辑
	ui.measureTable->horizontalHeader()->setHighlightSections(false);//点击表时不对表头行光亮（获取焦点）
	QFont font = ui.measureTable->horizontalHeader()->font();
	font.setBold(true);//表头字体加粗
	ui.measureTable->horizontalHeader()->setFont(font);
	ui.measureTable->horizontalHeader()->setStyleSheet("QHeaderView::section{background:#F3F4F6;color:#374151;border:none;border-right:1px solid #E5E7EB;border-bottom:1px solid #E5E7EB;padding:5px 8px;}"); //表头背景色（与全局主题一致）
	ui.measureTable->setStyleSheet("selection-background-color:#DBEAFE;selection-color:#111827;alternate-background-color:#F9FAFB;"); //设置选中背景色（与全局主题一致）
	ui.measureTable->setFont(QFont(QStringLiteral("Consolas"), 10, QFont::Bold));//P2-11：数值等宽字体加粗（中文自动回退雅黑）
	QStringList header;
	header << "特征号" << "特征名称" << "测量结果" << "最小值" <<"最大值" <<"次数" << "公称值" << "下限值" << "上限值";
	ui.measureTable->setHorizontalHeaderLabels(header);//设置表头（横）
	ui.measureTable->setShowGrid(true); //设置显示格子线
	ui.measureTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);//允许用户拖拽调整列宽
	ui.measureTable->horizontalHeader()->setStretchLastSection(false);//不要强制拉伸最后一列，超宽时交给水平滚动条
	ui.measureTable->horizontalHeader()->setMinimumSectionSize(56);
	ui.measureTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	ui.measureTable->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	ui.measureTable->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	ui.measureTable->setColumnWidth(0, 70);
	ui.measureTable->setColumnWidth(1, 96);
	ui.measureTable->setColumnWidth(2, 88);
	ui.measureTable->setColumnWidth(3, 78);
	ui.measureTable->setColumnWidth(4, 78);
	ui.measureTable->setColumnWidth(5, 60);
	ui.measureTable->setColumnWidth(6, 78);
	ui.measureTable->setColumnWidth(7, 78);
	ui.measureTable->setColumnWidth(8, 78);
	ui.measureTable->show();


	//设备参数初始化
	//远心相机
	cameraList[0].initInf("GCD22090931", 0);//远心相机
	cameraPtrList[0] = &cameraList[0];
	m_camThread_ptrList[0] = new camThread(cameraPtrList[0], &(cameraPtrList[0]->capturedImg), cameraPtrList[0]->camNumber);
	camCaptureFlag[0] = false;
	connect(cameraPtrList[0], SIGNAL(cameraErrorInf(QString)), this, SLOT(showDeviceErrorInf(QString)));
	connect(m_camThread_ptrList[0], SIGNAL(Display(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	// 右侧相机用于测孔
	cameraList[1].initInf("FCB22070932", 1);
	cameraPtrList[1] = &cameraList[1];
	m_camThread_ptrList[1] = new camThread(cameraPtrList[1], &(cameraPtrList[1]->capturedImg), cameraPtrList[1]->camNumber);
	camCaptureFlag[1] = false;
	connect(cameraPtrList[1], SIGNAL(cameraErrorInf(QString)), this, SLOT(showDeviceErrorInf(QString)));
	connect(m_camThread_ptrList[1], SIGNAL(Display(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	//左侧相机用于测量表面粗糙度 
	cameraList[2].initInf("GCK22050066", 2);
	cameraPtrList[2] = &cameraList[2];
	m_camThread_ptrList[2] = new camThread(cameraPtrList[2], &(cameraPtrList[2]->capturedImg), cameraPtrList[2]->camNumber);
	camCaptureFlag[2] = false;
	connect(cameraPtrList[2], SIGNAL(cameraErrorInf(QString)), this, SLOT(showDeviceErrorInf(QString)));
	connect(m_camThread_ptrList[2], SIGNAL(Display(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	//运动控制卡
	moveControlCardPtr = new moveControl();
	connect(moveControlCardPtr, SIGNAL(moveControlError(QString)), this, SLOT(showDeviceErrorInf(QString)));
	for (int i = 0; i < 8; i++)
	{
		moveThreadList[i] = new moveThread(i, moveControlCardPtr);
		connect(moveThreadList[i], SIGNAL(updateAxisInf(short)), this, SLOT(updateUiAxisStatus(short)));
	};
	//光幕传感器
	lsSensorPtr = new ls_device();
	connect(lsSensorPtr, SIGNAL(LsErrorInf(QString)), this, SLOT(showDeviceErrorInf(QString)));
	m_lsThread = new lsThread(lsSensorPtr);
	connect(m_lsThread, SIGNAL(updateLsResult()), this, SLOT(showCurrentLsValue()));
	//粗糙度检测对象
	roughnessFunPtr = new roughnessFun();

	//需要参照下面格式追加子程序
	//自动检测线程0初始化WZ10-45-1080-370
	m_program0_Ptr = new program_0(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_0 = "WZ10-45-1080-370";
	connect(m_program0_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program0_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program0_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program0_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program0_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program0_Ptr, SIGNAL(programTips(QString,int)), this, SLOT(programCheck(QString,int)));
	connect(m_program0_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program0_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program0_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));
	//自动检测线程1初始化（标准轴台阶轴）
	m_program1_Ptr = new program_1(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_1 = "001";
	connect(m_program1_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program1_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program1_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program1_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program1_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program1_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program1_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program1_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	//自动检测线程2初始化WZ10-15-1107-340
	m_program2_Ptr = new program_2(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_2 = "WZ10-15-1107-340";
	connect(m_program2_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program2_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program2_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program2_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program2_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program2_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program2_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program2_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program2_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));
	//自动检测线程3初始化（WZ10-15-1107-141）
	m_program3_Ptr = new program_3(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_3 = "WZ10-15-1107-141";
	connect(m_program3_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program3_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program3_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program3_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program3_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program3_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program3_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program3_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program3_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));
	//自动检测线程4初始化（WZ10-44-1112-290）
	m_program4_Ptr = new program_4(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_4 = "WZ10-44-1112-290";
	connect(m_program4_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program4_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program4_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program4_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program4_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program4_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program4_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program4_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program4_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));
	//自动检测线程5初始化(WZ10-45-1080-220)
	m_program5_Ptr = new program_5(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_5 = "WZ10-45-1080-220";
	connect(m_program5_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program5_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program5_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program5_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program5_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program5_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program5_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program5_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float))); 
	connect(m_program5_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));
	//自动检测线程6初始化(WZ10-44-1112-155)
	m_program6_Ptr = new program_6(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_6 = "WZ10-44-1112-155";
	connect(m_program6_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program6_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program6_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program6_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program6_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program6_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program6_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program6_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program6_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));
	//自动检测线程7初始化(WZ20-15-001-80)
	m_program7_Ptr = new program_7(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_7 = "WZ20-15-001-80";
	connect(m_program7_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program7_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program7_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program7_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program7_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program7_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program7_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program7_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program7_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));
	//自动检测线程8初始化(KF71-53-009-140)
	m_program8_Ptr = new program_8(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_8 = "KF71-53-009-140";
	connect(m_program8_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program8_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program8_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program8_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program8_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program8_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program8_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program8_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program8_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));
	//自动检测线程9初始化(WZ10-11-2006-430)
	m_program9_Ptr = new program_9(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_9 = "WZ10-11-2006-430";
	connect(m_program9_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program9_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program9_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program9_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program9_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program9_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program9_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program9_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program9_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));

	m_program10_Ptr = new program_10(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_10 = "010";
	connect(m_program10_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program10_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program10_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program10_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program10_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program10_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program10_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program10_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program10_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));

	//WZ10451080-370-1
	m_program11_Ptr = new program_11(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_11 = "WZ10451080-1";
	connect(m_program11_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program11_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program11_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program11_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program11_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program11_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program11_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program11_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program11_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));
	
	//WZ10451080-370-2
	m_program12_Ptr = new program_12(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_12 = "WZ10451080-2";
	connect(m_program12_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program12_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program12_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program12_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program12_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program12_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program12_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program12_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program12_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));

	//WZ10451080-370-3
	m_program13_Ptr = new program_13(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_13 = "WZ10451080-3";//
	connect(m_program13_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program13_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program13_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program13_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program13_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program13_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program13_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program13_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program13_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));

	//XD-WZ10451080-370
	m_program14_Ptr = new program_14(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_14 = "XD-WZ10451080-370";//
	connect(m_program14_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program14_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program14_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program14_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program14_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program14_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program14_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program14_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program14_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));

	//WZ10451080-370-4
	m_program15_Ptr = new program_15(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_15 = "WZ10451080-4";//
	connect(m_program15_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program15_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program15_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program15_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program15_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program15_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program15_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program15_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program15_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));
	
	//WZ10151107-160-1
	m_program16_Ptr = new program_16(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_16 = "WZ10151107-160-1";//
	connect(m_program16_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program16_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program16_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program16_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program16_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program16_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program16_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program16_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program16_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));

	//WZ10151107-HJC
	m_program17_Ptr = new program_17(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_17 = "WZ10151107-HJC";//
	connect(m_program17_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program17_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program17_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program17_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program17_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program17_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program17_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program17_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program17_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));

	//Z82292121270-HJC
	m_program18_Ptr = new program_18(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_18 = "Z82292121270-HJC";//
	connect(m_program18_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program18_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program18_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program18_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program18_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program18_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program18_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program18_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program18_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));

	//KF7153009-230
	m_program19_Ptr = new program_19(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_18 = "KF7153009-230";//
	connect(m_program19_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program19_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program19_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program19_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program19_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program19_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program19_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program19_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program19_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));

	//专用标准轴-HJC
	m_program20_Ptr = new program_20(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_20 = "STANDARDAXIS-HJC";//
	connect(m_program20_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program20_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program20_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program20_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program20_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program20_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program20_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program20_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program20_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));

	//螺纹测试-SL
	m_program21_Ptr = new program_21(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_21 = "LUOWENSL";//
	connect(m_program21_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program21_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program21_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program21_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program21_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program21_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program21_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program21_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program21_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));

	//线程22 螺纹测试-XD
	m_program22_Ptr = new program_22(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_22 = "LUOWEN-xd";//
	connect(m_program22_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program22_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program22_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program22_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program22_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program22_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program22_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program22_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program22_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));
	//线程23 螺纹测试-LDX
	m_program23_Ptr = new program_23(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_23 = "LUOWEN-ldx";//
	connect(m_program23_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program23_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program23_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program23_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program23_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program23_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program23_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program23_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program23_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));

	//线程24 螺纹测试-HJC
	m_program24_Ptr = new program_24(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_24 = "LUOWEN-HJC";//
	connect(m_program24_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program24_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program24_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program24_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program24_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program24_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program24_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program24_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program24_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));
	//线程25 
	m_program25_Ptr = new program_25(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_25 = "WZ10.11.2006-190";//
	connect(m_program25_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program25_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program25_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program25_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program25_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program25_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program25_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program25_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program25_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));

	m_program26_Ptr = new program_26(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_26 = "name26";//
	connect(m_program26_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program26_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program26_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program26_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program26_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program26_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program26_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program26_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program26_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));

	m_program27_Ptr = new program_27(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_27 = "name27";//
	connect(m_program27_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program27_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program27_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program27_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program27_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program27_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program27_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program27_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program27_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));

	m_program28_Ptr = new program_28(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_28 = "name28";//
	connect(m_program28_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program28_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program28_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program28_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program28_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program28_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program28_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program28_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program28_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));

	m_program29_Ptr = new program_29(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_29 = "name29";//
	connect(m_program29_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program29_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program29_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program29_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program29_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program29_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program29_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program29_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program29_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));

	m_program30_Ptr = new program_30(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_30 = "name30";//
	connect(m_program30_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program30_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program30_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program30_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program30_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program30_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program30_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program30_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program30_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));

	m_program31_Ptr = new program_31(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_31 = "name31";//
	connect(m_program31_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program31_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program31_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program31_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program31_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program31_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program31_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program31_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program31_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));

	m_program32_Ptr = new program_32(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_32 = "name32";//
	connect(m_program32_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program32_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program32_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program32_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program32_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program32_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program32_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program32_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program32_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));

	m_program33_Ptr = new program_33(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_33 = "name33";//
	connect(m_program33_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program33_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program33_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program33_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program33_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program33_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program33_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program33_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program33_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));

	m_program34_Ptr = new program_34(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_34 = "name34";//
	connect(m_program34_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program34_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program34_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program34_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program34_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program34_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program34_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program34_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program34_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));

	m_program35_Ptr = new program_35(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_35 = "name35";//
	connect(m_program35_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program35_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program35_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program35_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program35_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program35_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program35_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program35_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program35_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));

	m_program36_Ptr = new program_36(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_36 = "name36";//
	connect(m_program36_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program36_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program36_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program36_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program36_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program36_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program36_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program36_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program36_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));

	m_program37_Ptr = new program_37(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_37 = "name37";//
	connect(m_program37_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program37_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program37_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program37_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program37_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program37_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program37_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program37_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program37_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));

	m_program38_Ptr = new program_38(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_38 = "name38";//
	connect(m_program38_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program38_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program38_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program38_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program38_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program38_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program38_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program38_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program38_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));

	m_program39_Ptr = new program_39(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_39 = "name39";//
	connect(m_program39_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program39_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program39_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program39_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program39_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program39_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program39_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program39_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program39_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));

	m_program40_Ptr = new program_40(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_40 = "name40";//
	connect(m_program40_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program40_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program40_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program40_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program40_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program40_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program40_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program40_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program40_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));

	m_program41_Ptr = new program_41(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_41 = "name41";//
	connect(m_program41_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program41_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program41_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program41_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program41_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program41_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program41_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program41_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program41_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));

	m_program42_Ptr = new program_42(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_42 = "name42";//
	connect(m_program42_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program42_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program42_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program42_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program42_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program42_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program42_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program42_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program42_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));

	m_program43_Ptr = new program_43(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_43 = "name43";//
	connect(m_program43_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program43_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program43_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program43_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program43_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program43_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program43_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program43_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program43_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));

	m_program44_Ptr = new program_44(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_44 = "name44";//
	connect(m_program44_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program44_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program44_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program44_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program44_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program44_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program44_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program44_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program44_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));

	m_program45_Ptr = new program_45(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_45 = "name45";//
	connect(m_program45_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program45_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program45_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program45_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program45_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program45_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program45_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program45_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program45_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));

	m_program46_Ptr = new program_46(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_46 = "name46";//
	connect(m_program46_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program46_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program46_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program46_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program46_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program46_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program46_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program46_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program46_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));

	m_program47_Ptr = new program_47(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_47 = "name47";//
	connect(m_program47_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program47_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program47_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program47_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program47_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program47_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program47_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program47_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program47_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));

	m_program48_Ptr = new program_48(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_48 = "name48";//
	connect(m_program48_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program48_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program48_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program48_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program48_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program48_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program48_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program48_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program48_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));

	m_program49_Ptr = new program_49(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_49 = "name49";//
	connect(m_program49_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program49_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program49_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program49_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program49_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program49_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program49_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program49_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program49_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));

	m_program50_Ptr = new program_50(cameraPtrList[0], cameraPtrList[1], cameraPtrList[2], moveControlCardPtr, lsSensorPtr, resultTablePtr, DbPtr, roughnessFunPtr);
	partNumber_50 = "name50";//
	connect(m_program50_Ptr, SIGNAL(imgInf(const Mat*, QString, int)), this, SLOT(displayImg(const Mat*, QString, int)));
	connect(m_program50_Ptr, SIGNAL(partsNumber(QString)), this, SLOT(showPartNumber(QString)));
	connect(m_program50_Ptr, SIGNAL(lsResult(int, float)), this, SLOT(showLsResult(int, float)));
	connect(m_program50_Ptr, SIGNAL(programProcess(QString, int)), this, SLOT(showProgramProcess(QString, int)));
	connect(m_program50_Ptr, SIGNAL(updateDeviceInf(QString)), this, SLOT(showDeviceInf(QString)));
	connect(m_program50_Ptr, SIGNAL(programTips(QString, int)), this, SLOT(programCheck(QString, int)));
	connect(m_program50_Ptr, SIGNAL(Finished(bool)), this, SLOT(programFinish(bool)));
	connect(m_program50_Ptr, SIGNAL(measureStatistics(QString, int, int, float)), this, SLOT(show_programStatistics(QString, int, int, float)));
	connect(m_program50_Ptr, SIGNAL(fixtureTips(int)), this, SLOT(showFixturePicture(int)));



	//预留检测线程初始化*****************************************************************************************************

	//回原线程初始化
	goHomeThread_Ptr = new axisGoHome_thread(moveControlCardPtr);
	connect(goHomeThread_Ptr, SIGNAL(GoHomeProgress(QString)), this, SLOT(showDeviceInf(QString)));
	connect(goHomeThread_Ptr, SIGNAL(goHomeFinished(int, bool)), this, SLOT(goHomeThreadFinish(int, bool)));
	//connect(goHomeThread_Ptr, &QThread::finished, m_program0_Ptr, &QThread::deleteLater);//线程删除测试

	//连接菜单信号槽函数
	connect(ui.autoMeasureMode, SIGNAL(triggered()), this, SLOT(on_autoMeasureMode_Triggered()));
	connect(ui.ManualControl, SIGNAL(triggered()), this, SLOT(on_ManualControl_Triggered()));
	connect(ui.sdkAssist, SIGNAL(triggered()), this, SLOT(on_sdkAssist_Triggered()));

	//用于测试的部分
	/*
	string path = "E:/backup/picture/20230624_204714/2_1";
	//replace(roughnessFolder.begin(), roughnessFolder.end(), '\\', '/');
	pair<float, string> result = roughnessFunPtr->processFolder(path);
	float max_value = result.first;
	string max_filename = result.second;
	if (!max_filename.empty()) {
		cout << "最大值: " << max_value << endl;
		cout << "对应图像的图像名称: " << max_filename << endl;
	}
	else {
		cout << "数组为空" << endl;
	}
	//on_creatDatabase_clicked();
	//openDatabase();
	//m_program0_Ptr->creatDatabaseTable();
	//m_program0_Ptr->writeToDatabase();
	//m_program0_Ptr->start();
	//closeDatabase();
	*/

};

AxisMeasurement::~AxisMeasurement()
{
	for (int i = 0; i < 3; i++)
	{
		cameraPtrList[i]->unInit();
	};
	delete m_logIn;
	//delete cameraPtrList;
	delete moveControlCardPtr;
	delete lsSensorPtr;
	delete m_program0_Ptr;
	delete m_program1_Ptr;
	delete m_program2_Ptr;
	delete m_program3_Ptr;
	delete m_program4_Ptr;
	delete m_program5_Ptr;
	delete m_program6_Ptr;
	delete m_program7_Ptr;
	delete m_program8_Ptr;
	delete m_program9_Ptr;
	delete m_program10_Ptr;
	delete goHomeThread_Ptr;
	//delete moveThreadList;
	//delete m_camThread_ptrList;// cameraPtrList、moveThreadList 和 m_camThread_ptrList 是固定成员数组，不能直接 delete。
	//delete originalImgPtr;
	delete m_lsThread;
};

/// 菜单控制槽函数

void AxisMeasurement::on_autoMeasureMode_Triggered()
{
	cout << "设置为自动模式" << endl;
	currentMeasureMode = "AutoMeasureMode";
	if (camCaptureFlag[0] || camCaptureFlag[1] || camCaptureFlag[2])
	{
		showTips("有相机仍在采集中，请停止采集后重新尝试！");
		return;
	};
	ui.uiWidget->setCurrentIndex(0);
	//关闭轴以及其更新线程
	moveThreadList[currentAxisIndex]->requestInterruption();
	moveThreadList[currentAxisIndex]->quit();
	moveThreadList[currentAxisIndex]->exit();
	ui.startAutoMearsurement->setEnabled(false);
	ui.measureCancel->setEnabled(false);
	ui.programConfirm->setEnabled(false);
	ui.allAxisGoHome->setEnabled(false);
};
void AxisMeasurement::on_ManualControl_Triggered()
{
	cout << "on_ManualControl_Triggered()" << endl;
	if (kManualLayoutPreview) {
		if (programRunFlag || goHomeThread_Ptr->isRunning()) return;
		ui.uiWidget->setCurrentIndex(1);
		ui.axisControl->setEnabled(true);
		ui.axisNumber->setEnabled(false);
		// Enable only mode selectors. Do not start monitoring or touch the card.
		for (QWidget* group : { static_cast<QWidget*>(ui.axisControl), static_cast<QWidget*>(ui.jogControl), static_cast<QWidget*>(ui.trapControl) }) {
			for (QAbstractButton* button : group->findChildren<QAbstractButton*>())
				button->setEnabled(button == ui.jogMode || button == ui.trapMode);
			for (QLineEdit* input : group->findChildren<QLineEdit*>()) input->setReadOnly(true);
		}
		ui.cameraControl->setEnabled(false);
		ui.LS9000->setEnabled(false);
		ui.jogControl->setEnabled(ui.jogMode->isChecked());
		ui.trapControl->setEnabled(ui.trapMode->isChecked());
		statusBar()->showMessage(QStringLiteral("临时界面预览：可切换Jog/点位模式并查看对应页签；硬件动作与参数修改禁用，待反馈后恢复入口限制。"));
		return;
	}
	if (allDeviceOpenFlag && !programRunFlag)
	{
		cout << "手动控制模式" << endl;
		currentMeasureMode = "ManualControlMode";
		ui.uiWidget->setCurrentIndex(1);
		//设置手动控制的轴号以及线程
		moveControlCardPtr->setCurrentAxis(currentAxisNumber);
		updateTrapSettings(moveControlCardPtr->currentAxisNumber);
		updateJogSettings(moveControlCardPtr->currentAxisNumber);
		ui.jogControl->setEnabled(false);
		ui.trapControl->setEnabled(false);
		moveThreadList[moveControlCardPtr->currentAxisIndex]->start();
	};

};
void AxisMeasurement::on_sdkAssist_Triggered()
{
	cout << "Son_sdkAssist_Triggered()" << endl;
	currentMeasureMode = "sdk_assist";
	//ui.uiWidget->setCurrentIndex(2);
	m_sdk_assist->show();
};

//点位记录槽函数
void AxisMeasurement::diameterPostionRecordExecute()
{
	cout << "AxisMeasurement::diameterPostionRecordExecute()" << endl;
	moveControlCardPtr->updateAxisStatus(5);//获取光幕轴位置
	m_sdk_assist->m_diameterPositionInf[m_sdk_assist->currentDiameterOrder].axisGuangMuEncodePostion = moveControlCardPtr->dPrfPos[4];
	m_sdk_assist->m_diameterPositionInf[m_sdk_assist->currentDiameterOrder].axisGuangMuRealPostion = axis_compsation(moveControlCardPtr->dPrfPos[4]);
};
void  AxisMeasurement::roughnessPostionRecordExecute()
{
	cout << "roughnessPostionRecordExecute()" << endl;
	moveControlCardPtr->updateAxisStatus(5);//获取光幕轴位置
	m_sdk_assist->m_roughnessPositionInf[m_sdk_assist->currentRoughnessOrder].axisGuangMuEncodePostion = moveControlCardPtr->dPrfPos[4];
	m_sdk_assist->m_roughnessPositionInf[m_sdk_assist->currentRoughnessOrder].axisGuangMuRealPostion = axis_compsation(moveControlCardPtr->dPrfPos[4]);
	moveControlCardPtr->updateAxisStatus(1);//获取粗糙度轴位置
	m_sdk_assist->m_roughnessPositionInf[m_sdk_assist->currentRoughnessOrder].axisRoughnessEncodePostion = moveControlCardPtr->dPrfPos[0];
	m_sdk_assist->m_roughnessPositionInf[m_sdk_assist->currentRoughnessOrder].axisRoughnessRealPostion = axis1And2_caculation(moveControlCardPtr->dPrfPos[0]);
	m_sdk_assist->m_roughnessPositionInf[m_sdk_assist->currentRoughnessOrder].roughnessExposeTime = cameraPtrList[2]->exposeTime;//获取粗糙度相机的当前曝光

};
void  AxisMeasurement::cylindricityPostionRecordExecute()
{
	cout << "cylindricityPostionRecordExecute()" << endl;
	moveControlCardPtr->updateAxisStatus(5);//获取光幕轴位置(中间截面)
	m_sdk_assist->m_cylindricityPositionInf[m_sdk_assist->currentCylindricityOrder].axisGuangMuEncodePostion_middle = moveControlCardPtr->dPrfPos[4];
	m_sdk_assist->m_cylindricityPositionInf[m_sdk_assist->currentCylindricityOrder].axisGuangMuRealPostion_middle = axis_compsation(moveControlCardPtr->dPrfPos[4]);
	m_sdk_assist->m_cylindricityPositionInf[m_sdk_assist->currentCylindricityOrder].axisGuangMuEncodePostion_upper = moveControlCardPtr->dPrfPos[4]+ m_sdk_assist->cylindricityUpperRelativeLocation_current;
	m_sdk_assist->m_cylindricityPositionInf[m_sdk_assist->currentCylindricityOrder].axisGuangMuRealPostion_upper = axis_compsation(m_sdk_assist->m_cylindricityPositionInf[m_sdk_assist->currentCylindricityOrder].axisGuangMuEncodePostion_upper);
	m_sdk_assist->m_cylindricityPositionInf[m_sdk_assist->currentCylindricityOrder].axisGuangMuEncodePostion_bottom = moveControlCardPtr->dPrfPos[4]- m_sdk_assist->cylindricityBottomRelativeLocation_current;
	m_sdk_assist->m_cylindricityPositionInf[m_sdk_assist->currentCylindricityOrder].axisGuangMuRealPostion_bottom = axis_compsation(m_sdk_assist->m_cylindricityPositionInf[m_sdk_assist->currentCylindricityOrder].axisGuangMuEncodePostion_bottom);
};

void  AxisMeasurement::roundoutPostionRecordExecute()
{
	cout << "roundoutPostionRecordExecute()" << endl;
	moveControlCardPtr->updateAxisStatus(5);//获取光幕轴位置(中间截面)
	m_sdk_assist->m_roundoutPositionInf[m_sdk_assist->currentRoundoutOrder].axisGuangMuEncodePostion_middle = moveControlCardPtr->dPrfPos[4];
	m_sdk_assist->m_roundoutPositionInf[m_sdk_assist->currentRoundoutOrder].axisGuangMuRealPostion_middle = axis_compsation(moveControlCardPtr->dPrfPos[4]);
	m_sdk_assist->m_roundoutPositionInf[m_sdk_assist->currentRoundoutOrder].axisGuangMuEncodePostion_upper = moveControlCardPtr->dPrfPos[4] + m_sdk_assist->roundoutUpperRelativeLocation_current;
	m_sdk_assist->m_roundoutPositionInf[m_sdk_assist->currentRoundoutOrder].axisGuangMuRealPostion_upper = axis_compsation(m_sdk_assist->m_roundoutPositionInf[m_sdk_assist->currentRoundoutOrder].axisGuangMuEncodePostion_upper);
	m_sdk_assist->m_roundoutPositionInf[m_sdk_assist->currentRoundoutOrder].axisGuangMuEncodePostion_bottom = moveControlCardPtr->dPrfPos[4] - m_sdk_assist->roundoutBottomRelativeLocation_current;
	m_sdk_assist->m_roundoutPositionInf[m_sdk_assist->currentRoundoutOrder].axisGuangMuRealPostion_bottom = axis_compsation(m_sdk_assist->m_roundoutPositionInf[m_sdk_assist->currentRoundoutOrder].axisGuangMuEncodePostion_bottom);

}
void  AxisMeasurement::holePostionRecordExecute()
{
	cout << "holePostionRecordExecute()" << endl;
	moveControlCardPtr->updateAxisStatus(5);//获取光幕轴位置
	m_sdk_assist->m_holePositionInf[m_sdk_assist->currentHoleOrder].axisGuangMuEncodePostion = moveControlCardPtr->dPrfPos[4];
	m_sdk_assist->m_holePositionInf[m_sdk_assist->currentHoleOrder].axisGuangMuRealPostion = axis_compsation(moveControlCardPtr->dPrfPos[4]);
	moveControlCardPtr->updateAxisStatus(2);//获取孔轴位置
	m_sdk_assist->m_holePositionInf[m_sdk_assist->currentHoleOrder].axisHoleEncodePostion = moveControlCardPtr->dPrfPos[1];
	m_sdk_assist->m_holePositionInf[m_sdk_assist->currentHoleOrder].axisHoleRealPostion = axis1And2_caculation(moveControlCardPtr->dPrfPos[1]);
	m_sdk_assist->m_holePositionInf[m_sdk_assist->currentHoleOrder].holeExposeTime = cameraPtrList[1]->exposeTime;//获取测孔相机的当前曝光
};
void  AxisMeasurement::telecentricPostionRecordExecute()
{
	cout << "telecentricPostionRecordExecute()" << endl;
	moveControlCardPtr->updateAxisStatus(5);//获取光幕轴位置
	m_sdk_assist->m_telecentricPositionInf[m_sdk_assist->currentTelecentricOrder].axisGuangMuEncodePostion = moveControlCardPtr->dPrfPos[4];
	m_sdk_assist->m_telecentricPositionInf[m_sdk_assist->currentTelecentricOrder].axisGuangMuRealPostion = axis_compsation(moveControlCardPtr->dPrfPos[4]);
	m_sdk_assist->m_telecentricPositionInf[m_sdk_assist->currentTelecentricOrder].telecentricExposeTime = cameraPtrList[0]->exposeTime;//获取粗糙度相机的当前曝光

};
float AxisMeasurement::axis_compsation(long int encodePos)
{
	double axis5_realReference = encodePos * 0.0005;
	double axis5_real;
	if (0 <= axis5_realReference && axis5_realReference < 320)
	{
		axis5_real = axis5_realReference - ((1.82671795108333e-06) * axis5_realReference * axis5_realReference + (-0.000658181561431101) * axis5_realReference
			+ 0.0354087938903302);
	}
	else if (320 <= axis5_realReference && axis5_realReference < 960)
	{
		axis5_real = axis5_realReference - ((-7.23209761037024e-10) * axis5_realReference * axis5_realReference * axis5_realReference + (1.49651432096357e-06) * axis5_realReference * axis5_realReference
			- 0.000987356680310842 * axis5_realReference + 0.207519788734567);
	}
	else if (960 <= axis5_realReference && axis5_realReference < 1400)
	{
		axis5_real = axis5_realReference - ((1.03211182532580e-08) * axis5_realReference * axis5_realReference * axis5_realReference + (-3.67822483559402e-05) * axis5_realReference * axis5_realReference
			+ 0.0432501678284173 * axis5_realReference + 0.0354087938903302);
	}
	else if (1400 <= axis5_realReference && axis5_realReference < 1600)
	{
		axis5_real = axis5_realReference - ((4.66843760059769e-09) * axis5_realReference * axis5_realReference * axis5_realReference + (-2.15355753242042e-05) * axis5_realReference * axis5_realReference
			+ 0.0432501678284173 * axis5_realReference - 16.9040836936982);
	}
	else
	{
		axis5_real = axis5_realReference;
	};
	axis5_real = round(axis5_real * 100000) / 100000;//元整保留五位小数
	return axis5_real;
};
float  AxisMeasurement::axis1And2_caculation(long int encodePos)
{
	return encodePos * 0.0002;
};
/// 自动检测控件槽函数

void AxisMeasurement::on_openAllDevice_clicked()
{
	showDeviceInf("正在打开设备，请勿进行其他操作！");
	

	//打开连接数据库
   // openDatabase();
	// m_program0_Ptr->start();

	 //打开相机
	cameraPtrList[0]->openCam();
	cameraPtrList[0]->setExposeTime(11);
	if (!cameraPtrList[0]->isOpenCam)
	{
		showTips("远心相机打开失败，请检查！");
		return;
	};
	cameraPtrList[1]->openCam();
	cameraPtrList[1]->setExposeTime(400);
	if (!cameraPtrList[1]->isOpenCam)
	{
		showTips("孔径相机打开失败，请检查！");
	};
	cameraPtrList[2]->openCam();
	cameraPtrList[2]->setExposeTime(300);
	if (!cameraPtrList[1]->isOpenCam)
	{
		showTips("粗糙度相机打开失败，请检查！");
	};

	//光幕传感器
	
	lsSensorPtr->openLs();
	
	if (lsSensorPtr->lsOpenflag)
	{
		m_lsThread->start();
	}
	else 
	{
		showTips("光幕传感器打开失败，请检查！");
	};
	
	
	//运动控制卡
	moveControlCardPtr->openAxisController();
	cout <<"openAxisController----" << moveControlCardPtr->openControllerFlag << endl;
	


	

	//moveControlCardPtr->openControllerFlag = true;

	if (moveControlCardPtr->openControllerFlag)
	{
		ui.ManualControl->setEnabled(true);

		moveControlCardPtr->setArrivePrm();
		moveControlCardPtr->setStopPrm();
		moveControlCardPtr->setToLimitPrm();
		moveControlCardPtr->setTriggerPrm();
		moveControlCardPtr->setCurrentAxis(1);//1轴
		moveControlCardPtr->clearStatus();
		moveControlCardPtr->moveEnable();
		moveControlCardPtr->setMoveMode("Trap");
		moveControlCardPtr->setCurrentAxis(2);//2轴
		moveControlCardPtr->clearStatus();
		moveControlCardPtr->moveEnable();
		moveControlCardPtr->setMoveMode("Trap");
		moveControlCardPtr->setCurrentAxis(5);//5轴
		moveControlCardPtr->clearStatus();
		moveControlCardPtr->moveEnable();
		moveControlCardPtr->setMoveMode("Trap");
		moveControlCardPtr->setCurrentAxis(6);//6轴
		moveControlCardPtr->clearStatus();
		moveControlCardPtr->moveEnable();
		moveControlCardPtr->setMoveMode("Jog");
		moveControlCardPtr->setCurrentAxis(7);//7轴
		moveControlCardPtr->clearStatus();
		moveControlCardPtr->moveEnable();
		moveControlCardPtr->setMoveMode("Jog");
	}

	else
	{
		showTips("运动控制卡打开失败，请检查！");
	}
	//cout<<"设备打开情况"<< cameraPtrList[0]->isOpenCam<<"  " << cameraPtrList[1]->isOpenCam << "  " << cameraPtrList[2]->isOpenCam << "  "<< moveControlCardPtr->openControllerFlag<< lsSensorPtr->lsOpenflag
	if (cameraPtrList[0]->isOpenCam && cameraPtrList[1]->isOpenCam && cameraPtrList[2]->isOpenCam && moveControlCardPtr->openControllerFlag && lsSensorPtr->lsOpenflag )//所有设备均正常打开了

	//if (cameraPtrList[0]->isOpenCam && cameraPtrList[1]->isOpenCam && cameraPtrList[2]->isOpenCam && moveControlCardPtr->openControllerFlag && lsSensorPtr->lsOpenflag && DbOpenFlag)//所有设备均正常打开了
	{
		ui.programNumber->setEnabled(true);
		ui.ManualControl->setEnabled(true);
		ui.axisControl->setEnabled(true);
		ui.cameraControl->setEnabled(true);
		allDeviceOpenFlag = true;
		updateDeviceStatus(true); //P1-8 状态栏设备灯变绿
		
		int axisStatus=moveControlCardPtr->axisCheck();
		if (axisStatus == 0)//所有轴状态均正常
		{
			showDeviceInf("所有设备已经成功打开，请进行各轴回原！");

			ui.allAxisGoHome->setEnabled(true);
			ui.autoMoveAdjust->setEnabled(true);
		}
		else
		{
			ui.allAxisGoHome->setEnabled(false);
			ui.autoMoveAdjust->setEnabled(false);
			showTips(QString("%1状态异常! 请切换至手动模式排查错误，完成后关闭设备重新打开！").arg(axisMap[axisStatus]));
			showDeviceInf(QString("%1状态异常!").arg(axisMap[axisStatus]));
		};
		
	}
	else//设备未全部打开
	{
		cout << cameraPtrList[0]->isOpenCam << cameraPtrList[1]->isOpenCam << cameraPtrList[2]->isOpenCam << moveControlCardPtr->openControllerFlag << lsSensorPtr->lsOpenflag << DbOpenFlag << endl;
		showDeviceInf("设备未全部打开，请检查设备连接！");
		updateDeviceStatus(false); //P1-8 状态栏设备灯保持红色
		ui.programNumber->setEnabled(false);
		ui.allAxisGoHome->setEnabled(false);
		ui.autoMoveAdjust->setEnabled(false);
	};
	
	
	ui.openAllDevice->setEnabled(false);
	ui.closeAllDevice->setEnabled(true);
	ui.startAutoMearsurement->setEnabled(false);
	ui.measureCancel->setEnabled(false);
	ui.urgrentStopMearsure->setEnabled(true);
	ui.programConfirm->setEnabled(false);
	ui.creatDatabase->setEnabled(false);//暂时没用上
};
void AxisMeasurement::on_closeAllDevice_clicked()
{
	//cout << "on_closeAllDevice_clicked" << endl;

	closeDatabase();

	for (int i = 0; i < 3; i++)
	{
		cameraPtrList[i]->closeCam();
	};
	moveControlCardPtr->closeAxisController();
	lsSensorPtr->closeLs();
	m_lsThread->requestInterruption();
	m_lsThread->quit();
	m_lsThread->exit();
	cout << "相机：" << cameraPtrList[0]->isOpenCam << cameraPtrList[1]->isOpenCam << cameraPtrList[2]->isOpenCam << " 运动控制卡：" << moveControlCardPtr->openControllerFlag << "光幕" << lsSensorPtr->lsOpenflag << endl;

	if (!cameraPtrList[0]->isOpenCam && !cameraPtrList[1]->isOpenCam && !cameraPtrList[2]->isOpenCam && !moveControlCardPtr->openControllerFlag && !lsSensorPtr->lsOpenflag && !DbOpenFlag)
	{
		showDeviceInf("所有设备已经成功关闭，如需对设备操作请先打开设备！");
		ui.creatDatabase->setEnabled(true);
	}
	else
	{
		showDeviceInf("设备未全部关闭，请检查设备连接！");
	};
	updateDeviceStatus(false); //P1-8 状态栏设备灯变红
	ui.openAllDevice->setEnabled(true);
	ui.autoMoveAdjust->setEnabled(false);
	ui.programNumber->setEnabled(false);
	ui.closeAllDevice->setEnabled(false);
	ui.allAxisGoHome->setEnabled(false);
	ui.startAutoMearsurement->setEnabled(false);
	ui.programConfirm->setEnabled(false);
	ui.urgrentStopMearsure->setEnabled(false);

	ui.ManualControl->setEnabled(kManualLayoutPreview);
	ui.axisControl->setEnabled(false);
	ui.cameraControl->setEnabled(false);
	ui.autoMoveAdjust->setEnabled(false);
	ui.jogControl->setEnabled(false);
	ui.trapControl->setEnabled(false);
};
void AxisMeasurement::on_programNumber_currentIndexChanged(int nIndex)
{

	currentProgram = nIndex;
	showClampingPicture(currentProgram);
	//cout << "on_programNumber_currentIndexChanged-" << currentProgram << endl;
	//需要按照下面格式追加子程序相关内容
	switch (currentProgram)
	{
	case 0:
		showPartNumber(partNumber_0);
		break;
	case 1:
		showPartNumber(partNumber_1);
		break;
	case 2:
		showPartNumber(partNumber_2);
		break;
	case 3:
		showPartNumber(partNumber_3);
		break;
	case 4:
		showPartNumber(partNumber_4);
		break;
	case 5:
		showPartNumber(partNumber_5);
		break;
	case 6:
		showPartNumber(partNumber_6);
		break;
	case 7:
		showPartNumber(partNumber_7);
		break;
	case 8:
		showPartNumber(partNumber_8);
		break;
	case 9:
		showPartNumber(partNumber_9);
		break;
	case 10:
		showPartNumber(partNumber_10);
		break;
	case 11:
		showPartNumber(partNumber_11);
		break;
	case 12:
		showPartNumber(partNumber_12);
		break;
	case 13:
		showPartNumber(partNumber_13);
		break;
	case 14:
		showPartNumber(partNumber_14);
		break;
	case 15:
		showPartNumber(partNumber_15);
		break;
	case 16:
		showPartNumber(partNumber_16);
		break;
	case 17:
		showPartNumber(partNumber_17);
		break;
	case 18:
		showPartNumber(partNumber_18);
		break;
	case 19:
		showPartNumber(partNumber_19);
		break;
	case 20:
		showPartNumber(partNumber_20);
		break;
	case 21:
		showPartNumber(partNumber_21);
		break;
	case 22:
		showPartNumber(partNumber_22);
		break;
	case 23:
		showPartNumber(partNumber_23);
		break;
	case 24:
		showPartNumber(partNumber_24);
		break;
	case 25:
		showPartNumber(partNumber_25);
		break;
	case 26:
		showPartNumber(partNumber_25);
		break;
	case 27:
		showPartNumber(partNumber_25);
		break;
	case 28:
		showPartNumber(partNumber_25);
		break;
	case 29:
		showPartNumber(partNumber_25);
		break;
	case 30:
		showPartNumber(partNumber_25);
		break;
	case 31:
		showPartNumber(partNumber_25);
		break;
	case 32:
		showPartNumber(partNumber_25);
		break;
	case 33:
		showPartNumber(partNumber_25);
		break;
	case 34:
		showPartNumber(partNumber_25);
		break;
	case 35:
		showPartNumber(partNumber_25);
		break;
	case 36:
		showPartNumber(partNumber_25);
		break;
	case 37:
		showPartNumber(partNumber_25);
		break;
	case 38:
		showPartNumber(partNumber_25);
		break;
	case 39:
		showPartNumber(partNumber_25);
		break;
	case 40:
		showPartNumber(partNumber_25);
		break;
	case 41:
		showPartNumber(partNumber_25);
		break;
	case 42:
		showPartNumber(partNumber_25);
		break;
	case 43:
		showPartNumber(partNumber_25);
		break;
	case 44:
		showPartNumber(partNumber_25);
		break;
	case 45:
		showPartNumber(partNumber_25);
		break;
	case 46:
		showPartNumber(partNumber_25);
		break;
	case 47:
		showPartNumber(partNumber_25);
		break;
	case 48:
		showPartNumber(partNumber_25);
		break;
	case 49:
		showPartNumber(partNumber_25);
		break;
	case 50:
		showPartNumber(partNumber_25);
		break;


	default:
		break;
	};
};
void AxisMeasurement::on_startAutoMearsurement_clicked()
{
	//cout << "on_startAutoMearsurement_clicked()" << endl;
	programRunFlag = false;
	
	
	ui.startAutoMearsurement->setEnabled(false);
	ui.autoMoveAdjust->setEnabled(false);
	int axisStatus = moveControlCardPtr->axisCheck();
	if (axisStatus == 0)//所有轴状态均正常则执行自动检测程序
	{
		ui.allAxisGoHome->setEnabled(false);
		ui.ManualControl->setEnabled(false);
		ui.programNumber->setEnabled(false);
		ui.measureResultFlag->setStyleSheet(QStringLiteral(""));
		ui.measureResultFlag->setText(QStringLiteral("测量中"));
		ui.measureResultFlag->setProperty("resultState", "measuring");
		ui.measureResultFlag->style()->unpolish(ui.measureResultFlag);
		ui.measureResultFlag->style()->polish(ui.measureResultFlag);
		ui.measureResultFlag->update();
		ui.orginaImg->clear();
		ui.orginaImg->setText(QStringLiteral("数据采集中\n图像将在采集后显示"));
		ui.frame1->setProperty("imageState", "capturing");
		ui.orginaImg->setProperty("imageState", "capturing");
		ui.frame1->style()->unpolish(ui.frame1);
		ui.frame1->style()->polish(ui.frame1);
		ui.orginaImg->style()->unpolish(ui.orginaImg);
		ui.orginaImg->style()->polish(ui.orginaImg);
		ui.frame1->update();
		ui.orginaImg->update();
		programRunFlag = true;
		switch (currentProgram)
		{
	    //需要按照下面格式追加子程序相关内容
		case 0:
			m_program0_Ptr->start();
			break;
		case 1:
			m_program1_Ptr->start();
			break;
		case 2:
			m_program2_Ptr->start();
			break;
		case 3:
			m_program3_Ptr->start();
			break;
		case 4:
			m_program4_Ptr->start();
			break;
		case 5:
			m_program5_Ptr->start();
			break;
		case 6:
			m_program6_Ptr->start();
			break;
		case 7:
			m_program7_Ptr->start();
			break;
		case 8:
			m_program8_Ptr->start();
			break;
		case 9:
			m_program9_Ptr->start();
			break;
		case 10:
			m_program10_Ptr->start();
			break;
		case 11:
			m_program11_Ptr->start();
			break;
		case 12:
			m_program12_Ptr->start();
			break;
		case 13:
			m_program13_Ptr->start();
			break;
		case 14:
			m_program14_Ptr->start();
			break;
		case 15:
			m_program15_Ptr->start();
			break;
		case 16:
			m_program16_Ptr->start();
			break;
		case 17:
			m_program17_Ptr->start();
			break;
		case 18:
			m_program18_Ptr->start();
			break;
		case 19:
			m_program19_Ptr->start();
			break;
		case 20:
			m_program20_Ptr->start();
			break;
		case 21:
			m_program21_Ptr->start();
			break;
		case 22:
			m_program22_Ptr->start();
			break;
		case 23:
			m_program23_Ptr->start();
			break;
		case 24:
			m_program24_Ptr->start();
			break;
		case 25:
			m_program25_Ptr->start();
			break;
		case 26:
			m_program26_Ptr->start();
			break;
		case 27:
			m_program27_Ptr->start();
			break;
		case 28:
			m_program28_Ptr->start();
			break;
		case 29:
			m_program29_Ptr->start();
			break;
		case 30:
			m_program30_Ptr->start();
			break;
		case 31:
			m_program31_Ptr->start();
			break;
		case 32:
			m_program32_Ptr->start();
			break;
		case 33:
			m_program33_Ptr->start();
			break;
		case 34:
			m_program34_Ptr->start();
			break;
		case 35:
			m_program35_Ptr->start();
			break;
		case 36:
			m_program36_Ptr->start();
			break;
		case 37:
			m_program37_Ptr->start();
			break;
		case 38:
			m_program38_Ptr->start();
			break;
		case 39:
			m_program39_Ptr->start();
			break;
		case 40:
			m_program40_Ptr->start();
			break;
		case 41:
			m_program41_Ptr->start();
			break;
		case 42:
			m_program42_Ptr->start();
			break;
		case 43:
			m_program43_Ptr->start();
			break;
		case 44:
			m_program44_Ptr->start();
			break;
		case 45:
			m_program45_Ptr->start();
			break;
		case 46:
			m_program46_Ptr->start();
			break;
		case 47:
			m_program47_Ptr->start();
			break;
		case 48:
			m_program48_Ptr->start();
			break;
		case 49:
			m_program49_Ptr->start();
			break;
		case 50:
			m_program50_Ptr->start();
			break;
		
		default:
			break;
		};
		
	}
	else
	{
		showTips(QString("%1状态异常, 请切换至手动模式排查错误！").arg(axisMap[axisStatus]));
		showDeviceInf(QString("%1状态异常!").arg(axisMap[axisStatus]));
	};
};
void AxisMeasurement::on_urgrentStopMearsure_clicked()
{
	//cout << "on_urgrentStopMearsure_clicked" << endl;
	flashEmergencyBorder();//P2-12：急停触发，全窗口红色边框闪烁警示
	switch (currentProgram)
	{
    //需要按照下面格式追加子程序相关内容
	case 0:
		//需要测试的线程方式
	   //线程停止方式1
		/*
		m_program0_Ptr->quit();
		m_program0_Ptr->exit();
		*/

		//线程停止方式2
		m_program0_Ptr->terminate();
		m_program0_Ptr->wait();
		break;
	case 1:
		m_program1_Ptr->terminate();
		m_program1_Ptr->wait();
		break;
	case 2:
		m_program2_Ptr->terminate();
		m_program2_Ptr->wait();
		break;
	case 3:
		m_program3_Ptr->terminate();
		m_program3_Ptr->wait();
		break;
	case 4:
		m_program4_Ptr->terminate();
		m_program4_Ptr->wait();
		break;
	case 5:
		m_program5_Ptr->terminate();
		m_program5_Ptr->wait();
		break;
	case 6:
		m_program6_Ptr->terminate();
		m_program6_Ptr->wait();
		break;
	case 7:
		m_program7_Ptr->terminate();
		m_program7_Ptr->wait();
		break;
	case 8:
		m_program8_Ptr->terminate();
		m_program8_Ptr->wait();
		break;
	case 9:
		m_program9_Ptr->terminate();
		m_program9_Ptr->wait();
		break;
	case 10:
		m_program10_Ptr->terminate();
		m_program10_Ptr->wait();
		break;
	case 11:
		m_program11_Ptr->terminate();
		m_program11_Ptr->wait();
		break;
	case 12:
		m_program12_Ptr->terminate();
		m_program12_Ptr->wait();
		break;
	case 13:
		m_program13_Ptr->terminate();
		m_program13_Ptr->wait();
		break;
	case 14:
		m_program14_Ptr->terminate();
		m_program14_Ptr->wait();
		break;
	case 15:
		m_program15_Ptr->terminate();
		m_program15_Ptr->wait();
		break;
	case 16:
		m_program16_Ptr->terminate();
		m_program16_Ptr->wait();
		break;
	case 17:
		m_program17_Ptr->terminate();
		m_program17_Ptr->wait();
		break;
	case 18:
		m_program18_Ptr->terminate();
		m_program18_Ptr->wait();
		break;
	case 19:
		m_program19_Ptr->terminate();
		m_program19_Ptr->wait();
		break;
	case 20:
		m_program20_Ptr->terminate();
		m_program20_Ptr->wait();
		break;
	case 21:
		m_program21_Ptr->terminate();
		m_program21_Ptr->wait();
		break;
	case 22:
		m_program22_Ptr->terminate();
		m_program22_Ptr->wait();
		break;
	case 23:
		m_program23_Ptr->terminate();
		m_program23_Ptr->wait();
		break;
	case 24:
		m_program24_Ptr->terminate();
		m_program24_Ptr->wait();
		break;
	case 25:
		m_program25_Ptr->terminate();
		m_program25_Ptr->wait();
		break;
	case 26:
		m_program26_Ptr->terminate();
		m_program26_Ptr->wait();
		break;
	case 27:
		m_program27_Ptr->terminate();
		m_program27_Ptr->wait();
		break;
	case 28:
		m_program28_Ptr->terminate();
		m_program28_Ptr->wait();
		break;
	case 29:
		m_program29_Ptr->terminate();
		m_program29_Ptr->wait();
		break;
	case 30:
		m_program30_Ptr->terminate();
		m_program30_Ptr->wait();
		break;
	case 31:
		m_program31_Ptr->terminate();
		m_program31_Ptr->wait();
		break;
	case 32:
		m_program32_Ptr->terminate();
		m_program32_Ptr->wait();
		break;
	case 33:
		m_program33_Ptr->terminate();
		m_program33_Ptr->wait();
		break;
	case 34:
		m_program34_Ptr->terminate();
		m_program34_Ptr->wait();
		break;
	case 35:
		m_program35_Ptr->terminate();
		m_program35_Ptr->wait();
		break;
	case 36:
		m_program36_Ptr->terminate();
		m_program36_Ptr->wait();
		break;
	case 37:
		m_program37_Ptr->terminate();
		m_program37_Ptr->wait();
		break;
	case 38:
		m_program38_Ptr->terminate();
		m_program38_Ptr->wait();
		break;
	case 39:
		m_program39_Ptr->terminate();
		m_program39_Ptr->wait();
		break;
	case 40:
		m_program40_Ptr->terminate();
		m_program40_Ptr->wait();
		break;
	case 41:
		m_program41_Ptr->terminate();
		m_program41_Ptr->wait();
		break;
	case 42:
		m_program42_Ptr->terminate();
		m_program42_Ptr->wait();
		break;
	case 43:
		m_program43_Ptr->terminate();
		m_program43_Ptr->wait();
		break;
	case 44:
		m_program44_Ptr->terminate();
		m_program44_Ptr->wait();
		break;
	case 45:
		m_program45_Ptr->terminate();
		m_program45_Ptr->wait();
		break;
	case 46:
		m_program46_Ptr->terminate();
		m_program46_Ptr->wait();
		break;
	case 47:
		m_program47_Ptr->terminate();
		m_program47_Ptr->wait();
		break;
	case 48:
		m_program48_Ptr->terminate();
		m_program48_Ptr->wait();
		break;
	case 49:
		m_program49_Ptr->terminate();
		m_program49_Ptr->wait();
		break;
	case 50:
		m_program50_Ptr->terminate();
		m_program50_Ptr->wait();
		break;
	

	default:
		break;
	}
	moveControlCardPtr->stopMove("urgent", "all");
	programRunFlag = false;
	ui.autoMoveAdjust->setEnabled(true);
	showDeviceInf("设备已经停止运行");
	showPartNumber("未开始测量");
	showLsResult(999, 0);
	ui.programProcess->setWordWrap(true);//设置qlabel允许多行显示
	showProgramProcess("未开始测量！", 0);
	ui.programConfirm->setEnabled(false);
};
void AxisMeasurement::on_allAxisGoHome_clicked()//一键回原点槽函数
{
	ui.startAutoMearsurement->setEnabled(false);
	ui.autoMoveAdjust->setEnabled(false);
	ui.allAxisGoHome->setEnabled(false);
	int axisStatus = moveControlCardPtr->axisCheck();
	if (axisStatus == 0)
	{
		ui.ManualControl->setEnabled(false);
		goHomeThread_Ptr->setGoHomeAxis(9);
		goHomeThread_Ptr->start();
	}
	else
	{
		showTips(QString("%1状态异常!").arg(axisMap[axisStatus]));
		showDeviceInf(QString("%1状态异常!").arg(axisMap[axisStatus]));
	};
};
void AxisMeasurement::on_programConfirm_clicked()//图像质量确认槽函数
{
	switch (currentProgram)
	{
	//需要按照下面格式追加子程序
	case 0:
		m_program0_Ptr->programConfirm = "continue";
		break;
	case 1:
		m_program1_Ptr->programConfirm = "continue";
		break;
	case 2:
		m_program2_Ptr->programConfirm = "continue";
		break;
	case 3:
		m_program3_Ptr->programConfirm = "continue";
		break;
	case 4:
		m_program4_Ptr->programConfirm = "continue";
		break;
	case 5:
		m_program5_Ptr->programConfirm = "continue";
		break;
	case 6:
		m_program6_Ptr->programConfirm = "continue";
		break;
	case 7:
		m_program7_Ptr->programConfirm = "continue";
		break;
	case 8:
		m_program8_Ptr->programConfirm = "continue";
		break;
	case 9:
		m_program9_Ptr->programConfirm = "continue";
		break;
	case 10:
		m_program10_Ptr->programConfirm = "continue";
		break;
	case 11:
		m_program11_Ptr->programConfirm = "continue";
		break;
	case 12:
		m_program12_Ptr->programConfirm = "continue";
		break;
	case 13:
		m_program13_Ptr->programConfirm = "continue";
		break;
	case 14:
		m_program14_Ptr->programConfirm = "continue";
		break;
	case 15:
		m_program15_Ptr->programConfirm = "continue";
		break;
	case 16:
		m_program16_Ptr->programConfirm = "continue";
		break;
	case 17:
		m_program17_Ptr->programConfirm = "continue";
		break;
	case 18:
		m_program18_Ptr->programConfirm = "continue";
		break;
	case 19:
		m_program19_Ptr->programConfirm = "continue";
		break;
	case 20:
		m_program20_Ptr->programConfirm = "continue";
		break;
	case 21:
		m_program21_Ptr->programConfirm = "continue";
		break;
	case 22:
		m_program22_Ptr->programConfirm = "continue";
		break;
	case 23:
		m_program23_Ptr->programConfirm = "continue";
		break;
	case 24:
		m_program24_Ptr->programConfirm = "continue";
		break;
	case 25:
		m_program25_Ptr->programConfirm = "continue";
		break;
	case 26:
		m_program26_Ptr->programConfirm = "continue";
		break;
	case 27:
		m_program27_Ptr->programConfirm = "continue";
		break;
	case 28:
		m_program28_Ptr->programConfirm = "continue";
		break;
	case 29:
		m_program29_Ptr->programConfirm = "continue";
		break;
	case 30:
		m_program30_Ptr->programConfirm = "continue";
		break;
	case 31:
		m_program31_Ptr->programConfirm = "continue";
		break;
	case 32:
		m_program32_Ptr->programConfirm = "continue";
		break;
	case 33:
		m_program33_Ptr->programConfirm = "continue";
		break;
	case 34:
		m_program34_Ptr->programConfirm = "continue";
		break;
	case 35:
		m_program35_Ptr->programConfirm = "continue";
		break;
	case 36:
		m_program36_Ptr->programConfirm = "continue";
		break;
	case 37:
		m_program37_Ptr->programConfirm = "continue";
		break;
	case 38:
		m_program38_Ptr->programConfirm = "continue";
		break;
	case 39:
		m_program39_Ptr->programConfirm = "continue";
		break;
	case 40:
		m_program40_Ptr->programConfirm = "continue";
		break;
	case 41:
		m_program41_Ptr->programConfirm = "continue";
		break;
	case 42:
		m_program42_Ptr->programConfirm = "continue";
		break;
	case 43:
		m_program43_Ptr->programConfirm = "continue";
		break;
	case 44:
		m_program44_Ptr->programConfirm = "continue";
		break;
	case 45:
		m_program45_Ptr->programConfirm = "continue";
		break;
	case 46:
		m_program46_Ptr->programConfirm = "continue";
		break;
	case 47:
		m_program47_Ptr->programConfirm = "continue";
		break;
	case 48:
		m_program48_Ptr->programConfirm = "continue";
		break;
	case 49:
		m_program49_Ptr->programConfirm = "continue";
		break;
	case 50:
		m_program50_Ptr->programConfirm = "continue";
		break;

	default:
		break;
	};
	ui.partRotate_anticlockwise->setEnabled(false);
	ui.partRotate_clockwise->setEnabled(false);
	ui.apexMoveUp->setEnabled(false);
	ui.apexMoveDown->setEnabled(false);
	ui.lsMoveUp->setEnabled(false);
	ui.lsMoveDown->setEnabled(false);
	ui.programConfirm->setEnabled(false);
	ui.measureCancel->setEnabled(false);
	
};
void AxisMeasurement::on_measureCancel_clicked()//检测取消槽函数
{
	switch (currentProgram)
	{
		//需要按照下面格式追加子程序
	case 0:
		//需要测试的线程方式
	   //线程停止方式1
		/*
		m_program0_Ptr->quit();
		m_program0_Ptr->exit();
		*/

		//线程停止方式2
		m_program0_Ptr->terminate();
		m_program0_Ptr->wait();
		break;
	case 1:
		m_program1_Ptr->terminate();
		m_program1_Ptr->wait();
		break;
	case 2:
		m_program2_Ptr->terminate();
		m_program2_Ptr->wait();
		break;
	case 3:
		m_program3_Ptr->terminate();
		m_program3_Ptr->wait();
		break;
	case 4:
		m_program4_Ptr->terminate();
		m_program4_Ptr->wait();
		break;
	case 5:
		m_program5_Ptr->terminate();
		m_program5_Ptr->wait();
		break;
	case 6:
		m_program6_Ptr->terminate();
		m_program6_Ptr->wait();
		break;
	case 7:
		m_program7_Ptr->terminate();
		m_program7_Ptr->wait();
		break;
	case 8:
		m_program8_Ptr->terminate();
		m_program8_Ptr->wait();
		break;
	case 9:
		m_program9_Ptr->terminate();
		m_program9_Ptr->wait();
		break;
	case 10:
		m_program10_Ptr->terminate();
		m_program10_Ptr->wait();
		break;
	case 11:
		m_program11_Ptr->terminate();
		m_program11_Ptr->wait();
		break;
	case 12:
		m_program12_Ptr->terminate();
		m_program12_Ptr->wait();
		break;
	case 13:
		m_program13_Ptr->terminate();
		m_program13_Ptr->wait();
		break;
	case 14:
		m_program14_Ptr->terminate();
		m_program14_Ptr->wait();
		break;
	case 15:
		m_program15_Ptr->terminate();
		m_program15_Ptr->wait();
		break;
	case 16:
		m_program16_Ptr->terminate();
		m_program16_Ptr->wait();
		break;
	case 17:
		m_program17_Ptr->terminate();
		m_program17_Ptr->wait();
		break;
	case 18:
		m_program18_Ptr->terminate();
		m_program18_Ptr->wait();
		break;
	case 19:
		m_program19_Ptr->terminate();
		m_program19_Ptr->wait();
		break;
	case 20:
		m_program20_Ptr->terminate();
		m_program20_Ptr->wait();
		break;
	case 21:
		m_program21_Ptr->terminate();
		m_program21_Ptr->wait();
		break;
	case 22:
		m_program22_Ptr->terminate();
		m_program22_Ptr->wait();
		break;
	case 23:
		m_program23_Ptr->terminate();
		m_program23_Ptr->wait();
		break;
	case 24:
		m_program24_Ptr->terminate();
		m_program24_Ptr->wait();
		break;
	case 25:
		m_program25_Ptr->terminate();
		m_program25_Ptr->wait();
		break;
	case 26:
		m_program26_Ptr->terminate();
		m_program26_Ptr->wait();
		break;
	default:
		break;
	}
	ui.programConfirm->setEnabled(false);
	ui.measureCancel->setEnabled(false);
	ui.ManualControl->setEnabled(true);
	ui.startAutoMearsurement->setEnabled(true);
	ui.programNumber->setEnabled(true);
	int axisStatus = moveControlCardPtr->axisCheck();
	if (axisStatus == 0)//所有轴状态均正常
	{
		showDeviceInf("自动测量已取消！");

		ui.allAxisGoHome->setEnabled(true);
		ui.autoMoveAdjust->setEnabled(true);
	}
	else
	{
		ui.allAxisGoHome->setEnabled(false);
		ui.autoMoveAdjust->setEnabled(false);
		showTips(QString("%1状态异常! 请切换至手动模式排查错误，完成后关闭设备重新打开！").arg(axisMap[axisStatus]));
		showDeviceInf(QString("%1状态异常!").arg(axisMap[axisStatus]));
	};
};
void AxisMeasurement::on_apexMoveDown_pressed()
{
	// cout << "on_apexMoveDown_pressed" << endl;
	moveControlCardPtr->setCurrentAxis(6);
	moveControlCardPtr->jogVel[5] = 70;
	moveControlCardPtr->setMoveMode("Jog");
	moveControlCardPtr->startJogMove("backward");
};
void AxisMeasurement::on_apexMoveUp_pressed()
{
	//cout << "on_apexMoveUp_pressed()" << endl;
	moveControlCardPtr->setCurrentAxis(6);
	moveControlCardPtr->jogVel[5] = 70;
	moveControlCardPtr->setMoveMode("Jog");
	moveControlCardPtr->startJogMove("forward");
};
void AxisMeasurement::on_partRotate_anticlockwise_pressed()
{
	//cout << "on_partRotate_anticlockwise_pressed()" << endl;
	moveControlCardPtr->setCurrentAxis(7);
	moveControlCardPtr->jogVel[6] = 35;
	moveControlCardPtr->setMoveMode("Jog");
	moveControlCardPtr->startJogMove("forward");
};
void AxisMeasurement::on_partRotate_clockwise_pressed()
{
	//cout << "on_partRotate_clockwise_pressed()" << endl;
	moveControlCardPtr->setCurrentAxis(7);
	moveControlCardPtr->jogVel[6] = 2;
	moveControlCardPtr->setMoveMode("Jog");
	moveControlCardPtr->startJogMove("forward");
};
void AxisMeasurement::on_lsMoveUp_pressed()
{
	moveControlCardPtr->setCurrentAxis(5);
	moveControlCardPtr->jogVel[4] = 70;
	moveControlCardPtr->setMoveMode("Jog");
	moveControlCardPtr->startJogMove("forward");
};
void AxisMeasurement::on_lsMoveDown_pressed()
{
	moveControlCardPtr->setCurrentAxis(5);
	moveControlCardPtr->jogVel[4] = 70;
	moveControlCardPtr->setMoveMode("Jog");
	moveControlCardPtr->startJogMove("backward");
};
void AxisMeasurement::on_apexMoveDown_released()
{
	//cout << "on_apexMoveDown_released" << endl;
	moveControlCardPtr->stopMove("soomth", "currentAxis");
};
void AxisMeasurement::on_apexMoveUp_released()
{
	//cout << "on_apexMoveUp_released()" << endl;
	moveControlCardPtr->stopMove("soomth", "currentAxis");
};
void AxisMeasurement::on_partRotate_anticlockwise_released()
{
	//cout << "on_partRotate_anticlockwise_released()" << endl;
	moveControlCardPtr->stopMove("soomth", "currentAxis");
};
void AxisMeasurement::on_partRotate_clockwise_released()
{
	//cout << "on_partRotate_clockwise_released()" << endl;
	moveControlCardPtr->stopMove("soomth", "currentAxis");
};
void AxisMeasurement::on_lsMoveUp_released()
{
	moveControlCardPtr->stopMove("soomth", "currentAxis");
};
void AxisMeasurement::on_lsMoveDown_released()
{
	moveControlCardPtr->stopMove("soomth", "currentAxis");
};
void AxisMeasurement::showCurrentLsValue()
{
	//cout << "showCurrentLsValue" << endl;
	double diameterConference = m_lsThread->currentResult[0];
	double diameterReal;
	diameterReal = diameter_compensation(diameterConference);
	QString diameterReal_s = QString::number(diameterReal, 'd', 4);
	QString diameterConferencel_s = QString::number(diameterConference, 'd', 4);
	ui.lsCurrentValue_2->setText(diameterConferencel_s);
	ui.lsCurrentValue->setText(diameterReal_s);
	if (ui.lsCurrentValue->property("displayState").toByteArray() != "active")
	{
		ui.lsCurrentValue->setProperty("displayState", "active");
		ui.lsCurrentValue->style()->unpolish(ui.lsCurrentValue);
		ui.lsCurrentValue->style()->polish(ui.lsCurrentValue);
		ui.lsCurrentValue->update();
	}
	ui.lsMeasureOut1->setText(diameterReal_s);
	//ui.lsCurrentValue_2->setNum(diameterConference);
	//ui.lsCurrentValue->setNum(diameterReal);
	//ui.lsMeasureOut1->setNum(diameterReal);
	ui.lsMeasureOut2->setNum(m_lsThread->currentResult[1]);
	ui.lsMeasureOut3->setNum(m_lsThread->currentResult[2]);
	ui.lsMeasureOut4->setNum(m_lsThread->currentResult[3]);
};
void AxisMeasurement::paintEvent(QPaintEvent* event) {
	Q_UNUSED(event);
	/*
	QPainter painter(this);
	QPen pen;
	//painter.begin(this);
	QBrush brush(palette().window());
	QRect geometry = ui.orginaImg->geometry();
	QPoint center = geometry.center();
	int width = geometry.width();
	int height = geometry.height();
	cout << width << "\t" << height << "\t" << "\t" << geometry.x() << "\t" << geometry.y() << "\t" << geometry.center().x() << "\t" << geometry.center().y() << endl;
	cout << width << "\t" << height << "\t" << "\t" << ui.orginaImg->x() << "\t" << ui.orginaImg->y() << "\t" << geometry.center().x() << "\t" << geometry.center().y() << endl;
	if (paintMode==0)
	{
		painter.setBrush(brush);
		//painter.drawRect(0, 0, 500, 500);
	}
	else if (paintMode == 1)
	{

		painter.setRenderHint(QPainter::Antialiasing, true);//设置反锯齿
		pen.setWidth(2);
		pen.setColor(QColor(255, 255, 0));
		pen.setStyle(Qt::DashDotLine);
		painter.setPen(pen);//设置笔的大小
		painter.drawLine(geometry.x(), geometry.y() - height/2, geometry.center().x()+ width, geometry.center().y()+ height);//起点和终点
		painter.end();
	};
	*/
};
//测量数据展示操作区域槽函数

void AxisMeasurement::on_saveMeasureResult_clicked()
{
	switch (currentProgram)
	{
		//需要按照下面格式追加子程序
	case 0:
		m_program0_Ptr->programConfirm = "save";
		break;
	case 1:
		m_program1_Ptr->programConfirm = "save";
		break;
	case 2:
		m_program2_Ptr->programConfirm = "save";
		break;
	case 3:
		m_program3_Ptr->programConfirm = "save";
		break;
	case 4:
		m_program4_Ptr->programConfirm = "save";
		break;
	case 5:
		m_program5_Ptr->programConfirm = "save";
		break;
	case 6:
		m_program6_Ptr->programConfirm = "save";
		break;
	case 7:
		m_program7_Ptr->programConfirm = "save";
		break;
	case 8:
		m_program8_Ptr->programConfirm = "save";
		break;
	case 9:
		m_program9_Ptr->programConfirm = "save";
		break;
	case 10:
		m_program10_Ptr->programConfirm = "save";
		break;
	case 11:
		m_program11_Ptr->programConfirm = "save";
		break;
	case 12:
		m_program12_Ptr->programConfirm = "save";
		break;
	case 13:
		m_program13_Ptr->programConfirm = "save";
		break;
	case 14:
		m_program14_Ptr->programConfirm = "save";
		break;
	case 15:
		m_program15_Ptr->programConfirm = "save";
		break;
	case 16:
		m_program16_Ptr->programConfirm = "save";
		break;
	case 17:
		m_program17_Ptr->programConfirm = "save";
		break;
	case 18:
		m_program18_Ptr->programConfirm = "save";
		break;
	case 19:
		m_program19_Ptr->programConfirm = "save";
		break;
	case 20:
		m_program20_Ptr->programConfirm = "save";
		break;
	case 21:
		m_program21_Ptr->programConfirm = "save";
		break;
	case 22:
		m_program22_Ptr->programConfirm = "save";
		break;
	case 23:
		m_program23_Ptr->programConfirm = "save";
		break;
	case 24:
		m_program24_Ptr->programConfirm = "save";
		break;
	case 25:
		m_program25_Ptr->programConfirm = "save";
		break;
	case 26:
		m_program26_Ptr->programConfirm = "save";
		break;
	default:
		break;
	};
	ui.saveMeasureResult->setEnabled(false);
	ui.clearMeasureResult->setEnabled(false);
};
void AxisMeasurement::on_clearMeasureResult_clicked()
{
	switch (currentProgram)
	{
		//需要按照下面格式追加子程序
	case 0:
		m_program0_Ptr->programConfirm = "delete";
		break;
	case 1:
		m_program1_Ptr->programConfirm = "delete";
		break;
	case 2:
		m_program2_Ptr->programConfirm = "delete";
		break;
	case 3:
		m_program3_Ptr->programConfirm = "delete";
		break;
	case 4:
		m_program4_Ptr->programConfirm = "delete";
		break;
	case 5:
		m_program5_Ptr->programConfirm = "delete";
		break;
	case 6:
		m_program6_Ptr->programConfirm = "delete";
		break;
	case 7:
		m_program7_Ptr->programConfirm = "delete";
		break;
	case 8:
		m_program8_Ptr->programConfirm = "delete";
		break;
	case 9:
		m_program9_Ptr->programConfirm = "delete";
		break;
	case 10:
		m_program10_Ptr->programConfirm = "delete";
		break;
	case 11:
		m_program11_Ptr->programConfirm = "delete";
		break;
	case 12:
		m_program12_Ptr->programConfirm = "delete";
		break;
	case 13:
		m_program13_Ptr->programConfirm = "delete";
		break;
	case 14:
		m_program14_Ptr->programConfirm = "delete";
		break;
	case 15:
		m_program15_Ptr->programConfirm = "delete";
		break;
	case 16:
		m_program16_Ptr->programConfirm = "delete";
		break;
	case 17:
		m_program17_Ptr->programConfirm = "delete";
		break;
	case 18:
		m_program18_Ptr->programConfirm = "delete";
		break;
	case 19:
		m_program19_Ptr->programConfirm = "delete";
		break;
	case 20:
		m_program20_Ptr->programConfirm = "delete";
		break;
	case 21:
		m_program21_Ptr->programConfirm = "delete";
		break;
	case 22:
		m_program22_Ptr->programConfirm = "delete";
		break;
	case 23:
		m_program23_Ptr->programConfirm = "delete";
		break;
	case 24:
		m_program24_Ptr->programConfirm = "delete";
		break;
	case 25:
		m_program25_Ptr->programConfirm = "delete";
		break;
	default:
		break;
	};
	ui.measureResultFlag->setStyleSheet(QStringLiteral(""));
	ui.measureResultFlag->setText(QStringLiteral("--"));
	ui.measureResultFlag->setProperty("resultState", "idle");
	ui.measureResultFlag->style()->unpolish(ui.measureResultFlag);
	ui.measureResultFlag->style()->polish(ui.measureResultFlag);
	ui.measureResultFlag->update();
	ui.ngFeatureNum->setText(QStringLiteral("--"));
	ui.programMeasureNum->setText(QStringLiteral("--"));
	ui.programYield->setText(QStringLiteral("--"));
	ui.saveMeasureResult->setEnabled(false);
	ui.clearMeasureResult->setEnabled(false);
};
void AxisMeasurement::on_zeroMeasureNum_clicked()
{
	m_measurePartsNum_all = 0;//检测的所有零件总数
	m_okPartsNum_all = 0;//检测的所有零件良品数
	m_ngPartsNum_all = 0;//检测的所有零件NG数
	m_yield_all = 0;//检测的所有零件合格率
	show_Statistics();
	m_program0_Ptr->zeroMeasureNub();
	m_program1_Ptr->zeroMeasureNub();
	m_program2_Ptr->zeroMeasureNub();
	m_program3_Ptr->zeroMeasureNub();
	m_program4_Ptr->zeroMeasureNub();
	m_program5_Ptr->zeroMeasureNub();
	m_program6_Ptr->zeroMeasureNub();
	m_program7_Ptr->zeroMeasureNub();
	m_program8_Ptr->zeroMeasureNub();
	m_program9_Ptr->zeroMeasureNub();
	m_program10_Ptr->zeroMeasureNub();
};
void AxisMeasurement::on_creatDatabase_clicked()
{
	openDatabase();
	m_program0_Ptr->creatDatabaseTable();
	m_program1_Ptr->creatDatabaseTable();
	m_program2_Ptr->creatDatabaseTable();
	m_program3_Ptr->creatDatabaseTable();
	m_program4_Ptr->creatDatabaseTable();
	m_program5_Ptr->creatDatabaseTable();
	m_program6_Ptr->creatDatabaseTable();
	m_program7_Ptr->creatDatabaseTable();
	m_program8_Ptr->creatDatabaseTable();
	m_program9_Ptr->creatDatabaseTable();
	m_program10_Ptr->creatDatabaseTable();
	closeDatabase();
};
void AxisMeasurement::on_operatorName_editingFinished()
{
	currentOperatorName = ui.operatorName->text();
	m_program0_Ptr->operatorName = currentOperatorName;
	m_program1_Ptr->operatorName = currentOperatorName;
	m_program2_Ptr->operatorName = currentOperatorName;
	m_program3_Ptr->operatorName = currentOperatorName;
	m_program4_Ptr->operatorName = currentOperatorName;
	m_program5_Ptr->operatorName = currentOperatorName;
	m_program6_Ptr->operatorName = currentOperatorName;
	m_program7_Ptr->operatorName = currentOperatorName;
	m_program8_Ptr->operatorName = currentOperatorName;
	m_program9_Ptr->operatorName = currentOperatorName;
	m_program10_Ptr->operatorName = currentOperatorName;

};
void AxisMeasurement::on_partsId_editingFinished()
{
	currentPartsId = ui.partsId->text();
	cout << currentPartsId.toStdString();
	m_program0_Ptr->partsID = currentPartsId;
	m_program1_Ptr->partsID = currentPartsId;
	m_program2_Ptr->partsID = currentPartsId;
	m_program3_Ptr->partsID = currentPartsId;
	m_program4_Ptr->partsID = currentPartsId;
	m_program5_Ptr->partsID = currentPartsId;
	m_program6_Ptr->partsID = currentPartsId;
	m_program7_Ptr->partsID = currentPartsId;
	m_program8_Ptr->partsID = currentPartsId;
	m_program9_Ptr->partsID = currentPartsId;
	m_program10_Ptr->partsID = currentPartsId;
};
void AxisMeasurement::openDatabase()
{
	Db = QSqlDatabase::addDatabase("QSQLITE");
	Db.setDatabaseName("./measureData/axisDatabase.db");
	Db.setUserName("");        //数据库用户名
	Db.setPassword("");        //数据库密码
	DbPtr = &Db;
	m_program0_Ptr->databasePtr = DbPtr;
	m_program1_Ptr->databasePtr = DbPtr;
	m_program2_Ptr->databasePtr = DbPtr;
	m_program3_Ptr->databasePtr = DbPtr;
	m_program4_Ptr->databasePtr = DbPtr;
	m_program5_Ptr->databasePtr = DbPtr;
	m_program6_Ptr->databasePtr = DbPtr;
	m_program7_Ptr->databasePtr = DbPtr;
	m_program8_Ptr->databasePtr = DbPtr;
	m_program9_Ptr->databasePtr = DbPtr;
	m_program10_Ptr->databasePtr = DbPtr;
	DbOpenFlag = Db.open();
	if (!DbOpenFlag)
	{
		showDeviceInf("数据库打开失败！请关闭其他数据库软件");
		return;
	};
};
void AxisMeasurement::closeDatabase()
{
	Db.close();
	DbOpenFlag = false;
};
void AxisMeasurement::show_programStatistics(QString result, int ngFeatureNum, int measureNum, float currentYield)
{
	ui.measureResultFlag->setStyleSheet(QStringLiteral(""));
	ui.measureResultFlag->setText(result);
	const char* resultState = result == QStringLiteral("OK") ? "ok"
		: result == QStringLiteral("NG") ? "ng"
		: result.contains(QStringLiteral("测量")) ? "measuring"
		: "idle";
	ui.measureResultFlag->setProperty("resultState", resultState);
	ui.measureResultFlag->style()->unpolish(ui.measureResultFlag);
	ui.measureResultFlag->style()->polish(ui.measureResultFlag);
	ui.measureResultFlag->update();
	ui.ngFeatureNum->setNum(ngFeatureNum);
	ui.programMeasureNum->setNum(measureNum);
	ui.programYield->setText(QString("%1 %").arg(currentYield, 0, 'f', 1));
	ui.saveMeasureResult->setEnabled(true);
	ui.clearMeasureResult->setEnabled(true);
};
void AxisMeasurement::show_Statistics()
{
	ui.measurePartsNum->setNum(m_measurePartsNum_all);
	ui.okPartsNum_All->setNum(m_okPartsNum_all);
	ui.ngPartsNum_All->setNum(m_ngPartsNum_all);
	if (m_measurePartsNum_all > 0)
	{
		m_yield_all = (static_cast<double>(m_okPartsNum_all) / m_measurePartsNum_all) * 100.0;
		ui.yield_All->setText(QString("%1 %").arg(m_yield_all, 0, 'f', 1));
	}
	else
	{
		m_yield_all = 0;
		ui.yield_All->setText(QStringLiteral("--"));
	}
};

///轴控制控件槽函数

void AxisMeasurement::on_clearStatus_clicked()
{
	cout << "on_clearStatus_clicked()" << endl;
	moveControlCardPtr->clearStatus();

};
void AxisMeasurement::on_jogMode_clicked()
{
	cout << "on_jogMode_clicked()" << endl;
	if (kManualLayoutPreview) {
		ui.jogControl->setEnabled(true);
		ui.trapControl->setEnabled(false);
		return;
	}
	moveControlCardPtr->setMoveMode("Jog");
	ui.jogControl->setEnabled(true);
	ui.trapControl->setEnabled(false);

};
void AxisMeasurement::on_moveEnable_clicked()
{
	cout << "on_moveEnable_clicked" << endl;
	moveControlCardPtr->moveEnable();
};
void AxisMeasurement::on_smoothStop_clicked()
{
	cout << "on_smoothStop_clicked" << endl;
	moveControlCardPtr->stopMove("soomth", "all");

};
void AxisMeasurement::on_urgentStop_clicked()
{
	cout << "on_urgentStop_clicked" << endl;
	flashEmergencyBorder();//P2-12：急停触发，全窗口红色边框闪烁警示
	moveControlCardPtr->stopMove("urgent", "all");
};
void AxisMeasurement::on_trapMode_clicked()
{
	cout << "on_trapMode_clicked" << endl;
	if (kManualLayoutPreview) {
		ui.jogControl->setEnabled(false);
		ui.trapControl->setEnabled(true);
		return;
	}
	moveControlCardPtr->setMoveMode("Trap");
	ui.jogControl->setEnabled(false);
	ui.trapControl->setEnabled(true);
};
void AxisMeasurement::on_zeroPosition_clicked()
{
	cout << "zeroPosition" << endl;
	moveControlCardPtr->zeroPosition();
};
void AxisMeasurement::on_jogBackwardMove_pressed()
{
	cout << "on_jogBackwardMove_pressed" << endl;
	moveControlCardPtr->startJogMove("backward");
};
void AxisMeasurement::on_jogBackwardMove_released()
{
	cout << "on_jogBackwardMove_released" << endl;
	moveControlCardPtr->stopMove("soomth", "currentAxis");
};
void AxisMeasurement::on_jogForwardwardMove_pressed()
{
	cout << "on_jogForwardwardMove_pressed" << endl;
	moveControlCardPtr->startJogMove("forward");
};
void AxisMeasurement::on_jogForwardwardMove_released()
{
	cout << "jogForwardwardMove_released" << endl;
	moveControlCardPtr->stopMove("soomth", "currentAxis");
};
void AxisMeasurement::on_startTrap_clicked()
{
	cout << "startTrap" << endl;
	moveControlCardPtr->setTrapPrm(moveControlCardPtr->axisCore[moveControlCardPtr->currentAxisIndex], moveControlCardPtr->currentAxisNumber, moveControlCardPtr->trapManual[moveControlCardPtr->currentAxisIndex], moveControlCardPtr->trapStepLengthManual[moveControlCardPtr->currentAxisIndex], moveControlCardPtr->trapVelManual[moveControlCardPtr->currentAxisIndex]);
	moveControlCardPtr->startTrap();
};
void AxisMeasurement::on_goHome_clicked()
{
	cout << "goHome" << moveControlCardPtr->currentAxisNumber << endl;
	goHomeThread_Ptr->setGoHomeAxis(moveControlCardPtr->currentAxisNumber);
	goHomeThread_Ptr->start();
};
void AxisMeasurement::on_moveUnable_clicked()
{
	cout << "moveUnable" << endl;
	moveControlCardPtr->moveUnable();
};
void AxisMeasurement::on_jogSpeed_editingFinished()
{
	cout << "jogSpeed" << endl;
	moveControlCardPtr->jogVel[moveControlCardPtr->currentAxisIndex] = ui.jogSpeed->text().toDouble();
	cout << moveControlCardPtr->jogVel[moveControlCardPtr->currentAxisIndex] << endl;
};
void AxisMeasurement::on_jogAcceleratedSpeed_editingFinished()
{
	cout << "jogAcceleratedSpeed" << endl;
	moveControlCardPtr->jog[moveControlCardPtr->currentAxisIndex].acc = ui.jogAcceleratedSpeed->text().toDouble();
	cout << moveControlCardPtr->jog[moveControlCardPtr->currentAxisIndex].acc << endl;
};
void AxisMeasurement::on_jogDecelerationSpeed_editingFinished()
{
	cout << "jogDecelerationSpeed" << endl;
	moveControlCardPtr->jog[moveControlCardPtr->currentAxisIndex].dec = ui.jogDecelerationSpeed->text().toDouble();
	cout << moveControlCardPtr->jog[moveControlCardPtr->currentAxisIndex].dec << endl;
};
void AxisMeasurement::on_jogSmooth_editingFinished()
{
	cout << "jogSmooth" << endl;
	moveControlCardPtr->jog[moveControlCardPtr->currentAxisIndex].smooth = ui.jogSmooth->text().toDouble();
	cout << moveControlCardPtr->jog[moveControlCardPtr->currentAxisIndex].smooth << endl;
};

void AxisMeasurement::on_trapSmoothTime_editingFinished()
{
	cout << "trapSmoothTime" << endl;
	moveControlCardPtr->trapManual[moveControlCardPtr->currentAxisIndex].smoothTime = ui.trapSmoothTime->text().toShort();
	cout << moveControlCardPtr->trapManual[moveControlCardPtr->currentAxisIndex].smoothTime << endl;
};
void AxisMeasurement::on_stepLength_editingFinished()
{
	cout << "stepLength" << endl;
	moveControlCardPtr->trapStepLengthManual[moveControlCardPtr->currentAxisIndex] = ui.stepLength->text().toLong();
	cout << moveControlCardPtr->trapStepLengthManual[moveControlCardPtr->currentAxisIndex] << endl;
};
void AxisMeasurement::on_trapAcceleratedSpeed_editingFinished()
{
	cout << "trapAcceleratedSpeed" << endl;
	moveControlCardPtr->trapManual[moveControlCardPtr->currentAxisIndex].acc = ui.trapAcceleratedSpeed->text().toDouble();
	cout << moveControlCardPtr->trapManual[moveControlCardPtr->currentAxisIndex].acc << endl;
};
void AxisMeasurement::on_trapDeclarationSpeed_editingFinished()
{
	cout << "trapDeclarationSpeed" << endl;
	moveControlCardPtr->trapManual[moveControlCardPtr->currentAxisIndex].dec = ui.trapDeclarationSpeed->text().toDouble();
	cout << moveControlCardPtr->trapManual[moveControlCardPtr->currentAxisIndex].dec << endl;
};
void AxisMeasurement::on_trapSpeed_editingFinished()
{
	cout << "trapSpeed" << endl;
	moveControlCardPtr->trapVelManual[moveControlCardPtr->currentAxisIndex] = ui.trapSpeed->text().toDouble();
	cout << moveControlCardPtr->trapVelManual[moveControlCardPtr->currentAxisIndex] << endl;
};
void AxisMeasurement::on_axisNumber_currentIndexChanged(int nIndex)
{
	cout << "axisNumber_currentIndexChanged" << nIndex << endl;

	lastAxisIndex = currentAxisIndex;
	moveThreadList[lastAxisIndex]->requestInterruption();
	moveThreadList[lastAxisIndex]->quit();
	moveThreadList[lastAxisIndex]->exit();
	switch (nIndex)
	{
	case 0:
		currentAxisNumber = 1;
		break;
	case 1:
		currentAxisNumber = 2;
		break;
	case 2:
		currentAxisNumber = 5;
		break;
	case 3:
		currentAxisNumber = 6;
		break;
	case 4:
		currentAxisNumber = 7;
		break;
	};
	currentAxisIndex = currentAxisNumber - 1;
	cout << lastAxisIndex << currentAxisNumber << endl;
	moveControlCardPtr->setCurrentAxis(currentAxisNumber);
	cout << "currentAxisNumber  = " << currentAxisNumber << "lastAxisNumber =" << lastAxisIndex << endl;
	ui.jogControl->setEnabled(false);
	ui.trapControl->setEnabled(false);
	//////更改为选择轴号的时候使用线程更新信息//////
	updateTrapSettings(moveControlCardPtr->currentAxisNumber);
	updateJogSettings(moveControlCardPtr->currentAxisNumber);
	moveThreadList[moveControlCardPtr->currentAxisIndex]->start();
};
void AxisMeasurement::updateUiAxisStatus(short axisNumber)
{
	//cout << "updateUiAxisStatus" << endl;
	short axisIndex = axisNumber - 1;
	changeLedColor(ui.driveAlarmStatus, moveControlCardPtr->bFlagAlarm[axisIndex]);
	changeLedColor(ui.moveErrorStatus, moveControlCardPtr->bFlagMError[axisIndex]);
	changeLedColor(ui.forwardLimitStatus, moveControlCardPtr->bFlagPosLimit[axisIndex]);
	changeLedColor(ui.backwardLimitStatus, moveControlCardPtr->bFlagNegLimit[axisIndex]);
	changeLedColor(ui.ioUrgentStopStatus, moveControlCardPtr->bFlagAbruptStop[axisIndex]);
	changeLedColor(ui.axisEnableStatus, moveControlCardPtr->bFlagServoOn[axisIndex]);
	changeLedColor(ui.moveStatus, moveControlCardPtr->bFlagMotion[axisIndex]);
	changeLedColor(ui.arrivePosion, moveControlCardPtr->bFlagArrive[axisIndex]);

	ui.planPosion->setNum(moveControlCardPtr->dPrfPos[axisIndex]);
	ui.planSpeed->setNum(moveControlCardPtr->dPrfVel[axisIndex]);
	ui.planAcceleratedSpeed->setNum(moveControlCardPtr->dPrfAcc[axisIndex]);
	ui.actualPosion->setNum(moveControlCardPtr->dEncodePos[axisIndex]);
	ui.actualSpeed->setNum(moveControlCardPtr->dEncodeVel[axisIndex]);
	ui.actualAcceleratedSpeed->setNum(moveControlCardPtr->dEncodeAcc[axisIndex]);
	ui.moveMode->setText(QString::fromStdString(moveControlCardPtr->currentMoveMode[axisIndex]));
	if (axisNumber == 1 || axisNumber == 2)
	{
		ui.actualPosion_mm->setNum((moveControlCardPtr->dEncodePos[axisIndex]) * 0.0002);
	}
	else if (axisNumber == 5)
	{
		double axis5_realReference = moveControlCardPtr->dEncodePos[axisIndex] * 0.0005;
		double axis5_real;
		if (0 <= axis5_realReference && axis5_realReference < 320)
		{
			axis5_real = axis5_realReference - ((1.82671795108333e-06) * axis5_realReference * axis5_realReference + (-0.000658181561431101) * axis5_realReference
				+ 0.0354087938903302);
		}
		else if (320 <= axis5_realReference && axis5_realReference < 960)
		{
			axis5_real = axis5_realReference - ((-7.23209761037024e-10) * axis5_realReference * axis5_realReference * axis5_realReference + (1.49651432096357e-06) * axis5_realReference * axis5_realReference
				- 0.000987356680310842 * axis5_realReference + 0.207519788734567);
		}
		else if (960 <= axis5_realReference && axis5_realReference < 1400)
		{
			axis5_real = axis5_realReference - ((1.03211182532580e-08) * axis5_realReference * axis5_realReference * axis5_realReference + (-3.67822483559402e-05) * axis5_realReference * axis5_realReference
				+ 0.0432501678284173 * axis5_realReference + 0.0354087938903302);
		}
		else if (1400 <= axis5_realReference && axis5_realReference < 1600)
		{
			axis5_real = axis5_realReference - ((4.66843760059769e-09) * axis5_realReference * axis5_realReference * axis5_realReference + (-2.15355753242042e-05) * axis5_realReference * axis5_realReference
				+ 0.0432501678284173 * axis5_realReference - 16.9040836936982);
		}
		else
		{
			axis5_real = axis5_realReference;
		};
		axis5_real = round(axis5_real * 100000) / 100000;//元整保留五位小数
		ui.actualPosion_mm->setNum(axis5_real);
	}
	else if (axisNumber == 6)
	{
		ui.actualPosion_mm->setNum((moveControlCardPtr->dEncodePos[axisIndex]) * 0.0005);
	}
	else if (axisNumber == 7)
	{
		ui.actualPosion_mm->setNum((moveControlCardPtr->dEncodePos[axisIndex]) * 0.002);
	}
	else
	{

	};

};
void AxisMeasurement::updateTrapSettings(short axisNumber)
{
	cout << "updateTrapSettings" << endl;
	short axisIndex = axisNumber - 1;
	ui.trapAcceleratedSpeed->setText(QString::number(moveControlCardPtr->trapManual[axisIndex].acc));
	ui.trapDeclarationSpeed->setText(QString::number(moveControlCardPtr->trapManual[axisIndex].dec));
	ui.trapSmoothTime->setText(QString::number(moveControlCardPtr->trapManual[axisIndex].smoothTime));
	ui.trapSpeed->setText(QString::number(moveControlCardPtr->trapVelManual[axisIndex]));
	ui.stepLength->setText(QString::number(moveControlCardPtr->trapStepLengthManual[axisIndex]));

};
void AxisMeasurement::updateJogSettings(short axisNumber)
{
	cout << "updateJogSettings" << endl;
	short axisIndex = axisNumber - 1;
	ui.jogSpeed->setText(QString::number(moveControlCardPtr->jogVel[axisIndex]));
	ui.jogAcceleratedSpeed->setText(QString::number(moveControlCardPtr->jog[axisIndex].acc));
	ui.jogDecelerationSpeed->setText(QString::number(moveControlCardPtr->jog[axisIndex].dec));
	ui.jogSmooth->setText(QString::number(moveControlCardPtr->jog[axisIndex].smooth));
};
void AxisMeasurement::changeLedColor(QLabel* qlabelName, bool color)
{
	if (color)
	{
		qlabelName->setStyleSheet("QLabel{background-color:#DC2626;border-radius:3px;}");
	}
	else
	{
		qlabelName->setStyleSheet("QLabel{background-color:#16A34A;border-radius:3px;}");
	};
};

/// 传感器控制控件槽函数

void AxisMeasurement::on_selectCamera_currentIndexChanged(int nIndex)
{
	cout << "selectCamera" << nIndex << endl;
	currentCamNum = nIndex;
	ui.startCamCapture->setEnabled(true);
	ui.stopCamCapture->setEnabled(false);
	ui.selectCamera->setEnabled(true);
	ui.saveImg->setEnabled(false);
};
void AxisMeasurement::on_saveImg_clicked()
{
	cout << "saveImg" << endl;
	QString filename = QFileDialog::getSaveFileName(this, tr("Save Image"), "", tr(" (*.bmp );;(*.png );;(*.jpg)")); //选择路径
	string fileAsSave = filename.toStdString();
	cout << fileAsSave << endl;
	if (fileAsSave!="")
	{
		cameraPtrList[currentCamNum]->saveImg(fileAsSave, 0, 0, 0);
	};
	
};
void AxisMeasurement::on_startCamCapture_clicked()
{
	cout << "开始采集的是相机" << currentCamNum << endl;
	cameraPtrList[currentCamNum]->m_captureMode = "continuous";
	cameraPtrList[currentCamNum]->startCapture();
	m_camThread_ptrList[currentCamNum]->start();
	ui.stopCamCapture->setEnabled(true);
	ui.startCamCapture->setEnabled(false);
	ui.selectCamera->setEnabled(false);
	ui.saveImg->setEnabled(false);
	camCaptureFlag[currentCamNum] = true;
};
void AxisMeasurement::on_stopCamCapture_clicked()
{
	cout << "结束采集相机" << currentCamNum << endl;
	cameraPtrList[currentCamNum]->stopCapture();
	m_camThread_ptrList[currentCamNum]->requestInterruption();
	m_camThread_ptrList[currentCamNum]->quit();
	m_camThread_ptrList[currentCamNum]->exit();
	ui.startCamCapture->setEnabled(true);
	ui.stopCamCapture->setEnabled(false);
	ui.saveImg->setEnabled(true);
	ui.selectCamera->setEnabled(true);
	camCaptureFlag[currentCamNum] = false;
};
void AxisMeasurement::on_exposeTime_editingFinished()
{
	int newExposeTime = ui.exposeTime->text().toInt();
	cout << "  曝光时间设置：相机 " << currentCamNum << "  曝光时间  " << newExposeTime << endl;
	cameraPtrList[currentCamNum]->setExposeTime(newExposeTime);
};
void AxisMeasurement::on_gain_editingFinished()
{
	double newGain = ui.exposeTime->text().toDouble();
	cout << "增益设置：相机" << currentCamNum << "增益" << newGain << endl;
	cameraPtrList[currentCamNum]->setGain(newGain);
};
void AxisMeasurement::on_getLsResult_clicked()
{
	cout << "获取光幕传感器测量结果" << endl;
	float lsResult = lsSensorPtr->getLsMeasurementValue(1);//获取光幕传感器的对应结果
	QString measureDataStr = QString::number(lsResult);
	// ui.lsOut1->setText(measureDataStr);
};

//设备信息提示槽函数
void AxisMeasurement::showTime()
{
	dateTime = QDateTime::currentDateTime();
	ui.systemTime->setText(dateTime.toString("yyyy/MM/dd hh:mm"));
};
void AxisMeasurement::showDeviceErrorInf(QString errorInf)
{
	if (errorInf.contains(QStringLiteral("相机"))
		|| errorInf.contains(QStringLiteral("图像"))
		|| errorInf.contains(QStringLiteral("采集")))
	{
		ui.orginaImg->clear();
		ui.orginaImg->setText(QStringLiteral("图像采集异常\n请检查相机连接与采集状态"));
		ui.frame1->setProperty("imageState", "error");
		ui.orginaImg->setProperty("imageState", "error");
		ui.frame1->style()->unpolish(ui.frame1);
		ui.frame1->style()->polish(ui.frame1);
		ui.orginaImg->style()->unpolish(ui.orginaImg);
		ui.orginaImg->style()->polish(ui.orginaImg);
		ui.frame1->update();
		ui.orginaImg->update();
	}
	QMessageBox::warning(NULL, "warning!", errorInf, QMessageBox::Ok, QMessageBox::Ok);
};
void AxisMeasurement::showTips(QString tipsInf)
{
	QMessageBox::information(NULL, "通知!", tipsInf, QMessageBox::Ok, QMessageBox::Ok);
};
void AxisMeasurement::showDeviceInf(QString deviceInf)//用于显示设备状态栏信息
{
	ui.deviceInf->setText(deviceInf);

	const QString normalizedInfo = deviceInf.trimmed();
	const bool isError = normalizedInfo.contains(QStringLiteral("异常"))
		|| normalizedInfo.contains(QStringLiteral("失败"))
		|| normalizedInfo.contains(QStringLiteral("未全部"));
	const bool isWorking = normalizedInfo.contains(QStringLiteral("正在"))
		|| normalizedInfo.contains(QStringLiteral("运行中"))
		|| normalizedInfo.contains(QStringLiteral("测量中"));
	const bool isSuccess = normalizedInfo.contains(QStringLiteral("成功"))
		|| normalizedInfo.contains(QStringLiteral("已打开"))
		|| normalizedInfo.contains(QStringLiteral("已经打开"));
	const bool isWarning = normalizedInfo.contains(QStringLiteral("请"))
		|| normalizedInfo.contains(QStringLiteral("取消"))
		|| normalizedInfo.contains(QStringLiteral("停止"));
	const char* statusType = isError ? "error"
		: isWorking ? "working"
		: isSuccess ? "success"
		: isWarning ? "warning"
		: "idle";
	if (ui.deviceInf->property("statusType").toByteArray() != statusType)
	{
		ui.deviceInf->setProperty("statusType", statusType);
		ui.deviceInf->style()->unpolish(ui.deviceInf);
		ui.deviceInf->style()->polish(ui.deviceInf);
		ui.deviceInf->update();
	}

	if (m_statusInfoLabel)//P1-8 同步到状态栏提示区
		m_statusInfoLabel->setText(deviceInf);
};
//P1-8 状态栏设备状态灯：绿点=已打开，红点=未打开
void AxisMeasurement::updateDeviceStatus(bool online)
{
	if (!m_deviceStatusLabel)
		return;
	if (online)
	{
		m_deviceStatusLabel->setText(QStringLiteral("<span style='color:#16A34A;font-size:16px;'>&#9679;</span> 设备已打开"));
		m_deviceStatusLabel->setStyleSheet("color:#16A34A;font-weight:bold;");
	}
	else
	{
		m_deviceStatusLabel->setText(QStringLiteral("<span style='color:#DC2626;font-size:16px;'>&#9679;</span> 设备未打开"));
		m_deviceStatusLabel->setStyleSheet("color:#DC2626;font-weight:bold;");
	}
};

//P2-9/10/11 布局重构：把 .ui 的绝对定位布局重组为 QSplitter + QTabWidget，并设置仪表盘字体
//原理：只把现成控件 reparent 进新容器，控件 objectName 不变，所有 ui.xxx 引用和信号槽连接保持有效
void AxisMeasurement::restructureMainLayout()
{
	//—— P2-10：窗口自适应 ——最小 1440x900，可自由放大缩小（初始尺寸仍由 .ui 的 geometry 决定）
	setMinimumSize(1440, 900);

	//—— 布局修复（2026-09-10）：.ui 里各分组框内部都是绝对定位，没有尺寸提示，
	//   直接进布局会被压到只剩标题、内容被裁掉。这里统一处理：
	//   1) 布局接管前先读取各面板的 .ui 原始设计尺寸，设为最小尺寸 → 内容永不被裁；
	//   2) 高大/可压缩的面板包进 QScrollArea → 窗口太小时出滚动条，而不是遮挡或裁切。
	auto wrapScroll = [](QWidget* panel, const QSize& minViewport) -> QScrollArea* {
		panel->setMinimumSize(panel->geometry().size());//此时 geometry 仍是 .ui 设计值
		QScrollArea* scrollArea = new QScrollArea();
		scrollArea->setWidget(panel);
		scrollArea->setWidgetResizable(true);//空间够时填满；低于最小尺寸时自动出滚动条
		scrollArea->setFrameShape(QFrame::NoFrame);
		scrollArea->setMinimumSize(minViewport);
		return scrollArea;
	};

	//========== 一、自动测量页（autoMeasureUI）==========
	//常驻：设备控制 + 程序选择(groupBox) + 光幕实时显示(groupBox_2) + 图像区(frame1)
	//收纳：顶尖/旋转/光幕位置控制(autoMoveAdjust，内部三段) → 页签
	ui.pushButton->hide();//"图像绘制"调试按钮：槽函数逻辑已全部注释，不再占用界面位置

	//顶尖/旋转/光幕位置控制收进页签（内部绝对定位，包滚动区防裁切）
	QTabWidget* adjustTabs = new QTabWidget(ui.autoMeasureUI);
	adjustTabs->setObjectName(QStringLiteral("autoAdjustTabs"));
	QScrollArea* autoMoveScroll = wrapScroll(ui.autoMoveAdjust, QSize(300, 270));
	adjustTabs->addTab(autoMoveScroll, QStringLiteral("顶尖 / 旋转 / 光幕位置"));
	adjustTabs->setMinimumHeight(300);

	// 左下控制区改用布局管理，拖拽分割条时标题、按钮和白色背景板一起横向伸缩。
	ui.autoMoveAdjust->setMinimumSize(321, 270);
	QWidget* apexPanel = ui.apexMoveUp->parentWidget();
	QWidget* rotatePanel = ui.partRotate_clockwise->parentWidget();
	QWidget* lightPanel = ui.lsMoveUp->parentWidget();
	QVBoxLayout* autoMoveLayout = new QVBoxLayout(ui.autoMoveAdjust);
	autoMoveLayout->setContentsMargins(10, 4, 10, 8);
	autoMoveLayout->setSpacing(1);//顶尖/旋转/光幕位置距离后文的位置
	QLabel* autoMoveSectionTitles[] = { ui.label_44, ui.label_20, ui.label_65 };
	for (QLabel* title : autoMoveSectionTitles) {
		title->setAlignment(Qt::AlignCenter);
		title->setFixedHeight(24);
		title->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	}
	if (apexPanel) {
		apexPanel->setMinimumHeight(72);
		apexPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		if (QGridLayout* apexGrid = qobject_cast<QGridLayout*>(apexPanel->layout())) {
			apexGrid->setColumnStretch(0, 1);
			apexGrid->setColumnStretch(1, 1);
		}
	}
	if (rotatePanel) {
		rotatePanel->setMinimumHeight(40);
		rotatePanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		if (QGridLayout* rotateGrid = qobject_cast<QGridLayout*>(rotatePanel->layout())) {
			rotateGrid->setColumnStretch(0, 1);
			rotateGrid->setColumnStretch(1, 1);
		}
	}
	if (lightPanel) {
		lightPanel->setMinimumHeight(40);
		lightPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		if (QGridLayout* lightMoveGrid = qobject_cast<QGridLayout*>(lightPanel->layout())) {
			lightMoveGrid->setColumnStretch(0, 1);
			lightMoveGrid->setColumnStretch(1, 1);
		}
	}
	autoMoveLayout->addWidget(ui.label_44);
	if (apexPanel) autoMoveLayout->addWidget(apexPanel);
	autoMoveLayout->addWidget(ui.label_20);
	if (rotatePanel) autoMoveLayout->addWidget(rotatePanel);
	autoMoveLayout->addWidget(ui.label_65);
	if (lightPanel) autoMoveLayout->addWidget(lightPanel);
	autoMoveLayout->addStretch(1);
	QPushButton* autoMoveButtons[] = {
		ui.apexMoveUp, ui.apexMoveDown, ui.partRotate_anticlockwise,
		ui.partRotate_clockwise, ui.lsMoveUp, ui.lsMoveDown
	};
	for (QPushButton* button : autoMoveButtons) {
		button->setProperty("controlRole", "motion");
		button->setMinimumHeight(32);
		button->setMaximumHeight(34);
		button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	}
	QWidget* autoLeftPanel = new QWidget(ui.autoMeasureUI);
	autoLeftPanel->setMinimumSize(ui.groupBox->geometry().width(), 660);//左栏内容的最小可读尺寸，低分辨率时交给滚动区
	QVBoxLayout* autoLeftLayout = new QVBoxLayout(autoLeftPanel);
	autoLeftPanel->setObjectName(QStringLiteral("leftDashboardPanel"));
	autoLeftLayout->setContentsMargins(0, 0, 14, 6);
	autoLeftLayout->setSpacing(6);
	ui.groupBox->setObjectName(QStringLiteral("leftProcessCard"));
	ui.groupBox_2->setObjectName(QStringLiteral("leftLightCurtainCard"));
	ui.label_39->setObjectName(QStringLiteral("leftCardTitle"));
	ui.label_32->setObjectName(QStringLiteral("leftCardTitle"));
	ui.programProcess->setProperty("processState", "idle");
	ui.programProgressBar->setProperty("progressState", "idle");
	ui.lsDiameter->setProperty("displayRole", "measurement");
	ui.lsCurrentValue->setProperty("displayRole", "realtime");
	ui.lsCurrentValue->setProperty("displayState", "idle");
	ui.lsCurrentValue->setText(QStringLiteral("等待数据"));
	ui.label_39->setStyleSheet(QStringLiteral(""));
	ui.label_32->setStyleSheet(QStringLiteral(""));
	ui.label_39->setMinimumHeight(24);
	ui.label_32->setMinimumHeight(24);
	ui.label_39->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	ui.label_32->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	QGridLayout* processGrid = new QGridLayout();
	processGrid->setContentsMargins(0, 0, 0, 0);
	processGrid->setHorizontalSpacing(12);
	processGrid->setVerticalSpacing(14);
	processGrid->setColumnStretch(0, 0);
	processGrid->setColumnStretch(1, 1);
	ui.label_40->setMinimumWidth(112);
	ui.label_31->setMinimumWidth(112);
	ui.label_21->setMinimumWidth(112);
	ui.partNub->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	ui.programProcess->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	ui.programProgressBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	processGrid->addWidget(ui.label_40, 0, 0);
	processGrid->addWidget(ui.partNub, 0, 1);
	processGrid->addWidget(ui.label_31, 1, 0);
	processGrid->addWidget(ui.programProcess, 1, 1);
	processGrid->addWidget(ui.label_21, 2, 0, 1, 2);
	processGrid->addWidget(ui.programProgressBar, 3, 0, 1, 2);
	QVBoxLayout* processCardLayout = new QVBoxLayout(ui.groupBox);
	processCardLayout->setContentsMargins(14, 4, 14, 14);
	processCardLayout->setSpacing(3);//程序测量进程与后文的距离
	processCardLayout->addWidget(ui.label_39, 0, Qt::AlignTop | Qt::AlignHCenter);
	processCardLayout->addLayout(processGrid);

	QWidget* lightCurtainPanel = ui.lsCurrentValue->parentWidget();
	if (lightCurtainPanel) {
		QVBoxLayout* lightCurtainCardLayout = new QVBoxLayout(ui.groupBox_2);
		lightCurtainCardLayout->setContentsMargins(14, 4, 14, 14);
		lightCurtainCardLayout->setSpacing(3);//光幕实时显示与后文的距离
		lightCurtainPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		if (QGridLayout* lightGrid = qobject_cast<QGridLayout*>(lightCurtainPanel->layout())) {
			lightGrid->setColumnStretch(0, 0);
			lightGrid->setColumnStretch(1, 1);
		}
		lightCurtainCardLayout->addWidget(ui.label_32, 0, Qt::AlignTop | Qt::AlignHCenter);
		lightCurtainCardLayout->addWidget(lightCurtainPanel);
	}
	ui.groupBox->setMinimumSize(321, ui.groupBox->geometry().height());//程序测量进程：宽度交给布局随分割条伸缩
	autoLeftLayout->addWidget(ui.groupBox);
	ui.groupBox_2->setMinimumSize(321, 160);
	ui.groupBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	ui.groupBox_2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	autoLeftLayout->addWidget(ui.groupBox_2);
	autoLeftLayout->addWidget(adjustTabs, 1);
	QScrollArea* autoLeftScroll = new QScrollArea(ui.autoMeasureUI);
	autoLeftScroll->setObjectName(QStringLiteral("autoLeftPanelScroll"));
	autoLeftScroll->setWidget(autoLeftPanel);
	autoLeftScroll->setWidgetResizable(true);
	autoLeftScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	autoLeftScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	autoLeftScroll->setFrameShape(QFrame::NoFrame);
	autoLeftScroll->setMinimumWidth(ui.groupBox->geometry().width() + 22);

	ui.frame1->setMinimumSize(380, 280);//图像区最小尺寸，防止被挤没
	ui.frame1->setProperty("imageState", "idle");
	ui.orginaImg->setProperty("imageState", "idle");
	ui.orginaImg->setText(QStringLiteral("等待图像\n打开设备并开始测量后显示"));
	ui.orginaImg->setAlignment(Qt::AlignCenter);
	ui.orginaImg->setWordWrap(true);

	QSplitter* autoContentSplitter = new QSplitter(Qt::Horizontal, ui.autoMeasureUI);
	autoContentSplitter->setObjectName(QStringLiteral("autoContentSplitter"));
	autoContentSplitter->addWidget(autoLeftScroll);
	autoContentSplitter->addWidget(ui.frame1);//图像显示区
	autoContentSplitter->setStretchFactor(1, 1);//图像区优先吃掉多余空间
	autoContentSplitter->setCollapsible(0, false);
	autoContentSplitter->setCollapsible(1, false);
	autoContentSplitter->setSizes({ 374, 800 });

	//设备控制条：用布局接管旧的固定坐标行，避免首次打开时按钮被水平裁切
	ui.autoDeviceControl->setObjectName(QStringLiteral("topDeviceBarCard"));
	ui.deviceInf->setObjectName(QStringLiteral("topDeviceInfo"));
	ui.deviceInf->setProperty("statusType", "idle");
	ui.programNumber->setObjectName(QStringLiteral("topProgramCombo"));
	ui.openAllDevice->setProperty("buttonRole", "outline");
	ui.closeAllDevice->setProperty("buttonRole", "secondary");
	ui.allAxisGoHome->setProperty("buttonRole", "secondary");
	ui.startAutoMearsurement->setProperty("buttonRole", "primary");
	ui.measureCancel->setProperty("buttonRole", "secondary");
	ui.programConfirm->setProperty("buttonRole", "secondary");
	ui.urgrentStopMearsure->setProperty("buttonRole", "danger");
	if (QWidget* legacyTopRow = ui.autoDeviceControl->findChild<QWidget*>(QStringLiteral("layoutWidget"), Qt::FindDirectChildrenOnly))
		legacyTopRow->hide();
	QPushButton* topButtons[] = {
		ui.openAllDevice, ui.closeAllDevice, ui.allAxisGoHome, ui.startAutoMearsurement,
		ui.measureCancel, ui.programConfirm, ui.urgrentStopMearsure
	};
	QVBoxLayout* topDeviceLayout = new QVBoxLayout(ui.autoDeviceControl);
	topDeviceLayout->setContentsMargins(14, 10, 14, 10);
	topDeviceLayout->setSpacing(8);
	QHBoxLayout* topDeviceRow = new QHBoxLayout();
	topDeviceRow->setContentsMargins(0, 0, 0, 0);
	topDeviceRow->setSpacing(8);
	ui.label_19->setMinimumWidth(44);
	ui.label_19->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	topDeviceRow->addWidget(ui.label_19);
	//topDeviceRow->addSpacing(1);//程序模块下拉框和程序文本框之间的距离处理  距离设置有问题

	/*ui.programNumber->setMinimumWidth(168);
	ui.programNumber->setMinimumHeight(32);
	ui.programNumber->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	topDeviceRow->addWidget(ui.programNumber, 1);*/

	ui.programNumber->setMinimumWidth(168);
	ui.programNumber->setMaximumWidth(240);
	ui.programNumber->setMinimumHeight(32);
	ui.programNumber->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	topDeviceRow->addWidget(ui.programNumber);
	topDeviceRow->addSpacing(6);

	for (QPushButton* button : topButtons) {
		button->setMinimumWidth(76);
		button->setMinimumHeight(32);
		button->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
		topDeviceRow->addWidget(button);
	}
	ui.closeAllDevice->setMinimumWidth(96);
	ui.startAutoMearsurement->setMinimumWidth(84);
	ui.urgrentStopMearsure->setMinimumWidth(88);
	topDeviceLayout->addLayout(topDeviceRow);
	ui.deviceInf->setMinimumHeight(24);
	ui.deviceInf->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	topDeviceLayout->addWidget(ui.deviceInf);
	ui.autoDeviceControl->setMinimumSize(820, 92);
	ui.autoDeviceControl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	QScrollArea* autoDeviceScroll = new QScrollArea(ui.autoMeasureUI);
	autoDeviceScroll->setObjectName(QStringLiteral("autoDeviceControlScroll"));
	autoDeviceScroll->setWidget(ui.autoDeviceControl);
	autoDeviceScroll->setWidgetResizable(true);
	autoDeviceScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	autoDeviceScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	autoDeviceScroll->setFrameShape(QFrame::NoFrame);
	autoDeviceScroll->setMinimumHeight(ui.autoDeviceControl->minimumHeight() + 10);

	QVBoxLayout* autoLayout = new QVBoxLayout(ui.autoMeasureUI);
	autoLayout->setContentsMargins(10, 10, 10, 10);
	autoLayout->setSpacing(10);
	autoLayout->addWidget(autoDeviceScroll);//设备控制条（水平滚动防遮挡）
	autoLayout->addWidget(autoContentSplitter, 1);
	ui.label_51->hide();//游离标题"设备控制"：分组框卡片化后由边框+内容自明

	//========== 二、手动控制页（ManualControlUI）==========
	//按 .ui 原始设计还原三区并列：运动轴设置(axisControl) | 轴状态显示(axisMonitor) | 相机图像(frame)
	//底部页签：点位运动 / Jog运动 / 相机采集 / 光幕传感器
	QTabWidget* manualTabs = new QTabWidget(ui.ManualControlUI);
	manualTabs->setObjectName(QStringLiteral("manualControlTabs"));
	manualTabs->addTab(wrapScroll(ui.trapControl, QSize(240, 240)), QStringLiteral("点位运动"));
	manualTabs->addTab(wrapScroll(ui.jogControl, QSize(240, 240)), QStringLiteral("Jog运动"));
	manualTabs->addTab(wrapScroll(ui.cameraControl, QSize(240, 240)), QStringLiteral("相机采集"));
	manualTabs->addTab(wrapScroll(ui.LS9000, QSize(240, 240)), QStringLiteral("光幕传感器"));
	manualTabs->setMinimumHeight(300);

	//相机图像区保留自己的标题
	QWidget* cameraPanel = new QWidget(ui.ManualControlUI);
	QVBoxLayout* cameraLayout = new QVBoxLayout(cameraPanel);
	cameraLayout->setContentsMargins(0, 0, 0, 0);
	cameraLayout->setSpacing(2);
	cameraLayout->addWidget(ui.label_52, 0, Qt::AlignLeft);//"相机图像"标题
	ui.frame->setMinimumSize(350, 280);//相机图像区最小尺寸
	cameraLayout->addWidget(ui.frame, 1);

	QSplitter* manualTopSplitter = new QSplitter(Qt::Horizontal, ui.ManualControlUI);
	manualTopSplitter->setObjectName(QStringLiteral("manualTopSplitter"));
	manualTopSplitter->addWidget(wrapScroll(ui.axisControl, QSize(180, 300)));//运动轴设置（窄高面板，滚动兜底）
	manualTopSplitter->addWidget(wrapScroll(ui.axisMonitor, QSize(300, 300)));//轴状态显示（常驻）
	manualTopSplitter->addWidget(cameraPanel);
	manualTopSplitter->setStretchFactor(2, 1);//相机图像区优先吃掉多余空间
	manualTopSplitter->setCollapsible(0, false);
	manualTopSplitter->setCollapsible(1, false);
	manualTopSplitter->setCollapsible(2, false);
	manualTopSplitter->setSizes({ 200, 380, 600 });
	manualTopSplitter->setMinimumHeight(320);

	QVBoxLayout* manualLayout = new QVBoxLayout(ui.ManualControlUI);
	manualLayout->setContentsMargins(10, 10, 10, 10);
	manualLayout->setSpacing(6);
	manualLayout->addWidget(manualTopSplitter, 1);
	manualLayout->addWidget(manualTabs);

	//游离段落标题已由页签文字替代，隐藏避免与新布局重叠
	ui.label_43->hide();//"运动轴设置"
	ui.label_45->hide();//"点位运动模式"
	ui.label_46->hide();//"Jog运动模式"
	ui.label_47->hide();//"轴状态显示"（组框卡片化后自明）
	ui.label_49->hide();//"相机采集控制"
	ui.label_50->hide();//"光幕传感器"

	//========== 三、中央区（widget）：左（页面区）| 右（统计+结果），可拖动收放 ==========
	QWidget* rightPanel = new QWidget(ui.widget);
	rightPanel->setObjectName(QStringLiteral("rightDashboardPanel"));
	rightPanel->setMinimumWidth(500);//测量统计与结果表的最小可读宽度，避免首次打开时右栏把窗口撑出屏幕
	rightPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	ui.groupBox_6->setObjectName(QStringLiteral("rightStatsCard"));
	ui.groupBox_7->setObjectName(QStringLiteral("rightResultCard"));
	ui.groupBox_6->setMinimumWidth(480);
	ui.groupBox_7->setMinimumWidth(480);
	ui.groupBox_6->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	ui.groupBox_7->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	ui.label_60->setAlignment(Qt::AlignCenter);
	ui.label_60->setMinimumHeight(24);
	ui.label_60->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	ui.label_60->setObjectName(QStringLiteral("rightCardTitle"));
	ui.label_60->setStyleSheet(QStringLiteral(""));
	ui.label_61->setAlignment(Qt::AlignCenter);
	ui.label_61->setMinimumHeight(24);
	ui.label_61->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	ui.label_61->setObjectName(QStringLiteral("rightCardTitle"));
	ui.label_61->setStyleSheet(QStringLiteral(""));
	ui.partsId->setMinimumWidth(80);
	ui.partsId->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	ui.operatorName->setMinimumWidth(80);
	ui.operatorName->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	ui.systemTime->setProperty("textRole", "secondary");
	ui.measurePartsNum->setProperty("metricRole", "total");
	ui.okPartsNum_All->setProperty("metricRole", "ok");
	ui.ngPartsNum_All->setProperty("metricRole", "ng");
	ui.yield_All->setProperty("metricRole", "yield");
	ui.programMeasureNum->setProperty("metricRole", "total");
	ui.ngFeatureNum->setProperty("metricRole", "ng");
	ui.programYield->setProperty("metricRole", "yield");
	ui.measurePartsNum->setText(QStringLiteral("--"));
	ui.okPartsNum_All->setText(QStringLiteral("--"));
	ui.ngPartsNum_All->setText(QStringLiteral("--"));
	ui.yield_All->setText(QStringLiteral("--"));
	ui.programMeasureNum->setText(QStringLiteral("--"));
	ui.ngFeatureNum->setText(QStringLiteral("--"));
	ui.programYield->setText(QStringLiteral("--"));
	ui.measureResultFlag->setProperty("resultState", "idle");
	ui.measureResultFlag->setText(QStringLiteral("--"));
	ui.saveMeasureResult->setProperty("buttonRole", "primary");
	ui.clearMeasureResult->setProperty("buttonRole", "secondary");
	ui.zeroMeasureNum->setProperty("buttonRole", "dangerOutline");
	ui.creatDatabase->setProperty("buttonRole", "outline");
	QWidget* statLayoutWidget = ui.groupBox_6->findChild<QWidget*>(QStringLiteral("layoutWidget_5"));
	QGridLayout* statLayout = statLayoutWidget ? qobject_cast<QGridLayout*>(statLayoutWidget->layout()) : nullptr;
	if (statLayout) {
		statLayout->setContentsMargins(4, 2, 4, 4);
		statLayout->setHorizontalSpacing(8);
		statLayout->setVerticalSpacing(10);
		statLayout->setColumnStretch(0, 0);
		statLayout->setColumnStretch(1, 1);
		statLayout->setColumnStretch(2, 0);
		statLayout->setColumnStretch(3, 1);
	}
	if (statLayoutWidget) {
		QVBoxLayout* statBoxLayout = new QVBoxLayout(ui.groupBox_6);
		statBoxLayout->setContentsMargins(14, 4, 14, 14);
		statBoxLayout->setSpacing(10);
		QWidget* statTitleRow = new QWidget(ui.groupBox_6);
		QGridLayout* statTitleLayout = new QGridLayout(statTitleRow);
		statTitleLayout->setContentsMargins(0, 0, 0, 0);
		statTitleLayout->setSpacing(8);
		statTitleLayout->addWidget(ui.label_60, 0, 1, Qt::AlignTop | Qt::AlignHCenter);
		statTitleLayout->addWidget(ui.systemTime, 0, 2, Qt::AlignTop | Qt::AlignRight);
		statTitleLayout->setColumnStretch(0, 1);
		statTitleLayout->setColumnStretch(1, 1);
		statTitleLayout->setColumnStretch(2, 1);
		statBoxLayout->addWidget(statTitleRow);
		statBoxLayout->addWidget(statLayoutWidget);
		statLayoutWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	}
	QWidget* resultSummaryWidget = ui.groupBox_7->findChild<QWidget*>(QStringLiteral("layoutWidget_6"));
	QGridLayout* resultSummaryLayout = resultSummaryWidget ? qobject_cast<QGridLayout*>(resultSummaryWidget->layout()) : nullptr;
	if (resultSummaryLayout) {
		resultSummaryLayout->setColumnStretch(0, 0);
		resultSummaryLayout->setColumnStretch(1, 1);
		resultSummaryLayout->setColumnStretch(2, 0);
		resultSummaryLayout->setColumnStretch(3, 1);
	}
	ui.measureTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	ui.measureTable->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	ui.measureTable->setSizeAdjustPolicy(QAbstractScrollArea::AdjustIgnored);
	ui.measureTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
	ui.measureTable->horizontalHeader()->setStretchLastSection(false);
	if (resultSummaryWidget) {
		QVBoxLayout* resultBoxLayout = new QVBoxLayout(ui.groupBox_7);
		resultBoxLayout->setContentsMargins(14, 4, 14, 14);
		resultBoxLayout->setSpacing(10);
		resultBoxLayout->addWidget(ui.label_61, 0, Qt::AlignTop | Qt::AlignHCenter);
		resultBoxLayout->addWidget(resultSummaryWidget, 0);
		resultBoxLayout->addWidget(ui.measureTable, 1);
		resultSummaryWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		ui.measureTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	}
	QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
	rightLayout->setContentsMargins(10, 10, 10, 10);
	rightLayout->setSpacing(12);
	ui.groupBox_6->setMinimumHeight(124);//测量统计：卡片化后保留可读高度，低分辨率由右侧滚动区兜底
	rightLayout->addWidget(ui.groupBox_6);
	ui.groupBox_7->setMinimumHeight(240);//测量结果表最小高度
	rightLayout->addWidget(ui.groupBox_7, 1);//测量结果（吃掉剩余高度）

	m_mainSplitter = new QSplitter(Qt::Horizontal, ui.widget);
	m_mainSplitter->setObjectName(QStringLiteral("mainContentSplitter"));
	QScrollArea* rightScrollArea = new QScrollArea(ui.widget);
	rightScrollArea->setObjectName(QStringLiteral("rightStatisticsScroll"));
	rightScrollArea->setWidget(rightPanel);
	rightScrollArea->setWidgetResizable(true);
	rightScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	rightScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	rightScrollArea->setFrameShape(QFrame::NoFrame);
	rightScrollArea->setMinimumWidth(320);
	m_mainSplitter->addWidget(ui.uiWidget);//两个功能页
	m_mainSplitter->addWidget(rightScrollArea);
	m_mainSplitter->setStretchFactor(0, 2);
	m_mainSplitter->setStretchFactor(1, 1);
	m_mainSplitter->setCollapsible(0, false);
	m_mainSplitter->setCollapsible(1, false);
	m_mainSplitter->setSizes({ 920, 480 });//首次打开时优先保证所有顶部按钮和右侧卡片可见

	QVBoxLayout* centralLayout = new QVBoxLayout(ui.widget);
	centralLayout->setContentsMargins(0, 0, 0, 0);
	centralLayout->addWidget(m_mainSplitter);


	QScreen* currentScreen = screen() ? screen() : QApplication::primaryScreen();
	if (currentScreen) {
		const QRect available = currentScreen->availableGeometry();
		const int availableWidth = qMax(640, available.width() - 80);
		const int availableHeight = qMax(520, available.height() - 80);
		const int initialWidth = qMin(1440, availableWidth);
		const int initialHeight = qMin(900, availableHeight);
		setMinimumSize(qMin(1180, initialWidth), qMin(760, initialHeight));
		resize(initialWidth, initialHeight);
		move(available.center() - rect().center());
		rightLayout->activate();
		const int rightMinimum = qMax(500, rightPanel->minimumSizeHint().width()) + 20;
		rightScrollArea->setMinimumWidth(rightMinimum);
		const int usableWidth = initialWidth - m_mainSplitter->handleWidth();
		const int rightWidth = qMax(rightMinimum, usableWidth * 38 / 100);
		m_mainSplitter->setSizes({ qMax(1, usableWidth - rightWidth), rightWidth });
	}
	QLabel* autoMoveTitles[] = { ui.label_44, ui.label_20, ui.label_65 };
	for (QLabel* title : autoMoveTitles) {
		title->setProperty("textRole", "sectionTitle");
		title->setAlignment(Qt::AlignCenter);
		title->setMinimumHeight(24);
		title->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		title->setStyleSheet(QStringLiteral(""));
	}

	//========== 四、P2-11 关键数值仪表盘化：等宽字体 Consolas + 语义色 ==========
	//（布局修复：字号适配容器高度，光幕组布局区已同步加高，数值完整显示不裁切）
	QFont liveValueFont(QStringLiteral("Consolas"), 20, QFont::Bold);//光幕实时数值
	ui.lsCurrentValue->setFont(liveValueFont);
	ui.lsCurrentValue->setAlignment(Qt::AlignCenter);
	ui.lsCurrentValue->setStyleSheet(QStringLiteral(""));

	QFont diameterFont(QStringLiteral("Consolas"), 18, QFont::Bold);//直径
	ui.lsDiameter->setFont(diameterFont);
	ui.lsDiameter->setAlignment(Qt::AlignCenter);
	ui.lsDiameter->setStyleSheet(QStringLiteral(""));

	QFont channelFont(QStringLiteral("Consolas"), 20, QFont::Bold);//光幕 4 通道 + 补偿前
	ui.lsMeasureOut1->setFont(channelFont);
	ui.lsMeasureOut2->setFont(channelFont);
	ui.lsMeasureOut3->setFont(channelFont);
	ui.lsMeasureOut4->setFont(channelFont);
	ui.lsMeasureOut1->setAlignment(Qt::AlignCenter);
	ui.lsMeasureOut2->setAlignment(Qt::AlignCenter);
	ui.lsMeasureOut3->setAlignment(Qt::AlignCenter);
	ui.lsMeasureOut4->setAlignment(Qt::AlignCenter);
	ui.lsMeasureOut1->setStyleSheet("color:#16A34A;");//通道值：绿
	ui.lsMeasureOut2->setStyleSheet("color:#16A34A;");
	ui.lsMeasureOut3->setStyleSheet("color:#16A34A;");
	ui.lsMeasureOut4->setStyleSheet("color:#16A34A;");
	ui.lsCurrentValue_2->setFont(channelFont);
	ui.lsCurrentValue_2->setAlignment(Qt::AlignCenter);
	ui.lsCurrentValue_2->setMinimumHeight(36);//加高适配 20pt 字体防裁切
	ui.lsCurrentValue_2->setStyleSheet("color:#B45309;");//补偿前数值：琥珀（原始值）
	//测量结果表：数值列等宽字体加粗（表宽受限取 12pt，中文自动回退雅黑）
	ui.measureTable->setFont(QFont(QStringLiteral("Consolas"), 10, QFont::Bold));
};

//P2-12 进度条平滑过渡：OutCubic 缓动，400ms 滑动到目标值（替代生硬跳变）
void AxisMeasurement::setProgramProgressSmooth(int value)
{
	if (value <= 0 || value >= 100)
	{
		//0/100 及重置场景直接设置，不做动画
		if (m_progressAnim && m_progressAnim->state() == QAbstractAnimation::Running)
			m_progressAnim->stop();
		ui.programProgressBar->setValue(value);
		return;
	}
	if (!m_progressAnim)
	{
		m_progressAnim = new QPropertyAnimation(ui.programProgressBar, "value", this);
		m_progressAnim->setEasingCurve(QEasingCurve::OutCubic);
		m_progressAnim->setDuration(400);
	}
	int current = ui.programProgressBar->value();
	if (current == value)
		return;
	if (m_progressAnim->state() == QAbstractAnimation::Running)
		m_progressAnim->stop();
	m_progressAnim->setStartValue(current);
	m_progressAnim->setEndValue(value);
	m_progressAnim->start();
};

//P2-12 急停视觉警示：全窗口红色边框闪烁 3 次（边缘 QFrame + 透明度动画，鼠标穿透不影响操作）
void AxisMeasurement::flashEmergencyBorder()
{
	if (!m_alertFrame)
	{
		m_alertFrame = new QFrame(this);
		m_alertFrame->setObjectName(QStringLiteral("emergencyAlertFrame"));
		m_alertFrame->setStyleSheet("QFrame{border:6px solid #DC2626;background:transparent;}");
		m_alertFrame->setAttribute(Qt::WA_TransparentForMouseEvents);//鼠标穿透，不拦截任何点击
		m_alertEffect = new QGraphicsOpacityEffect(m_alertFrame);
		m_alertFrame->setGraphicsEffect(m_alertEffect);
	}
	m_alertFrame->setGeometry(rect());//覆盖整个客户区
	m_alertFrame->raise();
	m_alertFrame->show();
	if (m_flashAnim && m_flashAnim->state() == QAbstractAnimation::Running)
		m_flashAnim->stop();//连按急停时重新开始闪烁
	if (!m_flashAnim)
	{
		m_flashAnim = new QPropertyAnimation(m_alertEffect, "opacity", this);
		m_flashAnim->setDuration(400);
		m_flashAnim->setStartValue(1.0);
		m_flashAnim->setEndValue(0.0);
		m_flashAnim->setLoopCount(3);//闪 3 次，共 1.2 秒
		connect(m_flashAnim, &QPropertyAnimation::finished, this, [this]() { m_alertFrame->hide(); });
	}
	m_flashAnim->start();
};

//P2-10 窗口缩放：保持急停警示边框始终覆盖全窗口
void AxisMeasurement::resizeEvent(QResizeEvent* event)
{
	QMainWindow::resizeEvent(event);
	if (m_alertFrame && m_alertFrame->isVisible())
		m_alertFrame->setGeometry(rect());
};

//线程槽函数
/*
函数功能：根据采集图像的来源，在主界面上显示图像，并在图像上绘制相应图形
函数参数： imgPrt（图像指针）；
           source（图像来源，"org"表明是自动测量过程中采集的图像，图像在自动测量截面显示；"cam0"表示是手动控制模式下远心相机采集的图像；"cam1"是手动控制模式下测孔相机采集的图像；
		          "cam2"是手动控制模式下粗糙度相机采集的图像；其中手动控制模式采集的图片会在手动控制界面显示）；
		   drawMode(图像绘图模式。0对应不需 要绘制形状的图像；1对应最长轴花键图像；2对应最长轴孔径图像；3对应镂空轴1号孔径图像；4对应镂空轴2号图像；5对应中长轴1号图像；6对应中长轴2号图像):

*/
void AxisMeasurement::displayImg(const Mat* imgPrt, QString source, int drawMode)
{
	//"org"/"processed"/"cam"对应显示在面板上的对应位置
	//cout << "这里是displayImg" <<  source.toStdString() << endl;
	Mat src = imgPrt->clone();
	QImage imgForDisplay;
	if (imgPrt->channels() == 3)//RGB Img
	{
		cv::cvtColor(src, src, cv::COLOR_BGR2RGB);//颜色空间转换,opencv各颜色通道排序不同
		imgForDisplay = QImage((const uchar*)(src.data), src.cols, src.rows, src.cols * src.channels(), QImage::Format_RGB888);
	}
	else//Gray Img或者2通道图像
	{
		imgForDisplay = QImage((const uchar*)(src.data), src.cols, src.rows, src.cols * src.channels(), QImage::Format_Indexed8);
	};
	if (source == "org")
	{
		int width = imgForDisplay.width();
		int height = imgForDisplay.height();
		int centerX = width / 2;
		int centerY = height / 2;
		QPainter painter(&imgForDisplay);
		painter.setRenderHint(QPainter::Antialiasing, true);//设置反锯齿
		QPen pen;
		pen.setWidth(5);
		pen.setColor(QColor(0, 255, 0));
		pen.setStyle(Qt::DashDotLine);
		painter.setPen(pen);
		switch (drawMode)
		{
		//如果需要绘制孔等轮廓参照下面格式
		case 1://最长轴花键图像
			painter.drawLine(centerX - 150, 0, centerX - 150, height);//起点和终点  
			painter.drawLine(centerX + 150, 0, centerX + 150, height);
			break;
		case 2://最长轴大孔图像
			painter.drawEllipse(QPoint(centerX, centerY - 120), 760, 760);
			break;
		case 3://最长轴小孔图像（预留）
			painter.drawEllipse(QPoint(centerX, centerY - 120), 760, 760);
			break;
		case 4://镂空轴1号侧孔位置（小孔）
			painter.drawEllipse(QPoint(centerX, centerY + 15), 138, 138);
			break;
		case 5://镂空轴2号侧孔位置（大孔）
			painter.drawEllipse(QPoint(centerX, centerY + 80), 295, 295);
			break;
		case 6://中长轴2号侧孔位置
			painter.drawEllipse(QPoint(centerX, centerY - 10), 420, 420);
			break;
		case 7://WZ10-15-1107-141（大圆孔）
			painter.drawEllipse(QPoint(1220, 1000), 265, 265);
			break;
		case 8://WZ10-44-1112-155（圆孔）
			painter.drawEllipse(QPoint(1202, 965), 475, 475);
			break;
		case 9://WZ10-44-1112-290（圆孔）
			painter.drawEllipse(QPoint(1223, 1035), 455, 455);
			break;
		case 10://WZ10-44-1112-290（键槽）
			painter.drawLine(630, 600, 630, 1300);//起点和终点 
			painter.drawLine(1820, 600, 1820, 1300);//起点和终点 
			
			break;
		case 11://WZ10-11-2006-430（圆角）
			painter.drawLine(500, 1636, 500, 1900);//起点和终点
			painter.drawLine(1965, 1636, 1965, 1900);//起点和终点
			break;
		case 12://WZ10-11-2006-430（9个小细棱）
			painter.drawLine(1000, 850, 1000, 1300);//起点和终点
			painter.drawLine(1358, 850, 1358, 1300);//起点和终点
			break;
		case 13://WZ10-11-2006-430（槽宽）
			painter.drawLine(590, 1435, 590, 1600);//起点和终点 
			painter.drawLine(1890, 1435, 1890, 1600);//起点和终点 
			break;
		case 14://PROGRAM11测试
			painter.drawEllipse(QPoint(centerX, centerY - 120), 600, 600);
			break;
		case 15://P12测试孔位置
			painter.drawEllipse(QPoint(1209, 900), 700, 700);
			break;
		case 16://xd-WZ10451080-370-3 测试孔位置
			painter.drawEllipse(QPoint(1209, 983), 850, 850);
			break;
		case 17://WZ10451080-4 测试孔位置
			painter.drawEllipse(QPoint(1209, 983),720, 720);
			break;
		case 18://WZ10151107-HJC 测试孔位置
			painter.drawEllipse(QPoint(1210, 1040), 341, 341);
			break;
		default://需要3.5
			break;
		};
		painter.end();
		//painter.drawEllipse(QPoint(centerX, centerY), 100, 100);
		imgForDisplay = imgForDisplay.scaled(ui.orginaImg->size(), Qt::KeepAspectRatio);//此函数还有一个参数是图像的转换格式（缺省）//此函数还有一个参数是图像的转换格式（缺省）
		ui.orginaImg->setPixmap(QPixmap::fromImage(imgForDisplay));
		if (ui.frame1->property("imageState").toByteArray() != "capturing")
		{
			ui.frame1->setProperty("imageState", "capturing");
			ui.orginaImg->setProperty("imageState", "capturing");
			ui.frame1->style()->unpolish(ui.frame1);
			ui.frame1->style()->polish(ui.frame1);
			ui.orginaImg->style()->unpolish(ui.orginaImg);
			ui.orginaImg->style()->polish(ui.orginaImg);
			ui.frame1->update();
			ui.orginaImg->update();
		}
	}
	else if (source == "cam0" || source == "cam1" || source == "cam2")
	{
		imgForDisplay = imgForDisplay.scaled(ui.camImg->size(), Qt::KeepAspectRatio);
		ui.camImg->setPixmap(QPixmap::fromImage(imgForDisplay));
	};

};
void AxisMeasurement::showPartNumber(QString partNumber)
{
	ui.partNub->setText(partNumber);
};
void AxisMeasurement::showLsResult(int lsPosition, float result)
{
	ui.lsPosition->setText(QString("测量点位 %1").arg(lsPosition));
	ui.lsDiameter->setNum(result);
};
void AxisMeasurement::showProgramProcess(QString processInf, int precentage)
{
	ui.programProcess->setText(processInf);

	const QString normalizedProcess = processInf.trimmed();
	const bool isError = normalizedProcess.contains(QStringLiteral("异常"))
		|| normalizedProcess.contains(QStringLiteral("失败"))
		|| normalizedProcess.contains(QStringLiteral("错误"));
	const bool isComplete = precentage >= 100
		|| normalizedProcess.contains(QStringLiteral("完成"))
		|| normalizedProcess.contains(QStringLiteral("结束"));
	const bool isRunning = precentage > 0
		|| normalizedProcess.contains(QStringLiteral("正在"))
		|| normalizedProcess.contains(QStringLiteral("测量中"));
	const char* processState = isError ? "error"
		: isComplete ? "complete"
		: isRunning ? "running"
		: "idle";
	if (ui.programProcess->property("processState").toByteArray() != processState)
	{
		ui.programProcess->setProperty("processState", processState);
		ui.programProgressBar->setProperty("progressState", processState);
		QWidget* stateWidgets[] = { ui.programProcess, ui.programProgressBar };
		for (QWidget* widget : stateWidgets)
		{
			widget->style()->unpolish(widget);
			widget->style()->polish(widget);
			widget->update();
		}
	}

	setProgramProgressSmooth(precentage);//P2-12：进度条平滑动画替代直接 setValue
};
/*
函数作用：检测程序确认槽函数
函数说明：infTip是弹出提示对话框的显示内容，mode是主界面按钮的响应模式mode=1是对自动检测程序程序确认阶段进行响应，mode=2直径下行测量的响应;mode=3是对应安装机芯夹提示的响应;mode=4是对应孔径相机的响应
*/
void AxisMeasurement::programCheck(QString infTip,int mode)//图像质量确认槽函数
{
	ui.programConfirm->setEnabled(true);
	
	showTips(infTip);
	switch (mode)
	{
	case 1://自动检测程序程序确认阶段进行响应
		ui.measureCancel->setEnabled(true);
		ui.autoMoveAdjust->setEnabled(true);
		ui.partRotate_anticlockwise->setEnabled(true);
		ui.partRotate_clockwise->setEnabled(true);
		ui.apexMoveUp->setEnabled(false);
		ui.apexMoveDown->setEnabled(false);
		ui.lsMoveUp->setEnabled(true);
		ui.lsMoveDown->setEnabled(true);
		showClampingPicture(currentProgram);
		break;
	case 2://直径下行测量的响应
		ui.autoMoveAdjust->setEnabled(false);
		break;
	case 3://安装机芯夹提示的响应
		ui.autoMoveAdjust->setEnabled(true);
		ui.partRotate_anticlockwise->setEnabled(true);
		ui.partRotate_clockwise->setEnabled(true);
		ui.apexMoveUp->setEnabled(true);
		ui.apexMoveDown->setEnabled(true);
		ui.lsMoveUp->setEnabled(false);
		ui.lsMoveUp->setEnabled(false);
		showFixturePicture(currentProgram);
		break;
	case 4://对应孔径相机的响应
		ui.autoMoveAdjust->setEnabled(true);
		ui.partRotate_anticlockwise->setEnabled(true);
		ui.partRotate_clockwise->setEnabled(true);
		ui.apexMoveUp->setEnabled(true);
		ui.apexMoveDown->setEnabled(true);
		ui.lsMoveUp->setEnabled(false);
		ui.lsMoveUp->setEnabled(false);
		break;
	default:
		break;
	};
};
void AxisMeasurement::programFinish(bool normalFlag)
{
	programRunFlag = false;
	ui.orginaImg->clear();
	ui.orginaImg->setText(normalFlag ? QStringLiteral("数据采集完成") : QStringLiteral("数据采集异常结束"));
	const char* imageState = normalFlag ? "complete" : "error";
	ui.frame1->setProperty("imageState", imageState);
	ui.orginaImg->setProperty("imageState", imageState);
	ui.frame1->style()->unpolish(ui.frame1);
	ui.frame1->style()->polish(ui.frame1);
	ui.orginaImg->style()->unpolish(ui.orginaImg);
	ui.orginaImg->style()->polish(ui.orginaImg);
	ui.frame1->update();
	ui.orginaImg->update();
	ui.startAutoMearsurement->setEnabled(true);
	ui.allAxisGoHome->setEnabled(true);
	ui.programNumber->setEnabled(true);
	ui.autoMoveAdjust->setEnabled(true);
	ui.apexMoveUp->setEnabled(true);
	ui.apexMoveDown->setEnabled(true);
	ui.partRotate_anticlockwise->setEnabled(true);
	ui.partRotate_clockwise->setEnabled(true);
	ui.lsMoveDown->setEnabled(true);
	ui.lsMoveUp->setEnabled(true);
	ui.ManualControl->setEnabled(true);
	m_measurePartsNum_all++;
	if (normalFlag)
	{
		m_okPartsNum_all++;
		
	}
	else
	{
		m_ngPartsNum_all++;
	}
	show_Statistics();
	//showPartNumber("未选择程序");
	showLsResult(999, 0);
	showProgramProcess("未进行测量！", 0);
	showClampingPicture(currentProgram);
};

//函数功能：在主界面上显示拨叉安装示意图
//参数说明：programNum是对应的程序号
void AxisMeasurement::showFixturePicture(int programNum)
{
	QString imagePath;
	switch (programNum) {
	case 0:
		imagePath = "./programParmeter/program0/showFixturePicture.bmp";
		break;
	case 1:
		imagePath = "./programParmeter/program0/showFixturePicture.bmp";
		break;
	case 2:
		imagePath = "./programParmeter/program0/showFixturePicture.bmp";
		break;
	case 3:
		imagePath = "./programParmeter/WZ10-15-1107-141/showFixturePicture.bmp";
		break;
	case 4:
		imagePath = "./programParmeter/program0/showFixturePicture.bmp";
		break;
	case 5:
		imagePath = "./programParmeter/WZ10-45-1080-220/showFixturePicture.bmp";
		break;
	case 6:
		imagePath = "./programParmeter/WZ10-44-1112-155/showFixturePicture.bmp";
		break;
	case 7:
		imagePath = "./programParmeter/WZ20-15-001-80/showFixturePicture.bmp";
		break;
	case 8:
		imagePath = "./programParmeter/program0/showFixturePicture.bmp";
		break;
	case 9:
		imagePath = "./programParmeter/program0/showFixturePicture.bmp";
		break;
	case 10:
		imagePath = "./programParmeter/program0/showFixturePicture.bmp";
		break;
	default://可能需要
		break;
	}
	/*
	try {
		QImage imgForDisplay(imagePath);
		imgForDisplay = imgForDisplay.scaled(ui.orginaImg->size(), Qt::KeepAspectRatio);//此函数还有一个参数是图像的转换格式（缺省）//此函数还有一个参数是图像的转换格式（缺省）
		ui.orginaImg->setPixmap(QPixmap::fromImage(imgForDisplay));
	}
	catch (...)
	{
		showTips(QString("未找到该该零件的机芯夹安装图片！请检查“%1”是否存在！").arg(imagePath));
	}
	*/
};
//函数功能：在主界面上显示工件安装示意图
//参数说明：programNum是对应的程序号
void  AxisMeasurement::showClampingPicture(int programNum)
{

	QString imagePath ;
	
	switch (programNum) {
	case 0:
		imagePath = "./programParmeter/program0/clamplingPicture.bmp";
		break;
	case 1:
		imagePath = "./programParmeter/program0/clamplingPicture.bmp";
		break;
	case 2:
		imagePath = "./programParmeter/program0/clamplingPicture.bmp";
		break;
	case 3:
		imagePath = "./programParmeter/WZ10-15-1107-141/clamplingPicture.bmp";
		break;
	case 4:
		imagePath = "./programParmeter/program0/clamplingPicture.bmp";
		break;
	case 5:
		imagePath = "./programParmeter/WZ10-45-1080-220/clamplingPicture.bmp";
		break;
	case 6:
		imagePath = "./programParmeter/WZ10-44-1112-155/clamplingPicture.bmp";
		break;
	case 7:
		imagePath = "./programParmeter/WZ20-15-001-80/clamplingPicture.bmp";
		break;
	case 8:
		imagePath = "./programParmeter/program0/clamplingPicture.bmp";
		break;
	case 9:
		imagePath = "./programParmeter/program0/clamplingPicture.bmp";
		break;
	case 10:
		imagePath = "./programParmeter/program0/clamplingPicture.bmp";
		break;
	default:
		break;
	}
	
	/*
	try {
		QImage imgForDisplay(imagePath);
		imgForDisplay = imgForDisplay.scaled(ui.orginaImg->size(), Qt::KeepAspectRatio);//此函数还有一个参数是图像的转换格式（缺省）//此函数还有一个参数是图像的转换格式（缺省）
		ui.orginaImg->setPixmap(QPixmap::fromImage(imgForDisplay));
	}
	catch (...)
	{
		showTips(QString("未找到该该零件的装夹图片片！请检查“%1”是否存在！").arg(imagePath));
	}
	*/
	/*
	QImage imgForDisplay(imagePath);
	int width = imgForDisplay.width();
	int height = imgForDisplay.height();
	int centerX = width / 2;
	int centerY = height / 2;
	QPainter painter(&imgForDisplay);
	painter.setRenderHint(QPainter::Antialiasing, true);//设置反锯齿
	QPen pen;
	pen.setWidth(5);
	pen.setColor(QColor(0, 255, 0));
	pen.setStyle(Qt::DashDotLine);
	painter.setPen(pen);
	painter.drawLine(centerX - 150, 0, centerX - 150, height);//起点和终点  
	painter.drawLine(centerX + 150, 0, centerX + 150, height);
	case 1://最长轴花键图像
		painter.drawLine(centerX - 47, 0, centerX - 47, height);//起点和终点  
		painter.drawLine(centerX + 47, 0, centerX + 47, height);
		painter.end();
		break;
	//最长轴孔图像
	*/

	

	

};
void AxisMeasurement::goHomeThreadFinish(int goHomeAxis, bool normalFlag)
{
	//cout << "goHomeThreadFinis" << endl;
	if ((goHomeAxis) == 9)
	{
		ui.allAxisGoHome->setEnabled(true);
		ui.autoMoveAdjust->setEnabled(true);
		ui.ManualControl->setEnabled(true);
		if (normalFlag)
		{
			ui.startAutoMearsurement->setEnabled(true);
			ui.apexMoveUp->setEnabled(true);
			ui.apexMoveDown->setEnabled(true);
			ui.partRotate_anticlockwise->setEnabled(true);
			ui.partRotate_clockwise->setEnabled(true);
			ui.lsMoveUp->setEnabled(true);
			ui.lsMoveDown->setEnabled(true);
		};
	};
	

};

//其他测试槽函数***********************************************


void AxisMeasurement::on_roundoutDataCollection_clicked()//跳动数据存储读取
{
	vector <float> radius1;
	vector <float> center1;
	vector <float> up1;
	vector <float> bottom1;
	vector <float> radius2;
	vector <float> center2;
	vector <float> up2;
	vector <float> bottom2;
	vector <float> radius3;
	vector <float> center3;
	vector <float> up3;
	vector <float> bottom3;
	long axis7_steplength = 360 * 500;//一个脉冲是0.002度;
	moveControlCardPtr->setCurrentAxis(7);

	moveControlCardPtr->zeroPosition();//第一次测量数据采集
	moveControlCardPtr->setMoveMode("Trap");
	moveControlCardPtr->setTrapPrm(moveControlCardPtr->axisCore[moveControlCardPtr->currentAxisIndex], moveControlCardPtr->currentAxisNumber, moveControlCardPtr->trapAuto[moveControlCardPtr->currentAxisIndex], axis7_steplength, moveControlCardPtr->trapHighVelAuto[moveControlCardPtr->currentAxisIndex]);

	float a = lsSensorPtr->getLsMeasurementValue(5);
	radius1.push_back(lsSensorPtr->lsMeasureResult[0] / 2);
	up1.push_back(lsSensorPtr->lsMeasureResult[1]);
	bottom1.push_back(lsSensorPtr->lsMeasureResult[2]);
	center1.push_back(lsSensorPtr->lsMeasureResult[3]);//暂定
	moveControlCardPtr->startTrap();
	do
	{
		m_program0_Ptr->msleep(5);
		lsSensorPtr->getLsMeasurementValue(5);
		radius1.push_back(lsSensorPtr->lsMeasureResult[0] / 2);
		up1.push_back(lsSensorPtr->lsMeasureResult[1]);
		bottom1.push_back(lsSensorPtr->lsMeasureResult[2]);
		center1.push_back(lsSensorPtr->lsMeasureResult[3]);//暂定
		moveControlCardPtr->updateAxisStatus(moveControlCardPtr->currentAxisNumber);
	} while (moveControlCardPtr->bFlagMotion[moveControlCardPtr->currentAxisIndex]);
	m_program0_Ptr->msleep(1000);
	moveControlCardPtr->zeroPosition();//第2次测量数据采集
	moveControlCardPtr->setMoveMode("Trap");
	moveControlCardPtr->setTrapPrm(moveControlCardPtr->axisCore[moveControlCardPtr->currentAxisIndex], moveControlCardPtr->currentAxisNumber, moveControlCardPtr->trapAuto[moveControlCardPtr->currentAxisIndex], axis7_steplength, moveControlCardPtr->trapHighVelAuto[moveControlCardPtr->currentAxisIndex]);

	a = lsSensorPtr->getLsMeasurementValue(5);
	radius2.push_back(lsSensorPtr->lsMeasureResult[0] / 2);
	up2.push_back(lsSensorPtr->lsMeasureResult[1]);
	bottom2.push_back(lsSensorPtr->lsMeasureResult[2]);
	center2.push_back(lsSensorPtr->lsMeasureResult[3]);//暂定
	moveControlCardPtr->startTrap();
	do
	{
		m_program0_Ptr->msleep(5);
		lsSensorPtr->getLsMeasurementValue(5);
		radius2.push_back(lsSensorPtr->lsMeasureResult[0] / 2);
		up2.push_back(lsSensorPtr->lsMeasureResult[1]);
		bottom2.push_back(lsSensorPtr->lsMeasureResult[2]);
		center2.push_back(lsSensorPtr->lsMeasureResult[3]);//暂定
		moveControlCardPtr->updateAxisStatus(moveControlCardPtr->currentAxisNumber);
	} while (moveControlCardPtr->bFlagMotion[moveControlCardPtr->currentAxisIndex]);
	m_program0_Ptr->msleep(1000);
	moveControlCardPtr->zeroPosition();//第3次测量数据采集
	moveControlCardPtr->setMoveMode("Trap");
	moveControlCardPtr->setTrapPrm(moveControlCardPtr->axisCore[moveControlCardPtr->currentAxisIndex], moveControlCardPtr->currentAxisNumber, moveControlCardPtr->trapAuto[moveControlCardPtr->currentAxisIndex], axis7_steplength, moveControlCardPtr->trapHighVelAuto[moveControlCardPtr->currentAxisIndex]);

	a = lsSensorPtr->getLsMeasurementValue(5);
	radius3.push_back(lsSensorPtr->lsMeasureResult[0] / 2);
	up3.push_back(lsSensorPtr->lsMeasureResult[1]);
	bottom3.push_back(lsSensorPtr->lsMeasureResult[2]);
	center3.push_back(lsSensorPtr->lsMeasureResult[3]);//暂定
	moveControlCardPtr->startTrap();
	do
	{
		m_program0_Ptr->msleep(5);
		lsSensorPtr->getLsMeasurementValue(5);
		radius3.push_back(lsSensorPtr->lsMeasureResult[0] / 2);
		up3.push_back(lsSensorPtr->lsMeasureResult[1]);
		bottom3.push_back(lsSensorPtr->lsMeasureResult[2]);
		center3.push_back(lsSensorPtr->lsMeasureResult[3]);//暂定
		moveControlCardPtr->updateAxisStatus(moveControlCardPtr->currentAxisNumber);
	} while (moveControlCardPtr->bFlagMotion[moveControlCardPtr->currentAxisIndex]);


	//保存到excel文件
	string excelPath = "C:\\Users\\PC\\Desktop\\roundoutData.xlsx";//这个也需要修改
	QAxObject* excel = new QAxObject;
	if (excel->setControl("Excel.Application"))
	{
		excel->dynamicCall("SetVisible (bool Visible)", false);
		excel->setProperty("DisplayAlerts", false);
		QAxObject* workbooks = excel->querySubObject("WorkBooks");            //获取工作簿集合
		workbooks->dynamicCall("Add");                                        //新建一个工作簿
		QAxObject* workbook = excel->querySubObject("ActiveWorkBook");        //获取当前工作簿
		QAxObject* worksheet = workbook->querySubObject("Worksheets(int)", 1);
		QAxObject* cell;
		int rowCount_time1 = radius1.size();
		int rowCount_time2 = radius2.size();
		int rowCount_time3 = radius3.size();
		int columnCount = 12;
		//添加Excel表头数据
		for (int i = 1; i <= columnCount; i++)
		{
			cell = worksheet->querySubObject("Cells(int,int)", 1, i);
			cell->setProperty("RowHeight", 40);
			cell->dynamicCall("SetValue(const QString&)", "第列");
			cell->querySubObject("Font")->setProperty("Bold", true);
			cell->querySubObject("Interior")->setProperty("Color", QColor(220, 220, 220));

		};
		//将数据写入excel
		for (int j = 2; j <= rowCount_time1 + 1; j++)//写入第1次测量数据
		{

			cell = worksheet->querySubObject("Cells(int,int)", j, 1);
			cell->dynamicCall("SetValue(const float)", radius1[j - 2]);
			cell = worksheet->querySubObject("Cells(int,int)", j, 2);
			cell->dynamicCall("SetValue(const float)", center1[j - 2]);
			cell = worksheet->querySubObject("Cells(int,int)", j, 3);
			cell->dynamicCall("SetValue(const float)", up1[j - 2]);
			cell = worksheet->querySubObject("Cells(int,int)", j, 4);
			cell->dynamicCall("SetValue(const float)", bottom1[j - 2]);
		};
		for (int j = 2; j <= rowCount_time2 + 1; j++)//写入第2次测量数据
		{
			cell = worksheet->querySubObject("Cells(int,int)", j, 5);
			cell->dynamicCall("SetValue(const float)", radius2[j - 2]);
			cell = worksheet->querySubObject("Cells(int,int)", j, 6);
			cell->dynamicCall("SetValue(const float)", center2[j - 2]);
			cell = worksheet->querySubObject("Cells(int,int)", j, 7);
			cell->dynamicCall("SetValue(const float)", up2[j - 2]);
			cell = worksheet->querySubObject("Cells(int,int)", j, 8);
			cell->dynamicCall("SetValue(const float)", bottom2[j - 2]);
		};
		for (int j = 2; j <= rowCount_time3 + 1; j++)//写入第3次测量数据
		{

			cell = worksheet->querySubObject("Cells(int,int)", j, 9);
			cell->dynamicCall("SetValue(const float)", radius3[j - 2]);
			cell = worksheet->querySubObject("Cells(int,int)", j, 10);
			cell->dynamicCall("SetValue(const float)", center3[j - 2]);
			cell = worksheet->querySubObject("Cells(int,int)", j, 11);
			cell->dynamicCall("SetValue(const float)", up3[j - 2]);
			cell = worksheet->querySubObject("Cells(int,int)", j, 12);
			cell->dynamicCall("SetValue(const float)", bottom3[j - 2]);
		};

		QString fileName = QString(QString::fromLocal8Bit(excelPath.c_str()));
		workbook->dynamicCall("SaveAs(const QString&)", QDir::toNativeSeparators(fileName)); //保存至fileName
		workbook->dynamicCall("Close()");                                                   //关闭工作簿
		excel->dynamicCall("Quit()");                                                       //关闭excel
		delete excel;
		excel = NULL;
	};
};
void AxisMeasurement::on_pushButton_clicked()
{
	/*
	cout << "ssssssssssssssss" << endl;
	m_program7_Ptr->resultVectorList.push_back(1.1);
	m_program7_Ptr->pushback_vectors(m_program7_Ptr->resultVectorList, 8, 1,1.1, -0.5, 0.4);
	m_program7_Ptr->resultVectorList.push_back(1);
	m_program7_Ptr->pushback_vectors(m_program7_Ptr->resultVectorList, 8, 2,2 ,-0.5, 300);
	m_program7_Ptr->resultVectorList.push_back(301);
	m_program7_Ptr->pushback_vectors(m_program7_Ptr->resultVectorList, 8,3, 1, -0.4, 299);
	m_program7_Ptr->resultVectorList.push_back(301);
	m_program7_Ptr->pushback_vectors(m_program7_Ptr->resultVectorList, 8, 3, 1, -0.4,25);
	ui.measureTable->sortItems(0, Qt::AscendingOrder);
	*/
	//showClampingPicture(0);
	/*
	string holePath[10];
	holePath[0] = "D:/temporary_sun/AxisMeasurement/measureData/WZ10-44-1112-155/20230904_164452/cam1_1_1.bmp";
	holePath[1] = "D:/temporary_sun/AxisMeasurement/measureData/WZ10-44-1112-155/20230904_164452/cam1_1_2.bmp";
	holePath[2] = "D:/temporary_sun/AxisMeasurement/measureData/WZ10-44-1112-155/20230907_095934/cam1_1_3.bmp";
	holePath[3] = "D:/temporary_sun/AxisMeasurement/measureData/WZ10-44-1112-155/20230907_095934/cam1_1_4.bmp";
	holePath[4] = "D:/temporary_sun/AxisMeasurement/measureData/WZ10-44-1112-155/20230907_095934/cam1_1_5.bmp";
	holePath[5] = "D:/temporary_sun/AxisMeasurement/measureData/WZ10-44-1112-155/20230907_095934/cam1_1_6.bmp";
	holePath[6] = "D:/temporary_sun/AxisMeasurement/measureData/WZ10-44-1112-155/20230907_095934/cam1_1_7.bmp";
	holePath[7] = "D:/temporary_sun/AxisMeasurement/measureData/WZ10-44-1112-155/20230907_095934/cam1_1_8.bmp";
	holePath[8] = "D:/temporary_sun/AxisMeasurement/measureData/WZ10-44-1112-155/20230907_095934/cam1_1_9.bmp";
	holePath[9] = "D:/temporary_sun/AxisMeasurement/measureData/WZ10-44-1112-155/20230907_095934/cam1_1_10.bmp";
	int k = 0;
	m_program6_Ptr->resultVectorList.clear();
	m_program6_Ptr->resultVectorList.shrink_to_fit();
	try {
		for (int i = 0; i < 10; i++)
		{
			k++;
			cout << "第直径:" << i << endl;
			double diameter = m_program6_Ptr->cam1Picture_holeAlgorithm(holePath[i]);
			cout << "直径"<< i<<":" << diameter << endl;
			m_program6_Ptr->resultVectorList.push_back(diameter);
			
		};
		//m_program6_Ptr->pushback_vectors(m_program6_Ptr->resultVectorList, 8, 70, 5.4, 0, +0.15);
	}
	catch (...) {
		
		cout << "执行出错:" <<k<< endl;
	};
	m_program6_Ptr->resultVectorList.clear();
	m_program6_Ptr->resultVectorList.shrink_to_fit();
	/*
	vector <vector <float>> radius;
	vector <vector <float>> center;
	vector <float> r1 = { 1,1 };
	vector <float> c1 = { 1.1, 1.1 };
	radius.push_back(r1);
	center.push_back(c1);
	r1.push_back(1);
	c1.push_back(1.1);
	radius.push_back(r1);
	center.push_back(c1);
	r1.push_back(1);
	c1.push_back(1.1);
	radius.push_back(r1);
	center.push_back(c1);

	//保存到excel文件
	
	string excelPath = "C:\\Users\\Administrator\\Desktop\\YYroundoutData.xlsx";
	QAxObject* excel = new QAxObject;
	if (excel->setControl("Excel.Application"))
	{
		excel->dynamicCall("SetVisible (bool Visible)", false);
		excel->setProperty("DisplayAlerts", false);
		QAxObject* workbooks = excel->querySubObject("WorkBooks");            //获取工作簿集合
		workbooks->dynamicCall("Add");                                        //新建一个工作簿
		QAxObject* workbook = excel->querySubObject("ActiveWorkBook");        //获取当前工作簿
		QAxObject* worksheet = workbook->querySubObject("Worksheets(int)", 1);
		QAxObject* cell;
		int rowCount_time1 = radius[0].size();
		int rowCount_time2 = radius[1].size();
		int rowCount_time3 = radius[2].size();
		int columnCount = 6;
		//添加Excel表头数据
		for (int i = 1; i <= columnCount; i++)
		{
			cell = worksheet->querySubObject("Cells(int,int)", 1, i);
			cell->setProperty("RowHeight", 40);
			cell->dynamicCall("SetValue(const QString&)", "第列");
			cell->querySubObject("Font")->setProperty("Bold", true);
			cell->querySubObject("Interior")->setProperty("Color", QColor(220, 220, 220));

		};
		//将数据写入excel
		for (int j = 2; j <= rowCount_time1 + 1; j++)//写入第一次测量数据
		{

			cell = worksheet->querySubObject("Cells(int,int)", j, 1);
			cell->dynamicCall("SetValue(const float)", radius[0][j - 2]);
			cell = worksheet->querySubObject("Cells(int,int)", j, 2);
			cell->dynamicCall("SetValue(const QString&)", center[0][j - 2]);
		};
		for (int j = 2; j <= rowCount_time2 + 1; j++)//写入第2次测量数据
		{

			cell = worksheet->querySubObject("Cells(int,int)", j, 3);
			cell->dynamicCall("SetValue(const float)", radius[1][j - 2]);
			cell = worksheet->querySubObject("Cells(int,int)", j, 4);
			cell->dynamicCall("SetValue(const QString&)", center[1][j - 2]);
		};
		for (int j = 2; j <= rowCount_time3 + 1; j++)//写入第3次测量数据
		{

			cell = worksheet->querySubObject("Cells(int,int)", j, 5);
			cell->dynamicCall("SetValue(const float)", radius[2][j - 2]);
			cell = worksheet->querySubObject("Cells(int,int)", j, 6);
			cell->dynamicCall("SetValue(const QString&)", center[2][j - 2]);
		};

		QString fileName = QString(QString::fromLocal8Bit(excelPath.c_str()));
		workbook->dynamicCall("SaveAs(const QString&)", QDir::toNativeSeparators(fileName)); //保存至fileName
		workbook->dynamicCall("Close()");                                                   //关闭工作簿
		excel->dynamicCall("Quit()");                                                       //关闭excel
		delete excel;
		excel = NULL;
	};
	*/
	
	

	/*
	//图像测试代码
	string path[8];
	path[0] = "C:/Users/Administrator/Desktop/CHANG/4.bmp";
	path[1] = "C:/Users/Administrator/Desktop/CHANG/5.bmp";
	path[2] = "C:/Users/Administrator/Desktop/CHANG/6.bmp";
	path[3] = "C:/Users/Administrator/Desktop/CHANG/7.bmp";
	path[4] = "C:/Users/Administrator/Desktop/CHANG/8.bmp";
	path[5] = "C:/Users/Administrator/Desktop/CHANG/9.bmp";
	path[6] = "C:/Users/Administrator/Desktop/CHANG/10.bmp";
	path[7] = "C:/Users/Administrator/Desktop/CHANG/11.bmp";
	int i = 4;
	try {
		m_program5_Ptr->cam0Picture4_algorithm(path[0]);
		i++;
		m_program5_Ptr->cam0Picture5_algorithm(path[1]);
		i++;
		m_program5_Ptr->cam0Picture6_algorithm(path[2]);
		i++;
		m_program5_Ptr->cam0Picture7_algorithm(path[3]);
		i++;
		m_program5_Ptr->cam0Picture8_algorithm(path[4]);
		i++;
		m_program5_Ptr->cam0Picture9_algorithm(path[5]);
		i++;
		m_program5_Ptr->cam0Picture10_algorithm(path[6]);
		i++;
		m_program5_Ptr->cam0Picture11_algorithm(path[7]);
		i++;
	}
	catch (...) {
		cout << "图像序号" << i << "执行错误" << endl;
	}
	
	cout << "执行完了" << endl;
	*/

    /*
    string path[2];
	m_program6_Ptr->imgSavePath[0] = "D:/temporary_sun/AxisMeasurement/measureData/1112-155/20230904_103813/cam0_1.bmp";
	cout << m_program6_Ptr->imgSavePath[0] << endl;
	m_program6_Ptr->imgSavePath[0] = "D:/temporary_sun/AxisMeasurement/measureData/1112-155/20230904_103813/cam0_2.bmp";
	try {
		m_program6_Ptr->cam0Picture_algorithm();
	}
	catch (...)
	{
	};
	*/
};
