#include "graphical_canvas.h"

#include <QApplication>
#include <QFrame>
#include <QFocusEvent>
#include <QBrush>
#include <QColor>
#include <QContextMenuEvent>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsRectItem>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QMap>
#include <QSet>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>
#include <QScrollBar>
#include <QSize>
#include <QWheelEvent>
#include <QtMath>
#include <QTransform>
#include <climits>
/*中央画布（黑色图像区：显示图像、画图形、拖拽/旋转/缩放手柄）/绘图引擎*/
namespace {
bool arcCircle(const QPointF& a, const QPointF& b, const QPointF& c,
    QPointF& center, qreal& radius)
{
    const QPointF u = b - a, v = c - a;
    const qreal d = 2.0 * (u.x() * v.y() - u.y() * v.x());
    if (qAbs(d) < 1e-10) return false;
    const qreal uu = QPointF::dotProduct(u, u), vv = QPointF::dotProduct(v, v);
    center = a + QPointF((uu * v.y() - vv * u.y()) / d,
        (u.x() * vv - v.x() * uu) / d);
    radius = QLineF(center, a).length();
    return qIsFinite(radius) && radius > 0.0;
}

QPen makeCanvasPen(const QColor& color, qreal width = 3.0,
    Qt::PenStyle style = Qt::SolidLine)
{
    QPen pen(color, width, style);
    pen.setCosmetic(true);
    return pen;
}
}

GraphicalCanvas::GraphicalCanvas(QWidget* parent)
    : QGraphicsView(parent),
      m_scene(new QGraphicsScene(this))
{
    setScene(m_scene);
    setBackgroundBrush(QColor(35, 35, 35));
    setFrameShape(QFrame::NoFrame);
    setAlignment(Qt::AlignCenter);
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setDragMode(QGraphicsView::NoDrag);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    connect(m_scene, &QGraphicsScene::selectionChanged, this, [this]() {
        int selectedId = -1;
        const QList<QGraphicsItem*> selectedItems = m_scene->selectedItems();
        if (selectedItems.size() == 1)
            selectedId = selectedItems.first()->data(0).toInt();
        refreshCircleRadiusGuide();
        m_resizeFeatureId = -1;
        m_rotateFeatureId = -1;
        viewport()->update();
        emit selectedFeatureChanged(selectedId);
    });
}

bool GraphicalCanvas::loadImage(const QString& filePath)//loadImage/setImage/clearImage/fitImageInView：图像作为一个 QGraphicsPixmapItem 放进 Scene，"适合窗口"就是 fitInView。
{
    QImage image;
    if (!image.load(filePath))
        return false;

    setImage(image);
    return true;
}

void GraphicalCanvas::setImage(const QImage& image)
{
    setDetectionOverlay(QPainterPath(), QPainterPath());
    m_rotateFeatureId = -1;
    m_resizeFeatureId = -1;
    m_draggedFeatureItem = nullptr;
    m_circleRadiusGuide = nullptr;
    m_scene->clear();
    m_imageItem = nullptr;
    m_sourceImage = image;
    m_zoomStep = 0;
    m_previewItem = nullptr;
    m_arcPreviewItem = nullptr;
    m_arcPoints.clear();
    m_nextFeatureId = 1;
    emit featuresChanged();

    if (m_sourceImage.isNull()) {
        m_scene->setSceneRect(QRectF());
        resetTransform();
        return;
    }

    m_imageItem = m_scene->addPixmap(QPixmap::fromImage(m_sourceImage));
    m_imageItem->setPos(0.0, 0.0);
    m_scene->setSceneRect(m_imageItem->boundingRect());
    fitImageInView();
}

void GraphicalCanvas::clearImage()
{
    setImage(QImage());
}

void GraphicalCanvas::fitImageInView()
{
    if (!m_imageItem)
        return;

    resetTransform();
    fitInView(m_imageItem, Qt::KeepAspectRatio);
    m_zoomStep = 0;
}

bool GraphicalCanvas::hasImage() const
{
    return m_imageItem != nullptr && !m_sourceImage.isNull();
}

QImage GraphicalCanvas::sourceImage() const
{
    return m_sourceImage;
}

QPointF GraphicalCanvas::mapViewportToImage(const QPoint& viewportPosition) const
{
    if (!m_imageItem)
        return QPointF();

    return m_imageItem->mapFromScene(mapToScene(viewportPosition));
}

void GraphicalCanvas::setDrawingTool(DrawingTool tool)
{
    m_rotateFeatureId = -1;
    m_resizeFeatureId = -1;
    if (m_drawingTool == DrawingTool::Arc && tool != DrawingTool::Arc)
        cancelArcDraft();
    m_drawingTool = tool;
    setDragMode(tool == DrawingTool::Select
        ? QGraphicsView::RubberBandDrag
        : QGraphicsView::NoDrag);
    viewport()->setCursor(tool == DrawingTool::Select
        ? Qt::ArrowCursor
        : Qt::CrossCursor);
    viewport()->update();
}

void GraphicalCanvas::setResizeRatioLocked(bool locked)
{
    m_resizeRatioLocked = locked;
}

bool GraphicalCanvas::rotationHandleGeometry(QGraphicsItem* item, QPointF& center,
    QPointF& anchor, QPointF& handle) const
{
    if (!item) return false;
    const QString type = item->data(1).toString();
    QPointF localCenter, localAnchor;
    if (type == QStringLiteral("矩形")) {
        const QRectF r = static_cast<QGraphicsRectItem*>(item)->rect();
        localCenter = r.center(); localAnchor = QPointF(r.center().x(), r.top());
    }
    else if (type == QStringLiteral("直线")) {
        const QLineF line = static_cast<QGraphicsLineItem*>(item)->line();
        if (line.length() <= 1e-10) return false;
        localCenter = (line.p1() + line.p2()) / 2.0;
        localAnchor = localCenter + QPointF(line.dy(), -line.dx()) / line.length();
    }
    else if (type == QStringLiteral("圆弧")) {
        qreal radius;
        if (!arcCircle(item->data(3).toPointF(), item->data(4).toPointF(),
            item->data(5).toPointF(), localCenter, radius)) return false;
        localAnchor = item->data(4).toPointF();
    }
    else return false;
    center = item->mapToScene(localCenter);
    anchor = item->mapToScene(localAnchor);
    QLineF ray(center, anchor);
    if (ray.length() <= 1e-10) return false;
    ray.setLength(ray.length() + 28.0 / qMax<qreal>(0.0001, qAbs(transform().m11())));
    handle = ray.p2();
    if (type == QStringLiteral("直线")) anchor = center;
    return true;
}

bool GraphicalCanvas::rotationHandleAt(const QPoint& viewportPoint, QGraphicsItem*& item) const
{
    item = nullptr;
    if (m_drawingTool != DrawingTool::Select) return false;
    const QList<QGraphicsItem*> selected = m_scene->selectedItems();
    if (selected.size() != 1) return false;
    QPointF center, anchor, handle;
    if (!rotationHandleGeometry(selected.first(), center, anchor, handle)) return false;
    if (QLineF(QPointF(viewportPoint), QPointF(mapFromScene(handle))).length() > 10.0) return false;
    item = selected.first();
    return true;
}

void GraphicalCanvas::updateHandleRotation(const QPoint& viewportPoint)
{
    const QPointF delta = mapToScene(viewportPoint) - m_rotationCenterScene;
    if (QLineF(QPointF(), delta).length() * qAbs(transform().m11()) < 6.0) return;
    const qreal pointerAngle = qRadiansToDegrees(qAtan2(delta.y(), delta.x()));
    qreal angle = m_rotationStartAngle + pointerAngle - m_rotationPressAngle;
    while (angle < 0) angle += 360.0;
    while (angle >= 360.0) angle -= 360.0;
    QString error;
    if (!rotateFeature(m_rotateFeatureId, angle, error)) emit canvasMessage(error);
}

QVector<QPointF> GraphicalCanvas::resizeHandlePoints(QGraphicsItem* item) const
{
    QVector<QPointF> points;
    if (!item) return points;
    const QString type = item->data(1).toString();
    if (type == QStringLiteral("矩形")) {
        const QRectF r = static_cast<QGraphicsRectItem*>(item)->rect();
        points << r.topLeft() << r.topRight() << r.bottomRight() << r.bottomLeft();
    }
    else if (type == QStringLiteral("圆")) {
        const QRectF r = static_cast<QGraphicsEllipseItem*>(item)->rect();
        points << QPointF(r.right(), r.center().y()) << QPointF(r.center().x(), r.bottom())
            << QPointF(r.left(), r.center().y()) << QPointF(r.center().x(), r.top());
    }
    else if (type == QStringLiteral("直线")) {
        const QLineF line = static_cast<QGraphicsLineItem*>(item)->line();
        points << line.p1() << line.p2();
    }
    else if (type == QStringLiteral("圆弧")) {
        points << item->data(3).toPointF() << item->data(4).toPointF() << item->data(5).toPointF();
    }
    return points;
}

int GraphicalCanvas::resizeHandleAt(const QPoint& viewportPoint, QGraphicsItem*& item) const
{
    item = nullptr;
    if (m_drawingTool != DrawingTool::Select) return -1;
    const QList<QGraphicsItem*> selected = m_scene->selectedItems();
    if (selected.size() != 1) return -1;
    const QVector<QPointF> points = resizeHandlePoints(selected.first());
    int closest = -1;
    qreal distance = 9.0;
    for (int i = 0; i < points.size(); ++i) {
        const qreal candidate = QLineF(QPointF(viewportPoint),
            QPointF(mapFromScene(selected.first()->mapToScene(points.at(i))))).length();
        if (candidate < distance) { distance = candidate; closest = i; }
    }
    if (closest >= 0) item = selected.first();
    return closest;
}

void GraphicalCanvas::updateHandleResize(const QPoint& viewportPoint)
{
    QGraphicsItem* item = featureItemById(m_resizeFeatureId);
    if (!item || m_resizeFeatureId <= 0) { m_resizeFeatureId = -1; return; }
    const QPointF delta = item->mapFromScene(mapToScene(viewportPoint)) - m_resizePressLocal;
    const QPointF target = m_resizeVector + delta;
    const QString type = item->data(1).toString();
    qreal primary = m_resizeStartSize.width(), secondary = m_resizeStartSize.height();
    if (type == QStringLiteral("矩形") && !m_dragRatioLocked) {
        primary = 2.0 * target.x() * (m_resizeVector.x() < 0 ? -1.0 : 1.0);
        secondary = 2.0 * target.y() * (m_resizeVector.y() < 0 ? -1.0 : 1.0);
    }
    else {
        const qreal denominator = QPointF::dotProduct(m_resizeVector, m_resizeVector);
        if (denominator <= 1e-10) return;
        const qreal factor = QPointF::dotProduct(target, m_resizeVector) / denominator;
        primary *= factor;
        secondary *= factor;
    }
    if (primary <= 0.0001 || (type == QStringLiteral("矩形") && secondary <= 0.0001))
        return; // No flipping or zero-size geometry when crossing the center.
    QString error;
    if (!resizeFeature(m_resizeFeatureId, primary, secondary, error))
        emit canvasMessage(error);
}

void GraphicalCanvas::setRectangleDrawingMode(RectangleDrawingMode mode)
{
    m_rectangleDrawingMode = mode;
}

void GraphicalCanvas::setCircleDrawingMode(CircleDrawingMode mode)
{
    m_circleDrawingMode = mode;
}

bool GraphicalCanvas::buildArcPath(const QPointF& startPoint,
    const QPointF& middlePoint, const QPointF& endPoint, QPainterPath& path) const
{
    const qreal x1 = startPoint.x();
    const qreal y1 = startPoint.y();
    const qreal x2 = middlePoint.x();
    const qreal y2 = middlePoint.y();
    const qreal x3 = endPoint.x();
    const qreal y3 = endPoint.y();
    const qreal determinant = 2.0 * (x1 * (y2 - y3)
        + x2 * (y3 - y1) + x3 * (y1 - y2));

    const qreal scale = qMax<qreal>(1.0,
        qMax(QLineF(startPoint, middlePoint).length(),
            QLineF(middlePoint, endPoint).length()));
    if (qAbs(determinant) < 0.001 * scale * scale)
        return false;

    const qreal square1 = x1 * x1 + y1 * y1;
    const qreal square2 = x2 * x2 + y2 * y2;
    const qreal square3 = x3 * x3 + y3 * y3;
    const QPointF center(
        (square1 * (y2 - y3) + square2 * (y3 - y1) + square3 * (y1 - y2)) / determinant,
        (square1 * (x3 - x2) + square2 * (x1 - x3) + square3 * (x2 - x1)) / determinant);
    const qreal radius = QLineF(center, startPoint).length();
    if (!qIsFinite(radius) || radius < 1.0)
        return false;

    const auto pointAngle = [&center](const QPointF& point) {
        qreal angle = qRadiansToDegrees(qAtan2(center.y() - point.y(),
            point.x() - center.x()));
        if (angle < 0.0)
            angle += 360.0;
        return angle;
    };
    const auto positiveSweep = [](qreal angle) {
        while (angle < 0.0)
            angle += 360.0;
        while (angle >= 360.0)
            angle -= 360.0;
        return angle;
    };

    const qreal startAngle = pointAngle(startPoint);
    const qreal middleSweep = positiveSweep(pointAngle(middlePoint) - startAngle);
    const qreal endSweep = positiveSweep(pointAngle(endPoint) - startAngle);
    const qreal sweepLength = middleSweep <= endSweep ? endSweep : endSweep - 360.0;
    const QRectF circleBounds(center.x() - radius, center.y() - radius,
        radius * 2.0, radius * 2.0);

    path = QPainterPath();
    path.moveTo(startPoint);
    path.arcTo(circleBounds, startAngle, sweepLength);
    return true;
}

void GraphicalCanvas::updateArcPreview(const QPointF& currentPoint)
{
    if (m_arcPoints.isEmpty())
        return;

    if (!m_arcPreviewItem) {
        m_arcPreviewItem = m_scene->addPath(QPainterPath(),
            makeCanvasPen(QColor(255, 230, 0), 2.0, Qt::DashLine));
        m_arcPreviewItem->setZValue(20.0);
    }

    QPainterPath previewPath;
    const qreal markerRadius = 3.0;
    for (const QPointF& point : m_arcPoints)
        previewPath.addEllipse(point, markerRadius, markerRadius);

    if (m_arcPoints.size() == 1) {
        previewPath.moveTo(m_arcPoints.first());
        previewPath.lineTo(currentPoint);
    }
    else {
        QPainterPath arcPath;
        if (buildArcPath(m_arcPoints.at(0), m_arcPoints.at(1), currentPoint, arcPath))
            previewPath.addPath(arcPath);
        else {
            previewPath.moveTo(m_arcPoints.at(0));
            previewPath.lineTo(m_arcPoints.at(1));
            previewPath.lineTo(currentPoint);
        }
    }
    m_arcPreviewItem->setPath(previewPath);
}

void GraphicalCanvas::cancelArcDraft()
{
    if (m_arcPreviewItem) {
        m_scene->removeItem(m_arcPreviewItem);
        delete m_arcPreviewItem;
        m_arcPreviewItem = nullptr;
    }
    m_arcPoints.clear();
}

void GraphicalCanvas::showCircleRadiusGuide(QGraphicsItem* circleItem,
    const QRectF& circleRect)
{
    clearCircleRadiusGuide();
    if (!circleItem || circleRect.isEmpty())
        return;

    const QPointF center = circleRect.center();
    const QPointF radiusPoint(circleRect.right(), center.y());
    m_circleRadiusGuide = new QGraphicsLineItem(
        QLineF(center, radiusPoint), circleItem);
    m_circleRadiusGuide->setPen(makeCanvasPen(
        QColor(255, 230, 0), 2.0, Qt::DashLine));
    m_circleRadiusGuide->setAcceptedMouseButtons(Qt::NoButton);
    m_circleRadiusGuide->setZValue(1.0);
}

void GraphicalCanvas::clearCircleRadiusGuide()
{
    if (!m_circleRadiusGuide)
        return;
    delete m_circleRadiusGuide;
    m_circleRadiusGuide = nullptr;
}

void GraphicalCanvas::refreshCircleRadiusGuide()
{
    clearCircleRadiusGuide();
    const QList<QGraphicsItem*> selectedItems = m_scene->selectedItems();
    if (selectedItems.size() != 1)
        return;

    QGraphicsItem* selectedItem = selectedItems.first();
    if (selectedItem->data(1).toString() != QStringLiteral("圆"))
        return;

    QGraphicsEllipseItem* circleItem =
        qgraphicsitem_cast<QGraphicsEllipseItem*>(selectedItem);
    if (circleItem)
        showCircleRadiusGuide(circleItem, circleItem->rect());
}

GraphicalCanvas::DrawingTool GraphicalCanvas::drawingTool() const
{
    return m_drawingTool;
}

QStringList GraphicalCanvas::featureNames() const
{
    QMap<int, QString> namesById;
    const QList<QGraphicsItem*> allItems = m_scene->items();
    for (QGraphicsItem* item : allItems) {
        const int featureId = item->data(0).toInt();
        const QString featureName = item->data(2).toString();
        if (featureId > 0 && !featureName.isEmpty())
            namesById.insert(featureId, featureName);
    }
    return namesById.values();
}

QStringList GraphicalCanvas::featureProperties(int featureId) const
{
    if (featureId <= 0 || !m_imageItem)
        return QStringList();
    QGraphicsItem* item = featureItemById(featureId);
    if (!item)
        return QStringList();
    const auto number = [](qreal value) { return QString::number(value, 'f', 2); };
    const auto imagePoint = [this, item](const QPointF& point) {
        return m_imageItem->mapFromScene(item->mapToScene(point));
    };
    const auto pointText = [&number](const QPointF& point) {
        return QStringLiteral("(%1, %2) px").arg(number(point.x()), number(point.y()));
    };
    const QString type = item->data(1).toString();
    QString geometry;
    if (type == QStringLiteral("点")) {
        geometry = QStringLiteral("位置：%1").arg(pointText(imagePoint(QPointF())));
    }
    else if (type == QStringLiteral("直线")) {
        const QLineF localLine = static_cast<QGraphicsLineItem*>(item)->line();
        const QLineF line(imagePoint(localLine.p1()), imagePoint(localLine.p2()));
        qreal angle = qRadiansToDegrees(qAtan2(line.dy(), line.dx()));
        if (angle < 0.0) angle += 360.0;
        geometry = QStringLiteral("起点：%1\n终点：%2\n长度：%3 px\n方向角：%4°（向右为0°，顺时针）")
            .arg(pointText(line.p1()), pointText(line.p2()), number(line.length()), number(angle));
    }
    else if (type == QStringLiteral("矩形") || type == QStringLiteral("圆")) {
        const QRectF bounds = type == QStringLiteral("矩形")
            ? static_cast<QGraphicsRectItem*>(item)->rect()
            : static_cast<QGraphicsEllipseItem*>(item)->rect();
        const qreal width = QLineF(imagePoint(bounds.topLeft()), imagePoint(bounds.topRight())).length();
        const qreal height = QLineF(imagePoint(bounds.topLeft()), imagePoint(bounds.bottomLeft())).length();
        if (type == QStringLiteral("圆"))
            geometry = QStringLiteral("圆心：%1\n半径：%2 px\n直径：%3 px")
                .arg(pointText(imagePoint(bounds.center())), number(width / 2.0), number(width));
        else
            geometry = QStringLiteral("中心：%1\n宽：%2 px\n高：%3 px\n旋转角：%4°（顺时针）")
                .arg(pointText(imagePoint(bounds.center())), number(width), number(height), number(item->rotation()));
    }
    else if (type == QStringLiteral("圆弧")) {
        const QPointF a = imagePoint(item->data(3).toPointF());
        const QPointF b = imagePoint(item->data(4).toPointF());
        const QPointF c = imagePoint(item->data(5).toPointF());
        // Work relative to the start point to avoid cancellation after translation.
        const QPointF u = b - a;
        const QPointF v = c - a;
        const qreal determinant = 2.0 * (u.x() * v.y() - u.y() * v.x());
        geometry = QStringLiteral("起点：%1\n弧上点：%2\n终点：%3")
            .arg(pointText(a), pointText(b), pointText(c));
        if (qAbs(determinant) > 1e-10) {
            const qreal uu = QPointF::dotProduct(u, u);
            const qreal vv = QPointF::dotProduct(v, v);
            const QPointF center = a + QPointF((uu * v.y() - vv * u.y()) / determinant,
                (u.x() * vv - v.x() * uu) / determinant);
            geometry += QStringLiteral("\n圆心：%1\n半径：%2 px\n旋转角：%3°（顺时针）")
                .arg(pointText(center), number(QLineF(center, a).length()), number(item->rotation()));
        }
    }
    return QStringList() << item->data(2).toString() << type << geometry;
}

QSizeF GraphicalCanvas::featureDimensions(int featureId) const
{
    if (featureId <= 0) return QSizeF();
    QGraphicsItem* item = featureItemById(featureId);
    if (!item) return QSizeF();
    const QString type = item->data(1).toString();
    if (type == QStringLiteral("矩形"))
        return static_cast<QGraphicsRectItem*>(item)->rect().size();
    if (type == QStringLiteral("圆"))
        return QSizeF(static_cast<QGraphicsEllipseItem*>(item)->rect().width() / 2.0, 0);
    if (type == QStringLiteral("直线"))
        return QSizeF(static_cast<QGraphicsLineItem*>(item)->line().length(), 0);
    if (type == QStringLiteral("圆弧")) {
        QPointF center;
        qreal radius;
        if (arcCircle(item->data(3).toPointF(), item->data(4).toPointF(),
            item->data(5).toPointF(), center, radius)) return QSizeF(radius, 0);
    }
    return QSizeF();
}

qreal GraphicalCanvas::featureRotationAngle(int featureId) const
{
    QGraphicsItem* item = featureId > 0 ? featureItemById(featureId) : nullptr;
    if (!item) return 0.0;
    qreal angle = item->rotation();
    if (item->data(1).toString() == QStringLiteral("直线")) {
        const QLineF local = static_cast<QGraphicsLineItem*>(item)->line();
        const QLineF sceneLine(item->mapToScene(local.p1()), item->mapToScene(local.p2()));
        angle = qRadiansToDegrees(qAtan2(sceneLine.dy(), sceneLine.dx()));
    }
    while (angle < 0) angle += 360.0;
    while (angle >= 360.0) angle -= 360.0;
    return angle;
}

bool GraphicalCanvas::rotateFeature(int featureId, qreal angle, QString& error)
{
    QGraphicsItem* item = featureId > 0 ? featureItemById(featureId) : nullptr;
    if (!item || !m_imageItem || !qIsFinite(angle) || qAbs(angle) > 360.0) {
        error = QStringLiteral("请选择有效图形，角度范围为-360至360度。"); return false;
    }
    while (angle < 0) angle += 360.0;
    while (angle >= 360.0) angle -= 360.0;
    const QString type = item->data(1).toString();
    QPointF center;
    QPainterPath geometry;
    qreal rotation = angle;
    if (type == QStringLiteral("矩形")) {
        const QRectF bounds = static_cast<QGraphicsRectItem*>(item)->rect();
        center = bounds.center(); geometry.addRect(bounds);
    }
    else if (type == QStringLiteral("直线")) {
        const QLineF line = static_cast<QGraphicsLineItem*>(item)->line();
        if (line.length() <= 1e-10) { error = QStringLiteral("零长度直线无法设置方向。"); return false; }
        center = (line.p1() + line.p2()) / 2.0;
        geometry.moveTo(line.p1()); geometry.lineTo(line.p2());
        rotation -= qRadiansToDegrees(qAtan2(line.dy(), line.dx()));
    }
    else if (type == QStringLiteral("圆弧")) {
        qreal radius;
        if (!arcCircle(item->data(3).toPointF(), item->data(4).toPointF(),
            item->data(5).toPointF(), center, radius)) {
            error = QStringLiteral("圆弧定义无效。"); return false;
        }
        geometry = static_cast<QGraphicsPathItem*>(item)->path();
    }
    else { error = QStringLiteral("点和正圆无需旋转。"); return false; }

    const QPointF sceneCenter = item->mapToScene(center);
    QTransform rotationDelta;
    rotationDelta.translate(sceneCenter.x(), sceneCenter.y());
    rotationDelta.rotate(rotation - item->rotation());
    rotationDelta.translate(-sceneCenter.x(), -sceneCenter.y());
    const QRectF candidate = rotationDelta.map(item->mapToScene(geometry)).boundingRect();
    const QRectF allowed = m_imageItem->sceneBoundingRect();
    const qreal epsilon = 1e-6;
    if (candidate.left() < allowed.left() - epsilon || candidate.right() > allowed.right() + epsilon
        || candidate.top() < allowed.top() - epsilon || candidate.bottom() > allowed.bottom() + epsilon) {
        error = QStringLiteral("旋转后超出图像边界，原图形未改变。请先移动或缩小图形。"); return false;
    }
    // Keep the world-space center fixed even if the local origin changes.
    item->setTransformOriginPoint(center);
    item->setRotation(rotation);
    const QPointF correction = sceneCenter - item->mapToScene(center);
    item->moveBy(correction.x(), correction.y());
    viewport()->update();
    emit featureGeometryChanged(featureId);
    return true;
}

bool GraphicalCanvas::resizeFeature(int featureId, qreal primary, qreal secondary, QString& error)
{
    QGraphicsItem* item = featureId > 0 ? featureItemById(featureId) : nullptr;
    if (!item || !m_imageItem || !qIsFinite(primary) || primary <= 0) {
        error = QStringLiteral("请选择有效图形并输入大于零的尺寸。");
        return false;
    }
    const QString type = item->data(1).toString();
    QPainterPath proposed;
    QRectF bounds;
    QLineF line;
    QPointF a, b, c;
    if (type == QStringLiteral("矩形") || type == QStringLiteral("圆")) {
        const bool rectangle = type == QStringLiteral("矩形");
        if (rectangle && (!qIsFinite(secondary) || secondary <= 0)) {
            error = QStringLiteral("矩形高度必须大于零。"); return false;
        }
        const QRectF old = rectangle ? static_cast<QGraphicsRectItem*>(item)->rect()
            : static_cast<QGraphicsEllipseItem*>(item)->rect();
        const QSizeF size = rectangle ? QSizeF(primary, secondary) : QSizeF(2 * primary, 2 * primary);
        bounds = QRectF(old.center() - QPointF(size.width() / 2, size.height() / 2), size);
        if (rectangle) proposed.addRect(bounds); else proposed.addEllipse(bounds);
    }
    else if (type == QStringLiteral("直线")) {
        const QLineF old = static_cast<QGraphicsLineItem*>(item)->line();
        if (old.length() <= 1e-10) { error = QStringLiteral("零长度直线没有确定方向。"); return false; }
        const QPointF half = (old.p2() - old.p1()) * (primary / old.length() / 2.0);
        const QPointF center = (old.p1() + old.p2()) / 2.0;
        line = QLineF(center - half, center + half);
        proposed.moveTo(line.p1()); proposed.lineTo(line.p2());
    }
    else if (type == QStringLiteral("圆弧")) {
        a = item->data(3).toPointF(); b = item->data(4).toPointF(); c = item->data(5).toPointF();
        QPointF center; qreal radius;
        if (!arcCircle(a, b, c, center, radius)) { error = QStringLiteral("圆弧定义无效。"); return false; }
        a = center + (a - center) * (primary / radius);
        b = center + (b - center) * (primary / radius);
        c = center + (c - center) * (primary / radius);
        if (!buildArcPath(a, b, c, proposed)) { error = QStringLiteral("该半径无法生成有效圆弧。"); return false; }
    }
    else { error = QStringLiteral("此类型不支持尺寸编辑。"); return false; }

    // Validate before modifying: reject oversize geometry without moving its center.
    const QRectF candidate = item->mapToScene(proposed).boundingRect();
    const QRectF imageBounds = m_imageItem->sceneBoundingRect();
    const qreal epsilon = 1e-6;
    if (candidate.left() < imageBounds.left() - epsilon || candidate.right() > imageBounds.right() + epsilon
        || candidate.top() < imageBounds.top() - epsilon || candidate.bottom() > imageBounds.bottom() + epsilon) {
        error = QStringLiteral("尺寸修改后超出图像边界，原图形未改变。请减小尺寸或先移动图形。");
        return false;
    }
    if (type == QStringLiteral("矩形")) static_cast<QGraphicsRectItem*>(item)->setRect(bounds);
    else if (type == QStringLiteral("圆")) static_cast<QGraphicsEllipseItem*>(item)->setRect(bounds);
    else if (type == QStringLiteral("直线")) static_cast<QGraphicsLineItem*>(item)->setLine(line);
    else {
        static_cast<QGraphicsPathItem*>(item)->setPath(proposed);
        item->setData(3, a); item->setData(4, b); item->setData(5, c);
    }
    refreshCircleRadiusGuide();
    viewport()->update();
    emit featureGeometryChanged(featureId);
    return true;
}

QList<QPair<int, QString>> GraphicalCanvas::featureEntries() const
{
    QMap<int, QString> namesById;
    const QList<QGraphicsItem*> allItems = m_scene->items();
    for (QGraphicsItem* item : allItems) {
        const int featureId = item->data(0).toInt();
        const QString featureName = item->data(2).toString();
        if (featureId > 0 && !featureName.isEmpty())
            namesById.insert(featureId, featureName);
    }

    QList<QPair<int, QString>> entries;
    for (auto iterator = namesById.cbegin(); iterator != namesById.cend(); ++iterator)
        entries.append(qMakePair(iterator.key(), iterator.value()));
    return entries;
}

QVector<GraphicalCanvas::FeatureSnapshot> GraphicalCanvas::featureSnapshots() const
{
    QVector<FeatureSnapshot> result;
    if (!m_imageItem) return result;
    const auto imagePoint = [this](QGraphicsItem* item, const QPointF& local) {
        return m_imageItem->mapFromScene(item->mapToScene(local));
    };
    for (const auto& entry : featureEntries()) {
        QGraphicsItem* item = featureItemById(entry.first);
        if (!item) continue;
        FeatureSnapshot snapshot;
        snapshot.id = entry.first;
        snapshot.type = item->data(1).toString();
        snapshot.rotation = featureRotationAngle(entry.first);
        if (snapshot.type == QStringLiteral("点")) snapshot.points << imagePoint(item, QPointF());
        else if (snapshot.type == QStringLiteral("直线")) {
            const QLineF line = static_cast<QGraphicsLineItem*>(item)->line();
            snapshot.points << imagePoint(item, line.p1()) << imagePoint(item, line.p2());
        }
        else if (snapshot.type == QStringLiteral("矩形") || snapshot.type == QStringLiteral("圆")) {
            const QRectF rect = snapshot.type == QStringLiteral("矩形")
                ? static_cast<QGraphicsRectItem*>(item)->rect()
                : static_cast<QGraphicsEllipseItem*>(item)->rect();
            snapshot.points << imagePoint(item, rect.center());
            snapshot.size = rect.size();
        }
        else if (snapshot.type == QStringLiteral("圆弧")) {
            snapshot.points << imagePoint(item, item->data(3).toPointF())
                << imagePoint(item, item->data(4).toPointF())
                << imagePoint(item, item->data(5).toPointF());
        }
        result.append(snapshot);
    }
    return result;
}

bool GraphicalCanvas::validateFeatureSnapshots(const QVector<FeatureSnapshot>& snapshots,
    const QSize& imageSize, QString& error) const
{
    error.clear();
    if (!imageSize.isValid() || imageSize.isEmpty()) { error = QStringLiteral("工程图像尺寸无效。"); return false; }
    QSet<int> ids;
    const QRectF imageBounds(QPointF(0, 0), imageSize);
    for (const FeatureSnapshot& snapshot : snapshots) {
        const int requiredPoints = snapshot.type == QStringLiteral("点") ? 1
            : snapshot.type == QStringLiteral("直线") ? 2
            : snapshot.type == QStringLiteral("矩形") || snapshot.type == QStringLiteral("圆") ? 1
            : snapshot.type == QStringLiteral("圆弧") ? 3 : -1;
        if (snapshot.id <= 0 || snapshot.id == INT_MAX || ids.contains(snapshot.id) || requiredPoints < 0
            || snapshot.points.size() != requiredPoints || !qIsFinite(snapshot.rotation)) {
            error = QStringLiteral("工程包含无效或重复的图形定义。"); return false;
        }
        ids.insert(snapshot.id);
        for (const QPointF& point : snapshot.points)
            if (!qIsFinite(point.x()) || !qIsFinite(point.y()) || !imageBounds.contains(point)) {
                error = QStringLiteral("图形%1超出图像范围。").arg(snapshot.id); return false;
            }
        if ((snapshot.type == QStringLiteral("矩形") || snapshot.type == QStringLiteral("圆"))
            && (!snapshot.size.isValid() || snapshot.size.width() <= 0 || snapshot.size.height() <= 0)) {
            error = QStringLiteral("图形%1尺寸无效。").arg(snapshot.id); return false;
        }
        if (snapshot.type == QStringLiteral("圆")
            && qAbs(snapshot.size.width() - snapshot.size.height()) > 1e-6) {
            error = QStringLiteral("图形%1不是有效正圆。").arg(snapshot.id); return false;
        }
        if (snapshot.type == QStringLiteral("直线")
            && QLineF(snapshot.points[0], snapshot.points[1]).length() <= 1e-6) {
            error = QStringLiteral("图形%1是零长度直线。").arg(snapshot.id); return false;
        }
        if (snapshot.type == QStringLiteral("圆")) {
            const QRectF circleBounds(snapshot.points[0]
                - QPointF(snapshot.size.width() / 2.0, snapshot.size.height() / 2.0), snapshot.size);
            if (!imageBounds.contains(circleBounds)) {
                error = QStringLiteral("图形%1超出图像范围。").arg(snapshot.id); return false;
            }
        }
        if (snapshot.type == QStringLiteral("矩形")) {
            const QRectF localBounds(-snapshot.size.width() / 2.0, -snapshot.size.height() / 2.0,
                snapshot.size.width(), snapshot.size.height());
            QTransform transform;
            transform.translate(snapshot.points[0].x(), snapshot.points[0].y());
            transform.rotate(snapshot.rotation);
            const QPolygonF corners = transform.map(QPolygonF(localBounds));
            for (const QPointF& corner : corners)
                if (!imageBounds.contains(corner)) {
                    error = QStringLiteral("图形%1超出图像范围。").arg(snapshot.id); return false;
                }
        }
        if (snapshot.type == QStringLiteral("圆弧")) {
            QPainterPath path;
            if (!buildArcPath(snapshot.points[0], snapshot.points[1], snapshot.points[2], path)) {
                error = QStringLiteral("图形%1的圆弧三点无效。").arg(snapshot.id); return false;
            }
            if (!imageBounds.contains(path.boundingRect())) {
                error = QStringLiteral("图形%1的圆弧超出图像范围。").arg(snapshot.id); return false;
            }
        }
    }
    return true;
}

bool GraphicalCanvas::restoreFeatures(const QVector<FeatureSnapshot>& snapshots, QString& error)
{
    if (!m_imageItem) { error = QStringLiteral("工程图像尚未载入。"); return false; }
    if (!validateFeatureSnapshots(snapshots, m_sourceImage.size(), error)) return false;

    const QList<QGraphicsItem*> existing = m_scene->items();
    clearCircleRadiusGuide();
    m_resizeFeatureId = -1; m_rotateFeatureId = -1; m_draggedFeatureItem = nullptr;
    for (QGraphicsItem* item : existing)
        if (item != m_imageItem && item->data(0).toInt() > 0) { m_scene->removeItem(item); delete item; }
    m_nextFeatureId = 1;
    for (const FeatureSnapshot& snapshot : snapshots) {
        QGraphicsItem* item = nullptr;
        if (snapshot.type == QStringLiteral("点")) {
            auto* point = new QGraphicsEllipseItem(QRectF(-4, -4, 8, 8));
            point->setBrush(QColor(255, 80, 200)); point->setPen(makeCanvasPen(QColor(255, 80, 200)));
            point->setPos(snapshot.points[0]); item = point;
            point->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
        }
        else if (snapshot.type == QStringLiteral("直线")) {
            auto* line = new QGraphicsLineItem(QLineF(snapshot.points[0], snapshot.points[1]));
            line->setPen(makeCanvasPen(QColor(255, 100, 80))); item = line;
        }
        else if (snapshot.type == QStringLiteral("矩形") || snapshot.type == QStringLiteral("圆")) {
            const QRectF rect(QPointF(-snapshot.size.width() / 2, -snapshot.size.height() / 2), snapshot.size);
            if (snapshot.type == QStringLiteral("矩形")) {
                auto* rectangle = new QGraphicsRectItem(rect);
                rectangle->setPen(makeCanvasPen(QColor(0, 210, 255))); item = rectangle;
                rectangle->setRotation(snapshot.rotation);
            }
            else {
                auto* circle = new QGraphicsEllipseItem(rect);
                circle->setPen(makeCanvasPen(QColor(0, 255, 120))); item = circle;
            }
            item->setPos(snapshot.points[0]);
        }
        else {
            QPainterPath path;
            buildArcPath(snapshot.points[0], snapshot.points[1], snapshot.points[2], path);
            auto* arc = new QGraphicsPathItem(path);
            arc->setPen(makeCanvasPen(QColor(255, 170, 0)));
            arc->setData(3, snapshot.points[0]); arc->setData(4, snapshot.points[1]); arc->setData(5, snapshot.points[2]);
            item = arc;
        }
        m_scene->addItem(item);
        registerFeature(item, snapshot.type, snapshot.id, false);
    }
    emit featuresChanged();
    return true;
}

void GraphicalCanvas::selectFeatureById(int featureId, bool centerOnFeature)
{
    m_scene->clearSelection();
    QGraphicsItem* item = featureItemById(featureId);
    if (!item)
        return;

    item->setSelected(true);
    if (centerOnFeature)
        ensureVisible(item, 80, 80);
}

void GraphicalCanvas::deleteFeatureById(int featureId)
{
    QGraphicsItem* item = featureItemById(featureId);
    if (!item)
        return;

    m_scene->removeItem(item);
    delete item;
    emit featuresChanged();
}

void GraphicalCanvas::deleteSelectedFeatures()
{
    const QList<QGraphicsItem*> selectedItems = m_scene->selectedItems();
    bool removed = false;
    for (QGraphicsItem* item : selectedItems) {
        if (item == m_imageItem)
            continue;
        m_scene->removeItem(item);
        delete item;
        removed = true;
    }
    if (removed)
        emit featuresChanged();
}

QPointF GraphicalCanvas::boundedImagePoint(const QPoint& viewportPosition) const
{
    QPointF point = mapViewportToImage(viewportPosition);
    if (m_sourceImage.isNull())
        return point;

    point.setX(qBound(0.0, point.x(), static_cast<double>(m_sourceImage.width() - 1)));
    point.setY(qBound(0.0, point.y(), static_cast<double>(m_sourceImage.height() - 1)));
    return point;
}

bool GraphicalCanvas::isPointInsideImage(const QPointF& imagePoint) const
{
    return hasImage()
        && imagePoint.x() >= 0.0
        && imagePoint.y() >= 0.0
        && imagePoint.x() < m_sourceImage.width()
        && imagePoint.y() < m_sourceImage.height();
}
/*直线/矩形/圆：经典三段式——beginPreview（按下建预览项）→ updatePreview（拖动改几何）→ finishPreview（松手转正注册）。矩形/圆各按两种模式计算。*/
void GraphicalCanvas::beginPreview(const QPointF& imagePoint)//beginPreview（按下建预览项）
{
    m_drawingStart = imagePoint;
    const QPen previewPen = makeCanvasPen(QColor(255, 230, 0), 2.0, Qt::DashLine);

    switch (m_drawingTool) {
    case DrawingTool::Line:
        m_previewItem = m_scene->addLine(QLineF(imagePoint, imagePoint), previewPen);
        break;
    case DrawingTool::Rectangle:
        m_previewItem = m_scene->addRect(QRectF(imagePoint, imagePoint), previewPen);
        break;
    case DrawingTool::Circle:
        m_previewItem = m_scene->addEllipse(QRectF(imagePoint, imagePoint), previewPen);
        break;
    default:
        break;
    }

    if (m_previewItem)
        m_previewItem->setZValue(20.0);
    if (m_drawingTool == DrawingTool::Circle && m_previewItem)
        showCircleRadiusGuide(m_previewItem, QRectF(imagePoint, imagePoint));
}

void GraphicalCanvas::updatePreview(const QPointF& imagePoint)//updatePreview（拖动改几何）
{
    if (!m_previewItem)
        return;

    if (m_drawingTool == DrawingTool::Line) {
        static_cast<QGraphicsLineItem*>(m_previewItem)->setLine(QLineF(m_drawingStart, imagePoint));
        return;
    }

    if (m_drawingTool == DrawingTool::Circle) {
        const qreal deltaX = imagePoint.x() - m_drawingStart.x();
        const qreal deltaY = imagePoint.y() - m_drawingStart.y();
        QRectF circleRect;
        if (m_circleDrawingMode == CircleDrawingMode::CenterRadius) {
            qreal radius = QLineF(m_drawingStart, imagePoint).length();
            radius = qMin(radius, qMin(qMin(m_drawingStart.x(),
                m_sourceImage.width() - 1.0 - m_drawingStart.x()),
                qMin(m_drawingStart.y(),
                    m_sourceImage.height() - 1.0 - m_drawingStart.y())));
            circleRect = QRectF(m_drawingStart.x() - radius,
                m_drawingStart.y() - radius, radius * 2.0, radius * 2.0);
        }
        else {
            const qreal side = qMin(qAbs(deltaX), qAbs(deltaY));
            const qreal left = deltaX >= 0.0 ? m_drawingStart.x() : m_drawingStart.x() - side;
            const qreal top = deltaY >= 0.0 ? m_drawingStart.y() : m_drawingStart.y() - side;
            circleRect = QRectF(left, top, side, side);
        }
        QGraphicsEllipseItem* previewCircle =
            static_cast<QGraphicsEllipseItem*>(m_previewItem);
        previewCircle->setRect(circleRect);
        if (m_circleRadiusGuide) {
            const QPointF center = circleRect.center();
            QLineF direction(center, imagePoint);
            if (qFuzzyIsNull(direction.length()))
                direction = QLineF(center, QPointF(circleRect.right(), center.y()));
            else
                direction.setLength(circleRect.width() / 2.0);
            m_circleRadiusGuide->setLine(direction);
        }
        else {
            showCircleRadiusGuide(previewCircle, circleRect);
        }
        viewport()->update();
        return;
    }

    QRectF bounds;
    if (m_rectangleDrawingMode == RectangleDrawingMode::CenterOutward) {
        const qreal halfWidth = qMin(qAbs(imagePoint.x() - m_drawingStart.x()),
            qMin(m_drawingStart.x(), m_sourceImage.width() - 1.0 - m_drawingStart.x()));
        const qreal halfHeight = qMin(qAbs(imagePoint.y() - m_drawingStart.y()),
            qMin(m_drawingStart.y(), m_sourceImage.height() - 1.0 - m_drawingStart.y()));
        bounds = QRectF(m_drawingStart.x() - halfWidth,
            m_drawingStart.y() - halfHeight, halfWidth * 2.0, halfHeight * 2.0);
    }
    else {
        bounds = QRectF(m_drawingStart, imagePoint).normalized();
    }
    static_cast<QGraphicsRectItem*>(m_previewItem)->setRect(bounds);
}

void GraphicalCanvas::finishPreview(const QPointF& imagePoint, const QPoint& viewportPosition)//finishPreview（松手转正注册）
{
    if (!m_previewItem)
        return;

    updatePreview(imagePoint);
    QGraphicsItem* completedItem = m_previewItem;
    m_previewItem = nullptr;
    viewport()->update();

    const int dragDistance = (viewportPosition - m_drawingStartViewport).manhattanLength();
    if (dragDistance < QApplication::startDragDistance()) {
        if (m_circleRadiusGuide && m_circleRadiusGuide->parentItem() == completedItem)
            m_circleRadiusGuide = nullptr;
        m_scene->removeItem(completedItem);
        delete completedItem;
        refreshCircleRadiusGuide();
        return;
    }

    if (m_drawingTool == DrawingTool::Circle) {
        QGraphicsEllipseItem* previewCircle = static_cast<QGraphicsEllipseItem*>(completedItem);
        if (m_circleRadiusGuide && m_circleRadiusGuide->parentItem() == previewCircle)
            clearCircleRadiusGuide();
        previewCircle->setPen(makeCanvasPen(QColor(0, 255, 120)));
        registerFeature(completedItem, QStringLiteral("圆"));
        refreshCircleRadiusGuide();
    }
    else if (m_drawingTool == DrawingTool::Rectangle) {
        static_cast<QGraphicsRectItem*>(completedItem)->setPen(makeCanvasPen(QColor(0, 210, 255)));
        registerFeature(completedItem, QStringLiteral("矩形"));
    }
    else if (m_drawingTool == DrawingTool::Line) {
        static_cast<QGraphicsLineItem*>(completedItem)->setPen(makeCanvasPen(QColor(255, 100, 80)));
        registerFeature(completedItem, QStringLiteral("直线"));
    }
}

void GraphicalCanvas::registerFeature(QGraphicsItem* item, const QString& typeName,
    int restoredId, bool notify)//每个图形创建后都需要registerFeature:分配自增 ID（存在 item 的 data(0)）、类型名存 data(1)、发 featureAdded 信号。​图形 ID 就是 Editor 里 geometryId 的来源——这是两个模块之间的纽带。
{
    if (!item)
        return;

    const int featureId = restoredId > 0 ? restoredId : m_nextFeatureId++;
    m_nextFeatureId = qMax(m_nextFeatureId, featureId + 1);
    const QString featureName = QStringLiteral("%1_%2").arg(typeName).arg(featureId);
    item->setData(0, featureId);
    item->setData(1, typeName);
    item->setData(2, featureName);
    item->setFlag(QGraphicsItem::ItemIsSelectable, true);
    item->setFlag(QGraphicsItem::ItemIsMovable, true);
    item->setZValue(10.0);
    if (notify) {
        emit featureAdded(featureId, featureName, typeName);
        emit featuresChanged();
    }
}

QGraphicsItem* GraphicalCanvas::featureItemById(int featureId) const
{
    const QList<QGraphicsItem*> allItems = m_scene->items();
    for (QGraphicsItem* item : allItems) {
        if (item->data(0).toInt() == featureId)
            return item;
    }
    return nullptr;
}

QGraphicsItem* GraphicalCanvas::featureAtViewportPosition(const QPoint& position) const
{
    // Shared screen-space tolerance and stacking order for left and right clicks.
    const int hitRadius = 7;
    const QRect hitArea(position - QPoint(hitRadius, hitRadius),
        QSize(hitRadius * 2 + 1, hitRadius * 2 + 1));
    const QList<QGraphicsItem*> nearbyItems = items(hitArea, Qt::IntersectsItemShape);
    for (QGraphicsItem* item : nearbyItems) {
        if (item->data(0).toInt() > 0)
            return item;
    }
    return nullptr;
}

void GraphicalCanvas::constrainSelectedFeaturesToImage()//负责移动限位——图形不能脱出图像边界
{
    if (!m_imageItem)
        return;

    const QRectF imageBounds = m_imageItem->sceneBoundingRect();
    const QList<QGraphicsItem*> selectedItems = m_scene->selectedItems();
    for (QGraphicsItem* item : selectedItems) {
        if (item == m_imageItem || item->data(0).toInt() <= 0)
            continue;

        // Limit the ROI geometry, not its pen, selection box or control handles.
        // Mapping a local bounding box would include empty space after rotation.
        const QString type = item->data(1).toString();
        QRectF itemBounds;
        if (type == QStringLiteral("点")) {
            const QPointF point = item->mapToScene(QPointF(0.0, 0.0));
            itemBounds = QRectF(point, point);
        }
        else if (type == QStringLiteral("直线")) {
            const QLineF line = static_cast<QGraphicsLineItem*>(item)->line();
            itemBounds = QRectF(item->mapToScene(line.p1()),
                item->mapToScene(line.p2())).normalized();
        }
        else {
            QPainterPath geometry;
            if (type == QStringLiteral("矩形"))
                geometry.addRect(static_cast<QGraphicsRectItem*>(item)->rect());
            else if (type == QStringLiteral("圆"))
                geometry.addEllipse(static_cast<QGraphicsEllipseItem*>(item)->rect());
            else if (type == QStringLiteral("圆弧"))
                geometry = static_cast<QGraphicsPathItem*>(item)->path();
            else
                continue;
            itemBounds = item->mapToScene(geometry).boundingRect();
        }
        qreal deltaX = 0.0;
        qreal deltaY = 0.0;
        if (itemBounds.left() < imageBounds.left())
            deltaX = imageBounds.left() - itemBounds.left();
        else if (itemBounds.right() > imageBounds.right())
            deltaX = imageBounds.right() - itemBounds.right();

        if (itemBounds.top() < imageBounds.top())
            deltaY = imageBounds.top() - itemBounds.top();
        else if (itemBounds.bottom() > imageBounds.bottom())
            deltaY = imageBounds.bottom() - itemBounds.bottom();

        if (!qFuzzyIsNull(deltaX) || !qFuzzyIsNull(deltaY))
            item->moveBy(deltaX, deltaY);
    }
}

bool GraphicalCanvas::measurementRoi(int featureId, MeasurementRoi& roi) const//把一个图形转成试测用的 ROI 结构（圆：圆心+半径；矩形：角点+是否轴对齐），Editor 试测时拿它生成 HALCON region。
{
    roi = MeasurementRoi();
    QGraphicsItem* item = featureItemById(featureId);
    if (!item || !m_imageItem) return false;
    if (item->data(1).toString() == QStringLiteral("圆")) {
        const QRectF rect = static_cast<QGraphicsEllipseItem*>(item)->rect();
        roi.isCircle = true;
        roi.center = m_imageItem->mapFromScene(item->mapToScene(rect.center()));
        const QPointF edge = m_imageItem->mapFromScene(item->mapToScene(
            QPointF(rect.right(), rect.center().y())));
        roi.radius = QLineF(roi.center, edge).length();
        return qIsFinite(roi.radius) && roi.radius >= 1;
    }
    if (item->data(1).toString() != QStringLiteral("矩形")) return false;
    const QRectF rect = static_cast<QGraphicsRectItem*>(item)->rect();
    if (rect.width() < 2 || rect.height() < 2) return false;
    for (const QPointF& corner : {rect.topLeft(), rect.topRight(), rect.bottomRight(), rect.bottomLeft()})
        roi.corners.append(m_imageItem->mapFromScene(item->mapToScene(corner)));
    const qreal angle = item->rotation();
    roi.axisAligned = qAbs(angle / 90.0 - qRound(angle / 90.0)) <= 1e-8;
    return true;
}

void GraphicalCanvas::setDetectionOverlay(const QPainterPath& edges, const QPainterPath& fitted)//把试测得到的边缘轮廓和拟合结果存下来，在 drawForeground（第 1010 行）里叠加画到图像上——试测后你能在画布上看到检测到的边缘和拟合的弧。
{
    m_detectionEdges = edges;
    m_detectionFitted = fitted;
    viewport()->update();
}

void GraphicalCanvas::drawForeground(QPainter* painter, const QRectF& rect)//在 drawForeground里叠加画到图像上——试测后你能在画布上看到检测到的边缘和拟合的弧。
{
    QGraphicsView::drawForeground(painter, rect);
    painter->save();
    painter->setBrush(Qt::NoBrush);
    painter->setPen(makeCanvasPen(QColor(0, 255, 120), 2));
    painter->drawPath(m_detectionEdges);
    painter->setPen(makeCanvasPen(QColor(255, 100, 30), 2));
    painter->drawPath(m_detectionFitted);
    painter->restore();
    if (m_drawingTool == DrawingTool::Select) {
        const QList<QGraphicsItem*> selected = m_scene->selectedItems();
        if (selected.size() == 1) {
            painter->save();
            painter->setPen(makeCanvasPen(QColor(20, 40, 60), 1.0));
            painter->setBrush(QColor(255, 220, 60));
            const qreal halfSize = 5.0 / qMax<qreal>(0.0001, qAbs(transform().m11()));
            for (const QPointF& local : resizeHandlePoints(selected.first())) {
                const QPointF center = selected.first()->mapToScene(local);
                painter->drawRect(QRectF(center.x() - halfSize, center.y() - halfSize,
                    halfSize * 2, halfSize * 2));
            }
            QPointF center, anchor, handle;
            if (rotationHandleGeometry(selected.first(), center, anchor, handle)) {
                painter->setPen(makeCanvasPen(QColor(80, 210, 255), 1.5));
                painter->drawLine(anchor, handle);
                painter->setBrush(QColor(80, 210, 255));
                painter->drawEllipse(handle, halfSize * 1.2, halfSize * 1.2);
            }
            painter->restore();
        }
    }
    if (!m_previewItem || m_drawingTool != DrawingTool::Circle
        || m_circleDrawingMode != CircleDrawingMode::BoundingBox)
        return;

    const QGraphicsEllipseItem* circle =
        qgraphicsitem_cast<QGraphicsEllipseItem*>(m_previewItem);
    if (!circle)
        return;

    painter->save();
    painter->setBrush(Qt::NoBrush);
    painter->setPen(makeCanvasPen(QColor(180, 210, 230, 180), 1.0, Qt::DashLine));
    painter->drawRect(circle->mapRectToScene(circle->rect()));
    // Keep the fixed starting-corner marker approximately 10 screen pixels wide.
    const qreal markerRadius = 5.0 / qMax<qreal>(0.0001, qAbs(transform().m11()));
    painter->setPen(makeCanvasPen(QColor(255, 230, 0), 2.0));
    painter->drawLine(m_drawingStart + QPointF(-markerRadius, 0),
        m_drawingStart + QPointF(markerRadius, 0));
    painter->drawLine(m_drawingStart + QPointF(0, -markerRadius),
        m_drawingStart + QPointF(0, markerRadius));
    painter->restore();
}

void GraphicalCanvas::wheelEvent(QWheelEvent* event)//滚轮缩放
{
    if (!m_imageItem || event->angleDelta().y() == 0) {
        QGraphicsView::wheelEvent(event);
        return;
    }

    const bool zoomingIn = event->angleDelta().y() > 0;
    const int nextZoomStep = m_zoomStep + (zoomingIn ? 1 : -1);
    if (nextZoomStep < -8 || nextZoomStep > 24) {
        event->accept();
        return;
    }

    scale(zoomingIn ? 1.15 : 1.0 / 1.15,
          zoomingIn ? 1.15 : 1.0 / 1.15);
    m_zoomStep = nextZoomStep;
    event->accept();
}

void GraphicalCanvas::mousePressEvent(QMouseEvent* event)
{
    setFocus(Qt::MouseFocusReason);
    if (event->button() == Qt::MiddleButton
        || (event->button() == Qt::LeftButton && m_spacePressed)) {//中键或者空格+左键平移
        m_rotateFeatureId = -1;
        m_resizeFeatureId = -1;
        m_panning = true;
        m_lastPanPosition = event->pos();
        viewport()->setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton//圆弧:三点式（起点→弧上点→终点），buildArcPath 用三点求圆生成 QPainterPath；三点太近或共线会拒绝并提示"按 Esc 可取消"。
        && m_drawingTool == DrawingTool::Arc) {
        const QPointF imagePoint = mapViewportToImage(event->pos());
        if (!isPointInsideImage(imagePoint)) {
            event->accept();
            return;
        }

        if (m_arcPoints.size() < 2) {
            m_arcPoints.append(imagePoint);
            updateArcPreview(imagePoint);
            emit canvasMessage(m_arcPoints.size() == 1
                ? QStringLiteral("已记录圆弧起点，请点击弧上点")
                : QStringLiteral("已记录弧上点，请点击圆弧终点"));
        }
        else {
            QPainterPath arcPath;
            if (!buildArcPath(m_arcPoints.at(0), m_arcPoints.at(1), imagePoint, arcPath)) {
                emit canvasMessage(QStringLiteral("三点过近或接近共线，请重新选择圆弧终点；按 Esc 可取消"));
                updateArcPreview(imagePoint);
            }
            else {
                QGraphicsPathItem* arcItem = m_scene->addPath(
                    arcPath, makeCanvasPen(QColor(255, 170, 0)));
                arcItem->setData(3, m_arcPoints.at(0));
                arcItem->setData(4, m_arcPoints.at(1));
                arcItem->setData(5, imagePoint);
                registerFeature(arcItem, QStringLiteral("圆弧"));
                cancelArcDraft();
                emit canvasMessage(QStringLiteral("圆弧已创建"));
            }
        }
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton//select工具下点按时按照优先级判断
        && m_drawingTool == DrawingTool::Select) {
        QGraphicsItem* rotationItem = nullptr;//旋转柄（rotationHandleAt）：命中则记录中心、起始角度，拖动时 updateHandleRotation 实时旋转，Esc 恢复。
        if (rotationHandleAt(event->pos(), rotationItem)) {
            QPointF anchor, handle;
            rotationHandleGeometry(rotationItem, m_rotationCenterScene, anchor, handle);
            const QPointF delta = mapToScene(event->pos()) - m_rotationCenterScene;
            m_rotationPressAngle = qRadiansToDegrees(qAtan2(delta.y(), delta.x()));
            m_rotateFeatureId = rotationItem->data(0).toInt();
            m_rotationStartAngle = featureRotationAngle(m_rotateFeatureId);
            m_resizeFeatureId = -1;
            m_draggedFeatureItem = nullptr;
            viewport()->setCursor(Qt::ClosedHandCursor);
            emit canvasMessage(QStringLiteral("旋转中：绕中心拖动，Esc恢复本次旋转前角度"));
            event->accept(); return;
        }
        QGraphicsItem* handleItem = nullptr;//尺寸柄（resizeHandleAt）：8 个角点柄，拖动按初始向量比例缩放，矩形可锁宽高比。
        const int handleIndex = resizeHandleAt(event->pos(), handleItem);
        if (handleIndex >= 0) {
            const QSizeF size = featureDimensions(handleItem->data(0).toInt());
            if (size.width() <= 0) { event->accept(); return; }
            QPointF center;
            const QString type = handleItem->data(1).toString();
            if (type == QStringLiteral("矩形")) center = static_cast<QGraphicsRectItem*>(handleItem)->rect().center();
            else if (type == QStringLiteral("圆")) center = static_cast<QGraphicsEllipseItem*>(handleItem)->rect().center();
            else if (type == QStringLiteral("直线")) {
                const QLineF line = static_cast<QGraphicsLineItem*>(handleItem)->line();
                center = (line.p1() + line.p2()) / 2.0;
            }
            else {
                qreal radius;
                if (!arcCircle(handleItem->data(3).toPointF(), handleItem->data(4).toPointF(),
                    handleItem->data(5).toPointF(), center, radius)) { event->accept(); return; }
            }
            m_resizeFeatureId = handleItem->data(0).toInt();
            m_resizeStartSize = size;
            m_resizeVector = resizeHandlePoints(handleItem).at(handleIndex) - center;
            m_resizePressLocal = handleItem->mapFromScene(mapToScene(event->pos()));
            m_dragRatioLocked = m_resizeRatioLocked;
            m_draggedFeatureItem = nullptr;
            viewport()->setCursor(Qt::SizeAllCursor);
            event->accept();
            return;
        }
        QGraphicsItem* hitFeature = featureAtViewportPosition(event->pos());

        if (hitFeature) {
            m_scene->clearSelection();
            hitFeature->setSelected(true);
            m_draggedFeatureItem = hitFeature;
            m_lastFeatureDragScenePosition = mapToScene(event->pos());
            event->accept();
            return;
        }
    }

    if (event->button() == Qt::LeftButton
        && m_drawingTool != DrawingTool::Select
        && m_drawingTool != DrawingTool::Arc) {
        const QPointF imagePoint = mapViewportToImage(event->pos());
        if (!isPointInsideImage(imagePoint)) {
            event->accept();
            return;
        }

        if (m_drawingTool == DrawingTool::Point) {//点：单击直接放一个 8px 的小圆点，
            QGraphicsEllipseItem* pointItem = m_scene->addEllipse(
                QRectF(-4.0, -4.0, 8.0, 8.0),
                makeCanvasPen(QColor(255, 80, 200)),
                QBrush(QColor(255, 80, 200)));
            pointItem->setPos(imagePoint);
            pointItem->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);//ItemIgnoresTransformations 让它不随缩放变大
            registerFeature(pointItem, QStringLiteral("点"));
        }
        else {
            m_drawingStartViewport = event->pos();
            beginPreview(imagePoint);
        }
        event->accept();
        return;
    }

    QGraphicsView::mousePressEvent(event);
}

void GraphicalCanvas::mouseMoveEvent(QMouseEvent* event)
{
    if (m_panning) {
        const QPoint delta = event->pos() - m_lastPanPosition;
        m_lastPanPosition = event->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        event->accept();
        return;
    }

    if (m_rotateFeatureId > 0 && (event->buttons() & Qt::LeftButton)) {
        updateHandleRotation(event->pos());
        event->accept(); return;
    }
    if (m_resizeFeatureId > 0 && (event->buttons() & Qt::LeftButton)) {
        updateHandleResize(event->pos());
        event->accept();
        return;
    }

    if (m_draggedFeatureItem && (event->buttons() & Qt::LeftButton)) {
        const QPointF currentScenePosition = mapToScene(event->pos());
        const QPointF delta = currentScenePosition - m_lastFeatureDragScenePosition;
        m_lastFeatureDragScenePosition = currentScenePosition;
        m_draggedFeatureItem->moveBy(delta.x(), delta.y());
        constrainSelectedFeaturesToImage();
        refreshCircleRadiusGuide();
        viewport()->update();
        event->accept();
        return;
    }

    if (m_previewItem) {
        updatePreview(boundedImagePoint(event->pos()));
        event->accept();
        return;
    }

    if (m_drawingTool == DrawingTool::Arc && !m_arcPoints.isEmpty()) {
        updateArcPreview(boundedImagePoint(event->pos()));
        event->accept();
        return;
    }

    if (event->buttons() == Qt::NoButton && m_drawingTool == DrawingTool::Select) {
        QGraphicsItem* item = nullptr;
        viewport()->setCursor(m_spacePressed ? Qt::OpenHandCursor
            : rotationHandleAt(event->pos(), item) ? Qt::OpenHandCursor
            : resizeHandleAt(event->pos(), item) >= 0 ? Qt::SizeAllCursor : Qt::ArrowCursor);
    }
    QGraphicsView::mouseMoveEvent(event);
    if (m_drawingTool == DrawingTool::Select
        && (event->buttons() & Qt::LeftButton)) {
        constrainSelectedFeaturesToImage();
    }
}

void GraphicalCanvas::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_rotateFeatureId > 0) {
        updateHandleRotation(event->pos());
        m_rotateFeatureId = -1;
        viewport()->setCursor(m_spacePressed ? Qt::OpenHandCursor : Qt::ArrowCursor);
        event->accept(); return;
    }
    if (event->button() == Qt::LeftButton && m_resizeFeatureId > 0) {
        updateHandleResize(event->pos());
        m_resizeFeatureId = -1;
        viewport()->setCursor(m_spacePressed ? Qt::OpenHandCursor : Qt::ArrowCursor);
        event->accept();
        return;
    }
    if (m_panning
        && (event->button() == Qt::MiddleButton || event->button() == Qt::LeftButton)) {
        m_panning = false;
        viewport()->setCursor(m_spacePressed
            ? Qt::OpenHandCursor
            : (m_drawingTool == DrawingTool::Select ? Qt::ArrowCursor : Qt::CrossCursor));
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton && m_draggedFeatureItem) {
        constrainSelectedFeaturesToImage();
        const int featureId = m_draggedFeatureItem->data(0).toInt();
        m_draggedFeatureItem = nullptr;
        if (featureId > 0)
            emit featureGeometryChanged(featureId);
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton && m_previewItem) {
        finishPreview(boundedImagePoint(event->pos()), event->pos());
        event->accept();
        return;
    }

    QGraphicsView::mouseReleaseEvent(event);
    if (event->button() == Qt::LeftButton
        && m_drawingTool == DrawingTool::Select) {
        constrainSelectedFeaturesToImage();
        const QList<QGraphicsItem*> selectedItems = m_scene->selectedItems();
        for (QGraphicsItem* item : selectedItems) {
            const int featureId = item->data(0).toInt();
            if (featureId > 0)
                emit featureGeometryChanged(featureId);
        }
    }
}

void GraphicalCanvas::contextMenuEvent(QContextMenuEvent* event)
{
    QGraphicsItem* clickedItem = featureAtViewportPosition(event->pos());
    const int featureId = clickedItem ? clickedItem->data(0).toInt() : -1;
    if (featureId <= 0) {
        QGraphicsView::contextMenuEvent(event);
        return;
    }

    selectFeatureById(featureId, false);
    QMenu menu(this);
    QAction* locateAction = menu.addAction(QStringLiteral("定位到图形"));
    QAction* deleteAction = menu.addAction(QStringLiteral("删除图形"));
    QAction* chosenAction = menu.exec(event->globalPos());
    if (chosenAction == locateAction)
        ensureVisible(clickedItem, 80, 80);
    else if (chosenAction == deleteAction)
        deleteFeatureById(featureId);
    event->accept();
}

void GraphicalCanvas::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape && m_rotateFeatureId > 0) {
        const int id = m_rotateFeatureId;
        m_rotateFeatureId = -1;
        QString error;
        if (rotateFeature(id, m_rotationStartAngle, error))
            emit canvasMessage(QStringLiteral("已取消本次旋转拖动"));
        else emit canvasMessage(error);
        viewport()->setCursor(Qt::ArrowCursor);
        event->accept(); return;
    }
    if (event->key() == Qt::Key_Escape && m_resizeFeatureId > 0) {
        const int id = m_resizeFeatureId;
        m_resizeFeatureId = -1;
        QString error;
        if (resizeFeature(id, m_resizeStartSize.width(), m_resizeStartSize.height(), error))
            emit canvasMessage(QStringLiteral("已取消本次尺寸拖动"));
        else
            emit canvasMessage(error);
        viewport()->setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Escape && !m_arcPoints.isEmpty()) {
        cancelArcDraft();
        emit canvasMessage(QStringLiteral("已取消当前圆弧绘制"));
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_spacePressed = true;
        if (!m_panning)
            viewport()->setCursor(Qt::OpenHandCursor);
        event->accept();
        return;
    }
    QGraphicsView::keyPressEvent(event);
}

void GraphicalCanvas::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_spacePressed = false;
        if (!m_panning) {
            viewport()->setCursor(m_drawingTool == DrawingTool::Select
                ? Qt::ArrowCursor
                : Qt::CrossCursor);
        }
        event->accept();
        return;
    }
    QGraphicsView::keyReleaseEvent(event);
}

void GraphicalCanvas::focusOutEvent(QFocusEvent* event)
{
    m_rotateFeatureId = -1;
    m_resizeFeatureId = -1;
    m_draggedFeatureItem = nullptr;
    m_spacePressed = false;
    m_panning = false;
    viewport()->setCursor(m_drawingTool == DrawingTool::Select
        ? Qt::ArrowCursor
        : Qt::CrossCursor);
    QGraphicsView::focusOutEvent(event);
}
