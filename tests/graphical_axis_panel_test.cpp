// Offline UI checks. The backend below never calls any device SDK.
// Link editor/canvas objects with /DELAYLOAD:halconcpp.dll; algorithms are not executed.
#include "graphical_program_editor.h"
#include "graphical_sensor_measurement.h"
#include "graphical_program_contract.h"
#include "graphical_program_registry.h"
#include <QApplication>
#include <QPushButton>
#include <QComboBox>
#include <QToolBar>
#include <QtTest/QTest>
#include <QEvent>
#include <QFile>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QTableWidget>
#include <QTabWidget>
#include <QLabel>
#include <QTemporaryDir>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <limits>
#include <algorithm>
#include <iostream>
#include <stdexcept>

static void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

static void testCornerGeometry()
{
    require(std::abs(cornerPointSegmentDistance(QPointF(5, 2), QPointF(0, 0), QPointF(10, 0)) - 2) < 1e-9,
        "point-to-segment interior distance");
    require(std::abs(cornerPointSegmentDistance(QPointF(12, 0), QPointF(0, 0), QPointF(10, 0)) - 2) < 1e-9,
        "point-to-segment endpoint distance");
    QVector<double> mostlyStraight(9, 1.0);
    mostlyStraight.append(100.0);
    require(cornerDeviationPercentile(mostlyStraight) == 1.0,
        "P90 deviation must ignore one outlier in ten points");
    mostlyStraight[8] = 100.0;
    require(cornerDeviationPercentile(mostlyStraight) == 100.0,
        "P90 deviation must reject two outliers in ten points");
    require(!std::isfinite(cornerDeviationPercentile({ 1.0, std::numeric_limits<double>::quiet_NaN() })),
        "nonfinite deviations must be rejected");
    GraphicalCornerEdge first, second;
    first.first = QPointF(10, 10); first.second = QPointF(50, 10);
    second.first = QPointF(10, 10); second.second = QPointF(30, 30);
    GraphicalCornerPair pair;
    require(cornerPairGeometry(first, second, 0, pair), "adjacent corner must be accepted");
    require(std::abs(pair.smallerAngle - 45) < 1e-9, "known 45 degree corner");
    require(!cornerPairOverlay(first, second, pair, false).isEmpty(), "corner overlay required");
    require(!cornerPairOverlay(first, second, pair, true).isEmpty(), "supplement overlay required");
    std::swap(second.first, second.second);
    require(cornerPairGeometry(first, second, 0, pair) && std::abs(pair.smallerAngle - 45) < 1e-9, "endpoint reversal must preserve angle");
    second.first = QPointF(10, 10); second.second = QPointF(10, 50);
    require(cornerPairGeometry(first, second, 0, pair) && std::abs(pair.smallerAngle - 90) < 1e-9, "right angle");
    second.first = QPointF(10, 12); second.second = QPointF(10, 50);
    require(!cornerPairGeometry(first, second, 1, pair) && cornerPairGeometry(first, second, 2, pair), "gap threshold must be enforced");
    second.first = QPointF(10, 20); second.second = QPointF(50, 20);
    require(!cornerPairGeometry(first, second, 100, pair), "parallel lines must be rejected");
    second.first = second.second;
    require(!cornerPairGeometry(first, second, 100, pair), "zero length must be rejected");
    second.first.setX(std::numeric_limits<double>::quiet_NaN());
    require(!cornerPairGeometry(first, second, 100, pair), "nonfinite edges must be rejected");
    std::cout << "PASS: robust P90 deviation, corner angle, endpoint reversal, right angle, gap limits, parallel/degenerate/nonfinite rejection, overlays\n";
}

static void testSensorMeasurementAdapter()
{
    const auto diameterContract = GraphicalProgramGeneration::contractForType(QStringLiteral("直径"));
    require(diameterContract.supported && !diameterContract.requiresImage
        && diameterContract.axes == QVector<int>({ 5 })
        && diameterContract.legacyWorksheet == QStringLiteral("zhijing"),
        "diameter generation contract must use the light curtain axis");
    const auto holeContract = GraphicalProgramGeneration::contractForType(QStringLiteral("孔径"));
    require(holeContract.requiresImage && holeContract.requiresCalibration
        && holeContract.cameraIndex == 1 && holeContract.axes == QVector<int>({ 2, 5 }),
        "hole generation contract must preserve camera and both motion axes");
    const auto roundoutContract = GraphicalProgramGeneration::contractForType(QStringLiteral("跳动"));
    require(roundoutContract.threeSectionScan && roundoutContract.requiresTwoReferences
        && roundoutContract.legacyWorksheet == QStringLiteral("tiaodong"),
        "roundout generation contract must preserve sections and references");
    const QStringList supportedTypes = { QStringLiteral("直径"), QStringLiteral("孔径"),
        QStringLiteral("圆柱度"), QStringLiteral("跳动"), QStringLiteral("长度"),
        QStringLiteral("角度"), QStringLiteral("圆弧半径") };
    for (const QString& type : supportedTypes)
        require(GraphicalProgramGeneration::contractForType(type).supported,
            "every visible measurement type must have a generation contract");
    require(GraphicalProgramGeneration::isReservedProgramNumber(60)
        && !GraphicalProgramGeneration::isReservedProgramNumber(61),
        "new graphical programs must not reuse existing source slots");
    std::cout << "PASS: seven measurement generation contracts and reserved program slots\n";
    const auto positions = GraphicalSensorMeasurement::axialPositions(1000, 100, 200);
    require(positions.ok && positions.lower == 900 && positions.middle == 1000 && positions.upper == 1200,
        "sensor axial offsets must preserve lower/upper meaning");
    require(!GraphicalSensorMeasurement::axialPositions(1000, 0, 200).ok,
        "zero sensor offset must be rejected");

    const auto diameter = GraphicalSensorMeasurement::diameterMean({ 10.0, 12.0 });
    require(diameter.ok && std::abs(diameter.value - 11.0) < 1e-12,
        "diameter must average compensated samples");
    require(!GraphicalSensorMeasurement::diameterMean({ 10.0,
        std::numeric_limits<double>::quiet_NaN() }).ok,
        "diameter must reject nonfinite samples");

    std::array<QVector<double>, 3> cylinder;
    for (int section = 0; section < 3; ++section)
        for (int sample = 0; sample < 15; ++sample)
            cylinder[section].append(20.0 + section + sample * 0.01);
    const auto cylindricity = GraphicalSensorMeasurement::cylindricity(cylinder);
    require(cylindricity.ok && cylindricity.value >= 0,
        "complete three-section cylindricity samples must be accepted");
    cylinder[1].clear();
    require(!GraphicalSensorMeasurement::cylindricity(cylinder).ok,
        "incomplete cylindricity section must be rejected");

    QVector<double> radii(25, 10.0), distances;
    QString error;
    require(GraphicalSensorMeasurement::roundoutDistances(
        radii, 0, 0, 0, 0, true, distances, error) && distances.size() == radii.size(),
        "roundout distance conversion must accept a complete revolution");
    const auto roundout = GraphicalSensorMeasurement::roundoutFromDistances(distances);
    require(roundout.ok && std::abs(roundout.value) < 1e-9,
        "constant radius must have zero roundout");
    require(!GraphicalSensorMeasurement::roundoutFromDistances(QVector<double>(24, 1.0)).ok,
        "roundout trim must reject insufficient samples");
    const auto representative = GraphicalSensorMeasurement::representativeRoundout({ 0.3, 0.1, 0.2 });
    require(representative.ok && std::abs(representative.value - 0.2) < 1e-12,
        "roundout representative value must be the three-section median");

    require(!GraphicalSensorMeasurement::axisPointAtZ(
        { 0, 0, 0 }, { 1, 0, 0 }, 10).ok,
        "degenerate reference axis must be rejected");
    const auto axisPoint = GraphicalSensorMeasurement::axisPointAtZ(
        { 1, 2, 3 }, { 0, 0, 2 }, 7);
    require(axisPoint.ok && axisPoint.x == 1 && axisPoint.y == 2 && axisPoint.z == 7,
        "reference axis projection must preserve a vertical axis");

    int polls = 0, stops = 0;
    const auto timeout = GraphicalSensorMeasurement::waitForMotion(
        [&]() { ++polls; return GraphicalSensorMotionState{ true, true, false, false, QString() }; },
        [&]() { ++stops; return true; }, []() { return false; }, [](int) {}, 20, 10);
    require(!timeout.ok && timeout.stopAttempted && timeout.stopSucceeded && stops == 1,
        "motion timeout must request one successful stop");
    const auto arrived = GraphicalSensorMeasurement::waitForMotion(
        []() { return GraphicalSensorMotionState{ true, false, true, false, QString() }; },
        [&]() { ++stops; return true; }, []() { return false; }, [](int) {}, 20, 10);
    require(arrived.ok && !arrived.stopAttempted,
        "confirmed arrival must finish without an extra stop");
    std::cout << "PASS: sensor axial mapping, diameter/cylindricity/roundout guards, reference axis and motion stop policy\n";
}

static void testDetectionRecords()
{
    GraphicalDetectionParameters parameters;
    require(parameters.validationError().isEmpty(), "legacy defaults must be valid");
    require(parameters.cornerMaxDeviation == 2.5 && parameters.cornerMaxGap == 20,
        "single ROI defaults must tolerate a small rounded transition");
    parameters.lowThreshold = parameters.highThreshold;
    require(!parameters.validationError().isEmpty(), "equal thresholds must be rejected");
    parameters = GraphicalDetectionParameters();
    parameters.maxLength = parameters.minLength - 1;
    require(!parameters.validationError().isEmpty(), "reversed length range must be rejected");
    parameters.maxLength = 0;
    parameters.smoothing = std::numeric_limits<double>::quiet_NaN();
    require(!parameters.validationError().isEmpty(), "nonfinite parameter must be rejected");

    GraphicalProgramEditor editor;
    editor.setAttribute(Qt::WA_DontShowOnScreen);
    editor.show();
    QTest::qWait(250);
    const QStringList removedStaticHelp = {
        QStringLiteral("圆弧半径使用一个ROI"),
        QStringLiteral("原图左上角为原点"),
        QStringLiteral("请先开始采集，等待图像稳定"),
        QStringLiteral("填写测量方案信息后检查记录"),
        QStringLiteral("检查通过表示测量方案数据"),
        QStringLiteral("目标是绝对脉冲位置。加减速沿用")
    };
    for (const QLabel* label : editor.findChildren<QLabel*>())
        for (const QString& text : removedStaticHelp)
            require(!label->text().contains(text), "static instructional copy must stay out of the editor UI");
    QLabel* recipeResult = editor.findChild<QLabel*>(QStringLiteral("recipeValidationResult"));
    require(recipeResult && recipeResult->isHidden(),
        "recipe validation output must stay hidden until it has a result");
    auto button = [&](const QString& name) {
        for (auto* value : editor.findChildren<QPushButton*>()) if (value->text() == name) return value;
        throw std::runtime_error("detection button missing");
    };
    auto* type = editor.findChild<QComboBox*>(QStringLiteral("measurementType"));
    auto* axisSelector = editor.findChild<QComboBox*>(QStringLiteral("axisSelector"));
    auto* feature = editor.findChild<QLineEdit*>(QStringLiteral("measurementFeatureNumber"));
    auto* minimum = editor.findChild<QDoubleSpinBox*>(QStringLiteral("detectionMinLength"));
    auto* low = editor.findChild<QDoubleSpinBox*>(QStringLiteral("detectionLow"));
    auto* table = editor.findChild<QTableWidget*>();
    auto* diagnostic = editor.findChild<QLabel*>(QStringLiteral("detectionDiagnostic"));
    auto* lowerAxialOffset = editor.findChild<QSpinBox*>(QStringLiteral("lowerAxialOffsetPulse"));
    auto* upperAxialOffset = editor.findChild<QSpinBox*>(QStringLiteral("upperAxialOffsetPulse"));
    auto* roundoutReference1 = editor.findChild<QLineEdit*>(QStringLiteral("roundoutReference1"));
    auto* roundoutReference2 = editor.findChild<QLineEdit*>(QStringLiteral("roundoutReference2"));
    require(type && axisSelector && feature && minimum && low && table && diagnostic
        && lowerAxialOffset && upperAxialOffset && roundoutReference1 && roundoutReference2,
        "detection UI missing");
    require(type->findText(QStringLiteral("粗糙度")) < 0,
        "graphical workflow must not expose the retired roughness measurement entry");
    require(axisSelector->findData(1) < 0,
        "graphical workflow must not expose the retired roughness axis entry");
    auto enterFeatureNumber = [&](const QString& value) {
        feature->setFocus();
        feature->selectAll();
        QTest::keyClicks(feature, value);
    };
    type->setCurrentText(QStringLiteral("圆弧半径"));
    enterFeatureNumber(QStringLiteral("F1"));
    minimum->setValue(7);
    button(QStringLiteral("新增测量记录"))->click();
    enterFeatureNumber(QStringLiteral("F2"));
    minimum->setValue(13);
    button(QStringLiteral("新增测量记录"))->click();
    require(table->rowCount() == 2, "two measurement records required");
    table->setCurrentCell(0, 0);
    require(minimum->value() == 7, "first record must retain its own parameters");
    minimum->setValue(9); // Unsubmitted draft must not change the record.
    table->setCurrentCell(1, 0);
    require(minimum->value() == 13, "second record must retain its own parameters");
    table->setCurrentCell(0, 0);
    require(minimum->value() == 7, "unsubmitted draft must not alter saved parameters");
    low->setValue(100);
    button(QStringLiteral("应用检测参数"))->click();
    require(diagnostic->text().contains(QStringLiteral("低阈值")), "invalid threshold must explain rejection");
    table->setCurrentCell(1, 0);
    table->setCurrentCell(0, 0);
    require(low->value() == 20, "invalid parameter must not replace committed value");
    enterFeatureNumber(QStringLiteral("UNSUBMITTED"));
    minimum->setValue(8);
    button(QStringLiteral("应用检测参数"))->click();
    require(table->item(0, 1)->text() == QStringLiteral("F1"), "parameter-only apply must preserve feature number");
    require(table->item(0, 11)->text().contains(QStringLiteral("旧结果失效")), "parameter apply must invalidate execution state");
    button(QStringLiteral("恢复默认（未提交）"))->click();
    require(minimum->value() == 20, "reset must display legacy defaults");
    table->setCurrentCell(1, 0);
    table->setCurrentCell(0, 0);
    require(minimum->value() == 8, "reset draft must not overwrite committed parameters");
    for (auto* tabs : editor.findChildren<QTabWidget*>())
        for (int index = 0; index < tabs->count(); ++index)
            if (tabs->tabText(index) == QStringLiteral("检测参数")) tabs->setCurrentIndex(index);
    require(editor.grab().save("x64/detection_parameters.png"), "detection UI screenshot failed");
    auto* inputMode = editor.findChild<QComboBox*>(QStringLiteral("angleInputMode"));
    auto* candidate = editor.findChild<QComboBox*>(QStringLiteral("cornerCandidate"));
    auto* gap = editor.findChild<QDoubleSpinBox*>(QStringLiteral("cornerGap"));
    require(inputMode && candidate && gap, "single ROI UI missing");
    type->setCurrentText(QStringLiteral("角度"));
    inputMode->setCurrentIndex(1);
    enterFeatureNumber(QStringLiteral("CORNER"));
    gap->setValue(3);
    button(QStringLiteral("新增测量记录"))->click();
    require(table->item(2, 3)->text().startsWith(QStringLiteral("单ROI:")), "single ROI association required");
    require(!button(QStringLiteral("角度：选择/重选 ROI 2（直线2）"))->isEnabled(), "single ROI must disable second ROI");
    require(!candidate->isEnabled(), "no computed candidates must not be selectable");
    table->setCurrentCell(0, 0);
    table->setCurrentCell(2, 0);
    require(inputMode->currentIndex() == 1 && gap->value() == 3, "single ROI mode and parameters must survive record switching");
    inputMode->setCurrentIndex(0);
    table->setCurrentCell(0, 0); table->setCurrentCell(2, 0);
    require(inputMode->currentIndex() == 1, "unsubmitted mode must not change record");
    for (auto* tabs : editor.findChildren<QTabWidget*>())
        for (int index = 0; index < tabs->count(); ++index)
            if (tabs->tabText(index) == QStringLiteral("测量配置")) tabs->setCurrentIndex(index);
    QTest::qWait(100);
    require(editor.grab().save("x64/single_roi_angle.png"), "single ROI screenshot failed");
    inputMode->setCurrentIndex(0);
    button(QStringLiteral("更新选中记录"))->click();
    require(table->item(2, 3)->text().contains(QStringLiteral("ROI2:")), "committed mode must switch association");
    require(button(QStringLiteral("角度：选择/重选 ROI 2（直线2）"))->isEnabled(), "double ROI must enable second ROI");
    type->setCurrentText(QStringLiteral("圆柱度"));
    require(lowerAxialOffset->isEnabled() && upperAxialOffset->isEnabled()
        && !lowerAxialOffset->isHidden() && !upperAxialOffset->isHidden()
        && roundoutReference1->isHidden() && roundoutReference2->isHidden(),
        "cylindricity must expose only axial offsets");
    lowerAxialOffset->setValue(100);
    upperAxialOffset->setValue(200);
    enterFeatureNumber(QStringLiteral("CYTEST"));
    button(QStringLiteral("新增测量记录"))->click();
    require(table->rowCount() == 4, "cylindricity record required");
    table->setCurrentCell(3, 0);
    require(lowerAxialOffset->value() == 100 && upperAxialOffset->value() == 200,
        "cylindricity offsets must survive record selection");
    type->setCurrentText(QStringLiteral("跳动"));
    require(!roundoutReference1->isHidden() && !roundoutReference2->isHidden()
        && roundoutReference1->isEnabled() && roundoutReference2->isEnabled(),
        "roundout must expose reference fields");
    auto* programNumber = editor.findChild<QSpinBox*>(QStringLiteral("recipeProgramNumber"));
    require(programNumber && programNumber->minimum() == 0 && programNumber->maximum() == 999,
        "new program number input must allow the graphical generation range");
    programNumber->setValue(12);
    editor.findChild<QLineEdit*>(QStringLiteral("recipePartNumber"))->setText(QStringLiteral("P-001"));
    editor.findChild<QLineEdit*>(QStringLiteral("recipePartName"))->setText(QStringLiteral("测试零件"));
    editor.findChild<QLineEdit*>(QStringLiteral("recipeProcessNumber"))->setText(QStringLiteral("OP10"));
    const QStringList preflightIssues = editor.validateRecipeForExport();
    require(std::any_of(preflightIssues.cbegin(), preflightIssues.cend(), [](const QString& issue) {
        return issue.contains(QStringLiteral("新程序号须从61开始"));
    }), "preflight must reject reserved program numbers");
    require(std::none_of(preflightIssues.cbegin(), preflightIssues.cend(), [](const QString& issue) {
        return issue.contains(QStringLiteral("尚未接入生产程序映射"));
    }), "all current measurement types must have a generation mapping contract");
    std::cout << "PASS: detection defaults, invalid thresholds/ranges/nonfinite input, record isolation, draft discard, parameter-only apply, invalidation state, reset semantics\n";
}

static void testCameraWorkflow()
{
    GraphicalProgramEditor editor;
    editor.setAttribute(Qt::WA_DontShowOnScreen);
    bool connected = false;
    bool capturing = false;
    bool hasFrame = false;
    int starts = 0;
    int stops = 0;
    int snapshots = 0;
    using CameraCommand = GraphicalProgramEditor::CameraCommand;
    editor.setCameraBackend([&](int camera) {
        GraphicalProgramEditor::CameraSnapshot state;
        state.connected = connected;
        state.available = connected;
        state.capturing = capturing;
        state.hasFrame = hasFrame;
        state.exposure = 500;
        state.frameSize = hasFrame ? QSize(64, 48) : QSize();
        state.message = connected
            ? QStringLiteral("模拟相机%1已连接").arg(camera)
            : QStringLiteral("模拟相机%1未连接").arg(camera);
        return state;
    }, [&](int, CameraCommand command, int exposure) {
        GraphicalProgramEditor::CameraCommandResult result;
        if (!connected) {
            result.error = QStringLiteral("模拟相机未连接");
            return result;
        }
        if (command == CameraCommand::StartCapture) {
            ++starts;
            capturing = true;
            hasFrame = false;
        }
        else if (command == CameraCommand::StopCapture) {
            ++stops;
            capturing = false;
            hasFrame = true;
        }
        else {
            ++snapshots;
            result.image = QImage(64, 48, QImage::Format_RGB32);
            result.image.fill(QColor(40, 120, 200));
            result.exposure = exposure;
        }
        return result;
    });
    editor.show();
    QTest::qWait(250);
    auto button = [&](const QString& name) {
        for (auto* value : editor.findChildren<QPushButton*>()) if (value->text() == name) return value;
        throw std::runtime_error("camera button missing");
    };
    auto* start = button(QStringLiteral("开始连续采集"));
    auto* stop = button(QStringLiteral("停止采集"));
    auto* load = button(QStringLiteral("载入最后一帧"));
    auto* cameraSelector = editor.findChild<QComboBox*>(QStringLiteral("cameraSelector"));
    require(cameraSelector && cameraSelector->count() == 2
        && cameraSelector->findData(2) < 0,
        "graphical workflow must only expose telecentric and hole cameras");
    require(!start->isEnabled() && !stop->isEnabled() && !load->isEnabled(),
        "disconnected camera controls must be disabled");

    connected = true;
    QTest::qWait(250);
    require(start->isEnabled() && !stop->isEnabled() && !load->isEnabled(),
        "connected camera must allow capture start only");
    QTest::mouseClick(start, Qt::LeftButton);
    QTest::qWait(250);
    require(starts == 1 && capturing && stop->isEnabled(), "camera start command missing");
    QTest::mouseClick(stop, Qt::LeftButton);
    QTest::qWait(250);
    require(stops == 1 && !capturing && load->isEnabled(), "last frame must become available after stop");
    QTest::mouseClick(load, Qt::LeftButton);
    QTest::qWait(250);
    auto* canvas = editor.findChild<GraphicalCanvas*>();
    require(snapshots == 1 && canvas && canvas->hasImage(),
        "snapshot must be cached and loaded without a save dialog");
    QTemporaryDir recipeDirectory;
    require(recipeDirectory.isValid(), "temporary recipe directory missing");
    const QString recipePath = recipeDirectory.filePath(QStringLiteral("camera.axisproj.json"));
    editor.findChild<QSpinBox*>(QStringLiteral("recipeProgramNumber"))->setValue(27);
    editor.findChild<QLineEdit*>(QStringLiteral("recipePartNumber"))->setText(QStringLiteral("AX-27"));
    editor.findChild<QLineEdit*>(QStringLiteral("recipePartName"))->setText(QStringLiteral("轴类零件"));
    editor.findChild<QLineEdit*>(QStringLiteral("recipeProcessNumber"))->setText(QStringLiteral("20"));
    editor.findChild<QLineEdit*>(QStringLiteral("recipeNote"))->setText(QStringLiteral("离线测量方案测试"));
    QString recipeError;
    require(editor.saveRecipeFile(recipePath, recipeError),
        qPrintable(QStringLiteral("simulated camera recipe save failed: %1").arg(recipeError)));
    const QString assetPath = recipeDirectory.filePath(
        QStringLiteral("camera.axisproj.assets/frame_1_camera_0.png"));
    require(QFileInfo::exists(recipePath) && QFileInfo::exists(assetPath),
        "camera recipe and packaged asset required");
    GraphicalProgramEditor reopened;
    reopened.setAttribute(Qt::WA_DontShowOnScreen);
    require(reopened.loadRecipeFile(recipePath, recipeError),
        qPrintable(QStringLiteral("simulated camera recipe reopen failed: %1").arg(recipeError)));
    auto* reopenedCanvas = reopened.findChild<GraphicalCanvas*>();
    require(reopenedCanvas && reopenedCanvas->hasImage(), "packaged camera image must reopen");
    auto* reopenedSourceBadge = reopened.findChild<QLabel*>(QStringLiteral("canvasSourceBadge"));
    require(reopenedSourceBadge && !reopenedSourceBadge->isHidden()
        && reopenedSourceBadge->text().contains(QStringLiteral("非实时")),
        "reopened recipe image must be visibly identified as non-live");
    require(reopened.findChild<QSpinBox*>(QStringLiteral("recipeProgramNumber"))->value() == 27
        && reopened.findChild<QLineEdit*>(QStringLiteral("recipePartNumber"))->text() == QStringLiteral("AX-27")
        && reopened.findChild<QLineEdit*>(QStringLiteral("recipePartName"))->text() == QStringLiteral("轴类零件")
        && reopened.findChild<QLineEdit*>(QStringLiteral("recipeProcessNumber"))->text() == QStringLiteral("20")
        && reopened.findChild<QLineEdit*>(QStringLiteral("recipeNote"))->text() == QStringLiteral("离线测量方案测试"),
        "recipe metadata must survive save and reopen");
    const QStringList emptyRecipeIssues = reopened.validateRecipeForExport();
    require(emptyRecipeIssues.contains(QStringLiteral("至少需要一条测量记录。")),
        "preflight must reject a recipe without measurement records");
    auto* measurementType = editor.findChild<QComboBox*>(QStringLiteral("measurementType"));
    auto* lowerAxialOffset = editor.findChild<QSpinBox*>(QStringLiteral("lowerAxialOffsetPulse"));
    auto* upperAxialOffset = editor.findChild<QSpinBox*>(QStringLiteral("upperAxialOffsetPulse"));
    auto* reference1 = editor.findChild<QLineEdit*>(QStringLiteral("roundoutReference1"));
    auto* reference2 = editor.findChild<QLineEdit*>(QStringLiteral("roundoutReference2"));
    require(measurementType && lowerAxialOffset && upperAxialOffset && reference1 && reference2,
        "roundout configuration UI missing");
    measurementType->setCurrentText(QStringLiteral("跳动"));
    lowerAxialOffset->setValue(111);
    upperAxialOffset->setValue(222);
    reference1->setText(QStringLiteral("基准A"));
    reference2->setText(QStringLiteral("基准B"));
    button(QStringLiteral("新增测量记录"))->click();
    auto* recordTable = editor.findChild<QTableWidget*>();
    require(recordTable && recordTable->rowCount() == 1, "roundout record required");
    require(editor.saveRecipeFile(recipePath, recipeError),
        qPrintable(QStringLiteral("roundout recipe save failed: %1").arg(recipeError)));
    GraphicalProgramEditor reopenedRoundout;
    reopenedRoundout.setAttribute(Qt::WA_DontShowOnScreen);
    require(reopenedRoundout.loadRecipeFile(recipePath, recipeError),
        qPrintable(QStringLiteral("roundout recipe reopen failed: %1").arg(recipeError)));
    reopenedRoundout.show();
    QTest::qWait(100);
    auto* reopenedTable = reopenedRoundout.findChild<QTableWidget*>();
    require(reopenedTable && reopenedTable->rowCount() == 1, "roundout record must reopen");
    reopenedTable->setCurrentCell(0, 0);
    auto* reopenedType = reopenedRoundout.findChild<QComboBox*>(QStringLiteral("measurementType"));
    auto* reopenedLower = reopenedRoundout.findChild<QSpinBox*>(QStringLiteral("lowerAxialOffsetPulse"));
    auto* reopenedUpper = reopenedRoundout.findChild<QSpinBox*>(QStringLiteral("upperAxialOffsetPulse"));
    auto* reopenedReference1 = reopenedRoundout.findChild<QLineEdit*>(QStringLiteral("roundoutReference1"));
    auto* reopenedReference2 = reopenedRoundout.findChild<QLineEdit*>(QStringLiteral("roundoutReference2"));
    require(reopenedType && reopenedLower && reopenedUpper && reopenedReference1 && reopenedReference2
        && reopenedType->currentText() == QStringLiteral("跳动")
        && reopenedLower->value() == 111 && reopenedUpper->value() == 222
        && reopenedReference1->text() == QStringLiteral("基准A")
        && reopenedReference2->text() == QStringLiteral("基准B"),
        "roundout offsets and references must survive save and reopen");
    editor.hide();
    std::cout << "PASS: simulated camera states, start/stop, cached last-frame load, recipe metadata/assets packaging, roundout configuration, preflight and reopen\n";
}

static void testImageLessSensorPlan()
{
    GraphicalProgramEditor editor;
    editor.setAttribute(Qt::WA_DontShowOnScreen);
    editor.show();
    QTest::qWait(100);
    auto button = [&](const QString& name) {
        for (auto* value : editor.findChildren<QPushButton*>()) if (value->text() == name) return value;
        throw std::runtime_error("sensor plan button missing");
    };
    auto* type = editor.findChild<QComboBox*>(QStringLiteral("measurementType"));
    auto* lower = editor.findChild<QSpinBox*>(QStringLiteral("lowerAxialOffsetPulse"));
    auto* upper = editor.findChild<QSpinBox*>(QStringLiteral("upperAxialOffsetPulse"));
    auto* reference1 = editor.findChild<QLineEdit*>(QStringLiteral("roundoutReference1"));
    auto* reference2 = editor.findChild<QLineEdit*>(QStringLiteral("roundoutReference2"));
    require(type && lower && upper && reference1 && reference2,
        "image-less sensor plan controls missing");
    type->setCurrentText(QStringLiteral("跳动"));
    lower->setValue(135);
    upper->setValue(246);
    reference1->setText(QStringLiteral("基准A"));
    reference2->setText(QStringLiteral("基准B"));
    button(QStringLiteral("新增测量记录"))->click();
    auto* table = editor.findChild<QTableWidget*>();
    require(table && table->rowCount() == 1,
        "image-less sensor record must be configurable without a reference image");

    QTemporaryDir directory;
    require(directory.isValid(), "temporary sensor plan directory missing");
    const QString planPath = directory.filePath(QStringLiteral("sensor.axisproj.json"));
    QString error;
    require(editor.saveRecipeFile(planPath, error),
        qPrintable(QStringLiteral("image-less sensor plan save failed: %1").arg(error)));
    require(QFileInfo::exists(planPath)
        && !QFileInfo::exists(directory.filePath(QStringLiteral("sensor.axisproj.assets"))),
        "image-less sensor plan must not require an image asset directory");

    GraphicalProgramEditor reopened;
    reopened.setAttribute(Qt::WA_DontShowOnScreen);
    require(reopened.loadRecipeFile(planPath, error),
        qPrintable(QStringLiteral("image-less sensor plan reopen failed: %1").arg(error)));
    reopened.show();
    QTest::qWait(100);
    auto* reopenedCanvas = reopened.findChild<GraphicalCanvas*>();
    auto* reopenedTable = reopened.findChild<QTableWidget*>();
    auto* sourceBadge = reopened.findChild<QLabel*>(QStringLiteral("canvasSourceBadge"));
    auto* emptyHint = reopened.findChild<QLabel*>(QStringLiteral("canvasEmptyHint"));
    require(reopenedCanvas && !reopenedCanvas->hasImage()
        && sourceBadge && sourceBadge->isHidden()
        && emptyHint && !emptyHint->isHidden() && emptyHint->text().contains(QStringLiteral("光幕")),
        "image-less sensor plan must reopen with a clear sensor-only canvas state");
    require(reopenedTable && reopenedTable->rowCount() == 1,
        "image-less sensor record must reopen");
    reopenedTable->setCurrentCell(0, 0);
    require(reopened.findChild<QComboBox*>(QStringLiteral("measurementType"))->currentText() == QStringLiteral("跳动")
        && reopened.findChild<QSpinBox*>(QStringLiteral("lowerAxialOffsetPulse"))->value() == 135
        && reopened.findChild<QSpinBox*>(QStringLiteral("upperAxialOffsetPulse"))->value() == 246
        && reopened.findChild<QLineEdit*>(QStringLiteral("roundoutReference1"))->text() == QStringLiteral("基准A")
        && reopened.findChild<QLineEdit*>(QStringLiteral("roundoutReference2"))->text() == QStringLiteral("基准B"),
        "image-less sensor settings must survive save and reopen");

    GraphicalProgramEditor diameterEditor;
    diameterEditor.setAttribute(Qt::WA_DontShowOnScreen);
    diameterEditor.setAxisBackend([](int axis) {
        GraphicalProgramEditor::AxisSnapshot snapshot;
        snapshot.connected = true;
        snapshot.available = true;
        snapshot.valid = true;
        snapshot.status = 0;
        snapshot.planned = axis == 5 ? 12340 : 0;
        snapshot.encoder = axis == 5 ? 12345 : 0;
        return snapshot;
    }, [](int, GraphicalProgramEditor::AxisCommand, double, long) {
        return GraphicalProgramEditor::AxisCommandResult();
    });
    diameterEditor.show();
    QTest::qWait(100);
    auto* diameterType = diameterEditor.findChild<QComboBox*>(QStringLiteral("measurementType"));
    require(diameterType, "diameter measurement type missing");
    diameterType->setCurrentText(QStringLiteral("直径"));
    for (auto* value : diameterEditor.findChildren<QPushButton*>()) {
        if (value->text() == QStringLiteral("新增测量记录")) value->click();
    }
    auto* diameterTable = diameterEditor.findChild<QTableWidget*>();
    require(diameterTable && diameterTable->rowCount() == 1,
        "diameter record must be configurable without a reference image");
    diameterTable->setCurrentCell(0, 0);
    QTest::qWait(250);
    diameterEditor.findChild<QSpinBox*>(QStringLiteral("recipeProgramNumber"))->setValue(61);
    diameterEditor.findChild<QLineEdit*>(QStringLiteral("recipePartNumber"))->setText(QStringLiteral("D-61"));
    diameterEditor.findChild<QLineEdit*>(QStringLiteral("recipePartName"))->setText(QStringLiteral("直径测试件"));
    diameterEditor.findChild<QLineEdit*>(QStringLiteral("recipeProcessNumber"))->setText(QStringLiteral("OP10"));
    QPushButton* capturePosition = nullptr;
    for (auto* value : diameterEditor.findChildren<QPushButton*>()) {
        if (value->text() == QStringLiteral("记录选中记录的当前设备点位")) capturePosition = value;
    }
    require(capturePosition && capturePosition->isEnabled(),
        "diameter axis position capture must be enabled without a light-curtain sample");
    capturePosition->click();
    QTest::qWait(50);
    require(diameterTable->item(0, 10)->text().contains(QStringLiteral("轴5")),
        "diameter configuration must capture axis 5");

    const QString diameterPath = directory.filePath(QStringLiteral("diameter.axisproj.json"));
    require(diameterEditor.saveRecipeFile(diameterPath, error),
        qPrintable(QStringLiteral("diameter plan save without light-curtain sample failed: %1").arg(error)));
    QFile diameterFile(diameterPath);
    require(diameterFile.open(QIODevice::ReadOnly), "diameter plan file missing");
    const QByteArray diameterJson = diameterFile.readAll();
    require(diameterJson.contains("\"lightCurtain\"")
        && diameterJson.contains("\"status\": \"none\"")
        && !diameterJson.contains("\"rawValue\""),
        "diameter plan must store the point only, not a stale sensor reading");
    GraphicalProgramEditor reopenedDiameter;
    reopenedDiameter.setAttribute(Qt::WA_DontShowOnScreen);
    require(reopenedDiameter.loadRecipeFile(diameterPath, error),
        qPrintable(QStringLiteral("diameter plan reopen failed: %1").arg(error)));
    require(reopenedDiameter.findChild<QSpinBox*>(QStringLiteral("recipeProgramNumber"))->value() == 61,
        "new graphical program number must survive save and reopen");
    require(reopenedDiameter.validateRecipeForExport().isEmpty(),
        "diameter generation preflight must require the axis point, not a stale light-curtain sample");
    QString packagePath;
    require(diameterEditor.exportProgramPackage(directory.path(), packagePath, error),
        qPrintable(QStringLiteral("diameter generation package failed: %1").arg(error)));
    const QString manifestPath = QDir(packagePath).filePath(QStringLiteral("generation_manifest.json"));
    const QString definitionPath = QDir(packagePath).filePath(QStringLiteral("measurement.axisproj.json"));
    require(QFileInfo::exists(manifestPath) && QFileInfo::exists(definitionPath)
        && !QFileInfo::exists(QDir(packagePath).filePath(QStringLiteral("measurement.axisproj.assets"))),
        "sensor generation package must contain its definition without image assets");
    QFile manifestFile(manifestPath);
    require(manifestFile.open(QIODevice::ReadOnly), "generation manifest missing");
    const QJsonObject manifest = QJsonDocument::fromJson(manifestFile.readAll()).object();
    require(manifest.value(QStringLiteral("format")).toString()
            == QStringLiteral("AxisMeasurement.GraphicalProgramPackage")
        && manifest.value(QStringLiteral("programNumber")).toInt() == 61
        && manifest.value(QStringLiteral("records")).toArray().size() == 1,
        "generation manifest must identify program 61 and its measurement record");
    GraphicalProgramDescriptor descriptor;
    require(GraphicalProgramRegistry::loadPackage(packagePath, descriptor, error)
        && descriptor.programNumber == 61
        && descriptor.partNumber == QStringLiteral("D-61")
        && descriptor.recordCount == 1,
        qPrintable(QStringLiteral("generated package registration failed: %1").arg(error)));
    QVector<GraphicalProgramDescriptor> registeredPrograms;
    require(GraphicalProgramRegistry::scan(directory.path(), registeredPrograms, error)
        && registeredPrograms.size() == 1 && registeredPrograms.first().programNumber == 61,
        qPrintable(QStringLiteral("generated package scan failed: %1").arg(error)));
    QString duplicatePath;
    require(!diameterEditor.exportProgramPackage(directory.path(), duplicatePath, error)
        && error.contains(QStringLiteral("生成目标已存在")),
        "generation package must not overwrite an existing program target");
    QJsonObject tamperedManifest = manifest;
    QJsonArray tamperedRecords = tamperedManifest.value(QStringLiteral("records")).toArray();
    QJsonObject tamperedRecord = tamperedRecords.first().toObject();
    tamperedRecord[QStringLiteral("axes")] = QJsonArray{ 2 };
    tamperedRecords[0] = tamperedRecord;
    tamperedManifest[QStringLiteral("records")] = tamperedRecords;
    QFile tamperedManifestFile(manifestPath);
    require(tamperedManifestFile.open(QIODevice::WriteOnly | QIODevice::Truncate)
        && tamperedManifestFile.write(QJsonDocument(tamperedManifest).toJson()) > 0,
        "tampered manifest fixture write failed");
    tamperedManifestFile.close();
    require(!GraphicalProgramRegistry::loadPackage(packagePath, descriptor, error)
        && error.contains(QStringLiteral("运动轴契约")),
        "registry must reject a manifest whose device contract disagrees with its measurement type");
    std::cout << "PASS: image-less sensor plan, collision-safe generation package and registry validation\n";
}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QFile theme(QStringLiteral("config/theme.qss"));
    if (theme.open(QIODevice::ReadOnly)) app.setStyleSheet(QString::fromUtf8(theme.readAll()));
    try {
        testCornerGeometry();
        testSensorMeasurementAdapter();
        testDetectionRecords();
        testCameraWorkflow();
        testImageLessSensorPlan();
        GraphicalProgramEditor editor;
        editor.setAttribute(Qt::WA_DontShowOnScreen);
        using Command = GraphicalProgramEditor::AxisCommand;
        bool connected = false, moving = false, failStop = false, failStartAck = false;
        int starts = 0, stops = 0, lastAxis = -1;
        editor.setAxisBackend([&](int) {
            GraphicalProgramEditor::AxisSnapshot state;
            state.connected = state.available = state.valid = connected;
            state.status = 0x200 | (moving ? 0x400 : 0);
            state.message = connected ? QStringLiteral("测试后端（非硬件）") : QStringLiteral("未连接");
            return state;
        }, [&](int axis, Command command, double, long) -> GraphicalProgramEditor::AxisCommandResult {
            lastAxis = axis;
            if (command == Command::Stop || command == Command::EmergencyStop) {
                ++stops;
                if (failStop) return QStringLiteral("测试：停止通讯失败");
                moving = false;
            } else if (command == Command::JogNegative || command == Command::JogPositive || command == Command::MoveAbsolute) {
                ++starts; moving = true;
                if (failStartAck) return { QStringLiteral("测试：启动响应丢失"), true };
            }
            return QString();
        });
        editor.show();
        QTest::qWait(250);
        auto button = [&](const QString& name) {
            for (auto* value : editor.findChildren<QPushButton*>()) if (value->text() == name) return value;
            throw std::runtime_error("button missing");
        };
        auto* positive = button(QStringLiteral("正向 +（按住）"));
        auto* negative = button(QStringLiteral("负向 −（按住）"));
        auto* stop = button(QStringLiteral("停止当前轴"));
        auto* emergency = button(QStringLiteral("全部轴急停"));
        auto* selector = editor.findChild<QComboBox*>(QStringLiteral("axisSelector"));
        require(selector && selector->currentData().toInt() == 2,
            "axis selector must default to the first available axis");
        require(!positive->isEnabled() && !stop->isEnabled(), "offline controls must be disabled");
        QComboBox* previewMode = nullptr;
        for (auto* combo : editor.findChildren<QComboBox*>())
            if (combo->findText(QStringLiteral("绝对点位")) >= 0) previewMode = combo;
        require(previewMode && !previewMode->isEnabled(), "offline mode selector must be disabled after preview ends");
        require(starts == 0, "offline state must not issue motion");
        require(editor.windowModality() == Qt::NonModal, "editor must allow returning to the main window");
        require(editor.grab().save("x64/graphical_axis_offline.png"), "offline screenshot failed");
        connected = true;
        QTest::qWait(250);
        require(previewMode->isEnabled(), "ready mode selector must be enabled");
        previewMode->setCurrentIndex(1);
        require(!positive->isVisible() && button(QStringLiteral("移动至目标位置"))->isVisible(), "absolute mode must change visible controls");
        require(button(QStringLiteral("移动至目标位置"))->isEnabled() && starts == 0, "mode switch alone must not issue motion");
        previewMode->setCurrentIndex(0);
        require(positive->isVisible() && !button(QStringLiteral("移动至目标位置"))->isVisible(), "Jog mode must restore Jog controls");
        require(positive->isEnabled() && emergency->isEnabled(), "ready controls must be enabled");
        const int selectedAxis = selector->currentData().toInt();
        QTest::mousePress(positive, Qt::LeftButton);
        QTest::qWait(250);
        require(starts == 1 && moving && lastAxis == selectedAxis, "Jog must address selected axis");
        require(positive->isEnabled() && !negative->isEnabled(), "pressed Jog must retain release delivery");
        QTest::mouseRelease(positive, Qt::LeftButton);
        QTest::qWait(250);
        require(!moving && stops > 0 && lastAxis == selectedAxis, "Jog release must stop initiating axis");
        QComboBox* mode = nullptr;
        for (auto* combo : editor.findChildren<QComboBox*>()) {
            if (combo->findText(QStringLiteral("绝对点位")) >= 0) mode = combo;
        }
        require(mode, "axis mode selector missing");
        const int axis7Index = selector->findData(7);
        require(axis7Index >= 0, "turntable axis missing");
        selector->setCurrentIndex(axis7Index);
        QTest::mousePress(positive, Qt::LeftButton);
        QEvent deactivate(QEvent::WindowDeactivate);
        QApplication::sendEvent(&editor, &deactivate);
        require(!moving && lastAxis == 7, "deactivation must stop axis 7, not legacy current axis");
        QTest::mouseRelease(positive, Qt::LeftButton);
        QTest::qWait(250);
        mode->setCurrentIndex(1);
        QTest::qWait(250);
        QTest::mouseClick(button(QStringLiteral("移动至目标位置")), Qt::LeftButton);
        require(moving && lastAxis == 7, "absolute motion missing");
        QTest::mouseClick(stop, Qt::LeftButton);
        QTest::qWait(250);
        failStartAck = true;
        QTest::mouseClick(button(QStringLiteral("移动至目标位置")), Qt::LeftButton);
        require(moving, "uncertain start acknowledgement scenario missing");
        failStop = true;
        editor.close();
        require(editor.isVisible() && moving, "failed stop must block close");
        failStop = false;
        QTest::mouseClick(emergency, Qt::LeftButton);
        require(!moving, "emergency command missing");
        QTest::qWait(250);
        editor.centralWidget()->setEnabled(false);
        require(stop->isEnabled() && emergency->isEnabled(), "stop controls must survive disabled editor content");
        editor.centralWidget()->setEnabled(true);
        editor.close();
        require(!editor.isVisible(), "stationary editor must close");
        std::cout << "PASS: offline state, modality, Jog press/release, fixed-axis stop, deactivation, absolute move, uncertain start acknowledgement, failed-stop close guard, emergency, independent stop bar\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
