#pragma once

#include <QMainWindow>
#include <QVector>
#include <QPainterPath>
#include <QImage>
#include <QSize>
#include <functional>
#include <cmath>
#include "graphical_corner_geometry.h"

struct GraphicalDetectionParameters {
    double smoothing = 2;
    double lowThreshold = 20;
    double highThreshold = 60;
    double minLength = 20;
    double maxLength = 0; // 0 preserves the legacy image-width / 2 limit.
    double mergeDistance = 90;
    double cornerMaxDeviation = 2.5;
    double cornerMaxGap = 20;
    QString validationError() const {
        if (!std::isfinite(smoothing) || smoothing < 0.1 || smoothing > 20)
            return QStringLiteral("Canny平滑参数须在0.1–20之间。");
        if (!std::isfinite(lowThreshold) || !std::isfinite(highThreshold)
            || lowThreshold < 0 || highThreshold > 65535 || lowThreshold >= highThreshold)
            return QStringLiteral("边缘阈值须满足0 ≤ 低阈值 < 高阈值 ≤ 65535。");
        if (!std::isfinite(minLength) || !std::isfinite(maxLength) || minLength < 1 || minLength > 1000000
            || maxLength < 0 || maxLength > 1000000 || (maxLength > 0 && minLength > maxLength))
            return QStringLiteral("轮廓长度须在允许范围内，且最短长度不能大于最长长度；最长为0表示图像宽度的一半。");
        if (!std::isfinite(mergeDistance) || mergeDistance < 0 || mergeDistance > 1000000)
            return QStringLiteral("轮廓合并距离须在0–1000000 px之间。");
        if (!std::isfinite(cornerMaxDeviation) || cornerMaxDeviation < 0.1 || cornerMaxDeviation > 20
            || !std::isfinite(cornerMaxGap) || cornerMaxGap < 0 || cornerMaxGap > 1000)
            return QStringLiteral("单ROI角度拟合偏差须为0.1–20 px，角点间隙须为0–1000 px。");
        return QString();
    }
};

class GraphicalCanvas;
class QLabel;
class QListWidget;
class QTableWidget;
class QDoubleSpinBox;
class QSpinBox;
class QCheckBox;
class QPushButton;
class QComboBox;
class QLineEdit;
class QCloseEvent;

class GraphicalProgramEditor : public QMainWindow
{
public:
    explicit GraphicalProgramEditor(QWidget* parent = nullptr);
    struct AxisSnapshot {
        bool connected = false;
        bool available = false;
        bool valid = false;
        long status = 0;
        double planned = 0;
        double encoder = 0;
        QString message;
    };
    enum class AxisCommand { JogNegative, JogPositive, MoveAbsolute, Enable, Disable, Stop, EmergencyStop };
    struct AxisCommandResult {
        QString error;
        bool motionMayHaveStarted;
        AxisCommandResult(QString message = QString(), bool started = false)
            : error(std::move(message)), motionMayHaveStarted(started) {}
    };
    using AxisReader = std::function<AxisSnapshot(int)>;
    using AxisCommander = std::function<AxisCommandResult(int, AxisCommand, double, long)>;
    void setAxisBackend(AxisReader reader, AxisCommander commander);
    struct CameraSnapshot {
        bool connected = false;
        bool available = false;
        bool capturing = false;
        bool hasFrame = false;
        int exposure = -1;
        QSize frameSize;
        QString message;
    };
    enum class CameraCommand { StartCapture, StopCapture, Snapshot };
    struct CameraCommandResult {
        QString error;
        QImage image;
        int exposure = -1;
    };
    using CameraReader = std::function<CameraSnapshot(int)>;
    using CameraCommander = std::function<CameraCommandResult(int, CameraCommand, int)>;
    void setCameraBackend(CameraReader reader, CameraCommander commander);
    struct LightCurtainSnapshot {
        bool connected = false;
        bool available = false;
        bool hasSample = false;
        double rawOut1 = 0;
        double compensatedDiameter = 0;
        qint64 sampledAtMs = 0;
        QString message;
    };
    using LightCurtainReader = std::function<LightCurtainSnapshot()>;
    void setLightCurtainBackend(LightCurtainReader reader);

protected:
    void closeEvent(QCloseEvent* event) override;
    bool event(QEvent* event) override;

private:
    void buildInterface();
    QWidget* buildAxisPanel();
    void refreshAxisPanel();
    void executeAxisCommand(AxisCommand command);
    bool stopOwnedAxis();
    AxisReader m_axisReader;
    AxisCommander m_axisCommander;
    QComboBox* m_axisSelector = nullptr;
    QComboBox* m_axisMode = nullptr;
    QDoubleSpinBox* m_axisSpeed = nullptr;
    QDoubleSpinBox* m_axisTarget = nullptr;
    QWidget* m_axisInputs = nullptr;
    QLabel* m_axisState = nullptr;
    QLabel* m_axisPosition = nullptr;
    QLabel* m_axisMessage = nullptr;
    QPushButton* m_jogNegative = nullptr;
    QPushButton* m_jogPositive = nullptr;
    QPushButton* m_moveAbsolute = nullptr;
    QPushButton* m_axisEnable = nullptr;
    QPushButton* m_axisDisable = nullptr;
    QPushButton* m_axisStop = nullptr;
    QPushButton* m_axisEmergency = nullptr;
    int m_ownedAxis = -1;
    bool m_axisStopRequested = false;
    qint64 m_axisStartedAt = 0;
    void refreshCameraPanel();
    void executeCameraCommand(CameraCommand command);
    bool stopOwnedCamera();
    void recordSelectedDevicePosition();
    void clearSelectedDevicePosition();
    void refreshDevicePositionPanel();
    CameraReader m_cameraReader;
    CameraCommander m_cameraCommander;
    LightCurtainReader m_lightCurtainReader;
    QComboBox* m_cameraSelector = nullptr;
    QSpinBox* m_cameraExposure = nullptr;
    QLabel* m_cameraState = nullptr;
    QPushButton* m_cameraStart = nullptr;
    QPushButton* m_cameraStop = nullptr;
    QPushButton* m_cameraLoad = nullptr;
    QLabel* m_devicePositionState = nullptr;
    QLabel* m_lightCurtainState = nullptr;
    QPushButton* m_recordDevicePosition = nullptr;
    QPushButton* m_clearDevicePosition = nullptr;
    int m_ownedCamera = -1;
    bool m_cameraExposureEdited = false;
    bool m_axisBackendAvailable = false;
    void openLocalImage();
    void openProject();
    void saveProject();
    void saveProjectAs();
    bool writeProject(const QString& filePath, QString& error);
    bool readProject(const QString& filePath, QString& error);
    void refreshFeatureList();
    void refreshFeatureProperties(int featureId);
    void saveMeasurementRecord(bool update);//新增或者更新按钮公用这个函数。校验：特征号必填、下偏差<=上偏差
    void refreshMeasurementRecords();//把 m_records 刷到表格；同时检查关联图形有没有被删掉（删了就标"关联已删除"并清空试测结果）。
    void loadMeasurementRecord(int row);
    void cancelRelink();
    void trialSelectedRecord();
    void showRecordDetection(int row);
    GraphicalDetectionParameters detectionInputs() const;
    void setDetectionInputs(const GraphicalDetectionParameters& parameters);
    void applyDetectionParameters();
    void refreshAngleControls();
    void chooseCornerCandidate(int index);
    QDoubleSpinBox* m_detectionSmoothing = nullptr;
    QDoubleSpinBox* m_detectionLow = nullptr;
    QDoubleSpinBox* m_detectionHigh = nullptr;
    QDoubleSpinBox* m_detectionMinLength = nullptr;
    QDoubleSpinBox* m_detectionMaxLength = nullptr;
    QDoubleSpinBox* m_detectionMergeDistance = nullptr;
    QDoubleSpinBox* m_cornerDeviation = nullptr;
    QDoubleSpinBox* m_cornerGap = nullptr;
    QLabel* m_detectionDiagnostic = nullptr;
    bool m_trialRunning = false;
    bool m_projectDirty = false;
    bool m_loadingProject = false;
    QString m_imageFilePath;
    QString m_imageFileSha256;
    QString m_projectFilePath;
    int m_imageCameraIndex = -1;
    int m_imageExposure = -1;
    int m_relinkSequence = -1;
    int m_relinkSlot = 1;

    struct MeasurementRecord {//每条测量记录保存为一个结构体
        struct AxisPosition {
            int axis = 0;
            double planned = 0;
            double encoder = 0;
        };
        struct DevicePosition {
            bool collected = false;
            QString source = QStringLiteral("none");
            QString unit = QStringLiteral("pulse");
            QString capturedAtUtc;
            int cameraIndex = -1;
            int exposure = -1;
            bool hasLightCurtainSample = false;
            double lightCurtainRawOut1 = 0;
            double lightCurtainDiameter = 0;
            QString lightCurtainSampledAtUtc;
            QVector<AxisPosition> axes;
        };
        int sequence = 0;//记录序号，自增
        int geometryId = -1;//关联的画布图形ID
        int secondaryGeometryId = -1;//第二图形ID
        QString featureNumber;//特征号
        QString type; //测量类型
        int holeUniformCount = 0;//孔径旧表单holeNumber：圆周均布个数，不是H0/H1拍照位置
        bool hasTolerance = false;//公差
        double nominal = 0;
        double lower = 0;
        double upper = 0;
        double pixelRadius = -1;//试测结果px
        double trialAngle = -1;//试测结果°
        bool useSupplementaryAngle = false;//是否取补角
        bool singleRoiAngle = false;
        GraphicalDetectionParameters detection;
        QVector<GraphicalCornerEdge> cornerEdges;
        QVector<GraphicalCornerPair> cornerPairs;
        int selectedCornerPair = -1;
        QString candidateSelectionAuditMode = QStringLiteral("none");
        int candidateSelectionAuditFirst = -1;
        int candidateSelectionAuditSecond = -1;
        QString cornerDiagnostic;
        DevicePosition devicePosition;
        void clearTrial(const QString& reason) {
            pixelRadius = -1;
            trialAngle = -1;
            trialStatus = reason;
            detectedEdges = QPainterPath();
            fittedArc = QPainterPath();
            cornerEdges.clear(); cornerPairs.clear(); selectedCornerPair = -1;
            candidateSelectionAuditMode = QStringLiteral("none");
            candidateSelectionAuditFirst = -1; candidateSelectionAuditSecond = -1;
            cornerDiagnostic.clear();
        }
        QString trialStatus = QStringLiteral("未执行");//检测边缘+拟合结果（画回画布）
        QPainterPath detectedEdges;
        QPainterPath fittedArc;
    };
    QVector<MeasurementRecord> m_records;
    int m_nextRecordSequence = 1;
    QComboBox* m_measurementType = nullptr;
    QComboBox* m_angleResultMode = nullptr;
    QComboBox* m_angleInputMode = nullptr;
    QComboBox* m_cornerCandidate = nullptr;
    QSpinBox* m_holeUniformCount = nullptr;
    QPushButton* m_selectAngleRoi1 = nullptr;
    QPushButton* m_selectAngleRoi2 = nullptr;
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
