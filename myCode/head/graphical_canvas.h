#pragma once

#include <QGraphicsView>
#include <QImage>
#include <QPoint>
#include <QPair>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QSizeF>
#include <QPainterPath>
#include <QPolygonF>

class QGraphicsItem;
class QGraphicsLineItem;
class QGraphicsPixmapItem;
class QGraphicsPathItem;
class QGraphicsScene;
class QContextMenuEvent;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QPainterPath;
class QWheelEvent;

class GraphicalCanvas : public QGraphicsView
{
    Q_OBJECT

public:
    enum class DrawingTool {
        Select,
        Point,
        Line,
        Rectangle,
        Circle,
        Arc
    };
    enum class RectangleDrawingMode { CornerToCorner, CenterOutward };
    enum class CircleDrawingMode { BoundingBox, CenterRadius };

    explicit GraphicalCanvas(QWidget* parent = nullptr);

    bool loadImage(const QString& filePath);
    void setImage(const QImage& image);
    void clearImage();
    void fitImageInView();

    bool hasImage() const;
    QImage sourceImage() const;
    QPointF mapViewportToImage(const QPoint& viewportPosition) const;
    void setDrawingTool(DrawingTool tool);
    DrawingTool drawingTool() const;
    void setRectangleDrawingMode(RectangleDrawingMode mode);
    void setCircleDrawingMode(CircleDrawingMode mode);
    QStringList featureNames() const;
    QStringList featureProperties(int featureId) const;
    QSizeF featureDimensions(int featureId) const;
    bool resizeFeature(int featureId, qreal primary, qreal secondary, QString& error);
    qreal featureRotationAngle(int featureId) const;
    bool rotateFeature(int featureId, qreal angle, QString& error);
    void setResizeRatioLocked(bool locked);
    QList<QPair<int, QString>> featureEntries() const;
    struct FeatureSnapshot {
        int id = 0;
        QString type;
        QVector<QPointF> points;
        QSizeF size;
        qreal rotation = 0;
    };
    QVector<FeatureSnapshot> featureSnapshots() const;
    bool validateFeatureSnapshots(const QVector<FeatureSnapshot>& snapshots,
        const QSize& imageSize, QString& error) const;
    bool restoreFeatures(const QVector<FeatureSnapshot>& snapshots, QString& error);
    void selectFeatureById(int featureId, bool centerOnFeature = true);
    void deleteFeatureById(int featureId);
    void deleteSelectedFeatures();
    struct MeasurementRoi {
        bool isCircle = false;
        bool axisAligned = false;
        QPolygonF corners;
        QPointF center;
        qreal radius = 0;
    };
    bool measurementRoi(int featureId, MeasurementRoi& roi) const;
    void setDetectionOverlay(const QPainterPath& edges, const QPainterPath& fitted);

signals:
    void featureAdded(int featureId, const QString& featureName, const QString& featureType);
    void featuresChanged();
    void selectedFeatureChanged(int featureId);
    void featureGeometryChanged(int featureId);
    void canvasMessage(const QString& message);

protected:
    void drawForeground(QPainter* painter, const QRectF& rect) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

private:
    QPointF boundedImagePoint(const QPoint& viewportPosition) const;
    bool isPointInsideImage(const QPointF& imagePoint) const;
    void beginPreview(const QPointF& imagePoint);
    void updatePreview(const QPointF& imagePoint);
    void finishPreview(const QPointF& imagePoint, const QPoint& viewportPosition);
    bool buildArcPath(const QPointF& startPoint, const QPointF& middlePoint,
        const QPointF& endPoint, QPainterPath& path) const;
    void updateArcPreview(const QPointF& currentPoint);
    void cancelArcDraft();
    void showCircleRadiusGuide(QGraphicsItem* circleItem, const QRectF& circleRect);
    void clearCircleRadiusGuide();
    void refreshCircleRadiusGuide();
    void registerFeature(QGraphicsItem* item, const QString& typeName,
        int restoredId = 0, bool notify = true);
    QGraphicsItem* featureItemById(int featureId) const;
    QGraphicsItem* featureAtViewportPosition(const QPoint& position) const;
    void constrainSelectedFeaturesToImage();
    QVector<QPointF> resizeHandlePoints(QGraphicsItem* item) const;
    int resizeHandleAt(const QPoint& viewportPoint, QGraphicsItem*& item) const;
    void updateHandleResize(const QPoint& viewportPoint);
    bool rotationHandleGeometry(QGraphicsItem* item, QPointF& center, QPointF& anchor, QPointF& handle) const;
    bool rotationHandleAt(const QPoint& viewportPoint, QGraphicsItem*& item) const;
    void updateHandleRotation(const QPoint& viewportPoint);

    QGraphicsScene* m_scene = nullptr;
    QGraphicsPixmapItem* m_imageItem = nullptr;
    QImage m_sourceImage;
    bool m_panning = false;
    bool m_spacePressed = false;
    QPoint m_lastPanPosition;
    int m_zoomStep = 0;
    DrawingTool m_drawingTool = DrawingTool::Select;
    RectangleDrawingMode m_rectangleDrawingMode = RectangleDrawingMode::CornerToCorner;
    CircleDrawingMode m_circleDrawingMode = CircleDrawingMode::BoundingBox;
    QPointF m_drawingStart;
    QPoint m_drawingStartViewport;
    QGraphicsItem* m_previewItem = nullptr;
    QGraphicsLineItem* m_circleRadiusGuide = nullptr;
    QGraphicsPathItem* m_arcPreviewItem = nullptr;
    QVector<QPointF> m_arcPoints;
    QGraphicsItem* m_draggedFeatureItem = nullptr;
    QPointF m_lastFeatureDragScenePosition;
    int m_nextFeatureId = 1;
    bool m_resizeRatioLocked = true;
    bool m_dragRatioLocked = true;
    int m_resizeFeatureId = -1;
    QSizeF m_resizeStartSize;
    QPointF m_resizePressLocal;
    QPointF m_resizeVector;
    int m_rotateFeatureId = -1;
    QPointF m_rotationCenterScene;
    qreal m_rotationStartAngle = 0.0;
    qreal m_rotationPressAngle = 0.0;
    QPainterPath m_detectionEdges;
    QPainterPath m_detectionFitted;
};
