#pragma once

#include <QPointF>
#include <QPainterPath>
#include <QVector>
#include <algorithm>
#include <cmath>
#include <limits>

// Fitted image edges, never user-drawn line geometry.
struct GraphicalCornerEdge {
    int candidateId = 0;
    QPointF first;
    QPointF second;
    double maxDeviation = 0;
    QPainterPath contour;
};

struct GraphicalCornerPair {
    int firstEdge = -1;
    int secondEdge = -1;
    QPointF vertex;
    double smallerAngle = -1;
};

inline double cornerCross(const QPointF& first, const QPointF& second)
{
    return first.x() * second.y() - first.y() * second.x();
}

inline double cornerLength(const QPointF& vector)
{
    return std::hypot(vector.x(), vector.y());
}

inline double cornerNormalProjection(const QPointF& point, const QPointF& lineDirection)
{
    const double length = cornerLength(lineDirection);
    if (!std::isfinite(length) || length < 1e-12
        || !std::isfinite(point.x()) || !std::isfinite(point.y()))
        return std::numeric_limits<double>::quiet_NaN();
    const QPointF normal(-lineDirection.y() / length, lineDirection.x() / length);
    return point.x() * normal.x() + point.y() * normal.y();
}

inline bool cornerCommonDirection(const GraphicalCornerEdge& first,
    const GraphicalCornerEdge& second, QPointF& direction)
{
    QPointF firstDirection = first.second - first.first;
    QPointF secondDirection = second.second - second.first;
    const double firstLength = cornerLength(firstDirection);
    const double secondLength = cornerLength(secondDirection);
    if (!std::isfinite(firstLength) || !std::isfinite(secondLength)
        || firstLength < 1e-12 || secondLength < 1e-12) return false;
    firstDirection /= firstLength;
    secondDirection /= secondLength;
    if (QPointF::dotProduct(firstDirection, secondDirection) < 0) secondDirection = -secondDirection;
    direction = firstDirection + secondDirection;
    const double length = cornerLength(direction);
    if (!std::isfinite(length) || length < 1e-12) return false;
    direction /= length;
    return std::isfinite(direction.x()) && std::isfinite(direction.y());
}

inline double cornerProjectedOverlap(const GraphicalCornerEdge& first,
    const GraphicalCornerEdge& second, double* ratio = nullptr)
{
    QPointF axis;
    if (!cornerCommonDirection(first, second, axis)) return -1;
    const auto interval = [axis](const GraphicalCornerEdge& edge, double& low, double& high) {
        const double a = QPointF::dotProduct(edge.first, axis);
        const double b = QPointF::dotProduct(edge.second, axis);
        if (!std::isfinite(a) || !std::isfinite(b)) return false;
        low = qMin(a, b); high = qMax(a, b);
        return true;
    };
    double firstLow, firstHigh, secondLow, secondHigh;
    if (!interval(first, firstLow, firstHigh) || !interval(second, secondLow, secondHigh)) return -1;
    const double overlap = qMax(0.0, qMin(firstHigh, secondHigh) - qMax(firstLow, secondLow));
    const double shorterSpan = qMin(firstHigh - firstLow, secondHigh - secondLow);
    if (ratio) *ratio = shorterSpan > 1e-12 ? overlap / shorterSpan : 0;
    return overlap;
}

inline bool cornerOverlapReferencePoint(const GraphicalCornerEdge& first,
    const GraphicalCornerEdge& second, const QPointF& preferredPoint, QPointF& referencePoint)
{
    if (!std::isfinite(preferredPoint.x()) || !std::isfinite(preferredPoint.y())) return false;
    QPointF axis;
    if (!cornerCommonDirection(first, second, axis)) return false;
    const auto interval = [axis](const GraphicalCornerEdge& edge, double& low, double& high) {
        const double a = QPointF::dotProduct(edge.first, axis);
        const double b = QPointF::dotProduct(edge.second, axis);
        if (!std::isfinite(a) || !std::isfinite(b)) return false;
        low = qMin(a, b); high = qMax(a, b);
        return true;
    };
    double firstLow, firstHigh, secondLow, secondHigh;
    if (!interval(first, firstLow, firstHigh) || !interval(second, secondLow, secondHigh)) return false;
    const double overlapLow = qMax(firstLow, secondLow);
    const double overlapHigh = qMin(firstHigh, secondHigh);
    if (overlapHigh < overlapLow) return false;
    const double preferredPosition = QPointF::dotProduct(preferredPoint, axis);
    const double sectionPosition = qBound(overlapLow, preferredPosition, overlapHigh);
    referencePoint = preferredPoint + axis * (sectionPosition - preferredPosition);
    return std::isfinite(referencePoint.x()) && std::isfinite(referencePoint.y());
}

struct GraphicalParallelLineMeasurement {
    QPointF firstPoint;
    QPointF secondPoint;
    double distance = std::numeric_limits<double>::quiet_NaN();
};

inline bool cornerSharedDirectionMeasurement(const GraphicalCornerEdge& first,
    const GraphicalCornerEdge& second, const QPointF& referencePoint,
    GraphicalParallelLineMeasurement& measurement)
{
    if (!std::isfinite(referencePoint.x()) || !std::isfinite(referencePoint.y())) return false;
    QPointF commonDirection;
    if (!cornerCommonDirection(first, second, commonDirection)) return false;
    const QPointF normal(-commonDirection.y(), commonDirection.x());
    const QPointF firstMidpoint = (first.first + first.second) / 2.0;
    const QPointF secondMidpoint = (second.first + second.second) / 2.0;
    const double firstOffset = QPointF::dotProduct(firstMidpoint, normal);
    const double secondOffset = QPointF::dotProduct(secondMidpoint, normal);
    const double sectionPosition = QPointF::dotProduct(referencePoint, commonDirection);
    if (!std::isfinite(firstOffset) || !std::isfinite(secondOffset)
        || !std::isfinite(sectionPosition)) return false;
    measurement.firstPoint = commonDirection * sectionPosition + normal * firstOffset;
    measurement.secondPoint = commonDirection * sectionPosition + normal * secondOffset;
    measurement.distance = std::abs(secondOffset - firstOffset);
    return std::isfinite(measurement.distance) && measurement.distance > 0;
}

inline bool cornerParallelLineMeasurement(const GraphicalCornerEdge& first,
    const GraphicalCornerEdge& second, GraphicalParallelLineMeasurement& measurement,
    const QPointF* referencePoint = nullptr)
{
    for (const QPointF& point : { first.first, first.second, second.first, second.second })
        if (!std::isfinite(point.x()) || !std::isfinite(point.y())) return false;
    QPointF firstDirection = first.second - first.first;
    QPointF secondDirection = second.second - second.first;
    firstDirection /= cornerLength(firstDirection);
    secondDirection /= cornerLength(secondDirection);
    if (QPointF::dotProduct(firstDirection, secondDirection) < 0) secondDirection = -secondDirection;
    QPointF commonDirection;
    if (!cornerCommonDirection(first, second, commonDirection)) return false;

    if (referencePoint && (!std::isfinite(referencePoint->x())
        || !std::isfinite(referencePoint->y()))) return false;
    const double sectionPosition = referencePoint
        ? QPointF::dotProduct(*referencePoint, commonDirection)
        : (QPointF::dotProduct(first.first, commonDirection)
            + QPointF::dotProduct(first.second, commonDirection)
            + QPointF::dotProduct(second.first, commonDirection)
            + QPointF::dotProduct(second.second, commonDirection)) / 4.0;
    const auto pointAtSection = [sectionPosition, commonDirection](const QPointF& origin,
        const QPointF& direction, QPointF& point) {
        const double denominator = QPointF::dotProduct(direction, commonDirection);
        if (!std::isfinite(denominator) || std::abs(denominator) < 1e-12) return false;
        const double offset = (sectionPosition - QPointF::dotProduct(origin, commonDirection)) / denominator;
        point = origin + direction * offset;
        return std::isfinite(point.x()) && std::isfinite(point.y());
    };
    if (!pointAtSection(first.first, firstDirection, measurement.firstPoint)
        || !pointAtSection(second.first, secondDirection, measurement.secondPoint)) return false;
    measurement.distance = cornerLength(measurement.secondPoint - measurement.firstPoint);
    return std::isfinite(measurement.distance) && measurement.distance > 0;
}

inline double cornerParallelLineSeparation(const GraphicalCornerEdge& first,
    const GraphicalCornerEdge& second)
{
    GraphicalParallelLineMeasurement measurement;
    return cornerParallelLineMeasurement(first, second, measurement)
        ? measurement.distance : std::numeric_limits<double>::quiet_NaN();
}

inline double cornerPointSegmentDistance(const QPointF& point, const QPointF& first, const QPointF& second)
{
    const QPointF segment = second - first;
    const double squaredLength = segment.x() * segment.x() + segment.y() * segment.y();
    if (!std::isfinite(squaredLength) || squaredLength < 1e-12) return cornerLength(point - first);
    const QPointF offset = point - first;
    const double position = qBound(0.0,
        (offset.x() * segment.x() + offset.y() * segment.y()) / squaredLength, 1.0);
    return cornerLength(point - (first + segment * position));
}

inline double cornerDeviationPercentile(QVector<double> deviations, double percentile = 0.90)
{
    if (deviations.isEmpty() || !std::isfinite(percentile) || percentile <= 0 || percentile > 1)
        return std::numeric_limits<double>::infinity();
    for (double value : deviations)
        if (!std::isfinite(value) || value < 0) return std::numeric_limits<double>::infinity();
    std::sort(deviations.begin(), deviations.end());
    const int index = qBound(0, int(std::ceil(percentile * deviations.size())) - 1, deviations.size() - 1);
    return deviations[index];
}

inline bool cornerPairGeometry(const GraphicalCornerEdge& first, const GraphicalCornerEdge& second,
    double maxGap, GraphicalCornerPair& pair)
{
    if (!std::isfinite(maxGap) || maxGap < 0) return false;
    for (const QPointF& point : { first.first, first.second, second.first, second.second })
        if (!std::isfinite(point.x()) || !std::isfinite(point.y())) return false;
    const QPointF a = first.second - first.first;
    const QPointF b = second.second - second.first;
    const double aLength = cornerLength(a), bLength = cornerLength(b);
    if (aLength < 1e-6 || bLength < 1e-6) return false;
    const QPointF u = a / aLength, v = b / bLength;
    const double determinant = cornerCross(u, v);
    if (std::abs(determinant) < 1e-6) return false; // Parallel or numerically indistinguishable.
    const QPointF vertex = first.first + u * (cornerCross(second.first - first.first, v) / determinant);
    if (!std::isfinite(vertex.x()) || !std::isfinite(vertex.y())) return false;
    if (qMin(cornerLength(vertex - first.first), cornerLength(vertex - first.second)) > maxGap
        || qMin(cornerLength(vertex - second.first), cornerLength(vertex - second.second)) > maxGap) return false;
    const double dot = qBound(-1.0, u.x() * v.x() + u.y() * v.y(), 1.0);
    pair.vertex = vertex;
    pair.smallerAngle = std::atan2(std::abs(determinant), std::abs(dot)) * 180.0 / 3.14159265358979323846;
    return std::isfinite(pair.smallerAngle);
}

inline QPainterPath cornerPairOverlay(const GraphicalCornerEdge& first, const GraphicalCornerEdge& second,
    const GraphicalCornerPair& pair, bool supplementary)
{
    QPainterPath path;
    path.moveTo(first.first); path.lineTo(first.second);
    path.moveTo(second.first); path.lineTo(second.second);
    QPointF u = cornerLength(first.first - pair.vertex) > cornerLength(first.second - pair.vertex)
        ? first.first - pair.vertex : first.second - pair.vertex;
    QPointF v = cornerLength(second.first - pair.vertex) > cornerLength(second.second - pair.vertex)
        ? second.first - pair.vertex : second.second - pair.vertex;
    if (cornerLength(u) < 1e-6 || cornerLength(v) < 1e-6) return path;
    u /= cornerLength(u); v /= cornerLength(v);
    double dot = u.x() * v.x() + u.y() * v.y();
    if ((!supplementary && dot < 0) || (supplementary && dot > 0)) v = -v;
    dot = qBound(-1.0, u.x() * v.x() + u.y() * v.y(), 1.0);
    const double sweep = std::atan2(cornerCross(u, v), dot);
    const double start = std::atan2(u.y(), u.x());
    const double radius = qBound(2.0, qMin(cornerLength(first.second - first.first),
        cornerLength(second.second - second.first)) * 0.25, 20.0);
    path.moveTo(pair.vertex); path.lineTo(pair.vertex + u * radius);
    for (int i = 1; i <= 32; ++i) {
        const double angle = start + sweep * i / 32.0;
        path.lineTo(pair.vertex + QPointF(std::cos(angle), std::sin(angle)) * radius);
    }
    path.lineTo(pair.vertex);
    return path;
}
