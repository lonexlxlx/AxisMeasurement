#pragma once

#include <QMainWindow>
#include <functional>
#include <QMessageBox>
#include <QThread>
#include <iostream>
#include <ActiveQt/QAxObject>
#include <QDataWidgetMapper>
#include <QFileDialog>
#include "ui_sdk_assist.h"

#include <ctime>
#include <string>
#include <sstream>
#include <direct.h>
#include "Windows.h"

using namespace std;
class GraphicalProgramEditor;
class sdk_assist:public QMainWindow
{
    Q_OBJECT

public:
    sdk_assist(QWidget *parent = nullptr);
    std::function<QString()> graphicalEntryError;
	~sdk_assist();

    //全局变量初始化***************************************************************************************************************************
   
    int recordDiameterNb_all=0;
    int recordCylindricityNb_all=0;
    int recordRoughnessNb_all=0;
    int recordRoundoutNb_all=0;
    int recordTelecentricNb_all=0;
    int recordHoleNb_all = 0;
    QString recordpartNb="-";
    QString recordpartName="-";
    QString recordpartProcessingNb="-";
    QString recordpartNote="-";
 
    //直径点位初始化
    struct  diameterPositionInf {
        int  diameterFeatureNb=0;
        float   diameterNominalValue=0;
        float   diameterUpperOffset=0;
        float    diameterBottomOffset=0;
        long int axisGuangMuEncodePostion=0;
        float  axisGuangMuRealPostion=0;
        QString  diameterPostionNote="-";
        
    };
    diameterPositionInf m_diameterPositionInf[100];
    vector <int> recordDiameterList;
    int currentDiameterOrder=0;
  

    //粗糙度点位初始化
    struct  roughnessPositionInf {
        int  roughnessFeatureNb = 0;
        float   roughnessNominalValue = 0;
        int  roughnessExposeTime = 550;
        long int axisGuangMuEncodePostion=0;
        float  axisGuangMuRealPostion=0;
        long int axisRoughnessEncodePostion=0;
        float  axisRoughnessRealPostion=0;
        float roughnessReferenceD = 0;
        long int roughnessReferenceDEncodePostion = 0;//改动
        float roughnessReferenceDRealPostion = 0;//改动
        QString  roughnessPostionNote = "-";
    };
    roughnessPositionInf m_roughnessPositionInf[30];
    vector <int> recordRoughnessList;
    int currentRoughnessOrder = 0;

    //圆柱度点位信息初始化
    struct  cylindricityPositionInf {
        int  cylindricityFeatureNb = 0;
        float   cylindricityNominalValue = 0;
        long int axisGuangMuEncodePostion_bottom = 0;
        float  axisGuangMuRealPostion_bottom = 0;
        long int axisGuangMuEncodePostion_middle = 0;
        float  axisGuangMuRealPostion_middle = 0;
        long int axisGuangMuEncodePostion_upper = 0;
        float  axisGuangMuRealPostion_upper = 0;
        QString  cylindricityPostionNote = "-";
    };
    cylindricityPositionInf m_cylindricityPositionInf[100];
    vector <int> recordCylindricityList;
    int currentCylindricityOrder = 0;
    long int cylindricityUpperRelativeLocation_current=0;
    long int cylindricityBottomRelativeLocation_current=0;

    //跳动点位信息初始化
    struct  roudoutPositionInf {
        int  roundoutFeatureNb = 0;
        float   roundoutNominalValue = 0;
        long int axisGuangMuEncodePostion_bottom = 0;
        float  axisGuangMuRealPostion_bottom = 0;
        long int axisGuangMuEncodePostion_middle = 0;
        float  axisGuangMuRealPostion_middle = 0;
        long int axisGuangMuEncodePostion_upper = 0;
        float  axisGuangMuRealPostion_upper = 0;
        QString  roundoutPostionNote1 = "-";//改动
        QString  roundoutPostionNote2 = "-";//改动
    };
    roudoutPositionInf m_roundoutPositionInf[100];
    vector <int> recordRoundoutList;
    int currentRoundoutOrder = 0;
    long int roundoutUpperRelativeLocation_current=0;
    long int roundoutBottomRelativeLocation_current=0;

    //孔点位信息初始化
    struct  holePositionInf {
        int      holeFeatureNb = 0;
        float    holeNominalValue = 0;
        float    holeUpperOffset = 0;
        float    holeBottomOffset = 0;
        int      holeNumber = 0;
        int      holeExposeTime = 550;
        long int axisGuangMuEncodePostion = 0;
        float    axisGuangMuRealPostion = 0;
        long int axisHoleEncodePostion = 0;
        float    axisHoleRealPostion = 0;
        QString  holePostionNote = "-";
    };
    holePositionInf m_holePositionInf[30];
    vector <int> recordHoleList;
    int currentHoleOrder = 0;

    //远心点位信息初始化
    struct  telecentricPositionInf {
        int  telecentricExposeTime = 550;
        long int axisGuangMuEncodePostion = 0;
        float  axisGuangMuRealPostion = 0;
        QString  telecentricPostionNote = "-";
    };
    telecentricPositionInf m_telecentricPositionInf[30];
    vector <int> recordTelecentricList;
    int currentTelecentricOrder = 0;

    //保存函数
    QAxObject* workbook;// 当前工作簿指针
    QAxObject* worksheet;//活动工作表指针
    //槽函数定义*************************************************
private slots:
    
    //直径槽函数
    void on_diameterSequence_currentIndexChanged(int nIndex);
    void on_diameterPostionRecord_clicked();
    void on_diameterPostionClear_clicked();
    void updateDiameterPostionInf(int selectedPostion);
    void clearSingleDiameter(int index);

    //粗糙度槽函数
    void on_roughnessSequence_currentIndexChanged(int nIndex);
    void on_roughnessPostionRecord_clicked();
    void on_roughnessPostionClear_clicked();
    void on_roughnessReferenceDRecord_clicked();//改动
    void updateRoughnessPostionInf(int selectedPostion);
    void clearSingleRoughness(int index);
  

    //圆柱度槽函数
    void on_cylindricitySequence_currentIndexChanged(int nIndex);
    void on_cylindricityPostionRecord_clicked();
    void on_cylindricityPostionClear_clicked();
    void updateCylindricityPostionInf(int selectedPostion);
    void clearSingleCylindricity(int index);

    //跳动槽函数
    void on_roundoutSequence_currentIndexChanged(int nIndex);
    void on_roundoutPostionRecord_clicked();
    void on_roundoutPostionClear_clicked();
    void updateRoundoutPostionInf(int selectedPostion);
    void clearSingleRoundout(int index);

    //孔径槽函数
    void on_holeSequence_currentIndexChanged(int nIndex);
    void on_holePostionRecord_clicked();
    void on_holePostionClear_clicked();
    void updateHolePostionInf(int selectedPostion);
    void clearSingleHole(int index);

    //远心相机槽函数
    void on_telecentricSequence_currentIndexChanged(int nIndex);
    void on_telecentricPostionRecord_clicked();
    void on_telecentricPostionClear_clicked();
    void updateTelecentricPostionInf(int selectedPostion);
    void clearSingleTelecentric(int index);

    //点位信息输出
    void updatePostionInfOut();
    void on_PostionRecordOut_clicked();
    void  on_PostionClearOut_clicked();
    void on_recordpartNb_editingFinished();
    void on_recordpartName_editingFinished();
    void on_recordpartProcessingNb_editingFinished();
    void on_recordpartNote_editingFinished();
   
    bool mergeCells(QString start, QString end, QString value);
    void saveAsExcel();
 


private:
    Ui::sdk_assist ui;
    GraphicalProgramEditor* m_graphicalProgramEditor = nullptr;

signals:
    void graphicalEditorCreated(GraphicalProgramEditor* editor);
   
    void diameterPostionRecord();
    void roughnessPostionRecord();
    void roughnessReferenceDRecord();
    void cylindricityPostionRecord();
    void roundoutPostionRecord();
    void holePostionRecord();
    void telecentricPostionRecord();
    void tips(QString infDisplay);
 

};
