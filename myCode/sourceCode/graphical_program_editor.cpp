#include "graphical_program_editor.h"

#include "graphical_canvas.h"
#include "sharedFun.h"
#include <QAction>
#include <QComboBox>
#include <QLineEdit>
#include <QCloseEvent>
#include <QIcon>
#include <QKeySequence>
#include <QLabel>
#include <QLineF>
#include <QShortcut>
#include <QThread>
#include <QTransform>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QtMath>
#include <HalconCpp.h>
#include <HIOStream.h>
#include <memory>
#include <cstring>
#include <cmath>
#include <stdexcept>
#include <climits>
#include <sstream>
#include <algorithm>
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
    QVector<GraphicalCornerEdge> cornerEdges;
    QVector<GraphicalCornerPair> cornerPairs;
    QString diagnostic;
};

struct LinearTrialResult {
    double distancePixels = -1;
    double distanceMm = -1;
    double templateScore = -1;
    QByteArray templateModel;
    double templateReferenceRow = 0;
    double templateReferenceColumn = 0;
    double templateReferenceAngle = 0;
    bool templateCreated = false;
    QString status;
    QPainterPath edges;
    QPainterPath fitted;
};

struct CrossFrameLengthTrialResult {
    double startRow = -1;
    double endRow = -1;
    double pixelTermMm = 0;
    double movementMm = 0;
    double distanceMm = -1;
    QString status;
    QPainterPath startEdges;
    QPainterPath startFitted;
    QPainterPath endEdges;
    QPainterPath endFitted;
};

struct HoleTrialResult {
    double diameterPixels = -1;
    double diameterMm = -1;
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

QString extractUniqueContour(const HalconCpp::HObject& reduced, int imageWidth,
    const GraphicalDetectionParameters& parameters, HalconCpp::HObject& joined,
    QPainterPath& edgePath, QString& stage, QString& diagnostic)
{
    using namespace HalconCpp;
    stage = QStringLiteral("参数校验");
    const QString invalid = parameters.validationError();
    if (!invalid.isEmpty()) return invalid;
    const double maximum = parameters.maxLength > 0 ? parameters.maxLength : imageWidth / 2.0;
    if (maximum < parameters.minLength)
        return QStringLiteral("最短轮廓 %1 px 超过有效最长长度 %2 px；请调整长度参数。").arg(parameters.minLength).arg(maximum);
    HObject edges, split, selected;
    HTuple count;
    stage = QStringLiteral("边缘提取");
    EdgesSubPix(reduced, &edges, "canny", parameters.smoothing, parameters.lowThreshold, parameters.highThreshold);
    CountObj(edges, &count);
    diagnostic = QStringLiteral("边缘 %1").arg(qlonglong(count.I()));
    edgePath = contourPath(edges);
    if (count.I() == 0) return QStringLiteral("未提取到边缘；请检查图像对比度、ROI或边缘阈值。");
    stage = QStringLiteral("轮廓分段");
    SegmentContoursXld(edges, &split, "lines_circles", 5, 4, 2);
    CountObj(split, &count);
    diagnostic += QStringLiteral(" → 分段 %1").arg(qlonglong(count.I()));
    if (count.I() == 0) return QStringLiteral("分段后无轮廓；请检查边缘完整性。");
    stage = QStringLiteral("长度筛选");
    SelectContoursXld(split, &selected, "contour_length", parameters.minLength, maximum, -0.5, 0.5);
    CountObj(selected, &count);
    diagnostic += QStringLiteral(" → 筛选 %1（%2–%3 px）").arg(qlonglong(count.I())).arg(parameters.minLength).arg(maximum);
    if (count.I() == 0) return QStringLiteral("轮廓全部被长度筛选排除；请检查最短/最长长度。");
    stage = QStringLiteral("轮廓合并");
    UnionAdjacentContoursXld(selected, &joined, parameters.mergeDistance, 1, "attr_keep");
    CountObj(joined, &count);
    diagnostic += QStringLiteral(" → 合并 %1").arg(qlonglong(count.I()));
    if (count.I() == 0) return QStringLiteral("合并后无有效轮廓。");
    if (count.I() != 1) return QStringLiteral("有效候选 %1 条，无法确定目标；请调整ROI和参数使目标唯一。").arg(qlonglong(count.I()));
    return QString();
}

QString halconTrialFailure(const HalconCpp::HException& error, const QString& stage)
{
    if (error.ErrorCode() == 2042)
        return QStringLiteral("HALCON许可不可用或已过期（2042），无法执行试测；阶段：%1。").arg(stage);
    return QStringLiteral("测量失败［%1］：HALCON %2：%3").arg(stage)
        .arg(qlonglong(error.ErrorCode())).arg(QString::fromLocal8Bit(error.ErrorMessage().Text()));
}

ArcTrialResult runArcTrial(const QImage& source, const GraphicalCanvas::MeasurementRoi& roi,
    const GraphicalDetectionParameters& parameters)
{
    using namespace HalconCpp;
    ArcTrialResult result;
    QString stage = QStringLiteral("图像/ROI准备"), diagnostic;
    try {
        const QImage gray = source.convertToFormat(QImage::Format_Grayscale8);
        if (gray.isNull() || qint64(gray.width()) * gray.height() > 50000000) {
            result.status = QStringLiteral("测量失败：图像为空或超过本阶段5000万像素上限");
            return result;
        }
        QByteArray pixels(gray.width() * gray.height(), '\0');
        for (int row = 0; row < gray.height(); ++row)
            std::memcpy(pixels.data() + row * gray.width(), gray.constScanLine(row), gray.width());
        HObject image, region, reduced, joined, fitted;
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
        const QString failure = extractUniqueContour(reduced, gray.width(), parameters, joined, result.edges, stage, diagnostic);
        if (!failure.isEmpty()) {
            result.status = QStringLiteral("测量失败［%1］：%2\n%3").arg(stage, failure, diagnostic);
            return result;
        }
        stage = QStringLiteral("圆弧拟合");
        HTuple row, column, radius, start, end, order;
        FitCircleContourXld(joined, "algebraic", -1, 0, 0, 3, 2,
            &row, &column, &radius, &start, &end, &order);
        if (radius.Length() != 1 || !std::isfinite(radius[0].D()) || radius[0].D() <= 0
            || radius[0].D() > 100.0 * qMax(gray.width(), gray.height())) {
            result.status = QStringLiteral("测量失败［圆弧拟合］：未取得有效半径\n%1").arg(diagnostic);
            return result;
        }
        GenCircleContourXld(&fitted, row, column, radius, start, end, order, 1.0);
        result.fitted = contourPath(fitted);
        result.radius = radius[0].D();
        result.status = QStringLiteral("试测完成（像素，未标定）\n%1").arg(diagnostic);
    }
    catch (const HException& error) {
        result.status = halconTrialFailure(error, stage) + QStringLiteral("\n") + diagnostic;
    }
    catch (const std::exception& error) {
        result.status = QStringLiteral("测量失败：%1").arg(QString::fromLocal8Bit(error.what()));
    }
    catch (...) { result.status = QStringLiteral("测量失败：未知异常"); }
    return result;
}

LineTrialResult runLineAngleTrial(const QImage& source, const GraphicalCanvas::MeasurementRoi& roi,
    const GraphicalDetectionParameters& parameters)
{
    using namespace HalconCpp;
    LineTrialResult result;
    QString stage = QStringLiteral("图像/ROI准备"), diagnostic;
    try {
        const QImage gray = source.convertToFormat(QImage::Format_Grayscale8);
        if (gray.isNull() || qint64(gray.width()) * gray.height() > 50000000) {
            result.status = QStringLiteral("测量失败：图像为空或超过本阶段5000万像素上限");
            return result;
        }
        QByteArray pixels(gray.width() * gray.height(), '\0');
        for (int row = 0; row < gray.height(); ++row)
            std::memcpy(pixels.data() + row * gray.width(), gray.constScanLine(row), gray.width());
        HObject image, region, reduced, joined;
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
        const QString failure = extractUniqueContour(reduced, gray.width(), parameters, joined, result.edges, stage, diagnostic);
        if (!failure.isEmpty()) {
            result.status = QStringLiteral("测量失败［%1］：%2\n%3").arg(stage, failure, diagnostic);
            return result;
        }
        stage = QStringLiteral("直线拟合");
        HTuple rowBegin, colBegin, rowEnd, colEnd, normalRow, normalCol, distance;
        FitLineContourXld(joined, "tukey", -1, 0, 5, 2,
            &rowBegin, &colBegin, &rowEnd, &colEnd, &normalRow, &normalCol, &distance);
        if (rowBegin.Length() != 1 || !std::isfinite(rowBegin[0].D()) || !std::isfinite(colBegin[0].D())
            || !std::isfinite(rowEnd[0].D()) || !std::isfinite(colEnd[0].D())) {
            result.status = QStringLiteral("测量失败［直线拟合］：未取得有效拟合直线\n%1").arg(diagnostic);
            return result;
        }
        if (std::hypot(rowEnd[0].D() - rowBegin[0].D(), colEnd[0].D() - colBegin[0].D()) < 1e-6) {
            result.status = QStringLiteral("测量失败［直线拟合］：拟合端点重合，无法计算方向\n%1").arg(diagnostic);
            return result;
        }
        double angle = std::atan2(rowEnd[0].D() - rowBegin[0].D(), colEnd[0].D() - colBegin[0].D())
            * 180.0 / 3.14159265358979323846;
        while (angle < 0) angle += 180.0;
        while (angle >= 180.0) angle -= 180.0;
        result.fitted.moveTo(colBegin[0].D(), rowBegin[0].D());
        result.fitted.lineTo(colEnd[0].D(), rowEnd[0].D());
        result.angle = angle;
        result.status = QStringLiteral("试测完成（图像向右0°，顺时针为正，范围0°–180°）\n%1").arg(diagnostic);
    }
    catch (const HException& error) {
        result.status = halconTrialFailure(error, stage) + QStringLiteral("\n") + diagnostic;
    }
    catch (const std::exception& error) {
        result.status = QStringLiteral("测量失败：%1").arg(QString::fromLocal8Bit(error.what()));
    }
    catch (...) { result.status = QStringLiteral("测量失败：未知异常"); }
    return result;
}

LineTrialResult runSingleRoiAngleTrial(const QImage& source,
    const GraphicalCanvas::MeasurementRoi& roi, bool supplementary,
    const GraphicalDetectionParameters& parameters)
{
    using namespace HalconCpp;
    LineTrialResult result;
    QString stage = QStringLiteral("单ROI参数校验");
    try {
        if (!parameters.validationError().isEmpty()) {
            result.status = parameters.validationError(); return result;
        }
        const QImage gray = source.convertToFormat(QImage::Format_Grayscale8);
        if (gray.isNull() || qint64(gray.width()) * gray.height() > 50000000)
            throw std::runtime_error("Invalid image or image exceeds 50 MP");
        const double maximum = parameters.maxLength > 0 ? parameters.maxLength : gray.width() / 2.0;
        if (maximum < parameters.minLength) {
            result.status = QStringLiteral("测量失败：最长轮廓小于最短轮廓，请调整检测参数。"); return result;
        }
        QByteArray pixels(gray.width() * gray.height(), '\0');
        for (int row = 0; row < gray.height(); ++row)
            std::memcpy(pixels.data() + row * gray.width(), gray.constScanLine(row), gray.width());
        HObject image, region, reduced, edges, segments, regressed, collinear, selected;
        stage = QStringLiteral("单ROI边缘提取");
        GenImage1(&image, "byte", gray.width(), gray.height(), reinterpret_cast<Hlong>(pixels.data()));
        if (roi.isCircle) GenCircle(&region, roi.center.y(), roi.center.x(), roi.radius);
        else {
            HTuple rows, columns;
            for (int i = 0; i < roi.corners.size(); ++i) {
                rows[i] = roi.corners[i].y(); columns[i] = roi.corners[i].x();
            }
            GenRegionPolygonFilled(&region, rows, columns);
        }
        ReduceDomain(image, region, &reduced);
        EdgesSubPix(reduced, &edges, "canny", parameters.smoothing, parameters.lowThreshold, parameters.highThreshold);
        result.edges = contourPath(edges);
        stage = QStringLiteral("单ROI直线分段");
        // This mode deliberately returns straight-line segments. A rounded or
        // blurred corner may otherwise be labelled as an elliptic segment by
        // lines_circles and lose one of the two tangent edges before fitting.
        SegmentContoursXld(edges, &segments, "lines", 5, 4, 2);
        HTuple segmentCount, collinearCount, count;
        CountObj(segments, &segmentCount);
        if (segmentCount.I() == 0) {
            result.diagnostic = QStringLiteral("直线分段0条");
            result.status = QStringLiteral("测量失败：边缘未能分割成直线段；请检查ROI、对比度或边缘参数。\n")
                + result.diagnostic;
            return result;
        }
        if (segmentCount.I() > 512) {
            result.diagnostic = QStringLiteral("直线分段%1条").arg(segmentCount.I());
            result.status = QStringLiteral("测量失败：直线分段超过512条，请缩小ROI或提高边缘阈值。\n")
                + result.diagnostic;
            return result;
        }
        result.diagnostic = QStringLiteral("直线分段%1条").arg(segmentCount.I());
        // The legacy line detector also fits a guided physical edge as one
        // line. In the unguided ROI mode the same edge can be split by blur,
        // burrs or a rounded endpoint, so consolidate only nearly-collinear
        // pieces before applying the length and Tukey-fit quality gates.
        stage = QStringLiteral("单ROI共线合并");
        RegressContoursXld(segments, &regressed, "no", 1);
        UnionCollinearContoursXld(regressed, &collinear,
            parameters.cornerMaxGap, 1, parameters.cornerMaxDeviation, 0.10, "attr_keep");
        CountObj(collinear, &collinearCount);
        result.diagnostic += QStringLiteral(" → 共线合并%1条").arg(collinearCount.I());
        if (collinearCount.I() == 0) {
            result.status = QStringLiteral("测量失败：共线合并后没有可拟合轮廓。\n") + result.diagnostic;
            return result;
        }
        stage = QStringLiteral("单ROI长度筛选");
        SelectContoursXld(collinear, &selected, "contour_length", parameters.minLength, maximum, -0.5, 0.5);
        CountObj(selected, &count);
        result.diagnostic += QStringLiteral(" → 长度筛选%1条（%2–%3 px）")
            .arg(count.I()).arg(parameters.minLength).arg(maximum);
        if (count.I() == 0) {
            result.status = QStringLiteral("测量失败：直线轮廓全部被长度条件排除；小倒角可降低最短轮廓。\n")
                + result.diagnostic;
            return result;
        }
        if (count.I() > 256) {
            result.diagnostic = QStringLiteral("直线分段%1条 → 共线合并%2条 → 长度筛选%3条")
                .arg(segmentCount.I()).arg(collinearCount.I()).arg(count.I());
            result.status = QStringLiteral("测量失败：轮廓过多，请缩小ROI或提高边缘阈值。\n")
                + result.diagnostic;
            return result;
        }
        stage = QStringLiteral("单ROI直线拟合");
        int rejectedPointCount = 0;
        int rejectedFit = 0;
        int rejectedSpan = 0;
        int rejectedDeviation = 0;
        for (Hlong i = 1; i <= count.I(); ++i) {
            HObject contour;
            HTuple rows, columns, rb, cb, re, ce, nr, nc, distance;
            SelectObj(selected, &contour, i);
            GetContourXld(contour, &rows, &columns);
            if (rows.Length() < 6 || rows.Length() != columns.Length()) {
                ++rejectedPointCount; continue;
            }
            FitLineContourXld(contour, "tukey", -1, 0, 5, 2, &rb, &cb, &re, &ce, &nr, &nc, &distance);
            if (rb.Length() != 1 || cb.Length() != 1 || re.Length() != 1 || ce.Length() != 1) {
                ++rejectedFit; continue;
            }
            if (!std::isfinite(rb[0].D()) || !std::isfinite(cb[0].D())
                || !std::isfinite(re[0].D()) || !std::isfinite(ce[0].D())) {
                ++rejectedFit; continue;
            }
            GraphicalCornerEdge edge;
            edge.candidateId = static_cast<int>(i);
            edge.first = QPointF(cb[0].D(), rb[0].D()); edge.second = QPointF(ce[0].D(), re[0].D());
            const double length = cornerLength(edge.second - edge.first);
            if (!std::isfinite(length) || length < parameters.minLength) {
                ++rejectedSpan; continue;
            }
            const QPointF direction = (edge.second - edge.first) / length;
            QVector<double> deviations;
            deviations.reserve(static_cast<int>(rows.Length()));
            for (Hlong p = 0; p < rows.Length(); ++p) {
                const double deviation = std::abs(cornerCross(QPointF(columns[p].D(), rows[p].D()) - edge.first, direction));
                deviations.append(deviation);
            }
            // FitLineContourXld already uses Tukey robust fitting. Use P90 for
            // the independent quality gate as well, so one burr or a few
            // rounded transition pixels do not reject the whole fitted edge.
            edge.maxDeviation = cornerDeviationPercentile(deviations);
            if (edge.maxDeviation > parameters.cornerMaxDeviation) {
                ++rejectedDeviation; continue;
            }
            edge.contour = contourPath(contour);
            result.cornerEdges.append(edge);
        }
        if (result.cornerEdges.size() > 32) {
            result.cornerEdges.clear();
            result.status = QStringLiteral("测量失败：有效直线超过32条，请缩小ROI。"); return result;
        }
        stage = QStringLiteral("相邻边对筛选");
        for (int i = 0; i < result.cornerEdges.size(); ++i)
            for (int j = i + 1; j < result.cornerEdges.size(); ++j) {
                GraphicalCornerPair pair;
                if (!cornerPairGeometry(result.cornerEdges[i], result.cornerEdges[j], parameters.cornerMaxGap, pair)) continue;
                bool inside = roi.isCircle ? cornerLength(pair.vertex - roi.center) <= roi.radius + 2.0
                    : roi.corners.containsPoint(pair.vertex, Qt::OddEvenFill);
                if (!inside && !roi.isCircle) {
                    for (int side = 0; side < roi.corners.size(); ++side) {
                        if (cornerPointSegmentDistance(pair.vertex, roi.corners[side],
                            roi.corners[(side + 1) % roi.corners.size()]) <= 2.0) {
                            inside = true; break;
                        }
                    }
                }
                if (!inside || pair.vertex.x() < 0 || pair.vertex.y() < 0
                    || pair.vertex.x() >= gray.width() || pair.vertex.y() >= gray.height()) continue;
                pair.firstEdge = i; pair.secondEdge = j;
                result.cornerPairs.append(pair);
            }
        result.diagnostic = QStringLiteral("直线分段%1条 → 共线合并%2条 → 长度筛选%3条 → 有效直线%4条（点数排除%5、拟合排除%6、跨度排除%7、P90偏差排除%8）→ 相邻边对%9对；90%点线偏差≤%10 px，角点间隙≤%11 px")
            .arg(segmentCount.I()).arg(collinearCount.I()).arg(count.I()).arg(result.cornerEdges.size())
            .arg(rejectedPointCount).arg(rejectedFit).arg(rejectedSpan).arg(rejectedDeviation)
            .arg(result.cornerPairs.size()).arg(parameters.cornerMaxDeviation).arg(parameters.cornerMaxGap);
        if (result.cornerPairs.isEmpty())
            result.status = QStringLiteral("测量失败：未找到相邻直线边对。检查ROI是否包含角点和两条边；小倒角可适当降低最短轮廓或平滑参数。\n") + result.diagnostic;
        else if (result.cornerPairs.size() == 1) {
            const auto& pair = result.cornerPairs[0];
            result.angle = supplementary ? 180 - pair.smallerAngle : pair.smallerAngle;
            result.edges = result.cornerEdges[pair.firstEdge].contour;
            result.edges.addPath(result.cornerEdges[pair.secondEdge].contour);
            result.fitted = cornerPairOverlay(result.cornerEdges[pair.firstEdge], result.cornerEdges[pair.secondEdge], pair, supplementary);
            result.status = QStringLiteral("单ROI试测完成（唯一相邻边对，未判定）\n") + result.diagnostic;
        }
        else result.status = QStringLiteral("待选择候选边对：在测量配置中选择目标，尚未输出角度。\n") + result.diagnostic;
    }
    catch (const HException& error) {
        result.status = halconTrialFailure(error, stage);
        if (!result.diagnostic.isEmpty()) result.status += QStringLiteral("\n") + result.diagnostic;
        result.cornerPairs.clear();
    }
    catch (const std::exception& error) {
        result.status = QStringLiteral("测量失败［%1］：%2").arg(stage, QString::fromLocal8Bit(error.what()));
        if (!result.diagnostic.isEmpty()) result.status += QStringLiteral("\n") + result.diagnostic;
        result.cornerPairs.clear();
    }
    catch (...) { result.status = QStringLiteral("单ROI测量失败：未知异常"); result.cornerPairs.clear(); }
    return result;
}

LineTrialResult runTwoRoiAngleTrial(const QImage& source,
    const GraphicalCanvas::MeasurementRoi& firstRoi,
    const GraphicalCanvas::MeasurementRoi& secondRoi,
    bool supplementary, const GraphicalDetectionParameters& parameters)
{
    LineTrialResult first = runLineAngleTrial(source, firstRoi, parameters);
    if (first.angle < 0) {
        first.status = QStringLiteral("ROI 1：%1").arg(first.status);
        return first;
    }
    LineTrialResult second = runLineAngleTrial(source, secondRoi, parameters);
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
    result.status += QStringLiteral("\nROI 1：%1\nROI 2：%2").arg(first.status.section('\n', 1), second.status.section('\n', 1));
    return result;
}

double undirectedAngleDifference(double first, double second)
{
    double difference = std::abs(first - second);
    while (difference >= 180.0) difference -= 180.0;
    return difference > 90.0 ? 180.0 - difference : difference;
}

bool snapshotMeasurementRoi(const GraphicalCanvas::FeatureSnapshot& snapshot,
    GraphicalCanvas::MeasurementRoi& roi)
{
    roi = GraphicalCanvas::MeasurementRoi();
    if (snapshot.type != QStringLiteral("矩形") || snapshot.points.size() != 1
        || !snapshot.size.isValid() || snapshot.size.width() < 2 || snapshot.size.height() < 2)
        return false;
    const QRectF localBounds(-snapshot.size.width() / 2.0, -snapshot.size.height() / 2.0,
        snapshot.size.width(), snapshot.size.height());
    QTransform transform;
    transform.translate(snapshot.points[0].x(), snapshot.points[0].y());
    transform.rotate(snapshot.rotation);
    for (const QPointF& corner : { localBounds.topLeft(), localBounds.topRight(),
        localBounds.bottomRight(), localBounds.bottomLeft() })
        roi.corners.append(transform.map(corner));
    roi.axisAligned = qAbs(snapshot.rotation / 90.0 - qRound(snapshot.rotation / 90.0)) <= 1e-8;
    return true;
}

struct EndpointLineTrialResult {
    double row = -1;
    QString status;
    QPainterPath edges;
    QPainterPath fitted;
};

EndpointLineTrialResult runCrossFrameEndpointTrial(const QImage& source,
    const GraphicalCanvas::MeasurementRoi& roi,
    const GraphicalDetectionParameters& parameters, const QString& endpointName)
{
    EndpointLineTrialResult result;
    const LineTrialResult candidates = runSingleRoiAngleTrial(source, roi, false, parameters);
    if (roi.corners.size() != 4) {
        result.status = QStringLiteral("%1ROI无效").arg(endpointName);
        return result;
    }
    const QLineF guide(roi.corners[0], roi.corners[1]);
    if (guide.length() < 1e-6) {
        result.status = QStringLiteral("%1ROI方向无效").arg(endpointName);
        return result;
    }
    double expectedAngle = guide.angle() * -1.0;
    while (expectedAngle < 0) expectedAngle += 180.0;
    while (expectedAngle >= 180.0) expectedAngle -= 180.0;
    const QPointF roiCenter = roi.corners.boundingRect().center();
    struct Candidate {
        int index = -1;
        double centerDistance = 0;
        double length = 0;
    };
    QVector<Candidate> aligned;
    for (int index = 0; index < candidates.cornerEdges.size(); ++index) {
        const GraphicalCornerEdge& edge = candidates.cornerEdges[index];
        const QLineF line(edge.first, edge.second);
        double angle = std::atan2(edge.second.y() - edge.first.y(), edge.second.x() - edge.first.x())
            * 180.0 / 3.14159265358979323846;
        while (angle < 0) angle += 180.0;
        while (angle >= 180.0) angle -= 180.0;
        if (undirectedAngleDifference(angle, expectedAngle) > 20.0) continue;
        Candidate candidate;
        candidate.index = index;
        candidate.centerDistance = QLineF((edge.first + edge.second) / 2.0, roiCenter).length();
        candidate.length = line.length();
        aligned.append(candidate);
    }
    if (aligned.isEmpty()) {
        result.status = QStringLiteral("%1未找到与ROI方向一致的目标边；%2")
            .arg(endpointName, candidates.diagnostic.isEmpty() ? candidates.status : candidates.diagnostic);
        return result;
    }
    std::sort(aligned.begin(), aligned.end(), [](const Candidate& first, const Candidate& second) {
        if (!qFuzzyCompare(first.centerDistance + 1.0, second.centerDistance + 1.0))
            return first.centerDistance < second.centerDistance;
        return first.length > second.length;
    });
    if (aligned.size() > 1 && std::abs(aligned[1].centerDistance - aligned[0].centerDistance) <= 2.0
        && aligned[1].length >= aligned[0].length * 0.85) {
        result.status = QStringLiteral("%1检测到%2条方向相符且同样接近ROI中心的边，无法唯一定位；请缩小或移动ROI。%3")
            .arg(endpointName).arg(aligned.size())
            .arg(candidates.diagnostic.isEmpty() ? QString() : QStringLiteral("\n") + candidates.diagnostic);
        return result;
    }
    const GraphicalCornerEdge& selected = candidates.cornerEdges[aligned[0].index];
    result.row = (selected.first.y() + selected.second.y()) / 2.0;
    result.edges = selected.contour;
    result.fitted.moveTo(selected.first);
    result.fitted.lineTo(selected.second);
    result.status = QStringLiteral("%1行坐标=%2 px（方向候选%3条，选取距ROI中心最近边）")
        .arg(endpointName).arg(result.row, 0, 'f', 3).arg(aligned.size());
    return result;
}

CrossFrameLengthTrialResult runCrossFrameLengthTrial(const QImage& startImage,
    const GraphicalCanvas::MeasurementRoi& startRoi, const QImage& endImage,
    const GraphicalCanvas::MeasurementRoi& endRoi, double calibration,
    const GraphicalDetectionParameters& parameters, bool hasPositions, double movementMm)
{
    CrossFrameLengthTrialResult result;
    const EndpointLineTrialResult start = runCrossFrameEndpointTrial(startImage, startRoi, parameters,
        QStringLiteral("起点"));
    result.startEdges = start.edges;
    result.startFitted = start.fitted;
    if (start.row < 0) {
        result.status = QStringLiteral("测量失败［跨图起点拟合］：%1").arg(start.status);
        return result;
    }
    const EndpointLineTrialResult end = runCrossFrameEndpointTrial(endImage, endRoi, parameters,
        QStringLiteral("终点"));
    result.endEdges = end.edges;
    result.endFitted = end.fitted;
    if (end.row < 0) {
        result.status = QStringLiteral("测量失败［跨图终点拟合］：%1；%2").arg(end.status, start.status);
        return result;
    }
    result.startRow = start.row;
    result.endRow = end.row;
    result.pixelTermMm = (result.endRow - result.startRow) * calibration;
    result.movementMm = movementMm;
    if (!hasPositions) {
        result.status = QStringLiteral("跨图两端边缘拟合完成；%1；%2；轴5点位未采集完整，不输出毫米结果")
            .arg(start.status, end.status);
        return result;
    }
    result.distanceMm = result.pixelTermMm + result.movementMm;
    if (!std::isfinite(result.distanceMm) || result.distanceMm <= 0) {
        result.distanceMm = -1;
        result.status = QStringLiteral("测量失败［跨图长度计算］：结果方向不一致或非正值；请检查起终点顺序。起点行=%1 px，终点行=%2 px，像素项=%3 mm，轴5补偿位移=%4 mm")
            .arg(result.startRow, 0, 'f', 3).arg(result.endRow, 0, 'f', 3)
            .arg(result.pixelTermMm, 0, 'f', 4).arg(result.movementMm, 0, 'f', 4);
        return result;
    }
    result.status = QStringLiteral("跨图长度试测完成（未判定）：起点行=%1 px，终点行=%2 px，像素项=%3 mm，轴5补偿位移=%4 mm")
        .arg(result.startRow, 0, 'f', 3).arg(result.endRow, 0, 'f', 3)
        .arg(result.pixelTermMm, 0, 'f', 4).arg(result.movementMm, 0, 'f', 4);
    return result;
}

GraphicalCanvas::MeasurementRoi transformMeasurementRoi(
    const GraphicalCanvas::MeasurementRoi& source, const HalconCpp::HTuple& transform)
{
    using namespace HalconCpp;
    GraphicalCanvas::MeasurementRoi target = source;
    HTuple transformedColumn, transformedRow;
    if (source.isCircle) {
        AffineTransPoint2d(transform, source.center.x(), source.center.y(),
            &transformedColumn, &transformedRow);
        target.center = QPointF(transformedColumn[0].D(), transformedRow[0].D());
        return target;
    }
    target.corners.clear();
    for (const QPointF& point : source.corners) {
        AffineTransPoint2d(transform, point.x(), point.y(), &transformedColumn, &transformedRow);
        target.corners.append(QPointF(transformedColumn[0].D(), transformedRow[0].D()));
    }
    target.axisAligned = false;
    return target;
}

LinearTrialResult runSingleRoiTemplateLengthTrial(const QImage& source,
    const GraphicalCanvas::MeasurementRoi& referenceRoi,
    double calibration, const GraphicalDetectionParameters& parameters,
    const QByteArray& savedTemplateModel, double savedReferenceRow,
    double savedReferenceColumn, double savedReferenceAngle)
{
    using namespace HalconCpp;
    LinearTrialResult result;
    QString stage = QStringLiteral("长度模板准备");
    try {
        if (!std::isfinite(calibration) || calibration <= 0 || calibration > 1) {
            result.status = QStringLiteral("测量失败：远心标定系数须在0–1 mm/px之间");
            return result;
        }
        if (referenceRoi.isCircle || referenceRoi.corners.size() != 4) {
            result.status = QStringLiteral("测量失败［长度ROI］：长度模板须使用矩形ROI框住完整特征和两条目标边");
            return result;
        }
        const QImage gray = source.convertToFormat(QImage::Format_Grayscale8);
        if (gray.isNull() || qint64(gray.width()) * gray.height() > 50000000) {
            result.status = QStringLiteral("测量失败［图像/ROI准备］：图像为空或超过本阶段5000万像素上限");
            return result;
        }
        QByteArray pixels(gray.width() * gray.height(), '\0');
        for (int row = 0; row < gray.height(); ++row)
            std::memcpy(pixels.data() + row * gray.width(), gray.constScanLine(row), gray.width());
        HObject imageObject, region, reduced;
        GenImage1(&imageObject, "byte", gray.width(), gray.height(),
            reinterpret_cast<Hlong>(pixels.data()));
        HImage image(imageObject);
        HShapeModel model;
        double referenceRow = savedReferenceRow;
        double referenceColumn = savedReferenceColumn;
        double referenceAngle = savedReferenceAngle;
        if (savedTemplateModel.isEmpty()) {
            HTuple rows, columns;
            for (int index = 0; index < referenceRoi.corners.size(); ++index) {
                rows[index] = referenceRoi.corners[index].y();
                columns[index] = referenceRoi.corners[index].x();
            }
            GenRegionPolygonFilled(&region, rows, columns);
            ReduceDomain(imageObject, region, &reduced);
            stage = QStringLiteral("长度模板建立");
            HImage templateImage(reduced);
            model.CreateShapeModel(templateImage, HTuple("auto"), -0.17453292519943295,
                0.3490658503988659, HTuple("auto"), HTuple("auto"), "use_polarity",
                HTuple("auto"), HTuple(10));
            result.templateCreated = true;
        }
        else {
            stage = QStringLiteral("长度模板读取");
            const std::string serialized(savedTemplateModel.constData(),
                static_cast<size_t>(savedTemplateModel.size()));
            std::istringstream input(serialized, std::ios::in | std::ios::binary);
            input >> model;
            if (!input) throw std::runtime_error("Invalid serialized shape model");
        }

        stage = QStringLiteral("长度模板匹配");
        HTuple matchRow, matchColumn, matchAngle, matchScore;
        model.FindShapeModel(image, -0.17453292519943295, 0.3490658503988659,
            0.5, 1, 0.3, "least_squares", 0, 0.7,
            &matchRow, &matchColumn, &matchAngle, &matchScore);
        if (matchScore.Length() == 0) {
            result.status = QStringLiteral("测量失败［长度模板匹配］：未找到分数不低于0.5的模板；请检查ROI内特征、对比度和目标位置");
            return result;
        }
        result.templateScore = matchScore[0].D();
        if (result.templateCreated) {
            referenceRow = matchRow[0].D();
            referenceColumn = matchColumn[0].D();
            referenceAngle = matchAngle[0].D();
            std::ostringstream output(std::ios::out | std::ios::binary);
            output << model;
            if (!output) throw std::runtime_error("Shape model serialization failed");
            const std::string serialized = output.str();
            if (serialized.empty() || serialized.size() > 4 * 1024 * 1024)
                throw std::runtime_error("Serialized shape model is empty or exceeds 4 MB");
            result.templateModel = QByteArray(serialized.data(), static_cast<int>(serialized.size()));
            result.templateReferenceRow = referenceRow;
            result.templateReferenceColumn = referenceColumn;
            result.templateReferenceAngle = referenceAngle;
        }
        HTuple transform;
        VectorAngleToRigid(referenceRow, referenceColumn, referenceAngle,
            matchRow[0].D(), matchColumn[0].D(), matchAngle[0].D(), &transform);
        const GraphicalCanvas::MeasurementRoi matchedRoi =
            transformMeasurementRoi(referenceRoi, transform);
        QPainterPath matchedBoundary;
        if (!matchedRoi.corners.isEmpty()) {
            matchedBoundary.moveTo(matchedRoi.corners.first());
            for (int index = 1; index < matchedRoi.corners.size(); ++index)
                matchedBoundary.lineTo(matchedRoi.corners[index]);
            matchedBoundary.closeSubpath();
        }

        stage = QStringLiteral("单ROI目标边拟合");
        LineTrialResult candidates = runSingleRoiAngleTrial(source, matchedRoi, false, parameters);
        result.edges = candidates.edges;
        result.edges.addPath(matchedBoundary);
        const QPointF roiDirection = matchedRoi.corners[1] - matchedRoi.corners[0];
        if (cornerLength(roiDirection) < 1e-6) {
            result.status = QStringLiteral("测量失败［长度ROI］：矩形ROI方向无效");
            return result;
        }
        double expectedAngle = std::atan2(roiDirection.y(), roiDirection.x())
            * 180.0 / 3.14159265358979323846;
        while (expectedAngle < 0) expectedAngle += 180.0;
        while (expectedAngle >= 180.0) expectedAngle -= 180.0;
        QVector<int> eligible;
        for (int index = 0; index < candidates.cornerEdges.size(); ++index) {
            const QPointF delta = candidates.cornerEdges[index].second
                - candidates.cornerEdges[index].first;
            double angle = std::atan2(delta.y(), delta.x()) * 180.0 / 3.14159265358979323846;
            while (angle < 0) angle += 180.0;
            while (angle >= 180.0) angle -= 180.0;
            if (undirectedAngleDifference(angle, expectedAngle) <= 20.0)
                eligible.append(index);
        }
        if (eligible.size() < 2) {
            if (candidates.cornerEdges.isEmpty() && !candidates.status.isEmpty()) {
                result.status = QStringLiteral("测量失败［单ROI目标边拟合］：%1").arg(candidates.status);
                return result;
            }
            result.status = QStringLiteral("测量失败［单ROI目标边拟合］：检测到%1条有效直线，其中仅%2条与ROI横向接近；至少需要两条目标边，请调整ROI或检测参数\n%3")
                .arg(candidates.cornerEdges.size()).arg(eligible.size()).arg(candidates.diagnostic);
            return result;
        }
        QString edgeSelection = QStringLiteral("横向候选2条，直接采用");
        int firstIndex = eligible[0];
        int secondIndex = eligible[1];
        if (eligible.size() > 2) {
            std::sort(eligible.begin(), eligible.end(), [&candidates](int first, int second) {
                const GraphicalCornerEdge& firstEdge = candidates.cornerEdges[first];
                const GraphicalCornerEdge& secondEdge = candidates.cornerEdges[second];
                return (firstEdge.first.y() + firstEdge.second.y())
                    < (secondEdge.first.y() + secondEdge.second.y());
            });
            double largestGap = -1;
            double secondLargestGap = -1;
            int largestGapPosition = -1;
            for (int index = 0; index + 1 < eligible.size(); ++index) {
                const GraphicalCornerEdge& upper = candidates.cornerEdges[eligible[index]];
                const GraphicalCornerEdge& lower = candidates.cornerEdges[eligible[index + 1]];
                const double upperRow = (upper.first.y() + upper.second.y()) / 2.0;
                const double lowerRow = (lower.first.y() + lower.second.y()) / 2.0;
                const double gap = std::abs(lowerRow - upperRow);
                if (gap > largestGap) {
                    secondLargestGap = largestGap;
                    largestGap = gap;
                    largestGapPosition = index;
                }
                else if (gap > secondLargestGap) {
                    secondLargestGap = gap;
                }
            }
            const double requiredLead = qMax(5.0, secondLargestGap * 0.5);
            if (largestGapPosition < 0 || largestGap <= 0
                || (secondLargestGap >= 0 && largestGap - secondLargestGap < requiredLead)) {
                result.status = QStringLiteral("测量失败［单ROI目标边拟合］：检测到%1条横向候选，但相邻间距没有明显唯一的最大值（最大=%2 px，次大=%3 px）；请收紧ROI或调整检测参数\n%4")
                    .arg(eligible.size()).arg(largestGap, 0, 'f', 3)
                    .arg(secondLargestGap, 0, 'f', 3).arg(candidates.diagnostic);
                return result;
            }
            firstIndex = eligible[largestGapPosition];
            secondIndex = eligible[largestGapPosition + 1];
            edgeSelection = QStringLiteral("横向候选%1条，采用相邻间距明显最大的一对（%2 px；次大%3 px）")
                .arg(eligible.size()).arg(largestGap, 0, 'f', 3)
                .arg(secondLargestGap, 0, 'f', 3);
        }
        const GraphicalCornerEdge& first = candidates.cornerEdges[firstIndex];
        const GraphicalCornerEdge& second = candidates.cornerEdges[secondIndex];
        const QPointF firstDelta = first.second - first.first;
        const QPointF secondDelta = second.second - second.first;
        double firstAngle = std::atan2(firstDelta.y(), firstDelta.x()) * 180.0 / 3.14159265358979323846;
        double secondAngle = std::atan2(secondDelta.y(), secondDelta.x()) * 180.0 / 3.14159265358979323846;
        while (firstAngle < 0) firstAngle += 180.0;
        while (firstAngle >= 180.0) firstAngle -= 180.0;
        while (secondAngle < 0) secondAngle += 180.0;
        while (secondAngle >= 180.0) secondAngle -= 180.0;
        const double directionDifference = undirectedAngleDifference(firstAngle, secondAngle);
        if (directionDifference > 10.0) {
            result.status = QStringLiteral("测量失败［长度计算］：两条自动拟合边不平行；边1=%1°，边2=%2°，方向差=%3°")
                .arg(firstAngle, 0, 'f', 3).arg(secondAngle, 0, 'f', 3)
                .arg(directionDifference, 0, 'f', 3);
            return result;
        }
        const QPointF firstMidpoint = (first.first + first.second) / 2.0;
        const QPointF secondMidpoint = (second.first + second.second) / 2.0;
        result.distancePixels = std::abs(secondMidpoint.y() - firstMidpoint.y());
        result.distanceMm = result.distancePixels * calibration;
        if (!std::isfinite(result.distancePixels) || result.distancePixels <= 0
            || !std::isfinite(result.distanceMm) || result.distanceMm <= 0) {
            result.distancePixels = -1;
            result.distanceMm = -1;
            result.status = QStringLiteral("测量失败［长度计算］：两条拟合边的图像Y方向距离无效");
            return result;
        }
        result.edges = first.contour;
        result.edges.addPath(second.contour);
        result.edges.addPath(matchedBoundary);
        result.fitted.moveTo(first.first); result.fitted.lineTo(first.second);
        result.fitted.moveTo(second.first); result.fitted.lineTo(second.second);
        const double connectorX = (firstMidpoint.x() + secondMidpoint.x()) / 2.0;
        result.fitted.moveTo(connectorX, firstMidpoint.y());
        result.fitted.lineTo(connectorX, secondMidpoint.y());
        result.status = QStringLiteral("单ROI模板长度试测完成（未判定）；模板分数=%1（最高分匹配）；%2；边1=%3°，边2=%4°，方向差=%5°；%6")
            .arg(result.templateScore, 0, 'f', 4).arg(edgeSelection).arg(firstAngle, 0, 'f', 3)
            .arg(secondAngle, 0, 'f', 3).arg(directionDifference, 0, 'f', 3)
            .arg(candidates.diagnostic);
    }
    catch (const HException& error) {
        result.distancePixels = -1;
        result.distanceMm = -1;
        result.status = halconTrialFailure(error, stage);
    }
    catch (const std::exception& error) {
        result.distancePixels = -1;
        result.distanceMm = -1;
        result.status = QStringLiteral("测量失败［%1］：%2").arg(stage,
            QString::fromLocal8Bit(error.what()));
    }
    catch (...) {
        result.distancePixels = -1;
        result.distanceMm = -1;
        result.status = QStringLiteral("测量失败［%1］：未知异常").arg(stage);
    }
    return result;
}

HoleTrialResult runHoleDiameterTrial(const QImage& source,
    const GraphicalCanvas::MeasurementRoi& roi,
    double calibration)
{
    using namespace HalconCpp;
    HoleTrialResult result;
    QString stage = QStringLiteral("图像/ROI准备");
    HTuple metrologyHandle;
    if (!std::isfinite(calibration) || calibration <= 0 || calibration > 1) {
        result.status = QStringLiteral("测量失败：测孔标定系数须在0–1 mm/px之间");
        return result;
    }
    try {
        const QImage gray = source.convertToFormat(QImage::Format_Grayscale8);
        if (gray.isNull() || qint64(gray.width()) * gray.height() > 50000000) {
            result.status = QStringLiteral("测量失败［图像/ROI准备］：图像为空或超过本阶段5000万像素上限");
            return result;
        }
        QRectF bounds;
        if (roi.isCircle)
            bounds = QRectF(roi.center.x() - roi.radius, roi.center.y() - roi.radius,
                2.0 * roi.radius, 2.0 * roi.radius);
        else
            bounds = roi.corners.boundingRect();
        bounds = bounds.normalized().intersected(QRectF(0, 0, gray.width() - 1, gray.height() - 1));
        if (bounds.width() < 60 || bounds.height() < 60) {
            result.status = QStringLiteral("测量失败［图像/ROI准备］：孔径ROI过小；宽高至少60px");
            return result;
        }
        QByteArray pixels(gray.width() * gray.height(), '\0');
        for (int row = 0; row < gray.height(); ++row)
            std::memcpy(pixels.data() + row * gray.width(), gray.constScanLine(row), gray.width());

        HObject image, meanImage, emphasized, illuminated, equalized, resultContours;
        GenImage1(&image, "byte", gray.width(), gray.height(), reinterpret_cast<Hlong>(pixels.data()));
        stage = QStringLiteral("原测孔图像增强");
        MeanImage(image, &meanImage, 9, 9);
        Emphasize(meanImage, &emphasized, 3, 3, 0.9);
        Illuminate(emphasized, &illuminated, 10, 85, 0.88);
        EquHistoImage(illuminated, &equalized);

        const double centerX = bounds.center().x();
        const double centerY = bounds.center().y();
        // Locate the two transitions of the inner hole from a vertically smoothed
        // gray profile through the central part of the ROI.  The ROI may include a
        // wide annulus, so deriving probes only from its outer bounds can put one
        // metrology line on the annulus and the other on the inner hole.
        stage = QStringLiteral("ROI内孔定位");
        const int profileLeft = qMax(0, qCeil(bounds.left() + bounds.width() * 0.38));
        const int profileRight = qMin(gray.width() - 1, qFloor(bounds.right() - bounds.width() * 0.38));
        const int profileTop = qMax(0, qCeil(bounds.top()));
        const int profileBottom = qMin(gray.height() - 1, qFloor(bounds.bottom()));
        if (profileRight < profileLeft || profileBottom - profileTop < 40) {
            result.status = QStringLiteral("测量失败［ROI内孔定位］：ROI中央灰度剖面范围无效");
            return result;
        }
        QVector<double> grayProfile(profileBottom - profileTop + 1, 0.0);
        for (int row = profileTop; row <= profileBottom; ++row) {
            const uchar* scan = gray.constScanLine(row);
            double sum = 0;
            for (int column = profileLeft; column <= profileRight; ++column)
                sum += scan[column];
            grayProfile[row - profileTop] = sum / (profileRight - profileLeft + 1);
        }
        const int gradientRadius = qBound(2, qRound(bounds.height() * 0.015), 6);
        const int topStart = qMax(profileTop + gradientRadius, qCeil(bounds.top() + bounds.height() * 0.08));
        const int topEnd = qMin(profileBottom - gradientRadius, qFloor(centerY - bounds.height() * 0.10));
        const int bottomStart = qMax(profileTop + gradientRadius, qCeil(centerY + bounds.height() * 0.10));
        const int bottomEnd = qMin(profileBottom - gradientRadius, qFloor(bounds.bottom() - bounds.height() * 0.08));
        if (topEnd <= topStart || bottomEnd <= bottomStart) {
            result.status = QStringLiteral("测量失败［ROI内孔定位］：ROI没有为上下孔边保留足够搜索范围");
            return result;
        }
        auto profileGradient = [&](int row) {
            return grayProfile[row + gradientRadius - profileTop]
                - grayProfile[row - gradientRadius - profileTop];
        };
        double topMinimum = 1e9, topMaximum = -1e9;
        double bottomMinimum = 1e9, bottomMaximum = -1e9;
        int topMinimumRow = -1, topMaximumRow = -1;
        int bottomMinimumRow = -1, bottomMaximumRow = -1;
        for (int row = topStart; row <= topEnd; ++row) {
            const double gradient = profileGradient(row);
            if (gradient < topMinimum) { topMinimum = gradient; topMinimumRow = row; }
            if (gradient > topMaximum) { topMaximum = gradient; topMaximumRow = row; }
        }
        for (int row = bottomStart; row <= bottomEnd; ++row) {
            const double gradient = profileGradient(row);
            if (gradient < bottomMinimum) { bottomMinimum = gradient; bottomMinimumRow = row; }
            if (gradient > bottomMaximum) { bottomMaximum = gradient; bottomMaximumRow = row; }
        }
        const double darkHoleScore = -topMinimum + bottomMaximum;
        const double brightHoleScore = topMaximum - bottomMinimum;
        const bool darkHole = darkHoleScore >= brightHoleScore;
        const double topY = darkHole ? topMinimumRow : topMaximumRow;
        const double bottomY = darkHole ? bottomMaximumRow : bottomMinimumRow;
        const double topContrast = darkHole ? -topMinimum : topMaximum;
        const double bottomContrast = darkHole ? bottomMaximum : -bottomMinimum;
        if (topY < 0 || bottomY < 0 || bottomY - topY < bounds.height() * 0.25
            || topContrast < 5.0 || bottomContrast < 5.0) {
            result.status = QStringLiteral("测量失败［ROI内孔定位］：未找到成对的内孔上下灰度跃迁；请让ROI完整包含孔和周围少量背景");
            return result;
        }
        const double estimatedDiameter = bottomY - topY;
        // Keep the probes around the central, near-tangent part of the two edges.
        // The following metrology and Tukey parameters remain those of the legacy
        // hole algorithm; only their image positions are supplied by the ROI.
        const double halfSpan = qMin(bounds.width() * 0.35,
            qBound(30.0, estimatedDiameter * 0.12, 55.0));
        HTuple topLine, bottomLine, lineIndices;
        topLine[0] = topY; topLine[1] = centerX - halfSpan;
        topLine[2] = topY; topLine[3] = centerX + halfSpan;
        bottomLine[0] = bottomY; bottomLine[1] = centerX - halfSpan;
        bottomLine[2] = bottomY; bottomLine[3] = centerX + halfSpan;
        result.fitted.moveTo(centerX - halfSpan, topY);
        result.fitted.lineTo(centerX + halfSpan, topY);
        result.fitted.moveTo(centerX - halfSpan, bottomY);
        result.fitted.lineTo(centerX + halfSpan, bottomY);

        stage = QStringLiteral("ROI上下边Metrology");
        CreateMetrologyModel(&metrologyHandle);
        AddMetrologyObjectGeneric(metrologyHandle, "line", topLine.TupleConcat(bottomLine),
            20, 12, 10, 1, HTuple(), HTuple(), &lineIndices);
        ApplyMetrologyModel(equalized, metrologyHandle);
        GetMetrologyObjectResultContour(&resultContours, metrologyHandle, "all", "all", 1.5);
        result.edges = contourPath(resultContours);

        stage = QStringLiteral("上下边Tukey拟合");
        HTuple rowBegin, colBegin, rowEnd, colEnd, normalRow, normalCol, distance;
        FitLineContourXld(resultContours, "tukey", -1, 0, 5, 2,
            &rowBegin, &colBegin, &rowEnd, &colEnd, &normalRow, &normalCol, &distance);
        if (rowBegin.Length() < 2 || colBegin.Length() < 2
            || rowEnd.Length() < 2 || colEnd.Length() < 2) {
            ClearMetrologyModel(metrologyHandle); metrologyHandle.Clear();
            result.status = QStringLiteral("测量失败［上下边Tukey拟合］：未同时取得孔的上边缘和下边缘；请调整孔径ROI，使其框住完整孔并减少其他边缘干扰");
            return result;
        }
        const QPointF begin0(colBegin[0].D(), rowBegin[0].D());
        const QPointF end0(colEnd[0].D(), rowEnd[0].D());
        const QPointF begin1(colBegin[1].D(), rowBegin[1].D());
        const QPointF end1(colEnd[1].D(), rowEnd[1].D());
        if (!std::isfinite(begin0.x()) || !std::isfinite(begin0.y())
            || !std::isfinite(end0.x()) || !std::isfinite(end0.y())
            || !std::isfinite(begin1.x()) || !std::isfinite(begin1.y())
            || !std::isfinite(end1.x()) || !std::isfinite(end1.y())) {
            ClearMetrologyModel(metrologyHandle); metrologyHandle.Clear();
            result.status = QStringLiteral("测量失败［上下边Tukey拟合］：拟合端点包含无效数值");
            return result;
        }
        const QPointF points0[] = { begin0, end0, begin0, end0 };
        const QPointF points1[] = { end1, begin1, begin1, end1 };
        double candidateDistances[4] = { -1, -1, -1, -1 };
        double maximumDistance = -1;
        int maximumIndex = -1;
        for (int index = 0; index < 4; ++index) {
            const double candidate = QLineF(points0[index], points1[index]).length();
            candidateDistances[index] = candidate;
            if (std::isfinite(candidate) && candidate > maximumDistance) {
                maximumDistance = candidate;
                maximumIndex = index;
            }
        }
        ClearMetrologyModel(metrologyHandle); metrologyHandle.Clear();
        if (maximumIndex < 0 || maximumDistance <= 0) {
            result.status = QStringLiteral("测量失败［孔径计算］：上下边缘的跨边距离无效");
            return result;
        }
        result.fitted = QPainterPath();
        result.fitted.moveTo(begin0); result.fitted.lineTo(end0);
        result.fitted.moveTo(begin1); result.fitted.lineTo(end1);
        result.fitted.moveTo(points0[maximumIndex]); result.fitted.lineTo(points1[maximumIndex]);
        result.diameterPixels = maximumDistance;
        result.diameterMm = maximumDistance * calibration;
        if (!std::isfinite(result.diameterMm) || result.diameterMm <= 0) {
            result.diameterPixels = -1;
            result.diameterMm = -1;
            result.status = QStringLiteral("测量失败［孔径计算］：标定后的孔径无效");
            return result;
        }
        const QPointF midpoint0 = (begin0 + end0) / 2.0;
        const QPointF midpoint1 = (begin1 + end1) / 2.0;
        const double midpointDistance = QLineF(midpoint0, midpoint1).length();
        const double firstLineLength = QLineF(begin0, end0).length();
        const double secondLineLength = QLineF(begin1, end1).length();
        result.status = QStringLiteral("孔径试测完成（ROI灰度定位内孔上下边，原算法四组跨边距离取最大值，未判定）"
            "；定位=%1，上下行=%2/%3，对比=%4/%5；四组距离=%6/%7/%8/%9 px，取第%10组；中点距=%11 px；拟合线长=%12/%13 px")
            .arg(darkHole ? QStringLiteral("暗孔") : QStringLiteral("亮孔"))
            .arg(topY, 0, 'f', 1)
            .arg(bottomY, 0, 'f', 1)
            .arg(topContrast, 0, 'f', 1)
            .arg(bottomContrast, 0, 'f', 1)
            .arg(candidateDistances[0], 0, 'f', 3)
            .arg(candidateDistances[1], 0, 'f', 3)
            .arg(candidateDistances[2], 0, 'f', 3)
            .arg(candidateDistances[3], 0, 'f', 3)
            .arg(maximumIndex + 1)
            .arg(midpointDistance, 0, 'f', 3)
            .arg(firstLineLength, 0, 'f', 3)
            .arg(secondLineLength, 0, 'f', 3);
    }
    catch (const HException& error) {
        if (metrologyHandle.Length() > 0) {
            try { ClearMetrologyModel(metrologyHandle); } catch (...) {}
        }
        result.status = error.ErrorCode() == 8573 && stage == QStringLiteral("ROI上下边Metrology")
            ? QStringLiteral("测量失败［ROI上下边Metrology］：HALCON 8573：灰度定位后的上下测量区没有取得足够有效边缘点；画布叠加线为本次实际搜索位置，请让ROI完整包含孔和周围少量背景后重试")
            : halconTrialFailure(error, stage);
    }
    catch (const std::exception& error) {
        if (metrologyHandle.Length() > 0) {
            try { ClearMetrologyModel(metrologyHandle); } catch (...) {}
        }
        result.status = QStringLiteral("测量失败［%1］：%2").arg(stage, QString::fromLocal8Bit(error.what()));
    }
    catch (...) {
        if (metrologyHandle.Length() > 0) {
            try { ClearMetrologyModel(metrologyHandle); } catch (...) {}
        }
        result.status = QStringLiteral("测量失败［%1］：未知异常").arg(stage);
    }
    return result;
}
}
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QActionGroup>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QDir>
#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSet>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QList>
#include <QListWidget>
#include <QMessageBox>
#include <QMenu>
#include <QMenuBar>
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
#include <QStandardPaths>
#include <QUuid>

// Temporary offline layout preview; restore false after the user's feedback.
namespace {
constexpr bool kGraphicalAxisLayoutPreview = false;

QVector<int> deviceAxesForMeasurement(const QString& type)
{
    if (type == QStringLiteral("孔径")) return {2, 5};
    return {5};
}

int deviceCameraForMeasurement(const QString& type)
{
    if (type == QStringLiteral("孔径")) return 1;
    if (type == QStringLiteral("直径") || type == QStringLiteral("圆柱度")
        || type == QStringLiteral("跳动")) return -1;
    return 0;
}

void setCanvasSourceBadge(GraphicalCanvas* canvas, const QString& text)
{
    if (!canvas) return;
    if (QLabel* badge = canvas->findChild<QLabel*>(QStringLiteral("canvasSourceBadge"))) {
        badge->setText(text);
        badge->setVisible(!text.isEmpty());
        badge->raise();
    }
}
}

static QByteArray projectFileSha256(const QString& filePath, QString& error);

static bool writePngAtomically(const QImage& image, const QString& filePath, QString& error)
{
    QSaveFile output(filePath);
    if (!output.open(QIODevice::WriteOnly)) {
        error = output.errorString();
        return false;
    }
    if (!image.save(&output, "PNG")) {
        output.cancelWriting();
        error = QStringLiteral("无法编码PNG图像。");
        return false;
    }
    if (!output.commit()) {
        error = output.errorString();
        return false;
    }
    return true;
}

static QString cameraCaptureCachePath(int camera, QString& error)
{
    const QString applicationData = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (applicationData.isEmpty()) {
        error = QStringLiteral("无法确定应用数据目录。");
        return QString();
    }
    QDir directory(applicationData);
    if (!directory.mkpath(QStringLiteral("capture-cache"))) {
        error = QStringLiteral("无法创建相机图像缓存目录。");
        return QString();
    }
    if (!directory.cd(QStringLiteral("capture-cache"))) {
        error = QStringLiteral("无法访问相机图像缓存目录。");
        return QString();
    }
    const QString timestamp = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd_HHmmss_zzz"));
    const QString uniqueId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    return directory.filePath(QStringLiteral("camera_%1_%2_%3.png").arg(camera).arg(timestamp).arg(uniqueId));
}

GraphicalProgramEditor::GraphicalProgramEditor(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("图形化二次开发"));
    setObjectName(QStringLiteral("graphicalProgramEditor"));
    resize(1500, 900);
    setWindowFlag(Qt::Window, true);
    setWindowModality(Qt::NonModal);
    buildInterface();
    QTimer* axisTimer = new QTimer(this);
    connect(axisTimer, &QTimer::timeout, this, &GraphicalProgramEditor::refreshAxisPanel);
    connect(axisTimer, &QTimer::timeout, this, &GraphicalProgramEditor::refreshCameraPanel);
    axisTimer->start(200);
}

void GraphicalProgramEditor::setAxisBackend(AxisReader reader, AxisCommander commander)
{
    m_axisReader = std::move(reader);
    m_axisCommander = std::move(commander);
    refreshAxisPanel();
}

void GraphicalProgramEditor::setCameraBackend(CameraReader reader, CameraCommander commander)
{
    m_cameraReader = std::move(reader);
    m_cameraCommander = std::move(commander);
    refreshCameraPanel();
}

bool GraphicalProgramEditor::saveRecipeFile(const QString& filePath, QString& error)
{
    if (!writeProject(filePath, error)) return false;
    m_projectFilePath = QFileInfo(filePath).absoluteFilePath();
    m_projectDirty = false;
    return true;
}

bool GraphicalProgramEditor::loadRecipeFile(const QString& filePath, QString& error)
{
    if (m_trialRunning || m_ownedAxis > 0 || m_ownedCamera >= 0) {
        error = QStringLiteral("请等待试测结束，并停止当前轴运动和相机采集。");
        return false;
    }
    return readProject(filePath, error);
}

void GraphicalProgramEditor::setLightCurtainBackend(LightCurtainReader reader)
{
    m_lightCurtainReader = std::move(reader);
    refreshDevicePositionPanel();
}

void GraphicalProgramEditor::refreshCameraPanel()
{
    if (!m_cameraSelector || !isVisible()) return;
    const int camera = m_ownedCamera >= 0 ? m_ownedCamera : m_cameraSelector->currentData().toInt();
    CameraSnapshot snapshot;
    snapshot.message = QStringLiteral("相机接口未连接");
    if (m_cameraReader) snapshot = m_cameraReader(camera);
    if (m_ownedCamera >= 0 && (!snapshot.connected || !snapshot.available || m_trialRunning)) {
        stopOwnedCamera();
        return;
    }
    m_cameraState->setText(snapshot.message + (snapshot.hasFrame
        ? QStringLiteral("\n最后一帧：%1 × %2，曝光 %3 μs")
            .arg(snapshot.frameSize.width()).arg(snapshot.frameSize.height()).arg(snapshot.exposure)
        : QString()));
    if (!m_cameraExposureEdited && !snapshot.capturing && snapshot.exposure >= 0) {
        QSignalBlocker blocker(m_cameraExposure);
        m_cameraExposure->setValue(snapshot.exposure);
    }
    const bool idle = snapshot.available && snapshot.connected && !snapshot.capturing
        && m_ownedAxis < 0 && !m_trialRunning;
    m_cameraSelector->setEnabled(!snapshot.capturing && m_ownedCamera < 0);
    m_cameraExposure->setEnabled(idle);
    m_cameraStart->setEnabled(idle);
    m_cameraStop->setEnabled(snapshot.connected && snapshot.capturing);
    m_cameraLoad->setEnabled(idle && snapshot.hasFrame);
    refreshDevicePositionPanel();
}

bool GraphicalProgramEditor::stopOwnedCamera()
{
    if (m_ownedCamera < 0) return true;
    if (!m_cameraCommander) return false;
    const CameraCommandResult result = m_cameraCommander(
        m_ownedCamera, CameraCommand::StopCapture, m_cameraExposure ? m_cameraExposure->value() : 0);
    if (!result.error.isEmpty()) {
        if (m_cameraState) m_cameraState->setText(result.error);
        return false;
    }
    m_ownedCamera = -1;
    if (m_cameraState) m_cameraState->setText(QStringLiteral("采集已停止；可载入最后一帧。"));
    refreshCameraPanel();
    return true;
}

void GraphicalProgramEditor::executeCameraCommand(CameraCommand command)
{
    if (!m_cameraCommander || !m_cameraSelector || m_trialRunning) return;
    if (command == CameraCommand::StopCapture) {
        if (m_ownedCamera >= 0) { stopOwnedCamera(); return; }
        const int camera = m_cameraSelector->currentData().toInt();
        const CameraCommandResult result = m_cameraCommander(
            camera, command, m_cameraExposure->value());
        m_cameraState->setText(result.error.isEmpty()
            ? QStringLiteral("采集已停止；可载入最后一帧。") : result.error);
        refreshCameraPanel();
        return;
    }
    if (m_ownedAxis > 0) {
        m_cameraState->setText(QStringLiteral("请先停止当前轴运动。")); return;
    }
    const int camera = m_cameraSelector->currentData().toInt();
    if (command == CameraCommand::StartCapture) {
        const CameraCommandResult result = m_cameraCommander(
            camera, command, m_cameraExposure->value());
        if (!result.error.isEmpty()) { m_cameraState->setText(result.error); return; }
        m_ownedCamera = camera;
        m_cameraExposureEdited = false;
        m_cameraState->setText(QStringLiteral("正在连续采集；等待图像稳定后点击停止采集。"));
        refreshCameraPanel();
        return;
    }

    const int selectedRow = m_stepTable ? m_stepTable->currentRow() : -1;
    const bool appendCrossLengthFrame = camera == 0 && selectedRow >= 0
        && selectedRow < m_records.size()
        && m_records[selectedRow].type == QStringLiteral("长度")
        && m_records[selectedRow].crossFrameLength;
    if (!appendCrossLengthFrame && m_projectDirty && QMessageBox::question(this, QStringLiteral("载入相机图像"),
        QStringLiteral("载入新图像将清空当前图形和测量记录。是否继续？")) != QMessageBox::Yes) return;
    const CameraCommandResult result = m_cameraCommander(camera, command, m_cameraExposure->value());
    if (!result.error.isEmpty() || result.image.isNull()) {
        m_cameraState->setText(result.error.isEmpty() ? QStringLiteral("相机没有可载入的有效图像。") : result.error);
        return;
    }
    QString cacheError;
    const QString filePath = cameraCaptureCachePath(camera, cacheError);
    if (filePath.isEmpty() || !writePngAtomically(result.image, filePath, cacheError)) {
        QMessageBox::warning(this, QStringLiteral("载入采集图像失败"), cacheError);
        return;
    }
    QString hashError;
    const QByteArray imageSha256 = projectFileSha256(filePath, hashError);
    if (imageSha256.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("载入采集图像失败"), hashError); return;
    }
    cancelRelink();
    if (appendCrossLengthFrame) {
        storeCurrentFrame();
        ProjectFrame frame;
        frame.id = m_nextFrameId++;
        frame.filePath = QFileInfo(filePath).absoluteFilePath();
        frame.fileSha256 = QString::fromLatin1(imageSha256);
        frame.cameraIndex = camera;
        frame.exposure = result.exposure;
        frame.image = result.image;
        m_frames.append(frame);
        m_projectDirty = true;
        QString frameError;
        if (!activateFrame(frame.id, frameError)) {
            m_frames.removeLast();
            --m_nextFrameId;
            QMessageBox::warning(this, QStringLiteral("载入端点图像失败"), frameError);
            return;
        }
        m_stepTable->setCurrentCell(selectedRow, 0);
        m_cameraState->setText(QStringLiteral("已将相机0最后一帧加入跨图长度记录。"));
        statusBar()->showMessage(QStringLiteral("端点图像已载入且原记录已保留；请绘制矩形ROI并确认起点或终点。"), 7000);
        return;
    }
    m_loadingProject = true;
    m_canvas->setImage(result.image);
    setCanvasSourceBadge(m_canvas, QStringLiteral("相机快照 · 非实时"));
    m_loadingProject = false;
    if (QLabel* hint = m_canvas->findChild<QLabel*>(QStringLiteral("canvasEmptyHint"))) hint->hide();
    m_records.clear();
    m_nextRecordSequence = 1;
    m_imageFilePath = QFileInfo(filePath).absoluteFilePath();
    m_imageFileSha256 = QString::fromLatin1(imageSha256);
    m_imageCameraIndex = camera;
    m_imageExposure = result.exposure;
    m_frames.clear();
    ProjectFrame frame;
    frame.id = 1;
    frame.filePath = m_imageFilePath;
    frame.fileSha256 = m_imageFileSha256;
    frame.cameraIndex = m_imageCameraIndex;
    frame.exposure = m_imageExposure;
    frame.image = result.image;
    m_frames.append(frame);
    m_currentFrameId = 1;
    m_nextFrameId = 2;
    m_recipeProgramNumber->setValue(0);
    m_recipePartNumber->clear();
    m_recipePartName->clear();
    m_recipeProcessNumber->clear();
    m_recipeNote->clear();
    m_recipeValidationResult->setText(QStringLiteral("填写配方信息后检查记录、ROI、标定和设备点位。"));
    refreshFrameSelector();
    m_projectFilePath.clear();
    m_projectDirty = true;
    refreshFeatureList();
    refreshMeasurementRecords();
    m_canvas->fitImageInView();
    m_cameraState->setText(QStringLiteral("已载入相机%1最后一帧。").arg(camera));
    statusBar()->showMessage(QStringLiteral("相机图像已载入；请重新创建ROI和测量记录。"), 6000);
}

void GraphicalProgramEditor::refreshDevicePositionPanel()
{
    if (!m_devicePositionState || !m_stepTable) return;
    if (m_lightCurtainState) {
        LightCurtainSnapshot sensor;
        sensor.message = QStringLiteral("光幕接口未连接");
        if (m_lightCurtainReader) sensor = m_lightCurtainReader();
        m_lightCurtainState->setText(sensor.message + (sensor.hasSample
            ? QStringLiteral("\nOUT1原始值：%1 mm；补偿直径：%2 mm")
                .arg(sensor.rawOut1, 0, 'f', 4).arg(sensor.compensatedDiameter, 0, 'f', 4)
            : QString()));
    }
    const int row = m_stepTable->currentRow();
    const bool validRow = row >= 0 && row < m_records.size();
    const bool crossLength = validRow && m_records[row].type == QStringLiteral("长度")
        && m_records[row].crossFrameLength;
    m_recordDevicePosition->setEnabled(validRow && !crossLength && m_axisBackendAvailable
        && !m_trialRunning && m_ownedAxis < 0 && m_ownedCamera < 0);
    m_clearDevicePosition->setEnabled(validRow && !crossLength && m_records[row].devicePosition.collected);
    if (!validRow) {
        m_devicePositionState->setText(QStringLiteral("请先选择一条测量记录。")); return;
    }
    if (crossLength) {
        const auto endpointText = [](const QString& name,
            const MeasurementRecord::DevicePosition& position) {
            if (!position.collected) return name + QStringLiteral("：未采集");
            for (const auto& axis : position.axes) if (axis.axis == 5)
                return QStringLiteral("%1：轴5规划 %2 pulse；编码器 %3 pulse；相机%4/%5 μs")
                    .arg(name).arg(axis.planned, 0, 'f', 1).arg(axis.encoder, 0, 'f', 1)
                    .arg(position.cameraIndex).arg(position.exposure);
            return name + QStringLiteral("：数据无效");
        };
        QStringList endpointLines;
        endpointLines << endpointText(QStringLiteral("起点"), m_records[row].lengthStartPosition)
            << endpointText(QStringLiteral("终点"), m_records[row].lengthEndPosition)
            << QStringLiteral("请在测量配置页通过起点/终点确认按钮重采。");
        m_devicePositionState->setText(endpointLines.join(QLatin1Char('\n')));
        return;
    }
    const auto& position = m_records[row].devicePosition;
    if (!position.collected) {
        const MeasurementRecord& record = m_records[row];
        const bool axialScan = record.type == QStringLiteral("圆柱度")
            || record.type == QStringLiteral("跳动");
        QString state = axialScan
            ? QStringLiteral("记录 %1：轴5中间点位未采集。").arg(record.sequence)
            : QStringLiteral("记录 %1：设备点位未采集。").arg(record.sequence);
        if (axialScan)
            state += QStringLiteral("\n已配置下侧偏移 %1 pulse；上侧偏移 %2 pulse。")
                .arg(record.lowerAxialOffsetPulse).arg(record.upperAxialOffsetPulse);
        m_devicePositionState->setText(state);
        return;
    }
    QStringList lines;
    lines << QStringLiteral("记录 %1：硬件点位已采集").arg(m_records[row].sequence);
    if (!position.capturedAtUtc.isEmpty())
        lines << QStringLiteral("采集时间（UTC）：%1").arg(position.capturedAtUtc);
    for (const auto& axis : position.axes)
        lines << QStringLiteral("轴%1：规划 %2 pulse；编码器 %3 pulse")
            .arg(axis.axis).arg(axis.planned, 0, 'f', 1).arg(axis.encoder, 0, 'f', 1);
    const MeasurementRecord& record = m_records[row];
    if (record.type == QStringLiteral("圆柱度") || record.type == QStringLiteral("跳动")) {
        for (const auto& axis : position.axes) {
            if (axis.axis != 5 || !std::isfinite(axis.encoder)) continue;
            lines << QStringLiteral("轴5三截面：下侧 %1 pulse；中间 %2 pulse；上侧 %3 pulse")
                .arg(axis.encoder - record.lowerAxialOffsetPulse, 0, 'f', 1)
                .arg(axis.encoder, 0, 'f', 1)
                .arg(axis.encoder + record.upperAxialOffsetPulse, 0, 'f', 1);
            break;
        }
        if (record.type == QStringLiteral("跳动")) {
            lines << QStringLiteral("基准1：%1").arg(record.roundoutReference1.isEmpty()
                ? QStringLiteral("未填写") : record.roundoutReference1);
            lines << QStringLiteral("基准2：%1").arg(record.roundoutReference2.isEmpty()
                ? QStringLiteral("未填写") : record.roundoutReference2);
        }
    }
    if (position.cameraIndex >= 0)
        lines << QStringLiteral("相机%1：曝光 %2 μs").arg(position.cameraIndex).arg(position.exposure);
    if (position.hasLightCurtainSample)
        lines << QStringLiteral("光幕OUT1：%1 mm；补偿直径：%2 mm")
            .arg(position.lightCurtainRawOut1, 0, 'f', 4)
            .arg(position.lightCurtainDiameter, 0, 'f', 4);
    m_devicePositionState->setText(lines.join(QLatin1Char('\n')));
}

void GraphicalProgramEditor::recordSelectedDevicePosition()
{
    const int row = m_stepTable ? m_stepTable->currentRow() : -1;
    if (row < 0 || row >= m_records.size()) return;
    MeasurementRecord::DevicePosition next;
    QString error;
    if (!collectCurrentDevicePosition(m_records[row].type, next, error)) {
        QMessageBox::warning(this, QStringLiteral("点位采集失败"), error);
        return;
    }
    m_records[row].devicePosition = next;
    m_projectDirty = true;
    refreshMeasurementRecords();
    m_stepTable->setCurrentCell(row, 0);
    refreshDevicePositionPanel();
    statusBar()->showMessage(QStringLiteral("记录 %1 的设备点位已采集；保存配方后持久化。")
        .arg(m_records[row].sequence), 5000);
}

bool GraphicalProgramEditor::collectCurrentDevicePosition(const QString& type,
    MeasurementRecord::DevicePosition& position, QString& error) const
{
    error.clear();
    if (!m_axisReader) {
        error = QStringLiteral("控制卡点位接口未连接；原点位保持不变。");
        return false;
    }
    if (m_ownedAxis > 0 || m_trialRunning) {
        error = QStringLiteral("轴仍在运动或算法仍在执行；原点位保持不变。");
        return false;
    }
    const QVector<int> axes = deviceAxesForMeasurement(type);
    const int camera = deviceCameraForMeasurement(type);

    MeasurementRecord::DevicePosition next;
    next.collected = true;
    next.source = QStringLiteral("hardware");
    next.capturedAtUtc = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    for (int axis : axes) {
        const AxisSnapshot snapshot = m_axisReader(axis);
        if (!snapshot.available || !snapshot.valid || (snapshot.status & 0x400)) {
            error = QStringLiteral("轴%1状态不可用或仍在运动；原点位保持不变。\n%2")
                .arg(axis).arg(snapshot.message);
            return false;
        }
        MeasurementRecord::AxisPosition value;
        value.axis = axis; value.planned = snapshot.planned; value.encoder = snapshot.encoder;
        next.axes.append(value);
    }
    if (camera >= 0) {
        CameraSnapshot snapshot;
        if (m_cameraReader) snapshot = m_cameraReader(camera);
        if (!snapshot.available || !snapshot.connected || snapshot.capturing
            || !snapshot.hasFrame || snapshot.exposure < 0) {
            error = QStringLiteral("相机%1没有已停止采集的有效帧或实际曝光；请完成一次采集后重试，原点位保持不变。\n%2")
                .arg(camera).arg(snapshot.message);
            return false;
        }
        next.cameraIndex = camera;
        next.exposure = snapshot.exposure;
    }
    if (type == QStringLiteral("直径")) {
        LightCurtainSnapshot snapshot;
        if (m_lightCurtainReader) snapshot = m_lightCurtainReader();
        if (!snapshot.connected || !snapshot.available || !snapshot.hasSample
            || !std::isfinite(snapshot.rawOut1) || !std::isfinite(snapshot.compensatedDiameter)) {
            error = QStringLiteral("光幕OUT1没有有效样本；原点位保持不变。\n%1")
                .arg(snapshot.message);
            return false;
        }
        next.hasLightCurtainSample = true;
        next.lightCurtainRawOut1 = snapshot.rawOut1;
        next.lightCurtainDiameter = snapshot.compensatedDiameter;
        next.lightCurtainSampledAtUtc = QDateTime::fromMSecsSinceEpoch(
            snapshot.sampledAtMs, Qt::UTC).toString(Qt::ISODateWithMs);
    }
    position = next;
    return true;
}

void GraphicalProgramEditor::clearSelectedDevicePosition()
{
    const int row = m_stepTable ? m_stepTable->currentRow() : -1;
    if (row < 0 || row >= m_records.size()) return;
    m_records[row].devicePosition = MeasurementRecord::DevicePosition();
    m_projectDirty = true;
    refreshMeasurementRecords();
    m_stepTable->setCurrentCell(row, 0);
    refreshDevicePositionPanel();
}

QWidget* GraphicalProgramEditor::buildAxisPanel()
{
    QGroupBox* panel = new QGroupBox(QStringLiteral("手动轴控制"));
    panel->setObjectName(QStringLiteral("graphicalAxisControlCard"));
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
    m_axisSelector->setObjectName(QStringLiteral("axisSelector"));
    const int axes[] = { 2, 5, 6, 7 };
    const QStringList names = { QStringLiteral("测孔轴"), QStringLiteral("光幕轴"),
        QStringLiteral("上顶尖轴"), QStringLiteral("转台轴") };
    for (int i = 0; i < 4; ++i)
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
    m_axisBackendAvailable = snapshot.available && snapshot.valid;
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
    if (event->type() == QEvent::WindowDeactivate || event->type() == QEvent::Hide) {
        stopOwnedAxis();
        stopOwnedCamera();
    }
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
    m_axisStop->setObjectName(QStringLiteral("graphicalAxisStopButton"));
    m_axisEmergency->setObjectName(QStringLiteral("graphicalAxisEmergencyButton"));
    m_axisEmergency->setStyleSheet(QString());
    m_axisStop->setEnabled(false);
    m_axisEmergency->setEnabled(false);
    axisSafetyBar->addWidget(m_axisStop);
    axisSafetyBar->addWidget(m_axisEmergency);
    connect(m_axisStop, &QPushButton::clicked, this, [this]() { executeAxisCommand(AxisCommand::Stop); });
    connect(m_axisEmergency, &QPushButton::clicked, this, [this]() { executeAxisCommand(AxisCommand::EmergencyStop); });

    QAction* openImageAction = toolBar->addAction(QStringLiteral("打开图像"));
    QAction* openProjectAction = toolBar->addAction(QStringLiteral("打开配方"));
    QAction* saveProjectAction = toolBar->addAction(QStringLiteral("保存配方"));
    saveProjectAction->setShortcut(QKeySequence::Save);
    QAction* cameraAction = toolBar->addAction(QStringLiteral("相机图像"));
    m_frameSelector = new QComboBox(toolBar);
    m_frameSelector->setMinimumWidth(120);
    m_frameSelector->setToolTip(QStringLiteral("切换记录使用的端点图像；当前图像ROI会先自动保存"));
    m_frameSelector->setVisible(false);
    toolBar->addWidget(m_frameSelector);
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
        { cameraAction, "camera", nullptr, "打开设备点位页，从相机采集并载入图像" },
        { selectAction, "select", "V", "选择 (V)：选中/移动/调整图形" },
        { pointAction, "point", "P", "点 (P)：单击标注特征点" },
        { lineAction, "line", "L", "直线 (L)：拖动画直线" },
        { rectangleAction, "rect", "R", "矩形 (R)：拖动画矩形，下拉可选绘制模式" },
        { circleAction, "circle", "C", "圆 (C)：拖动画圆，下拉可选绘制模式" },
        { arcAction, "arc", "A", "圆弧 (A)：依次点击起点、弧上点、终点" },
        { fitAction, "fit", "F", "适合窗口 (F)：图像缩放到充满画布" },
        { undoAction, "undo", nullptr, "撤销（尚未接入）" },
        { redoAction, "redo", nullptr, "重做（尚未接入）" },
        { deleteAction, "delete", nullptr, "删除选中图形" },
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
        undoAction, redoAction//撤销、重做是占位按钮
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

    QMenu* offlineMenu = menuBar()->addMenu(QStringLiteral("离线调试"));
    QAction* addFrameAction = offlineMenu->addAction(QStringLiteral("导入端点图…"));
    QAction* removeFrameAction = offlineMenu->addAction(QStringLiteral("移除当前端点图"));
    addFrameAction->setStatusTip(QStringLiteral("使用本地图像模拟另一个相机端点"));
    removeFrameAction->setStatusTip(QStringLiteral("只从工程移除当前端点图，不删除磁盘原文件"));

    QWidget* centralWidget = new QWidget(this);
    centralWidget->setObjectName(QStringLiteral("graphicalProgramEditorPage"));
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
    m_canvas->setBackgroundBrush(QColor(QStringLiteral("#202938")));
    //P1-7 空态提示：未打开图像时居中显示，打开图像后隐藏
    QLabel* emptyHint = new QLabel(
        QStringLiteral("尚未打开图像\n\n点击工具栏「打开图像」选择本地图片开始编辑"), m_canvas);
    emptyHint->setObjectName(QStringLiteral("canvasEmptyHint"));
    emptyHint->setAlignment(Qt::AlignCenter);
    emptyHint->setAttribute(Qt::WA_TransparentForMouseEvents);
    emptyHint->setStyleSheet(QStringLiteral("color:#9CA3AF; font-size:14px; background:transparent;"));
    QLabel* sourceBadge = new QLabel(m_canvas);
    sourceBadge->setObjectName(QStringLiteral("canvasSourceBadge"));
    sourceBadge->setAttribute(Qt::WA_TransparentForMouseEvents);
    sourceBadge->setStyleSheet(QStringLiteral(
        "QLabel { color:#FFFFFF; background:rgba(180,70,0,210); border-radius:4px; padding:5px 9px; font-weight:600; }"));
    sourceBadge->hide();
    QGridLayout* hintLayout = new QGridLayout(m_canvas);
    hintLayout->addWidget(emptyHint, 0, 0, Qt::AlignCenter);
    hintLayout->addWidget(sourceBadge, 0, 0, Qt::AlignTop | Qt::AlignRight);
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
    const int featureTab = propertyTabs->addTab(featurePropertyPage, QStringLiteral("特征属性"));
    propertyTabs->setTabToolTip(featureTab, QStringLiteral("特征属性"));

    QWidget* measurementPage = new QWidget(propertyTabs);//测量配置页
    QVBoxLayout* measurementLayout = new QVBoxLayout(measurementPage);
    QFormLayout* measurementForm = new QFormLayout;
    measurementForm->setRowWrapPolicy(QFormLayout::WrapAllRows);
    measurementLayout->addLayout(measurementForm);
    m_measurementType = new QComboBox(measurementPage);
    m_measurementType->addItems(QStringList() << QStringLiteral("直径")
        << QStringLiteral("孔径") << QStringLiteral("圆柱度") << QStringLiteral("跳动")
        << QStringLiteral("长度") << QStringLiteral("角度") << QStringLiteral("圆弧半径"));
    m_featureNumber = new QLineEdit(measurementPage);
    m_featureNumber->setObjectName(QStringLiteral("measurementFeatureNumber"));
    m_measurementType->setObjectName(QStringLiteral("measurementType"));
    m_featureNumber->setMaxLength(64);
    m_featureNumber->setPlaceholderText(QStringLiteral("自动编号，可修改"));
    connect(m_featureNumber, &QLineEdit::textEdited, this,
        [this]() { m_featureNumberEditedSinceLoad = true; });
    measurementForm->addRow(QStringLiteral("测量类型："), m_measurementType);
    m_angleInputMode = new QComboBox(measurementPage);
    m_angleInputMode->setObjectName(QStringLiteral("angleInputMode"));
    m_angleInputMode->addItems({QStringLiteral("双ROI：分别选取两条直线"), QStringLiteral("单ROI：框选相邻角 / 倒角")});
    measurementForm->addRow(QStringLiteral("角度选取："), m_angleInputMode);
    m_cornerCandidate = new QComboBox(measurementPage);
    m_cornerCandidate->setObjectName(QStringLiteral("cornerCandidate"));
    measurementForm->addRow(QStringLiteral("候选边对："), m_cornerCandidate);
    connect(m_cornerCandidate, QOverload<int>::of(&QComboBox::activated), this, &GraphicalProgramEditor::chooseCornerCandidate);
    connect(m_angleInputMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() { refreshAngleControls(); });
    m_angleResultMode = new QComboBox(measurementPage);
    m_angleResultMode->addItems(QStringList() << QStringLiteral("较小夹角（0°–90°）")
        << QStringLiteral("较大补角（90°–180°）"));
    measurementForm->addRow(QStringLiteral("角度结果："), m_angleResultMode);
    m_holeUniformCount = new QSpinBox(measurementPage);
    m_holeUniformCount->setObjectName(QStringLiteral("holeUniformCount"));
    m_holeUniformCount->setRange(0, 999);
    m_holeUniformCount->setSpecialValueText(QStringLiteral("未设置"));
    m_holeUniformCount->setToolTip(QStringLiteral("旧孔径表单中的“均布个数”；与H0/H1拍照位置无关。"));
    m_holeUniformCount->setEnabled(false);
    m_holeCalibration = new QDoubleSpinBox(measurementPage);
    m_holeCalibration->setObjectName(QStringLiteral("holeCalibration"));
    m_holeCalibration->setRange(0.000001, 1.0);
    m_holeCalibration->setDecimals(8);
    m_holeCalibration->setSingleStep(0.00000001);
    m_holeCalibration->setValue(0.00691842);
    m_holeCalibration->setSuffix(QStringLiteral(" mm/px"));
    m_holeCalibration->setToolTip(QStringLiteral("原软件测孔相机CalikKong标定系数；现场重新标定后按实际值修改。"));
    m_holeCalibration->setEnabled(false);
    m_lengthCalibration = new QDoubleSpinBox(measurementPage);
    m_lengthCalibration->setObjectName(QStringLiteral("lengthCalibration"));
    m_lengthCalibration->setRange(0.000001, 1.0);
    m_lengthCalibration->setDecimals(8);
    m_lengthCalibration->setSingleStep(0.00000001);
    m_lengthCalibration->setValue(0.01218603);
    m_lengthCalibration->setSuffix(QStringLiteral(" mm/px"));
    m_lengthCalibration->setToolTip(QStringLiteral("原软件远心相机Calik标定系数；现场重新标定后按实际值修改。"));
    m_lengthCalibration->setEnabled(false);
    m_lengthMode = new QComboBox(measurementPage);
    m_lengthMode->addItem(QStringLiteral("同图长度（单ROI）"), false);
    m_lengthMode->addItem(QStringLiteral("跨图长度（起点/终点）"), true);
    m_lengthMode->setEnabled(false);
    connect(m_lengthMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
        [this]() { refreshAngleControls(); });
    m_lowerAxialOffset = new QSpinBox(measurementPage);
    m_lowerAxialOffset->setObjectName(QStringLiteral("lowerAxialOffsetPulse"));
    m_lowerAxialOffset->setRange(0, 100000000);
    m_lowerAxialOffset->setSpecialValueText(QStringLiteral("未设置"));
    m_lowerAxialOffset->setSuffix(QStringLiteral(" pulse"));
    m_lowerAxialOffset->setToolTip(QStringLiteral("以当前轴5点位为中间截面，向下侧移动的绝对脉冲差。"));
    m_upperAxialOffset = new QSpinBox(measurementPage);
    m_upperAxialOffset->setObjectName(QStringLiteral("upperAxialOffsetPulse"));
    m_upperAxialOffset->setRange(0, 100000000);
    m_upperAxialOffset->setSpecialValueText(QStringLiteral("未设置"));
    m_upperAxialOffset->setSuffix(QStringLiteral(" pulse"));
    m_upperAxialOffset->setToolTip(QStringLiteral("以当前轴5点位为中间截面，向上侧移动的绝对脉冲差。"));
    m_roundoutReference1 = new QLineEdit(measurementPage);
    m_roundoutReference1->setObjectName(QStringLiteral("roundoutReference1"));
    m_roundoutReference1->setMaxLength(128);
    m_roundoutReference1->setPlaceholderText(QStringLiteral("可选：基准1"));
    m_roundoutReference2 = new QLineEdit(measurementPage);
    m_roundoutReference2->setObjectName(QStringLiteral("roundoutReference2"));
    m_roundoutReference2->setMaxLength(128);
    m_roundoutReference2->setPlaceholderText(QStringLiteral("可选：基准2"));
    connect(m_measurementType, &QComboBox::currentTextChanged, this, [this](const QString& type) {
        m_holeUniformCount->setEnabled(type == QStringLiteral("孔径"));
        m_holeCalibration->setEnabled(type == QStringLiteral("孔径"));
        m_lengthCalibration->setEnabled(type == QStringLiteral("长度"));
        m_lengthMode->setEnabled(type == QStringLiteral("长度"));
        const bool axialScan = type == QStringLiteral("圆柱度") || type == QStringLiteral("跳动");
        m_lowerAxialOffset->setEnabled(axialScan);
        m_upperAxialOffset->setEnabled(axialScan);
        m_roundoutReference1->setEnabled(type == QStringLiteral("跳动"));
        m_roundoutReference2->setEnabled(type == QStringLiteral("跳动"));
        if (!m_featureNumberEditedSinceLoad)
            m_featureNumber->setText(suggestedFeatureNumber(type));
        refreshAngleControls();
    });
    measurementForm->addRow(QStringLiteral("特征号："), m_featureNumber);
    measurementForm->addRow(QStringLiteral("孔均布个数："), m_holeUniformCount);
    measurementForm->addRow(QStringLiteral("测孔标定："), m_holeCalibration);
    measurementForm->addRow(QStringLiteral("远心标定："), m_lengthCalibration);
    measurementForm->addRow(QStringLiteral("长度模式："), m_lengthMode);
    measurementForm->addRow(QStringLiteral("下侧偏移："), m_lowerAxialOffset);
    measurementForm->addRow(QStringLiteral("上侧偏移："), m_upperAxialOffset);
    measurementForm->addRow(QStringLiteral("圆跳动基准1："), m_roundoutReference1);
    measurementForm->addRow(QStringLiteral("圆跳动基准2："), m_roundoutReference2);
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
    const auto setFormFieldVisible = [measurementForm](QWidget* field, bool visible) {
        field->setVisible(visible);
        if (QWidget* label = measurementForm->labelForField(field)) label->setVisible(visible);
    };
    const auto refreshMeasurementFieldVisibility = [this, setFormFieldVisible]() {
        const QString type = m_measurementType->currentText();
        const bool angle = type == QStringLiteral("角度");
        const bool hole = type == QStringLiteral("孔径");
        const bool length = type == QStringLiteral("长度");
        const bool axialScan = type == QStringLiteral("圆柱度") || type == QStringLiteral("跳动");
        const bool roundout = type == QStringLiteral("跳动");
        setFormFieldVisible(m_angleInputMode, angle);
        setFormFieldVisible(m_cornerCandidate, angle);
        setFormFieldVisible(m_angleResultMode, angle);
        setFormFieldVisible(m_holeUniformCount, hole);
        setFormFieldVisible(m_holeCalibration, hole);
        setFormFieldVisible(m_lengthCalibration, length);
        setFormFieldVisible(m_lengthMode, length);
        setFormFieldVisible(m_lowerAxialOffset, axialScan);
        setFormFieldVisible(m_upperAxialOffset, axialScan);
        setFormFieldVisible(m_roundoutReference1, roundout);
        setFormFieldVisible(m_roundoutReference2, roundout);
        const bool tolerance = m_hasTolerance->isChecked();
        setFormFieldVisible(m_nominal, tolerance);
        setFormFieldVisible(m_lowerDeviation, tolerance);
        setFormFieldVisible(m_upperDeviation, tolerance);
    };
    connect(m_measurementType, &QComboBox::currentTextChanged, this,
        [refreshMeasurementFieldVisibility](const QString&) { refreshMeasurementFieldVisibility(); });
    connect(m_hasTolerance, &QCheckBox::toggled, this,
        [this, refreshMeasurementFieldVisibility](bool enabled) {
            for (QDoubleSpinBox* input : {m_nominal, m_lowerDeviation, m_upperDeviation})
                input->setEnabled(enabled);
            refreshMeasurementFieldVisibility();
        });
    m_featureNumber->setText(suggestedFeatureNumber(m_measurementType->currentText()));
    refreshMeasurementFieldVisibility();
    QPushButton* addRecord = new QPushButton(QStringLiteral("新增测量记录"), measurementPage);
    QPushButton* updateRecord = new QPushButton(QStringLiteral("更新选中记录"), measurementPage);
    QPushButton* deleteRecord = new QPushButton(QStringLiteral("删除选中记录（保留图形）"), measurementPage);
    measurementLayout->addWidget(addRecord);
    measurementLayout->addWidget(updateRecord);
    QPushButton* relinkRecord = new QPushButton(QStringLiteral("重新关联图形"), measurementPage);
    QPushButton* selectAngleRoi1 = new QPushButton(QStringLiteral("角度：选择/重选 ROI 1（直线1）"), measurementPage);
    QPushButton* selectAngleRoi2 = new QPushButton(QStringLiteral("角度：选择/重选 ROI 2（直线2）"), measurementPage);
    m_selectAngleRoi1 = selectAngleRoi1;
    m_selectAngleRoi2 = selectAngleRoi2;
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
    const auto armLineRoi = [this, selectAction](int slot) {
        const int row = m_stepTable->currentRow();
        if (slot == 2 && row >= 0 && row < m_records.size()
            && (m_records[row].singleRoiAngle
                || (m_records[row].type == QStringLiteral("长度")
                    && !m_records[row].crossFrameLength))) return;
        const bool supported = row >= 0 && row < m_records.size()
            && (m_records[row].type == QStringLiteral("角度")
                || m_records[row].type == QStringLiteral("长度"));
        if (!supported) {
            QMessageBox::warning(this, QStringLiteral("未开始关联"), QStringLiteral("请先选中一条已保存的角度或长度记录。"));
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
        statusBar()->showMessage(m_records[row].type == QStringLiteral("长度")
            ? (m_records[row].crossFrameLength
                ? QStringLiteral("正在确认跨图长度%1端：请点击当前帧中的矩形ROI；相机帧将同时采集轴5点位，本地图像保留为未采集。")
                    .arg(slot == 1 ? QStringLiteral("起点") : QStringLiteral("终点"))
                : QStringLiteral("正在为长度记录 %1 选择单个模板ROI：请点击一个框住完整长度特征和两条目标边的矩形；Esc取消。")
                    .arg(sequence))
            : QStringLiteral("正在为角度记录 %1 选择 ROI %2：请点击一个矩形或圆形ROI；Esc取消。")
                .arg(sequence).arg(slot));
    };
    connect(selectAngleRoi1, &QPushButton::clicked, this, [armLineRoi]() { armLineRoi(1); });
    connect(selectAngleRoi2, &QPushButton::clicked, this, [armLineRoi]() { armLineRoi(2); });
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
        "圆弧半径使用一个ROI；角度支持双ROI或单ROI相邻角。\n"
        "单图长度只使用一个矩形ROI框住完整特征；首次试测建立模板，随后自动定位并拟合两条近似水平边。\n"
        "跨图长度分别在两帧确认起点和终点矩形ROI；相机帧确认时自动采集轴5点位，本地图像保持点位未采集。\n"
        "孔径使用一个圆形或矩形ROI框住目标孔，程序自动定位上下测量区并按原测孔算法输出mm。\n"
        "圆柱度和圆跳动以采集的轴5点位为中间截面，通过上下偏移生成三处测量位置；实际结果需要光幕与转台完成整周采样。\n"
        "长度类公差按mm、角度按°录入（配置约定，非标定结果）。\n"
        "可先选图形进行关联，也可建立待关联记录。\n"
        "工程可保存为JSON；重新载入后历史试测结果失效。程序导出尚未接入。"), measurementPage);
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
        m_projectDirty = true;
        refreshMeasurementRecords();
        m_canvas->setDetectionOverlay(QPainterPath(), QPainterPath());
    });
    measurementLayout->addStretch();
    auto* measurementScroll = new QScrollArea(propertyTabs);
    measurementScroll->setWidgetResizable(true);
    measurementScroll->setWidget(measurementPage);
    const int measurementTab = propertyTabs->addTab(measurementScroll, QStringLiteral("测量配置"));
    propertyTabs->setTabToolTip(measurementTab, QStringLiteral("测量配置"));

    QScrollArea* detectionScroll = new QScrollArea(propertyTabs);
    detectionScroll->setWidgetResizable(true);
    QWidget* detectionPage = new QWidget(detectionScroll);
    QVBoxLayout* detectionLayout = new QVBoxLayout(detectionPage);
    QLabel* detectionHint = new QLabel(QStringLiteral("圆弧半径 / 双ROI角度 / 单ROI倒角 / 单ROI模板长度。\n试测使用已提交值，修改后请应用。单ROI保留独立直线段，不使用合并距离；角点间隙判断两条边是否相邻。"), detectionPage);
    detectionHint->setWordWrap(true);
    detectionLayout->addWidget(detectionHint);
    QFormLayout* detectionForm = new QFormLayout;
    detectionForm->setRowWrapPolicy(QFormLayout::WrapAllRows);
    detectionLayout->addLayout(detectionForm);
    auto parameterInput = [detectionPage, detectionForm](const QString& label, const char* name, double low, double high) {
        auto* input = new QDoubleSpinBox(detectionPage);
        input->setObjectName(QString::fromLatin1(name));
        input->setDecimals(2);
        input->setRange(low, high);
        input->setKeyboardTracking(false);
        detectionForm->addRow(label, input);
        return input;
    };
    m_detectionSmoothing = parameterInput(QStringLiteral("Canny平滑参数"), "detectionSmoothing", 0.1, 20);
    m_detectionLow = parameterInput(QStringLiteral("边缘低阈值"), "detectionLow", 0, 65535);
    m_detectionHigh = parameterInput(QStringLiteral("边缘高阈值"), "detectionHigh", 0, 65535);
    m_detectionMinLength = parameterInput(QStringLiteral("最短轮廓 px"), "detectionMinLength", 1, 1000000);
    m_detectionMaxLength = parameterInput(QStringLiteral("最长轮廓 px"), "detectionMaxLength", 0, 1000000);
    m_detectionMaxLength->setSpecialValueText(QStringLiteral("自动：图宽 / 2"));
    m_detectionMergeDistance = parameterInput(QStringLiteral("合并距离 px"), "detectionMergeDistance", 0, 1000000);
    m_cornerDeviation = parameterInput(QStringLiteral("单ROI：90%点线偏差 px"), "cornerDeviation", 0.1, 20);
    m_cornerGap = parameterInput(QStringLiteral("单ROI：最大角点间隙 px"), "cornerGap", 0, 1000);
    setDetectionInputs(GraphicalDetectionParameters());
    auto* applyDetection = new QPushButton(QStringLiteral("应用检测参数"), detectionPage);
    auto* resetDetection = new QPushButton(QStringLiteral("恢复默认（未提交）"), detectionPage);
    detectionLayout->addWidget(applyDetection);
    detectionLayout->addWidget(resetDetection);
    connect(applyDetection, &QPushButton::clicked, this, &GraphicalProgramEditor::applyDetectionParameters);
    connect(resetDetection, &QPushButton::clicked, this, [this]() { setDetectionInputs(GraphicalDetectionParameters()); });
    QLabel* parameterHint = new QLabel(QStringLiteral("最短长度减小可保留短边，也可能引入噪声；最长长度0沿用图宽的一半。单ROI以90%轮廓点的点线偏差控制拟合质量，允许少量毛刺或圆角过渡点；角点间隙允许两条拟合线跨过过渡后相交。数值过大可能接纳干扰边。"), detectionPage);
    parameterHint->setWordWrap(true);
    detectionLayout->addWidget(parameterHint);
    m_detectionDiagnostic = new QLabel(QStringLiteral("选中记录后显示最近执行状态。"), detectionPage);
    m_detectionDiagnostic->setObjectName(QStringLiteral("detectionDiagnostic"));
    m_detectionDiagnostic->setWordWrap(true);
    m_detectionDiagnostic->setTextInteractionFlags(Qt::TextSelectableByMouse);
    detectionLayout->addWidget(m_detectionDiagnostic);
    detectionLayout->addStretch();
    detectionScroll->setWidget(detectionPage);
    const int detectionTab = propertyTabs->addTab(detectionScroll, QStringLiteral("检测参数"));
    propertyTabs->setTabToolTip(detectionTab, QStringLiteral("检测参数"));

    QWidget* positionPage = new QWidget(propertyTabs);//设备点位页
    QVBoxLayout* positionLayout = new QVBoxLayout(positionPage);
    QGroupBox* cameraGroup = new QGroupBox(QStringLiteral("相机采集"), positionPage);
    QVBoxLayout* cameraLayout = new QVBoxLayout(cameraGroup);
    QFormLayout* cameraForm = new QFormLayout;
    cameraForm->setRowWrapPolicy(QFormLayout::WrapAllRows);
    m_cameraSelector = new QComboBox(cameraGroup);
    m_cameraSelector->addItem(QStringLiteral("0 · 远心相机"), 0);
    m_cameraSelector->addItem(QStringLiteral("1 · 孔径相机"), 1);
    cameraForm->addRow(QStringLiteral("相机"), m_cameraSelector);
    m_cameraExposure = new QSpinBox(cameraGroup);
    m_cameraExposure->setRange(0, 30000);
    m_cameraExposure->setValue(400);
    m_cameraExposure->setSuffix(QStringLiteral(" μs"));
    m_cameraExposure->setKeyboardTracking(false);
    cameraForm->addRow(QStringLiteral("曝光"), m_cameraExposure);
    cameraLayout->addLayout(cameraForm);
    m_cameraState = new QLabel(QStringLiteral("相机接口未连接"), cameraGroup);
    m_cameraState->setWordWrap(true);
    cameraLayout->addWidget(m_cameraState);
    m_cameraStart = new QPushButton(QStringLiteral("开始连续采集"), cameraGroup);
    m_cameraStop = new QPushButton(QStringLiteral("停止采集"), cameraGroup);
    m_cameraLoad = new QPushButton(QStringLiteral("载入最后一帧"), cameraGroup);
    cameraLayout->addWidget(m_cameraStart);
    cameraLayout->addWidget(m_cameraStop);
    cameraLayout->addWidget(m_cameraLoad);
    QLabel* cameraHint = new QLabel(QStringLiteral(
        "请先开始采集，等待图像稳定后停止，再载入最后一帧。配方参考图由软件自动管理；载入新图像会清空当前图形和测量记录。"), cameraGroup);
    cameraHint->setWordWrap(true);
    cameraLayout->addWidget(cameraHint);
    positionLayout->addWidget(cameraGroup);

    QGroupBox* lightCurtainGroup = new QGroupBox(QStringLiteral("光幕传感器"), positionPage);
    QVBoxLayout* lightCurtainLayout = new QVBoxLayout(lightCurtainGroup);
    m_lightCurtainState = new QLabel(QStringLiteral("光幕接口未连接"), lightCurtainGroup);
    m_lightCurtainState->setWordWrap(true);
    m_lightCurtainState->setTextInteractionFlags(Qt::TextSelectableByMouse);
    lightCurtainLayout->addWidget(m_lightCurtainState);
    QLabel* lightCurtainHint = new QLabel(QStringLiteral(
        "直径点位记录读取光幕OUT1最新有效样本，并同时保存原始值和原软件补偿结果。"), lightCurtainGroup);
    lightCurtainHint->setWordWrap(true);
    lightCurtainLayout->addWidget(lightCurtainHint);
    positionLayout->addWidget(lightCurtainGroup);

    QGroupBox* pointGroup = new QGroupBox(QStringLiteral("测量记录点位"), positionPage);
    QVBoxLayout* pointLayout = new QVBoxLayout(pointGroup);
    m_devicePositionState = new QLabel(QStringLiteral("请先选择一条测量记录。"), pointGroup);
    m_devicePositionState->setWordWrap(true);
    m_devicePositionState->setTextInteractionFlags(Qt::TextSelectableByMouse);
    pointLayout->addWidget(m_devicePositionState);
    m_recordDevicePosition = new QPushButton(QStringLiteral("记录选中记录的当前设备点位"), pointGroup);
    m_clearDevicePosition = new QPushButton(QStringLiteral("清除选中记录的设备点位"), pointGroup);
    pointLayout->addWidget(m_recordDevicePosition);
    pointLayout->addWidget(m_clearDevicePosition);
    QLabel* pointHint = new QLabel(QStringLiteral(
        "直径/圆柱度/跳动记录光幕轴；孔径记录测孔轴、光幕轴和孔径相机曝光；远心图像测量记录光幕轴和远心相机曝光。单位为pulse。"), pointGroup);
    pointHint->setWordWrap(true);
    pointLayout->addWidget(pointHint);
    positionLayout->addWidget(pointGroup);
    positionLayout->addStretch();
    const int positionTab = propertyTabs->addTab(positionPage, QStringLiteral("设备点位"));
    propertyTabs->setTabToolTip(positionTab, QStringLiteral("设备点位"));

    QWidget* recipePage = new QWidget(propertyTabs);
    QVBoxLayout* recipeLayout = new QVBoxLayout(recipePage);
    QFormLayout* recipeForm = new QFormLayout;
    m_recipeProgramNumber = new QSpinBox(recipePage);
    m_recipeProgramNumber->setObjectName(QStringLiteral("recipeProgramNumber"));
    m_recipeProgramNumber->setRange(0, 50);
    m_recipeProgramNumber->setSpecialValueText(QStringLiteral("未设置"));
    m_recipeProgramNumber->setToolTip(QStringLiteral("当前主程序已登记0–50号槽位；生成前还需检查该编号是否已被占用。"));
    m_recipePartNumber = new QLineEdit(recipePage);
    m_recipePartNumber->setObjectName(QStringLiteral("recipePartNumber"));
    m_recipePartName = new QLineEdit(recipePage);
    m_recipePartName->setObjectName(QStringLiteral("recipePartName"));
    m_recipeProcessNumber = new QLineEdit(recipePage);
    m_recipeProcessNumber->setObjectName(QStringLiteral("recipeProcessNumber"));
    m_recipeNote = new QLineEdit(recipePage);
    m_recipeNote->setObjectName(QStringLiteral("recipeNote"));
    for (QLineEdit* input : {m_recipePartNumber, m_recipePartName, m_recipeProcessNumber, m_recipeNote})
        input->setMaxLength(128);
    recipeForm->addRow(QStringLiteral("程序号（1–50）："), m_recipeProgramNumber);
    recipeForm->addRow(QStringLiteral("零件图号："), m_recipePartNumber);
    recipeForm->addRow(QStringLiteral("零件名称："), m_recipePartName);
    recipeForm->addRow(QStringLiteral("工序号："), m_recipeProcessNumber);
    recipeForm->addRow(QStringLiteral("备注："), m_recipeNote);
    recipeLayout->addLayout(recipeForm);
    QPushButton* validateRecipe = new QPushButton(QStringLiteral("检查生成条件"), recipePage);
    validateRecipe->setObjectName(QStringLiteral("validateRecipeButton"));
    recipeLayout->addWidget(validateRecipe);
    m_recipeValidationResult = new QLabel(
        QStringLiteral("填写配方信息后检查记录、ROI、标定和设备点位。"), recipePage);
    m_recipeValidationResult->setObjectName(QStringLiteral("recipeValidationResult"));
    m_recipeValidationResult->setWordWrap(true);
    m_recipeValidationResult->setTextInteractionFlags(Qt::TextSelectableByMouse);
    recipeLayout->addWidget(m_recipeValidationResult);
    QLabel* recipeHint = new QLabel(QStringLiteral(
        "检查通过表示配方数据已具备进入程序映射的条件；程序号冲突检查和旧Excel/VBA生成将在接入生成器后执行。"), recipePage);
    recipeHint->setWordWrap(true);
    recipeLayout->addWidget(recipeHint);
    recipeLayout->addStretch();
    QScrollArea* recipeScroll = new QScrollArea(propertyTabs);
    recipeScroll->setWidgetResizable(true);
    recipeScroll->setWidget(recipePage);
    const int recipeTab = propertyTabs->addTab(recipeScroll, QStringLiteral("配方信息"));
    propertyTabs->setTabToolTip(recipeTab, QStringLiteral("配方信息与生成检查"));
    const auto markRecipeDirty = [this]() {
        if (!m_loadingProject) m_projectDirty = true;
        if (m_recipeValidationResult)
            m_recipeValidationResult->setText(QStringLiteral("配方已修改，请重新检查生成条件。"));
    };
    connect(m_recipeProgramNumber, QOverload<int>::of(&QSpinBox::valueChanged), this,
        [markRecipeDirty](int) { markRecipeDirty(); });
    for (QLineEdit* input : {m_recipePartNumber, m_recipePartName, m_recipeProcessNumber, m_recipeNote})
        connect(input, &QLineEdit::textEdited, this, markRecipeDirty);
    connect(validateRecipe, &QPushButton::clicked, this, [this]() {
        const QStringList issues = validateRecipeForExport();
        if (issues.isEmpty()) {
            m_recipeValidationResult->setText(QStringLiteral(
                "检查通过：配方基础数据完整，可以进入程序映射。"));
            statusBar()->showMessage(QStringLiteral("生成条件检查通过；尚未生成生产程序。"), 6000);
        }
        else {
            QStringList lines;
            for (int index = 0; index < issues.size(); ++index)
                lines << QStringLiteral("%1. %2").arg(index + 1).arg(issues[index]);
            m_recipeValidationResult->setText(QStringLiteral("发现%1项问题：\n%2")
                .arg(issues.size()).arg(lines.join(QLatin1Char('\n'))));
            statusBar()->showMessage(QStringLiteral("生成条件检查未通过：%1项问题。").arg(issues.size()), 6000);
        }
    });
    propertyTabs->setMinimumWidth(400);

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
    mainSplitter->setSizes(QList<int>() << 300 << 740 << 400);

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
    connect(addFrameAction, &QAction::triggered, this, [this]() { addLocalFrame(); });
    connect(removeFrameAction, &QAction::triggered, this, [this]() { removeCurrentFrame(); });
    connect(m_frameSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
        [this](int index) {
            if (m_loadingProject || index < 0) return;
            const int frameId = m_frameSelector->itemData(index).toInt();
            if (frameId == m_currentFrameId) return;
            QString error;
            if (!activateFrame(frameId, error)) {
                QMessageBox::warning(this, QStringLiteral("切换帧失败"), error);
                refreshFrameSelector();
            }
        });
    connect(cameraAction, &QAction::triggered, this, [propertyTabs, positionPage]() {
        propertyTabs->setCurrentWidget(positionPage);
    });
    connect(openProjectAction, &QAction::triggered, this, &GraphicalProgramEditor::openProject);
    connect(saveProjectAction, &QAction::triggered, this, &GraphicalProgramEditor::saveProject);
    connect(m_cameraSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
        [this]() { m_cameraExposureEdited = false; refreshCameraPanel(); });
    connect(m_cameraExposure, QOverload<int>::of(&QSpinBox::valueChanged), this,
        [this]() { m_cameraExposureEdited = true; });
    connect(m_cameraStart, &QPushButton::clicked, this,
        [this]() { executeCameraCommand(CameraCommand::StartCapture); });
    connect(m_cameraStop, &QPushButton::clicked, this,
        [this]() { executeCameraCommand(CameraCommand::StopCapture); });
    connect(m_cameraLoad, &QPushButton::clicked, this,
        [this]() { executeCameraCommand(CameraCommand::Snapshot); });
    connect(m_recordDevicePosition, &QPushButton::clicked, this,
        &GraphicalProgramEditor::recordSelectedDevicePosition);
    connect(m_clearDevicePosition, &QPushButton::clicked, this,
        &GraphicalProgramEditor::clearSelectedDevicePosition);
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
    connect(deleteAction, &QAction::triggered, this, [this]() {
        if (m_canvas->hasSelectedFeatures()) m_canvas->deleteSelectedFeatures();
        else statusBar()->showMessage(QStringLiteral("请先在画布中选中要删除的图形。"), 3000);
    });
    connect(m_canvas, &GraphicalCanvas::featuresChanged, this, &GraphicalProgramEditor::refreshFeatureList);
    connect(m_canvas, &GraphicalCanvas::featuresChanged, this, &GraphicalProgramEditor::refreshMeasurementRecords);
    connect(m_canvas, &GraphicalCanvas::featuresChanged, this, [this]() {
        if (!m_loadingProject) m_projectDirty = true;
    });
    connect(m_canvas, &GraphicalCanvas::featureGeometryChanged, this,
        [this](int featureId) {
            if (!m_loadingProject) m_projectDirty = true;
            for (MeasurementRecord& record : m_records) {
                const bool primary = record.frameId == m_currentFrameId && record.geometryId == featureId;
                const bool secondary = record.secondaryFrameId == m_currentFrameId
                    && record.secondaryGeometryId == featureId;
                if (!primary && !secondary) continue;
                record.clearTrial(QStringLiteral("未执行（图形已修改）"));
                if (record.type == QStringLiteral("长度")) {
                    if (record.crossFrameLength) {
                        if (primary) record.lengthStartPosition = MeasurementRecord::DevicePosition();
                        if (secondary) record.lengthEndPosition = MeasurementRecord::DevicePosition();
                    }
                    record.lengthTemplateModel.clear();
                    record.lengthTemplateReferenceRow = 0;
                    record.lengthTemplateReferenceColumn = 0;
                    record.lengthTemplateReferenceAngle = 0;
                }
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
                                QStringLiteral("角度ROI必须是宽高至少2px的矩形或半径至少1px的圆。请重新选择；Esc取消。"));
                            return;
                        }
                        const int otherGeometry = m_relinkSlot == 2
                            ? m_records[row].geometryId : m_records[row].secondaryGeometryId;
                        const int otherFrame = m_relinkSlot == 2
                            ? m_records[row].frameId : m_records[row].secondaryFrameId;
                        if (otherGeometry > 0 && otherFrame != m_currentFrameId) {
                            QMessageBox::warning(this, QStringLiteral("不能关联"),
                                QStringLiteral("同一角度记录的两个ROI必须位于同一图像。请先切换到图像%1；Esc取消。")
                                    .arg(otherFrame));
                            return;
                        }
                        if (otherGeometry == featureId && otherFrame == m_currentFrameId) {
                            QMessageBox::warning(this, QStringLiteral("不能关联"),
                                QStringLiteral("ROI 1 和 ROI 2 不能选择同一个图形。请重新选择；Esc取消。"));
                            return;
                        }
                    }
                    else if (m_records[row].type == QStringLiteral("长度")) {
                        GraphicalCanvas::MeasurementRoi roi;
                        if (!m_canvas->measurementRoi(featureId, roi) || roi.isCircle) {
                            QMessageBox::warning(this, QStringLiteral("不能关联"),
                                QStringLiteral("长度模板须关联一个宽高至少2px的矩形ROI，并框住完整特征和两条目标边。请重新选择；Esc取消。"));
                            return;
                        }
                        if (m_records[row].crossFrameLength) {
                            if (m_imageCameraIndex >= 0 && m_imageCameraIndex != 0) {
                                QMessageBox::warning(this, QStringLiteral("不能确认端点"),
                                    QStringLiteral("跨图长度端点必须使用远心相机0图像。"));
                                return;
                            }
                            const int otherFrame = m_relinkSlot == 2
                                ? m_records[row].frameId : m_records[row].secondaryFrameId;
                            if (otherFrame > 0 && otherFrame == m_currentFrameId) {
                                QMessageBox::warning(this, QStringLiteral("不能确认端点"),
                                    QStringLiteral("跨图长度的起点和终点必须来自两次不同的端点图像；请切换图像后再确认。"));
                                return;
                            }
                        }
                    }
                    else if (m_records[row].type == QStringLiteral("孔径")) {
                        GraphicalCanvas::MeasurementRoi roi;
                        if (!m_canvas->measurementRoi(featureId, roi)) {
                            QMessageBox::warning(this, QStringLiteral("不能关联"),
                                QStringLiteral("孔径ROI必须是宽高至少2px的矩形或半径至少1px的圆。请重新选择；Esc取消。"));
                            return;
                        }
                    }
                    const int completedSlot = m_relinkSlot;
                    MeasurementRecord::DevicePosition endpointPosition;
                    if (m_records[row].type == QStringLiteral("长度")
                        && m_records[row].crossFrameLength && m_imageCameraIndex >= 0) {
                        QString pointError;
                        if (!collectCurrentDevicePosition(QStringLiteral("长度"), endpointPosition, pointError)) {
                            QMessageBox::warning(this, QStringLiteral("端点确认失败"), pointError);
                            return;
                        }
                    }
                    if (completedSlot == 2) {
                        m_records[row].secondaryGeometryId = featureId;
                        m_records[row].secondaryFrameId = m_currentFrameId;
                    }
                    else {
                        m_records[row].geometryId = featureId;
                        m_records[row].frameId = m_currentFrameId;
                    }
                    if (m_records[row].type == QStringLiteral("长度")) {
                        if (m_records[row].crossFrameLength) {
                            if (completedSlot == 1) m_records[row].lengthStartPosition = endpointPosition;
                            else m_records[row].lengthEndPosition = endpointPosition;
                        }
                        else {
                            m_records[row].secondaryGeometryId = -1;
                            m_records[row].secondaryFrameId = 0;
                        }
                        m_records[row].lengthTemplateModel.clear();
                        m_records[row].lengthTemplateReferenceRow = 0;
                        m_records[row].lengthTemplateReferenceColumn = 0;
                        m_records[row].lengthTemplateReferenceAngle = 0;
                    }
                    m_records[row].clearTrial(QStringLiteral("未执行（关联已修改）"));
                    m_projectDirty = true;
                    cancelRelink();
                    refreshMeasurementRecords();
                    QSignalBlocker blocker(m_stepTable);
                    m_stepTable->setCurrentCell(row, 0);
                    m_stepTable->selectRow(row);
                    statusBar()->showMessage(QStringLiteral("记录 %1 已关联图形 %2；序号、类型和公差保持不变。")
                        .arg(targetSequence).arg(featureId));
                    break;
                }
            }
            {
                QSignalBlocker tableBlocker(m_stepTable);
                const int current = m_stepTable->currentRow();
                if (current < 0 || current >= m_records.size()
                    || !(m_records[current].frameId == m_currentFrameId
                        && m_records[current].geometryId == featureId)
                    && !(m_records[current].secondaryFrameId == m_currentFrameId
                        && m_records[current].secondaryGeometryId == featureId)) {
                    m_stepTable->clearSelection();
                    m_stepTable->setCurrentCell(-1, -1);
                    for (int row = 0; featureId > 0 && row < m_records.size(); ++row) {
                        if ((m_records[row].frameId == m_currentFrameId
                                && m_records[row].geometryId == featureId)
                            || (m_records[row].secondaryFrameId == m_currentFrameId
                                && m_records[row].secondaryGeometryId == featureId)) {
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
                    m_angleInputMode->setCurrentIndex(record.singleRoiAngle ? 1 : 0);
                    m_holeUniformCount->setValue(record.holeUniformCount);
                    m_holeCalibration->setValue(record.holeCalibration);
                    m_lengthCalibration->setValue(record.lengthCalibration);
                    m_lengthMode->setCurrentIndex(record.crossFrameLength ? 1 : 0);
                    m_lowerAxialOffset->setValue(record.lowerAxialOffsetPulse);
                    m_upperAxialOffset->setValue(record.upperAxialOffsetPulse);
                    m_roundoutReference1->setText(record.roundoutReference1);
                    m_roundoutReference2->setText(record.roundoutReference2);
                    setDetectionInputs(record.detection);
                    m_featureNumber->setText(record.featureNumber);
                    m_featureNumberEditedSinceLoad = false;
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
    refreshAngleControls();
    statusBar()->showMessage(QStringLiteral("请打开本地图像开始编辑"));
}

GraphicalDetectionParameters GraphicalProgramEditor::detectionInputs() const
{
    GraphicalDetectionParameters parameters;
    parameters.smoothing = m_detectionSmoothing->value();
    parameters.lowThreshold = m_detectionLow->value();
    parameters.highThreshold = m_detectionHigh->value();
    parameters.minLength = m_detectionMinLength->value();
    parameters.maxLength = m_detectionMaxLength->value();
    parameters.mergeDistance = m_detectionMergeDistance->value();
    parameters.cornerMaxDeviation = m_cornerDeviation->value();
    parameters.cornerMaxGap = m_cornerGap->value();
    return parameters;
}

void GraphicalProgramEditor::setDetectionInputs(const GraphicalDetectionParameters& parameters)
{
    m_detectionSmoothing->setValue(parameters.smoothing);
    m_detectionLow->setValue(parameters.lowThreshold);
    m_detectionHigh->setValue(parameters.highThreshold);
    m_detectionMinLength->setValue(parameters.minLength);
    m_detectionMaxLength->setValue(parameters.maxLength);
    m_detectionMergeDistance->setValue(parameters.mergeDistance);
    m_cornerDeviation->setValue(parameters.cornerMaxDeviation);
    m_cornerGap->setValue(parameters.cornerMaxGap);
}

void GraphicalProgramEditor::applyDetectionParameters()
{
    const int row = m_stepTable->currentRow();
    if (m_trialRunning || m_relinkSequence > 0) {
        m_detectionDiagnostic->setText(QStringLiteral("计算或关联期间不能应用参数。")); return;
    }
    if (row < 0 || row >= m_records.size()) {
        m_detectionDiagnostic->setText(QStringLiteral("请先选择一条测量记录。")); return;
    }
    if (m_records[row].type != QStringLiteral("圆弧半径")
        && m_records[row].type != QStringLiteral("角度")
        && m_records[row].type != QStringLiteral("长度")) {
        m_detectionDiagnostic->setText(QStringLiteral("当前圆弧半径、角度和单图长度支持这些检测参数。孔径沿用原算法固定参数。")); return;
    }
    const auto parameters = detectionInputs();
    const QString error = parameters.validationError();
    if (!error.isEmpty()) { m_detectionDiagnostic->setText(error + QStringLiteral("原记录保持不变。")); return; }
    auto& record = m_records[row];
    record.detection = parameters;
    record.clearTrial(QStringLiteral("未执行（检测参数已更新，旧结果失效）"));
    m_projectDirty = true;
    refreshMeasurementRecords();
    showRecordDetection(row);
    statusBar()->showMessage(QStringLiteral("已应用记录 %1 的检测参数；其他测量配置输入未提交。").arg(record.sequence));
}

QString GraphicalProgramEditor::suggestedFeatureNumber(const QString& type) const
{
    QString prefix;
    if (type == QStringLiteral("直径")) prefix = QStringLiteral("D");
    else if (type == QStringLiteral("孔径")) prefix = QStringLiteral("H");
    else if (type == QStringLiteral("圆柱度")) prefix = QStringLiteral("CY");
    else if (type == QStringLiteral("跳动")) prefix = QStringLiteral("T");
    else if (type == QStringLiteral("长度")) prefix = QStringLiteral("L");
    else if (type == QStringLiteral("角度")) prefix = QStringLiteral("A");
    else if (type == QStringLiteral("圆弧半径")) prefix = QStringLiteral("AR");
    else prefix = QStringLiteral("F");
    int number = 1;
    while (true) {
        const QString candidate = prefix + QString::number(number++);
        bool used = false;
        for (const MeasurementRecord& record : m_records)
            if (record.featureNumber.compare(candidate, Qt::CaseInsensitive) == 0) { used = true; break; }
        if (!used) return candidate;
    }
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
    if (!update && !m_featureNumberEditedSinceLoad)
        m_featureNumber->setText(suggestedFeatureNumber(m_measurementType->currentText()));
    if (m_featureNumber->text().trimmed().isEmpty())
        m_featureNumber->setText(suggestedFeatureNumber(m_measurementType->currentText()));
    if (m_hasTolerance->isChecked() && m_lowerDeviation->value() > m_upperDeviation->value()) {
        QMessageBox::warning(this, QStringLiteral("未记录"), QStringLiteral("下偏差不能大于上偏差。"));
        return;
    }
    if (m_measurementType->currentText() == QStringLiteral("孔径")
        && m_holeUniformCount->value() <= 0) {
        QMessageBox::warning(this, QStringLiteral("未记录"),
            QStringLiteral("孔径记录需要填写孔均布个数。该值对应旧表单的“均布个数”，不是H0/H1拍照位置。"));
        return;
    }
    const bool axialScan = m_measurementType->currentText() == QStringLiteral("圆柱度")
        || m_measurementType->currentText() == QStringLiteral("跳动");
    if (axialScan && (m_lowerAxialOffset->value() <= 0 || m_upperAxialOffset->value() <= 0)) {
        QMessageBox::warning(this, QStringLiteral("未记录"),
            QStringLiteral("圆柱度和圆跳动需要设置大于0的下侧、上侧轴5偏移量。"));
        return;
    }
    const auto parameters = detectionInputs();
    const QString parameterError = parameters.validationError();
    if (!parameterError.isEmpty() && (m_measurementType->currentText() == QStringLiteral("角度")
        || m_measurementType->currentText() == QStringLiteral("长度")
        || m_measurementType->currentText() == QStringLiteral("圆弧半径"))) {
        QMessageBox::warning(this, QStringLiteral("未记录"), parameterError); return;
    }
    MeasurementRecord record;
    record.detection = parameters;
    record.sequence = update ? m_records[row].sequence : m_nextRecordSequence++;
    record.frameId = update ? m_records[row].frameId : m_currentFrameId;
    record.geometryId = update ? m_records[row].geometryId : m_selectedFeatureId;
    record.secondaryFrameId = update ? m_records[row].secondaryFrameId : 0;
    record.secondaryGeometryId = update ? m_records[row].secondaryGeometryId : -1;
    record.featureNumber = m_featureNumber->text().trimmed();
    record.type = m_measurementType->currentText();
    record.holeUniformCount = record.type == QStringLiteral("孔径") ? m_holeUniformCount->value() : 0;
    record.holeCalibration = m_holeCalibration->value();
    record.lengthCalibration = m_lengthCalibration->value();
    record.lowerAxialOffsetPulse = axialScan ? m_lowerAxialOffset->value() : 0;
    record.upperAxialOffsetPulse = axialScan ? m_upperAxialOffset->value() : 0;
    record.roundoutReference1 = record.type == QStringLiteral("跳动")
        ? m_roundoutReference1->text().trimmed() : QString();
    record.roundoutReference2 = record.type == QStringLiteral("跳动")
        ? m_roundoutReference2->text().trimmed() : QString();
    record.crossFrameLength = record.type == QStringLiteral("长度")
        && m_lengthMode->currentData().toBool();
    record.useSupplementaryAngle = m_angleResultMode->currentIndex() == 1;
    record.singleRoiAngle = record.type == QStringLiteral("角度") && m_angleInputMode->currentIndex() == 1;
    if (record.singleRoiAngle || record.type != QStringLiteral("角度"))
        { record.secondaryGeometryId = -1; record.secondaryFrameId = 0; }
    if (record.crossFrameLength && update && m_records[row].type == QStringLiteral("长度")
        && m_records[row].crossFrameLength) {
        record.secondaryGeometryId = m_records[row].secondaryGeometryId;
        record.secondaryFrameId = m_records[row].secondaryFrameId;
        record.lengthStartPosition = m_records[row].lengthStartPosition;
        record.lengthEndPosition = m_records[row].lengthEndPosition;
    }
    if (update && record.type == QStringLiteral("长度")
        && m_records[row].type == QStringLiteral("长度")
        && !record.crossFrameLength && !m_records[row].crossFrameLength
        && record.geometryId == m_records[row].geometryId) {
        record.lengthTemplateModel = m_records[row].lengthTemplateModel;
        record.lengthTemplateReferenceRow = m_records[row].lengthTemplateReferenceRow;
        record.lengthTemplateReferenceColumn = m_records[row].lengthTemplateReferenceColumn;
        record.lengthTemplateReferenceAngle = m_records[row].lengthTemplateReferenceAngle;
    }
    record.hasTolerance = m_hasTolerance->isChecked();
    record.nominal = m_nominal->value();
    record.lower = m_lowerDeviation->value();
    record.upper = m_upperDeviation->value();
    if (update && record.type == m_records[row].type)
        record.devicePosition = m_records[row].devicePosition;
    if (update) m_records[row] = record;
    else m_records.append(record);
    m_projectDirty = true;
    refreshMeasurementRecords();
    m_stepTable->setCurrentCell(update ? row : m_records.size() - 1, 0);
    const bool pointCollected = m_records[update ? row : m_records.size() - 1].devicePosition.collected;
    statusBar()->showMessage(QStringLiteral("记录 %1 已配置；算法未执行，点位%2，工程尚未保存。")
        .arg(record.sequence).arg(pointCollected ? QStringLiteral("已保留") : QStringLiteral("未采集")));
}

void GraphicalProgramEditor::refreshMeasurementRecords()//把 m_records 刷到表格；同时检查关联图形有没有被删掉（删了就标"关联已删除"并清空试测结果）。
{
    if (m_loadingProject) return;
    QSignalBlocker blocker(m_stepTable);
    const int previousRow = m_stepTable->currentRow();
    m_stepTable->setRowCount(m_records.size());
    for (int row = 0; row < m_records.size(); ++row) {
        MeasurementRecord& record = m_records[row];
        const bool primaryOnCurrentFrame = record.frameId == m_currentFrameId;
        const bool secondaryOnCurrentFrame = record.secondaryFrameId == m_currentFrameId;
        const QStringList geometry = primaryOnCurrentFrame
            ? m_canvas->featureProperties(record.geometryId) : QStringList();
        const QStringList secondaryGeometry = secondaryOnCurrentFrame
            ? m_canvas->featureProperties(record.secondaryGeometryId) : QStringList();
        const bool primaryMissing = primaryOnCurrentFrame
            && record.geometryId > 0 && geometry.size() != 3;
        const bool secondaryMissing = (record.type == QStringLiteral("角度")
                || (record.type == QStringLiteral("长度") && record.crossFrameLength))
            && secondaryOnCurrentFrame && record.secondaryGeometryId > 0
            && secondaryGeometry.size() != 3;
        if (primaryMissing || secondaryMissing) {
            record.clearTrial(QStringLiteral("未执行（关联已删除）"));
            if (record.type == QStringLiteral("长度")) {
                if (record.crossFrameLength) {
                    if (primaryMissing) record.lengthStartPosition = MeasurementRecord::DevicePosition();
                    if (secondaryMissing) record.lengthEndPosition = MeasurementRecord::DevicePosition();
                }
                record.lengthTemplateModel.clear();
                record.lengthTemplateReferenceRow = 0;
                record.lengthTemplateReferenceColumn = 0;
                record.lengthTemplateReferenceAngle = 0;
            }
        }
        QString association;
        if (record.type == QStringLiteral("角度")) {
            const QString first = record.geometryId <= 0 ? QStringLiteral("待关联")
                : !primaryOnCurrentFrame ? QStringLiteral("图%1/图形%2").arg(record.frameId).arg(record.geometryId)
                : geometry.size() == 3 ? geometry[0] : QStringLiteral("已删除");
            const QString second = record.secondaryGeometryId <= 0 ? QStringLiteral("待关联")
                : !secondaryOnCurrentFrame ? QStringLiteral("图%1/图形%2").arg(record.secondaryFrameId).arg(record.secondaryGeometryId)
                : secondaryGeometry.size() == 3 ? secondaryGeometry[0] : QStringLiteral("已删除");
            association = record.type == QStringLiteral("角度") && record.singleRoiAngle
                ? QStringLiteral("单ROI:%1").arg(first)
                : QStringLiteral("ROI1:%1；ROI2:%2").arg(first, second);
        }
        else if (record.type == QStringLiteral("长度")) {
            const QString target = record.geometryId <= 0 ? QStringLiteral("待关联")
                : !primaryOnCurrentFrame ? QStringLiteral("图%1/图形%2").arg(record.frameId).arg(record.geometryId)
                : geometry.size() == 3 ? geometry[0] : QStringLiteral("已删除");
            if (record.crossFrameLength) {
                const auto endpointName = [this](int frameId, int geometryId,
                    const QStringList& currentProperties, bool onCurrentFrame) {
                    if (geometryId <= 0) return QStringLiteral("待关联");
                    QString name;
                    if (onCurrentFrame && currentProperties.size() == 3) name = currentProperties[0];
                    else {
                        for (const ProjectFrame& frame : m_frames) {
                            if (frame.id != frameId) continue;
                            for (const auto& feature : frame.features)
                                if (feature.id == geometryId) {
                                    name = QStringLiteral("%1_%2").arg(feature.type).arg(feature.id);
                                    break;
                                }
                        }
                    }
                    if (name.isEmpty()) name = QStringLiteral("已删除");
                    return QStringLiteral("图像%1/%2").arg(frameId).arg(name);
                };
                const QString start = endpointName(record.frameId, record.geometryId,
                    geometry, primaryOnCurrentFrame);
                const QString end = endpointName(record.secondaryFrameId, record.secondaryGeometryId,
                    secondaryGeometry, secondaryOnCurrentFrame);
                association = QStringLiteral("起点:%1；终点:%2").arg(start, end);
            }
            else association = QStringLiteral("单ROI:%1").arg(target);
        }
        else association = record.geometryId <= 0 ? QStringLiteral("待关联")
            : !primaryOnCurrentFrame ? QStringLiteral("图%1/图形%2").arg(record.frameId).arg(record.geometryId)
            : geometry.size() == 3 ? geometry[0] : QStringLiteral("关联图形已删除");
        const QString unit = record.type == QStringLiteral("角度") ? QStringLiteral("°") : QStringLiteral("mm");
        QString deviceSummary = QStringLiteral("未采集");
        if ((record.type == QStringLiteral("圆柱度") || record.type == QStringLiteral("跳动"))
            && !record.devicePosition.collected)
            deviceSummary = QStringLiteral("中点未采集；偏移-%1/+%2 pulse")
                .arg(record.lowerAxialOffsetPulse).arg(record.upperAxialOffsetPulse);
        if (record.type == QStringLiteral("长度") && record.crossFrameLength) {
            const auto axis5Encoder = [](const MeasurementRecord::DevicePosition& position,
                double& encoder) {
                if (!position.collected) return false;
                for (const auto& axis : position.axes) if (axis.axis == 5) {
                    encoder = axis.encoder; return std::isfinite(encoder);
                }
                return false;
            };
            double start = 0, end = 0;
            const bool hasStart = axis5Encoder(record.lengthStartPosition, start);
            const bool hasEnd = axis5Encoder(record.lengthEndPosition, end);
            QStringList parts;
            parts << (hasStart ? QStringLiteral("起点E:%1").arg(start, 0, 'f', 1)
                : QStringLiteral("起点未采集"));
            parts << (hasEnd ? QStringLiteral("终点E:%1").arg(end, 0, 'f', 1)
                : QStringLiteral("终点未采集"));
            if (hasStart && hasEnd)
                parts << QStringLiteral("补偿位移:%1mm")
                    .arg(axis5_compensation(end) - axis5_compensation(start), 0, 'f', 4);
            deviceSummary = parts.join(QStringLiteral("；"));
        }
        else if (record.devicePosition.collected) {
            QStringList parts;
            for (const auto& axis : record.devicePosition.axes)
                parts << QStringLiteral("轴%1:%2").arg(axis.axis).arg(axis.encoder, 0, 'f', 1);
            if (record.type == QStringLiteral("圆柱度") || record.type == QStringLiteral("跳动")) {
                for (const auto& axis : record.devicePosition.axes) {
                    if (axis.axis != 5 || !std::isfinite(axis.encoder)) continue;
                    parts << QStringLiteral("三截面:%1/%2/%3")
                        .arg(axis.encoder - record.lowerAxialOffsetPulse, 0, 'f', 1)
                        .arg(axis.encoder, 0, 'f', 1)
                        .arg(axis.encoder + record.upperAxialOffsetPulse, 0, 'f', 1);
                    break;
                }
            }
            if (record.devicePosition.cameraIndex >= 0)
                parts << QStringLiteral("相机%1/%2μs")
                    .arg(record.devicePosition.cameraIndex).arg(record.devicePosition.exposure);
            if (record.devicePosition.hasLightCurtainSample)
                parts << QStringLiteral("OUT1:%1→%2mm")
                    .arg(record.devicePosition.lightCurtainRawOut1, 0, 'f', 4)
                    .arg(record.devicePosition.lightCurtainDiameter, 0, 'f', 4);
            deviceSummary = parts.join(QStringLiteral("；"));
        }
        const QStringList cells = QStringList() << QString::number(record.sequence)
            << record.featureNumber << record.type << association
            << (record.hasTolerance ? QString::number(record.nominal, 'f', 4) : QStringLiteral("未设置"))
            << (record.hasTolerance ? QString::number(record.lower, 'f', 4) : QStringLiteral("—"))
            << (record.hasTolerance ? QString::number(record.upper, 'f', 4) : QStringLiteral("—"))
            << unit << (record.trialAngle >= 0 ? QStringLiteral("%1 °").arg(record.trialAngle, 0, 'f', 4)
                : record.trialLinearMm > 0 ? QStringLiteral("%1 mm").arg(record.trialLinearMm, 0, 'f', 4)
                : record.pixelRadius > 0 ? QStringLiteral("%1 px").arg(record.pixelRadius, 0, 'f', 4) : QStringLiteral("—"))
            << QStringLiteral("未判定") << deviceSummary << record.trialStatus.section('\n', 0, 0);
        for (int column = 0; column < cells.size(); ++column) {
            QTableWidgetItem* cell = new QTableWidgetItem(cells[column]);
            cell->setToolTip(column == 11 ? record.trialStatus : cells[column]);
            m_stepTable->setItem(row, column, cell);
        }
    }
    if (previousRow >= 0 && previousRow < m_records.size())
        m_stepTable->setCurrentCell(previousRow, 0);
    showRecordDetection(m_stepTable->currentRow());
    refreshDevicePositionPanel();
}

void GraphicalProgramEditor::loadMeasurementRecord(int row)
{
    if (row < 0 || row >= m_records.size()) return;
    const MeasurementRecord record = m_records[row];
    setDetectionInputs(record.detection);
    m_measurementType->setCurrentText(record.type);
    m_angleResultMode->setCurrentIndex(record.useSupplementaryAngle ? 1 : 0);
    m_angleInputMode->setCurrentIndex(record.singleRoiAngle ? 1 : 0);
    m_holeUniformCount->setValue(record.holeUniformCount);
    m_holeCalibration->setValue(record.holeCalibration);
    m_lengthCalibration->setValue(record.lengthCalibration);
    m_lengthMode->setCurrentIndex(record.crossFrameLength ? 1 : 0);
    m_lowerAxialOffset->setValue(record.lowerAxialOffsetPulse);
    m_upperAxialOffset->setValue(record.upperAxialOffsetPulse);
    m_roundoutReference1->setText(record.roundoutReference1);
    m_roundoutReference2->setText(record.roundoutReference2);
    m_featureNumber->setText(record.featureNumber);
    m_featureNumberEditedSinceLoad = false;
    m_hasTolerance->setChecked(record.hasTolerance);
    m_nominal->setValue(record.nominal);
    m_lowerDeviation->setValue(record.lower);
    m_upperDeviation->setValue(record.upper);
    if (record.frameId == m_currentFrameId)
        m_canvas->selectFeatureById(record.geometryId);
    // Selection signals may clear the table for a missing/unassociated geometry.
    QSignalBlocker blocker(m_stepTable);
    m_stepTable->setCurrentCell(row, 0);
    m_stepTable->selectRow(row);
    showRecordDetection(row);
    statusBar()->showMessage(QStringLiteral("记录 %1 / 特征 %2：%3；未判定，点位%4。")
        .arg(record.sequence).arg(record.featureNumber).arg(record.trialStatus)
        .arg(record.devicePosition.collected ? QStringLiteral("已采集") : QStringLiteral("未采集")));
    refreshDevicePositionPanel();
}

void GraphicalProgramEditor::showRecordDetection(int row)
{
    QSignalBlocker candidateBlocker(m_cornerCandidate);
    m_cornerCandidate->clear();
    m_cornerCandidate->addItem(QStringLiteral("请选择（选择后采用该边对）"));
    if (row >= 0 && row < m_records.size()) {
        const auto& record = m_records[row];
        for (const auto& pair : record.cornerPairs)
            m_cornerCandidate->addItem(QStringLiteral("边%1 + 边%2：%3°")
                .arg(record.cornerEdges[pair.firstEdge].candidateId)
                .arg(record.cornerEdges[pair.secondEdge].candidateId)
                .arg(record.useSupplementaryAngle ? 180 - pair.smallerAngle : pair.smallerAngle, 0, 'f', 4));
        m_cornerCandidate->setCurrentIndex(record.selectedCornerPair + 1);
    }
    refreshAngleControls();
    m_detectionDiagnostic->setText(row >= 0 && row < m_records.size()
        ? QStringLiteral("记录 %1：%2").arg(m_records[row].sequence).arg(m_records[row].trialStatus)
        : QStringLiteral("选中记录后显示最近执行状态。"));
    if (row >= 0 && row < m_records.size()) {
        const MeasurementRecord& record = m_records[row];
        if (record.type == QStringLiteral("长度") && record.crossFrameLength) {
            if (m_currentFrameId == record.frameId)
                m_canvas->setDetectionOverlay(record.crossStartDetectedEdges, record.crossStartFittedLine);
            else if (m_currentFrameId == record.secondaryFrameId)
                m_canvas->setDetectionOverlay(record.crossEndDetectedEdges, record.crossEndFittedLine);
            else m_canvas->setDetectionOverlay(QPainterPath(), QPainterPath());
        }
        else m_canvas->setDetectionOverlay(record.detectedEdges, record.fittedArc);
    }
    else m_canvas->setDetectionOverlay(QPainterPath(), QPainterPath());
}

void GraphicalProgramEditor::refreshAngleControls()
{
    const bool angle = m_measurementType->currentText() == QStringLiteral("角度");
    m_angleInputMode->setEnabled(angle);
    m_angleResultMode->setEnabled(angle);
    const int row = m_stepTable ? m_stepTable->currentRow() : -1;
    const bool storedAngle = row >= 0 && row < m_records.size() && m_records[row].type == QStringLiteral("角度");
    const bool storedLength = row >= 0 && row < m_records.size() && m_records[row].type == QStringLiteral("长度");
    const bool crossLength = storedLength && m_records[row].crossFrameLength;
    const bool single = storedAngle && m_records[row].singleRoiAngle;
    if (m_selectAngleRoi1) {
        m_selectAngleRoi1->setVisible(storedAngle || storedLength);
        m_selectAngleRoi1->setEnabled(storedAngle || storedLength);
        m_selectAngleRoi1->setText(crossLength ? QStringLiteral("跨图长度：确认起点ROI并采集点位")
            : storedLength ? QStringLiteral("长度：选择/重选单ROI（完整特征）")
            : single ? QStringLiteral("角度：选择/重选包含相邻角的ROI")
            : QStringLiteral("角度：选择/重选 ROI 1（直线1）"));
    }
    if (m_selectAngleRoi2) {
        m_selectAngleRoi2->setVisible((storedAngle && !single) || crossLength);
        m_selectAngleRoi2->setEnabled((storedAngle && !single) || crossLength);
        m_selectAngleRoi2->setText(crossLength ? QStringLiteral("跨图长度：确认终点ROI并采集点位")
            : storedLength ? QStringLiteral("长度：单ROI自动提取两条边")
            : QStringLiteral("角度：选择/重选 ROI 2（直线2）"));
    }
    m_cornerCandidate->setEnabled(single && !m_records[row].cornerPairs.isEmpty() && !m_trialRunning && m_relinkSequence <= 0);
}

void GraphicalProgramEditor::chooseCornerCandidate(int index)
{
    const int row = m_stepTable->currentRow();
    if (m_trialRunning || m_relinkSequence > 0 || row < 0 || row >= m_records.size()) return;
    auto& record = m_records[row];
    const int selected = index - 1;
    if (!record.singleRoiAngle || selected < 0 || selected >= record.cornerPairs.size()) {
        showRecordDetection(row); return;
    }
    const auto& pair = record.cornerPairs[selected];
    record.selectedCornerPair = selected;
    record.candidateSelectionAuditMode = QStringLiteral("manual");
    record.candidateSelectionAuditFirst = record.cornerEdges[pair.firstEdge].candidateId;
    record.candidateSelectionAuditSecond = record.cornerEdges[pair.secondEdge].candidateId;
    record.trialAngle = record.useSupplementaryAngle ? 180 - pair.smallerAngle : pair.smallerAngle;
    record.detectedEdges = record.cornerEdges[pair.firstEdge].contour;
    record.detectedEdges.addPath(record.cornerEdges[pair.secondEdge].contour);
    record.fittedArc = cornerPairOverlay(record.cornerEdges[pair.firstEdge], record.cornerEdges[pair.secondEdge], pair, record.useSupplementaryAngle);
    record.trialStatus = QStringLiteral("单ROI试测完成（已选择边%1 + 边%2，未判定）\n%3")
        .arg(record.cornerEdges[pair.firstEdge].candidateId).arg(record.cornerEdges[pair.secondEdge].candidateId).arg(record.cornerDiagnostic);
    m_projectDirty = true;
    refreshMeasurementRecords();
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
    const bool holeTrial = m_records[row].type == QStringLiteral("孔径");
    const bool lengthTrial = m_records[row].type == QStringLiteral("长度");
    const bool crossFrameLengthTrial = lengthTrial && m_records[row].crossFrameLength;
    const bool single = angleTrial && m_records[row].singleRoiAngle;
    const bool doubleRoi = angleTrial && !single;
    QImage crossStartImage, crossEndImage;
    GraphicalCanvas::MeasurementRoi crossStartRoi, crossEndRoi;
    bool crossHasPositions = false;
    double crossMovementMm = 0;
    if (m_records[row].type != QStringLiteral("圆弧半径") && !angleTrial && !holeTrial && !lengthTrial)
        validationError = QStringLiteral("当前已接入圆弧半径、角度、孔径和长度试测。其他类型算法尚未接入。");
    else if (crossFrameLengthTrial) {
        storeCurrentFrame();
        const ProjectFrame* startFrame = nullptr;
        const ProjectFrame* endFrame = nullptr;
        for (const ProjectFrame& frame : m_frames) {
            if (frame.id == m_records[row].frameId) startFrame = &frame;
            if (frame.id == m_records[row].secondaryFrameId) endFrame = &frame;
        }
        if (m_records[row].geometryId <= 0 || !startFrame)
            validationError = QStringLiteral("跨图长度起点图像或ROI尚未关联。");
        else if (m_records[row].secondaryGeometryId <= 0 || !endFrame)
            validationError = QStringLiteral("跨图长度终点图像或ROI尚未关联。");
        else if (startFrame->image.isNull() || endFrame->image.isNull())
            validationError = QStringLiteral("跨图长度的端点图像不可用。");
        else if (startFrame->image.size() != endFrame->image.size())
            validationError = QStringLiteral("跨图长度的两张端点图像尺寸不同，像素行坐标不能直接合成。");
        else if (!std::isfinite(m_records[row].lengthCalibration)
            || m_records[row].lengthCalibration <= 0)
            validationError = QStringLiteral("远心标定必须是大于0的有限数值。");
        else if ((startFrame->cameraIndex >= 0 && startFrame->cameraIndex != 0)
            || (endFrame->cameraIndex >= 0 && endFrame->cameraIndex != 0))
            validationError = QStringLiteral("跨图长度需要远心相机0的两张端点图像。");
        else {
            const GraphicalCanvas::FeatureSnapshot* startFeature = nullptr;
            const GraphicalCanvas::FeatureSnapshot* endFeature = nullptr;
            for (const auto& feature : startFrame->features)
                if (feature.id == m_records[row].geometryId) { startFeature = &feature; break; }
            for (const auto& feature : endFrame->features)
                if (feature.id == m_records[row].secondaryGeometryId) { endFeature = &feature; break; }
            if (!startFeature || !snapshotMeasurementRoi(*startFeature, crossStartRoi))
                validationError = QStringLiteral("跨图长度起点ROI已删除或不是有效矩形。");
            else if (!endFeature || !snapshotMeasurementRoi(*endFeature, crossEndRoi))
                validationError = QStringLiteral("跨图长度终点ROI已删除或不是有效矩形。");
            else {
                crossStartImage = startFrame->image;
                crossEndImage = endFrame->image;
                const auto axis5Encoder = [](const MeasurementRecord::DevicePosition& position,
                    double& encoder) {
                    if (!position.collected) return false;
                    for (const auto& axis : position.axes)
                        if (axis.axis == 5 && std::isfinite(axis.encoder)) {
                            encoder = axis.encoder;
                            return true;
                        }
                    return false;
                };
                double startEncoder = 0, endEncoder = 0;
                crossHasPositions = axis5Encoder(m_records[row].lengthStartPosition, startEncoder)
                    && axis5Encoder(m_records[row].lengthEndPosition, endEncoder);
                if (crossHasPositions)
                    crossMovementMm = axis5_compensation(endEncoder) - axis5_compensation(startEncoder);
            }
        }
    }
    else if (!m_canvas->hasImage())
        validationError = QStringLiteral("请先打开图像。");
    else if (m_records[row].frameId > 0 && m_records[row].frameId != m_currentFrameId)
        validationError = QStringLiteral("该记录的主ROI位于端点图像%1；请先在工具栏切换到对应图像。")
            .arg(m_records[row].frameId);
    else if (holeTrial && m_imageCameraIndex >= 0 && m_imageCameraIndex != 1)
        validationError = QStringLiteral("孔径试测需要测孔相机1图像；当前工程图像来自其他相机。");
    else if (lengthTrial && m_imageCameraIndex >= 0 && m_imageCameraIndex != 0)
        validationError = QStringLiteral("单图长度试测需要远心相机0图像；当前工程图像来自其他相机。");
    else if (m_records[row].geometryId <= 0)
        validationError = holeTrial ? QStringLiteral("孔径记录尚未关联ROI。")
            : (angleTrial || lengthTrial) ? QStringLiteral("记录尚未关联 ROI 1。")
            : QStringLiteral("当前记录尚未关联图形。请点击重新关联图形，再选择矩形或圆。");
    else if (doubleRoi && m_records[row].secondaryGeometryId <= 0)
        validationError = QStringLiteral("记录尚未关联 ROI 2。");
    else if (doubleRoi && m_records[row].geometryId == m_records[row].secondaryGeometryId)
        validationError = QStringLiteral("ROI 1 和 ROI 2 不能是同一个图形。");
    else if (m_canvas->featureProperties(m_records[row].geometryId).size() != 3)
        validationError = holeTrial ? QStringLiteral("孔径ROI已删除，请重新关联。")
            : (angleTrial || lengthTrial) ? QStringLiteral("ROI 1 已删除，请重新关联。")
            : QStringLiteral("关联图形已删除。请重新关联一个矩形或圆。");
    else if (doubleRoi && m_canvas->featureProperties(m_records[row].secondaryGeometryId).size() != 3)
        validationError = QStringLiteral("ROI 2 已删除，请重新关联。");
    else if (!m_canvas->measurementRoi(m_records[row].geometryId, roi))
        validationError = QStringLiteral("ROI须为矩形或圆：矩形宽高至少2px，圆半径至少1px。点、直线、圆弧尚不作为面积ROI支持。");
    else if (lengthTrial && roi.isCircle)
        validationError = QStringLiteral("长度模板须使用矩形ROI框住完整特征和两条目标边，不能使用圆形ROI。");
    else if (doubleRoi && !m_canvas->measurementRoi(m_records[row].secondaryGeometryId, secondaryRoi))
        validationError = QStringLiteral("ROI 2须为矩形或圆：矩形宽高至少2px，圆半径至少1px。");
    if (!validationError.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("不能试测"), validationError);
        return;
    }
    m_records[row].clearTrial(QStringLiteral("计算中"));
    refreshMeasurementRecords();
    m_trialRunning = true;
    centralWidget()->setEnabled(false);
    for (QToolBar* toolbar : findChildren<QToolBar*>())
        if (toolbar->objectName() != QStringLiteral("graphicalAxisSafetyBar")) toolbar->setEnabled(false);
    statusBar()->showMessage(holeTrial
        ? QStringLiteral("正在由孔径ROI自动定位上下测量区，并按原软件流程计算孔径；不作合格判定。")
        : crossFrameLengthTrial ? QStringLiteral("正在拟合跨图长度的起点边和终点边，并合成像素项与轴5补偿位移。")
        : lengthTrial ? QStringLiteral("正在执行单ROI模板长度试测：匹配完整特征，自动定位并拟合两条目标边；不作合格判定。")
        : single ? QStringLiteral("正在后台提取单ROI相邻边对；多候选时需选择目标边对。") : angleTrial
        ? QStringLiteral("正在后台分别拟合 ROI 1 和 ROI 2 的目标直线并计算夹角；不作合格判定。")
        : QStringLiteral("正在后台计算圆弧半径；使用列表中已提交的记录，未标定、不作合格判定。"));
    const QImage source = m_canvas->sourceImage();
    const GraphicalDetectionParameters parameters = m_records[row].detection;
    if (holeTrial) {
        const double calibration = m_records[row].holeCalibration;
        const auto result = std::make_shared<HoleTrialResult>();
        QThread* worker = QThread::create([source, roi, calibration, result]() {
            *result = runHoleDiameterTrial(source, roi, calibration);
        });
        connect(worker, &QThread::finished, this, [this, row, result]() {
            m_trialRunning = false;
            centralWidget()->setEnabled(true);
            for (QToolBar* toolbar : findChildren<QToolBar*>()) toolbar->setEnabled(true);
            if (row < m_records.size()) {
                m_records[row].trialLinearMm = result->diameterMm;
                m_records[row].trialStatus = result->status;
                m_records[row].detectedEdges = result->edges;
                m_records[row].fittedArc = result->fitted;
            }
            refreshMeasurementRecords();
            m_stepTable->setCurrentCell(row, 0);
            showRecordDetection(row);
            statusBar()->showMessage(result->status + (result->diameterMm > 0
                ? QStringLiteral("：%1 px × 标定 = %2 mm；未判定")
                    .arg(result->diameterPixels, 0, 'f', 3).arg(result->diameterMm, 0, 'f', 4)
                : QString()));
        });
        connect(worker, &QThread::finished, worker, &QObject::deleteLater);
        worker->start();
        return;
    }
    if (lengthTrial) {
        const double calibration = m_records[row].lengthCalibration;
        if (crossFrameLengthTrial) {
            const auto result = std::make_shared<CrossFrameLengthTrialResult>();
            QThread* worker = QThread::create([crossStartImage, crossStartRoi, crossEndImage,
                crossEndRoi, calibration, parameters, crossHasPositions, crossMovementMm, result]() {
                *result = runCrossFrameLengthTrial(crossStartImage, crossStartRoi, crossEndImage,
                    crossEndRoi, calibration, parameters, crossHasPositions, crossMovementMm);
            });
            connect(worker, &QThread::finished, this, [this, row, result]() {
                m_trialRunning = false;
                centralWidget()->setEnabled(true);
                for (QToolBar* toolbar : findChildren<QToolBar*>()) toolbar->setEnabled(true);
                if (row < m_records.size()) {
                    m_records[row].trialLinearMm = result->distanceMm;
                    m_records[row].trialStatus = result->status;
                    m_records[row].crossStartDetectedEdges = result->startEdges;
                    m_records[row].crossStartFittedLine = result->startFitted;
                    m_records[row].crossEndDetectedEdges = result->endEdges;
                    m_records[row].crossEndFittedLine = result->endFitted;
                }
                refreshMeasurementRecords();
                m_stepTable->setCurrentCell(row, 0);
                showRecordDetection(row);
                statusBar()->showMessage(result->status + (result->distanceMm > 0
                    ? QStringLiteral("；跨图长度=%1 mm").arg(result->distanceMm, 0, 'f', 4)
                    : QString()));
            });
            connect(worker, &QThread::finished, worker, &QObject::deleteLater);
            worker->start();
            return;
        }
        const QByteArray templateModel = m_records[row].lengthTemplateModel;
        const double referenceRow = m_records[row].lengthTemplateReferenceRow;
        const double referenceColumn = m_records[row].lengthTemplateReferenceColumn;
        const double referenceAngle = m_records[row].lengthTemplateReferenceAngle;
        const auto result = std::make_shared<LinearTrialResult>();
        QThread* worker = QThread::create([source, roi, calibration, parameters, templateModel,
            referenceRow, referenceColumn, referenceAngle, result]() {
            *result = runSingleRoiTemplateLengthTrial(source, roi, calibration, parameters,
                templateModel, referenceRow, referenceColumn, referenceAngle);
        });
        connect(worker, &QThread::finished, this, [this, row, result]() {
            m_trialRunning = false;
            centralWidget()->setEnabled(true);
            for (QToolBar* toolbar : findChildren<QToolBar*>()) toolbar->setEnabled(true);
            if (row < m_records.size()) {
                m_records[row].trialLinearMm = result->distanceMm;
                m_records[row].trialStatus = result->status;
                m_records[row].detectedEdges = result->edges;
                m_records[row].fittedArc = result->fitted;
                if (result->templateCreated && !result->templateModel.isEmpty()) {
                    m_records[row].lengthTemplateModel = result->templateModel;
                    m_records[row].lengthTemplateReferenceRow = result->templateReferenceRow;
                    m_records[row].lengthTemplateReferenceColumn = result->templateReferenceColumn;
                    m_records[row].lengthTemplateReferenceAngle = result->templateReferenceAngle;
                    m_projectDirty = true;
                }
            }
            refreshMeasurementRecords();
            m_stepTable->setCurrentCell(row, 0);
            showRecordDetection(row);
            statusBar()->showMessage(result->status + (result->distanceMm > 0
                ? QStringLiteral("：%1 px × 标定 = %2 mm；未判定")
                    .arg(result->distancePixels, 0, 'f', 3).arg(result->distanceMm, 0, 'f', 4)
                : QString()));
        });
        connect(worker, &QThread::finished, worker, &QObject::deleteLater);
        worker->start();
        return;
    }
    if (angleTrial) {
        const bool supplementary = m_records[row].useSupplementaryAngle;
        const auto result = std::make_shared<LineTrialResult>();
        QThread* worker = QThread::create([source, roi, secondaryRoi, supplementary, parameters, single, result]() {
            *result = single ? runSingleRoiAngleTrial(source, roi, supplementary, parameters)
                : runTwoRoiAngleTrial(source, roi, secondaryRoi, supplementary, parameters);
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
                m_records[row].cornerEdges = result->cornerEdges;
                m_records[row].cornerPairs = result->cornerPairs;
                m_records[row].cornerDiagnostic = result->diagnostic;
                m_records[row].selectedCornerPair = result->cornerPairs.size() == 1 && result->angle >= 0 ? 0 : -1;
                if (m_records[row].selectedCornerPair == 0) {
                    const auto& pair = result->cornerPairs[0];
                    m_records[row].candidateSelectionAuditMode = QStringLiteral("unique");
                    m_records[row].candidateSelectionAuditFirst = result->cornerEdges[pair.firstEdge].candidateId;
                    m_records[row].candidateSelectionAuditSecond = result->cornerEdges[pair.secondEdge].candidateId;
                }
                if (m_records[row].selectedCornerPair == 0) m_projectDirty = true;
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
    QThread* worker = QThread::create([source, roi, parameters, result]() { *result = runArcTrial(source, roi, parameters); });
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

static QByteArray projectFileSha256(const QString& filePath, QString& error)
{
    QFile input(filePath);
    if (!input.open(QIODevice::ReadOnly)) {
        error = QStringLiteral("无法读取工程引用文件：%1").arg(input.errorString());
        return QByteArray();
    }
    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (!input.atEnd()) {
        const QByteArray block = input.read(1024 * 1024);
        if (block.isEmpty() && input.error() != QFile::NoError) {
            error = QStringLiteral("读取工程引用文件失败：%1").arg(input.errorString());
            return QByteArray();
        }
        hash.addData(block);
    }
    return hash.result().toHex();
}

static bool isJsonInteger(const QJsonValue& value)
{
    if (!value.isDouble()) return false;
    const double number = value.toDouble();
    return std::isfinite(number) && std::floor(number) == number
        && number >= INT_MIN && number <= INT_MAX;
}

QStringList GraphicalProgramEditor::validateRecipeForExport() const
{
    QStringList issues;
    if (!m_recipeProgramNumber || m_recipeProgramNumber->value() <= 0)
        issues << QStringLiteral("请设置大于0的程序号。");
    if (!m_recipePartNumber || m_recipePartNumber->text().trimmed().isEmpty())
        issues << QStringLiteral("请填写零件图号。");
    if (!m_recipePartName || m_recipePartName->text().trimmed().isEmpty())
        issues << QStringLiteral("请填写零件名称。");
    if (!m_recipeProcessNumber || m_recipeProcessNumber->text().trimmed().isEmpty())
        issues << QStringLiteral("请填写工序号。");
    if (m_records.isEmpty()) {
        issues << QStringLiteral("至少需要一条测量记录。");
        return issues;
    }

    const auto frameHasFeature = [this](int frameId, int geometryId) {
        if (frameId <= 0 || geometryId <= 0) return false;
        if (frameId == m_currentFrameId && m_canvas) {
            for (const auto& feature : m_canvas->featureSnapshots())
                if (feature.id == geometryId) return true;
            return false;
        }
        for (const ProjectFrame& frame : m_frames) {
            if (frame.id != frameId) continue;
            for (const auto& feature : frame.features)
                if (feature.id == geometryId) return true;
            return false;
        }
        return false;
    };
    QSet<QString> featureNumbers;
    const QSet<QString> productionTypes = {
        QStringLiteral("直径"), QStringLiteral("孔径"),
        QStringLiteral("长度"), QStringLiteral("角度")
    };
    for (const MeasurementRecord& record : m_records) {
        const QString label = QStringLiteral("记录%1（%2）").arg(record.sequence).arg(record.type);
        const QString featureNumber = record.featureNumber.trimmed();
        if (featureNumber.isEmpty())
            issues << label + QStringLiteral("缺少特征号。");
        else {
            const QString key = featureNumber.toCaseFolded();
            if (featureNumbers.contains(key))
                issues << label + QStringLiteral("的特征号与其他记录重复：%1。").arg(featureNumber);
            featureNumbers.insert(key);
        }
        if (record.type == QStringLiteral("圆柱度") || record.type == QStringLiteral("跳动")) {
            if (record.lowerAxialOffsetPulse <= 0 || record.upperAxialOffsetPulse <= 0)
                issues << label + QStringLiteral("需要设置大于0的轴5下侧、上侧偏移量。");
        }
        if (!productionTypes.contains(record.type)) {
            issues << label + QStringLiteral("尚未接入生产程序映射。");
            continue;
        }
        if (record.type != QStringLiteral("直径")) {
            if (!frameHasFeature(record.frameId, record.geometryId))
                issues << label + QStringLiteral("的主ROI不存在或已删除。");
        }
        if (record.type == QStringLiteral("角度") && !record.singleRoiAngle
            && !frameHasFeature(record.secondaryFrameId, record.secondaryGeometryId))
            issues << label + QStringLiteral("的第二ROI不存在或已删除。");
        if (record.type == QStringLiteral("孔径")) {
            if (record.holeUniformCount <= 0)
                issues << label + QStringLiteral("未设置孔均布个数。");
            if (!std::isfinite(record.holeCalibration) || record.holeCalibration <= 0)
                issues << label + QStringLiteral("的测孔标定无效。");
        }
        if (record.type == QStringLiteral("长度")) {
            if (!std::isfinite(record.lengthCalibration) || record.lengthCalibration <= 0)
                issues << label + QStringLiteral("的远心标定无效。");
            if (record.crossFrameLength) {
                if (!frameHasFeature(record.secondaryFrameId, record.secondaryGeometryId))
                    issues << label + QStringLiteral("的终点ROI不存在或已删除。");
                if (!record.lengthStartPosition.collected || !record.lengthEndPosition.collected)
                    issues << label + QStringLiteral("需要采集起点和终点设备点位。");
            }
            else if (record.lengthTemplateModel.isEmpty())
                issues << label + QStringLiteral("尚未通过试测建立长度模板。");
        }
        if (!(record.type == QStringLiteral("长度") && record.crossFrameLength)
            && !record.devicePosition.collected)
            issues << label + QStringLiteral("尚未采集设备点位。");
        if (record.type == QStringLiteral("直径") && record.devicePosition.collected
            && !record.devicePosition.hasLightCurtainSample)
            issues << label + QStringLiteral("缺少有效光幕样本。");
    }
    return issues;
}

bool GraphicalProgramEditor::writeProject(const QString& filePath, QString& error)
{
    error.clear();
    storeCurrentFrame();
    if (!m_canvas->hasImage() || m_frames.isEmpty()) {
        error = QStringLiteral("当前配方没有可保存的图像。"); return false;
    }
    const QVector<GraphicalCanvas::FeatureSnapshot> featureSnapshots = m_canvas->featureSnapshots();
    if (!m_canvas->validateFeatureSnapshots(featureSnapshots, m_canvas->sourceImage().size(), error)) return false;

    struct PersistedFrame {
        int id = 0;
        QString absolutePath;
        QByteArray sha256;
    };
    QVector<PersistedFrame> persistedFrames;
    persistedFrames.reserve(m_frames.size());
    const QFileInfo projectInfo(filePath);
    const QDir projectDirectory(projectInfo.absolutePath());
    const QString assetDirectoryName = projectInfo.completeBaseName() + QStringLiteral(".assets");
    bool assetDirectoryReady = false;
    for (const ProjectFrame& frame : m_frames) {
        if (frame.id <= 0 || frame.image.isNull()) {
            error = QStringLiteral("配方图像数据不完整，无法保存。"); return false;
        }
        if (!m_canvas->validateFeatureSnapshots(frame.features, frame.image.size(), error)) return false;

        PersistedFrame persisted;
        persisted.id = frame.id;
        if (frame.cameraIndex >= 0) {
            if (!assetDirectoryReady) {
                if (!projectDirectory.mkpath(assetDirectoryName)) {
                    error = QStringLiteral("无法创建配方图像资源目录。"); return false;
                }
                assetDirectoryReady = true;
            }
            persisted.absolutePath = projectDirectory.filePath(assetDirectoryName
                + QStringLiteral("/frame_%1_camera_%2.png").arg(frame.id).arg(frame.cameraIndex));
            if (!writePngAtomically(frame.image, persisted.absolutePath, error)) {
                error = QStringLiteral("保存配方图像资源失败：%1").arg(error); return false;
            }
        }
        else {
            if (frame.filePath.isEmpty()) {
                error = QStringLiteral("图像%1没有可用的本地来源。").arg(frame.id); return false;
            }
            persisted.absolutePath = QFileInfo(frame.filePath).absoluteFilePath();
        }
        persisted.sha256 = projectFileSha256(persisted.absolutePath, error);
        if (persisted.sha256.isEmpty()) return false;
        if (frame.cameraIndex < 0 && !frame.fileSha256.isEmpty()
            && QString::fromLatin1(persisted.sha256).compare(frame.fileSha256, Qt::CaseInsensitive) != 0) {
            error = QStringLiteral("图像%1的源文件在打开后已被修改。").arg(frame.id); return false;
        }
        persistedFrames.append(persisted);
    }

    const ProjectFrame* currentFrame = nullptr;
    const PersistedFrame* currentPersistedFrame = nullptr;
    for (const ProjectFrame& frame : m_frames) {
        if (frame.id == m_currentFrameId) { currentFrame = &frame; break; }
    }
    for (const PersistedFrame& frame : persistedFrames) {
        if (frame.id == m_currentFrameId) { currentPersistedFrame = &frame; break; }
    }
    if (!currentFrame || !currentPersistedFrame) {
        error = QStringLiteral("当前图像不属于该配方。"); return false;
    }

    QJsonObject root;
    root[QStringLiteral("format")] = QStringLiteral("AxisMeasurement.GraphicalProject");
    root[QStringLiteral("version")] = 2;
    QJsonObject recipe;
    recipe[QStringLiteral("programNumber")] = m_recipeProgramNumber ? m_recipeProgramNumber->value() : 0;
    recipe[QStringLiteral("partNumber")] = m_recipePartNumber ? m_recipePartNumber->text().trimmed() : QString();
    recipe[QStringLiteral("partName")] = m_recipePartName ? m_recipePartName->text().trimmed() : QString();
    recipe[QStringLiteral("processNumber")] = m_recipeProcessNumber ? m_recipeProcessNumber->text().trimmed() : QString();
    recipe[QStringLiteral("note")] = m_recipeNote ? m_recipeNote->text().trimmed() : QString();
    root[QStringLiteral("recipe")] = recipe;
    QJsonObject imageObject;
    imageObject[QStringLiteral("path")] = projectDirectory.relativeFilePath(currentPersistedFrame->absolutePath);
    imageObject[QStringLiteral("width")] = m_canvas->sourceImage().width();
    imageObject[QStringLiteral("height")] = m_canvas->sourceImage().height();
    imageObject[QStringLiteral("sha256")] = QString::fromLatin1(currentPersistedFrame->sha256);
    imageObject[QStringLiteral("source")] = currentFrame->cameraIndex >= 0
        ? QStringLiteral("camera") : QStringLiteral("local");
    if (currentFrame->cameraIndex >= 0) {
        imageObject[QStringLiteral("cameraIndex")] = currentFrame->cameraIndex;
        imageObject[QStringLiteral("exposure")] = currentFrame->exposure;
    }
    root[QStringLiteral("image")] = imageObject;

    QJsonArray features;
    for (const auto& feature : featureSnapshots) {
        QJsonObject object;
        object[QStringLiteral("id")] = feature.id;
        object[QStringLiteral("type")] = feature.type;
        object[QStringLiteral("rotation")] = feature.rotation;
        object[QStringLiteral("width")] = feature.size.width();
        object[QStringLiteral("height")] = feature.size.height();
        QJsonArray points;
        for (const QPointF& point : feature.points) {
            QJsonArray coordinates; coordinates.append(point.x()); coordinates.append(point.y());
            points.append(coordinates);
        }
        object[QStringLiteral("points")] = points;
        features.append(object);
    }
    root[QStringLiteral("features")] = features;

    QJsonArray frameArray;
    for (const ProjectFrame& frame : m_frames) {
        const PersistedFrame* persisted = nullptr;
        for (const PersistedFrame& candidate : persistedFrames) {
            if (candidate.id == frame.id) { persisted = &candidate; break; }
        }
        if (!persisted) { error = QStringLiteral("配方图像索引不完整。"); return false; }
        QJsonObject frameObject;
        frameObject[QStringLiteral("id")] = frame.id;
        frameObject[QStringLiteral("path")] = projectDirectory.relativeFilePath(persisted->absolutePath);
        frameObject[QStringLiteral("width")] = frame.image.width();
        frameObject[QStringLiteral("height")] = frame.image.height();
        frameObject[QStringLiteral("sha256")] = QString::fromLatin1(persisted->sha256);
        frameObject[QStringLiteral("source")] = frame.cameraIndex >= 0 ? QStringLiteral("camera") : QStringLiteral("local");
        if (frame.cameraIndex >= 0) {
            frameObject[QStringLiteral("cameraIndex")] = frame.cameraIndex;
            frameObject[QStringLiteral("exposure")] = frame.exposure;
        }
        QJsonArray frameFeatures;
        for (const auto& feature : frame.features) {
            QJsonObject object;
            object[QStringLiteral("id")] = feature.id;
            object[QStringLiteral("type")] = feature.type;
            object[QStringLiteral("rotation")] = feature.rotation;
            object[QStringLiteral("width")] = feature.size.width();
            object[QStringLiteral("height")] = feature.size.height();
            QJsonArray points;
            for (const QPointF& point : feature.points) {
                QJsonArray coordinates; coordinates.append(point.x()); coordinates.append(point.y());
                points.append(coordinates);
            }
            object[QStringLiteral("points")] = points;
            frameFeatures.append(object);
        }
        frameObject[QStringLiteral("features")] = frameFeatures;
        frameArray.append(frameObject);
    }
    root[QStringLiteral("frames")] = frameArray;
    root[QStringLiteral("currentFrameId")] = m_currentFrameId;
    root[QStringLiteral("nextFrameId")] = m_nextFrameId;

    QJsonArray records;
    for (const MeasurementRecord& record : m_records) {
        QJsonObject object;
        object[QStringLiteral("sequence")] = record.sequence;
        object[QStringLiteral("frameId")] = record.frameId;
        object[QStringLiteral("geometryId")] = record.geometryId;
        object[QStringLiteral("secondaryFrameId")] = record.secondaryFrameId;
        object[QStringLiteral("secondaryGeometryId")] = record.secondaryGeometryId;
        object[QStringLiteral("featureNumber")] = record.featureNumber;
        object[QStringLiteral("type")] = record.type;
        object[QStringLiteral("holeUniformCount")] = record.holeUniformCount;
        if (record.type == QStringLiteral("孔径"))
            object[QStringLiteral("holeCalibrationMmPerPixel")] = record.holeCalibration;
        if ((record.type == QStringLiteral("圆柱度") || record.type == QStringLiteral("跳动"))
            && record.lowerAxialOffsetPulse > 0 && record.upperAxialOffsetPulse > 0) {
            object[QStringLiteral("lowerAxialOffsetPulse")] = record.lowerAxialOffsetPulse;
            object[QStringLiteral("upperAxialOffsetPulse")] = record.upperAxialOffsetPulse;
        }
        if (record.type == QStringLiteral("跳动")) {
            object[QStringLiteral("roundoutReference1")] = record.roundoutReference1;
            object[QStringLiteral("roundoutReference2")] = record.roundoutReference2;
        }
        if (record.type == QStringLiteral("长度")) {
            object[QStringLiteral("lengthCalibrationMmPerPixel")] = record.lengthCalibration;
            object[QStringLiteral("crossFrameLength")] = record.crossFrameLength;
            const auto endpointJson = [](const MeasurementRecord::DevicePosition& position) {
                QJsonObject endpoint;
                endpoint[QStringLiteral("status")] = position.collected
                    ? QStringLiteral("collected") : QStringLiteral("uncollected");
                endpoint[QStringLiteral("source")] = position.source;
                endpoint[QStringLiteral("unit")] = position.unit;
                if (position.collected) {
                    endpoint[QStringLiteral("capturedAtUtc")] = position.capturedAtUtc;
                    endpoint[QStringLiteral("cameraIndex")] = position.cameraIndex;
                    endpoint[QStringLiteral("exposure")] = position.exposure;
                    QJsonArray axes;
                    for (const auto& axis : position.axes) {
                        QJsonObject item;
                        item[QStringLiteral("axis")] = axis.axis;
                        item[QStringLiteral("planned")] = axis.planned;
                        item[QStringLiteral("encoder")] = axis.encoder;
                        axes.append(item);
                    }
                    endpoint[QStringLiteral("axes")] = axes;
                }
                return endpoint;
            };
            object[QStringLiteral("lengthStartPosition")] = endpointJson(record.lengthStartPosition);
            object[QStringLiteral("lengthEndPosition")] = endpointJson(record.lengthEndPosition);
            QJsonObject lengthTemplate;
            lengthTemplate[QStringLiteral("status")] = record.lengthTemplateModel.isEmpty()
                ? QStringLiteral("untrained") : QStringLiteral("trained");
            if (!record.lengthTemplateModel.isEmpty()) {
                lengthTemplate[QStringLiteral("encoding")] = QStringLiteral("halcon-shape-model-base64");
                lengthTemplate[QStringLiteral("data")] = QString::fromLatin1(record.lengthTemplateModel.toBase64());
                lengthTemplate[QStringLiteral("referenceRow")] = record.lengthTemplateReferenceRow;
                lengthTemplate[QStringLiteral("referenceColumn")] = record.lengthTemplateReferenceColumn;
                lengthTemplate[QStringLiteral("referenceAngle")] = record.lengthTemplateReferenceAngle;
            }
            object[QStringLiteral("lengthTemplate")] = lengthTemplate;
        }
        object[QStringLiteral("hasTolerance")] = record.hasTolerance;
        object[QStringLiteral("nominal")] = record.nominal;
        object[QStringLiteral("lower")] = record.lower;
        object[QStringLiteral("upper")] = record.upper;
        object[QStringLiteral("supplementaryAngle")] = record.useSupplementaryAngle;
        object[QStringLiteral("singleRoiAngle")] = record.singleRoiAngle;
        QJsonObject detection;
        detection[QStringLiteral("smoothing")] = record.detection.smoothing;
        detection[QStringLiteral("lowThreshold")] = record.detection.lowThreshold;
        detection[QStringLiteral("highThreshold")] = record.detection.highThreshold;
        detection[QStringLiteral("minLength")] = record.detection.minLength;
        detection[QStringLiteral("maxLength")] = record.detection.maxLength;
        detection[QStringLiteral("mergeDistance")] = record.detection.mergeDistance;
        detection[QStringLiteral("cornerMaxDeviation")] = record.detection.cornerMaxDeviation;
        detection[QStringLiteral("cornerMaxGap")] = record.detection.cornerMaxGap;
        object[QStringLiteral("detection")] = detection;
        QJsonObject selection;
        if (record.selectedCornerPair >= 0 && record.selectedCornerPair < record.cornerPairs.size()
            && record.cornerPairs[record.selectedCornerPair].firstEdge >= 0
            && record.cornerPairs[record.selectedCornerPair].firstEdge < record.cornerEdges.size()
            && record.cornerPairs[record.selectedCornerPair].secondEdge >= 0
            && record.cornerPairs[record.selectedCornerPair].secondEdge < record.cornerEdges.size()) {
            const auto& pair = record.cornerPairs[record.selectedCornerPair];
            selection[QStringLiteral("mode")] = record.cornerPairs.size() == 1
                ? QStringLiteral("unique") : QStringLiteral("manual");
            selection[QStringLiteral("firstCandidateId")] = record.cornerEdges[pair.firstEdge].candidateId;
            selection[QStringLiteral("secondCandidateId")] = record.cornerEdges[pair.secondEdge].candidateId;
        }
        else if ((record.candidateSelectionAuditMode == QStringLiteral("unique")
                || record.candidateSelectionAuditMode == QStringLiteral("manual"))
            && record.candidateSelectionAuditFirst > 0 && record.candidateSelectionAuditSecond > 0) {
            selection[QStringLiteral("mode")] = record.candidateSelectionAuditMode;
            selection[QStringLiteral("firstCandidateId")] = record.candidateSelectionAuditFirst;
            selection[QStringLiteral("secondCandidateId")] = record.candidateSelectionAuditSecond;
        }
        else selection[QStringLiteral("mode")] = QStringLiteral("none");
        selection[QStringLiteral("requiresRetest")] = true;
        object[QStringLiteral("candidateSelection")] = selection;
        QJsonObject devicePosition;
        const QVector<int> expectedAxes = deviceAxesForMeasurement(record.type);
        const int expectedCamera = deviceCameraForMeasurement(record.type);
        QVector<int> actualAxes;
        for (const auto& axis : record.devicePosition.axes) actualAxes.append(axis.axis);
        const bool validLightCurtain = record.type == QStringLiteral("直径")
            ? record.devicePosition.hasLightCurtainSample
                && std::isfinite(record.devicePosition.lightCurtainRawOut1)
                && std::isfinite(record.devicePosition.lightCurtainDiameter)
                && QDateTime::fromString(record.devicePosition.lightCurtainSampledAtUtc,
                    Qt::ISODateWithMs).isValid()
            : !record.devicePosition.hasLightCurtainSample;
        const bool validUncollected = !record.devicePosition.collected
            && record.devicePosition.source == QStringLiteral("none")
            && record.devicePosition.unit == QStringLiteral("pulse")
            && record.devicePosition.capturedAtUtc.isEmpty()
            && actualAxes.isEmpty() && record.devicePosition.cameraIndex == -1
            && record.devicePosition.exposure == -1
            && !record.devicePosition.hasLightCurtainSample;
        const bool validCollected = record.devicePosition.collected
            && record.devicePosition.source == QStringLiteral("hardware")
            && record.devicePosition.unit == QStringLiteral("pulse")
            && QDateTime::fromString(record.devicePosition.capturedAtUtc, Qt::ISODateWithMs).isValid()
            && actualAxes == expectedAxes
            && record.devicePosition.cameraIndex == expectedCamera
            && validLightCurtain
            && ((expectedCamera < 0 && record.devicePosition.exposure == -1)
                || (expectedCamera >= 0 && record.devicePosition.exposure >= 0
                    && record.devicePosition.exposure <= 30000));
        if (!validUncollected && !validCollected) {
            error = QStringLiteral("记录%1的设备点位与测量类型不匹配。").arg(record.sequence);
            return false;
        }
        devicePosition[QStringLiteral("status")] = record.devicePosition.collected
            ? QStringLiteral("collected") : QStringLiteral("uncollected");
        devicePosition[QStringLiteral("source")] = record.devicePosition.source;
        devicePosition[QStringLiteral("unit")] = record.devicePosition.unit;
        QJsonObject lightCurtain;
        lightCurtain[QStringLiteral("status")] = record.devicePosition.hasLightCurtainSample
            ? QStringLiteral("sampled") : QStringLiteral("none");
        if (record.devicePosition.hasLightCurtainSample) {
            lightCurtain[QStringLiteral("output")] = 1;
            lightCurtain[QStringLiteral("rawValue")] = record.devicePosition.lightCurtainRawOut1;
            lightCurtain[QStringLiteral("rawUnit")] = QStringLiteral("mm");
            lightCurtain[QStringLiteral("compensatedDiameter")] = record.devicePosition.lightCurtainDiameter;
            lightCurtain[QStringLiteral("compensatedUnit")] = QStringLiteral("mm");
            lightCurtain[QStringLiteral("sampledAtUtc")] = record.devicePosition.lightCurtainSampledAtUtc;
        }
        devicePosition[QStringLiteral("lightCurtain")] = lightCurtain;
        if (record.devicePosition.collected) {
            devicePosition[QStringLiteral("capturedAtUtc")] = record.devicePosition.capturedAtUtc;
            QJsonArray axes;
            for (const auto& axis : record.devicePosition.axes) {
                if (axis.axis < 1 || axis.axis > 8 || !std::isfinite(axis.planned)
                    || !std::isfinite(axis.encoder)) {
                    error = QStringLiteral("记录%1包含无效设备点位。").arg(record.sequence); return false;
                }
                QJsonObject axisObject;
                axisObject[QStringLiteral("axis")] = axis.axis;
                axisObject[QStringLiteral("planned")] = axis.planned;
                axisObject[QStringLiteral("encoder")] = axis.encoder;
                axes.append(axisObject);
            }
            devicePosition[QStringLiteral("axes")] = axes;
            devicePosition[QStringLiteral("cameraIndex")] = record.devicePosition.cameraIndex;
            devicePosition[QStringLiteral("exposure")] = record.devicePosition.exposure;
        }
        object[QStringLiteral("devicePosition")] = devicePosition;
        records.append(object);
    }
    root[QStringLiteral("records")] = records;
    root[QStringLiteral("nextRecordSequence")] = m_nextRecordSequence;

    QSaveFile output(filePath);
    if (!output.open(QIODevice::WriteOnly)) { error = output.errorString(); return false; }
    const QByteArray json = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (output.write(json) != json.size()) { error = output.errorString(); output.cancelWriting(); return false; }
    if (!output.commit()) { error = output.errorString(); return false; }
    for (ProjectFrame& frame : m_frames) {
        if (frame.cameraIndex < 0) continue;
        for (const PersistedFrame& persisted : persistedFrames) {
            if (persisted.id != frame.id) continue;
            frame.filePath = persisted.absolutePath;
            frame.fileSha256 = QString::fromLatin1(persisted.sha256);
            break;
        }
    }
    if (currentFrame->cameraIndex >= 0) {
        m_imageFilePath = currentPersistedFrame->absolutePath;
        m_imageFileSha256 = QString::fromLatin1(currentPersistedFrame->sha256);
    }
    return true;
}

bool GraphicalProgramEditor::readProject(const QString& filePath, QString& error)
{
    error.clear();
    QFile input(filePath);
    if (!input.open(QIODevice::ReadOnly)) { error = input.errorString(); return false; }
    if (input.size() > 20 * 1024 * 1024) { error = QStringLiteral("工程文件超过20 MB限制。"); return false; }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(input.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        error = QStringLiteral("JSON解析失败：%1").arg(parseError.errorString()); return false;
    }
    const QJsonObject root = document.object();
    const int projectVersion = root.value(QStringLiteral("version")).toInt(-1);
    if (root.value(QStringLiteral("format")).toString() != QStringLiteral("AxisMeasurement.GraphicalProject")
        || !isJsonInteger(root.value(QStringLiteral("version")))
        || (projectVersion != 1 && projectVersion != 2)) {
        error = QStringLiteral("不支持的工程格式或版本。"); return false;
    }
    if (!root.value(QStringLiteral("image")).isObject()
        || !root.value(QStringLiteral("features")).isArray()
        || !root.value(QStringLiteral("records")).isArray()
        || !isJsonInteger(root.value(QStringLiteral("nextRecordSequence")))
        || root.value(QStringLiteral("nextRecordSequence")).toInt() <= 0
        || root.value(QStringLiteral("nextRecordSequence")).toInt() == INT_MAX) {
        error = QStringLiteral("工程缺少图像、图形或测量记录数据。"); return false;
    }
    int recipeProgramNumber = 0;
    QString recipePartNumber, recipePartName, recipeProcessNumber, recipeNote;
    const QJsonValue recipeValue = root.value(QStringLiteral("recipe"));
    if (!recipeValue.isUndefined()) {
        if (!recipeValue.isObject()) {
            error = QStringLiteral("配方信息格式错误。"); return false;
        }
        const QJsonObject recipe = recipeValue.toObject();
        if (!isJsonInteger(recipe.value(QStringLiteral("programNumber")))
            || !recipe.value(QStringLiteral("partNumber")).isString()
            || !recipe.value(QStringLiteral("partName")).isString()
            || !recipe.value(QStringLiteral("processNumber")).isString()
            || !recipe.value(QStringLiteral("note")).isString()) {
            error = QStringLiteral("配方信息字段格式错误。"); return false;
        }
        recipeProgramNumber = recipe.value(QStringLiteral("programNumber")).toInt();
        recipePartNumber = recipe.value(QStringLiteral("partNumber")).toString().trimmed();
        recipePartName = recipe.value(QStringLiteral("partName")).toString().trimmed();
        recipeProcessNumber = recipe.value(QStringLiteral("processNumber")).toString().trimmed();
        recipeNote = recipe.value(QStringLiteral("note")).toString().trimmed();
        if (recipeProgramNumber < 0 || recipeProgramNumber > 50
            || recipePartNumber.size() > 128 || recipePartName.size() > 128
            || recipeProcessNumber.size() > 128 || recipeNote.size() > 128) {
            error = QStringLiteral("配方信息值超出允许范围。"); return false;
        }
    }
    const QJsonObject imageObject = root.value(QStringLiteral("image")).toObject();
    if (!imageObject.value(QStringLiteral("path")).isString()
        || !isJsonInteger(imageObject.value(QStringLiteral("width")))
        || !isJsonInteger(imageObject.value(QStringLiteral("height")))
        || !imageObject.value(QStringLiteral("sha256")).isString()) {
        error = QStringLiteral("工程图像信息格式错误。"); return false;
    }
    const QString storedImagePath = imageObject.value(QStringLiteral("path")).toString();
    const QString storedImageSha256 = imageObject.value(QStringLiteral("sha256")).toString();
    const QString imageSource = imageObject.value(QStringLiteral("source")).toString(QStringLiteral("local"));
    int imageCameraIndex = -1;
    int imageExposure = -1;
    if (imageSource != QStringLiteral("local") && imageSource != QStringLiteral("camera")) {
        error = QStringLiteral("工程图像来源无效。"); return false;
    }
    if (imageSource == QStringLiteral("camera")) {
        if (!isJsonInteger(imageObject.value(QStringLiteral("cameraIndex")))
            || !isJsonInteger(imageObject.value(QStringLiteral("exposure")))) {
            error = QStringLiteral("工程相机图像信息格式错误。"); return false;
        }
        imageCameraIndex = imageObject.value(QStringLiteral("cameraIndex")).toInt();
        imageExposure = imageObject.value(QStringLiteral("exposure")).toInt();
        if (imageCameraIndex < 0 || imageCameraIndex > 2 || imageExposure < 0 || imageExposure > 30000) {
            error = QStringLiteral("工程相机编号或曝光值无效。"); return false;
        }
    }
    if (storedImagePath.isEmpty()) { error = QStringLiteral("工程缺少图像路径。"); return false; }
    if (storedImageSha256.size() != 64) { error = QStringLiteral("工程图像校验值无效。"); return false; }
    const QString imagePath = QFileInfo(QDir(QFileInfo(filePath).absolutePath()).filePath(storedImagePath)).absoluteFilePath();
    const QByteArray currentImageSha256 = projectFileSha256(imagePath, error);
    if (currentImageSha256.isEmpty()) return false;
    if (QString::fromLatin1(currentImageSha256).compare(storedImageSha256, Qt::CaseInsensitive) != 0) {
        error = QStringLiteral("工程引用的图像内容已变化，当前工程保持不变。"); return false;
    }
    QImage image;
    if (!image.load(imagePath)) { error = QStringLiteral("无法读取工程图像：%1").arg(imagePath); return false; }
    if (image.width() != imageObject.value(QStringLiteral("width")).toInt()
        || image.height() != imageObject.value(QStringLiteral("height")).toInt()) {
        error = QStringLiteral("工程图像尺寸与保存时不一致，当前工程保持不变。"); return false;
    }

    const QJsonArray featureArray = root.value(QStringLiteral("features")).toArray();
    if (featureArray.size() > 10000) { error = QStringLiteral("工程图形数量超过10000。"); return false; }
    QVector<GraphicalCanvas::FeatureSnapshot> features;
    QSet<int> featureIds;
    for (const QJsonValue& value : featureArray) {
        if (!value.isObject()) { error = QStringLiteral("工程图形条目格式错误。"); return false; }
        const QJsonObject object = value.toObject();
        if (!isJsonInteger(object.value(QStringLiteral("id")))
            || !object.value(QStringLiteral("type")).isString()
            || !object.value(QStringLiteral("rotation")).isDouble()
            || !object.value(QStringLiteral("width")).isDouble()
            || !object.value(QStringLiteral("height")).isDouble()
            || !object.value(QStringLiteral("points")).isArray()) {
            error = QStringLiteral("工程图形条目字段格式错误。"); return false;
        }
        GraphicalCanvas::FeatureSnapshot feature;
        feature.id = object.value(QStringLiteral("id")).toInt();
        feature.type = object.value(QStringLiteral("type")).toString();
        feature.rotation = object.value(QStringLiteral("rotation")).toDouble();
        feature.size = QSizeF(object.value(QStringLiteral("width")).toDouble(), object.value(QStringLiteral("height")).toDouble());
        const QJsonArray points = object.value(QStringLiteral("points")).toArray();
        for (const QJsonValue& pointValue : points) {
            const QJsonArray point = pointValue.toArray();
            if (point.size() != 2 || !point[0].isDouble() || !point[1].isDouble()) {
                error = QStringLiteral("图形%1坐标格式错误。").arg(feature.id); return false;
            }
            feature.points.append(QPointF(point[0].toDouble(), point[1].toDouble()));
        }
        features.append(feature); featureIds.insert(feature.id);
    }
    if (!m_canvas->validateFeatureSnapshots(features, image.size(), error)) return false;

    QVector<ProjectFrame> frames;
    int currentFrameId = 1;
    int nextFrameId = 2;
    if (projectVersion == 1) {
        ProjectFrame frame;
        frame.id = 1; frame.filePath = imagePath;
        frame.fileSha256 = QString::fromLatin1(currentImageSha256);
        frame.cameraIndex = imageCameraIndex; frame.exposure = imageExposure;
        frame.image = image; frame.features = features;
        frames.append(frame);
    }
    else {
        if (!root.value(QStringLiteral("frames")).isArray()
            || !isJsonInteger(root.value(QStringLiteral("currentFrameId")))
            || !isJsonInteger(root.value(QStringLiteral("nextFrameId")))) {
            error = QStringLiteral("双帧工程缺少帧集合信息。"); return false;
        }
        const QJsonArray storedFrames = root.value(QStringLiteral("frames")).toArray();
        if (storedFrames.isEmpty() || storedFrames.size() > 100) {
            error = QStringLiteral("工程帧数量无效。"); return false;
        }
        QSet<int> frameIds;
        for (const QJsonValue& frameValue : storedFrames) {
            if (!frameValue.isObject()) { error = QStringLiteral("工程帧条目格式错误。"); return false; }
            const QJsonObject frameObject = frameValue.toObject();
            if (!isJsonInteger(frameObject.value(QStringLiteral("id")))
                || !frameObject.value(QStringLiteral("path")).isString()
                || !isJsonInteger(frameObject.value(QStringLiteral("width")))
                || !isJsonInteger(frameObject.value(QStringLiteral("height")))
                || !frameObject.value(QStringLiteral("sha256")).isString()
                || !frameObject.value(QStringLiteral("features")).isArray()) {
                error = QStringLiteral("工程帧字段格式错误。"); return false;
            }
            ProjectFrame frame;
            frame.id = frameObject.value(QStringLiteral("id")).toInt();
            if (frame.id <= 0 || frameIds.contains(frame.id)) {
                error = QStringLiteral("工程帧编号无效或重复。"); return false;
            }
            frameIds.insert(frame.id);
            frame.filePath = QFileInfo(QDir(QFileInfo(filePath).absolutePath())
                .filePath(frameObject.value(QStringLiteral("path")).toString())).absoluteFilePath();
            const QByteArray frameHash = projectFileSha256(frame.filePath, error);
            if (frameHash.isEmpty()) return false;
            if (QString::fromLatin1(frameHash).compare(frameObject.value(QStringLiteral("sha256")).toString(),
                    Qt::CaseInsensitive) != 0) {
                error = QStringLiteral("帧%1图像内容已变化。").arg(frame.id); return false;
            }
            if (!frame.image.load(frame.filePath)
                || frame.image.width() != frameObject.value(QStringLiteral("width")).toInt()
                || frame.image.height() != frameObject.value(QStringLiteral("height")).toInt()) {
                error = QStringLiteral("帧%1图像无法读取或尺寸不符。").arg(frame.id); return false;
            }
            frame.fileSha256 = QString::fromLatin1(frameHash);
            const QString source = frameObject.value(QStringLiteral("source")).toString(QStringLiteral("local"));
            if (source == QStringLiteral("camera")) {
                if (!isJsonInteger(frameObject.value(QStringLiteral("cameraIndex")))
                    || !isJsonInteger(frameObject.value(QStringLiteral("exposure")))) {
                    error = QStringLiteral("帧%1相机字段无效。").arg(frame.id); return false;
                }
                frame.cameraIndex = frameObject.value(QStringLiteral("cameraIndex")).toInt();
                frame.exposure = frameObject.value(QStringLiteral("exposure")).toInt();
                if (frame.cameraIndex < 0 || frame.cameraIndex > 2
                    || frame.exposure < 0 || frame.exposure > 30000) {
                    error = QStringLiteral("帧%1相机值无效。").arg(frame.id); return false;
                }
            }
            else if (source != QStringLiteral("local")) {
                error = QStringLiteral("帧%1来源无效。").arg(frame.id); return false;
            }
            QSet<int> ids;
            for (const QJsonValue& featureValue : frameObject.value(QStringLiteral("features")).toArray()) {
                if (!featureValue.isObject()) { error = QStringLiteral("帧%1图形格式错误。").arg(frame.id); return false; }
                const QJsonObject object = featureValue.toObject();
                if (!isJsonInteger(object.value(QStringLiteral("id")))
                    || !object.value(QStringLiteral("type")).isString()
                    || !object.value(QStringLiteral("rotation")).isDouble()
                    || !object.value(QStringLiteral("width")).isDouble()
                    || !object.value(QStringLiteral("height")).isDouble()
                    || !object.value(QStringLiteral("points")).isArray()) {
                    error = QStringLiteral("帧%1图形字段错误。").arg(frame.id); return false;
                }
                GraphicalCanvas::FeatureSnapshot feature;
                feature.id = object.value(QStringLiteral("id")).toInt();
                if (ids.contains(feature.id)) { error = QStringLiteral("帧%1图形编号重复。").arg(frame.id); return false; }
                ids.insert(feature.id);
                feature.type = object.value(QStringLiteral("type")).toString();
                feature.rotation = object.value(QStringLiteral("rotation")).toDouble();
                feature.size = QSizeF(object.value(QStringLiteral("width")).toDouble(), object.value(QStringLiteral("height")).toDouble());
                for (const QJsonValue& pointValue : object.value(QStringLiteral("points")).toArray()) {
                    const QJsonArray point = pointValue.toArray();
                    if (point.size() != 2 || !point[0].isDouble() || !point[1].isDouble()) {
                        error = QStringLiteral("帧%1图形坐标错误。").arg(frame.id); return false;
                    }
                    feature.points.append(QPointF(point[0].toDouble(), point[1].toDouble()));
                }
                frame.features.append(feature);
            }
            if (!m_canvas->validateFeatureSnapshots(frame.features, frame.image.size(), error)) return false;
            frames.append(frame);
        }
        currentFrameId = root.value(QStringLiteral("currentFrameId")).toInt();
        nextFrameId = root.value(QStringLiteral("nextFrameId")).toInt();
        if (!frameIds.contains(currentFrameId) || nextFrameId <= 0
            || frameIds.contains(nextFrameId)) {
            error = QStringLiteral("当前帧或下一帧编号无效。"); return false;
        }
        const ProjectFrame* current = nullptr;
        for (const ProjectFrame& frame : frames) if (frame.id == currentFrameId) { current = &frame; break; }
        image = current->image; features = current->features;
        imageCameraIndex = current->cameraIndex; imageExposure = current->exposure;
    }

    const QJsonArray recordArray = root.value(QStringLiteral("records")).toArray();
    if (recordArray.size() > 10000) { error = QStringLiteral("工程测量记录超过10000。"); return false; }
    QVector<MeasurementRecord> records;
    QSet<int> sequences;
    int nextSequence = 1;
    for (const QJsonValue& value : recordArray) {
        if (!value.isObject()) { error = QStringLiteral("测量记录格式错误。"); return false; }
        const QJsonObject object = value.toObject();
        if (!isJsonInteger(object.value(QStringLiteral("sequence")))
            || (projectVersion == 2 && (!isJsonInteger(object.value(QStringLiteral("frameId")))
                || !isJsonInteger(object.value(QStringLiteral("secondaryFrameId")))))
            || !isJsonInteger(object.value(QStringLiteral("geometryId")))
            || !isJsonInteger(object.value(QStringLiteral("secondaryGeometryId")))
            || !object.value(QStringLiteral("featureNumber")).isString()
            || !object.value(QStringLiteral("type")).isString()
            || !object.value(QStringLiteral("hasTolerance")).isBool()
            || !object.value(QStringLiteral("nominal")).isDouble()
            || !object.value(QStringLiteral("lower")).isDouble()
            || !object.value(QStringLiteral("upper")).isDouble()
            || !object.value(QStringLiteral("supplementaryAngle")).isBool()
            || !object.value(QStringLiteral("singleRoiAngle")).isBool()
            || !object.value(QStringLiteral("detection")).isObject()
            || !object.value(QStringLiteral("candidateSelection")).isObject()
            || !object.value(QStringLiteral("devicePosition")).isObject()) {
            error = QStringLiteral("测量记录字段格式错误。"); return false;
        }
        MeasurementRecord record;
        record.sequence = object.value(QStringLiteral("sequence")).toInt();
        record.frameId = projectVersion == 1 ? 1 : object.value(QStringLiteral("frameId")).toInt();
        record.geometryId = object.value(QStringLiteral("geometryId")).toInt(-1);
        record.secondaryFrameId = projectVersion == 1 ? 1
            : object.value(QStringLiteral("secondaryFrameId")).toInt();
        record.secondaryGeometryId = object.value(QStringLiteral("secondaryGeometryId")).toInt(-1);
        record.featureNumber = object.value(QStringLiteral("featureNumber")).toString();
        record.type = object.value(QStringLiteral("type")).toString();
        const QJsonValue holeUniformCount = object.value(QStringLiteral("holeUniformCount"));
        if (!holeUniformCount.isUndefined()
            && (!isJsonInteger(holeUniformCount) || holeUniformCount.toInt() < 0
                || holeUniformCount.toInt() > 999)) {
            error = QStringLiteral("测量记录%1的孔均布个数无效。").arg(record.sequence); return false;
        }
        record.holeUniformCount = holeUniformCount.toInt(0);
        const QJsonValue lowerAxialOffset = object.value(QStringLiteral("lowerAxialOffsetPulse"));
        const QJsonValue upperAxialOffset = object.value(QStringLiteral("upperAxialOffsetPulse"));
        const QJsonValue roundoutReference1 = object.value(QStringLiteral("roundoutReference1"));
        const QJsonValue roundoutReference2 = object.value(QStringLiteral("roundoutReference2"));
        const bool axialScanType = record.type == QStringLiteral("圆柱度")
            || record.type == QStringLiteral("跳动");
        if ((!lowerAxialOffset.isUndefined()
                && (!isJsonInteger(lowerAxialOffset) || lowerAxialOffset.toInt() < 0
                    || lowerAxialOffset.toInt() > 100000000))
            || (!upperAxialOffset.isUndefined()
                && (!isJsonInteger(upperAxialOffset) || upperAxialOffset.toInt() < 0
                    || upperAxialOffset.toInt() > 100000000))
            || (!roundoutReference1.isUndefined()
                && (!roundoutReference1.isString() || roundoutReference1.toString().size() > 128))
            || (!roundoutReference2.isUndefined()
                && (!roundoutReference2.isString() || roundoutReference2.toString().size() > 128))) {
            error = QStringLiteral("测量记录%1的轴向扫描配置无效。").arg(record.sequence); return false;
        }
        record.lowerAxialOffsetPulse = lowerAxialOffset.toInt(0);
        record.upperAxialOffsetPulse = upperAxialOffset.toInt(0);
        record.roundoutReference1 = roundoutReference1.toString().trimmed();
        record.roundoutReference2 = roundoutReference2.toString().trimmed();
        const QJsonValue holeCalibration = object.value(QStringLiteral("holeCalibrationMmPerPixel"));
        if (!holeCalibration.isUndefined()
            && (!holeCalibration.isDouble() || !std::isfinite(holeCalibration.toDouble())
                || holeCalibration.toDouble() <= 0 || holeCalibration.toDouble() > 1)) {
            error = QStringLiteral("测量记录%1的测孔标定系数无效。").arg(record.sequence); return false;
        }
        record.holeCalibration = holeCalibration.toDouble(0.00691842);
        const QJsonValue lengthCalibration = object.value(QStringLiteral("lengthCalibrationMmPerPixel"));
        if (!lengthCalibration.isUndefined()
            && (!lengthCalibration.isDouble() || !std::isfinite(lengthCalibration.toDouble())
                || lengthCalibration.toDouble() <= 0 || lengthCalibration.toDouble() > 1)) {
            error = QStringLiteral("测量记录%1的远心标定系数无效。").arg(record.sequence); return false;
        }
        record.lengthCalibration = lengthCalibration.toDouble(0.01218603);
        const QJsonValue crossFrameLength = object.value(QStringLiteral("crossFrameLength"));
        if (!crossFrameLength.isUndefined() && !crossFrameLength.isBool()) {
            error = QStringLiteral("测量记录%1的跨图长度模式无效。").arg(record.sequence); return false;
        }
        record.crossFrameLength = crossFrameLength.toBool(false);
        const auto readLengthEndpoint = [&error, &record](const QJsonValue& value,
            MeasurementRecord::DevicePosition& position, const QString& name) {
            if (value.isUndefined()) return true;
            if (!value.isObject()) {
                error = QStringLiteral("测量记录%1的%2点位格式错误。").arg(record.sequence).arg(name); return false;
            }
            const QJsonObject endpoint = value.toObject();
            const QString status = endpoint.value(QStringLiteral("status")).toString();
            if (endpoint.value(QStringLiteral("unit")).toString() != QStringLiteral("pulse")
                || (status != QStringLiteral("collected") && status != QStringLiteral("uncollected"))) {
                error = QStringLiteral("测量记录%1的%2点位状态错误。").arg(record.sequence).arg(name); return false;
            }
            if (status == QStringLiteral("uncollected")) {
                if (endpoint.value(QStringLiteral("source")).toString() != QStringLiteral("none")) {
                    error = QStringLiteral("测量记录%1的%2未采集点位来源错误。").arg(record.sequence).arg(name); return false;
                }
                return true;
            }
            if (endpoint.value(QStringLiteral("source")).toString() != QStringLiteral("hardware")
                || !endpoint.value(QStringLiteral("capturedAtUtc")).isString()
                || !isJsonInteger(endpoint.value(QStringLiteral("cameraIndex")))
                || !isJsonInteger(endpoint.value(QStringLiteral("exposure")))
                || !endpoint.value(QStringLiteral("axes")).isArray()) {
                error = QStringLiteral("测量记录%1的%2硬件点位字段错误。").arg(record.sequence).arg(name); return false;
            }
            const QJsonArray axes = endpoint.value(QStringLiteral("axes")).toArray();
            if (axes.size() != 1) {
                error = QStringLiteral("测量记录%1的%2必须且只能包含轴5点位。").arg(record.sequence).arg(name); return false;
            }
            const QJsonObject axisObject = axes.at(0).toObject();
            if (!isJsonInteger(axisObject.value(QStringLiteral("axis")))
                || axisObject.value(QStringLiteral("axis")).toInt() != 5
                || !axisObject.value(QStringLiteral("planned")).isDouble()
                || !axisObject.value(QStringLiteral("encoder")).isDouble()) {
                error = QStringLiteral("测量记录%1的%2轴5点位无效。").arg(record.sequence).arg(name); return false;
            }
            MeasurementRecord::AxisPosition axis;
            axis.axis = 5;
            axis.planned = axisObject.value(QStringLiteral("planned")).toDouble();
            axis.encoder = axisObject.value(QStringLiteral("encoder")).toDouble();
            position.collected = true;
            position.source = QStringLiteral("hardware");
            position.capturedAtUtc = endpoint.value(QStringLiteral("capturedAtUtc")).toString();
            position.cameraIndex = endpoint.value(QStringLiteral("cameraIndex")).toInt();
            position.exposure = endpoint.value(QStringLiteral("exposure")).toInt();
            position.axes.append(axis);
            if (!std::isfinite(axis.planned) || !std::isfinite(axis.encoder)
                || position.cameraIndex != 0 || position.exposure < 0 || position.exposure > 30000
                || !QDateTime::fromString(position.capturedAtUtc, Qt::ISODateWithMs).isValid()) {
                error = QStringLiteral("测量记录%1的%2点位值无效。").arg(record.sequence).arg(name); return false;
            }
            return true;
        };
        if (!readLengthEndpoint(object.value(QStringLiteral("lengthStartPosition")),
                record.lengthStartPosition, QStringLiteral("起点"))
            || !readLengthEndpoint(object.value(QStringLiteral("lengthEndPosition")),
                record.lengthEndPosition, QStringLiteral("终点"))) return false;
        const QJsonValue lengthTemplateValue = object.value(QStringLiteral("lengthTemplate"));
        if (!lengthTemplateValue.isUndefined()) {
            if (record.type != QStringLiteral("长度") || !lengthTemplateValue.isObject()) {
                error = QStringLiteral("测量记录%1的长度模板字段无效。").arg(record.sequence); return false;
            }
            const QJsonObject lengthTemplate = lengthTemplateValue.toObject();
            const QString templateStatus = lengthTemplate.value(QStringLiteral("status")).toString();
            if (templateStatus == QStringLiteral("trained")) {
                const QJsonValue templateData = lengthTemplate.value(QStringLiteral("data"));
                if (lengthTemplate.value(QStringLiteral("encoding")).toString()
                        != QStringLiteral("halcon-shape-model-base64")
                    || !templateData.isString()
                    || templateData.toString().toLatin1().size() > 6 * 1024 * 1024
                    || !lengthTemplate.value(QStringLiteral("referenceRow")).isDouble()
                    || !lengthTemplate.value(QStringLiteral("referenceColumn")).isDouble()
                    || !lengthTemplate.value(QStringLiteral("referenceAngle")).isDouble()) {
                    error = QStringLiteral("测量记录%1的长度模板格式错误。").arg(record.sequence); return false;
                }
                const QByteArray encoded = templateData.toString().toLatin1();
                const QByteArray decoded = QByteArray::fromBase64(encoded);
                record.lengthTemplateReferenceRow = lengthTemplate.value(QStringLiteral("referenceRow")).toDouble();
                record.lengthTemplateReferenceColumn = lengthTemplate.value(QStringLiteral("referenceColumn")).toDouble();
                record.lengthTemplateReferenceAngle = lengthTemplate.value(QStringLiteral("referenceAngle")).toDouble();
                if (decoded.isEmpty() || decoded.size() > 4 * 1024 * 1024
                    || decoded.toBase64() != encoded
                    || !std::isfinite(record.lengthTemplateReferenceRow)
                    || !std::isfinite(record.lengthTemplateReferenceColumn)
                    || !std::isfinite(record.lengthTemplateReferenceAngle)) {
                    error = QStringLiteral("测量记录%1的长度模板数据无效。").arg(record.sequence); return false;
                }
                record.lengthTemplateModel = decoded;
            }
            else if (templateStatus != QStringLiteral("untrained")) {
                error = QStringLiteral("测量记录%1的长度模板状态无效。").arg(record.sequence); return false;
            }
        }
        record.hasTolerance = object.value(QStringLiteral("hasTolerance")).toBool();
        record.nominal = object.value(QStringLiteral("nominal")).toDouble();
        record.lower = object.value(QStringLiteral("lower")).toDouble();
        record.upper = object.value(QStringLiteral("upper")).toDouble();
        record.useSupplementaryAngle = object.value(QStringLiteral("supplementaryAngle")).toBool();
        record.singleRoiAngle = object.value(QStringLiteral("singleRoiAngle")).toBool();
        const QJsonObject detection = object.value(QStringLiteral("detection")).toObject();
        if (!detection.value(QStringLiteral("smoothing")).isDouble()
            || !detection.value(QStringLiteral("lowThreshold")).isDouble()
            || !detection.value(QStringLiteral("highThreshold")).isDouble()
            || !detection.value(QStringLiteral("minLength")).isDouble()
            || !detection.value(QStringLiteral("maxLength")).isDouble()
            || !detection.value(QStringLiteral("mergeDistance")).isDouble()
            || !detection.value(QStringLiteral("cornerMaxDeviation")).isDouble()
            || !detection.value(QStringLiteral("cornerMaxGap")).isDouble()) {
            error = QStringLiteral("测量记录%1的检测参数格式错误。").arg(record.sequence); return false;
        }
        const QJsonObject selection = object.value(QStringLiteral("candidateSelection")).toObject();
        const QJsonObject devicePosition = object.value(QStringLiteral("devicePosition")).toObject();
        const QString selectionMode = selection.value(QStringLiteral("mode")).toString();
        const QString deviceStatus = devicePosition.value(QStringLiteral("status")).toString();
        if (!selection.value(QStringLiteral("mode")).isString()
            || !selection.value(QStringLiteral("requiresRetest")).isBool()
            || !selection.value(QStringLiteral("requiresRetest")).toBool()
            || (selectionMode != QStringLiteral("none") && selectionMode != QStringLiteral("unique")
                && selectionMode != QStringLiteral("manual"))
            || (selectionMode != QStringLiteral("none")
                && (!isJsonInteger(selection.value(QStringLiteral("firstCandidateId")))
                    || !isJsonInteger(selection.value(QStringLiteral("secondCandidateId")))
                    || selection.value(QStringLiteral("firstCandidateId")).toInt() <= 0
                    || selection.value(QStringLiteral("secondCandidateId")).toInt() <= 0))
            || (deviceStatus != QStringLiteral("uncollected") && deviceStatus != QStringLiteral("collected"))
            || devicePosition.value(QStringLiteral("unit")).toString() != QStringLiteral("pulse")
            || (deviceStatus == QStringLiteral("uncollected")
                && devicePosition.value(QStringLiteral("source")).toString() != QStringLiteral("none"))
            || (deviceStatus == QStringLiteral("collected")
                && devicePosition.value(QStringLiteral("source")).toString() != QStringLiteral("hardware"))) {
            error = QStringLiteral("测量记录%1的候选或设备点位状态无效。").arg(record.sequence); return false;
        }
        const QJsonValue lightCurtainValue = devicePosition.value(QStringLiteral("lightCurtain"));
        if (!lightCurtainValue.isUndefined()) {
            if (!lightCurtainValue.isObject()) {
                error = QStringLiteral("测量记录%1的光幕样本格式错误。").arg(record.sequence); return false;
            }
            const QJsonObject lightCurtain = lightCurtainValue.toObject();
            const QString sensorStatus = lightCurtain.value(QStringLiteral("status")).toString();
            if (sensorStatus == QStringLiteral("sampled")) {
                if (!isJsonInteger(lightCurtain.value(QStringLiteral("output")))
                    || lightCurtain.value(QStringLiteral("output")).toInt() != 1
                    || !lightCurtain.value(QStringLiteral("rawValue")).isDouble()
                    || lightCurtain.value(QStringLiteral("rawUnit")).toString() != QStringLiteral("mm")
                    || !lightCurtain.value(QStringLiteral("compensatedDiameter")).isDouble()
                    || lightCurtain.value(QStringLiteral("compensatedUnit")).toString() != QStringLiteral("mm")
                    || !lightCurtain.value(QStringLiteral("sampledAtUtc")).isString()) {
                    error = QStringLiteral("测量记录%1的光幕样本字段无效。").arg(record.sequence); return false;
                }
                record.devicePosition.hasLightCurtainSample = true;
                record.devicePosition.lightCurtainRawOut1 = lightCurtain.value(QStringLiteral("rawValue")).toDouble();
                record.devicePosition.lightCurtainDiameter = lightCurtain.value(QStringLiteral("compensatedDiameter")).toDouble();
                record.devicePosition.lightCurtainSampledAtUtc = lightCurtain.value(QStringLiteral("sampledAtUtc")).toString();
                if (!std::isfinite(record.devicePosition.lightCurtainRawOut1)
                    || !std::isfinite(record.devicePosition.lightCurtainDiameter)
                    || !QDateTime::fromString(record.devicePosition.lightCurtainSampledAtUtc,
                        Qt::ISODateWithMs).isValid()) {
                    error = QStringLiteral("测量记录%1的光幕样本值无效。").arg(record.sequence); return false;
                }
            }
            else if (sensorStatus != QStringLiteral("none")) {
                error = QStringLiteral("测量记录%1的光幕样本状态无效。").arg(record.sequence); return false;
            }
        }
        if (deviceStatus == QStringLiteral("collected")) {
            if (!devicePosition.value(QStringLiteral("axes")).isArray()
                || !devicePosition.value(QStringLiteral("capturedAtUtc")).isString()
                || !isJsonInteger(devicePosition.value(QStringLiteral("cameraIndex")))
                || !isJsonInteger(devicePosition.value(QStringLiteral("exposure")))) {
                error = QStringLiteral("测量记录%1的设备点位字段格式错误。").arg(record.sequence); return false;
            }
            const QJsonArray axes = devicePosition.value(QStringLiteral("axes")).toArray();
            if (axes.isEmpty() || axes.size() > 8) {
                error = QStringLiteral("测量记录%1的轴点位数量无效。").arg(record.sequence); return false;
            }
            QSet<int> axisIds;
            for (const QJsonValue& axisValue : axes) {
                if (!axisValue.isObject()) {
                    error = QStringLiteral("测量记录%1的轴点位格式错误。").arg(record.sequence); return false;
                }
                const QJsonObject axisObject = axisValue.toObject();
                if (!isJsonInteger(axisObject.value(QStringLiteral("axis")))
                    || !axisObject.value(QStringLiteral("planned")).isDouble()
                    || !axisObject.value(QStringLiteral("encoder")).isDouble()) {
                    error = QStringLiteral("测量记录%1的轴点位字段格式错误。").arg(record.sequence); return false;
                }
                MeasurementRecord::AxisPosition axis;
                axis.axis = axisObject.value(QStringLiteral("axis")).toInt();
                axis.planned = axisObject.value(QStringLiteral("planned")).toDouble();
                axis.encoder = axisObject.value(QStringLiteral("encoder")).toDouble();
                if (axis.axis < 1 || axis.axis > 8 || axisIds.contains(axis.axis)
                    || !std::isfinite(axis.planned) || !std::isfinite(axis.encoder)) {
                    error = QStringLiteral("测量记录%1的轴点位值无效。").arg(record.sequence); return false;
                }
                axisIds.insert(axis.axis);
                record.devicePosition.axes.append(axis);
            }
            record.devicePosition.collected = true;
            record.devicePosition.source = QStringLiteral("hardware");
            record.devicePosition.capturedAtUtc = devicePosition.value(QStringLiteral("capturedAtUtc")).toString();
            record.devicePosition.cameraIndex = devicePosition.value(QStringLiteral("cameraIndex")).toInt();
            record.devicePosition.exposure = devicePosition.value(QStringLiteral("exposure")).toInt();
            if (!QDateTime::fromString(record.devicePosition.capturedAtUtc, Qt::ISODateWithMs).isValid()
                || record.devicePosition.cameraIndex < -1 || record.devicePosition.cameraIndex > 2
                || record.devicePosition.exposure < -1 || record.devicePosition.exposure > 30000
                || (record.devicePosition.cameraIndex < 0 && record.devicePosition.exposure != -1)
                || (record.devicePosition.cameraIndex >= 0 && record.devicePosition.exposure < 0)) {
                error = QStringLiteral("测量记录%1的相机点位值无效。").arg(record.sequence); return false;
            }
        }
        record.detection.smoothing = detection.value(QStringLiteral("smoothing")).toDouble();
        record.detection.lowThreshold = detection.value(QStringLiteral("lowThreshold")).toDouble();
        record.detection.highThreshold = detection.value(QStringLiteral("highThreshold")).toDouble();
        record.detection.minLength = detection.value(QStringLiteral("minLength")).toDouble();
        record.detection.maxLength = detection.value(QStringLiteral("maxLength")).toDouble();
        record.detection.mergeDistance = detection.value(QStringLiteral("mergeDistance")).toDouble();
        record.detection.cornerMaxDeviation = detection.value(QStringLiteral("cornerMaxDeviation")).toDouble();
        record.detection.cornerMaxGap = detection.value(QStringLiteral("cornerMaxGap")).toDouble();
        // A per-edge ROI from the earlier two-ROI baseline does not have the
        // same meaning as the formal full-feature template ROI. Preserve the
        // record, but require an explicit new association instead of silently
        // treating one old edge as a complete template.
        const bool migratedLegacyLength = record.type == QStringLiteral("长度")
            && record.secondaryGeometryId > 0;
        if (migratedLegacyLength) {
            record.geometryId = -1;
            record.secondaryGeometryId = -1;
        }
        const bool invalidAngleReferences = record.type == QStringLiteral("角度")
            && ((record.singleRoiAngle && record.secondaryGeometryId > 0)
                || (!record.singleRoiAngle && record.geometryId > 0
                    && record.secondaryGeometryId > 0
                    && (record.frameId != record.secondaryFrameId
                        || record.geometryId == record.secondaryGeometryId)));
        const auto frameHasFeature = [&frames](int frameId, int geometryId) {
            if (geometryId <= 0) return true;
            for (const ProjectFrame& frame : frames) {
                if (frame.id != frameId) continue;
                for (const auto& feature : frame.features) if (feature.id == geometryId) return true;
                return false;
            }
            return false;
        };
        if (record.sequence <= 0 || record.sequence == INT_MAX || sequences.contains(record.sequence)
            || record.featureNumber.trimmed().isEmpty()
            || record.frameId <= 0 || (record.secondaryGeometryId > 0 && record.secondaryFrameId <= 0)
            || record.geometryId < -1 || record.secondaryGeometryId < -1
            || m_measurementType->findText(record.type) < 0 || record.lower > record.upper
            || !std::isfinite(record.nominal) || !std::isfinite(record.lower) || !std::isfinite(record.upper)
            || !record.detection.validationError().isEmpty()
            || !frameHasFeature(record.frameId, record.geometryId)
            || !frameHasFeature(record.secondaryFrameId, record.secondaryGeometryId)
            || (record.type != QStringLiteral("角度") && record.type != QStringLiteral("长度")
                && (record.secondaryGeometryId > 0 || record.singleRoiAngle))
            || (record.type == QStringLiteral("长度") && record.singleRoiAngle)
            || (record.type == QStringLiteral("长度") && !record.crossFrameLength
                && (record.secondaryGeometryId > 0
                    || record.lengthStartPosition.collected || record.lengthEndPosition.collected))
            || (record.type == QStringLiteral("长度") && record.crossFrameLength
                && record.secondaryGeometryId > 0 && record.frameId == record.secondaryFrameId)
            || (record.type != QStringLiteral("孔径") && record.holeUniformCount != 0)
            || (record.type != QStringLiteral("孔径") && !holeCalibration.isUndefined())
            || (!axialScanType && (!lowerAxialOffset.isUndefined() || !upperAxialOffset.isUndefined()))
            || (record.type != QStringLiteral("跳动")
                && (!roundoutReference1.isUndefined() || !roundoutReference2.isUndefined()))
            || (axialScanType && ((lowerAxialOffset.isUndefined() != upperAxialOffset.isUndefined())
                || (!lowerAxialOffset.isUndefined()
                    && (record.lowerAxialOffsetPulse <= 0 || record.upperAxialOffsetPulse <= 0))))
            || (record.type != QStringLiteral("长度") && !lengthCalibration.isUndefined())
            || (record.type != QStringLiteral("长度") && !lengthTemplateValue.isUndefined())
            || (record.type != QStringLiteral("长度") && (!crossFrameLength.isUndefined()
                || !object.value(QStringLiteral("lengthStartPosition")).isUndefined()
                || !object.value(QStringLiteral("lengthEndPosition")).isUndefined()))
            || invalidAngleReferences) {
            error = QStringLiteral("测量记录%1内容或图形引用无效。").arg(record.sequence); return false;
        }
        const QVector<int> expectedAxes = deviceAxesForMeasurement(record.type);
        const int expectedCamera = deviceCameraForMeasurement(record.type);
        QVector<int> actualAxes;
        for (const auto& axis : record.devicePosition.axes) actualAxes.append(axis.axis);
        if ((record.devicePosition.collected
                && (actualAxes != expectedAxes || record.devicePosition.cameraIndex != expectedCamera
                    || (record.type == QStringLiteral("直径")
                        && !record.devicePosition.hasLightCurtainSample)
                    || (record.type != QStringLiteral("直径")
                        && record.devicePosition.hasLightCurtainSample)))
            || (!record.devicePosition.collected
                && (!actualAxes.isEmpty() || record.devicePosition.cameraIndex != -1
                    || record.devicePosition.exposure != -1
                    || record.devicePosition.hasLightCurtainSample))) {
            error = QStringLiteral("测量记录%1的设备点位与测量类型不匹配。").arg(record.sequence);
            return false;
        }
        sequences.insert(record.sequence); nextSequence = qMax(nextSequence, record.sequence + 1);
        record.clearTrial(migratedLegacyLength
            ? QStringLiteral("未执行（旧长度记录已迁移，请重新关联包含完整特征的单ROI）")
            : QStringLiteral("未执行（从工程载入，需重新试测并重新确认候选）"));
        record.candidateSelectionAuditMode = selectionMode;
        if (selectionMode != QStringLiteral("none")) {
            record.candidateSelectionAuditFirst = selection.value(QStringLiteral("firstCandidateId")).toInt();
            record.candidateSelectionAuditSecond = selection.value(QStringLiteral("secondCandidateId")).toInt();
        }
        records.append(record);
    }

    m_loadingProject = true;
    cancelRelink();
    m_canvas->setImage(image);
    setCanvasSourceBadge(m_canvas, QStringLiteral("配方参考图 · 非实时"));
    if (!m_canvas->restoreFeatures(features, error)) { m_loadingProject = false; return false; }
    if (QLabel* hint = m_canvas->findChild<QLabel*>(QStringLiteral("canvasEmptyHint")))
        hint->hide();
    m_records = records;
    m_nextRecordSequence = qMax(nextSequence, root.value(QStringLiteral("nextRecordSequence")).toInt(1));
    m_frames = frames;
    m_currentFrameId = currentFrameId;
    m_nextFrameId = nextFrameId;
    m_imageFilePath = imagePath;
    m_imageFileSha256 = QString::fromLatin1(currentImageSha256);
    m_imageCameraIndex = imageCameraIndex;
    m_imageExposure = imageExposure;
    m_recipeProgramNumber->setValue(recipeProgramNumber);
    m_recipePartNumber->setText(recipePartNumber);
    m_recipePartName->setText(recipePartName);
    m_recipeProcessNumber->setText(recipeProcessNumber);
    m_recipeNote->setText(recipeNote);
    m_recipeValidationResult->setText(recipeValue.isUndefined()
        ? QStringLiteral("旧配方未包含配方信息，请填写后检查生成条件。")
        : QStringLiteral("配方已载入，请检查生成条件。"));
    m_projectFilePath = QFileInfo(filePath).absoluteFilePath();
    m_loadingProject = false;
    m_projectDirty = false;
    refreshFrameSelector(); refreshFeatureList(); refreshMeasurementRecords();
    m_canvas->fitImageInView();
    return true;
}

void GraphicalProgramEditor::saveProject()
{
    if (m_projectFilePath.isEmpty()) { saveProjectAs(); return; }
    QString error;
    if (!saveRecipeFile(m_projectFilePath, error)) {
        QMessageBox::warning(this, QStringLiteral("保存配方失败"), error); return;
    }
    statusBar()->showMessage(QStringLiteral("配方已保存：%1").arg(m_projectFilePath), 6000);
}

void GraphicalProgramEditor::saveProjectAs()
{
    const QString initial = m_projectFilePath.isEmpty() ? QString() : m_projectFilePath;
    QString filePath = QFileDialog::getSaveFileName(this, QStringLiteral("保存测量配方"), initial,
        QStringLiteral("AxisMeasurement配方 (*.axisproj.json);;JSON文件 (*.json)"));
    if (filePath.isEmpty()) return;
    if (!filePath.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive)) filePath += QStringLiteral(".axisproj.json");
    QString error;
    if (!saveRecipeFile(filePath, error)) { QMessageBox::warning(this, QStringLiteral("保存配方失败"), error); return; }
    statusBar()->showMessage(QStringLiteral("配方已保存：%1").arg(m_projectFilePath), 6000);
}

void GraphicalProgramEditor::openProject()
{
    if (m_trialRunning || m_ownedAxis > 0 || m_ownedCamera >= 0) {
        QMessageBox::warning(this, QStringLiteral("不能打开配方"),
            QStringLiteral("请等待试测结束，并停止当前轴运动和相机采集。")); return;
    }
    if (m_projectDirty && QMessageBox::question(this, QStringLiteral("打开配方"),
        QStringLiteral("当前配方有未保存修改，继续将丢失这些修改。是否打开其他配方？")) != QMessageBox::Yes) return;
    const QString filePath = QFileDialog::getOpenFileName(this, QStringLiteral("打开测量配方"), QString(),
        QStringLiteral("AxisMeasurement配方 (*.axisproj.json *.json)"));
    if (filePath.isEmpty()) return;
    QString error;
    if (!loadRecipeFile(filePath, error)) { QMessageBox::warning(this, QStringLiteral("打开配方失败"), error); return; }
    statusBar()->showMessage(QStringLiteral("配方已载入；历史试测结果已失效，请重新试测。"), 6000);
}

void GraphicalProgramEditor::closeEvent(QCloseEvent* event)
{
    const bool cameraStopped = stopOwnedCamera();
    stopOwnedAxis();
    refreshAxisPanel();
    if (!cameraStopped || m_ownedCamera >= 0) {
        statusBar()->showMessage(QStringLiteral("相机采集未确认停止，请检查相机状态后再次关闭。"));
        event->ignore();
        return;
    }
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
    if (m_projectDirty && QMessageBox::question(this, QStringLiteral("关闭图形化编程"),
        QStringLiteral("当前配方有未保存修改。是否仍关闭窗口？")) != QMessageBox::Yes) {
        event->ignore();
        return;
    }
    QMainWindow::closeEvent(event);
}

void GraphicalProgramEditor::storeCurrentFrame()
{
    if (m_currentFrameId <= 0 || !m_canvas || !m_canvas->hasImage()) return;
    for (ProjectFrame& frame : m_frames) {
        if (frame.id != m_currentFrameId) continue;
        frame.features = m_canvas->featureSnapshots();
        frame.image = m_canvas->sourceImage();
        frame.filePath = m_imageFilePath;
        frame.fileSha256 = m_imageFileSha256;
        frame.cameraIndex = m_imageCameraIndex;
        frame.exposure = m_imageExposure;
        return;
    }
}

void GraphicalProgramEditor::refreshFrameSelector()
{
    if (!m_frameSelector) return;
    QSignalBlocker blocker(m_frameSelector);
    m_frameSelector->clear();
    for (const ProjectFrame& frame : m_frames) {
        const QString source = frame.cameraIndex >= 0
            ? QStringLiteral("相机%1").arg(frame.cameraIndex)
            : QFileInfo(frame.filePath).fileName();
        m_frameSelector->addItem(QStringLiteral("图像%1 · %2").arg(frame.id).arg(source), frame.id);
        if (frame.id == m_currentFrameId) m_frameSelector->setCurrentIndex(m_frameSelector->count() - 1);
    }
    m_frameSelector->setEnabled(m_frames.size() > 1);
    m_frameSelector->setVisible(m_frames.size() > 1);
}

bool GraphicalProgramEditor::activateFrame(int frameId, QString& error)
{
    error.clear();
    if (frameId <= 0) { error = QStringLiteral("目标帧编号无效。"); return false; }
    storeCurrentFrame();
    ProjectFrame* target = nullptr;
    for (ProjectFrame& frame : m_frames)
        if (frame.id == frameId) { target = &frame; break; }
    if (!target) { error = QStringLiteral("工程中不存在帧%1。").arg(frameId); return false; }
    if (target->image.isNull() && !target->image.load(target->filePath)) {
        error = QStringLiteral("无法读取帧%1图像：%2").arg(frameId).arg(target->filePath); return false;
    }
    if (!m_canvas->validateFeatureSnapshots(target->features, target->image.size(), error))
        return false;
    m_loadingProject = true;
    m_canvas->setImage(target->image);
    setCanvasSourceBadge(m_canvas, QStringLiteral("端点参考图 · 非实时"));
    if (!m_canvas->restoreFeatures(target->features, error)) {
        m_loadingProject = false;
        return false;
    }
    m_currentFrameId = target->id;
    m_imageFilePath = target->filePath;
    m_imageFileSha256 = target->fileSha256;
    m_imageCameraIndex = target->cameraIndex;
    m_imageExposure = target->exposure;
    m_loadingProject = false;
    if (QLabel* hint = m_canvas->findChild<QLabel*>(QStringLiteral("canvasEmptyHint"))) hint->hide();
    refreshFrameSelector();
    refreshFeatureList();
    refreshMeasurementRecords();
    m_canvas->fitImageInView();
    statusBar()->showMessage(QStringLiteral("已切换到端点图像%1；该图像ROI已恢复。").arg(frameId), 4000);
    return true;
}

void GraphicalProgramEditor::addLocalFrame()
{
    if (m_ownedCamera >= 0 || m_trialRunning) {
        QMessageBox::warning(this, QStringLiteral("不能添加帧"),
            QStringLiteral("请先停止相机采集并等待算法结束。"));
        return;
    }
    const QString filePath = QFileDialog::getOpenFileName(this, QStringLiteral("导入端点图像"), QString(),
        QStringLiteral("图像文件 (*.bmp *.png *.jpg *.jpeg *.tif *.tiff);;所有文件 (*.*)"));
    if (filePath.isEmpty()) return;
    QString hashError;
    const QByteArray hash = projectFileSha256(filePath, hashError);
    QImage image;
    if (hash.isEmpty() || !image.load(filePath)) {
        QMessageBox::warning(this, QStringLiteral("添加帧失败"),
            hash.isEmpty() ? hashError : QStringLiteral("无法读取所选图像。"));
        return;
    }
    storeCurrentFrame();
    ProjectFrame frame;
    frame.id = m_nextFrameId++;
    frame.filePath = QFileInfo(filePath).absoluteFilePath();
    frame.fileSha256 = QString::fromLatin1(hash);
    frame.image = image;
    m_frames.append(frame);
    m_projectDirty = true;
    QString error;
    if (!activateFrame(frame.id, error))
        QMessageBox::warning(this, QStringLiteral("添加帧失败"), error);
}

void GraphicalProgramEditor::removeCurrentFrame()
{
    if (m_trialRunning || m_relinkSequence > 0 || m_ownedCamera >= 0) {
        QMessageBox::warning(this, QStringLiteral("不能移除端点图"),
            QStringLiteral("请先结束试测、关联操作或相机采集。"));
        return;
    }
    if (m_frames.size() <= 1 || m_currentFrameId <= 0) {
        QMessageBox::warning(this, QStringLiteral("不能移除端点图"),
            QStringLiteral("工程至少需要保留一张图像。"));
        return;
    }
    QStringList references;
    for (const MeasurementRecord& record : m_records) {
        if (record.frameId == m_currentFrameId || record.secondaryFrameId == m_currentFrameId)
            references << QStringLiteral("记录%1/%2").arg(record.sequence).arg(record.featureNumber);
    }
    if (!references.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("端点图仍被引用"),
            QStringLiteral("图像%1仍被%2引用。请先删除相应测量记录或把端点重新关联到其他图像。")
                .arg(m_currentFrameId).arg(references.join(QStringLiteral("、"))));
        return;
    }
    int currentIndex = -1;
    for (int index = 0; index < m_frames.size(); ++index)
        if (m_frames[index].id == m_currentFrameId) { currentIndex = index; break; }
    if (currentIndex < 0) {
        QMessageBox::warning(this, QStringLiteral("不能移除端点图"), QStringLiteral("当前图像不在工程中。"));
        return;
    }
    const int removedFrameId = m_currentFrameId;
    const int replacementFrameId = currentIndex + 1 < m_frames.size()
        ? m_frames[currentIndex + 1].id : m_frames[currentIndex - 1].id;
    if (QMessageBox::question(this, QStringLiteral("移除端点图"),
        QStringLiteral("从工程移除图像%1及其ROI？磁盘上的原图片文件不会被删除。")
            .arg(removedFrameId)) != QMessageBox::Yes)
        return;
    m_frames.removeAt(currentIndex);
    m_projectDirty = true;
    QString error;
    if (!activateFrame(replacementFrameId, error)) {
        refreshFrameSelector();
        QMessageBox::warning(this, QStringLiteral("切换图像失败"), error);
        return;
    }
    statusBar()->showMessage(QStringLiteral("已从工程移除端点图像%1；磁盘原文件未删除。")
        .arg(removedFrameId), 5000);
}

void GraphicalProgramEditor::openLocalImage()
{
    if (m_ownedCamera >= 0) {
        QMessageBox::warning(this, QStringLiteral("不能打开图像"), QStringLiteral("请先停止相机采集。"));
        return;
    }
    cancelRelink();
    const QString filePath = QFileDialog::getOpenFileName(
        this, QStringLiteral("打开测量图像"), QString(),
        QStringLiteral("图像文件 (*.bmp *.png *.jpg *.jpeg *.tif *.tiff);;所有文件 (*.*)"));
    if (filePath.isEmpty())
        return;

    QString hashError;
    const QByteArray imageSha256 = projectFileSha256(filePath, hashError);
    if (imageSha256.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("打开图像失败"), hashError); return;
    }

    if (m_projectDirty && QMessageBox::question(this, QStringLiteral("更换图像"),
        QStringLiteral("当前配方有未保存修改。更换图像将清空记录和图形，是否继续？")) != QMessageBox::Yes)
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
    m_imageFilePath = QFileInfo(filePath).absoluteFilePath();
    m_imageFileSha256 = QString::fromLatin1(imageSha256);
    m_imageCameraIndex = -1;
    m_imageExposure = -1;
    m_frames.clear();
    ProjectFrame frame;
    frame.id = m_nextFrameId = 1;
    frame.filePath = m_imageFilePath;
    frame.fileSha256 = m_imageFileSha256;
    frame.image = m_canvas->sourceImage();
    m_frames.append(frame);
    m_currentFrameId = frame.id;
    m_nextFrameId = 2;
    m_recipeProgramNumber->setValue(0);
    m_recipePartNumber->clear();
    m_recipePartName->clear();
    m_recipeProcessNumber->clear();
    m_recipeNote->clear();
    m_recipeValidationResult->setText(QStringLiteral("填写配方信息后检查记录、ROI、标定和设备点位。"));
    refreshFrameSelector();
    m_projectFilePath.clear();
    m_projectDirty = true;
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
    setCanvasSourceBadge(m_canvas, QStringLiteral("离线图像 · 非实时"));

    for (const QPair<int, QString>& featureEntry : featureEntries) {
        const QString displayName = m_currentFrameId > 0
            ? QStringLiteral("图像%1/%2").arg(m_currentFrameId).arg(featureEntry.second)
            : featureEntry.second;
        QListWidgetItem* item = new QListWidgetItem(displayName, m_featureList);
        item->setData(Qt::UserRole, featureEntry.first);
    }
    m_featureList->setEnabled(true);
}

void GraphicalProgramEditor::refreshFeatureProperties(int featureId)
{
    const QStringList properties = m_canvas->featureProperties(featureId);
    m_selectedFeatureId = properties.size() == 3 ? featureId : -1;
    m_featureNameLabel->setText(properties.size() == 3
        ? QStringLiteral("图像%1/%2").arg(m_currentFrameId).arg(properties.at(0))
        : QStringLiteral("未选择"));
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
