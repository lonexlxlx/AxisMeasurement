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
