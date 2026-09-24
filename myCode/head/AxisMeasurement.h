#pragma once

#include <QtWidgets/QMainWindow>
#include <QThread>
#include <QMessageBox>
#include <QFileDialog>
#include <QTimer>
#include <QPainter>
#include <QPen>
#include <QPaintEvent>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QtSql>
#include <QSplitter>//P2-10：中央区分隔条
#include <QTabWidget>//P2-9：控制面板收纳页签
#include <QScrollArea>//布局修复：绝对定位面板的滚动兜底，防止内容被裁切
#include <QPropertyAnimation>//P2-12：进度条/警示边框动画
#include <QGraphicsOpacityEffect>//P2-12：警示边框闪烁效果
#include <QFrame>//P2-12：急停警示边框层
#include <QResizeEvent>//P2-10：窗口缩放事件

#include <iostream>

#include <HalconCpp.h>
#include <Halcon.h>
#include <opencv2/opencv.hpp>

#include "ui_AxisMeasurement.h"
#include "cam_device.h"
#include "camThread.h"
#include "moveControl.h"
#include "ls_device.h"
#include "lsThread.h"
#include "program_0.h"
#include "program_1.h"
#include "program_2.h"
#include "program_3.h"
#include "program_4.h"
#include "program_5.h"
#include "program_6.h"
#include "program_7.h"
#include "program_8.h"
#include "program_9.h"
#include "program_10.h"
#include "program_11.h"
#include "program_12.h"
#include "program_13.h"
#include "program_14.h"
#include "program_15.h"//需要按照上面格式增加对应的子程序声明
#include "program_16.h"
#include "program_17.h"
#include "program_18.h"
#include "program_19.h"
#include "program_20.h"
#include "program_21.h"
#include "program_22.h"
#include "program_23.h"
#include "program_24.h"
#include "program_25.h"
#include "program_26.h"
#include "program_27.h"
#include "program_28.h"
#include "program_29.h"
#include "program_30.h"
#include "program_31.h"
#include "program_32.h"
#include "program_33.h"
#include "program_34.h"
#include "program_35.h"
#include "program_36.h"
#include "program_37.h"
#include "program_38.h"
#include "program_39.h"
#include "program_40.h"
#include "program_41.h"
#include "program_42.h"
#include "program_43.h"
#include "program_44.h"
#include "program_45.h"
#include "program_46.h"
#include "program_47.h"
#include "program_48.h"
#include "program_49.h"
#include "program_50.h"
#include "axisGoHome_thread.h"
#include "moveThread.h"
#include "GalaxyIncludes.h"
#include "logIn.h"
#include "graphical_program_editor.h"
#include "roughnessFun.h"
#include"sharedFun.h"
//HalconCpp测试


using namespace cv;
using namespace HalconCpp;
//#include "LS9_IF.h"
//#include "LS9_ErrorCode.h"

using namespace std;

//文件删除测试
#include "sharedFun.h"

class AxisMeasurement : public QMainWindow
{
    Q_OBJECT

public:
    AxisMeasurement(QWidget *parent = nullptr);
    ~AxisMeasurement();
 
private:
    Ui::AxisMeasurementClass ui;
    
    //系统相关
    logIn* m_logIn;//登陆界面对象
    GraphicalProgramEditor* m_graphicalProgramEditor = nullptr;
    QDateTime dateTime;//系统时间
    QTimer * updateDateTimer;//用于更新系统时间的定时器

    //P1-8 状态栏分区：设备状态灯 + 提示信息（永久部件）
    QLabel* m_deviceStatusLabel = nullptr;//状态栏设备状态灯（绿点=已打开，红点=未打开）
    QLabel* m_statusInfoLabel = nullptr;//状态栏提示信息（同步显示 showDeviceInf 内容）
    void updateDeviceStatus(bool online);//刷新状态栏设备状态灯

    //主界面视觉角色与动态状态统一入口：只负责属性和 QSS 刷新，不参与业务判断
    void initializeVisualRoles();
    void refreshWidgetStyle(QWidget* widget);
    bool setVisualProperty(QWidget* widget, const char* propertyName, const char* propertyValue);
    void enableStartupMotionControls();

    bool motionControlReady() const;

    //P2-9/10 布局重构：12 个分组框收纳 + QSplitter 窗口自适应
    void restructureMainLayout();//把 .ui 的绝对定位布局重组为 Splitter + TabWidget（只动容器，不动控件本身）
    QSplitter* m_mainSplitter = nullptr;//中央区主分隔条：左侧页面区 | 右侧统计+结果

    //P2-11 关键数值仪表盘化：光幕实时数值/直径等用 20pt+ 等宽字体 + 语义色（在 restructureMainLayout 内设置）

    //P2-12 视觉反馈：进度条平滑动画 + 急停全窗口边框闪红
    void setProgramProgressSmooth(int value);//进度条平滑过渡（替代直接 setValue）
    void flashEmergencyBorder();//急停触发时全窗口红色边框闪烁 3 次
    QPropertyAnimation* m_progressAnim = nullptr;//进度条动画
    QFrame* m_alertFrame = nullptr;//急停警示边框层（覆盖全窗口，鼠标穿透）
    QGraphicsOpacityEffect* m_alertEffect = nullptr;//警示边框透明度效果
    QPropertyAnimation* m_flashAnim = nullptr;//警示边框闪烁动画
    void resizeEvent(QResizeEvent* event) override;//窗口缩放时保持警示边框覆盖全窗口

    //程序全局变量
    int currentProgram;
    int lastAxisIndex;//手动轴类控制上一次选择的轴号
    int currentAxisNumber; //手动轴类控制当前选择的轴号
    int currentAxisIndex = 0;//手动轴类控制当前选择的索引号
    bool programRunFlag;//自动程序测量运行标志
    QTableWidget* resultTablePtr;//主界面中的结果显示列表
    bool DbOpenFlag;//数据库打开标志
    QSqlDatabase Db;//程序连接的数据库
    QSqlDatabase* DbPtr;//程序连接的数据库指针
    int m_measurePartsNum_all;//检测的所有零件总数
    int m_okPartsNum_all;//检测的所有零件良品数
    int m_ngPartsNum_all;//检测的所有零件NG数
    float m_yield_all;//检测的所有零件合格率

    //设备相关变量
    string currentMeasureMode;//当前测量模式有AutoMeasureMode和ManualControlMode
    bool allDeviceOpenFlag;//所有设备正常打开标志
    cam_device cameraList[3];//三个相机对象
    cam_device* cameraPtrList[3];//三个相机对象对应的指针
    bool camCaptureFlag[3];//三个相机的手动采集状态
    int currentCamNum;//手动模式下选择的相机
    map <int, QString> axisMap = {{1,"测粗糙度轴"},{2,"测孔轴"},{5,"光幕轴"},{6,"上顶尖轴"},{5,"转台轴"}};
    moveControl* moveControlCardPtr;//运动控制卡对象指针
    ls_device* lsSensorPtr;//光幕传感器对象指针
    roughnessFun* roughnessFunPtr;//粗糙度计算对象指针

    //检测程序线程
    //需要按照下面格式追加子程序
    program_0* m_program0_Ptr;//program0线程指针（最长轴）
    QString partNumber_0;//检测程序0对应的轴型号

    program_1* m_program1_Ptr;//program1线程指针（标准台阶轴）
    QString partNumber_1;//检测程序1对应的轴型号
    
    program_2* m_program2_Ptr;//program2线程指针（镂空轴）
    QString partNumber_2;//检测程序2对应的轴型号

    program_3* m_program3_Ptr;//program3线程指针（中长轴）
    QString partNumber_3;//检测程序3对应的轴型号

    program_4* m_program4_Ptr;//program4线程指针（最短轴）
    QString partNumber_4;//检测程序1对应的轴型号

    //预留的程序指针**************************************************************************************************************************************************************************
    
    program_5* m_program5_Ptr;//program5线程指针
    QString partNumber_5;//program5对应的轴型号

    program_6* m_program6_Ptr;//program6线程指针
    QString partNumber_6;//program6对应的轴型号

    program_7* m_program7_Ptr;//program7线程指针
    QString partNumber_7;//program7对应的轴型号

    program_8* m_program8_Ptr;//program8线程指针
    QString partNumber_8;//program8对应的轴型号

    program_9* m_program9_Ptr;//program9线程指针
    QString partNumber_9;//program9对应的轴型号

    program_10* m_program10_Ptr;//program10线程指针
    QString partNumber_10;//program10对应的轴型号
    
    program_11* m_program11_Ptr;//program11线程指针
    QString partNumber_11;//program11对应的轴型号

    program_12* m_program12_Ptr;//program11线程指针
    QString partNumber_12;//program11对应的轴型号
    
    program_13* m_program13_Ptr;//program11线程指针
    QString partNumber_13;//program11对应的轴型号

    program_14* m_program14_Ptr;//program14线程指针
    QString partNumber_14;//program14对应的轴型号 XD-WZ10451080-370


    program_15* m_program15_Ptr;//program14线程指针
    QString partNumber_15;//program15对应的轴型号 WZ10451080-370-4

    program_16* m_program16_Ptr;//program14线程指针
    QString partNumber_16;//program16对应的轴型号 WZ10151107-160

    program_17* m_program17_Ptr;//program17线程指针_HJC
    QString partNumber_17;//program17对应的轴型号 WZ10151107

    program_18* m_program18_Ptr;//program17线程指针_HJC
    QString partNumber_18;//program17对应的轴型号 Z82292121270-60

    program_19* m_program19_Ptr;//program19线程指针
    QString partNumber_19;//program19对应的轴型号KF7153009-230

    program_20* m_program20_Ptr;//program20线程指针
    QString partNumber_20;//program20对应专用标准轴

    program_21* m_program21_Ptr;//program20线程指针
    QString partNumber_21;//program20对应专用标准轴

    program_22* m_program22_Ptr;//program20线程指针
    QString partNumber_22;//program20对应专用标准轴
    program_23* m_program23_Ptr;//program20线程指针
    QString partNumber_23;//program20对应专用标准轴

    program_24* m_program24_Ptr;//program20线程指针
    QString partNumber_24;//program20对应专用标准轴

    program_25* m_program25_Ptr;//program25线程指针
    QString partNumber_25;//program25对应专用标准轴

    program_26* m_program26_Ptr;//program25线程指针
    QString partNumber_26;//program25对应专用标准轴

    program_27* m_program27_Ptr;// 
    QString partNumber_27;// 

    program_28* m_program28_Ptr;// 
    QString partNumber_28;// 

    program_29* m_program29_Ptr;// 
    QString partNumber_29;// 

    program_30* m_program30_Ptr;// 
    QString partNumber_30;// 

    program_31* m_program31_Ptr;// 
    QString partNumber_31;// 

    program_32* m_program32_Ptr;// 
    QString partNumber_32;// 

    program_33* m_program33_Ptr;// 
    QString partNumber_33;// 

    program_34* m_program34_Ptr;// 
    QString partNumber_34;// 

    program_35* m_program35_Ptr;// 
    QString partNumber_35;// 

    program_36* m_program36_Ptr;// 
    QString partNumber_36;// 

    program_37* m_program37_Ptr;// 
    QString partNumber_37;// 

    program_38* m_program38_Ptr;// 
    QString partNumber_38;// 

    program_39* m_program39_Ptr;// 
    QString partNumber_39;// 

    program_40* m_program40_Ptr;// 
    QString partNumber_40;// 

    program_41* m_program41_Ptr;// 
    QString partNumber_41;// 

    program_42* m_program42_Ptr;// 
    QString partNumber_42;// 

    program_43* m_program43_Ptr;// 
    QString partNumber_43;// 

    program_44* m_program44_Ptr;// 
    QString partNumber_44;// 

    program_45* m_program45_Ptr;// 
    QString partNumber_45;// 

    program_46* m_program46_Ptr;// 
    QString partNumber_46;// 

    program_47* m_program47_Ptr;// 
    QString partNumber_47;// 

    program_48* m_program48_Ptr;// 
    QString partNumber_48;// 

    program_49* m_program49_Ptr;// 
    QString partNumber_49;// 

    program_50* m_program50_Ptr;// 
    QString partNumber_50;// 


    //预留的程序指针**************************************************************************************************************************************************************************

    
    
    axisGoHome_thread* goHomeThread_Ptr;//回原点线程指针
    moveThread* moveThreadList[8];//轴状态更新线程
    camThread* m_camThread_ptrList[3];//相机显示线程的指针
    lsThread* m_lsThread;//光幕传感器实时显示的线程
    QString currentOperatorName;//检测人员姓名
    QString currentPartsId;//检测零件id
    

    //测试变量
    cv::Mat* originalImgPtr;
  


private slots:

    //菜单栏槽函数 

    void on_autoMeasureMode_Triggered();
    void on_ManualControl_Triggered();
    void on_graphicalProgramEditor_Triggered();

    //点位记录接口已由图形化编辑器的设备点位页面直接管理
    float axis_compsation(long int encodePos);
    float axis1And2_caculation(long int encodePos);
    //自动测量模式中的槽函数

    void on_openAllDevice_clicked();
    void on_closeAllDevice_clicked();
    void on_programNumber_currentIndexChanged(int nIndex);
    void on_startAutoMearsurement_clicked();
    void on_urgrentStopMearsure_clicked();
    void on_allAxisGoHome_clicked();
    void on_programConfirm_clicked();
    void on_measureCancel_clicked();

    void on_apexMoveDown_pressed();
    void on_apexMoveUp_pressed();
    void on_partRotate_anticlockwise_pressed();
    void on_partRotate_clockwise_pressed();
    void on_lsMoveUp_pressed();
    void on_lsMoveDown_pressed();
    void on_apexMoveDown_released();
    void on_apexMoveUp_released();
    void on_partRotate_anticlockwise_released();
    void on_partRotate_clockwise_released();
    void on_lsMoveUp_released();
    void on_lsMoveDown_released();
    
    //测量数据展示操作区的槽函数creatDatabase

    void show_Statistics();
    void on_saveMeasureResult_clicked();
    void on_clearMeasureResult_clicked();
    void on_zeroMeasureNum_clicked();
    void openDatabase();//打开数据库
    void closeDatabase();//关闭数据库
    void on_creatDatabase_clicked();
    void on_operatorName_editingFinished();
    void on_partsId_editingFinished();
    void show_programStatistics(QString result, int ngFeatureNum, int measureNum, float currentYield);
    
  
    //轴手动控制模式中的槽函数

    void on_clearStatus_clicked();
    void on_jogMode_clicked();
    void on_moveEnable_clicked();
    void on_moveUnable_clicked();
    void on_smoothStop_clicked();
    void on_urgentStop_clicked();
    void on_trapMode_clicked();
    void on_zeroPosition_clicked();
    void on_jogBackwardMove_pressed();
    void on_jogBackwardMove_released();
    void on_jogForwardwardMove_pressed();
    void on_jogForwardwardMove_released();
    void on_startTrap_clicked();
    void on_goHome_clicked();
    
    void on_jogSpeed_editingFinished();
    void on_jogAcceleratedSpeed_editingFinished();
    void on_jogDecelerationSpeed_editingFinished();
    void on_jogSmooth_editingFinished();
    void on_trapSmoothTime_editingFinished();
    void on_stepLength_editingFinished();
    void on_trapAcceleratedSpeed_editingFinished();
    void on_trapDeclarationSpeed_editingFinished();
    void on_trapSpeed_editingFinished();

    void on_axisNumber_currentIndexChanged(int nIndex);
    void updateUiAxisStatus(short axisNumber);
    void updateTrapSettings(short axisNumber);
    void updateJogSettings(short axisNumber);
    void changeLedColor(QLabel* qlabelName, bool colour);

    //传感器控制模式中的槽函数

    void on_selectCamera_currentIndexChanged(int nIndex);
    void on_saveImg_clicked();
    void on_startCamCapture_clicked();
    void on_stopCamCapture_clicked();
    void on_exposeTime_editingFinished();
    void on_gain_editingFinished();
    void on_getLsResult_clicked();



    //提示槽函数

    void showTime();//显示当前时间日期
    void showDeviceErrorInf(QString errorInf); // 设备异常槽函数
    void showTips(QString tipsInf);//用于显示弹出消息对话框
    void showDeviceInf(QString deviceInf);//用于显示设备状态栏信息
    
  

    //线程槽函数

    void displayImg(const Mat* imgPrt, QString source, int drawMode);//图像显示槽函数
    void showPartNumber(QString partNumber);//显示检测程序号对应的零件编号
    void showLsResult(int lsPosition, float result);//显示LS传感器的点位测试结果
    void showCurrentLsValue();//显示LS传感器实时读数
    void showProgramProcess(QString processInf, int precentage);//显示线程执行过程,Precentage用于更新进度条
    void programCheck(QString infTip,int mode);//程序确认槽函数
    void programFinish(bool normalFlag);//自动检测程序结束槽函数
    void showFixturePicture(int programNum);//在主界面上显示拨叉安装示意图
    void showClampingPicture(int programNum);//在主界面上显示零件安装示意图
    void goHomeThreadFinish(int goHomeAxis,bool normalFlag);//回原线程结束槽函数
   
    
    
    //其他测试槽函数

    void on_roundoutDataCollection_clicked();//跳动数据获取
    void on_pushButton_clicked();//
    
protected:
    int paintMode;//绘图模式
    void paintEvent(QPaintEvent* event);//在窗口绘图
};
