#ifndef XN_OVERLAYPAINTER_H
#define XN_OVERLAYPAINTER_H

#include <QPointF>
#include <QRectF>
#include <QVector>

class QPainter;

namespace xn {
class NodeEditor;
class Selection;

// Everything CanvasItem draws on top of the page cache rather than into it.
class OverlayPainter
{
public:
    static void drawCurveAnchors(QPainter *p, const QVector<QPointF> &anchors, qreal zoom);
    static void drawMarquee(QPainter *p, const QRectF &rect, qreal zoom);
    static void drawLasso(QPainter *p, const QVector<QPointF> &path, qreal zoom);
    static void drawSelection(QPainter *p, const Selection *selection, qreal zoom);
    static void drawNodes(QPainter *p, const NodeEditor *nodes, qreal zoom);
};
}

#endif
