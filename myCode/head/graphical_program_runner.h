#pragma once

#include "graphical_program_registry.h"
#include "graphical_sensor_measurement.h"

#include <QDateTime>
#include <QHash>
#include <QImage>
#include <QString>
#include <QVector>

#include <exception>
#include <functional>

enum class GraphicalProgramRunState {
    Idle,
    Loading,
    Moving,
    Capturing,
    Computing,
    Complete,
    Failed,
    Cancelled
};

struct GraphicalProgramRuntimeFrame {
    bool hasImage = false;
    QImage image;
    int cameraIndex = -1;
    int exposure = -1;
    QString source;
    QVector<double> compensatedDiameterSamples;
    QVector<double> roundoutDistanceSamples;
};

struct GraphicalProgramMotionTarget {
    QString label;
    QHash<int, qint64> axisEncoderTargets;
    int cameraIndex = -1;
    int exposure = -1;
};

class GraphicalProgramRuntimePlanning final
{
public:
    static bool buildMotionTargets(const GraphicalProgramStep& step,
        QVector<GraphicalProgramMotionTarget>& targets, QString& error)
    {
        targets.clear();
        error.clear();
        const auto readPosition = [&](const QJsonObject& position, const QString& label,
            GraphicalProgramMotionTarget& target) {
            if (position.value(QStringLiteral("status")).toString() != QStringLiteral("collected")
                || !position.value(QStringLiteral("axes")).isArray()) {
                error = QStringLiteral("记录%1的%2运行点位无效。").arg(step.sequence).arg(label);
                return false;
            }
            target.label = label;
            target.cameraIndex = position.value(QStringLiteral("cameraIndex")).toInt(-1);
            target.exposure = position.value(QStringLiteral("exposure")).toInt(-1);
            for (const QJsonValue& value : position.value(QStringLiteral("axes")).toArray()) {
                const QJsonObject axis = value.toObject();
                const int axisNumber = axis.value(QStringLiteral("axis")).toInt();
                const double encoder = axis.value(QStringLiteral("encoder")).toDouble(
                    std::numeric_limits<double>::quiet_NaN());
                if (axisNumber <= 0 || !std::isfinite(encoder)
                    || target.axisEncoderTargets.contains(axisNumber)) {
                    error = QStringLiteral("记录%1的%2轴目标无效。").arg(step.sequence).arg(label);
                    return false;
                }
                target.axisEncoderTargets.insert(axisNumber, qRound64(encoder));
            }
            if (target.axisEncoderTargets.isEmpty()) {
                error = QStringLiteral("记录%1的%2没有轴目标。").arg(step.sequence).arg(label);
                return false;
            }
            return true;
        };

        if (step.definition.value(QStringLiteral("crossFrameLength")).toBool(false)) {
            GraphicalProgramMotionTarget start, end;
            if (!readPosition(step.definition.value(QStringLiteral("lengthStartPosition")).toObject(),
                    QStringLiteral("起点"), start)
                || !readPosition(step.definition.value(QStringLiteral("lengthEndPosition")).toObject(),
                    QStringLiteral("终点"), end)) return false;
            targets = { start, end };
            return true;
        }

        GraphicalProgramMotionTarget middle;
        if (!readPosition(step.definition.value(QStringLiteral("devicePosition")).toObject(),
                QStringLiteral("测量点"), middle)) return false;
        if (!step.contract.threeSectionScan) {
            targets.append(middle);
            return true;
        }
        if (!middle.axisEncoderTargets.contains(5)) {
            error = QStringLiteral("记录%1的三截面点位缺少轴5。").arg(step.sequence);
            return false;
        }
        const auto sections = GraphicalSensorMeasurement::axialPositions(
            middle.axisEncoderTargets.value(5),
            qRound64(step.definition.value(QStringLiteral("lowerAxialOffsetPulse")).toDouble()),
            qRound64(step.definition.value(QStringLiteral("upperAxialOffsetPulse")).toDouble()));
        if (!sections.ok) { error = sections.error; return false; }
        GraphicalProgramMotionTarget lower = middle, upper = middle;
        lower.label = QStringLiteral("下侧截面");
        middle.label = QStringLiteral("中间截面");
        upper.label = QStringLiteral("上侧截面");
        lower.axisEncoderTargets[5] = sections.lower;
        middle.axisEncoderTargets[5] = sections.middle;
        upper.axisEncoderTargets[5] = sections.upper;
        targets = { lower, middle, upper };
        return true;
    }

    static bool dispatchMotionTargets(const QVector<GraphicalProgramMotionTarget>& targets,
        const std::function<bool(const GraphicalProgramMotionTarget&, QString&)>& execute,
        const std::function<bool()>& cancelRequested, QString& error)
    {
        error.clear();
        if (targets.isEmpty() || !execute) {
            error = QStringLiteral("运行运动目标或执行回调无效。");
            return false;
        }
        for (const GraphicalProgramMotionTarget& target : targets) {
            if (cancelRequested && cancelRequested()) {
                error = QStringLiteral("运行运动已取消，未执行%1。").arg(target.label);
                return false;
            }
            QString targetError;
            if (!execute(target, targetError)) {
                error = QStringLiteral("%1运动失败：%2").arg(target.label,
                    targetError.isEmpty() ? QStringLiteral("后端未返回原因") : targetError);
                return false;
            }
        }
        return true;
    }
};

struct GraphicalProgramMeasurementResult {
    QString featureNumber;
    QString type;
    double value = 0.0;
    QString unit;
    bool hasTolerance = false;
    double nominal = 0.0;
    double lower = 0.0;
    double upper = 0.0;
    QString judgement;
    QDateTime time;
    QString error;

    static GraphicalProgramMeasurementResult fromStepValue(
        const GraphicalProgramStep& step, double measuredValue, const QString& measuredUnit)
    {
        GraphicalProgramMeasurementResult result;
        result.featureNumber = step.featureNumber;
        result.type = step.type;
        result.value = measuredValue;
        result.unit = measuredUnit;
        result.hasTolerance = step.definition.value(QStringLiteral("hasTolerance")).toBool(false);
        result.nominal = step.definition.value(QStringLiteral("nominal")).toDouble(0.0);
        result.lower = step.definition.value(QStringLiteral("lower")).toDouble(0.0);
        result.upper = step.definition.value(QStringLiteral("upper")).toDouble(0.0);
        result.time = QDateTime::currentDateTime();
        result.judgement = judgementFor(result.hasTolerance, result.nominal,
            result.lower, result.upper, result.value);
        return result;
    }

    static GraphicalProgramMeasurementResult failed(
        const GraphicalProgramStep& step, const QString& message)
    {
        GraphicalProgramMeasurementResult result;
        result.featureNumber = step.featureNumber;
        result.type = step.type;
        result.unit = unitForType(step.type);
        result.hasTolerance = step.definition.value(QStringLiteral("hasTolerance")).toBool(false);
        result.nominal = step.definition.value(QStringLiteral("nominal")).toDouble(0.0);
        result.lower = step.definition.value(QStringLiteral("lower")).toDouble(0.0);
        result.upper = step.definition.value(QStringLiteral("upper")).toDouble(0.0);
        result.judgement = QStringLiteral("错误");
        result.time = QDateTime::currentDateTime();
        result.error = message;
        return result;
    }

    static QString unitForType(const QString& type)
    {
        return type == QStringLiteral("角度") ? QStringLiteral("deg") : QStringLiteral("mm");
    }

    static QString judgementFor(bool hasTolerance, double nominal,
        double lower, double upper, double measuredValue)
    {
        if (!hasTolerance) return QStringLiteral("未判定");
        const double lowLimit = nominal + lower;
        const double highLimit = nominal + upper;
        return measuredValue >= lowLimit && measuredValue <= highLimit
            ? QStringLiteral("OK") : QStringLiteral("NG");
    }
};

struct GraphicalProgramRunStepResult {
    bool ok = false;
    GraphicalProgramRuntimeFrame frame;
    QVector<GraphicalProgramMeasurementResult> measurements;
    QString error;

    static GraphicalProgramRunStepResult success()
    {
        GraphicalProgramRunStepResult result;
        result.ok = true;
        return result;
    }

    static GraphicalProgramRunStepResult successWithFrame(const GraphicalProgramRuntimeFrame& capturedFrame)
    {
        GraphicalProgramRunStepResult result;
        result.ok = true;
        result.frame = capturedFrame;
        return result;
    }

    static GraphicalProgramRunStepResult successWithMeasurements(
        const QVector<GraphicalProgramMeasurementResult>& measuredResults)
    {
        GraphicalProgramRunStepResult result;
        result.ok = true;
        result.measurements = measuredResults;
        return result;
    }

    static GraphicalProgramRunStepResult failure(const QString& message)
    {
        GraphicalProgramRunStepResult result;
        result.error = message;
        return result;
    }
};

struct GraphicalProgramMeasurementCallbacks {
    using Measure = std::function<GraphicalSensorValueResult(const GraphicalProgramStep&,
        const QVector<GraphicalProgramMotionTarget>&,
        const QVector<GraphicalProgramRuntimeFrame>&)>;
    Measure visual;
};

class GraphicalProgramMeasurementDispatcher final
{
public:
    static GraphicalProgramRunStepResult compute(const GraphicalProgramStep& step,
        const QVector<GraphicalProgramMotionTarget>& targets,
        const QVector<GraphicalProgramRuntimeFrame>& frames,
        const GraphicalProgramMeasurementCallbacks& callbacks)
    {
        const int expectedCount = step.contract.threeSectionScan ? 3
            : (step.definition.value(QStringLiteral("crossFrameLength")).toBool(false) ? 2 : 1);
        if (targets.size() != expectedCount || frames.size() != expectedCount
            || frames.size() != targets.size()) {
            return GraphicalProgramRunStepResult::failure(
                QStringLiteral("记录%1的运行目标或采集帧数量无效。").arg(step.sequence));
        }
        if (step.contract.requiresImage) {
            for (int index = 0; index < frames.size(); ++index) {
                if (!frames.at(index).hasImage || frames.at(index).image.isNull()
                    || frames.at(index).cameraIndex != step.contract.cameraIndex) {
                    return GraphicalProgramRunStepResult::failure(
                        QStringLiteral("记录%1的第%2帧图像或相机通道无效。")
                            .arg(step.sequence).arg(index + 1));
                }
            }
        }

        GraphicalSensorValueResult value;
        if (step.type == QStringLiteral("直径")) {
            value = GraphicalSensorMeasurement::diameterMean(
                frames.first().compensatedDiameterSamples);
        }
        else if (step.type == QStringLiteral("圆柱度")) {
            std::array<QVector<double>, 3> sections;
            for (int index = 0; index < 3; ++index)
                sections[index] = frames.at(index).compensatedDiameterSamples;
            value = GraphicalSensorMeasurement::cylindricity(sections);
        }
        else if (step.type == QStringLiteral("跳动")) {
            std::array<double, 3> sectionResults{};
            for (int index = 0; index < 3; ++index) {
                const GraphicalSensorValueResult section =
                    GraphicalSensorMeasurement::roundoutFromDistances(
                        frames.at(index).roundoutDistanceSamples);
                if (!section.ok) {
                    return GraphicalProgramRunStepResult::failure(
                        QStringLiteral("圆跳动第%1截面计算失败：%2")
                            .arg(index + 1).arg(section.error));
                }
                sectionResults[index] = section.value;
            }
            value = GraphicalSensorMeasurement::representativeRoundout(sectionResults);
        }
        else if (step.type == QStringLiteral("角度") || step.type == QStringLiteral("孔径")
            || step.type == QStringLiteral("长度") || step.type == QStringLiteral("圆弧半径")) {
            if (!callbacks.visual) {
                return GraphicalProgramRunStepResult::failure(
                    QStringLiteral("记录%1的%2计算后端尚未接入。")
                        .arg(step.sequence).arg(step.type));
            }
            value = callbacks.visual(step, targets, frames);
        }
        else {
            return GraphicalProgramRunStepResult::failure(
                QStringLiteral("记录%1的%2计算后端尚未接入。")
                    .arg(step.sequence).arg(step.type));
        }
        if (!value.ok || !std::isfinite(value.value)) {
            return GraphicalProgramRunStepResult::failure(value.error.isEmpty()
                ? QStringLiteral("记录%1的%2计算未返回有效值。")
                    .arg(step.sequence).arg(step.type)
                : value.error);
        }
        const GraphicalProgramMeasurementResult measurement =
            GraphicalProgramMeasurementResult::fromStepValue(step, value.value,
                GraphicalProgramMeasurementResult::unitForType(step.type));
        return GraphicalProgramRunStepResult::successWithMeasurements(
            QVector<GraphicalProgramMeasurementResult>{ measurement });
    }
};

struct GraphicalProgramRunResult {
    bool ok = false;
    GraphicalProgramRunState finalState = GraphicalProgramRunState::Idle;
    int completedSteps = 0;
    bool stopAttempted = false;
    bool stopSucceeded = true;
    QVector<GraphicalProgramMeasurementResult> measurements;
    int failedSequence = 0;
    QString failedFeatureNumber;
    QString failedType;
    QString error;

    QString overallJudgement() const
    {
        if (!ok) return QStringLiteral("错误");
        if (measurements.isEmpty()) return QStringLiteral("未判定");
        bool allOk = true;
        bool hasNg = false;
        for (const GraphicalProgramMeasurementResult& measurement : measurements) {
            if (!measurement.error.isEmpty() || measurement.judgement == QStringLiteral("错误"))
                return QStringLiteral("错误");
            if (measurement.judgement == QStringLiteral("NG")) hasNg = true;
            if (measurement.judgement != QStringLiteral("OK")) allOk = false;
        }
        if (hasNg) return QStringLiteral("NG");
        return allOk ? QStringLiteral("OK") : QStringLiteral("未判定");
    }

    int ngMeasurementCount() const
    {
        int count = 0;
        for (const GraphicalProgramMeasurementResult& measurement : measurements)
            if (measurement.judgement == QStringLiteral("NG")) ++count;
        return count;
    }
};

struct GraphicalProgramRunnerCallbacks {
    std::function<void(GraphicalProgramRunState, const GraphicalProgramStep*)> stateChanged;
    std::function<void()> clearPreviousResults;
    std::function<bool()> cancelRequested;
    std::function<GraphicalProgramRunStepResult(const GraphicalProgramStep&)> validateStep;
    std::function<GraphicalProgramRunStepResult(const GraphicalProgramStep&)> moveToStep;
    std::function<GraphicalProgramRunStepResult(const GraphicalProgramStep&)> captureStep;
    std::function<GraphicalProgramRunStepResult(const GraphicalProgramStep&,
        const GraphicalProgramRuntimeFrame&)> computeStep;
    std::function<GraphicalProgramRunStepResult(const GraphicalProgramStep&,
        const GraphicalProgramMotionTarget&)> moveToTarget;
    std::function<GraphicalProgramRunStepResult(const GraphicalProgramStep&,
        const GraphicalProgramMotionTarget&)> captureTarget;
    std::function<GraphicalProgramRunStepResult(const GraphicalProgramStep&,
        const QVector<GraphicalProgramMotionTarget>&,
        const QVector<GraphicalProgramRuntimeFrame>&)> computeTargets;
    std::function<bool()> requestStop;
};

class GraphicalProgramRunner final
{
public:
    GraphicalProgramRunner() = default;

    GraphicalProgramRunState state() const { return m_state; }

    GraphicalProgramRunResult execute(const GraphicalProgramExecutionPlan& plan,
        const GraphicalProgramRunnerCallbacks& callbacks)
    {
        GraphicalProgramRunResult result;
        try {
            m_state = GraphicalProgramRunState::Idle;
            if (plan.steps.isEmpty()) {
                return fail(result, callbacks, nullptr, GraphicalProgramRunState::Failed,
                    QStringLiteral("执行计划没有测量步骤。"), false);
            }
            const bool hasAnyTargetCallbacks = callbacks.moveToTarget
                || callbacks.captureTarget || callbacks.computeTargets;
            const bool hasTargetCallbacks = callbacks.moveToTarget
                && callbacks.captureTarget && callbacks.computeTargets;
            const bool hasLegacyCallbacks = callbacks.moveToStep
                && callbacks.captureStep && callbacks.computeStep;
            if (!callbacks.validateStep || !callbacks.requestStop
                || (hasAnyTargetCallbacks && !hasTargetCallbacks)
                || (!hasTargetCallbacks && !hasLegacyCallbacks)) {
                return fail(result, callbacks, nullptr, GraphicalProgramRunState::Failed,
                    QStringLiteral("统一运行器回调未完整接入。"), false);
            }
            setState(callbacks, GraphicalProgramRunState::Loading, nullptr);
            if (callbacks.clearPreviousResults) callbacks.clearPreviousResults();
            for (const GraphicalProgramStep& step : plan.steps) {
                if (isCancelRequested(callbacks)) {
                    return fail(result, callbacks, &step, GraphicalProgramRunState::Cancelled,
                        QStringLiteral("自动测量已取消。"), true);
                }
                const GraphicalProgramRunStepResult validation = callbacks.validateStep(step);
                if (!validation.ok) {
                    return fail(result, callbacks, &step, GraphicalProgramRunState::Failed,
                        validation.error.isEmpty()
                            ? QStringLiteral("执行步骤校验失败。") : validation.error,
                        false);
                }
                if (hasTargetCallbacks) {
                    QVector<GraphicalProgramMotionTarget> targets;
                    QString planningError;
                    if (!GraphicalProgramRuntimePlanning::buildMotionTargets(
                            step, targets, planningError)) {
                        return fail(result, callbacks, &step, GraphicalProgramRunState::Failed,
                            planningError.isEmpty()
                                ? QStringLiteral("运行运动目标规划失败。") : planningError,
                            false);
                    }
                    QVector<GraphicalProgramRuntimeFrame> capturedFrames;
                    capturedFrames.reserve(targets.size());
                    for (const GraphicalProgramMotionTarget& target : targets) {
                        if (isCancelRequested(callbacks)) {
                            return fail(result, callbacks, &step,
                                GraphicalProgramRunState::Cancelled,
                                QStringLiteral("自动测量已取消。"), true);
                        }
                        setState(callbacks, GraphicalProgramRunState::Moving, &step);
                        GraphicalProgramRunStepResult stepResult =
                            callbacks.moveToTarget(step, target);
                        if (!stepResult.ok) {
                            return fail(result, callbacks, &step,
                                GraphicalProgramRunState::Failed,
                                QStringLiteral("%1运动失败：%2").arg(target.label,
                                    stepResult.error.isEmpty()
                                        ? QStringLiteral("后端未返回原因") : stepResult.error),
                                true);
                        }
                        if (isCancelRequested(callbacks)) {
                            return fail(result, callbacks, &step,
                                GraphicalProgramRunState::Cancelled,
                                QStringLiteral("自动测量已取消。"), true);
                        }
                        setState(callbacks, GraphicalProgramRunState::Capturing, &step);
                        stepResult = callbacks.captureTarget(step, target);
                        if (!stepResult.ok) {
                            return fail(result, callbacks, &step,
                                GraphicalProgramRunState::Failed,
                                QStringLiteral("%1采集失败：%2").arg(target.label,
                                    stepResult.error.isEmpty()
                                        ? QStringLiteral("后端未返回原因") : stepResult.error),
                                true);
                        }
                        capturedFrames.append(stepResult.frame);
                    }
                    if (isCancelRequested(callbacks)) {
                        return fail(result, callbacks, &step,
                            GraphicalProgramRunState::Cancelled,
                            QStringLiteral("自动测量已取消。"), true);
                    }
                    setState(callbacks, GraphicalProgramRunState::Computing, &step);
                    const GraphicalProgramRunStepResult stepResult =
                        callbacks.computeTargets(step, targets, capturedFrames);
                    if (!stepResult.ok) {
                        if (!stepResult.measurements.isEmpty())
                            result.measurements += stepResult.measurements;
                        return fail(result, callbacks, &step, GraphicalProgramRunState::Failed,
                            stepResult.error.isEmpty()
                                ? QStringLiteral("计算失败。") : stepResult.error,
                            true);
                    }
                    result.measurements += stepResult.measurements;
                    ++result.completedSteps;
                    continue;
                }
                setState(callbacks, GraphicalProgramRunState::Moving, &step);
                GraphicalProgramRunStepResult stepResult = callbacks.moveToStep(step);
                if (!stepResult.ok) {
                    return fail(result, callbacks, &step, GraphicalProgramRunState::Failed,
                        stepResult.error.isEmpty() ? QStringLiteral("运动到位失败。") : stepResult.error,
                        true);
                }
                if (isCancelRequested(callbacks)) {
                    return fail(result, callbacks, &step, GraphicalProgramRunState::Cancelled,
                        QStringLiteral("自动测量已取消。"), true);
                }
                setState(callbacks, GraphicalProgramRunState::Capturing, &step);
                stepResult = callbacks.captureStep(step);
                if (!stepResult.ok) {
                    return fail(result, callbacks, &step, GraphicalProgramRunState::Failed,
                        stepResult.error.isEmpty() ? QStringLiteral("采集失败。") : stepResult.error,
                        true);
                }
                const GraphicalProgramRuntimeFrame capturedFrame = stepResult.frame;
                if (isCancelRequested(callbacks)) {
                    return fail(result, callbacks, &step, GraphicalProgramRunState::Cancelled,
                        QStringLiteral("自动测量已取消。"), true);
                }
                setState(callbacks, GraphicalProgramRunState::Computing, &step);
                stepResult = callbacks.computeStep(step, capturedFrame);
                if (!stepResult.ok) {
                    if (!stepResult.measurements.isEmpty())
                        result.measurements += stepResult.measurements;
                    return fail(result, callbacks, &step, GraphicalProgramRunState::Failed,
                        stepResult.error.isEmpty() ? QStringLiteral("计算失败。") : stepResult.error,
                        true);
                }
                result.measurements += stepResult.measurements;
                ++result.completedSteps;
            }
            result.ok = true;
            result.finalState = GraphicalProgramRunState::Complete;
            setState(callbacks, GraphicalProgramRunState::Complete, nullptr);
            return result;
        }
        catch (const std::exception& ex) {
            return fail(result, callbacks, nullptr, GraphicalProgramRunState::Failed,
                QStringLiteral("统一运行器异常：%1").arg(QString::fromLocal8Bit(ex.what())), true);
        }
        catch (...) {
            return fail(result, callbacks, nullptr, GraphicalProgramRunState::Failed,
                QStringLiteral("统一运行器发生未知异常。"), true);
        }
    }

private:
    static bool isCancelRequested(const GraphicalProgramRunnerCallbacks& callbacks)
    {
        return callbacks.cancelRequested && callbacks.cancelRequested();
    }

    void setState(const GraphicalProgramRunnerCallbacks& callbacks,
        GraphicalProgramRunState state, const GraphicalProgramStep* step)
    {
        m_state = state;
        if (callbacks.stateChanged) callbacks.stateChanged(state, step);
    }

    GraphicalProgramRunResult fail(GraphicalProgramRunResult result,
        const GraphicalProgramRunnerCallbacks& callbacks, const GraphicalProgramStep* step,
        GraphicalProgramRunState finalState, const QString& message, bool requestStop)
    {
        if (requestStop && callbacks.requestStop) {
            result.stopAttempted = true;
            result.stopSucceeded = callbacks.requestStop();
        }
        result.ok = false;
        result.finalState = finalState;
        if (step) {
            result.failedSequence = step->sequence;
            result.failedFeatureNumber = step->featureNumber;
            result.failedType = step->type;
        }
        result.error = message;
        if (result.stopAttempted && !result.stopSucceeded)
            result.error += QStringLiteral("；停止请求失败");
        setState(callbacks, finalState, step);
        return result;
    }

    GraphicalProgramRunState m_state = GraphicalProgramRunState::Idle;
};
