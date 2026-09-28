// Offline UI checks. The backend below never calls any device SDK.
// Link editor/canvas objects with /DELAYLOAD:halconcpp.dll; algorithms are not executed.
#include "graphical_program_editor.h"
#include "graphical_sensor_measurement.h"
#include "graphical_program_contract.h"
#include "graphical_program_registry.h"
#include "graphical_program_runner.h"
#include <QAction>
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
    require(GraphicalSensorMeasurement::validateAxisMotionStart(5, 0x200, 1000, 1200).ok,
        "enabled idle axis must accept a target away from inactive limits");
    require(!GraphicalSensorMeasurement::validateAxisMotionStart(5, 0x200 | 0x20, 1000, 1200).ok
        && !GraphicalSensorMeasurement::validateAxisMotionStart(5, 0x200 | 0x40, 1000, 800).ok,
        "axis start policy must reject motion toward an active directional limit");
    require(!GraphicalSensorMeasurement::validateAxisMotionStart(5, 0x400, 1000, 1200).ok
        && !GraphicalSensorMeasurement::validateAxisMotionStart(5, 0, 1000, 1200).ok,
        "axis start policy must reject a moving or disabled axis");
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
    int diameterReads = 0;
    const auto acquiredDiameter = GraphicalSensorMeasurement::acquireDiameter(
        [&](double& raw, QString&) {
            raw = ++diameterReads == 1 ? 9.8 : 10.2;
            return true;
        },
        [](double raw) { return raw + 0.1; });
    require(acquiredDiameter.ok && diameterReads == 2
        && std::abs(acquiredDiameter.value - 10.1) < 1e-12,
        "diameter acquisition must read two current OUT1 values and compensate each sample");
    const auto failedDiameterRead = GraphicalSensorMeasurement::acquireDiameter(
        [](double&, QString& error) {
            error = QStringLiteral("simulated disconnect");
            return false;
        },
        [](double raw) { return raw; });
    require(!failedDiameterRead.ok
        && failedDiameterRead.error.contains(QStringLiteral("第1次"))
        && failedDiameterRead.error.contains(QStringLiteral("simulated disconnect")),
        "diameter acquisition must preserve the failed sample index and device error");

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
    GraphicalAxisRuntimeCallbacks axisCallbacks;
    axisCallbacks.readStartState = [](int) {
        return GraphicalAxisRuntimeState{ true, 0x200, 1000, QString() };
    };
    axisCallbacks.issueAbsoluteMove = [](int, qint64 target, QString&) { return target == 1200; };
    int runtimePolls = 0;
    axisCallbacks.queryMotion = [&](int, qint64) {
        return ++runtimePolls < 2
            ? GraphicalSensorMotionState{ true, true, false, false, QString() }
            : GraphicalSensorMotionState{ true, false, true, false, QString() };
    };
    int runtimeStops = 0;
    axisCallbacks.stopAxis = [&](int) { ++runtimeStops; return true; };
    axisCallbacks.cancelRequested = []() { return false; };
    axisCallbacks.delay = [](int) {};
    const auto completedMotion = GraphicalSensorMeasurement::executeAxisMotion(
        5, 1200, axisCallbacks, 20, 10);
    require(completedMotion.ok && runtimePolls == 2 && runtimeStops == 0,
        "axis runtime must issue the target and wait until arrival without stopping");
    axisCallbacks.issueAbsoluteMove = [](int, qint64, QString& error) {
        error = QStringLiteral("simulated SDK command failure");
        return false;
    };
    const auto commandFailure = GraphicalSensorMeasurement::executeAxisMotion(
        5, 1200, axisCallbacks, 20, 10);
    require(!commandFailure.ok && commandFailure.stopAttempted && runtimeStops == 1
        && commandFailure.error.contains(QStringLiteral("simulated SDK")),
        "axis runtime must stop after a target command failure");
    std::cout << "PASS: sensor axial mapping, diameter/cylindricity/roundout guards, reference axis and motion stop policy\n";
}

static GraphicalProgramExecutionPlan makeRunnerPlan()
{
    GraphicalProgramExecutionPlan plan;
    plan.descriptor.programNumber = 61;
    for (int index = 0; index < 2; ++index) {
        GraphicalProgramStep step;
        step.sequence = index + 1;
        step.featureNumber = QStringLiteral("F%1").arg(index + 1);
        step.type = QStringLiteral("直径");
        step.contract = GraphicalProgramGeneration::contractForType(step.type);
        step.definition[QStringLiteral("hasTolerance")] = true;
        step.definition[QStringLiteral("nominal")] = 10.0;
        step.definition[QStringLiteral("lower")] = -0.1;
        step.definition[QStringLiteral("upper")] = 0.1;
        plan.steps.append(step);
    }
    return plan;
}

static void testGraphicalFreshImageAcquisition()
{
    unsigned long long serial = 10;
    int delays = 0;
    int stops = 0;
    int copies = 0;
    GraphicalFreshImageCallbacks callbacks;
    callbacks.frameSerial = [&]() { return serial; };
    callbacks.startContinuousCapture = [](int exposure, QString&) { return exposure == 120; };
    callbacks.stopCapture = [&](QString&) { ++stops; return true; };
    callbacks.copyStoppedFrame = [&](QImage& image, int& exposure, QString&) {
        ++copies;
        image = QImage(8, 8, QImage::Format_Grayscale8);
        exposure = 118;
        return true;
    };
    callbacks.cancelRequested = []() { return false; };
    callbacks.delay = [&](int) {
        if (++delays == 2) ++serial;
    };
    const GraphicalProgramRunStepResult fresh =
        GraphicalFreshImageAcquisition::capture(0, 120, callbacks, 50, 10);
    require(fresh.ok && fresh.frame.hasImage && fresh.frame.cameraIndex == 0
        && fresh.frame.exposure == 118 && delays == 2 && stops == 1 && copies == 1,
        "fresh image acquisition must wait for a newer frame, stop, then copy its snapshot");

    serial = 20;
    delays = 0;
    stops = 0;
    copies = 0;
    callbacks.delay = [&](int) { ++delays; };
    const GraphicalProgramRunStepResult timeout =
        GraphicalFreshImageAcquisition::capture(0, 120, callbacks, 25, 10);
    require(!timeout.ok && timeout.error.contains(QStringLiteral("新帧超时"))
        && delays == 3 && stops == 1 && copies == 0,
        "fresh image timeout must stop capture and must not copy the stale cached frame");

    bool cancelled = false;
    serial = 30;
    delays = 0;
    stops = 0;
    copies = 0;
    callbacks.cancelRequested = [&]() { return cancelled; };
    callbacks.delay = [&](int) { ++delays; cancelled = true; };
    const GraphicalProgramRunStepResult cancelResult =
        GraphicalFreshImageAcquisition::capture(0, 120, callbacks, 50, 10);
    require(!cancelResult.ok && cancelResult.error.contains(QStringLiteral("已取消"))
        && delays == 1 && stops == 1 && copies == 0,
        "fresh image cancellation must remain observable until capture stops");
    std::cout << "PASS: fresh camera frame sequence, timeout/cancel stop and stale-frame rejection\n";
}

static void testGraphicalAcquisitionDispatcher()
{
    GraphicalProgramAcquisitionCallbacks callbacks;
    QStringList routes;
    callbacks.captureImage = [&](const GraphicalProgramStep& step,
        const GraphicalProgramMotionTarget&, QImage& image, QString&) {
        routes.append(QStringLiteral("image:%1").arg(step.type));
        image = QImage(8, 8, QImage::Format_Grayscale8);
        return true;
    };
    callbacks.captureCompensatedDiameters = [&](const GraphicalProgramStep& step,
        const GraphicalProgramMotionTarget&, QVector<double>& samples, QString&) {
        routes.append(QStringLiteral("diameter:%1").arg(step.type));
        samples = QVector<double>(15, 20.0);
        return true;
    };
    callbacks.captureRoundoutDistances = [&](const GraphicalProgramStep& step,
        const GraphicalProgramMotionTarget&, QVector<double>& samples, QString&) {
        routes.append(QStringLiteral("roundout:%1").arg(step.type));
        samples = QVector<double>(25, 5.0);
        return true;
    };

    const QStringList types = { QStringLiteral("角度"), QStringLiteral("孔径"),
        QStringLiteral("长度"), QStringLiteral("圆弧半径"), QStringLiteral("直径"),
        QStringLiteral("圆柱度"), QStringLiteral("跳动") };
    for (int index = 0; index < types.size(); ++index) {
        GraphicalProgramStep step;
        step.sequence = index + 1;
        step.type = types.at(index);
        step.contract = GraphicalProgramGeneration::contractForType(step.type);
        GraphicalProgramMotionTarget target;
        target.label = QStringLiteral("测量点");
        target.cameraIndex = step.contract.cameraIndex;
        target.exposure = step.contract.requiresImage ? 100 : -1;
        const GraphicalProgramRunStepResult result =
            GraphicalProgramAcquisitionDispatcher::capture(step, target, callbacks);
        require(result.ok
            && (step.contract.requiresImage
                ? result.frame.hasImage && !result.frame.image.isNull()
                    && result.frame.cameraIndex == step.contract.cameraIndex
                : step.type == QStringLiteral("跳动")
                    ? result.frame.roundoutDistanceSamples.size() == 25
                    : result.frame.compensatedDiameterSamples.size() == 15),
            "acquisition dispatcher must produce the runtime input required by every type");
    }
    require(routes == QStringList({ QStringLiteral("image:角度"), QStringLiteral("image:孔径"),
        QStringLiteral("image:长度"), QStringLiteral("image:圆弧半径"),
        QStringLiteral("diameter:直径"), QStringLiteral("diameter:圆柱度"),
        QStringLiteral("roundout:跳动") }),
        "acquisition dispatcher must keep camera, diameter and roundout inputs separated");

    GraphicalProgramStep failedStep;
    failedStep.sequence = 8;
    failedStep.type = QStringLiteral("直径");
    failedStep.contract = GraphicalProgramGeneration::contractForType(failedStep.type);
    callbacks.captureCompensatedDiameters = [](const GraphicalProgramStep&,
        const GraphicalProgramMotionTarget&, QVector<double>&, QString& error) {
        error = QStringLiteral("simulated light curtain failure");
        return false;
    };
    const GraphicalProgramRunStepResult failed =
        GraphicalProgramAcquisitionDispatcher::capture(
            failedStep, GraphicalProgramMotionTarget(), callbacks);
    require(!failed.ok && failed.error.contains(QStringLiteral("simulated light curtain failure")),
        "acquisition dispatcher must preserve the first device failure without fake samples");
    std::cout << "PASS: seven-type runtime acquisition routing and device failure propagation\n";
}

static void testGraphicalMeasurementDispatcher()
{
    GraphicalProgramMeasurementCallbacks callbacks;
    QStringList routes;
    callbacks.visual = [&](const GraphicalProgramStep& step,
        const QVector<GraphicalProgramMotionTarget>&,
        const QVector<GraphicalProgramRuntimeFrame>&) {
        routes.append(QStringLiteral("visual:%1").arg(step.type));
        return GraphicalSensorValueResult::success(12.5);
    };

    const QStringList types = { QStringLiteral("角度"), QStringLiteral("孔径"),
        QStringLiteral("长度"), QStringLiteral("圆弧半径"), QStringLiteral("直径"),
        QStringLiteral("圆柱度"), QStringLiteral("跳动") };
    for (int index = 0; index < types.size(); ++index) {
        GraphicalProgramStep step;
        step.sequence = index + 1;
        step.featureNumber = QStringLiteral("D%1").arg(index + 1);
        step.type = types.at(index);
        step.contract = GraphicalProgramGeneration::contractForType(step.type);
        QVector<GraphicalProgramMotionTarget> targets(step.contract.threeSectionScan ? 3 : 1);
        QVector<GraphicalProgramRuntimeFrame> frames(targets.size());
        if (step.contract.requiresImage) {
            frames[0].hasImage = true;
            frames[0].image = QImage(8, 8, QImage::Format_Grayscale8);
            frames[0].cameraIndex = step.contract.cameraIndex;
        }
        else if (step.type == QStringLiteral("直径")) {
            frames[0].compensatedDiameterSamples = { 20.0, 22.0 };
        }
        else if (step.type == QStringLiteral("圆柱度")) {
            for (int section = 0; section < frames.size(); ++section) {
                for (int sample = 0; sample < 15; ++sample)
                    frames[section].compensatedDiameterSamples.append(
                        20.0 + section * 0.1 + sample * 0.001);
            }
        }
        else if (step.type == QStringLiteral("跳动")) {
            for (GraphicalProgramRuntimeFrame& frame : frames)
                frame.roundoutDistanceSamples = QVector<double>(25, 5.0);
        }
        const GraphicalProgramRunStepResult result =
            GraphicalProgramMeasurementDispatcher::compute(step, targets, frames, callbacks);
        require(result.ok && result.measurements.size() == 1
            && result.measurements.first().featureNumber == step.featureNumber
            && result.measurements.first().type == step.type
            && result.measurements.first().unit
                == (step.type == QStringLiteral("角度") ? QStringLiteral("deg") : QStringLiteral("mm")),
            "dispatcher must route every supported type into one structured result");
    }
    require(routes == QStringList({ QStringLiteral("visual:角度"), QStringLiteral("visual:孔径"),
        QStringLiteral("visual:长度"), QStringLiteral("visual:圆弧半径") }),
        "dispatcher must route only image measurements through the visual algorithm backend");

    GraphicalProgramStep invalidVisual;
    invalidVisual.sequence = 8;
    invalidVisual.type = QStringLiteral("角度");
    invalidVisual.contract = GraphicalProgramGeneration::contractForType(invalidVisual.type);
    const GraphicalProgramRunStepResult invalidResult =
        GraphicalProgramMeasurementDispatcher::compute(invalidVisual,
            QVector<GraphicalProgramMotionTarget>(1),
            QVector<GraphicalProgramRuntimeFrame>(1), callbacks);
    require(!invalidResult.ok && invalidResult.error.contains(QStringLiteral("图像或相机通道无效")),
        "visual dispatch must reject a missing runtime image before calling the algorithm");

    GraphicalProgramStep missingSensor;
    missingSensor.sequence = 9;
    missingSensor.type = QStringLiteral("直径");
    missingSensor.contract = GraphicalProgramGeneration::contractForType(missingSensor.type);
    const GraphicalProgramRunStepResult missingSensorResult =
        GraphicalProgramMeasurementDispatcher::compute(missingSensor,
            QVector<GraphicalProgramMotionTarget>(1),
            QVector<GraphicalProgramRuntimeFrame>(1), callbacks);
    require(!missingSensorResult.ok
        && missingSensorResult.error.contains(QStringLiteral("没有有效光幕样本")),
        "sensor dispatch must reject a frame without acquired samples");
    std::cout << "PASS: seven-type runtime calculation dispatch, frame guards and structured results\n";
}

static GraphicalProgramStep makeRuntimePipelineStep(const QString& type, int sequence)
{
    GraphicalProgramStep step;
    step.sequence = sequence;
    step.featureNumber = QStringLiteral("P%1").arg(sequence);
    step.type = type;
    step.contract = GraphicalProgramGeneration::contractForType(type);
    step.definition[QStringLiteral("hasTolerance")] = false;
    QJsonArray axes;
    for (int axisNumber : step.contract.axes) {
        QJsonObject axis;
        axis[QStringLiteral("axis")] = axisNumber;
        axis[QStringLiteral("encoder")] = 1000.0 + sequence * 100 + axisNumber;
        axes.append(axis);
    }
    QJsonObject position;
    position[QStringLiteral("status")] = QStringLiteral("collected");
    position[QStringLiteral("axes")] = axes;
    position[QStringLiteral("cameraIndex")] = step.contract.cameraIndex;
    position[QStringLiteral("exposure")] = step.contract.requiresImage ? 100 : -1;
    step.definition[QStringLiteral("devicePosition")] = position;
    if (step.contract.threeSectionScan) {
        step.definition[QStringLiteral("lowerAxialOffsetPulse")] = 100.0;
        step.definition[QStringLiteral("upperAxialOffsetPulse")] = 150.0;
    }
    return step;
}

static void testGraphicalProgramRuntimePipeline()
{
    GraphicalProgramExecutionPlan plan;
    const QStringList types = { QStringLiteral("角度"), QStringLiteral("孔径"),
        QStringLiteral("长度"), QStringLiteral("圆弧半径"), QStringLiteral("直径"),
        QStringLiteral("圆柱度"), QStringLiteral("跳动") };
    for (int index = 0; index < types.size(); ++index)
        plan.steps.append(makeRuntimePipelineStep(types.at(index), index + 1));

    GraphicalProgramAcquisitionCallbacks acquisition;
    acquisition.captureImage = [](const GraphicalProgramStep&,
        const GraphicalProgramMotionTarget&, QImage& image, QString&) {
        image = QImage(8, 8, QImage::Format_Grayscale8);
        return true;
    };
    acquisition.captureCompensatedDiameters = [](const GraphicalProgramStep&,
        const GraphicalProgramMotionTarget&, QVector<double>& samples, QString&) {
        samples = QVector<double>(15, 20.0);
        return true;
    };
    acquisition.captureRoundoutDistances = [](const GraphicalProgramStep&,
        const GraphicalProgramMotionTarget&, QVector<double>& samples, QString&) {
        samples = QVector<double>(25, 5.0);
        return true;
    };
    GraphicalProgramMeasurementCallbacks measurement;
    measurement.visual = [](const GraphicalProgramStep& step,
        const QVector<GraphicalProgramMotionTarget>&,
        const QVector<GraphicalProgramRuntimeFrame>&) {
        return GraphicalSensorValueResult::success(double(step.sequence));
    };

    GraphicalProgramRunnerCallbacks callbacks;
    int moves = 0;
    int captures = 0;
    int stops = 0;
    callbacks.cancelRequested = []() { return false; };
    callbacks.validateStep = [](const GraphicalProgramStep& step) {
        return step.contract.supported
            ? GraphicalProgramRunStepResult::success()
            : GraphicalProgramRunStepResult::failure(QStringLiteral("unsupported"));
    };
    callbacks.moveToTarget = [&](const GraphicalProgramStep&,
        const GraphicalProgramMotionTarget&) {
        ++moves;
        return GraphicalProgramRunStepResult::success();
    };
    callbacks.captureTarget = [&](const GraphicalProgramStep& step,
        const GraphicalProgramMotionTarget& target) {
        ++captures;
        return GraphicalProgramAcquisitionDispatcher::capture(step, target, acquisition);
    };
    callbacks.computeTargets = [&](const GraphicalProgramStep& step,
        const QVector<GraphicalProgramMotionTarget>& targets,
        const QVector<GraphicalProgramRuntimeFrame>& frames) {
        return GraphicalProgramMeasurementDispatcher::compute(step, targets, frames, measurement);
    };
    callbacks.requestStop = [&]() { ++stops; return true; };

    GraphicalProgramRunner runner;
    const GraphicalProgramRunResult result = runner.execute(plan, callbacks);
    require(result.ok && result.completedSteps == 7 && result.measurements.size() == 7
        && moves == 11 && captures == 11 && stops == 0
        && result.measurements.first().featureNumber == QStringLiteral("P1")
        && result.measurements.last().featureNumber == QStringLiteral("P7"),
        "complete simulated seven-type program must preserve per-target acquisition and result order");

    GraphicalProgramExecutionPlan invalidPlan = plan;
    QJsonObject invalidPosition = invalidPlan.steps[6].definition
        .value(QStringLiteral("devicePosition")).toObject();
    invalidPosition[QStringLiteral("status")] = QStringLiteral("uncollected");
    invalidPlan.steps[6].definition[QStringLiteral("devicePosition")] = invalidPosition;
    moves = 0;
    captures = 0;
    const GraphicalProgramRunResult invalidResult = runner.execute(invalidPlan, callbacks);
    require(!invalidResult.ok && invalidResult.failedSequence == 7
        && moves == 0 && captures == 0 && stops == 0,
        "whole-program preflight must reject a later invalid target before any motion or capture");
    std::cout << "PASS: complete seven-type simulated move, acquisition, calculation and result pipeline\n";
}

static void testGraphicalProgramRunnerCore()
{
    const GraphicalProgramExecutionPlan plan = makeRunnerPlan();
    GraphicalProgramRunner runner;
    QVector<GraphicalProgramRunState> states;
    QStringList order;
    int clears = 0;
    int stops = 0;
    GraphicalProgramRunnerCallbacks callbacks;
    callbacks.stateChanged = [&](GraphicalProgramRunState state, const GraphicalProgramStep* step) {
        states.append(state);
        if (step) order.append(QStringLiteral("%1:%2").arg(int(state)).arg(step->sequence));
    };
    callbacks.clearPreviousResults = [&]() { ++clears; };
    callbacks.cancelRequested = []() { return false; };
    callbacks.validateStep = [](const GraphicalProgramStep& step) {
        return step.sequence > 0 && step.contract.supported
            ? GraphicalProgramRunStepResult::success()
            : GraphicalProgramRunStepResult::failure(QStringLiteral("bad step"));
    };
    callbacks.moveToStep = [](const GraphicalProgramStep&) {
        return GraphicalProgramRunStepResult::success();
    };
    callbacks.captureStep = [](const GraphicalProgramStep& step) {
        GraphicalProgramRuntimeFrame frame;
        frame.hasImage = true;
        frame.cameraIndex = 0;
        frame.exposure = step.sequence;
        frame.source = QStringLiteral("simulated-current-frame");
        return GraphicalProgramRunStepResult::successWithFrame(frame);
    };
    int computedExposureSum = 0;
    callbacks.computeStep = [&](const GraphicalProgramStep&,
        const GraphicalProgramRuntimeFrame& frame) {
        require(frame.hasImage && frame.cameraIndex == 0
            && frame.source == QStringLiteral("simulated-current-frame"),
            "runner compute callback must receive the captured frame metadata");
        computedExposureSum += frame.exposure;
        GraphicalProgramMeasurementResult measurement =
            GraphicalProgramMeasurementResult::fromStepValue(plan.steps.at(frame.exposure - 1),
                frame.exposure == 1 ? 10.02 : 10.20,
                GraphicalProgramMeasurementResult::unitForType(QStringLiteral("直径")));
        return GraphicalProgramRunStepResult::successWithMeasurements(
            QVector<GraphicalProgramMeasurementResult>{ measurement });
    };
    callbacks.requestStop = [&]() { ++stops; return true; };
    const GraphicalProgramRunResult ok = runner.execute(plan, callbacks);
    require(ok.ok && ok.completedSteps == 2 && ok.finalState == GraphicalProgramRunState::Complete
        && clears == 1 && stops == 0 && computedExposureSum == 3
        && ok.measurements.size() == 2
        && ok.measurements.at(0).judgement == QStringLiteral("OK")
        && ok.measurements.at(1).judgement == QStringLiteral("NG")
        && ok.overallJudgement() == QStringLiteral("NG")
        && ok.ngMeasurementCount() == 1
        && ok.measurements.at(0).unit == QStringLiteral("mm")
        && runner.state() == GraphicalProgramRunState::Complete,
        "runner must execute all simulated steps and retain structured measurement results");
    require(states.contains(GraphicalProgramRunState::Loading)
        && states.contains(GraphicalProgramRunState::Moving)
        && states.contains(GraphicalProgramRunState::Capturing)
        && states.contains(GraphicalProgramRunState::Computing),
        "runner must expose loading/moving/capturing/computing states");
    GraphicalProgramRunResult mixedResult = ok;
    mixedResult.measurements[1].error = QStringLiteral("simulated result error");
    require(mixedResult.overallJudgement() == QStringLiteral("错误"),
        "result errors must take precedence over an earlier NG judgement");
    GraphicalProgramRunResult unjudgedResult = ok;
    unjudgedResult.measurements[0].judgement = QStringLiteral("未判定");
    unjudgedResult.measurements[1].judgement = QStringLiteral("OK");
    require(unjudgedResult.overallJudgement() == QStringLiteral("未判定"),
        "a successful run without complete tolerance judgements must stay unjudged");

    bool cancel = false;
    callbacks.computeStep = [&](const GraphicalProgramStep& step,
        const GraphicalProgramRuntimeFrame&) {
        if (step.sequence == 1) cancel = true;
        return GraphicalProgramRunStepResult::success();
    };
    callbacks.cancelRequested = [&]() { return cancel; };
    const int stopsBeforeCancel = stops;
    const GraphicalProgramRunResult cancelled = runner.execute(plan, callbacks);
    require(!cancelled.ok && cancelled.finalState == GraphicalProgramRunState::Cancelled
        && cancelled.completedSteps == 1 && cancelled.stopAttempted
        && stops == stopsBeforeCancel + 1,
        "runner cancellation must stop once and skip later steps");

    callbacks.cancelRequested = []() { return false; };
    callbacks.computeStep = [](const GraphicalProgramStep&, const GraphicalProgramRuntimeFrame&) {
        return GraphicalProgramRunStepResult::success();
    };
    callbacks.captureStep = [](const GraphicalProgramStep& step) {
        return step.sequence == 2
            ? GraphicalProgramRunStepResult::failure(QStringLiteral("simulated capture failure"))
            : GraphicalProgramRunStepResult::success();
    };
    const int stopsBeforeFailure = stops;
    const GraphicalProgramRunResult failed = runner.execute(plan, callbacks);
    require(!failed.ok && failed.finalState == GraphicalProgramRunState::Failed
        && failed.completedSteps == 1 && failed.stopAttempted
        && stops == stopsBeforeFailure + 1
        && failed.failedSequence == 2
        && failed.failedFeatureNumber == QStringLiteral("F2")
        && failed.failedType == QStringLiteral("直径")
        && failed.overallJudgement() == QStringLiteral("错误")
        && failed.error.contains(QStringLiteral("simulated capture failure")),
        "runner failure must stop once and preserve the first failure message");

    GraphicalProgramExecutionPlan threeSectionPlan;
    GraphicalProgramStep threeSectionStep = plan.steps.first();
    threeSectionStep.contract.threeSectionScan = true;
    QJsonObject axis5;
    axis5[QStringLiteral("axis")] = 5;
    axis5[QStringLiteral("encoder")] = 1000.0;
    QJsonArray axes;
    axes.append(axis5);
    QJsonObject devicePosition;
    devicePosition[QStringLiteral("status")] = QStringLiteral("collected");
    devicePosition[QStringLiteral("axes")] = axes;
    devicePosition[QStringLiteral("cameraIndex")] = 1;
    devicePosition[QStringLiteral("exposure")] = 120;
    threeSectionStep.definition[QStringLiteral("devicePosition")] = devicePosition;
    threeSectionStep.definition[QStringLiteral("lowerAxialOffsetPulse")] = 100.0;
    threeSectionStep.definition[QStringLiteral("upperAxialOffsetPulse")] = 150.0;
    threeSectionPlan.steps.append(threeSectionStep);

    QStringList targetOrder;
    callbacks.moveToTarget = [&](const GraphicalProgramStep&,
        const GraphicalProgramMotionTarget& target) {
        targetOrder.append(QStringLiteral("move:%1").arg(target.label));
        return GraphicalProgramRunStepResult::success();
    };
    callbacks.captureTarget = [&](const GraphicalProgramStep&,
        const GraphicalProgramMotionTarget& target) {
        targetOrder.append(QStringLiteral("capture:%1").arg(target.label));
        GraphicalProgramRuntimeFrame frame;
        frame.source = target.label;
        return GraphicalProgramRunStepResult::successWithFrame(frame);
    };
    callbacks.computeTargets = [&](const GraphicalProgramStep&,
        const QVector<GraphicalProgramMotionTarget>& targets,
        const QVector<GraphicalProgramRuntimeFrame>& frames) {
        require(targets.size() == 3 && frames.size() == 3
            && targets.at(0).axisEncoderTargets.value(5) == 900
            && targets.at(1).axisEncoderTargets.value(5) == 1000
            && targets.at(2).axisEncoderTargets.value(5) == 1150
            && frames.at(0).source == QStringLiteral("下侧截面")
            && frames.at(1).source == QStringLiteral("中间截面")
            && frames.at(2).source == QStringLiteral("上侧截面"),
            "multi-target compute must receive each target and its captured frame in order");
        return GraphicalProgramRunStepResult::success();
    };
    callbacks.cancelRequested = []() { return false; };
    const GraphicalProgramRunResult threeSection = runner.execute(threeSectionPlan, callbacks);
    require(threeSection.ok && threeSection.completedSteps == 1
        && targetOrder == QStringList({
            QStringLiteral("move:下侧截面"), QStringLiteral("capture:下侧截面"),
            QStringLiteral("move:中间截面"), QStringLiteral("capture:中间截面"),
            QStringLiteral("move:上侧截面"), QStringLiteral("capture:上侧截面") }),
        "multi-target runner must move and capture at each section before continuing");

    targetOrder.clear();
    callbacks.captureTarget = [&](const GraphicalProgramStep&,
        const GraphicalProgramMotionTarget& target) {
        targetOrder.append(QStringLiteral("capture:%1").arg(target.label));
        return target.label == QStringLiteral("中间截面")
            ? GraphicalProgramRunStepResult::failure(QStringLiteral("simulated target capture failure"))
            : GraphicalProgramRunStepResult::success();
    };
    const int stopsBeforeTargetFailure = stops;
    const GraphicalProgramRunResult targetFailed = runner.execute(threeSectionPlan, callbacks);
    require(!targetFailed.ok && targetFailed.completedSteps == 0
        && targetFailed.stopAttempted && stops == stopsBeforeTargetFailure + 1
        && !targetOrder.contains(QStringLiteral("capture:上侧截面"))
        && targetFailed.error.contains(QStringLiteral("中间截面")),
        "multi-target capture failure must stop once and skip remaining targets");
    std::cout << "PASS: unified runner states, result clearing, sequential execution, cancellation and failure stop policy\n";
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
    require(editor.findChild<QPushButton*>(QStringLiteral("generateProgramPackageButton")),
        "editor must expose the generation package command");
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
    auto action = [&](const QString& name) {
        for (auto* value : editor.findChildren<QAction*>()) if (value->text() == name) return value;
        throw std::runtime_error("editor action missing");
    };
    QAction* undoAction = action(QStringLiteral("撤销"));
    QAction* redoAction = action(QStringLiteral("重做"));
    require(undoAction->shortcut() == QKeySequence::Undo
        && redoAction->shortcut() == QKeySequence::Redo,
        "undo and redo must expose the standard shortcuts");
    require(undoAction->isEnabled() && !redoAction->isEnabled(),
        "undo must become available after adding a measurement record");
    undoAction->trigger();
    require(table->rowCount() == 0 && !undoAction->isEnabled() && redoAction->isEnabled(),
        "undo must remove the added measurement record and enable redo");
    redoAction->trigger();
    require(table->rowCount() == 1 && undoAction->isEnabled() && !redoAction->isEnabled(),
        "redo must restore the measurement record");
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
    undoAction->trigger();
    require(table->rowCount() == 2 && minimum->value() == 7 && redoAction->isEnabled(),
        "undo must restore the detection parameters from before apply");
    redoAction->trigger();
    require(table->rowCount() == 2 && minimum->value() == 8,
        "redo must restore the applied detection parameters");
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
    std::cout << "PASS: detection records, undo/redo history, parameter isolation and invalidation semantics\n";
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
    GraphicalProgramExecutionPlan executionPlan;
    require(GraphicalProgramRegistry::loadExecutionPlan(packagePath, executionPlan, error)
        && executionPlan.descriptor.programNumber == 61
        && executionPlan.steps.size() == 1
        && executionPlan.steps.first().type == QStringLiteral("直径")
        && executionPlan.steps.first().contract.axes == QVector<int>({ 5 }),
        qPrintable(QStringLiteral("generated execution plan load failed: %1").arg(error)));
    QVector<GraphicalProgramMotionTarget> motionTargets;
    require(GraphicalProgramRuntimePlanning::buildMotionTargets(
            executionPlan.steps.first(), motionTargets, error)
        && motionTargets.size() == 1
        && motionTargets.first().axisEncoderTargets.value(5) == 12345,
        qPrintable(QStringLiteral("diameter runtime target planning failed: %1").arg(error)));
    GraphicalProgramStep threeSectionStep = executionPlan.steps.first();
    threeSectionStep.type = QStringLiteral("圆柱度");
    threeSectionStep.contract = GraphicalProgramGeneration::contractForType(threeSectionStep.type);
    threeSectionStep.definition[QStringLiteral("lowerAxialOffsetPulse")] = 100;
    threeSectionStep.definition[QStringLiteral("upperAxialOffsetPulse")] = 200;
    require(GraphicalProgramRuntimePlanning::buildMotionTargets(
            threeSectionStep, motionTargets, error)
        && motionTargets.size() == 3
        && motionTargets.at(0).axisEncoderTargets.value(5) == 12245
        && motionTargets.at(1).axisEncoderTargets.value(5) == 12345
        && motionTargets.at(2).axisEncoderTargets.value(5) == 12545,
        qPrintable(QStringLiteral("three-section runtime target planning failed: %1").arg(error)));
    QStringList dispatchedTargets;
    require(GraphicalProgramRuntimePlanning::dispatchMotionTargets(motionTargets,
            [&](const GraphicalProgramMotionTarget& target, QString&) {
                dispatchedTargets.append(target.label);
                return true;
            }, []() { return false; }, error)
        && dispatchedTargets == QStringList({ QStringLiteral("下侧截面"),
            QStringLiteral("中间截面"), QStringLiteral("上侧截面") }),
        qPrintable(QStringLiteral("three-section motion dispatch failed: %1").arg(error)));
    int failedDispatchCount = 0;
    require(!GraphicalProgramRuntimePlanning::dispatchMotionTargets(motionTargets,
            [&](const GraphicalProgramMotionTarget&, QString& targetError) {
                ++failedDispatchCount;
                if (failedDispatchCount == 2) {
                    targetError = QStringLiteral("simulated motion fault");
                    return false;
                }
                return true;
            }, []() { return false; }, error)
        && failedDispatchCount == 2
        && error.contains(QStringLiteral("中间截面"))
        && error.contains(QStringLiteral("simulated motion fault")),
        "motion dispatch must stop at the first failed target");
    QJsonObject incompleteDefinition = executionPlan.steps.first().definition;
    QJsonObject incompletePosition = incompleteDefinition.value(QStringLiteral("devicePosition")).toObject();
    incompletePosition[QStringLiteral("status")] = QStringLiteral("uncollected");
    incompleteDefinition[QStringLiteral("devicePosition")] = incompletePosition;
    GraphicalProgramStep invalidStep;
    require(!GraphicalProgramRegistry::buildExecutionStep(incompleteDefinition, invalidStep, error)
        && error.contains(QStringLiteral("未包含完整硬件点位")),
        "execution plan must reject a record without a collected hardware point");
    QHash<int, QSet<int>> visualFeatures;
    QSet<int> featureIds;
    featureIds.insert(11);
    featureIds.insert(12);
    visualFeatures.insert(1, featureIds);
    QJsonObject visualRecord;
    visualRecord[QStringLiteral("sequence")] = 1;
    visualRecord[QStringLiteral("featureNumber")] = QStringLiteral("A1");
    visualRecord[QStringLiteral("type")] = QStringLiteral("角度");
    visualRecord[QStringLiteral("frameId")] = 1;
    visualRecord[QStringLiteral("geometryId")] = 11;
    visualRecord[QStringLiteral("secondaryFrameId")] = 1;
    visualRecord[QStringLiteral("secondaryGeometryId")] = 12;
    visualRecord[QStringLiteral("hasTolerance")] = false;
    visualRecord[QStringLiteral("nominal")] = 0.0;
    visualRecord[QStringLiteral("lower")] = 0.0;
    visualRecord[QStringLiteral("upper")] = 0.0;
    QJsonObject axisPosition;
    axisPosition[QStringLiteral("axis")] = 5;
    axisPosition[QStringLiteral("planned")] = 123.0;
    axisPosition[QStringLiteral("encoder")] = 124.0;
    QJsonObject visualPosition;
    visualPosition[QStringLiteral("status")] = QStringLiteral("collected");
    visualPosition[QStringLiteral("source")] = QStringLiteral("hardware");
    visualPosition[QStringLiteral("unit")] = QStringLiteral("pulse");
    visualPosition[QStringLiteral("cameraIndex")] = 0;
    visualPosition[QStringLiteral("exposure")] = 10;
    visualPosition[QStringLiteral("axes")] = QJsonArray{ axisPosition };
    visualRecord[QStringLiteral("devicePosition")] = visualPosition;
    GraphicalProgramStep visualStep;
    require(GraphicalProgramRegistry::buildExecutionStep(visualRecord, visualStep, error, &visualFeatures)
        && visualStep.contract.requiresImage && visualStep.contract.requiresSecondRoi,
        qPrintable(QStringLiteral("visual execution step should accept existing ROIs: %1").arg(error)));
    QHash<int, QSet<int>> missingSecondRoi;
    QSet<int> onlyPrimaryFeature;
    onlyPrimaryFeature.insert(11);
    missingSecondRoi.insert(1, onlyPrimaryFeature);
    require(!GraphicalProgramRegistry::buildExecutionStep(visualRecord, visualStep, error, &missingSecondRoi)
        && error.contains(QStringLiteral("第二运行ROI")),
        "visual execution step must reject a missing second ROI before dispatch");
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
        testGraphicalFreshImageAcquisition();
        testGraphicalAcquisitionDispatcher();
        testGraphicalMeasurementDispatcher();
        testGraphicalProgramRuntimePipeline();
        testGraphicalProgramRunnerCore();
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
