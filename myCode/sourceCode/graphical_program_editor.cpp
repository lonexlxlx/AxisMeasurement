#include "graphical_program_editor.h"

#include "graphical_canvas.h"

#include <QAction>
#include <QComboBox>
#include <QLineEdit>
#include <QCloseEvent>
#include <QIcon>
#include <QKeySequence>
#include <QLabel>
#include <QShortcut>
#include <QThread>
#include <QVBoxLayout>
#include <HalconCpp.h>
#include <memory>
#include <cstring>
#include <cmath>
#include <stdexcept>
/*工具栏、左右下面板、记录表、试测调度、HALCON 算法*/
namespace {
struct ArcTrialResult {
    double radius = -1;
    QString status;
    QPainterPath edges;
    QPainterPath fitted;
};

struct LineTrialResult {
    double angle = -1;
    QString status;
    QPainterPath edges;
    QPainterPath fitted;
};

QPainterPath contourPath(const HalconCpp::HObject& contours)
{
    using namespace HalconCpp;
    HTuple count;
    CountObj(contours, &count);
    QPainterPath path;
    int total = 0;
    for (Hlong index = 1; index <= count.I(); ++index) {
        HObject contour;
        HTuple rows, columns;
        SelectObj(contours, &contour, index);
        GetContourXld(contour, &rows, &columns);
        if (rows.Length() > 1000000 - total) throw std::runtime_error("Too many contour points");
        total += static_cast<int>(rows.Length());
        for (Hlong point = 0; point < rows.Length(); ++point) {
            const QPointF position(columns[point].D(), rows[point].D());
            if (point == 0) path.moveTo(position);
            else path.lineTo(position);
        }
    }
    return path;
}

ArcTrialResult runArcTrial(const QImage& source, const GraphicalCanvas::MeasurementRoi& roi)
{
    using namespace HalconCpp;
    ArcTrialResult result;
    try {
        const QImage gray = source.convertToFormat(QImage::Format_Grayscale8);
        if (gray.isNull() || qint64(gray.width()) * gray.height() > 50000000) {
            result.status = QStringLiteral("测量失败：图像为空或超过本阶段5000万像素上限");
            return result;
        }
        QByteArray pixels(gray.width() * gray.height(), '\0');
        for (int row = 0; row < gray.height(); ++row)
            std::memcpy(pixels.data() + row * gray.width(), gray.constScanLine(row), gray.width());
        HObject image, region, reduced, edges, split, selected, joined, fitted;
        GenImage1(&image, "byte", gray.width(), gray.height(), reinterpret_cast<Hlong>(pixels.data()));
        if (roi.isCircle) {
            GenCircle(&region, roi.center.y(), roi.center.x(), roi.radius);
        }
        else if (roi.axisAligned) {
            const QRectF bounds = roi.corners.boundingRect();
            GenRectangle1(&region, qMax(0.0, bounds.top()), qMax(0.0, bounds.left()),
                qMin(double(gray.height() - 1), bounds.bottom()), qMin(double(gray.width() - 1), bounds.right()));
        }
        else {
            HTuple rows, columns;
            for (int index = 0; index < roi.corners.size(); ++index) {
                rows[index] = roi.corners[index].y();
                columns[index] = roi.corners[index].x();
            }
            GenRegionPolygonFilled(&region, rows, columns);
        }
        ReduceDomain(image, region, &reduced);
        // Preserve program_45::TLineFP_Chamfer_P's Segment=true processing parameters.
        EdgesSubPix(reduced, &edges, "canny", 2, 20, 60);
        result.edges = contourPath(edges);
        SegmentContoursXld(edges, &split, "lines_circles", 5, 4, 2);
        SelectContoursXld(split, &selected, "contour_length", 20, gray.width() / 2.0, -0.5, 0.5);
        UnionAdjacentContoursXld(selected, &joined, 90, 1, "attr_keep");
        HTuple count;
        CountObj(joined, &count);
        if (count.I() != 1) {
            result.status = QStringLiteral("测量失败：有效轮廓 %1 条，请调整ROI使目标唯一").arg(qlonglong(count.I()));
            return result;
        }
        HTuple row, column, radius, start, end, order;
        FitCircleContourXld(joined, "algebraic", -1, 0, 0, 3, 2,
            &row, &column, &radius, &start, &end, &order);
        if (radius.Length() != 1 || !std::isfinite(radius[0].D()) || radius[0].D() <= 0
            || radius[0].D() > 100.0 * qMax(gray.width(), gray.height())) {
            result.status = QStringLiteral("测量失败：未取得有效半径");
            return result;
        }
        GenCircleContourXld(&fitted, row, column, radius, start, end, order, 1.0);
        result.fitted = contourPath(fitted);
        result.radius = radius[0].D();
        result.status = QStringLiteral("试测完成（像素，未标定）");
    }
    catch (const HException& error) {
        result.status = QStringLiteral("测量失败：HALCON %1").arg(QString::fromLocal8Bit(error.ErrorMessage().Text()));
    }
    catch (const std::exception& error) {
        result.status = QStringLiteral("测量失败：%1").arg(QString::fromLocal8Bit(error.what()));
    }
    catch (...) { result.status = QStringLiteral("测量失败：未知异常"); }
    return result;
}

LineTrialResult runLineAngleTrial(const QImage& source, const GraphicalCanvas::MeasurementRoi& roi)
{
    using namespace HalconCpp;
    LineTrialResult result;
    try {
        const QImage gray = source.convertToFormat(QImage::Format_Grayscale8);
        if (gray.isNull() || qint64(gray.width()) * gray.height() > 50000000) {
            result.status = QStringLiteral("测量失败：图像为空或超过本阶段5000万像素上限");
            return result;
        }
        QByteArray pixels(gray.width() * gray.height(), '\0');
        for (int row = 0; row < gray.height(); ++row)
            std::memcpy(pixels.data() + row * gray.width(), gray.constScanLine(row), gray.width());
        HObject image, region, reduced, edges, split, selected, joined;
        GenImage1(&image, "byte", gray.width(), gray.height(), reinterpret_cast<Hlong>(pixels.data()));
        if (roi.isCircle) GenCircle(&region, roi.center.y(), roi.center.x(), roi.radius);
        else if (roi.axisAligned) {
            const QRectF bounds = roi.corners.boundingRect();
            GenRectangle1(&region, qMax(0.0, bounds.top()), qMax(0.0, bounds.left()),
                qMin(double(gray.height() - 1), bounds.bottom()), qMin(double(gray.width() - 1), bounds.right()));
        }
        else {
            HTuple rows, columns;
            for (int index = 0; index < roi.corners.size(); ++index) {
                rows[index] = roi.corners[index].y();
                columns[index] = roi.corners[index].x();
            }
            GenRegionPolygonFilled(&region, rows, columns);
        }
        ReduceDomain(image, region, &reduced);
        EdgesSubPix(reduced, &edges, "canny", 2, 20, 60);
        result.edges = contourPath(edges);
        SegmentContoursXld(edges, &split, "lines_circles", 5, 4, 2);
        SelectContoursXld(split, &selected, "contour_length", 20, gray.width() / 2.0, -0.5, 0.5);
        UnionAdjacentContoursXld(selected, &joined, 90, 1, "attr_keep");
        HTuple count;
        CountObj(joined, &count);
        if (count.I() == 0) {
            result.status = QStringLiteral("测量失败：ROI内未找到达到最小长度的直线轮廓；请检查对比度、边缘完整性或调整ROI");
            return result;
        }
        if (count.I() > 1) {
            result.status = QStringLiteral("测量失败：ROI内找到 %1 条有效轮廓，无法确定目标；请缩小或调整ROI使边缘唯一")
                .arg(qlonglong(count.I()));
            return result;
        }
        HTuple rowBegin, colBegin, rowEnd, colEnd, normalRow, normalCol, distance;
        FitLineContourXld(joined, "tukey", -1, 0, 5, 2,
            &rowBegin, &colBegin, &rowEnd, &colEnd, &normalRow, &normalCol, &distance);
        if (rowBegin.Length() != 1 || !std::isfinite(rowBegin[0].D()) || !std::isfinite(colBegin[0].D())
            || !std::isfinite(rowEnd[0].D()) || !std::isfinite(colEnd[0].D())) {
            result.status = QStringLiteral("测量失败：未取得有效拟合直线");
            return result;
        }
        double angle = std::atan2(rowEnd[0].D() - rowBegin[0].D(), colEnd[0].D() - colBegin[0].D())
            * 180.0 / 3.14159265358979323846;
        while (angle < 0) angle += 180.0;
        while (angle >= 180.0) angle -= 180.0;
        result.fitted.moveTo(colBegin[0].D(), rowBegin[0].D());
        result.fitted.lineTo(colEnd[0].D(), rowEnd[0].D());
        result.angle = angle;
        result.status = QStringLiteral("试测完成（图像向右0°，顺时针为正，范围0°–180°）");
    }
    catch (const HException& error) {
        result.status = QStringLiteral("测量失败：HALCON %1").arg(QString::fromLocal8Bit(error.ErrorMessage().Text()));
    }
    catch (const std::exception& error) {
        result.status = QStringLiteral("测量失败：%1").arg(QString::fromLocal8Bit(error.what()));
    }
    catch (...) { result.status = QStringLiteral("测量失败：未知异常"); }
    return result;
}

LineTrialResult runTwoRoiAngleTrial(const QImage& source,
    const GraphicalCanvas::MeasurementRoi& firstRoi,
    const GraphicalCanvas::MeasurementRoi& secondRoi,
    bool supplementary)
{
    LineTrialResult first = runLineAngleTrial(source, firstRoi);
    if (first.angle < 0) {
        first.status = QStringLiteral("ROI 1：%1").arg(first.status);
        return first;
    }
    LineTrialResult second = runLineAngleTrial(source, secondRoi);
    if (second.angle < 0) {
        second.status = QStringLiteral("ROI 2：%1").arg(second.status);
        return second;
    }
    LineTrialResult result;
    result.edges.addPath(first.edges);
    result.edges.addPath(second.edges);
    result.fitted.addPath(first.fitted);
    result.fitted.addPath(second.fitted);
    double difference = std::abs(first.angle - second.angle);
    if (difference > 90.0) difference = 180.0 - difference;
    result.angle = supplementary ? 180.0 - difference : difference;
    result.status = supplementary
        ? QStringLiteral("试测完成（两条拟合线的较大补角）")
        : QStringLiteral("试测完成（两条拟合线的较小夹角）");
    return result;
}
}
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QActionGroup>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QList>
#include <QListWidget>
#include <QMessageBox>
#include <QMenu>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStatusBar>
#include <QStringList>
#include <QTableWidget>
#include <QTabWidget>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QTimer>
#include <QDateTime>
#include <QScrollArea>
#include <QEvent>

// Temporary offline layout preview; restore false after the user's feedback.
namespace { constexpr bool kGraphicalAxisLayoutPreview = false; }

GraphicalProgramEditor::GraphicalProgramEditor(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("图形化二次开发"));
    resize(1500, 900);
    setWindowModality(Qt::ApplicationModal);
    buildInterface();
    QTimer* axisTimer = new QTimer(this);
    connect(axisTimer, &QTimer::timeout, this, &GraphicalProgramEditor::refreshAxisPanel);
    axisTimer->start(200);
}

void GraphicalProgramEditor::setAxisBackend(AxisReader reader, AxisCommander commander)
{
    m_axisReader = std::move(reader);
    m_axisCommander = std::move(commander);
    refreshAxisPanel();
}

QWidget* GraphicalProgramEditor::buildAxisPanel()
{
    QGroupBox* panel = new QGroupBox(QStringLiteral("手动轴控制"));
    QVBoxLayout* layout = new QVBoxLayout(panel);
    m_axisState = new QLabel(QStringLiteral("未连接"), panel);
    m_axisState->setWordWrap(true);
    layout->addWidget(m_axisState);
    m_axisPosition = new QLabel(QStringLiteral("规划 / 编码器：未采集"), panel);
    m_axisPosition->setWordWrap(true);
    layout->addWidget(m_axisPosition);
    m_axisInputs = new QWidget(panel);
    QFormLayout* form = new QFormLayout(m_axisInputs);
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    form->setContentsMargins(0, 0, 0, 0);
    m_axisSelector = new QComboBox(m_axisInputs);
    const int axes[] = { 1, 2, 5, 6, 7 };
    const QStringList names = { QStringLiteral("测粗糙度轴"), QStringLiteral("测孔轴"),
        QStringLiteral("光幕轴"), QStringLiteral("上顶尖轴"), QStringLiteral("转台轴") };
    for (int i = 0; i < 5; ++i)
        m_axisSelector->addItem(QStringLiteral("%1 · %2").arg(axes[i]).arg(names[i]), axes[i]);
    form->addRow(QStringLiteral("运动轴"), m_axisSelector);
    m_axisMode = new QComboBox(m_axisInputs);
    m_axisMode->addItems({ QStringLiteral("Jog（按住移动）"), QStringLiteral("绝对点位") });
    form->addRow(QStringLiteral("模式"), m_axisMode);
    m_axisSpeed = new QDoubleSpinBox(m_axisInputs);
    m_axisSpeed->setDecimals(3);
    m_axisSpeed->setRange(0.001, 1000);
    m_axisSpeed->setValue(1);
    m_axisSpeed->setKeyboardTracking(false);
    form->addRow(QStringLiteral("速度 pulse/ms"), m_axisSpeed);
    m_axisTarget = new QDoubleSpinBox(m_axisInputs);
    m_axisTarget->setDecimals(0);
    m_axisTarget->setRange(-2147483647.0, 2147483647.0);
    m_axisTarget->setKeyboardTracking(false);
    form->addRow(QStringLiteral("目标 pulse"), m_axisTarget);
    layout->addWidget(m_axisInputs);
    QHBoxLayout* jogRow = new QHBoxLayout;
    m_jogNegative = new QPushButton(QStringLiteral("负向 −（按住）"), panel);
    m_jogPositive = new QPushButton(QStringLiteral("正向 +（按住）"), panel);
    jogRow->addWidget(m_jogNegative);
    jogRow->addWidget(m_jogPositive);
    layout->addLayout(jogRow);
    m_moveAbsolute = new QPushButton(QStringLiteral("移动至目标位置"), panel);
    layout->addWidget(m_moveAbsolute);
    QHBoxLayout* servoRow = new QHBoxLayout;
    m_axisEnable = new QPushButton(QStringLiteral("轴使能"), panel);
    m_axisDisable = new QPushButton(QStringLiteral("关闭使能"), panel);
    servoRow->addWidget(m_axisEnable);
    servoRow->addWidget(m_axisDisable);
    layout->addLayout(servoRow);
    QLabel* hint = new QLabel(QStringLiteral("目标是绝对脉冲位置。加减速沿用原轴参数；速度上限仅为输入范围，不代表设备安全速度。"), panel);
    hint->setWordWrap(true);
    layout->addWidget(hint);
    m_axisMessage = new QLabel(panel);
    m_axisMessage->setWordWrap(true);
    layout->addWidget(m_axisMessage);
    layout->addStretch();
    connect(m_jogNegative, &QPushButton::pressed, this, [this]() { executeAxisCommand(AxisCommand::JogNegative); });
    connect(m_jogPositive, &QPushButton::pressed, this, [this]() { executeAxisCommand(AxisCommand::JogPositive); });
    connect(m_jogNegative, &QPushButton::released, this, [this]() { stopOwnedAxis(); });
    connect(m_jogPositive, &QPushButton::released, this, [this]() { stopOwnedAxis(); });
    connect(m_moveAbsolute, &QPushButton::clicked, this, [this]() { executeAxisCommand(AxisCommand::MoveAbsolute); });
    connect(m_axisEnable, &QPushButton::clicked, this, [this]() { executeAxisCommand(AxisCommand::Enable); });
    connect(m_axisDisable, &QPushButton::clicked, this, [this]() { executeAxisCommand(AxisCommand::Disable); });
    connect(m_axisSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
        stopOwnedAxis();
        m_axisPosition->setText(QStringLiteral("规划 / 编码器：未采集"));
        refreshAxisPanel();
    });
    connect(m_axisMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
        stopOwnedAxis(); refreshAxisPanel();
    });
    return panel;
}

void GraphicalProgramEditor::refreshAxisPanel()
{
    if (!m_axisSelector || !isVisible()) return;
    const int axis = m_ownedAxis > 0 ? m_ownedAxis : m_axisSelector->currentData().toInt();
    AxisSnapshot snapshot;
    snapshot.message = QStringLiteral("控制接口未连接");
    if (m_axisReader) snapshot = m_axisReader(axis);
    const bool moving = snapshot.valid && (snapshot.status & 0x400);
    const bool held = m_jogNegative->isDown() || m_jogPositive->isDown();
    if (m_ownedAxis > 0 && snapshot.valid && !moving &&
        (m_axisStopRequested || (!held && QDateTime::currentMSecsSinceEpoch() - m_axisStartedAt > 600))) {
        m_ownedAxis = -1;
        m_axisStopRequested = false;
    }
    if (m_ownedAxis > 0 && (!snapshot.available || !snapshot.valid || m_trialRunning)) stopOwnedAxis();
    m_axisState->setText(snapshot.message + (snapshot.valid
        ? QStringLiteral("\n使能：%1  运动：%2\n正限位：%3  负限位：%4  报警/停止：%5")
            .arg((snapshot.status & 0x200) ? QStringLiteral("开") : QStringLiteral("关"))
            .arg(moving ? QStringLiteral("是") : QStringLiteral("否"))
            .arg((snapshot.status & 0x20) ? QStringLiteral("触发") : QStringLiteral("无"))
            .arg((snapshot.status & 0x40) ? QStringLiteral("触发") : QStringLiteral("无"))
            .arg((snapshot.status & 0x192) ? QStringLiteral("有") : QStringLiteral("无")) : QString()));
    m_axisPosition->setText(snapshot.valid
        ? QStringLiteral("规划：%1 pulse\n编码器：%2 pulse").arg(snapshot.planned, 0, 'f', 1).arg(snapshot.encoder, 0, 'f', 1)
        : QStringLiteral("规划 / 编码器：未采集"));
    const bool idle = snapshot.available && snapshot.valid && !moving && m_ownedAxis < 0 && !m_trialRunning;
    const bool offlinePreview = kGraphicalAxisLayoutPreview && !snapshot.connected && m_ownedAxis < 0 && !m_trialRunning;
    m_axisInputs->setEnabled(idle || offlinePreview);
    if (offlinePreview)
        m_axisState->setText(QStringLiteral("未连接 · 临时布局预览\n可选轴和切换模式；实际运动禁用。"));
    const bool jog = m_axisMode->currentIndex() == 0;
    m_axisTarget->setEnabled(!jog);
    m_jogNegative->setVisible(jog);
    m_jogPositive->setVisible(jog);
    m_moveAbsolute->setVisible(!jog);
    const bool canMove = idle && (snapshot.status & 0x200) && !(snapshot.status & 0x192);
    // Keep the pressed Jog button enabled so release is delivered normally.
    m_jogNegative->setEnabled((canMove && !(snapshot.status & 0x40)) || (held && m_jogNegative->isDown()));
    m_jogPositive->setEnabled((canMove && !(snapshot.status & 0x20)) || (held && m_jogPositive->isDown()));
    m_moveAbsolute->setEnabled(canMove);
    m_axisEnable->setEnabled(idle && !(snapshot.status & 0x200) && !(snapshot.status & 0x192));
    m_axisDisable->setEnabled(idle && (snapshot.status & 0x200));
    m_axisStop->setEnabled(snapshot.connected);
    m_axisEmergency->setEnabled(snapshot.connected);
}

void GraphicalProgramEditor::executeAxisCommand(AxisCommand command)
{
    if (!m_axisCommander) return;
    const bool stop = command == AxisCommand::Stop || command == AxisCommand::EmergencyStop;
    if (!stop && (m_trialRunning || m_ownedAxis > 0)) return;
    const int axis = m_ownedAxis > 0 ? m_ownedAxis : m_axisSelector->currentData().toInt();
    const auto result = m_axisCommander(axis, command, m_axisSpeed->value(), static_cast<long>(m_axisTarget->value()));
    const QString error = result.error;
    if (!error.isEmpty()) {
        // Update may have reached the card even if its acknowledgement failed.
        if (result.motionMayHaveStarted) {
            m_ownedAxis = axis;
            m_axisStopRequested = false;
            m_axisStartedAt = QDateTime::currentMSecsSinceEpoch();
        }
        m_axisMessage->setText(error);
        return;
    }
    const bool starts = command == AxisCommand::JogNegative || command == AxisCommand::JogPositive || command == AxisCommand::MoveAbsolute;
    if (starts) {
        m_ownedAxis = axis;
        m_axisStopRequested = false;
        m_axisStartedAt = QDateTime::currentMSecsSinceEpoch();
    }
    if (stop && m_ownedAxis > 0) m_axisStopRequested = true;
    m_axisMessage->setText(stop ? QStringLiteral("停止指令已发送，请观察轴状态。") : QStringLiteral("指令已发送；以设备实际状态为准。"));
}

bool GraphicalProgramEditor::stopOwnedAxis()
{
    if (m_ownedAxis < 0) return true;
    if (!m_axisCommander) return false;
    const QString error = m_axisCommander(m_ownedAxis, AxisCommand::Stop, 0, 0).error;
    m_axisStopRequested = error.isEmpty();
    m_axisMessage->setText(error.isEmpty() ? QStringLiteral("已请求停止轴 %1。").arg(m_ownedAxis) : error);
    return error.isEmpty();
}

bool GraphicalProgramEditor::event(QEvent* event)
{
    if (event->type() == QEvent::WindowDeactivate || event->type() == QEvent::Hide)
        stopOwnedAxis();
    return QMainWindow::event(event);
}
/*整个界面在一个函数里面搭出来*/
void GraphicalProgramEditor::buildInterface()
{
    QToolBar* toolBar = addToolBar(QStringLiteral("图形工具"));
    toolBar->setObjectName(QStringLiteral("graphicalProgramToolBar"));
    toolBar->setMovable(false);
    //P1-7 工具栏升级：图标在上、文字在下，按钮加图标与快捷键提示
    toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    toolBar->setIconSize(QSize(20, 20));

    QToolBar* axisSafetyBar = new QToolBar(QStringLiteral("轴停止"), this);
    axisSafetyBar->setObjectName(QStringLiteral("graphicalAxisSafetyBar"));
    axisSafetyBar->setMovable(false);
    addToolBar(Qt::TopToolBarArea, axisSafetyBar);
    m_axisStop = new QPushButton(QStringLiteral("停止当前轴"), axisSafetyBar);
    m_axisEmergency = new QPushButton(QStringLiteral("全部轴急停"), axisSafetyBar);
    m_axisEmergency->setStyleSheet(QStringLiteral("QPushButton { background: #DC2626; color: white; font-weight: bold; padding: 8px; }"));
    m_axisStop->setEnabled(false);
    m_axisEmergency->setEnabled(false);
    axisSafetyBar->addWidget(m_axisStop);
    axisSafetyBar->addWidget(m_axisEmergency);
    connect(m_axisStop, &QPushButton::clicked, this, [this]() { executeAxisCommand(AxisCommand::Stop); });
    connect(m_axisEmergency, &QPushButton::clicked, this, [this]() { executeAxisCommand(AxisCommand::EmergencyStop); });

    QAction* openImageAction = toolBar->addAction(QStringLiteral("打开图像"));
    QAction* cameraAction = toolBar->addAction(QStringLiteral("相机图像"));
    toolBar->addSeparator();
    QAction* selectAction = toolBar->addAction(QStringLiteral("选择"));
    QAction* pointAction = toolBar->addAction(QStringLiteral("点"));
    QAction* lineAction = toolBar->addAction(QStringLiteral("直线"));
    QAction* rectangleAction = toolBar->addAction(QStringLiteral("矩形"));
    QAction* circleAction = toolBar->addAction(QStringLiteral("圆"));
    QAction* arcAction = toolBar->addAction(QStringLiteral("圆弧"));
    toolBar->addSeparator();
    QAction* fitAction = toolBar->addAction(QStringLiteral("适合窗口"));
    QAction* undoAction = toolBar->addAction(QStringLiteral("撤销"));
    QAction* redoAction = toolBar->addAction(QStringLiteral("重做"));
    QAction* deleteAction = toolBar->addAction(QStringLiteral("删除"));

    //图标资源已注册在 AxisMeasurement.qrc（:/AxisMeasurement/config/icons/）
    const struct { QAction* action; const char* icon; const char* key; const char* tip; } toolbarInfo[] = {
        { openImageAction, "open", nullptr, "打开本地图像（记录与图形将清空）" },
        { cameraAction, "camera", nullptr, "从相机采集图像（尚未接入）" },
        { selectAction, "select", "V", "选择 (V)：选中/移动/调整图形" },
        { pointAction, "point", "P", "点 (P)：单击标注特征点" },
        { lineAction, "line", "L", "直线 (L)：拖动画直线" },
        { rectangleAction, "rect", "R", "矩形 (R)：拖动画矩形，下拉可选绘制模式" },
        { circleAction, "circle", "C", "圆 (C)：拖动画圆，下拉可选绘制模式" },
        { arcAction, "arc", "A", "圆弧 (A)：依次点击起点、弧上点、终点" },
        { fitAction, "fit", "F", "适合窗口 (F)：图像缩放到充满画布" },
        { undoAction, "undo", nullptr, "撤销（尚未接入）" },
        { redoAction, "redo", nullptr, "重做（尚未接入）" },
        { deleteAction, "delete", nullptr, "删除选中图形 (Del)" },
    };
    for (const auto& info : toolbarInfo) {
        info.action->setIcon(QIcon(QStringLiteral(":/AxisMeasurement/config/icons/%1.png").arg(info.icon)));
        info.action->setToolTip(QString::fromUtf8(info.tip));
        if (info.key) {
            info.action->setShortcut(QKeySequence(QString::fromLatin1(info.key)));
            //只在画布获得焦点时生效，避免与右侧输入框打字冲突
            info.action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        }
    }

    const QList<QAction*> futureActions = {
        cameraAction, undoAction, redoAction//撤销、重做、删除这几个是占位按钮
    };
    for (QAction* action : futureActions)
        action->setEnabled(false);//创建后功能并未做

    QActionGroup* drawingToolGroup = new QActionGroup(this);//设置了上面的工具栏的功能
    drawingToolGroup->setExclusive(true);//设置单选切换
    const QList<QAction*> drawingActions = {
        selectAction, pointAction, lineAction, rectangleAction, circleAction, arcAction//选择、点、直线、矩形、圆、圆弧
    };
    for (QAction* action : drawingActions) {
        action->setCheckable(true);
        drawingToolGroup->addAction(action);//将上面的6个单点事件添加进工具栏中
    }
    selectAction->setChecked(true);

    QMenu* rectangleMenu = new QMenu(toolBar);//矩形下拉箭头小菜单，矩形有“角点-角点”和“中心向外”两种画法
    QActionGroup* rectangleModeGroup = new QActionGroup(rectangleMenu);//小菜单事件群组
    rectangleModeGroup->setExclusive(true);
    QAction* rectangleCornerAction = rectangleMenu->addAction(QStringLiteral("角点-角点"));
    QAction* rectangleCenterAction = rectangleMenu->addAction(QStringLiteral("中心向外"));
    rectangleCornerAction->setCheckable(true);
    rectangleCenterAction->setCheckable(true);
    rectangleModeGroup->addAction(rectangleCornerAction);
    rectangleModeGroup->addAction(rectangleCenterAction);
    rectangleCornerAction->setChecked(true);
    if (QToolButton* button = qobject_cast<QToolButton*>(toolBar->widgetForAction(rectangleAction))) {
        button->setMenu(rectangleMenu);
        button->setPopupMode(QToolButton::MenuButtonPopup);
    }

    QMenu* circleMenu = new QMenu(toolBar);//圆的下拉箭头小菜单，圆有“外接框”和“圆心-半径”两种画法
    QActionGroup* circleModeGroup = new QActionGroup(circleMenu);
    circleModeGroup->setExclusive(true);
    QAction* circleBoxAction = circleMenu->addAction(QStringLiteral("外接框"));
    QAction* circleCenterAction = circleMenu->addAction(QStringLiteral("圆心-半径"));
    circleBoxAction->setCheckable(true);
    circleCenterAction->setCheckable(true);
    circleModeGroup->addAction(circleBoxAction);
    circleModeGroup->addAction(circleCenterAction);
    circleBoxAction->setChecked(true);
    if (QToolButton* button = qobject_cast<QToolButton*>(toolBar->widgetForAction(circleAction))) {
        button->setMenu(circleMenu);
        button->setPopupMode(QToolButton::MenuButtonPopup);
    }

    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* rootLayout = new QVBoxLayout(centralWidget);
    rootLayout->setContentsMargins(6, 6, 6, 6);

    QSplitter* verticalSplitter = new QSplitter(Qt::Vertical, centralWidget);
    QSplitter* mainSplitter = new QSplitter(Qt::Horizontal, verticalSplitter);/*用水平QSplitter分成三块*/

    QGroupBox* featureGroup = new QGroupBox(QStringLiteral("图形特征"), mainSplitter);//左栏的图形特征
    QVBoxLayout* featureLayout = new QVBoxLayout(featureGroup);
    m_featureList = new QListWidget(featureGroup);//m_featureList,列出画布上已经画好的图形
    m_featureList->addItem(QStringLiteral("尚未创建图形特征"));
    m_featureList->setEnabled(false);
    m_featureList->setContextMenuPolicy(Qt::CustomContextMenu);
    featureLayout->addWidget(m_featureList);
    featureGroup->setMinimumWidth(220);

    m_canvas = new GraphicalCanvas(mainSplitter);//m_canvas为中间的黑色图像区域,主要是canvas.cpp里的代码
    m_canvas->setMinimumSize(640, 420);
    //P1-7 画布深色背景：图像边界更清晰
    m_canvas->setBackgroundBrush(QColor(QStringLiteral("#2B2B2B")));
    //P1-7 空态提示：未打开图像时居中显示，打开图像后隐藏
    QLabel* emptyHint = new QLabel(
        QStringLiteral("尚未打开图像\n\n点击工具栏「打开图像」选择本地图片开始编辑"), m_canvas);
    emptyHint->setObjectName(QStringLiteral("canvasEmptyHint"));
    emptyHint->setAlignment(Qt::AlignCenter);
    emptyHint->setAttribute(Qt::WA_TransparentForMouseEvents);
    emptyHint->setStyleSheet(QStringLiteral("color:#9CA3AF; font-size:14px; background:transparent;"));
    QVBoxLayout* hintLayout = new QVBoxLayout(m_canvas);
    hintLayout->addWidget(emptyHint, 0, Qt::AlignCenter);
    //工具快捷键挂到画布上（WidgetWithChildrenShortcut 上下文需要 action 属于该 widget）
    m_canvas->addActions({ selectAction, pointAction, lineAction, rectangleAction, circleAction, arcAction, fitAction });

    QTabWidget* propertyTabs = new QTabWidget(mainSplitter);//右栏三个属性
    QWidget* featurePropertyPage = new QWidget(propertyTabs);//特征属性页
    QFormLayout* featurePropertyLayout = new QFormLayout(featurePropertyPage);
    m_featureNameLabel = new QLabel(QStringLiteral("未选择"), featurePropertyPage);
    m_featureTypeLabel = new QLabel(QStringLiteral("-"), featurePropertyPage);
    featurePropertyLayout->addRow(QStringLiteral("特征名称："), m_featureNameLabel);
    featurePropertyLayout->addRow(QStringLiteral("特征类型："), m_featureTypeLabel);
    m_coordinateLabel = new QLabel(QStringLiteral("-"), featurePropertyPage);
    m_coordinateLabel->setWordWrap(true);
    featurePropertyLayout->addRow(QStringLiteral("图像坐标："), m_coordinateLabel);
    m_coordinateLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    QLabel* coordinateHint = new QLabel(QStringLiteral("原图左上角为原点，X向右、Y向下。\n这里只显示手绘几何，不是算法测量结果。"), featurePropertyPage);
    coordinateHint->setWordWrap(true);
    featurePropertyLayout->addRow(coordinateHint);
    m_primarySizeLabel = new QLabel(QStringLiteral("尺寸："), featurePropertyPage);
    m_primarySize = new QDoubleSpinBox(featurePropertyPage);
    m_secondarySize = new QDoubleSpinBox(featurePropertyPage);
    for (QDoubleSpinBox* input : {m_primarySize, m_secondarySize}) {
        input->setDecimals(4);
        input->setRange(0.0001, 1000000.0);
        input->setSuffix(QStringLiteral(" px"));
        input->setKeyboardTracking(false);
        input->setEnabled(false);
    }
    m_lockRatio = new QCheckBox(QStringLiteral("矩形锁定宽高比例"), featurePropertyPage);
    m_lockRatio->setChecked(true);
    m_lockRatio->setEnabled(false);
    m_applySize = new QPushButton(QStringLiteral("应用尺寸"), featurePropertyPage);
    m_applySize->setEnabled(false);
    featurePropertyLayout->addRow(m_primarySizeLabel, m_primarySize);
    featurePropertyLayout->addRow(QStringLiteral("高度："), m_secondarySize);
    featurePropertyLayout->addRow(m_lockRatio);
    featurePropertyLayout->addRow(m_applySize);
    connect(m_primarySize, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
        [this](double value) {
            if (m_lockRatio->isEnabled() && m_lockRatio->isChecked()) {
                QSignalBlocker blocker(m_secondarySize);
                m_secondarySize->setValue(value / m_sizeRatio);
            }
        });
    connect(m_secondarySize, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
        [this](double value) {
            if (m_lockRatio->isEnabled() && m_lockRatio->isChecked()) {
                QSignalBlocker blocker(m_primarySize);
                m_primarySize->setValue(value * m_sizeRatio);
            }
        });
    connect(m_lockRatio, &QCheckBox::toggled, this, [this](bool checked) {
        m_canvas->setResizeRatioLocked(checked);
        if (checked && m_secondarySize->value() > 0)
            m_sizeRatio = m_primarySize->value() / m_secondarySize->value();
    });
    connect(m_applySize, &QPushButton::clicked, this, [this]() {
        QString error;
        if (!m_canvas->resizeFeature(m_selectedFeatureId, m_primarySize->value(),
            m_secondarySize->value(), error)) {
            QMessageBox::warning(this, QStringLiteral("未应用尺寸"), error);
        }
    });
    m_rotationLabel = new QLabel(QStringLiteral("旋转角："), featurePropertyPage);
    m_rotationAngle = new QDoubleSpinBox(featurePropertyPage);
    m_rotationAngle->setRange(-360.0, 360.0);
    m_rotationAngle->setDecimals(2);
    m_rotationAngle->setSuffix(QStringLiteral(" °"));
    m_rotationAngle->setKeyboardTracking(false);
    m_rotationAngle->setEnabled(false);
    m_rotationAngle->setToolTip(QStringLiteral("顺时针为正；输入目标角度，不是每次累加的角度。"));
    m_applyRotation = new QPushButton(QStringLiteral("应用角度"), featurePropertyPage);
    m_applyRotation->setEnabled(false);
    featurePropertyLayout->addRow(m_rotationLabel, m_rotationAngle);
    featurePropertyLayout->addRow(m_applyRotation);
    connect(m_applyRotation, &QPushButton::clicked, this, [this]() {
        QString error;
        if (!m_canvas->rotateFeature(m_selectedFeatureId, m_rotationAngle->value(), error))
            QMessageBox::warning(this, QStringLiteral("未应用角度"), error);
    });
    propertyTabs->addTab(featurePropertyPage, QStringLiteral("特征属性"));

    QWidget* measurementPage = new QWidget(propertyTabs);//测量配置页
    QVBoxLayout* measurementLayout = new QVBoxLayout(measurementPage);
    QFormLayout* measurementForm = new QFormLayout;
    measurementLayout->addLayout(measurementForm);
    m_measurementType = new QComboBox(measurementPage);
    m_measurementType->addItems(QStringList() << QStringLiteral("直径") << QStringLiteral("粗糙度")
        << QStringLiteral("孔径") << QStringLiteral("圆柱度") << QStringLiteral("跳动")
        << QStringLiteral("长度") << QStringLiteral("角度") << QStringLiteral("圆弧半径"));
    m_featureNumber = new QLineEdit(measurementPage);
    m_featureNumber->setMaxLength(64);
    m_featureNumber->setPlaceholderText(QStringLiteral("工序特征号，必填"));
    measurementForm->addRow(QStringLiteral("测量类型："), m_measurementType);
    m_angleResultMode = new QComboBox(measurementPage);
    m_angleResultMode->addItems(QStringList() << QStringLiteral("较小夹角（0°–90°）")
        << QStringLiteral("较大补角（90°–180°）"));
    measurementForm->addRow(QStringLiteral("角度结果："), m_angleResultMode);
    m_angleResultMode->setEnabled(m_measurementType->currentText() == QStringLiteral("角度"));
    connect(m_measurementType, &QComboBox::currentTextChanged, this, [this](const QString& type) {
        m_angleResultMode->setEnabled(type == QStringLiteral("角度"));
    });
    measurementForm->addRow(QStringLiteral("特征号："), m_featureNumber);
    m_hasTolerance = new QCheckBox(QStringLiteral("设置公称值和上下偏差"), measurementPage);
    measurementForm->addRow(m_hasTolerance);
    m_nominal = new QDoubleSpinBox(measurementPage);
    m_lowerDeviation = new QDoubleSpinBox(measurementPage);
    m_upperDeviation = new QDoubleSpinBox(measurementPage);
    for (QDoubleSpinBox* input : {m_nominal, m_lowerDeviation, m_upperDeviation}) {
        input->setRange(-1000000.0, 1000000.0);
        input->setDecimals(4);
        input->setKeyboardTracking(false);
        input->setEnabled(false);
    }
    measurementForm->addRow(QStringLiteral("公称值："), m_nominal);
    measurementForm->addRow(QStringLiteral("下偏差："), m_lowerDeviation);
    measurementForm->addRow(QStringLiteral("上偏差："), m_upperDeviation);
    connect(m_hasTolerance, &QCheckBox::toggled, this, [this](bool enabled) {
        for (QDoubleSpinBox* input : {m_nominal, m_lowerDeviation, m_upperDeviation})
            input->setEnabled(enabled);
    });
    QPushButton* addRecord = new QPushButton(QStringLiteral("新增测量记录"), measurementPage);
    QPushButton* updateRecord = new QPushButton(QStringLiteral("更新选中记录"), measurementPage);
    QPushButton* deleteRecord = new QPushButton(QStringLiteral("删除选中记录（保留图形）"), measurementPage);
    measurementLayout->addWidget(addRecord);
    measurementLayout->addWidget(updateRecord);
    QPushButton* relinkRecord = new QPushButton(QStringLiteral("重新关联图形"), measurementPage);
    QPushButton* selectAngleRoi1 = new QPushButton(QStringLiteral("角度：选择/重选 ROI 1（直线1）"), measurementPage);
    QPushButton* selectAngleRoi2 = new QPushButton(QStringLiteral("角度：选择/重选 ROI 2（直线2）"), measurementPage);
    QPushButton* cancelRelinkButton = new QPushButton(QStringLiteral("取消关联（Esc）"), measurementPage);
    measurementLayout->addWidget(relinkRecord);
    measurementLayout->addWidget(selectAngleRoi1);
    measurementLayout->addWidget(selectAngleRoi2);
    measurementLayout->addWidget(cancelRelinkButton);
    connect(relinkRecord, &QPushButton::clicked, this, [this, selectAction]() {//关联机制
        const int row = m_stepTable->currentRow();
        if (row < 0 || row >= m_records.size()) {
            QMessageBox::warning(this, QStringLiteral("未开始关联"), QStringLiteral("请先选中下方需要重新关联的记录。"));
            return;
        }
        const int sequence = m_records[row].sequence;
        m_relinkSlot = 1;
        selectAction->trigger();
        // Clear the previous geometry before arming, so it cannot bind itself.
        m_canvas->selectFeatureById(-1, false);
        {
            QSignalBlocker blocker(m_stepTable);
            m_stepTable->setCurrentCell(row, 0);
            m_stepTable->selectRow(row);
        }
        m_relinkSequence = sequence;
        m_stepTable->setEnabled(false);
        statusBar()->showMessage(QStringLiteral("正在关联记录 %1：请点击画布或左侧列表中的目标图形；Esc取消。未应用的配置输入不会保存。")
            .arg(sequence));
    });
    const auto armAngleRoi = [this, selectAction](int slot) {
        const int row = m_stepTable->currentRow();
        if (row < 0 || row >= m_records.size() || m_records[row].type != QStringLiteral("角度")) {
            QMessageBox::warning(this, QStringLiteral("未开始关联"), QStringLiteral("请先选中一条已保存的角度记录。"));
            return;
        }
        const int sequence = m_records[row].sequence;
        selectAction->trigger();
        m_canvas->selectFeatureById(-1, false);
        {
            QSignalBlocker blocker(m_stepTable);
            m_stepTable->setCurrentCell(row, 0);
            m_stepTable->selectRow(row);
        }
        m_relinkSequence = sequence;
        m_relinkSlot = slot;
        m_stepTable->setEnabled(false);
        if (QShortcut* shortcut = findChild<QShortcut*>(QStringLiteral("cancelRelinkShortcut")))
            shortcut->setEnabled(true);
        statusBar()->showMessage(QStringLiteral("正在为角度记录 %1 选择 ROI %2：请点击一个矩形或圆形ROI；Esc取消。")
            .arg(sequence).arg(slot));
    };
    connect(selectAngleRoi1, &QPushButton::clicked, this, [armAngleRoi]() { armAngleRoi(1); });
    connect(selectAngleRoi2, &QPushButton::clicked, this, [armAngleRoi]() { armAngleRoi(2); });
    connect(cancelRelinkButton, &QPushButton::clicked, this, [this]() { cancelRelink(); });
    QShortcut* cancelRelinkShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    cancelRelinkShortcut->setEnabled(false);
    // Enable only during relinking; normal drawing/resize Esc remains with the canvas.
    connect(relinkRecord, &QPushButton::clicked, this, [this, cancelRelinkShortcut]() {
        cancelRelinkShortcut->setEnabled(m_relinkSequence > 0);
    });
    connect(cancelRelinkShortcut, &QShortcut::activated, this, [this]() { cancelRelink(); });
    cancelRelinkShortcut->setObjectName(QStringLiteral("cancelRelinkShortcut"));
    connect(drawingToolGroup, &QActionGroup::triggered, this, [this](QAction*) { cancelRelink(); });
    measurementLayout->addWidget(deleteRecord);
    QPushButton* trialArc = new QPushButton(QStringLiteral("试测选中记录"), measurementPage);
    measurementLayout->addWidget(trialArc);
    connect(trialArc, &QPushButton::clicked, this, [this]() { trialSelectedRecord(); });
    QLabel* measurementHint = new QLabel(QStringLiteral(
        "圆弧半径使用一个ROI；角度记录需分别选择ROI 1和ROI 2。当前小步只建立双ROI关联，夹角算法随后接入。\n"
        "长度类公差按mm、角度按°、粗糙度按μm录入（配置约定，非标定结果）。\n"
        "可先选图形进行关联，也可建立待关联记录。\n"
        "记录暂存于当前窗口，尚未接入保存和程序导出。"), measurementPage);
    measurementHint->setWordWrap(true);
    measurementLayout->addWidget(measurementHint);
    connect(addRecord, &QPushButton::clicked, this, [this]() { saveMeasurementRecord(false); });
    connect(updateRecord, &QPushButton::clicked, this, [this]() { saveMeasurementRecord(true); });
    connect(deleteRecord, &QPushButton::clicked, this, [this]() {
        if (m_relinkSequence > 0) { cancelRelink(); return; }
        const int row = m_stepTable->currentRow();
        if (row < 0 || row >= m_records.size()) return;
        if (QMessageBox::question(this, QStringLiteral("删除记录"), QStringLiteral("仅删除选中测量记录，保留画布图形？")) != QMessageBox::Yes) return;
        m_records.removeAt(row);
        refreshMeasurementRecords();
        m_canvas->setDetectionOverlay(QPainterPath(), QPainterPath());
    });
    measurementLayout->addStretch();
    propertyTabs->addTab(measurementPage, QStringLiteral("测量配置"));

    QWidget* positionPage = new QWidget(propertyTabs);//设备点位页
    QVBoxLayout* positionLayout = new QVBoxLayout(positionPage);
    positionLayout->addWidget(new QLabel(
        QStringLiteral("设备点位尚未采集；硬件读取将在后续接入，不以图像坐标代替。"),
        positionPage));
    positionLayout->addStretch();
    propertyTabs->addTab(positionPage, QStringLiteral("设备点位"));
    propertyTabs->setMinimumWidth(300);

    QSplitter* leftSplitter = new QSplitter(Qt::Vertical, mainSplitter);
    leftSplitter->setObjectName(QStringLiteral("graphicalAxisFeatureSplitter"));
    leftSplitter->setMinimumWidth(300);
    QScrollArea* axisScroll = new QScrollArea(leftSplitter);
    axisScroll->setWidgetResizable(true);
    axisScroll->setWidget(buildAxisPanel());
    leftSplitter->addWidget(axisScroll);
    leftSplitter->addWidget(featureGroup);
    leftSplitter->setChildrenCollapsible(false);
    featureGroup->setMinimumHeight(100);
    leftSplitter->setSizes({ 470, 160 });
    mainSplitter->addWidget(leftSplitter);
    mainSplitter->addWidget(m_canvas);
    mainSplitter->addWidget(propertyTabs);
    mainSplitter->setStretchFactor(0, 0);
    mainSplitter->setStretchFactor(1, 1);
    mainSplitter->setStretchFactor(2, 0);
    mainSplitter->setChildrenCollapsible(false);
    mainSplitter->setSizes(QList<int>() << 310 << 800 << 330);

    QGroupBox* stepGroup = new QGroupBox(QStringLiteral("测量流程"), verticalSplitter);//底部的“测量流程”表
    QVBoxLayout* stepLayout = new QVBoxLayout(stepGroup);
    m_stepTable = new QTableWidget(0, 12, stepGroup);
    m_stepTable->setHorizontalHeaderLabels(QStringList()
        << QStringLiteral("记录序号") << QStringLiteral("特征号") << QStringLiteral("测量类型")
        << QStringLiteral("关联图形") << QStringLiteral("公称值") << QStringLiteral("下偏差")
        << QStringLiteral("上偏差") << QStringLiteral("配置单位") << QStringLiteral("测量值")
        << QStringLiteral("判定") << QStringLiteral("设备点位") << QStringLiteral("执行状态"));
    m_stepTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_stepTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_stepTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_stepTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    connect(m_stepTable, &QTableWidget::currentCellChanged, this,
        [this](int row, int, int, int) { loadMeasurementRecord(row); });
    m_stepTable->horizontalHeader()->setStretchLastSection(true);
    m_stepTable->setMinimumHeight(160);
    stepLayout->addWidget(m_stepTable);

    verticalSplitter->addWidget(mainSplitter);
    verticalSplitter->addWidget(stepGroup);
    verticalSplitter->setStretchFactor(0, 1);
    verticalSplitter->setStretchFactor(1, 0);
    verticalSplitter->setSizes(QList<int>() << 680 << 200);
    rootLayout->addWidget(verticalSplitter);
    setCentralWidget(centralWidget);

    connect(openImageAction, &QAction::triggered, this, [this]() { openLocalImage(); });
    connect(fitAction, &QAction::triggered, m_canvas, &GraphicalCanvas::fitImageInView);
    connect(selectAction, &QAction::triggered, this, [this]() {
        m_canvas->setDrawingTool(GraphicalCanvas::DrawingTool::Select);
    });
    connect(pointAction, &QAction::triggered, this, [this]() {
        m_canvas->setDrawingTool(GraphicalCanvas::DrawingTool::Point);
    });
    connect(lineAction, &QAction::triggered, this, [this]() {
        m_canvas->setDrawingTool(GraphicalCanvas::DrawingTool::Line);
    });
    connect(rectangleAction, &QAction::triggered, this, [this]() {
        m_canvas->setDrawingTool(GraphicalCanvas::DrawingTool::Rectangle);
    });
    connect(circleAction, &QAction::triggered, this, [this]() {
        m_canvas->setDrawingTool(GraphicalCanvas::DrawingTool::Circle);
    });
    connect(rectangleCornerAction, &QAction::triggered, this,
        [this, rectangleAction]() {
            m_canvas->setRectangleDrawingMode(GraphicalCanvas::RectangleDrawingMode::CornerToCorner);
            rectangleAction->setChecked(true);
            m_canvas->setDrawingTool(GraphicalCanvas::DrawingTool::Rectangle);
            statusBar()->showMessage(QStringLiteral("矩形绘制模式：角点-角点"), 3000);
        });
    connect(rectangleCenterAction, &QAction::triggered, this,
        [this, rectangleAction]() {
            m_canvas->setRectangleDrawingMode(GraphicalCanvas::RectangleDrawingMode::CenterOutward);
            rectangleAction->setChecked(true);
            m_canvas->setDrawingTool(GraphicalCanvas::DrawingTool::Rectangle);
            statusBar()->showMessage(QStringLiteral("矩形绘制模式：中心向外"), 3000);
        });
    connect(circleBoxAction, &QAction::triggered, this,
        [this, circleAction]() {
            m_canvas->setCircleDrawingMode(GraphicalCanvas::CircleDrawingMode::BoundingBox);
            circleAction->setChecked(true);
            m_canvas->setDrawingTool(GraphicalCanvas::DrawingTool::Circle);
            statusBar()->showMessage(QStringLiteral("圆绘制模式：外接框"), 3000);
        });
    connect(circleCenterAction, &QAction::triggered, this,
        [this, circleAction]() {
            m_canvas->setCircleDrawingMode(GraphicalCanvas::CircleDrawingMode::CenterRadius);
            circleAction->setChecked(true);
            m_canvas->setDrawingTool(GraphicalCanvas::DrawingTool::Circle);
            statusBar()->showMessage(QStringLiteral("圆绘制模式：圆心-半径"), 3000);
        });
    connect(arcAction, &QAction::triggered, this, [this]() {//底部提示
        m_canvas->setDrawingTool(GraphicalCanvas::DrawingTool::Arc);
        statusBar()->showMessage(QStringLiteral("请依次点击圆弧起点、弧上点和终点"));
    });
    connect(deleteAction, &QAction::triggered, m_canvas, &GraphicalCanvas::deleteSelectedFeatures);
    connect(m_canvas, &GraphicalCanvas::featuresChanged, this, &GraphicalProgramEditor::refreshFeatureList);
    connect(m_canvas, &GraphicalCanvas::featuresChanged, this, &GraphicalProgramEditor::refreshMeasurementRecords);
    connect(m_canvas, &GraphicalCanvas::featureGeometryChanged, this,
        [this](int featureId) {
            for (MeasurementRecord& record : m_records) {
                if (record.geometryId != featureId && record.secondaryGeometryId != featureId) continue;
                record.pixelRadius = -1;
                record.trialAngle = -1;
                record.trialStatus = QStringLiteral("未执行（图形已修改）");
                record.detectedEdges = QPainterPath();
                record.fittedArc = QPainterPath();
            }
            refreshMeasurementRecords();
            m_canvas->setDetectionOverlay(QPainterPath(), QPainterPath());
            if (featureId == m_selectedFeatureId)
                refreshFeatureProperties(featureId);
            statusBar()->showMessage(QStringLiteral("特征 %1 的图像坐标已更新").arg(featureId), 3000);
        });
    connect(m_canvas, &GraphicalCanvas::canvasMessage, this,
        [this](const QString& message) { statusBar()->showMessage(message, 5000); });
    connect(m_featureList, &QListWidget::currentItemChanged, this,
        [this](QListWidgetItem* currentItem) {
            if (!currentItem)
                return;
            const int featureId = currentItem->data(Qt::UserRole).toInt();
            if (featureId > 0)
                m_canvas->selectFeatureById(featureId);
        });
    connect(m_canvas, &GraphicalCanvas::selectedFeatureChanged, this,//重关联机制
        [this](int featureId) {
            refreshFeatureProperties(featureId);
            if (m_relinkSequence > 0 && featureId > 0 && m_selectedFeatureId == featureId) {
                const int targetSequence = m_relinkSequence;
                for (int row = 0; row < m_records.size(); ++row) {
                    if (m_records[row].sequence != targetSequence) continue;
                    if (m_records[row].type == QStringLiteral("角度")) {
                        GraphicalCanvas::MeasurementRoi roi;
                        if (!m_canvas->measurementRoi(featureId, roi)) {
                            QMessageBox::warning(this, QStringLiteral("不能关联"),
                                QStringLiteral("角度的ROI必须是宽高至少2px的矩形或半径至少1px的圆。请重新选择；Esc取消。"));
                            return;
                        }
                        const int otherGeometry = m_relinkSlot == 2
                            ? m_records[row].geometryId : m_records[row].secondaryGeometryId;
                        if (otherGeometry == featureId) {
                            QMessageBox::warning(this, QStringLiteral("不能关联"),
                                QStringLiteral("ROI 1 和 ROI 2 不能选择同一个图形。请重新选择；Esc取消。"));
                            return;
                        }
                    }
                    const int completedSlot = m_relinkSlot;
                    if (completedSlot == 2) m_records[row].secondaryGeometryId = featureId;
                    else m_records[row].geometryId = featureId;
                    m_records[row].pixelRadius = -1;
                    m_records[row].trialAngle = -1;
                    m_records[row].trialStatus = QStringLiteral("未执行（关联已修改）");
                    m_records[row].detectedEdges = QPainterPath();
                    m_records[row].fittedArc = QPainterPath();
                    cancelRelink();
                    refreshMeasurementRecords();
                    QSignalBlocker blocker(m_stepTable);
                    m_stepTable->setCurrentCell(row, 0);
                    m_stepTable->selectRow(row);
                    statusBar()->showMessage(QStringLiteral("记录 %1 的 ROI %2 已关联图形 %3；序号、类型和公差保持不变。")
                        .arg(targetSequence).arg(completedSlot).arg(featureId));
                    break;
                }
            }
            {
                QSignalBlocker tableBlocker(m_stepTable);
                const int current = m_stepTable->currentRow();
                if (current < 0 || current >= m_records.size()
                    || (m_records[current].geometryId != featureId
                        && m_records[current].secondaryGeometryId != featureId)) {
                    m_stepTable->clearSelection();
                    m_stepTable->setCurrentCell(-1, -1);
                    for (int row = 0; featureId > 0 && row < m_records.size(); ++row) {
                        if (m_records[row].geometryId == featureId
                            || m_records[row].secondaryGeometryId == featureId) {
                            m_stepTable->setCurrentCell(row, 0);
                            m_stepTable->selectRow(row);
                            break;
                        }
                    }
                }
                const int selectedRow = m_stepTable->currentRow();
                if (selectedRow >= 0 && selectedRow < m_records.size()) {
                    const MeasurementRecord& record = m_records[selectedRow];
                    m_measurementType->setCurrentText(record.type);
                    m_angleResultMode->setCurrentIndex(record.useSupplementaryAngle ? 1 : 0);
                    m_featureNumber->setText(record.featureNumber);
                    m_hasTolerance->setChecked(record.hasTolerance);
                    m_nominal->setValue(record.nominal);
                    m_lowerDeviation->setValue(record.lower);
                    m_upperDeviation->setValue(record.upper);
                }
            }
            QSignalBlocker blocker(m_featureList);
            showRecordDetection(m_stepTable->currentRow());
            m_featureList->clearSelection();
            m_featureList->setCurrentItem(nullptr);
            for (int row = 0; row < m_featureList->count(); ++row) {
                QListWidgetItem* item = m_featureList->item(row);
                if (item->data(Qt::UserRole).toInt() == featureId) {
                    m_featureList->setCurrentItem(item);
                    item->setSelected(true);
                    m_featureList->scrollToItem(item);
                    break;
                }
            }
        });
    connect(m_featureList, &QListWidget::customContextMenuRequested, this,
        [this](const QPoint& position) {
            QListWidgetItem* item = m_featureList->itemAt(position);
            if (!item)
                return;
            const int featureId = item->data(Qt::UserRole).toInt();
            if (featureId <= 0)
                return;

            m_featureList->setCurrentItem(item);
            m_canvas->selectFeatureById(featureId);
            QMenu menu(m_featureList);
            QAction* locateAction = menu.addAction(QStringLiteral("定位并高亮"));
            QAction* deleteFeatureAction = menu.addAction(QStringLiteral("删除图形"));
            QAction* chosenAction = menu.exec(m_featureList->viewport()->mapToGlobal(position));
            if (chosenAction == locateAction)
                m_canvas->selectFeatureById(featureId);
            else if (chosenAction == deleteFeatureAction)
                m_canvas->deleteFeatureById(featureId);
        });
    statusBar()->showMessage(QStringLiteral("请打开本地图像开始编辑"));
}

void GraphicalProgramEditor::saveMeasurementRecord(bool update)////新增或者更新按钮公用这个函数。校验：特征号必填、下偏差<=上偏差
{
    if (m_relinkSequence > 0) {
        QMessageBox::warning(this, QStringLiteral("正在关联"), QStringLiteral("请先选择目标图形或取消关联，再新增/更新参数。"));
        return;
    }
    const int row = m_stepTable->currentRow();
    if (update && (row < 0 || row >= m_records.size())) {
        QMessageBox::warning(this, QStringLiteral("未更新"), QStringLiteral("请先在下方选中一条记录。"));
        return;
    }
    if (m_featureNumber->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("未记录"), QStringLiteral("请输入工序特征号。"));
        return;
    }
    if (m_hasTolerance->isChecked() && m_lowerDeviation->value() > m_upperDeviation->value()) {
        QMessageBox::warning(this, QStringLiteral("未记录"), QStringLiteral("下偏差不能大于上偏差。"));
        return;
    }
    MeasurementRecord record;
    record.sequence = update ? m_records[row].sequence : m_nextRecordSequence++;
    record.geometryId = update ? m_records[row].geometryId : m_selectedFeatureId;
    record.secondaryGeometryId = update ? m_records[row].secondaryGeometryId : -1;
    record.featureNumber = m_featureNumber->text().trimmed();
    record.type = m_measurementType->currentText();
    record.useSupplementaryAngle = m_angleResultMode->currentIndex() == 1;
    record.hasTolerance = m_hasTolerance->isChecked();
    record.nominal = m_nominal->value();
    record.lower = m_lowerDeviation->value();
    record.upper = m_upperDeviation->value();
    if (update) m_records[row] = record;
    else m_records.append(record);
    refreshMeasurementRecords();
    m_stepTable->setCurrentCell(update ? row : m_records.size() - 1, 0);
    statusBar()->showMessage(QStringLiteral("记录 %1 已配置；算法未执行，点位未采集，尚未保存到文件。")
        .arg(record.sequence));
}

void GraphicalProgramEditor::refreshMeasurementRecords()//把 m_records 刷到表格；同时检查关联图形有没有被删掉（删了就标"关联已删除"并清空试测结果）。
{
    QSignalBlocker blocker(m_stepTable);
    const int previousRow = m_stepTable->currentRow();
    m_stepTable->setRowCount(m_records.size());
    for (int row = 0; row < m_records.size(); ++row) {
        MeasurementRecord& record = m_records[row];
        const QStringList geometry = m_canvas->featureProperties(record.geometryId);
        const QStringList secondaryGeometry = m_canvas->featureProperties(record.secondaryGeometryId);
        const bool primaryMissing = record.geometryId > 0 && geometry.size() != 3;
        const bool secondaryMissing = record.type == QStringLiteral("角度")
            && record.secondaryGeometryId > 0 && secondaryGeometry.size() != 3;
        if (primaryMissing || secondaryMissing) {
            record.pixelRadius = -1;
            record.trialAngle = -1;
            record.trialStatus = QStringLiteral("未执行（关联已删除）");
            record.detectedEdges = QPainterPath();
            record.fittedArc = QPainterPath();
        }
        QString association;
        if (record.type == QStringLiteral("角度")) {
            const QString first = record.geometryId <= 0 ? QStringLiteral("待关联")
                : geometry.size() == 3 ? geometry[0] : QStringLiteral("已删除");
            const QString second = record.secondaryGeometryId <= 0 ? QStringLiteral("待关联")
                : secondaryGeometry.size() == 3 ? secondaryGeometry[0] : QStringLiteral("已删除");
            association = QStringLiteral("ROI1:%1；ROI2:%2").arg(first, second);
        }
        else association = record.geometryId <= 0 ? QStringLiteral("待关联")
            : geometry.size() == 3 ? geometry[0] : QStringLiteral("关联图形已删除");
        const QString unit = record.type == QStringLiteral("角度") ? QStringLiteral("°")
            : record.type == QStringLiteral("粗糙度") ? QStringLiteral("μm") : QStringLiteral("mm");
        const QStringList cells = QStringList() << QString::number(record.sequence)
            << record.featureNumber << record.type << association
            << (record.hasTolerance ? QString::number(record.nominal, 'f', 4) : QStringLiteral("未设置"))
            << (record.hasTolerance ? QString::number(record.lower, 'f', 4) : QStringLiteral("—"))
            << (record.hasTolerance ? QString::number(record.upper, 'f', 4) : QStringLiteral("—"))
            << unit << (record.trialAngle >= 0 ? QStringLiteral("%1 °").arg(record.trialAngle, 0, 'f', 4)
                : record.pixelRadius > 0 ? QStringLiteral("%1 px").arg(record.pixelRadius, 0, 'f', 4) : QStringLiteral("—"))
            << QStringLiteral("未判定") << QStringLiteral("未采集") << record.trialStatus;
        for (int column = 0; column < cells.size(); ++column) {
            QTableWidgetItem* cell = new QTableWidgetItem(cells[column]);
            cell->setToolTip(cells[column]);
            m_stepTable->setItem(row, column, cell);
        }
    }
    if (previousRow >= 0 && previousRow < m_records.size())
        m_stepTable->setCurrentCell(previousRow, 0);
    showRecordDetection(m_stepTable->currentRow());
}

void GraphicalProgramEditor::loadMeasurementRecord(int row)
{
    if (row < 0 || row >= m_records.size()) return;
    const MeasurementRecord record = m_records[row];
    m_measurementType->setCurrentText(record.type);
    m_angleResultMode->setCurrentIndex(record.useSupplementaryAngle ? 1 : 0);
    m_featureNumber->setText(record.featureNumber);
    m_hasTolerance->setChecked(record.hasTolerance);
    m_nominal->setValue(record.nominal);
    m_lowerDeviation->setValue(record.lower);
    m_upperDeviation->setValue(record.upper);
    m_canvas->selectFeatureById(record.geometryId);
    // Selection signals may clear the table for a missing/unassociated geometry.
    QSignalBlocker blocker(m_stepTable);
    m_stepTable->setCurrentCell(row, 0);
    m_stepTable->selectRow(row);
    showRecordDetection(row);
    statusBar()->showMessage(QStringLiteral("记录 %1 / 特征 %2：%3；未判定，点位未采集。")
        .arg(record.sequence).arg(record.featureNumber).arg(record.trialStatus));
}

void GraphicalProgramEditor::showRecordDetection(int row)
{
    if (row >= 0 && row < m_records.size())
        m_canvas->setDetectionOverlay(m_records[row].detectedEdges, m_records[row].fittedArc);
    else m_canvas->setDetectionOverlay(QPainterPath(), QPainterPath());
}

void GraphicalProgramEditor::trialSelectedRecord()
{
    stopOwnedAxis();
    refreshAxisPanel();
    if (m_ownedAxis > 0) {
        statusBar()->showMessage(QStringLiteral("请等待轴停止后再试测。"));
        return;
    }
    const int row = m_stepTable->currentRow();
    if (m_trialRunning || m_relinkSequence > 0) return;
    if (row < 0 || row >= m_records.size()) {
        QMessageBox::warning(this, QStringLiteral("不能试测"), QStringLiteral("请先选中一条已保存到列表的测量记录。"));
        return;
    }
    GraphicalCanvas::MeasurementRoi roi, secondaryRoi;
    QString validationError;
    const bool angleTrial = m_records[row].type == QStringLiteral("角度");
    if (m_records[row].type != QStringLiteral("圆弧半径") && !angleTrial)
        validationError = QStringLiteral("当前仅接入圆弧半径和双ROI角度试测。长度不能由单条边缘的可见长度可靠代替。其他类型算法尚未接入。");
    else if (!m_canvas->hasImage())
        validationError = QStringLiteral("请先打开图像。");
    else if (m_records[row].geometryId <= 0)
        validationError = angleTrial ? QStringLiteral("角度记录尚未关联 ROI 1。")
            : QStringLiteral("当前记录尚未关联图形。请点击重新关联图形，再选择矩形或圆。");
    else if (angleTrial && m_records[row].secondaryGeometryId <= 0)
        validationError = QStringLiteral("角度记录尚未关联 ROI 2。");
    else if (angleTrial && m_records[row].geometryId == m_records[row].secondaryGeometryId)
        validationError = QStringLiteral("ROI 1 和 ROI 2 不能是同一个图形。");
    else if (m_canvas->featureProperties(m_records[row].geometryId).size() != 3)
        validationError = angleTrial ? QStringLiteral("ROI 1 已删除，请重新关联。")
            : QStringLiteral("关联图形已删除。请重新关联一个矩形或圆。");
    else if (angleTrial && m_canvas->featureProperties(m_records[row].secondaryGeometryId).size() != 3)
        validationError = QStringLiteral("ROI 2 已删除，请重新关联。");
    else if (!m_canvas->measurementRoi(m_records[row].geometryId, roi))
        validationError = QStringLiteral("ROI须为矩形或圆：矩形宽高至少2px，圆半径至少1px。点、直线、圆弧尚不作为面积ROI支持。");
    else if (angleTrial && !m_canvas->measurementRoi(m_records[row].secondaryGeometryId, secondaryRoi))
        validationError = QStringLiteral("ROI 2须为矩形或圆：矩形宽高至少2px，圆半径至少1px。");
    if (!validationError.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("不能试测"), validationError);
        return;
    }
    m_records[row].pixelRadius = -1;
    m_records[row].trialAngle = -1;
    m_records[row].detectedEdges = QPainterPath();
    m_records[row].fittedArc = QPainterPath();
    m_records[row].trialStatus = QStringLiteral("计算中");
    refreshMeasurementRecords();
    m_trialRunning = true;
    centralWidget()->setEnabled(false);
    for (QToolBar* toolbar : findChildren<QToolBar*>())
        if (toolbar->objectName() != QStringLiteral("graphicalAxisSafetyBar")) toolbar->setEnabled(false);
    statusBar()->showMessage(angleTrial
        ? QStringLiteral("正在后台分别拟合 ROI 1 和 ROI 2 的目标直线并计算夹角；不作合格判定。")
        : QStringLiteral("正在后台计算圆弧半径；使用列表中已提交的记录，未标定、不作合格判定。"));
    const QImage source = m_canvas->sourceImage();
    if (angleTrial) {
        const bool supplementary = m_records[row].useSupplementaryAngle;
        const auto result = std::make_shared<LineTrialResult>();
        QThread* worker = QThread::create([source, roi, secondaryRoi, supplementary, result]() {
            *result = runTwoRoiAngleTrial(source, roi, secondaryRoi, supplementary);
        });
        connect(worker, &QThread::finished, this, [this, row, result]() {
            m_trialRunning = false;
            centralWidget()->setEnabled(true);
            for (QToolBar* toolbar : findChildren<QToolBar*>()) toolbar->setEnabled(true);
            if (row < m_records.size()) {
                m_records[row].trialAngle = result->angle;
                m_records[row].trialStatus = result->status;
                m_records[row].detectedEdges = result->edges;
                m_records[row].fittedArc = result->fitted;
            }
            refreshMeasurementRecords();
            m_stepTable->setCurrentCell(row, 0);
            showRecordDetection(row);
            statusBar()->showMessage(result->status + (result->angle >= 0
                ? QStringLiteral("：角度 %1°；未判定").arg(result->angle, 0, 'f', 4) : QString()));
        });
        connect(worker, &QThread::finished, worker, &QObject::deleteLater);
        worker->start();
        return;
    }
    const auto result = std::make_shared<ArcTrialResult>();
    QThread* worker = QThread::create([source, roi, result]() { *result = runArcTrial(source, roi); });
    connect(worker, &QThread::finished, this, [this, row, result]() {
        m_trialRunning = false;
        centralWidget()->setEnabled(true);
        for (QToolBar* toolbar : findChildren<QToolBar*>()) toolbar->setEnabled(true);
        if (row < m_records.size()) {
            m_records[row].pixelRadius = result->radius;
            m_records[row].trialStatus = result->status;
            m_records[row].detectedEdges = result->edges;
            m_records[row].fittedArc = result->fitted;
        }
        refreshMeasurementRecords();
        m_stepTable->setCurrentCell(row, 0);
        showRecordDetection(row);
        statusBar()->showMessage(result->status + (result->radius > 0
            ? QStringLiteral("：半径 %1 px；未判定").arg(result->radius, 0, 'f', 4) : QString()));
    });
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start();
}

void GraphicalProgramEditor::cancelRelink()
{
    if (m_relinkSequence <= 0) return;
    m_relinkSequence = -1;
    m_stepTable->setEnabled(true);
    if (QShortcut* shortcut = findChild<QShortcut*>(QStringLiteral("cancelRelinkShortcut")))
        shortcut->setEnabled(false);
    statusBar()->showMessage(QStringLiteral("已结束关联操作；未选择新图形时原关联保持不变。"), 4000);
}

void GraphicalProgramEditor::closeEvent(QCloseEvent* event)
{
    stopOwnedAxis();
    refreshAxisPanel();
    if (m_ownedAxis > 0) {
        statusBar()->showMessage(QStringLiteral("已请求停止，请确认轴停止后再次关闭；通讯失败时使用设备物理急停。"));
        event->ignore();
        return;
    }
    if (m_trialRunning) {
        statusBar()->showMessage(QStringLiteral("算法计算中，请等待结束后关闭。"));
        event->ignore();
        return;
    }
    cancelRelink();
    if (!m_records.isEmpty() && QMessageBox::question(this, QStringLiteral("关闭图形化编程"),
        QStringLiteral("测量记录仅在当前窗口暂存，尚未接入文件保存。是否仍关闭窗口？")) != QMessageBox::Yes) {
        event->ignore();
        return;
    }
    QMainWindow::closeEvent(event);
}

void GraphicalProgramEditor::openLocalImage()
{
    cancelRelink();
    const QString filePath = QFileDialog::getOpenFileName(
        this, QStringLiteral("打开测量图像"), QString(),
        QStringLiteral("图像文件 (*.bmp *.png *.jpg *.jpeg *.tif *.tiff);;所有文件 (*.*)"));
    if (filePath.isEmpty())
        return;

    if (!m_records.isEmpty() && QMessageBox::question(this, QStringLiteral("更换图像"),
        QStringLiteral("当前记录尚未保存。更换图像将清空记录和图形，是否继续？")) != QMessageBox::Yes)
        return;

    if (!m_canvas->loadImage(filePath)) {
        QMessageBox::warning(this, QStringLiteral("打开图像失败"),
            QStringLiteral("无法读取所选图像，请检查文件格式或文件是否损坏。"));
        return;
    }

    //打开成功后隐藏空态提示
    if (QLabel* hint = m_canvas->findChild<QLabel*>(QStringLiteral("canvasEmptyHint")))
        hint->hide();

    m_records.clear();
    m_nextRecordSequence = 1;
    refreshMeasurementRecords();
    statusBar()->showMessage(QStringLiteral("图像已打开：%1 × %2 像素")
        .arg(m_canvas->sourceImage().width())
        .arg(m_canvas->sourceImage().height()));
}

void GraphicalProgramEditor::refreshFeatureList()
{
    refreshFeatureProperties(-1);
    m_featureList->clear();
    const QList<QPair<int, QString>> featureEntries = m_canvas->featureEntries();
    if (featureEntries.isEmpty()) {
        m_featureList->addItem(QStringLiteral("尚未创建图形特征"));
        m_featureList->setEnabled(false);
        return;
    }

    for (const QPair<int, QString>& featureEntry : featureEntries) {
        QListWidgetItem* item = new QListWidgetItem(featureEntry.second, m_featureList);
        item->setData(Qt::UserRole, featureEntry.first);
    }
    m_featureList->setEnabled(true);
}

void GraphicalProgramEditor::refreshFeatureProperties(int featureId)
{
    const QStringList properties = m_canvas->featureProperties(featureId);
    m_selectedFeatureId = properties.size() == 3 ? featureId : -1;
    m_featureNameLabel->setText(properties.size() == 3 ? properties.at(0) : QStringLiteral("未选择"));
    m_featureTypeLabel->setText(properties.size() == 3 ? properties.at(1) : QStringLiteral("-"));
    m_coordinateLabel->setText(properties.size() == 3 ? properties.at(2) : QStringLiteral("-"));
    const QSizeF dimensions = m_canvas->featureDimensions(m_selectedFeatureId);
    const QString type = properties.size() == 3 ? properties.at(1) : QString();
    const bool rectangle = type == QStringLiteral("矩形");
    const bool rotatable = rectangle || type == QStringLiteral("直线") || type == QStringLiteral("圆弧");
    m_rotationLabel->setText(type == QStringLiteral("直线") ? QStringLiteral("目标方向角：") : QStringLiteral("目标旋转角："));
    m_rotationAngle->setEnabled(rotatable);
    m_applyRotation->setEnabled(rotatable);
    QSignalBlocker blockRotation(m_rotationAngle);
    m_rotationAngle->setValue(rotatable ? m_canvas->featureRotationAngle(m_selectedFeatureId) : 0.0);
    const bool supported = dimensions.width() > 0 && (!rectangle || dimensions.height() > 0);
    m_primarySizeLabel->setText(rectangle ? QStringLiteral("宽度：")
        : type == QStringLiteral("直线") ? QStringLiteral("长度：")
        : (type == QStringLiteral("圆") || type == QStringLiteral("圆弧"))
        ? QStringLiteral("半径：") : QStringLiteral("尺寸："));
    m_primarySize->setEnabled(supported);
    m_secondarySize->setEnabled(supported && rectangle);
    m_lockRatio->setEnabled(supported && rectangle);
    m_applySize->setEnabled(supported);
    QSignalBlocker blockPrimary(m_primarySize);
    QSignalBlocker blockSecondary(m_secondarySize);
    m_primarySize->setValue(supported ? dimensions.width() : m_primarySize->minimum());
    m_secondarySize->setValue(supported && rectangle ? dimensions.height() : m_secondarySize->minimum());
    m_sizeRatio = supported && rectangle ? dimensions.width() / dimensions.height() : 1.0;
}
