#include "model/path.h"

#include <qmath.h>

#include <QPainterPathStroker>
#include <QPolygonF>

namespace xn {
Path::Path()
    : color(Qt::black)
    , width(1.41)
    , closed(false)
    , fill(-1)
    , gradientAngle(0)
    , m_pathValid(false)
{
}

QPainterPath Path::painterPath() const
{
    if (!m_pathValid) {
        m_path = QPainterPath();
        m_path.moveTo(start);
        for (int i = 0; i < segments.size(); ++i) {
            const CubicSegment &s = segments.at(i);
            m_path.cubicTo(s.c1, s.c2, s.to);
        }
        if (closed || !extra.isEmpty())
            m_path.closeSubpath();

        for (int k = 0; k < extra.size(); ++k) {
            const SubPath &sub = extra.at(k);
            if (sub.segments.isEmpty())
                continue;
            m_path.moveTo(sub.start);
            for (int i = 0; i < sub.segments.size(); ++i) {
                const CubicSegment &s = sub.segments.at(i);
                m_path.cubicTo(s.c1, s.c2, s.to);
            }
            m_path.closeSubpath();
        }
        m_pathValid = true;
    }
    return m_path;
}

QRectF Path::bounds() const
{
    if (segments.isEmpty() && extra.isEmpty())
        return QRectF(start, start).adjusted(-width / 2, -width / 2, width / 2, width / 2);
    return painterPath().boundingRect().adjusted(-width / 2, -width / 2, width / 2, width / 2);
}

Element *Path::clone() const
{
    return new Path(*this);
}

static QPointF scaledPoint(const QPointF &p, const QPointF &origin, qreal sx, qreal sy)
{
    return QPointF(origin.x() + (p.x() - origin.x()) * sx,
                   origin.y() + (p.y() - origin.y()) * sy);
}

void Path::scale(const QPointF &origin, qreal sx, qreal sy)
{
    start = scaledPoint(start, origin, sx, sy);
    for (int i = 0; i < segments.size(); ++i) {
        segments[i].c1 = scaledPoint(segments.at(i).c1, origin, sx, sy);
        segments[i].c2 = scaledPoint(segments.at(i).c2, origin, sx, sy);
        segments[i].to = scaledPoint(segments.at(i).to, origin, sx, sy);
    }
    for (int k = 0; k < extra.size(); ++k) {
        extra[k].start = scaledPoint(extra.at(k).start, origin, sx, sy);
        for (int i = 0; i < extra.at(k).segments.size(); ++i) {
            extra[k].segments[i].c1 = scaledPoint(extra.at(k).segments.at(i).c1, origin, sx, sy);
            extra[k].segments[i].c2 = scaledPoint(extra.at(k).segments.at(i).c2, origin, sx, sy);
            extra[k].segments[i].to = scaledPoint(extra.at(k).segments.at(i).to, origin, sx, sy);
        }
    }
    width *= qSqrt(qAbs(sx * sy));
    invalidate();
}

void Path::translate(qreal dx, qreal dy)
{
    const QPointF d(dx, dy);
    start += d;
    for (int i = 0; i < segments.size(); ++i) {
        segments[i].c1 += d;
        segments[i].c2 += d;
        segments[i].to += d;
    }
    for (int k = 0; k < extra.size(); ++k) {
        extra[k].start += d;
        for (int i = 0; i < extra.at(k).segments.size(); ++i) {
            extra[k].segments[i].c1 += d;
            extra[k].segments[i].c2 += d;
            extra[k].segments[i].to += d;
        }
    }
    invalidate();
}

QVector<QPointF> Path::anchors() const
{
    QVector<QPointF> out;
    out.reserve(segments.size() + 1);
    out << start;
    for (int i = 0; i < segments.size(); ++i)
        out << segments.at(i).to;
    return out;
}

QVector<QPointF> Path::flattened() const
{
    QVector<QPointF> out;
    const QList<QPolygonF> polys = painterPath().toSubpathPolygons();
    for (int i = 0; i < polys.size(); ++i) {
        const QPolygonF &poly = polys.at(i);
        for (int j = 0; j < poly.size(); ++j)
            out << poly.at(j);
    }
    return out;
}

bool Path::hits(const QPointF &pos, qreal radius) const
{
    if (!bounds().adjusted(-radius, -radius, radius, radius).contains(pos))
        return false;

    const QPainterPath p = painterPath();
    if (fill >= 0 && p.contains(pos))
        return true;

    QPainterPathStroker stroker;
    stroker.setWidth(width + 2 * radius);
    stroker.setCapStyle(Qt::RoundCap);
    stroker.setJoinStyle(Qt::RoundJoin);
    return stroker.createStroke(p).contains(pos);
}

void Path::setSegments(const QPointF &from, const QVector<CubicSegment> &segs)
{
    start = from;
    segments = segs;
    invalidate();
}

void Path::invalidate()
{
    m_pathValid = false;
}
}
