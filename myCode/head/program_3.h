//WZ10-15-1107-141***************************************************************************


#pragma once

#include <QThread>
#include <QObject>
#include <QTableWidget>
#include <ActiveQt/QAxObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QtSql>
#include <QDataWidgetMapper>
#include <QFileDialog>
#include <iostream>
#include <opencv2/opencv.hpp>

#include <HalconCpp.h>
#include <Halcon.h>
#include "HDevThread.h"

#include <ctime>
#include <string>
#include <sstream>
#include <direct.h>
#include "Windows.h"

#include "cam_device.h"
#include "moveControl.h"
#include "ls_device.h"
#include "roughnessFun.h"
#include "sharedFun.h"

#include "rtwtypes.h"//matlab相关头文件
#include <cstddef>
#include <cstdlib>
#include "funFitLine.h"
#include "funFitLine_terminate.h"
#include "rt_nonfinite.h"
#include "coder_array.h"
#include "funFitCos.h"
#include "funFitCos_terminate.h"

using namespace std;
using namespace HalconCpp;


class program_3 :public QThread
{
	Q_OBJECT;
public:

	//测试程序初始化参数**************************************************************************************************************************************************************


	program_3(cam_device* cam1, cam_device* cam2, cam_device* cam3, moveControl* cradDevicePtr, ls_device* lsDevicePtr, QTableWidget* tablePtr, QSqlDatabase* DbPtr, roughnessFun* roughnessObjPtr);
	~program_3();

	QString partName = "转子连接轴";
	QString partNub="WZ10-15-1107-141";
	QString operatorName;
	QString partsID;
	QString allFeatureFlag;
	vector <double> resultVectorList;//存储以上所有特征信息，第一位是特征序号fIndex(按照该序号排序，示例代码如下)
	QAxObject* workbook;// 当前工作簿指针
	QAxObject* worksheet;//活动工作表指针
	map <int, QString> featureMap={
		{1,"直径"},{2,"倒角尺寸"},{3,"倒角角度"},{4,"轴向尺寸"},{5,"跳动"},{6,"圆柱度"},{7,"花键尺寸"},{8,"孔径"},{9,"粗糙度"},{10,"圆度"},{11,"螺纹中径"}
	};
	map <int, QString> featureQualityMap = {
	{0,"OK"},{1,"NG"} };
	
	int ngFeatureNum;//NG特征数量
	int measurePartsNum_all;//该型号轴测量总数
	int measurePartsNum_ok;//该型号轴合格数量
	int measurePartsNum_ng;//该型号轴NG数量
	float yield;//该型号轴合格率
	QString programConfirm;//用于对测量最终结果进行确认
	string dataSavePath;//数据存储文件夹路径
	string stime;//时间字符串
	cam_device* camPtrList[3];//3个相机指针
	moveControl* moveControlPtr;//运动控制卡指针
	ls_device* ls_devicePtr;//光幕传感器指针
	QTableWidget* measureTablePtr;//主页面tabelWiget指针
	QSqlDatabase* databasePtr;//主界面的数据库指针
	cv::Mat* originalImgPtr = NULL;
	cv::Mat* processedImgPtr = NULL;
	roughnessFun* m_roughnessObjPtr;//粗糙度计算相关指针
	vector <double> dataScreenOrg;


	//光幕传感器直径&圆柱度测量初始化参数******************************************************************************************************************************************************************


	//光幕传感器测量直径对应的移动位置
	long ls_StepLength_axis5[25] = {
		628500, 638000, 652300, 658850, 705000, 757000, 780000, 791200, 799750, 803600,804550,805500,
		807900,808100, 808300,810700,811600,812500, 863000, 877500,883700, 883900, 884100, 894500, 907000};
	//光幕传感器直径测量结果存储
	double lsDiameterResult[25][10] = { //0号是一次测量值，1号为2次测量值，2号是两次均值，3号是工序卡序号,4为标准值，5为上限，6为下限，7第一次旋转距离（补偿），8第二次旋转距离（补偿），9旋转距离均值
		{ 0, 0, 0, 66, 39.11, 0, -0.24, 0, 0, 0}, { 0, 0, 0, 68, 41.275, 0, -0.05, 0, 0, 0}, { 0, 0, 0, 65, 39.11, 0, -0.24, 0, 0, 0}, { 0, 0, 0, 60, 42.05, 0, -0.05, 0, 0, 0},
	    { 0, 0, 0, 58, 40, 0.12, -0.12, 0, 0, 0},{ 0, 0, 0, 56, 44.069, 0, -0.127, 0, 0, 0}, { 0, 0, 0, 58, 40, 0.12, -0.12, 0, 0, 0}, { 0, 0, 0, 95, 45.16, 0, -0.1, 0, 0, 0}, 
		{ 0, 0, 0, 55, 43.58, 0.12, -0.12, 0, 0, 0}, { 0, 0, 0, 96, 50.1, 0, -0.13, 0, 0, 0}, { 0, 0, 0, 96, 50.1, 0, -0.13, 0, 0, 0}, { 0, 0, 0, 96, 50.1, 0, -0.13, 0, 0, 0}, { 0, 0, 0, 49, 49, 0.2, -0.2, 0, 0, 0},{ 0, 0, 0, 49, 49, 0.2, -0.2, 0, 0, 0}, { 0, 0, 0, 49, 49, 0.2, -0.2, 0, 0, 0},  { 0, 0, 0, 4, 55.1, 0, -0.12, 0, 0, 0},
		 { 0, 0, 0, 4, 55.1, 0, -0.12, 0, 0, 0}, { 0, 0, 0, 4, 55.1, 0, -0.12, 0, 0, 0},{ 0, 0, 0, 39, 61.2, 0.1, -0.1, 0, 0, 0}, { 0, 0, 0, 40, 61.95, 0, -0.05, 0, 0, 0},{ 0, 0, 0, 42, 61.422, 0.05, -0.05, 0, 0, 0},{ 0, 0, 0, 42, 61.422, 0.05, -0.05, 0, 0, 0}, { 0, 0, 0, 42, 61.422, 0.05, -0.05, 0, 0, 0}, { 0, 0, 0, 25, 66.6, 0, -0.14, 0, 0, 0},
		{ 0, 0, 0, 27, 91.83, 0, -0.03, 0, 0}};
	
	
	//光幕传感器圆柱度测量参数设定
	long lsCylindricitySteplength_axis5[3][3] = {//0号是第一个截面5轴对应的位置，1号是2截面，2号是3截面
		{714000,717000,723000},  {844000,850000,856000},{2347400,2348900,2350400} };
	//光幕传感器圆柱度测量结果
	double lsCylindricityResult[3][3] = {//0号是工序卡序号，1号是测量值，2号是上限
		{19, 0, 0.004}, {25, 0, 0.004}, {162, 0, 0.004}};
    
	

	// 光幕传感器跳动初始化参数 * *******************************************************************************************************************************************************
	//141无法对基准G进行跳动测量
    
	long lsRoundoutSteplength_axis5[4][3] = {//前4个点是跳动待测特征位置数据；索引0也是基准N
		{656850,658850,660350},{750000,757000,764000},{790200,791200,792200},{903000,907000,911000}};
	float lsRoundoutResult[4][3] = {//0号工序卡序号，1号是测量值，2号是上限
		{160.013, 0, 0.005}, {141.057, 0, 0.02}, {160.010, 0, 0.005}, {160.008, 0, 0.003}};
	struct roundoutData {
		double axisPoint[3][3];//跳动测量截面轴心点的x,y,z坐标;每一行数据代表对应的一个点位
		double cosFit[3][3];//测量截面的拟合结果0号元素是偏心距e;1号元素是相位；2号元素是轴心偏移距离;每一行数据代表一次对应的拟合结果
		vector<float> radius0;//用于存储跳动测量的半径;
		vector<float> radius1;
		vector<float> radius2;
		vector<float> center0;//用于存储跳动测量中的直径数据;
		vector<float> center1;
		vector<float> center2;
		vector<float> distance0;//用于存储截面上各点到旋转中心的距离
		vector<float> distance1;
		vector<float> distance2;
		double roundoutResult[3];//分别储存三个跳动结果
	};
	//光幕传感器跳动测量数据
	roundoutData*  roundoutDataPtr[4];//前4个点是跳动待测特征位置数据；索引0为是基准N
	double directionVector_N[3];
	double aixsReferancePoint_N[3];
	


	//远心相机测量参数初始化****************************************************************************************************************************************************************************


	int fault_detect = 0;//用于记录远心图像算法的异常情况
	string imgSavePath[3];//远心相机采集图像位置
	long cam0_StepLength_axis5[3] = {402600,519300,586600};//远心相机测量时轴5对应的点位（小顶尖）
	double cam0_arriveOrgEncode_axis5[3] = { 0, 0, 0};//远心相机到位对应的光栅尺原始读数（用于补偿粗糙度测量移动位置）
	int cam0_exposeTime=14;//远心相机的曝光

	//远心相机使用的全局参数*
	double Calik = 0.01218603;//相机标定系数
	//double PI = 3.1415926;
	double ImageRowSize = 5120;//图像像素行数
	double ImageColSize = 5120;//图像像素列数
	double ImageRowSize_Cali;//图像换算后行距离
	double ImageColSize_Cali;//图像换算后列距离
	double pic1To2_moveDistance = 51.841196992557826;//相机从位置1到2位移距离
	double pic2To3_moveDistance = 51.08171442899268;//相机从位置2到3位移距离
	double pic3To4_moveDistance = 54.791406012354344;//相机从位置3到4位移距离

	
	//孔径相机测量参数初始化***********************************************************************************************************************************************************************************


	vector<string> cam1PathList;//孔径相机拍照储存位置（需要多次拍照）
	double CalikKong = 0.00691842;//测孔花键相机标定系数   从0.686改
	long cam1_prepare[2][5] = {//0号元素是5轴位置，1号元素是2轴拍照位置，2号元素是2轴退回位置,3号元素为特征的均布个数,4号是图像的绘制模式（参考主程序中的AxisMeasurement::displayImg说明）
		{-93000 - 600,-281000,-220000,12,6},{31000 - 600,-269000,-220000,4,7}
	};
	int cam1_exposeTime=550;//测圆孔相机曝光


	//粗糙度相机测量参数设定***********************************************************************************************************************************************************************************


	string roughnessPicPath[8];
	long cam2_parpare[1][3] = {//0号元素五轴移动位置；1号1轴移动到位置（参考）；2号1轴退回位置(其中前三组位置是在凸台下方)；                                 
		 {-107500, -189020-1000, 120000}};
	float cam2_radius[1][2] = {//0号元素参考回转半径值；1号元素实际回转半径值
		{21.0215, 0} };
	double roughnessResult[1][3] = {//0号粗糙度编号，1号粗糙度上限；2号测量值
		{103, 1.6, 99}};
	int cam2_exposeTime=550;//粗糙度镜头曝光
	

	//调试程序需要使用的变量*****************************************************************************************************************************************************************************************


	

	//程序需要使用的基本函数*************************************************************************************************************************************************************************************
	

	void run();
	void lsSensorMeasure_diameter(int positionNumber,int time);//使用光幕传感器进行直径测量
	//void lsSensorMeasure_cylindricity(int positionNumber, float intervalTime);//使用光幕传感器进行圆柱度测量
	string cam0_Measure_prepare(int positionNumber);//利用远心镜头进行测量,移动和采集函数
	void cam1_Measure_prepare(int positionNumber);//使用cam1进行孔径测量
	void cam2_Measure_prepare(int positionNumber);//使用cam2进行粗糙度测量
	void saveAsExcel();//将主界面上的表格保存为excel文件
	bool mergeCells(QString start, QString end, QString value);//合并单元格
	void creatDatabaseTable();//创建数据库中的表格
	void writeToDatabase();//写入到数据库中
	void zeroMeasureNub();//清空测量统计数据
	void pushback_vectors(vector<double>& fResult, int fType, double fIndex, double fNominalsize, double fLowerTolerance, double fUpperTolerance);//将一条完整的测量数据储存并在excel上显示void clearVector(vector <float>& list);//清空vector数组内容并且缩至最小


	//跳动测量需要使用的函数****************************************************************************************************************************************************************************************
	
	
	void lsSensorMeasure_roundOutPrepare(int startLocation, int endLocation, int positionNumber, int intervalTime);//使用光幕传感器进行跳动测量;
	void axisLineCalculate(double pointsData[][3], int pointsNum, double* directionVector, double* aixsReferancePoint);//计算拟合轴线
	void centerCosFit(int positionNumber, int positionIndex);//拟合对应跳动位置的余弦曲线
	void axisLoaction(int positionNumber, int positionIndex, double* directionVector, double* aixsReferancePoint);//计算各跳动位置实际回转轴心
	void roundoutCalculate(int positionNumber, int positionIndex, int calculateMode);//计算对应的跳动位置的跳动值
	void clearRoundoutData();//清除跳动测量数据


	//远心相机图像检测算法***********************************************************************************************************************************************************************************************


	void Straight_TLineFP_P(double* TLineFP_result, HObject ho_Image, int Line1, int Line2, int Line3, int Line4,
		int Metrology1, int Metrology2, int Metrology3, int Metrology4);


	void TemplateMatching_TLineFP_P(double* TLineFP_result, HObject ho_Image, HTuple hv_ModelFile, int MatchingLine1, int MatchingLine2, int MatchingLine3, int MatchingLine4,
		int Metrology1, int Metrology2, int Metrology3, int Metrology4,
		double ShapeModel1, double ShapeModel2, double ShapeModel3, double ShapeModel4, double ShapeModel5);


	double TemplateMatching_Chamfer_P(HObject ho_Image, HTuple hv_ModelFile, double ShapeModel1, double ShapeModel2, double ShapeModel3, double ShapeModel4, double ShapeModel5,
		int Rectangle1, int Rectangle2, int Rectangle3, int Rectangle4,
		int EdgesSubP1, int EdgesSubP2, int EdgesSubP3,
		bool Segment, int SegmentContour1, int SegmentContour2, int SegmentContour3,
		double SelectContour1, double SelectContour2, double SelectContour3,
		int UnionContour1, int UnionContour2,
		int FitCircleContour1, int FitCircleContour2, int FitCircleContour3, int FitCircleContour4, int FitCircleContour5);


	double TLineFP_Chamfer_P(HObject ho_Image, double GenRect1, double GenRect2, double GenRect3, double GenRect4,
		int EdgesSubP1, int EdgesSubP2, int EdgesSubP3,
		bool Segment, int SegmentContour1, int SegmentContour2, int SegmentContour3,
		double SelectContour1, double SelectContour2, double SelectContour3,
		int UnionContour1, int UnionContour2,
		int FitCircleContour1, int FitCircleContour2, int FitCircleContour3, int FitCircleContour4, int FitCircleContour5);

	void cam0Picture_algorithm();//远心相机图像算法
	HObject imgAug(string imgPath);

	//孔径相机检测函数****************************************************************************************************************************************************************************************************


	double cam1Picture_holeAlgorithm(string imgPath);


	///测试函数************************************************************************************************************************************************************************************************************


signals:
	void partsNumber(QString partsNumber);//返回当前轴的零件号
	void updateDeviceInf(QString deviceInf);//更新设备信息
	void imgInf(const Mat* image, QString source,int displayMode);//图像显示信号
	void lsResult(int position, float result);//光幕传感器结果显示信号
	void programProcess(QString processInf,int Precentage);//显示测量进程信息和进度条
	void Finished(bool normalFlag);//program结束信号
	void programTips(QString programInf,int mode);//程序到达位置提示信号
	void measureStatistics(QString result, int ngFeatureNum, int measureNum, float currentYield);
	void fixtureTips(int programNum);//机心夹安装示意信号
};

	




