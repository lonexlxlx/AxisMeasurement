//WZ10-45-1080-370****************************************************************************


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




class program_0 :public QThread
{
	Q_OBJECT;
public:

	//测试程序初始化参数**************************************************************************************************************************************************************


	program_0(cam_device* cam1, cam_device* cam2, cam_device* cam3, moveControl* cradDevicePtr, ls_device* lsDevicePtr, QTableWidget* tablePtr, QSqlDatabase* DbPtr, roughnessFun* roughnessObjPtr);
	~program_0();

	QString partName = "动力涡轮传动轴";
	QString partNub = "WZ10-45-1080-370";
	QString operatorName;
	QString partsID;
	QString allFeatureFlag;
	//std::string stime;//获取当前运行时间时间
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
	long stepLengthCompensation_cam0Place = 716900;//五轴补偿计算初始拍照位置
	long stepLengthCompensation_axis5 = 0;//5轴移动位置补偿

	//光幕传感器直径&圆柱度测量初始化参数******************************************************************************************************************************************************************


	//光幕传感器测量对应的移动位置
	long ls_StepLength_axis5[42] = {
		634000 ,695000, 756000, 800000, 850000 ,891922, 920000, 934000, 954500, 970500,
		982000,1066300, 1204000, 1328620, 1453000, 1608040, 1800000, 1960000, 2178000, 2201000,
		2210425,2223255,2236050,2252940,2271850, 2302000, 2348900, 2365000, 2387370,2429900,
		2430200,2430500 ,2433080,2433230,2433380,2433530,2433680,2436400,2436700,2437000,
		2447400, 2555500 };


	//光幕传感器直径测量结果存储
	double lsDiameterResult[42][10] = { //0号是一次测量值，1号为2次测量值，2号是两次均值，3号是工序卡序号,4为标准值，5为上限，6为下限，7第一次旋转距离（补偿），8第二次旋转距离（补偿），9旋转距离均值
		{ 0, 0, 0, 17, 37.7, 0, -0.2, 0, 0, 0}, { 0, 0, 0, 4, 40.015, 0, -0.007, 0, 0, 0}, { 0, 0, 0, 5, 39.6, 0, -0.2, 0, 0, 0}, { 0, 0, 0, 7, 40.055, 0, -0.011, 0, 0, 0},{ 0, 0, 0, 10, 40.053, 0.013, 0.005, 0, 0, 0},
		{ 0, 0, 0, 5, 39.6, 0, -0.2, 0, 0, 0}, { 0, 0, 0, 11, 40.18, 0, -0.011, 0, 0, 0}, { 0, 0, 0, 12, 46, 0, -0.2, 0, 0, 0},{ 0, 0, 0, 76, 85.65, 0.225, 0.215, 0, 0, 0}, { 0, 0, 0,77, 47.24, 0, -0.25, 0, 0, 0},
		{ 0, 0, 0, 95, 29.6, 0,-0.1,0,0,0},{ 0, 0, 0, 97, 28.98, 5, 0, 0, 0, 0}, { 0, 0, 0, 98, 32.1, 0, -0.1, 0, 0, 0},{ 0, 0, 0, 99, 28.98, 5, 0, 0, 0, 0}, { 0, 0, 0, 100, 30.3, 0, -0.1, 0, 0, 0},
		{ 0, 0, 0, 101, 28.98, 5, 0, 0, 0, 0},{ 0, 0, 0, 102, 29.7, 0, -0.1, 0, 0, 0},{ 0, 0, 0, 94, 28.98, 5, 0, 0, 0, 0}, { 0, 0, 0, 156, 32.1, 0, -0.2, 0, 0, 0}, { 0, 0, 0, 143, 29.3, 0, -0.1, 0, 0, 0},
	    { 0, 0, 0, 144, 29.7, 0, -0.2, 0, 0, 0},{ 0, 0, 0, 144, 29.7, 0, -0.2, 0, 0, 0},{ 0, 0, 0, 144, 29.7, 0, -0.2, 0, 0, 0},{ 0, 0, 0, 145, 29.3, 0, -0.1, 0, 0, 0},{ 0, 0, 0, 146, 32.6, 0, -0.2, 0, 0, 0},
		{ 0, 0, 0, 147, 33.6, 0, -0.2, 0, 0, 0}, { 0, 0, 0, 148, 35.14, 0, -0.011, 0, 0, 0}, { 0, 0, 0, 149, 29.5, 0.1, -0.1, 0, 0, 0}, {0, 0, 0, 150, 32.38, 0, -0.2, 0, 0, 0},{0,0,0,207,34.8,0,-0.2,0,0,0},
		{0,0,0,207,34.8,0,-0.2,0,0,0},{0,0,0,207,34.8,0,-0.2,0,0,0},{0,0,0,208,31.34,0,-0.081,0,0,0}, {0,0,0,208,31.34,0,-0.081,0,0,0},{0,0,0,208,31.34,0,-0.081,0,0,0},
		{0,0,0,208,31.34,0,-0.081,0,0,0},{0,0,0,208,31.34,0,-0.081,0,0,0}, {0,0,0,207,34.8,0,-0.2,0,0,0},{0,0,0,207,34.8,0,-0.2,0,0,0},{0,0,0,207,34.8,0,-0.2,0,0,0},
		{ 0, 0, 0, 151, 29.6, 0, -0.2, 0, 0, 0}, { 0, 0, 0, 168, 29.3, 0, -0.2, 0, 0, 0} };


	//光幕传感器圆柱度测量参数设定
	long lsCylindricitySteplength_axis5[3][3] = {//0号是第一个截面5轴对应的位置，1号是2截面，2号是3截面
		{714000,717000,723000},  {844000,850000,856000},{2347400,2348900,2350400} };
	//光幕传感器圆柱度测量结果
	double lsCylindricityResult[3][3] = {//0号是工序卡序号，1号是测量值，2号是上限
		{20, 0, 0.004}, {26, 0, 0.004}, {162, 0, 0.004} };


	// 光幕传感器跳动(15号花键不测)初始化参数 * *******************************************************************************************************************************************************
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

	long lsRoundoutSteplength_axis5[15][3] = {//第一组是下基准,第17组是上基准
		{714000,717000,723000},{755200,760000,768000},{796000,800000,804000},{844000,850000,856000},{874000,877000,880000},
		{953500,954500,955500},{968500,970000,971500},{1198000,1204000,1210000},{1447000,1453000,1459000},{1792000,1798000,1804000},
		{1880000,1980000,2040000},{ 2174000,2180000,2186000 },{2317000, 2323000, 2329000}, {2347000,2349000,2351000} ,{2555300,2555500,2555700},
	};
	float lsRoundoutResult[15][3] = {//0号工序卡序号，1号是测量值，2号是上限
		{0,0,0}, {6, 0, 0.05}, {24, 0, 0.01}, {25, 0, 0.01}, {27, 0, 0.01}, {71, 0, 0.012}, {81, 0, 0.03}, {86, 0, 0.03}, {87, 0, 0.038},{103, 0, 0.038},
		{104, 0, 0.038}, {157, 0, 0.038},  {160, 0, 0.05},{0,0,0},  {154, 0, 0.05}
	};
	//光幕传感器跳动测量数据
	roundoutData* roundoutDataPtr[15];//第一组是下基准,第17组是上基准
	double directionVector[3];
	double aixsReferancePoint[3];





	//远心相机测量参数初始化****************************************************************************************************************************************************************************


	string imgSavePath[11];//远心相机采集图像地址
	long cam0_StepLength_axis5[11] = { 401400,505100,607300,716900,955000,1204000,1552000,1942000,2043000,2148000,2239000 };//远心相机测量时轴5对应的点位
	double cam0_arriveOrgEncode_axis5[11] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };//远心相机到位对应的光栅尺原始读数（用于补偿粗糙度测量移动位置）
	int cam0_exposeTime;//远心相机的曝光

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
	double pic4To5_moveDistance = 119.0640418275006;//相机从位置4到5位移距离
	double pic5To6_moveDistance = 124.50084039846894;//相机从位置5到6位移距离
	double pic6To7_moveDistance = 173.99314483562853;//相机从位置6到7位移距离
	double pic7To8_moveDistance = 195.0172795271967;//相机从位置7到8位移距离
	double pic8To9_moveDistance = 50.46466599040002;//相机从位置8到9位移距离
	double pic9To10_moveDistance = 52.491020829582;//相机从位置9到10位移距离
	double pic10To11_moveDistance = 45.50826979759722;//相机从位置10到11位移距离
	double pic4To11_moveDistance = 761.039263206374;//相机从位置4到11位移距离
	double pic1To11_moveDistance = 918.7535806402789;//相机从位置1到11位移距离 总的实际距离

	// 远心相机1需要传递的参数
	double Row_begindaduan;
	double Row_30right;
	double Row_31right;
	double cam0_1_460_32TLineFP[4];
	// 远心相机2需要传递的参数
	double Row_63left;
	double Row_62left;
	double cam0_2_460_59TLineFP[4];
	double cam0_2_460_62LeftChamfer22_TLineFP[4];
	// 远心相机3需要传递的参数
	double Row_62right;
	double Row_45left;
	// 远心相机4需要传递的参数
	double Row_61right;
	// 远心相机5需要传递的参数
	double Row_186left;
	// 远心相机6需要传递的参数

	// 远心相机7需要传递的参数

	// 远心相机8需要传递的参数
	double Row_108left;
	double Row_109left;
	// 远心相机9需要传递的参数
	double Row_122right;
	// 远心相机10需要传递的参数

	// 远心相机11需要传递的参数


	//螺纹中径参数初始化
	int screwPrepare[1][3] = { //1号元素是远心相机五轴移动位置(螺纹采集)，2号是光幕五轴的移动位置（光幕采集），3号元素是旋转一圈需要采集的照片数量
		{401400,634000,4} };
	struct screwRoundoutData {
		vector<float> diameter;//用于存储跳动测量的直径;
		vector<float> out2;
		vector<float> out3;
		vector<float> out4;
		vector <string> screwImgPath;
		vector <string> referanceImgPath;
	};
	screwRoundoutData* myScrewRoundoutDataPtr[1];
	vector <double> screwResult;//用于存储中径数据
	vector <double> screwResult_Adl;//用于存储中径跳动数据
	vector <double> screwX00;//用于存储中径数据
	vector <double> screwZhongjingCOl;//
	//孔径相机测量参数初始化***********************************************************************************************************************************************************************************


	vector<string> cam1PathList;//孔径相机拍照储存位置（需要多次拍照）
	double CalikKong = 0.00691842;//测孔花键相机标定系数   从0.686改
	long cam1_prepare[2][5] = {//0号元素是5轴位置，1号元素是2轴拍照位置，2号元素是2轴退回位置,3号元素为特征的均布个数,4号是图像的绘制模式（参考主程序中的AxisMeasurement::displayImg说明）
		{1523640 - 600,-297800,-220000,3,2},{1745050 - 600,-282700,-220000,1,1}
	};
	int cam1_exposeTime;//测圆孔相机曝光


	//粗糙度相机测量参数设定***********************************************************************************************************************************************************************************


	string roughnessPicPath[5];
	long cam2_parpare[7][3] = {//0号元素五轴移动位置；1号1轴移动到位置（参考）；2号1轴退回位置(其中前三组位置是在凸台下方)；

		{-71350, 0-1000,120000}, {33650, -194100 - 1000, 120000}, {83650, -194000 - 1000,120000}, {125572, -195500 - 1000, 120000},{188150, -79600 - 1000, 120000},
		{562270, -221370 - 1000, 120000},{1582550, -206400 - 1000, 130000}
	};
	float cam2_radius[7][2] = {//0号元素参考回转半径值；1号元素实际回转半径值
		{20.0089, 0},{20.0273, 0},{20.0337, 0},{19.7484, 0},{42.9309, 0},
		{14.549, 0},{17.5693, 0}
	};
	double roughnessResult[7][3] = {//0号粗糙度编号，1号粗糙度上限；2号测量值
		{33, 0.8, 99},{44, 1.6, 99},{37, 0.8, 99},{41, 0.8, 99},{243, 0.8, 99},
		{91, 1.6, 99},{161, 0.8, 99}
	};
	int cam2_exposeTime;//粗糙度镜头曝光


	//调试程序需要使用的变量*****************************************************************************************************************************************************************************************
	clock_t startTime[9], endTime[9];
	double tot_time[9];




	//程序需要使用的基本函数*************************************************************************************************************************************************************************************


	void run();
	bool stepLengthCompensationCaculation_axis5();//根据零件计算5轴补偿位置
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
	void axisLineCalculate(double pointsData[][3], int pointsNum);//计算拟合轴线
	void centerCosFit(int positionNumber, int positionIndex);//拟合对应跳动位置的余弦曲线
	void axisLoaction(int positionNumber, int positionIndex);//计算各跳动位置实际回转轴心
	void roundoutCalculate(int positionNumber, int positionIndex, int calculateMode);//计算对应的跳动位置的跳动值
	void clearRoundoutData();//清除跳动测量数据

	//远心相机图像检测算法***********************************************************************************************************************************************************************************************


	string cam0_Measure_prepare(int positionNumber);//利用远心镜头进行测量,移动和采集函数

	void cam0Picture1_algorithm(string imgPath);//远心相机1图像算法
	void cam0Picture2_algorithm(string imgPath);//远心相机2图像算法
	void cam0Picture3_algorithm(string imgPath);//远心相机3图像算法
	void cam0Picture4_algorithm(string imgPath);//远心相机4图像算法
	void cam0Picture5_algorithm(string imgPath);//远心相机5图像算法
	void cam0Picture6_algorithm(string imgPath);//远心相机6图像算法
	void cam0Picture7_algorithm(string imgPath);//远心相机7图像算法
	void cam0Picture8_algorithm(string imgPath);//远心相机8图像算法
	void cam0Picture9_algorithm(string imgPath);//远心相机9图像算法
	void cam0Picture10_algorithm(string imgPath);//远心相机10图像算法
	void cam0Picture11_algorithm(string imgPath);//远心相机11图像算法
	double cam0Picture1_algorithmLuowen(HObject ho_Image, HObject ho_Image_L, double L, float OUT2, int m, double Pix_zhoujing_chuanru);//远心螺纹处理函数
	HObject imgAug(string imgPath);
	double cam0Picture1_L_K_b(HObject ho_Image, double* hv_Box);
	void TemplateMatching_TLineFP_P(double* TLineFP_result, HObject ho_Image, HTuple hv_ModelFile, int MatchingLine1, int MatchingLine2, int MatchingLine3, int MatchingLine4,
		int Metrology1, int Metrology2, int Metrology3, int Metrology4,
		double ShapeModel1, double ShapeModel2, double ShapeModel3, double ShapeModel4, double ShapeModel5);
	void cam0_roundoutMeasure_prepare(int positionNumber);//远心螺纹数据采集
	void clearScrewRoundOutData();//清空螺纹跳动数据
	//孔径相机检测函数****************************************************************************************************************************************************************************************************


	void cam1Picture_keyAlgorithm(string imgPath);
	double cam1Picture_holeAlgorithm(string imgPath);


	//数值补偿函数********************************************************************************************************************************************************************************************************


	///测试函数************************************************************************************************************************************************************************************************************


signals:
	void partsNumber(QString partsNumber);//返回当前轴的零件号
	void updateDeviceInf(QString deviceInf);//更新设备信息
	void imgInf(const Mat* image, QString source, int displayMode);//图像显示信号
	void lsResult(int position, float result);//光幕传感器结果显示信号
	void programProcess(QString processInf, int Precentage);//显示测量进程信息和进度条
	void Finished(bool normalFlag);//program结束信号
	void programTips(QString programInf, int mode);//设备到达关位置，显示提示信息,同时根据不同的mode值对主界面按钮进行相关控制
	void measureStatistics(QString result, int ngFeatureNum, int measureNum, float currentYield);
	void fixtureTips(int programNum);//机心夹安装示意信号
};






