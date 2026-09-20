#include "model/stroke.h"

#include <qmath.h>

namespace xn {
Stroke::Stroke()
    : color(Qt::black)
    , width(1.41)
    , tool(Pen)
    , cap(RoundCap)
    , fill(-1)
    , gradientAngle(0)
    , shape(FreeShape)
    , m_boundsValid(false)
{
}

QRectF Stroke::bounds() const
{
    if (!m_boundsValid) {
        if (points.isEmpty()) {
            m_bounds = QRectF();
        } else {
            qreal minX = points.first().x, maxX = minX;
            qreal minY = points.first().y, maxY = minY;
            for (int i = 1; i < points.size(); ++i) {
                const StrokePoint &p = points.at(i);
                if (p.x < minX) minX = p.x;
                if (p.x > maxX) maxX = p.x;
                if (p.y < minY) minY = p.y;
                if (p.y > maxY) maxY = p.y;
            }
            const qreal pad = maxWidth() * 0.5;
            m_bounds = QRectF(minX - pad, minY - pad,
                              maxX - minX + 2 * pad, maxY - minY + 2 * pad);
        }
        m_boundsValid = true;
    }
    return m_bounds;
}

Element *Stroke::clone() const
{
    return new Stroke(*this);
}

void Stroke::scale(const QPointF &origin, qreal sx, qreal sy)
{
    const qreal widthFactor = qSqrt(qAbs(sx * sy));
    for (int i = 0; i < points.size(); ++i) {
        points[i].x = origin.x() + (points.at(i).x - origin.x()) * sx;
        points[i].y = origin.y() + (points.at(i).y - origin.y()) * sy;
        if (points.at(i).width > 0)
            points[i].width *= widthFactor;
    }
    width *= widthFactor;
    invalidate();
}

void Stroke::translate(qreal dx, qreal dy)
{
    for (int i = 0; i < points.size(); ++i) {
        points[i].x += dx;
        points[i].y += dy;
    }
    m_boundsValid = false;
}

bool Stroke::hasPressure() const
{
    for (int i = 0; i < points.size(); ++i) {
        if (points.at(i).width > 0)
            return true;
    }
    return false;
}

qreal Stroke::widthAt(int i) const
{
    if (i < 0 || i >= points.size())
        return width;
    const qreal w = points.at(i).width;
    return w > 0 ? w : width;
}

qreal Stroke::maxWidth() const
{
    qreal w = width;
    for (int i = 0; i < points.size(); ++i) {
        if (points.at(i).width > w)
            w = points.at(i).width;
    }
    return w;
}

void Stroke::addPoint(const StrokePoint &p)
{
    points.append(p);
    m_boundsValid = false;
}

void Stroke::setPoints(const QVector<StrokePoint> &pts)
{
    points = pts;
    m_boundsValid = false;
}

void Stroke::invalidate()
{
    m_boundsValid = false;
}
}
