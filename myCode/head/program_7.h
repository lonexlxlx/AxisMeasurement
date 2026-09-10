//WZ20-15-001-80****************************************************************************


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


class program_7 :public QThread
{
	Q_OBJECT;
public:

	//测试程序初始化参数**************************************************************************************************************************************************************


	program_7(cam_device* cam1, cam_device* cam2, cam_device* cam3, moveControl* cradDevicePtr, ls_device* lsDevicePtr, QTableWidget* tablePtr, QSqlDatabase* DbPtr, roughnessFun* roughnessObjPtr);
	~program_7();

	QString partName = "前轴颈";
	QString partNub="WZ10-15-001-80";
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


	//光幕传感器测量直径对应的移动位置（其中第一个测量位置是对螺纹中径测量的补测直径点位）
	long ls_StepLength_axis5[15] = {
		733000,756000,783200,783500,783800,798000,814000,823200,851000,853250,853400,853550,853700,853850,857000};
	//光幕传感器直径测量结果存储
	double lsDiameterResult[15][10] = { //0号是一次测量值，1号为2次测量值，2号是两次均值，3号是工序卡序号,4为标准值，5为上限，6为下限，7第一次旋转距离（补偿），8第二次旋转距离（补偿），9旋转距离均值
		{ 0,0,0,30,46.45,0,-0.1,0,0,0},{ 0,0,0,31,45.9,0,-0.1,0,0,0},{ 0,0,0,32,42.9,0,-0.1,0,0,0},{ 0,0,0,32,42.9,0,-0.1,0,0,0}, { 0,0,0,32,42.9,0,-0.1,0,0,0},{ 0,0,0,34,44.983,0,-0.127,0,0,0},{ 0,0,0,35,43.15,0,-0.1,0,0,0},
		{ 0,0,0,36,40.1,0,-0.1,0,0,0}, { 0,0,0,38,38.6,0,-0.1,0,0,0}, { 0,0,0,43,39.8,0,-0.1,0,0,0},{ 0,0,0,43,39.8,0,-0.1,0,0,0},{ 0,0,0,43,39.8,0,-0.1,0,0,0},{ 0,0,0,43,39.8,0,-0.1,0,0,0},{ 0,0,0,43,39.8,0,-0.1,0,0,0},{ 0,0,0,42,38.9,0,-0.1,0,0,0} };
	
	
	//圆柱度测量参数初始化***********************************************************************************************************************************************************************
	
	/*
	//该零件工序卡无圆柱度要求
	//光幕传感器圆柱度测量参数设定
	long lsCylindricitySteplength_axis5[3][3] = {//0号是第一个截面5轴对应的位置，1号是2截面，2号是3截面
		{714000,717000,723000},  {844000,850000,856000},{2347400,2348900,2350400} };
	//光幕传感器圆柱度测量结果
	double lsCylindricityResult[3][3] = {//0号是工序卡序号，1号是测量值，2号是上限
		{19, 0, 0.004}, {25, 0, 0.004}, {162, 0, 0.004}};
	*/


	//光幕传感器圆度测量参数初始化********************************************************************************************************************************************************
	
	/*
	//该零件工序无圆度要求
	//圆度测量对应的5轴移动
	long roundness_axis5_steplength[1] = { 800000 };
	double roundnessResult[1][3] = {//0号是工序卡序号，1号是测量值，2号是上限
		{230.028, 0, 0.003}};
		*/
	
	// 光幕传感器跳动初始化参数 * *******************************************************************************************************************************************************
	
     /*
	 //该零件无跳动测量要求
	long lsRoundoutSteplength_axis5[18][3] = {//前16个点是跳动待测特征位置数据；索引为16是下基准；索引17是上基准
		{755200,760000,768000},{796000,800000,804000},{844000,850000,856000},{874000,877000,880000},{953500,954500,955500},
		{968500,970000,971500},{1198000,1204000,1210000},{1447000,1453000,1459000},{1792000,1798000,1804000},{1880000,1980000,2080000}, 
		{ 2174000,2180000,2186000 }, {2271500,2271700,2271900},{2301000,2302000,2302000},{2317000, 2323000, 2329000}, {0,0,0},
		{2555300,2555500,2555700},{714000,717000,723000},{2347000,2349000,2351000}
};                                                                                   
	float lsRoundoutResult[16][3] = {//0号工序卡序号，1号是测量值，2号是上限
		{6, 0, 0.05}, {23, 0, 0.01}, {24, 0, 0.01}, {26, 0, 0.01}, {70, 0, 0.012}, {80, 0, 0.03}, {85, 0, 0.03}, {86, 0, 0.038},
		{102, 0, 0.038}, {103, 0, 0.038}, {157, 0, 0.038}, {139.1, 0, 0.05}, {139.2, 0, 0.05}, {160, 0, 0.05}, {165, 0, 0.05}, {154, 0, 0.05}
	};
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
	roundoutData*  roundoutDataPtr[18];//前16个点是跳动待测特征位置数据；索引为16是下基准；索引17是上基准
	double directionVector[3];
	double aixsReferancePoint[3];
	

	*/

	//远心相机测量参数初始化****************************************************************************************************************************************************************************
	string imgSavePath[2];//远心相机采集图像地址
	long cam0_StepLength_axis5[2] = {412000,525000};//远心相机测量时轴5对应的点位
	double cam0_arriveOrgEncode_axis5[2] = { 0, 0};//远心相机到位对应的光栅尺原始读数（用于补偿粗糙度测量移动位置）
	int cam0_exposeTime=14;//远心相机的曝光

	//远心相机使用的全局参数*
	double Calik = 0.01218603;//相机标定系数
	//double PI = 3.1415926;
	double ImageRowSize = 5120;//图像像素行数
	double ImageColSize = 5120;//图像像素列数
	double ImageRowSize_Cali;//图像换算后行距离
	double ImageColSize_Cali;//图像换算后列距离

	double pic1To2_moveDistance=0 ;//相机从位置1到2位移距离
	int fault_detect = 0;//用于记录远心图像算法的异常情况
	
	
	
	//孔径相机测量参数初始化***********************************************************************************************************************************************************************************

	/*
	//该零件无孔径测量要求
	vector<string> cam1PathList;//孔径相机拍照储存位置（需要多次拍照）
	double CalikKong = 0.00691842;//测孔花键相机标定系数   从0.686改
	long cam1_prepare[1][5] = {//0号元素是5轴位置，1号元素是2轴拍照位置，2号元素是2轴退回位置,3号元素为特征的均布个数,4号是图像的绘制模式（参考主程序中的AxisMeasurement::displayImg说明）
		{-85000,-255000,-220000,10,8}};
	int cam1_exposeTime=550;//测圆孔相机曝光
	*/


	//粗糙度相机测量参数设定***********************************************************************************************************************************************************************************


	
	//测量粗糙度
	string roughnessPicPath[8];
	long cam2_parpare[1][3] = {//0号元素五轴移动位置；1号1轴移动到位置（参考）；2号1轴退回位置(；                     
		{-33700, -177790-1000, 120000} };
	float cam2_radius[1][2] = {//0号元素参考回转半径值；1号元素实际回转半径值
		{23.2170, 0}};
	double roughnessResult[1][3] = {//0号粗糙度编号，1号粗糙度上限；2号测量值
		{89, 1.6, 99}};
	int cam2_exposeTime=550;//粗糙度镜头曝光
	

	//使用远心相机测量螺纹中径的变量********************************************************************
	int screwPrepare[1][4] = { //1号元素是远心相机五轴移动位置(螺纹采集)，2号是光幕五轴的移动位置（光幕采集），3号是远心底二次采集，4号元素是旋转一圈需要采集的照片数量
		{525000,733000,412000,12} };
	vector <double> screwResult;//用于存储中径数据
	vector <double> screwResult_Adl;//用于存储中径跳动数据
	vector <double> screwX00;//用于存储中径数据
	vector <double> screwZhongjingCOl;//

	
    //pair<double, double> program_7::cam0Picture1_algorithmLuowen(HObject ho_Image, HObject ho_Image_L, double L, float OUT2, int m, double Pix_zhoujing_chuanru);//用远心相机0的第一张图像测螺纹
	pair<double, double> cam0Picture1_algorithmLuowen(HObject ho_Image, HObject ho_Image_L, double L, float OUT2, int m, double Pix_zhoujing_chuanru);//用远心相机0的第一张图像测螺纹
	double guangmuceD;//传递第一次的直径
	double OUT2_1;//传递第一次的OUT2
	double cam0Picture1_L_K_b(HObject ho_Image, double* hv_Box);//返回值为最下面的点col Box分别是hv_Nr_new, hv_Nc_new, hv_Dist_new
	
	struct screwRoundoutData {
		vector<float> diameter;//用于存储跳动测量的直径;
		vector<float> out2;
		vector<float> out3;
		vector<float> out4;
		vector <string> screwImgPath;
		vector <string> referanceImgPath;
	};
	/*
	struct screwRoundoutData {
		float diameter[12];//用于存储跳动测量的直径;
		float out2[12];
		float out3[12];
		float out4[12];
	};
	*/
	//光幕传感器跳动测量数据
	screwRoundoutData*  myScrewRoundoutDataPtr[1];

	//调试程序需要使用的变量*****************************************************************************************************************************************************************************************


	

	//程序需要使用的基本函数*************************************************************************************************************************************************************************************
	

	void run();
	void lsSensorMeasure_diameter(int positionNumber,int time);//使用光幕传感器进行直径测量
	//void lsSensorMeasure_cylindricity(int positionNumber, float intervalTime);//使用光幕传感器进行圆柱度测量
	string cam0_Measure_prepare(int positionNumber);//利用远心镜头进行测量,移动和采集函数
	void cam0_roundoutMeasure_prepare(int positionNumber);//利用远心镜头进行跳动的数据采集。
	//void cam1_Measure_prepare(int positionNumber);//使用cam1进行孔径测量
	void cam2_Measure_prepare(int positionNumber);//使用cam2进行粗糙度测量
	void saveAsExcel();//将主界面上的表格保存为excel文件
	bool mergeCells(QString start, QString end, QString value);//合并单元格
	
	void creatDatabaseTable();//创建数据库中的表格
	void writeToDatabase();//写入到数据库中
	void zeroMeasureNub();//清空测量统计数据
	void pushback_vectors(vector<double>& fResult, int fType, double fIndex, double fNominalsize, double fLowerTolerance, double fUpperTolerance);//将一条完整的测量数据储存并在excel上显示
	void clearVector(vector <float>& list);//清空vector数组内容并且缩至最小
	void clearScrewRoundOutData();//清空螺纹跳动数据

	//跳动测量需要使用的函数（以内孔作为跳动基准，测不了）****************************************************************************************************************************************************************************************
	

	/*
	void lsSensorMeasure_roundOutPrepare(int startLocation, int endLocation, int positionNumber, int intervalTime);//使用光幕传感器进行跳动测量;
	void axisLineCalculate(double pointsData[][3], int pointsNum, double* directionVector, double* aixsReferancePoint);//计算拟合轴线
	void centerCosFit(int positionNumber, int positionIndex);//拟合对应跳动位置的余弦曲线
	void axisLoaction(int positionNumber, int positionIndex, double* directionVector, double* aixsReferancePoint);//计算各跳动位置实际回转轴心
	void roundoutCalculate(int positionNumber, int positionIndex, int calculateMode);//计算对应的跳动位置的跳动值
	void clearRoundoutData();//清除跳动测量数据
	*/
	

	//圆度测量需要使用的函数*****************************************************************************************************************************************************************************************************
	
	//零件没有圆度测量要求
	//void lsSensorMeasure_roundness(int positionNumber, float intervalTime);//使用光幕传感器进行圆度测量


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

	HObject imgAug(string imgPath);
	void cam0Picture_algorithm();//远心相机各点位图像算法
	

	
	
	
	


	//孔径相机检测函数****************************************************************************************************************************************************************************************************


	//double cam1Picture_holeAlgorithm(string imgPath);


	//数值补偿函数********************************************************************************************************************************************************************************************************


	///测试函数************************************************************************************************************************************************************************************************************


signals:
	void partsNumber(QString partsNumber);//返回当前轴的零件号
	void updateDeviceInf(QString deviceInf);//更新设备信息
	void imgInf(const Mat* image, QString source,int displayMode);//图像显示信号
	void lsResult(int position, float result);//光幕传感器结果显示信号
	void programProcess(QString processInf,int Precentage);//显示测量进程信息和进度条
	void Finished(bool normalFlag);//program结束信号
	void programTips(QString programInf, int mode);//程序到达位置提示信号
	void measureStatistics(QString result, int ngFeatureNum, int measureNum, float currentYield);
	void fixtureTips(int programNum);//机心夹安装示意信号
};

	




