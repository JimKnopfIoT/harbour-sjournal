#ifndef XN_STROKE_H
#define XN_STROKE_H

#include "model/element.h"
#include "model/gradient.h"

#include <QColor>
#include <QString>
#include <QVector>

namespace xn {
struct StrokePoint
{
    StrokePoint(): x(0), y(0), width(-1) {}
    StrokePoint(qreal px, qreal py, qreal w = -1): x(px), y(py), width(w) {}

    qreal x;
    qreal y;
    qreal width;
};

class Stroke : public Element
{
public:
    enum Tool { Pen, Highlighter, Eraser };
    enum Cap { RoundCap, ButtCap, SquareCap };

    enum Shape { FreeShape, LineShape, RectShape, EllipseShape, ArrowShape,
                 PolygonShape, StarShape, PolylineShape };

    Stroke();

    Type type() const { return StrokeType; }
    QRectF bounds() const;
    Element *clone() const;
    void translate(qreal dx, qreal dy);
    void scale(const QPointF &origin, qreal sx, qreal sy);

    bool hasPressure() const;

    qreal widthAt(int i) const;

    qreal maxWidth() const;

    void addPoint(const StrokePoint &p);
    void setPoints(const QVector<StrokePoint> &pts);

    void invalidate();

    QVector<StrokePoint> points;
    QColor color;
    qreal width;
    Tool tool;
    Cap cap;
    QString lineStyle;    // empty or "dash", "dashdot", "dot", "plain"
    int fill;
    QColor fillColor;
    GradientStops gradient;
    int gradientAngle;
    Shape shape;

private:
    mutable QRectF m_bounds;
    mutable bool m_boundsValid;
};
}

#endif
