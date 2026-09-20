#ifndef XN_CANVASITEM_H
#define XN_CANVASITEM_H

#include "app/documentcontroller.h"
#include "model/path.h"
#include "tools/strokebuilder.h"

#include <QBasicTimer>
#include <QImage>
#include <QQuickPaintedItem>

namespace xn {
class CanvasItem : public QQuickPaintedItem
{
    Q_OBJECT

    Q_PROPERTY(xn::DocumentController *controller READ controller WRITE setController NOTIFY controllerChanged)
    Q_PROPERTY(int pageIndex READ pageIndex WRITE setPageIndex NOTIFY pageIndexChanged)
    Q_PROPERTY(qreal zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(bool drawingEnabled READ drawingEnabled WRITE setDrawingEnabled NOTIFY drawingEnabledChanged)

    Q_PROPERTY(qreal eraserRadius READ eraserRadius WRITE setEraserRadius NOTIFY eraserRadiusChanged)

    Q_PROPERTY(qreal palmThreshold READ palmThreshold WRITE setPalmThreshold NOTIFY palmThresholdChanged)

    Q_PROPERTY(bool pressureAvailable READ pressureAvailable NOTIFY pressureAvailableChanged)

    Q_PROPERTY(bool drawing READ isDrawing NOTIFY drawingChanged)

public:
    explicit CanvasItem(QQuickItem *parent = 0);
    ~CanvasItem();

    void paint(QPainter *painter);

    DocumentController *controller() const { return m_controller; }
    void setController(DocumentController *c);

    int pageIndex() const { return m_pageIndex; }
    void setPageIndex(int index);

    qreal zoom() const { return m_zoom; }
    void setZoom(qreal z);

    bool drawingEnabled() const { return m_drawingEnabled; }
    void setDrawingEnabled(bool on);

    qreal eraserRadius() const { return m_eraserRadius; }
    void setEraserRadius(qreal r);

    qreal palmThreshold() const { return m_palmThreshold; }
    void setPalmThreshold(qreal t);

    bool pressureAvailable() const { return m_pressureAvailable; }
    bool isDrawing() const
    { return m_builder.isActive() || m_erasing || m_shapeStroke != 0 || m_marqueeActive
             || !m_curveAnchors.isEmpty() || m_draggingNode || m_scalingSelection
             || m_lassoActive; }

    Q_INVOKABLE void refresh();

Q_SIGNALS:
    void controllerChanged();
    void pageIndexChanged();
    void zoomChanged();
    void drawingEnabledChanged();
    void eraserRadiusChanged();
    void palmThresholdChanged();
    void pressureAvailableChanged();
    void drawingChanged();
    void strokeFinished();

protected:
    void touchEvent(QTouchEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry);
    void timerEvent(QTimerEvent *event);

private Q_SLOTS:
    void onPageContentChanged(int pageIndex);
    void onDocumentReplaced();
    void onToolChanged();

private:
    void rebuildCache();
    void invalidateCache();
    void configureBuilder();

    qreal snapRadius() const;
    QPointF toPage(const QPointF &itemPos) const;
    QRect pageRectToItem(const QRectF &pageRect, qreal padPoints) const;

    void applyStyle(Stroke *s) const;
    void applyStyle(Path *p) const;

    Element *buildCurve(const QVector<QPointF> &pts, bool closed) const;
    bool curveClosesByDefault() const;
    void curveTap(const QPointF &snappedPos);
    void commitCurve(bool closed);
    void discardCurve();

    void inputBegin(const QPointF &itemPos, qreal pressure, qint64 timeMs);
    void inputMove(const QPointF &itemPos, qreal pressure, qint64 timeMs);
    void inputEnd();
    void inputCancel();

    void notePressure(qreal pressure);

    DocumentController *m_controller;
    int m_pageIndex;
    qreal m_zoom;
    bool m_drawingEnabled;
    qreal m_eraserRadius;
    qreal m_palmThreshold;
    bool m_pressureAvailable;
    bool m_erasing;

    StrokeBuilder m_builder;
    Stroke *m_shapeStroke;
    QPointF m_shapeOrigin;

    QVector<QPointF> m_curveAnchors;
    QPointF m_curveCursor;
    bool m_curveCursorValid;

    bool m_marqueeActive;
    QRectF m_marquee;

    bool m_lassoActive;
    QVector<QPointF> m_lasso;

    bool m_movingSelection;
    QPointF m_lastMovePos;

    bool m_draggingNode;
    bool m_scalingSelection;

    QBasicTimer m_longPressTimer;
    QPointF m_pressPagePos;
    bool m_longPressHandled;
    QImage m_cache;
    bool m_cacheValid;
    QRectF m_liveBounds;
};
}

#endif
