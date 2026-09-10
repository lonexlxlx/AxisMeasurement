//program_34(J644123B-260)****************************************************************************


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




class program_34 :public QThread//需要将所有 program_34替换为新的子程序号
{
	Q_OBJECT;
public:

	//测试程序初始化参数**************************************************************************************************************************************************************


	program_34(cam_device* cam1, cam_device* cam2, cam_device* cam3, moveControl* cradDevicePtr, ls_device* lsDevicePtr, QTableWidget* tablePtr, QSqlDatabase* DbPtr, roughnessFun* roughnessObjPtr);
	~program_34();

	QString partName = "动力涡轮传动轴";//需要修改为零件名称
	QString partNub="S100453017-180";//需要修改为零件图号
	QString operatorName;
	QString partsID;
	QString allFeatureFlag;
	vector <double> resultVectorList;//存储以上所有特征信息，第一位是特征序号fIndex(按照该序号排序，示例代码如下)
	QAxObject* workbook;// 当前工作簿指针
	QAxObject* worksheet;//活动工作表指针
	map <int, QString> featureMap = {
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
	long stepLengthCompensation_cam0Place= 678502;//五轴补偿计算初始拍照位置
	long stepLengthCompensation_axis5=0;//5轴移动位置补偿

	//光幕传感器直径&圆柱度测量初始化参数******************************************************************************************************************************************************************


	//光幕传感器测量对应的移动位置
	

	//光幕传感器直径测量结果存储（//需要将数组第一维长度修改为直径测量位置的总数），并将直径信息填入
	long ls_StepLength_axis5[16] = { 664518,722180,760449,781848,828385,873521,904334,934125,972649,1393360,1568379,2106970,2169910,2194903,2237639,2307255 };

	long lsCylindricitySteplength_axis5[2][3] = { {858827,859268,859500},{859033,859268,859489} };

	//光幕传感器圆柱度测量参数设定（需要将数组长度修改为圆柱度测量位置的总数，并将点位信息填入）
	double lsDiameterResult[16][10] = { {0,0,0,47,22,0,-.2,0,0,0},{0,0,0,69,23.05,0,-.1,0,0,0},{0,0,0,68,23.25,0,-.1,0,0,0},{0,0,0,670,26.5,0,-.2,0,0,0},{0,0,0,46,29,0,-.13,0,0,0},{0,0,0,42,26,.05,-.1,0,0,0},{0,0,0,41,31.5,0,-.1,0,0,0},{0,0,0,40,26.8,0,-.4,0,0,0},{0,0,0,39,32,0,-.236,0,0,0},{0,0,0,36,29,0,-.1,0,0,0},{0,0,0,9,29,0,-0.1,0,0,0},{0,0,0,5,29,0,-.1,0,0,0},{0,0,0,4,33,0,-.1,0,0,0},{0,0,0,3,34.3,0,-.1,0,0,0},{0,0,0,2,37.2,0,-.1,0,0,0},{0,0,0,1,43.2,0,-.1,0,0,0} };
	//光幕传感器圆柱度测量结果（需要将数组长度修改为圆柱度测量位置的总数，并将圆柱度工序卡信息填入）
	double lsCylindricityResult[2][3] = {//0号是工序卡序号，1号是测量值，2号是上限
		{26, 0, 0.004},{162,0,0.004} };


	// 光幕传感器跳动初始化参数 * *******************************************************************************************************************************************************
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

	//（需要将数组长度修改为跳动采集轴段位置的总数，并将点位信息填入）
	long lsRoundoutSteplength_axis5[7][3] = {//第一组是A基准,第二组是基准B也是137号待测圆柱面
		{710260,719637,729014},{798400,803436,808472},{832650,832697,832744},{951457,954289,957122},{1196580,1207530,1218480},{2344950,2348860,2352770},{2545170,2554290,2563410}

 };
	//（需要将数组长度修改为跳动采集轴段位置的总数，并将跳动图纸信息填入，如果采集轴段为基准可以默认填入{0，0,0}）
	float lsRoundoutResult[7][3] = {//0号工序卡序号，1号是测量值，2号是上限
		{0,0,0}, {24,0,0.01},{25,0,0.01},{71,0,0.012},{86, 0, 0.03},{0,0,0},{154,0,0.05} };
	//光幕传感器跳动测量数据（需要将数组长度修改为跳动采集轴段位置的总数）
	roundoutData* roundoutDataPtr[7];//第一组是A基准,第2组是100号跳动尺寸,是第三组诗99号；第四组是基准b;
	





	//远心相机测量参数初始化****************************************************************************************************************************************************************************


	string imgSavePath[1];//远心相机采集图像地址（需要将数组长度修改为远心拍照位置的总数）
	long cam0_StepLength_axis5[1] = { 647856 };//远心相机测量时轴5对应的点位（需要将数组长度修改为远心拍照位置的总数，并将拍摄的点位写入）
	double cam0_arriveOrgEncode_axis5[1] = { 0 };//远心相机到位对应的光栅尺原始读数（用于补偿粗糙度测量移动位置）（需要将数组长度修改为远心拍照位置的总数，默认填入0）
	int cam0_exposeTime=14;//远心相机的曝光
	int fault_detect;//远心相机异常捕获记录

	//远心相机使用的全局参数*
	double Calik = 0.01218603;//相机标定系数
	//double PI = 3.1415926;
	double ImageRowSize = 5120;//图像像素行数
	double ImageColSize = 5120;//图像像素列数
	double ImageRowSize_Cali;//图像换算后行距离
	double ImageColSize_Cali;//图像换算后列距离
	
	

	

	//螺纹中径参数初始化
	int screwPrepare[2][3] = { //1号元素是远心相机五轴移动位置(螺纹采集)，2号是光幕五轴的移动位置（光幕采集），3号元素是旋转一圈需要采集的照片数量
		{366600,640000,4},{693000,993000,4} };
	struct screwRoundoutData {
		vector<float> diameter;//用于存储跳动测量的直径;
		vector<float> out2;
		vector<float> out3;
		vector<float> out4;
		vector <string> screwImgPath;
		vector <string> referanceImgPath;
	};
	screwRoundoutData* myScrewRoundoutDataPtr[2];
	vector <double> screwResult;//用于存储中径数据
	//vector <double> screwResult_Adl;//用于存储中径跳动数据
	//vector <double> screwX00;//用于存储中径数据
	//vector <double> screwZhongjingCOl;//

	//孔径相机测量参数初始化***********************************************************************************************************************************************************************************


	vector<string> cam1PathList;//孔径相机拍照储存位置（需要多次拍照）
	double CalikKong = 0.00691842;//测孔花键相机标定系数   从0.686改
	//需要修改数组第一维长度为孔径相机拍摄的位置总数，并将孔径相机的点位写入
	long cam1_prepare[1][5] = {//0号元素是个光幕轴位置(孔径镜头对准时），1号元素是x轴拍照位置，2号元素是2轴退回位置,3号元素为特征的均布个数,4号是图像的绘制模式（参考主程序中的AxisMeasurement::displayImg说明）
		{1522150,-294386,-220000,3,17}
		//需要3.8 改绘图模式至AXISMEASUREMENT.CPP 中3.5 的CASE 17
	};
	int cam1_exposeTime=550;//测圆孔相机曝光
	double cam1_axis5Place[1] = {0};//将修改数组第一维长度为孔径相机拍摄的位置总数（数组内的元素默认写为0）
	
	//粗糙度相机测量参数设定***********************************************************************************************************************************************************************************


	string roughnessPicPath[5];


	//需要修改数组为粗糙度点位总数，并写入点位信息
	long cam2_parpare[3][3] = {//0号元素五轴移动位置；1号1轴移动到位置（参考）；2号1轴退回位置(其中前三组位置是在凸台下方)；

		{79841,-195140,120000},{187590,-80531,120000},{1582160,-207467,120000}
	};
	//需要修改数组为粗糙度点位总数，并写入半径信息
	float cam2_radius[3][2] = {//0号元素参考回转半径值；1号元素实际回转半径值
		{20.026,0},{42.938,0},{17.562,0}
	};
	//需要修改数组为粗糙度点位总数，并写入粗糙度尺寸信息
	double roughnessResult[3][3] = {//0号粗糙度特征号，1号粗糙度上限；2号测量值
		{37, 0.8, 99},{46, 0.8, 99},{161, 0.8, 99}
	};
	int cam2_exposeTime=550;//粗糙度镜头曝光


	//调试程序需要使用的变量*****************************************************************************************************************************************************************************************
	




	//程序需要使用的基本函数*************************************************************************************************************************************************************************************


	void run();
	bool stepLengthCompensationCaculation_axis5();
	void lsSensorMeasure_diameter(int positionNumber, int time);//使用光幕传感器进行直径测量
	void lsSensorMeasure_cylindricity(int positionNumber, float intervalTime);//使用光幕传感器进行圆柱度测量
	void cam1_Measure_prepare(int positionNumber);//使用cam1进行孔径测量数据采集
	void cam2_Measure_prepare(int positionNumber);//使用cam2进行粗糙度测量数据采集
	void saveAsExcel();//将主界面上的表格保存为excel文件
	bool mergeCells(QString start, QString end, QString value);//合并单元格
	void creatDatabaseTable();//创建数据库中的表格
	void writeToDatabase();//写入到数据库中
	void zeroMeasureNub();//清空测量统计数据
	void pushback_vectors(vector<double>& fResult, int fType, double fIndex, double fNominalsize, double fLowerTolerance, double fUpperTolerance);//将一条完整的测量数据储存并在excel上显示

	//跳动测量需要使用的函数****************************************************************************************************************************************************************************************


	void lsSensorMeasure_roundOutPrepare(int startLocation, int endLocation, int positionNumber, int intervalTime);//使用光幕传感器进行跳动测量;
	void axisLineCalculate(double pointsData[][3], int pointsNum, double* directionVector, double* aixsReferancePoint);//计算拟合轴线
	void centerCosFit(int positionNumber, int positionIndex);//拟合对应跳动位置的余弦曲线
	void axisLoaction(int positionNumber, int positionIndex, double* directionVector, double* aixsReferancePoint);//计算各跳动位置实际回转轴心
	void roundoutCalculate(int positionNumber, int positionIndex, int calculateMode);//计算对应的跳动位置的跳动值
	void clearRoundoutData();//清除跳动测量数据


	//远心相机图像检测算法***********************************************************************************************************************************************************************************************


	string cam0_Measure_prepare(int positionNumber);//利用远心镜头进行测量,移动和采集函数

	void cam0Picture_algorithm();//远心相机图像算法（需要在此函数内写入远心视觉处理算法）

	double cam0Picture1_algorithmLuowenDown(HObject ho_Image, HObject ho_Image_L, double L, float OUT2, int m, double Pix_zhoujing_chuanru);//远心螺纹处理函数
	double cam0Picture1_algorithmLuowenUp(HObject ho_Image, HObject ho_Image_L, double L, float OUT2, int m, double Pix_zhoujing_chuanru);//远心螺纹处理函数

	HObject imgAug(string imgPath);
	double cam0Picture1_L_K_b_Down(HObject ho_Image, double* hv_Box);
	double cam0Picture1_L_K_b_Up(HObject ho_Image, double* hv_Box);
	void TemplateMatching_TLineFP_P(double* TLineFP_result, HObject ho_Image, HTuple hv_ModelFile, int MatchingLine1, int MatchingLine2, int MatchingLine3, int MatchingLine4,
		int Metrology1, int Metrology2, int Metrology3, int Metrology4,
		double ShapeModel1, double ShapeModel2, double ShapeModel3, double ShapeModel4, double ShapeModel5);
	void Straight_TLineFP_P137(double* TLineFP_result, HObject ho_Image, int Line1, int Line2, int Line3, int Line4,
		int Metrology1, int Metrology2, int Metrology3, int Metrology4);
	void Straight_TLineFP_P116(double* TLineFP_result, HObject ho_Image, int Line1, int Line2, int Line3, int Line4,
		int Metrology1, int Metrology2, int Metrology3, int Metrology4);
	void Straight_TLineFP_P170(double* TLineFP_result, HObject ho_Image, int Line1, int Line2, int Line3, int Line4,
		int Metrology1, int Metrology2, int Metrology3, int Metrology4);
	void Straight_TLineFP_P102(double* TLineFP_result, HObject ho_Image, int Line1, int Line2, int Line3, int Line4,
		int Metrology1, int Metrology2, int Metrology3, int Metrology4);
	void Straight_TLineFP_P(double* TLineFP_result, HObject ho_Image, int Line1, int Line2, int Line3, int Line4,
		int Metrology1, int Metrology2, int Metrology3, int Metrology4);
	double TLineFP_Chamfer_P(HObject ho_Image, double GenRect1, double GenRect2, double GenRect3, double GenRect4,
		int EdgesSubP1, int EdgesSubP2, int EdgesSubP3,
		bool Segment, int SegmentContour1, int SegmentContour2, int SegmentContour3,
		double SelectContour1, double SelectContour2, double SelectContour3,
		int UnionContour1, int UnionContour2,
		int FitCircleContour1, int FitCircleContour2, int FitCircleContour3, int FitCircleContour4, int FitCircleContour5);
	double TLineFP_Chamfer_H2_117up(HObject ho_Image, double GenRect1, double GenRect2, double GenRect3, double GenRect4,
		int EdgesSubP1, int EdgesSubP2, int EdgesSubP3,
		bool Segment, int SegmentContour1, int SegmentContour2, int SegmentContour3,
		double SelectContour1, double SelectContour2, double SelectContour3,
		int UnionContour1, int UnionContour2,
		int FitCircleContour1, int FitCircleContour2, int FitCircleContour3, int FitCircleContour4, int FitCircleContour5);
	double TLineFP_Chamfer_H2(HObject ho_Image, double GenRect1, double GenRect2, double GenRect3, double GenRect4,
		int EdgesSubP1, int EdgesSubP2, int EdgesSubP3,
		bool Segment, int SegmentContour1, int SegmentContour2, int SegmentContour3,
		double SelectContour1, double SelectContour2, double SelectContour3,
		int UnionContour1, int UnionContour2,
		int FitCircleContour1, int FitCircleContour2, int FitCircleContour3, int FitCircleContour4, int FitCircleContour5);


	
	void cam0_roundoutMeasure_prepare(int positionNumber);//远心螺纹数据采集
	void clearScrewRoundOutData();//清空螺纹跳动数据
	//孔径相机检测函数****************************************************************************************************************************************************************************************************


	double cam1Picture_holeAlgorithm0(string imgPath);//h0孔测量函数，需要将调试的h0孔径测量算法写入该函数
	double cam1Picture_holeAlgorithm1(string imgPath);//h1孔测量函数，需要将调试的h1孔径测量算法写入该函数
	double cam1Picture_holeAlgorithm2(string imgPath);//h2孔测量函数，需要将调试的h2孔径测量算法写入该函数
	double cam1Picture_holeAlgorithm3(string imgPath);//h3孔测量函数，需要将调试的h3孔径测量算法写入该函数

	//数值补偿函数********************************************************************************************************************************************************************************************************


	///测试函数************************************************************************************************************************************************************************************************************


signals:
	void partsNumber(QString partsNumber);//返回当前轴的零件号
	void updateDeviceInf(QString deviceInf);//更新设备信息
	void imgInf(const Mat* image, QString source, int displayMode);//图像显示信号
	void lsResult(int position, float result);//光幕传感器结果显示信号
	void programProcess(QString processInf, int Precentage);//显示测量进程信息和进度条
	void Finished(bool normalFlag);//program结束信号
	void programTips(QString programInf, int mode);//设备到达相关位置，显示提示信息,同时根据不同的mode值对主界面按钮进行相关控制
	void measureStatistics(QString result, int ngFeatureNum, int measureNum, float currentYield);
	void fixtureTips(int programNum);//机心夹安装示意信号
};






