#ifndef XN_PATH_H
#define XN_PATH_H

#include "model/element.h"
#include "model/gradient.h"

#include <QColor>
#include <QPainterPath>
#include <QPointF>
#include <QString>
#include <QVector>

namespace xn {
struct CubicSegment
{
    CubicSegment() {}
    CubicSegment(const QPointF &h1, const QPointF &h2, const QPointF &end)
        : c1(h1), c2(h2), to(end) {}

    QPointF c1;
    QPointF c2;
    QPointF to;
};

struct SubPath
{
    QPointF start;
    QVector<CubicSegment> segments;
};

class Path : public Element
{
public:
    Path();

    Type type() const { return PathType; }
    QRectF bounds() const;
    Element *clone() const;
    void translate(qreal dx, qreal dy);
    void scale(const QPointF &origin, qreal sx, qreal sy);

    QPainterPath painterPath() const;
    QVector<QPointF> anchors() const;
    QVector<QPointF> flattened() const;

    bool hits(const QPointF &pos, qreal radius) const;

    void setSegments(const QPointF &from, const QVector<CubicSegment> &segs);
    void invalidate();

    QPointF start;
    QVector<CubicSegment> segments;
    // Inner contours, such as the middle of an e; they punch holes odd-even.
    QVector<SubPath> extra;
    QColor color;
    qreal width;
    bool closed;
    QString lineStyle;
    int fill;
    QColor fillColor;
    GradientStops gradient;
    int gradientAngle;

private:
    mutable QPainterPath m_path;
    mutable bool m_pathValid;
};
}

#endif
