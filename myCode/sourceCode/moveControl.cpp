#include "moveControl.h"
#include "moveControl.h"

moveControl::moveControl()
{
    //初始化map,使用列表初始化map失败，只能这样逐个进行
    errorMap[0] = "指令执行成功";
    errorMap[1] = "指令执行错误";
    errorMap[2] = "license不支持 ";
    errorMap[7] = "指令参数错误";
    errorMap[-1] = "主机和运动控制器通讯失败 ";
    errorMap[-6] = "打开控制器失败 ";
    errorMap[-7] = "运动控制器没有响应";
    errorMap[-10] = "主机和运动控制器通讯失败 ";
    errorMap[-11] = "动态库加载失败 ";
    errorMap[-13] = "编码器初始化失败1";
    errorMap[-14] = "编码器初始化失败2";
    errorMap[-15] = "动态库版本不匹配1";
    errorMap[15] = "动态库版本不匹配2";
    errorMap[16] = "不具备版本匹配功能";
    errorMap[-131] = "网络初始化失败 ";
    errorMap[-133] = "用户设置网络模块配置信息和实际所接网络模块不一致";

    moveModeMap[0] = "Trap";
    moveModeMap[1] = "Jog";
    moveModeMap[2] = "PT";
    moveModeMap[3] = "Gear";
    moveModeMap[4] = "Follow";
    moveModeMap[5] = "Interpolation";
    moveModeMap[6] = "PVT";

    //初始化自定义参数
    sRtn = 0;
    errorOut = "";
    
    openControllerFlag = false;
    for (int i = 0; i < 8; i++)
    {
        //jog相关参数初始化
        jogVel[i] = 60;
        jog[i].acc = 1;
        jog[i].dec = 1;
        jog[i].smooth = 0.5;

        //trap相关参数初始化
        trapMoveToIndex[i].acc = trapIndexSearch[i].acc = trapAuto[i].acc = trapManual[i].acc = 1;
        trapMoveToIndex[i].dec = trapIndexSearch[i].dec = trapAuto[i].dec = trapManual[i].dec = 1;
        trapMoveToIndex[i].smoothTime = trapIndexSearch[i].smoothTime = trapAuto[i].smoothTime = trapManual[i].smoothTime = 30;
    };
    //初始化选择轴号
    currentAxisNumber = 1;
    currentAxisIndex = 0;

};

moveControl::~moveControl()
{};
void moveControl::clearStatus()
{
    //cout << "clearStatus" << endl;
    sRtn = GTN_ClrSts(axisCore[currentAxisIndex], currentAxisNumber, 8);//最后一个参数默认为1但是参考程序写的是8，可能需要修改
    commandhandler("GTN_ClrSts", sRtn);
};

void moveControl::setMoveMode(string newMode)
{
    //设置当前轴的运动模式Jog or Trap,输入参数为对应字符串

    //cout << "setMoveMode" << endl;
    if (newMode == "Jog")
    {
        sRtn = GTN_PrfJog(axisCore[currentAxisIndex], currentAxisNumber);
        commandhandler("GTN_PrfJog", sRtn);
        currentMoveMode[currentAxisIndex] = "Jog";
    };
    if (newMode == "Trap")
    {
        sRtn = GTN_PrfTrap(axisCore[currentAxisIndex], currentAxisNumber);//设置为trapmode
        commandhandler("GTN_PrfTrap", sRtn);
        currentMoveMode[currentAxisIndex] = "Trap";
    };  
};
void moveControl::moveEnable()
{
    //当前轴使能
    //cout << "moveEnable" << endl;
    sRtn = GTN_AxisOn(axisCore[currentAxisIndex], currentAxisNumber);
    commandhandler("GTN_AxisOn", sRtn);
};

void moveControl::zeroPosition()
{
    //将当前轴的实际位置和规划位置清零

    //cout << "zeroPosition" << endl;
    sRtn = GTN_ZeroPos(axisCore[currentAxisIndex], currentAxisNumber);//函数的功能是规划位置和实际位置清零
    commandhandler("GTN_ZeroPos", sRtn);
    //将规划位置清零（疑问点既然已经清0了为什么又要重新设置规划位置为0）
    sRtn = GTN_SetPrfPos(axisCore[currentAxisIndex], currentAxisNumber, 0);
    commandhandler("GTN_SetPrfPos", sRtn);
};

void moveControl::startJogMove(string jogDirection)
{
    //当前轴开始jog运动，参数为移动的方向(forward/backward)
    //cout << "startJog 运动方向为" << jogDirection << endl;
    sRtn = GTN_SetJogPrm(axisCore[currentAxisIndex], currentAxisNumber, &jog[currentAxisIndex]);
    commandhandler("GTN_SetJogPrm", sRtn);
    if (jogDirection == "forward")
    {
        sRtn = GTN_SetVel(axisCore[currentAxisIndex], currentAxisNumber, jogVel[currentAxisIndex]);
    }
    else
    {
        sRtn = GTN_SetVel(axisCore[currentAxisIndex], currentAxisNumber, -jogVel[currentAxisIndex]);
    };
    commandhandler("GTN_SetVel", sRtn);
    sRtn = GTN_Update(axisCore[currentAxisIndex], 1 << (currentAxisIndex));
    commandhandler("GTN_Update", sRtn);
};

void moveControl::stopMove(string stopMode, string target)
{
    //使得轴停止 第一个参数为停止模式"urgent"/"smooth",第二个参数为需要停止的目标"currentAxis"/"all",其中当对所有轴都停止的时候回关闭伺服使能
    //cout << "stopMove!" << " stopMode:" << stopMode << " stopTarget:" << target << endl;
    if (target == "all")
    {
        if (stopMode == "urgent")
        {
            //所有轴急停
            sRtn = GTN_Stop(axisCore[currentAxisIndex], 0xffff, 0xffff);
        }
        else
        {
            //所有轴平滑停止
            sRtn = GTN_Stop(axisCore[currentAxisIndex], 0xffff, 0);
        }
        for (int i = 0; i < 8; i++)
        {
            sRtn = GTN_AxisOff(axisCore[i], i + 1);
        };
    }
    else
    {
        if (stopMode == "urgent")
        {
            //当前轴急停
            sRtn = GTN_Stop(axisCore[currentAxisIndex], 1 << currentAxisIndex, 0xffff);
        }
        else
        {
            //当前轴平滑停止
            sRtn = GTN_Stop(axisCore[currentAxisIndex], 1 << currentAxisIndex, 0);
        };
    };
    commandhandler("GTN_Stop", sRtn);
};

void moveControl::setTrapPrm(short axisCoreNumber, short axisNumber, TTrapPrm newTrapPrm, long newStepLength, double newTrapVel)
{
    //cout << "setTrapPrm" << endl;
    short axisIndex = axisNumber - 1;
    //设置轴trap运动模式的参数
    sRtn = GTN_SetTrapPrm(axisCoreNumber, axisNumber, &newTrapPrm);
    commandhandler("GTN_SetTrapPrm", sRtn);
    // 设置轴的目标位置(步长)
    sRtn = GTN_SetPos(axisCoreNumber, axisNumber, newStepLength);
    commandhandler("GTN_SetPos", sRtn);
    // 设置AXIS轴的目标速度
    sRtn = GTN_SetVel(axisCoreNumber, axisNumber, newTrapVel);
    commandhandler("GTN_SetVel", sRtn);
};

void moveControl::startTrap()
{
    //当前轴开始trap运动

    //cout << "startTrap" << endl; 
    //设置轴trap运动模式的参数
    //setTrapPrm(axisCore[currentAxisIndex], currentAxisNumber, trap[currentAxisIndex], trapStepLength[currentAxisIndex], trapVel[currentAxisIndex]);
    // 启动AXIS轴的运动
    sRtn = GTN_Update(axisCore[currentAxisIndex], 1 << currentAxisIndex);//<<是左移运算符，从右边至左分别代表1-32轴，对应位1有效
    commandhandler("GTN_Update", sRtn);
};

void moveControl::openAxisController()
{
    //打开运动控制卡，并清楚各轴的伺服报警

    cout << "openAxisController" << endl;
    sRtn = GTN_Open();
    openControllerFlag = false;
    
    
    if (sRtn == 0)
    {
        openControllerFlag = true;
        cout << "open" << sRtn << endl;
    }
    
    else
    {
        commandhandler("GTN_Open", sRtn);//该函数已经包括弹出错误执行提示了，下面行就不用了
        emit moveControlError("运动控制卡打开失败，请检查连接！");
        return;
    };
    
    
    sRtn = GTN_Reset(1);//第一个参数是核号
    if (sRtn != 0)
    {
        commandhandler("GTN_Reset", sRtn);
        cout << "aaaaaa1" << endl;
        return;
    };
    string str = "test1.cfg";
    char* str1 = const_cast<char*>(str.c_str());
    sRtn = GTN_LoadConfig(1, str1);
    if (sRtn != 0)
    {
        commandhandler("GTN_LoadConfig", sRtn);
        return;
    };
    
    for (int i = 0; i < 8; ++i)
    {
        sRtn = GTN_ClrSts(1, i + 1, 8);
        commandhandler("GTN_ClrSts", sRtn);
        sRtn = GTN_AlarmOff(1, i + 1);
        commandhandler("GTN_Alarmoff", sRtn);
    };
};
void moveControl::closeAxisController()
{
    sRtn = GTN_Close();
    //cout <<"控制卡" << sRtn << endl;
    if (sRtn)
    {
        commandhandler("GTN_Close", sRtn);
    }
    else
    {
        openControllerFlag = false;
    };
   
};
void moveControl::setToLimitPrm()
{
    //cout << "setToLimitPrm" << endl;
    for (int i = 0; i < 8; i++)
    {
        //对全部参数进行设置
        sRtn = GTN_GetHomePrm(axisCore[i], i+1, &tHomePrm[i]);
        tHomePrm[i].mode = 10;//11使用限位+home回原；10限位回原点
        tHomePrm[i].moveDir = -1;//-1是回负限位；1是回正限位
        tHomePrm[i].indexDir = 1;
        tHomePrm[i].edge = 0;
        tHomePrm[i].velHigh = 50;
        tHomePrm[i].velLow = 40;
        tHomePrm[i].acc = 0.1;
        tHomePrm[i].dec = 0.1;
        tHomePrm[i].searchHomeDistance = 200000;
        tHomePrm[i].searchIndexDistance = 30000;
        tHomePrm[i].escapeStep = 1000;
    };
    //对部分参数进行调整
    tHomePrm[0].moveDir = tHomePrm[1].moveDir = 1;//1和2轴修改为回负限位
    tHomePrm[4].velHigh = 50;//修改5轴的回限位高速
};
void moveControl::getToLimitStatus()
{
    sRtn = GTN_GetHomeStatus(axisCore[currentAxisIndex], currentAxisNumber, &tHomeSts[currentAxisIndex]);//获取回原点状态。.run1是正在回原;0回原完成
    commandhandler("GTN_GetHomeStatus", sRtn);
};
void moveControl::goToLimit()
{
    sRtn = GTN_GoHome(axisCore[currentAxisIndex], currentAxisNumber, &tHomePrm[currentAxisIndex]);//启动Smart Home回原点 
    //cout << "goToLimit"<<currentAxisNumber << currentAxisIndex << endl;
    commandhandler("GTN_GoHome", sRtn);

};
void moveControl::setTriggerPrm()
{
    //对所有轴的高速捕获参数设置
    for (int i = 0; i < 8; i++)
    {
        sRtn = GTN_GetTriggerEx(axisCore[i], i+1, &trigger[i]);//读取捕获参数，第三个参数详见224页
        trigger[i].probeType = CAPTURE_INDEX;
    };
};
void moveControl::startTrigge()
{
    //cout << "startTrigge" << endl;
    //当前轴开启高速捕获
    sRtn = GTN_SetTriggerEx(axisCore[currentAxisIndex], currentAxisNumber, &trigger[currentAxisIndex]);//设置捕获参数并启动捕获
    commandhandler("GTN_SetTriggerEx", sRtn);
};
void moveControl::getTriggeStatus()
{
    //获取当前轴捕获状态
    sRtn = GTN_GetTriggerStatusEx(axisCore[currentAxisIndex], currentAxisNumber, &triggerSts[currentAxisIndex]);
    commandhandler("GTN_GetTriggerStatusEx", sRtn);
    if (triggerSts[currentAxisIndex].done)
    {
        if (currentAxisNumber == 1 || currentAxisNumber == 2)//对各轴对应的捕获位置进行换算
        {
            indexPos[currentAxisIndex] = triggerSts[currentAxisIndex].position / 4;
        }
        else if (currentAxisNumber == 5)
        {
            indexPos[currentAxisIndex] = triggerSts[currentAxisIndex].position / 10;
        };
        
    }
    else
    {
        
    };

    //常用参数说明triggerSts[currentAxisIndex].done 是bool类型表示完成捕获; triggerSts.position是是捕获的Index值（原始值）
   
};
void moveControl::setStopPrm()
{
    //对所有轴的停止参数进行设置

    //cout << "setStopPrm" << endl;
    for (int i = 0; i < 8; i++)
    {
        sRtn = GTN_SetStopDec(axisCore[i], i + 1, pDecSmoothStop[i], pDecAbruptStop[i]);
        commandhandler("GTN_SetStopDec", sRtn);
        //用于测试是否成功设置了轴停止参数
        /*
        sRtn = GTN_GetStopDec(axisCore[i], i+1, &pDecSmoothStop[i], &pDecAbruptStop[i]);
        commandhandler("GetStopDec", sRtn);
        cout << "轴-"<<i+1 << "  新设置的停止参数" << "平滑停止的减速度=" << (pDecSmoothStop) << "    紧急停止的减速度=" << (pDecAbruptStop) << endl;
        */ 
    };
};
void moveControl::setArrivePrm()
{
    //对所有轴的到位参数进行设置
    //cout << "setArrivePrm" << endl;
    for (int i = 0; i < 8; i++)
    {
        sRtn = GTN_SetAxisBand(axisCore[i], i+1, pBand[i], pTime[i]);
        commandhandler("SetAxisBand", sRtn);
        //用于测试是否成功设置了相关到位参数
        /*
        sRtn = GTN_GetAxisBand(axisCore[i], i+1, &pBand[i], &pTime[i]);
        commandhandler("GetAxisBand", sRtn);
        cout <<"轴-"<<i+1 << "新设置的到位误差   " << "到位误差带大小= " << pBand << "    到位误差带保持时间= " << pTime << endl;
        */
    };
};

void moveControl::moveUnable()
{
    //关闭当前轴使能

    //cout << "moveUnable" << endl;
    sRtn = GTN_AxisOff(axisCore[currentAxisIndex], currentAxisNumber);
    commandhandler("GTN_AxisOff", sRtn);
};



void moveControl::updateAxisStatus(short axisNumber)
{
    //更新当前轴的信息

    //更新轴的bool信息
    //cout << "updateAxisStatus-Axis:" << axisNumber << endl;
    short axisIndex = axisNumber - 1;
    sRtn = GTN_GetSts(axisCore[axisIndex], axisNumber, &axisStatus[axisIndex]);
    commandhandler("GTN_GetSts", sRtn);
    // 伺服报警标志
    if (axisStatus[axisIndex] & 0x2)
    {
        bFlagAlarm[axisIndex] = true;
    }
    else
    {
        bFlagAlarm[axisIndex] = false;
    };
    // 跟随误差越限标志(对应ui面板中的运动出错,跟随误差大于0时输出true,对应面板显示为红色)
    if (axisStatus[axisIndex] & 0x10)
    {
        bFlagMError[axisIndex] = true;
    }
    else
    {
        bFlagMError[axisIndex] = false;
    };
    // 正向限位
    if (axisStatus[axisIndex] & 0x20)
    {
        bFlagPosLimit[axisIndex] = true;
    }
    else
    {
        bFlagPosLimit[axisIndex] = false;
    };
    // 负向限位
    if (axisStatus[axisIndex] & 0x40)
    {
        bFlagNegLimit[axisIndex] = true;
    }
    else
    {
        bFlagNegLimit[axisIndex] = false;
    };
    // I/O平滑停止
    if (axisStatus[axisIndex] & 0x80)
    {
        bFlagSmoothStop[axisIndex] = true;
    }
    else
    {
        bFlagSmoothStop[axisIndex] = false;
    };
    // I/O急停标志
    if (axisStatus[axisIndex] & 0x100)
    {
        bFlagAbruptStop[axisIndex] = true;
    }
    else
    {
        bFlagAbruptStop[axisIndex] = false;
    };
    // 伺服使能标志
    if (axisStatus[axisIndex] & 0x200)
    {
        bFlagServoOn[axisIndex] = true;
    }
    else
    {
        bFlagServoOn[axisIndex] = false;
    };
    // 规划器正在运动标志
    if (axisStatus[axisIndex] & 0x400)
    {
        bFlagMotion[axisIndex] = true;
    }
    else
    {
        bFlagMotion[axisIndex] = false;
    };
    //到位标志（当规划误差和实际误差小于设定误差带时，并且在误差带保持一段时间后置起运动到位标志）
    if (axisStatus[axisIndex] & 0x800)
    {
        bFlagArrive[axisIndex] = true;
    }
    else
    {
        bFlagArrive[axisIndex] = false;
    };

    //更新轴的文本信息
 
    //获取规划位置（脉冲计数器）
    sRtn = GTN_GetPrfPos(axisCore[axisIndex], axisNumber, &dPrfPos[axisIndex]);
    commandhandler("GTN_GetPrfPos", sRtn);
    //获取规划速度
    sRtn = GTN_GetPrfVel(axisCore[axisIndex], axisNumber, &dPrfVel[axisIndex]);
    commandhandler("GTN_GetPrfVel", sRtn);
    //获取规划加速度
    sRtn = GTN_GetPrfAcc(axisCore[axisIndex], axisNumber, &dPrfAcc[axisIndex]);
    commandhandler("GTN_GetPrfAcc", sRtn);
    //读取实际位置（编码器的位置）
    sRtn = GTN_GetAxisEncPos(axisCore[axisIndex], axisNumber, &dEncodePos[axisIndex]);
    commandhandler("GetAxisEncPos", sRtn);
    //读取实际速度
    sRtn = GTN_GetAxisEncVel(axisCore[axisIndex], axisNumber, &dEncodeVel[axisIndex]);
    commandhandler("GetAxisEncVel", sRtn);
    //读取实际加速度
    sRtn = GTN_GetAxisEncAcc(axisCore[axisIndex], axisNumber, &dEncodeAcc[axisIndex]);
    commandhandler("GetAxisEncPos", sRtn);
    // 读取运动模式
    sRtn = GTN_GetPrfMode(axisCore[axisIndex], axisNumber, &PrfMode[axisIndex]);
    currentMoveMode[currentAxisIndex] = moveModeMap[PrfMode[axisIndex]];
    commandhandler("GTN_GetPrfMode", sRtn);
    /////测试部分////测试两种读取编码器函数的区别（和上面的实际位置获取函数对比）
    
    //读取实际位置_org
    sRtn = GTN_GetEncPos(axisCore[axisIndex], axisNumber, &dEncodePos_org[axisIndex]);
    commandhandler("GetEncPos_1", sRtn);
    
    // 读取实际速度_org
    sRtn = GTN_GetEncVel(axisCore[axisIndex], axisNumber, &dEncodeVel_org[axisIndex]);
    commandhandler("GetEncVel_1", sRtn);
    
};

void moveControl::commandhandler(string command, short error)
{
    //如果指令执行错误，控制台输出错误代码以及错误原因

    if (error)
    {     
        errorOut = command+"error!, error code: "+to_string(error)+" error reason : "+errorMap[error];
        //cout << errorOut << endl;
        emit moveControlError(QString::fromStdString(errorOut));
    }
};

/*
函数作用：检查各轴状态、返回状态异常轴的编号
返回值说明:0（所有轴状态正常）；1（粗糙度相机轴异常）；2（孔径相机轴异常）；5（光幕轴异常）；6（顶尖轴）；7（旋转轴）
*/
int moveControl::axisCheck()
{
    
    int axisNum[5] = { 1,2,5,6,7 };
    for (int i = 0; i < 5; i++)
    {
        int currentAxisNum = axisNum[i];
        setCurrentAxis(currentAxisNum);
        moveEnable();
        updateAxisStatus(currentAxisNum);
        int index = currentAxisNum - 1;
       //cout << bFlagAlarm[index] << bFlagMError[index] << bFlagPosLimit[index] << bFlagNegLimit[index] << bFlagAbruptStop[index] << bFlagServoOn[index] << endl;
        bool axisNormalFlag =(bFlagAlarm[index] | bFlagMError[index] | bFlagPosLimit[index]
            | bFlagNegLimit[index] | bFlagAbruptStop[index] | !bFlagServoOn[index]);
        if (axisNormalFlag)
        {
            return axisNum[i];
        };
    }
    return 0;
};

void moveControl::setCurrentAxis(int axisNumber)
{
    //设置当前轴号，同时计算相应的索引号
    lastAxisIndex = currentAxisIndex;
    currentAxisNumber = axisNumber;
    currentAxisIndex = axisNumber-1;
};
void moveControl::testFun()
{
    cout << "testFun" << endl;
};
