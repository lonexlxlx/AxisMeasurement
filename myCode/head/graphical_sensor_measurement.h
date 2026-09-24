#pragma once

#include <QString>
#include <QVector>
#include <QtGlobal>

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <limits>

struct GraphicalSensorValueResult {
    bool ok = false;
    double value = 0;
    QString error;

    static GraphicalSensorValueResult success(double measured)
    {
        GraphicalSensorValueResult result;
        result.ok = true;
        result.value = measured;
        return result;
    }

    static GraphicalSensorValueResult failure(const QString& message)
    {
        GraphicalSensorValueResult result;
        result.error = message;
        return result;
    }
};

struct GraphicalSensorAxialPositions {
    bool ok = false;
    qint64 lower = 0;
    qint64 middle = 0;
    qint64 upper = 0;
    QString error;
};

struct GraphicalSensorAxisPointResult {
    bool ok = false;
    double x = 0;
    double y = 0;
    double z = 0;
    QString error;
};

struct GraphicalSensorMotionState {
    bool valid = false;
    bool moving = false;
    bool arrived = false;
    bool fault = false;
    QString error;
};

struct GraphicalSensorMotionResult {
    bool ok = false;
    bool stopAttempted = false;
    bool stopSucceeded = true;
    QString error;
};

class GraphicalSensorMeasurement final
{
public:
    // 中间点位加减两个独立偏移，避免旧记录路径中上下偏移变量对调。
    static GraphicalSensorAxialPositions axialPositions(qint64 middle,
        qint64 lowerOffset, qint64 upperOffset)
    {
        GraphicalSensorAxialPositions result;
        if (lowerOffset <= 0 || upperOffset <= 0) {
            result.error = QStringLiteral("轴5上下偏移必须为大于0的pulse值。");
            return result;
        }
        if (middle < std::numeric_limits<qint64>::min() + lowerOffset
            || middle > std::numeric_limits<qint64>::max() - upperOffset) {
            result.error = QStringLiteral("轴5三截面点位超出整数范围。");
            return result;
        }
        result.ok = true;
        result.lower = middle - lowerOffset;
        result.middle = middle;
        result.upper = middle + upperOffset;
        return result;
    }

    // 输入必须是已由原软件diameter_compensation处理后的OUT1样本。
    static GraphicalSensorValueResult diameterMean(const QVector<double>& compensatedSamples)
    {
        if (compensatedSamples.isEmpty())
            return GraphicalSensorValueResult::failure(QStringLiteral("直径没有有效光幕样本。"));
        long double sum = 0;
        for (double sample : compensatedSamples) {
            if (!std::isfinite(sample) || sample <= 0)
                return GraphicalSensorValueResult::failure(QStringLiteral("直径光幕样本包含非有限值或非正数。"));
            sum += sample;
        }
        const double value = double(sum / compensatedSamples.size());
        if (!std::isfinite(value) || value <= 0)
            return GraphicalSensorValueResult::failure(QStringLiteral("直径平均值无效。"));
        return GraphicalSensorValueResult::success(value);
    }

    // 保留旧程序的计算语义：合并三个截面，排序后忽略最高14个直径，
    // 以第15大直径和最小直径之差的一半作为圆柱度。
    static GraphicalSensorValueResult cylindricity(
        const std::array<QVector<double>, 3>& compensatedSections,
        int minimumSamplesPerSection = 15)
    {
        if (minimumSamplesPerSection < 1)
            return GraphicalSensorValueResult::failure(QStringLiteral("圆柱度最小采样数配置无效。"));
        QVector<double> samples;
        for (int section = 0; section < 3; ++section) {
            if (compensatedSections[section].size() < minimumSamplesPerSection)
                return GraphicalSensorValueResult::failure(
                    QStringLiteral("圆柱度第%1截面样本不足：%2，至少需要%3。").arg(section + 1)
                    .arg(compensatedSections[section].size()).arg(minimumSamplesPerSection));
            for (double sample : compensatedSections[section]) {
                if (!std::isfinite(sample) || sample <= 0)
                    return GraphicalSensorValueResult::failure(
                        QStringLiteral("圆柱度第%1截面包含非有限值或非正数。").arg(section + 1));
                samples.append(sample);
            }
        }
        constexpr int legacyHighSampleRank = 15;
        if (samples.size() < legacyHighSampleRank)
            return GraphicalSensorValueResult::failure(QStringLiteral("圆柱度总样本不足。"));
        std::sort(samples.begin(), samples.end());
        const double value = (samples[samples.size() - legacyHighSampleRank] - samples.first()) / 2.0;
        if (!std::isfinite(value) || value < 0)
            return GraphicalSensorValueResult::failure(QStringLiteral("圆柱度计算结果无效。"));
        return GraphicalSensorValueResult::success(value);
    }

    // 依据余弦拟合得到的旋转中心和基准轴心，将整周半径样本转换为到基准轴的距离。
    static bool roundoutDistances(const QVector<double>& radiusSamples,
        double eccentricity, double phase, double axisX, double axisY,
        bool closingSampleRepeated, QVector<double>& distances, QString& error)
    {
        error.clear();
        distances.clear();
        if (radiusSamples.size() < 25) {
            error = QStringLiteral("圆跳动截面样本不足：%1，至少需要25。").arg(radiusSamples.size());
            return false;
        }
        if (!std::isfinite(eccentricity) || !std::isfinite(phase)
            || !std::isfinite(axisX) || !std::isfinite(axisY)) {
            error = QStringLiteral("圆跳动拟合或基准轴参数包含非有限值。");
            return false;
        }
        const int angularIntervals = closingSampleRepeated
            ? radiusSamples.size() - 1 : radiusSamples.size();
        if (angularIntervals <= 0) {
            error = QStringLiteral("圆跳动角度采样区间无效。");
            return false;
        }
        constexpr double pi = 3.1415926535897932384626433832795;
        const double step = 2.0 * pi / angularIntervals;
        const double centerX = eccentricity * std::cos(phase);
        const double centerY = eccentricity * std::sin(phase);
        double angle = pi;
        distances.reserve(radiusSamples.size());
        for (double radius : radiusSamples) {
            if (!std::isfinite(radius) || radius <= 0) {
                error = QStringLiteral("圆跳动半径样本包含非有限值或非正数。");
                distances.clear();
                return false;
            }
            const double x = centerX + radius * std::cos(angle);
            const double y = centerY + radius * std::sin(angle);
            const double distance = std::hypot(x - axisX, y - axisY);
            if (!std::isfinite(distance)) {
                error = QStringLiteral("圆跳动距离计算产生非有限值。");
                distances.clear();
                return false;
            }
            distances.append(distance);
            angle += step;
        }
        return true;
    }

    // 保留旧程序每端剔除12个极值后的峰谷差，同时补齐样本边界检查。
    static GraphicalSensorValueResult roundoutFromDistances(const QVector<double>& input)
    {
        constexpr int trimEachEnd = 12;
        if (input.size() <= trimEachEnd * 2)
            return GraphicalSensorValueResult::failure(
                QStringLiteral("圆跳动距离样本不足：%1，至少需要25。").arg(input.size()));
        QVector<double> values = input;
        for (double value : values)
            if (!std::isfinite(value) || value < 0)
                return GraphicalSensorValueResult::failure(QStringLiteral("圆跳动距离样本包含无效值。"));
        std::sort(values.begin(), values.end());
        const double result = values[values.size() - trimEachEnd - 1] - values[trimEachEnd];
        if (!std::isfinite(result) || result < 0)
            return GraphicalSensorValueResult::failure(QStringLiteral("圆跳动计算结果无效。"));
        return GraphicalSensorValueResult::success(result);
    }

    static GraphicalSensorValueResult representativeRoundout(
        const std::array<double, 3>& sectionResults)
    {
        std::array<double, 3> values = sectionResults;
        for (double value : values)
            if (!std::isfinite(value) || value < 0)
                return GraphicalSensorValueResult::failure(QStringLiteral("圆跳动三截面结果包含无效值。"));
        std::sort(values.begin(), values.end());
        return GraphicalSensorValueResult::success(values[1]);
    }

    static GraphicalSensorAxisPointResult axisPointAtZ(
        const std::array<double, 3>& referencePoint,
        const std::array<double, 3>& direction, double targetZ)
    {
        GraphicalSensorAxisPointResult result;
        for (double value : referencePoint)
            if (!std::isfinite(value)) {
                result.error = QStringLiteral("圆跳动基准轴参考点包含非有限值。");
                return result;
            }
        for (double value : direction)
            if (!std::isfinite(value)) {
                result.error = QStringLiteral("圆跳动基准轴方向包含非有限值。");
                return result;
            }
        if (!std::isfinite(targetZ) || std::abs(direction[2]) <= 1e-12) {
            result.error = QStringLiteral("圆跳动基准轴退化，无法投影到目标轴5位置。");
            return result;
        }
        const double scale = (targetZ - referencePoint[2]) / direction[2];
        result.x = referencePoint[0] + scale * direction[0];
        result.y = referencePoint[1] + scale * direction[1];
        result.z = targetZ;
        result.ok = std::isfinite(result.x) && std::isfinite(result.y);
        if (!result.ok) result.error = QStringLiteral("圆跳动基准轴投影结果无效。");
        return result;
    }

    // 运动后端由调用者提供。取消、故障、未到位停止和超时都会主动请求停止。
    static GraphicalSensorMotionResult waitForMotion(
        const std::function<GraphicalSensorMotionState()>& query,
        const std::function<bool()>& stop,
        const std::function<bool()>& cancelRequested,
        const std::function<void(int)>& delay,
        int timeoutMs, int pollIntervalMs)
    {
        GraphicalSensorMotionResult result;
        if (!query || !stop || !delay || timeoutMs <= 0 || pollIntervalMs <= 0) {
            result.error = QStringLiteral("运动等待策略或回调无效。");
            return result;
        }
        const int maximumPolls = qMax(1, (timeoutMs + pollIntervalMs - 1) / pollIntervalMs);
        const auto failAndStop = [&](const QString& message) {
            GraphicalSensorMotionResult failure;
            failure.stopAttempted = true;
            failure.stopSucceeded = stop();
            failure.error = message;
            if (!failure.stopSucceeded) failure.error += QStringLiteral("；停止命令失败");
            return failure;
        };
        for (int poll = 0; poll <= maximumPolls; ++poll) {
            if (cancelRequested && cancelRequested())
                return failAndStop(QStringLiteral("传感器测量已取消。"));
            const GraphicalSensorMotionState state = query();
            if (!state.valid)
                return failAndStop(state.error.isEmpty()
                    ? QStringLiteral("无法读取运动状态。") : state.error);
            if (state.fault)
                return failAndStop(state.error.isEmpty()
                    ? QStringLiteral("运动轴报告故障。") : state.error);
            if (state.arrived) {
                result.ok = true;
                return result;
            }
            if (!state.moving)
                return failAndStop(QStringLiteral("运动已停止但未确认到位。"));
            if (poll == maximumPolls)
                return failAndStop(QStringLiteral("等待运动到位超时。"));
            delay(pollIntervalMs);
        }
        return failAndStop(QStringLiteral("等待运动到位超时。"));
    }
};
