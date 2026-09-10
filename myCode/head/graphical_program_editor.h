#pragma once

#include <QMainWindow>
#include <QVector>
#include <QPainterPath>

class GraphicalCanvas;
class QLabel;
class QListWidget;
class QTableWidget;
class QDoubleSpinBox;
class QCheckBox;
class QPushButton;
class QComboBox;
class QLineEdit;
class QCloseEvent;

class GraphicalProgramEditor : public QMainWindow
{
public:
    explicit GraphicalProgramEditor(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void buildInterface();
    void openLocalImage();
    void refreshFeatureList();
    void refreshFeatureProperties(int featureId);
    void saveMeasurementRecord(bool update);//新增或者更新按钮公用这个函数。校验：特征号必填、下偏差<=上偏差
    void refreshMeasurementRecords();//把 m_records 刷到表格；同时检查关联图形有没有被删掉（删了就标"关联已删除"并清空试测结果）。
    void loadMeasurementRecord(int row);
    void cancelRelink();
    void trialSelectedRecord();
    void showRecordDetection(int row);
    bool m_trialRunning = false;
    int m_relinkSequence = -1;
    int m_relinkSlot = 1;

    struct MeasurementRecord {//每条测量记录保存为一个结构体
        int sequence = 0;//记录序号，自增
        int geometryId = -1;//关联的画布图形ID
        int secondaryGeometryId = -1;//第二图形ID
        QString featureNumber;//特征号
        QString type; //测量类型
        bool hasTolerance = false;//公差
        double nominal = 0;
        double lower = 0;
        double upper = 0;
        double pixelRadius = -1;//试测结果px
        double trialAngle = -1;//试测结果°
        bool useSupplementaryAngle = false;//是否取补角
        QString trialStatus = QStringLiteral("未执行");//检测边缘+拟合结果（画回画布）
        QPainterPath detectedEdges;
        QPainterPath fittedArc;
    };
    QVector<MeasurementRecord> m_records;
    int m_nextRecordSequence = 1;
    QComboBox* m_measurementType = nullptr;
    QComboBox* m_angleResultMode = nullptr;
    QLineEdit* m_featureNumber = nullptr;
    QCheckBox* m_hasTolerance = nullptr;
    QDoubleSpinBox* m_nominal = nullptr;
    QDoubleSpinBox* m_lowerDeviation = nullptr;
    QDoubleSpinBox* m_upperDeviation = nullptr;

    GraphicalCanvas* m_canvas = nullptr;
    QListWidget* m_featureList = nullptr;
    QTableWidget* m_stepTable = nullptr;
    QLabel* m_coordinateLabel = nullptr;
    QLabel* m_featureNameLabel = nullptr;
    QLabel* m_featureTypeLabel = nullptr;
    int m_selectedFeatureId = -1;
    QLabel* m_primarySizeLabel = nullptr;
    QDoubleSpinBox* m_primarySize = nullptr;
    QDoubleSpinBox* m_secondarySize = nullptr;
    QCheckBox* m_lockRatio = nullptr;
    QPushButton* m_applySize = nullptr;
    double m_sizeRatio = 1.0;
    QLabel* m_rotationLabel = nullptr;
    QDoubleSpinBox* m_rotationAngle = nullptr;
    QPushButton* m_applyRotation = nullptr;
};
