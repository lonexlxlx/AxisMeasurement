///运动控制卡对象///
#pragma once
#include <QtWidgets/QMainWindow>
#include <iostream>
#include <map>
#include <string>

#include "gts.h"//以下三个是运动控制的头文件
#include "ringnet.h"
#include "config.h"

using namespace std;

class moveControl : public QMainWindow
{
    Q_OBJECT

public:
    moveControl();
    ~moveControl();

    //全局自定义参数
 
    map <short, string> errorMap ;
    map <long,  string> moveModeMap ;
    short sRtn;//伺服控制返回代码;
    short currentAxisNumber;
    short currentAxisIndex;
    short lastAxisIndex;
    bool openControllerFlag = false;
    string errorOut;
    

    ///轴自定义参数////

    //轴状态显示相关
    short bFlagAlarm[8] = { false,false,false,false,false ,false,false,false }; // 伺服报警标志
    short bFlagMError[8] = { false,false,false,false,false ,false,false,false }; // 跟随误差越限标志
    short bFlagPosLimit[8] = { false,false,false,false,false ,false,false,false }; // 正限位触发标志
    short bFlagNegLimit[8] = { false,false,false,false,false ,false,false,false }; // 负限位触发标志
    short bFlagSmoothStop[8] = { false,false,false,false,false ,false,false,false };// 平滑停止标志
    short bFlagAbruptStop[8] = { false,false,false,false,false ,false,false,false }; // 急停标志
    short bFlagServoOn[8] = { false,false,false,false,false ,false,false,false }; // 伺服使能标志
    short bFlagMotion[8] = { false,false,false,false,false ,false,false,false }; // 规划器运动标志
    bool bFlagArrive[8] = { false,false,false,false,false ,false,false,false }; // 运动到位标志
    double dPrfPos[8]; // 规划位置
    double dPrfVel[8]; // 规划速度
    double dPrfAcc[8]; // 规划加速度
    double dEncodePos[8];//实际位置（编码器位置）
    double dEncodeVel[8];//实际速度（编码器速度）
    double dEncodeAcc[8];//实际加速度（编码器加速度）
    double dEncodePos_org[8];//实际位置-1（编码器位置）
    double dEncodeVel_org[8];//实际速度-1（编码器速度）
    long PrfMode[8]; // 运动模式(代码)
    string currentMoveMode[8] = { "","","","","","","","" };//运动模式(文本)
    long axisStatus[8]; // 轴状态

    //轴移动相关

    short axisCore[8] = { 1,1,1,1,1,1,1,1 };//轴所属的core序号
    //手动模式下各轴Trap参数
    TTrapPrm trapManual[8] ;//手动模式下各轴点位（trap）运动参数
    double trapVelManual[8] = { 30,30,30,30,30,30,30,30 };//手动模式下trap各轴的速度
    long trapStepLengthManual[8] = {2000,2000,2000,2000,2000,2000,2000,2000 };//手动模式下trap各轴步长
    //自动模式下各轴Trap参数
    TTrapPrm trapAuto[8] ;
    double trapHighVelAuto[8] = { 70,70,30,30,70,70,60,30 };
    double trapLowVelAuto[8] = { 3,3,1,1,3,1,1,1 };
    //自动模式下的步长在对应的程序中设置long trapStepLengthAuto[8] = { 2000,2000,2000,2000,2000,2000,2000,2000 };
    //搜索index各轴Trap参数
    TTrapPrm trapIndexSearch[8];
    double trapIndexSearchVel[8] = { 60,60,20,20,50,20,20,20 };
    long trapIndexSearchStepLength[8] = { -138500,-128000,2000,2000,130000,2000,2000,2000 };
    
    
    //回index各轴Trap参数
    TTrapPrm trapMoveToIndex[8];
    long trapIndexStepLength[8] = { -140000,-129000,2000,2000,132000,2000,2000,2000 };
    double trapMoveToIndexVel[8] = { 3,3,3,3,3,3,3,3 };
    TJogPrm jog[8];//各轴jog参数
    double jogVel[8] = {30,30,30,30,30,40,30,30 };//jog模式下各轴的速度
    THomePrm  tHomePrm[8];//各轴gohome对应的参数
    THomeStatus tHomeSts[8];//各轴goHome状态信息
    TTriggerEx trigger[8];//各轴高速采集对应参数
    TTriggerStatusEx triggerSts[8];//各轴高速采集对应的状态
    long indexPos[8] = {0,0,0,0,0,0,0,0};//各轴高速捕获到的index值（换算值）
    double pDecSmoothStop[8] = {2,2,2,2,2,2,2,2 };//平滑停止减速度
    double pDecAbruptStop[8] = {80,80,80,80,80,80,80,80 };//紧急停止的减速度
    long pBand[8] = {80,80,800,800,200,800,800,800 };//轴到位误差带;
    long pTime[8] = {500,500,100,100,500,100,100,100 };//轴到位时间;


    //运动控制器设定函数

    void openAxisController();//打开运动控制卡
    void closeAxisController();//GTN_Close()关闭运动控制卡
    void clearStatus();//清除当前轴状态
    void moveEnable();//开启当前轴使能
    void moveUnable();//关闭当前轴使能
    void zeroPosition();//当前轴规划位置&实际位置清零
    void setMoveMode(string newMode);//设置轴的运动模式
    void startJogMove(string jogDirection);//启动当前轴jog运动
    void stopMove(string stopMode, string target);
    void setTrapPrm(short axisCoreNumber, short axisNumber, TTrapPrm newTrapPrm, long newStepLength, double newTrapVel);
    void startTrap();//启动当前轴trap运动
    void setArrivePrm();//设置所有轴的运动到位参数
    void setStopPrm();//设置所有轴的停止参数
    void setToLimitPrm();//设置所有轴回限位参数
    void getToLimitStatus();//获取回限位的状态
    void goToLimit();//当前轴回限位（仅触发，不监测）
    void setTriggerPrm();//对所有轴的高速捕获参数设置
    void startTrigge();//当前轴开启高速捕获
    void getTriggeStatus();//获取当前轴捕获状态
    void updateAxisStatus(short axisNumber);//更新轴状态信息
    void setCurrentAxis(int axisNumber);//设置当前运动轴号

    //自定义函数
    void commandhandler(string command, short error);//控制台输出错误代码，和错误原因  
    int axisCheck();//检查各轴的运动状态,返回异常轴号
    
    void testFun();
signals:
    void moveControlError(QString inf);//轴运动错误信息

};
