#include "render/strokegeometry.h"

#include "model/stroke.h"

#include <QLineF>
#include <QPolygonF>
#include <qmath.h>

namespace xn {
namespace StrokeGeometry {
static QString num(qreal v)
{
    return QString::number(v, 'g', 6);
}

bool isUniformWidth(const Stroke &s)
{
    if (!s.hasPressure())
        return true;

    qreal minW = s.widthAt(0);
    qreal maxW = minW;
    for (int i = 1; i < s.points.size(); ++i) {
        const qreal w = s.widthAt(i);
        if (w < minW) minW = w;
        if (w > maxW) maxW = w;
    }
    return minW > 0 && (maxW - minW) / minW < 0.05;
}

QPainterPath centerline(const Stroke &s)
{
    QPainterPath path;
    const int n = s.points.size();
    if (n == 0)
        return path;

    const StrokePoint &p0 = s.points.at(0);
    path.moveTo(p0.x, p0.y);

    if (s.shape != Stroke::FreeShape) {
        for (int i = 1; i < n; ++i)
            path.lineTo(s.points.at(i).x, s.points.at(i).y);
        return path;
    }

    if (n == 1) {
        path.lineTo(p0.x, p0.y);
        return path;
    }
    if (n == 2) {
        path.lineTo(s.points.at(1).x, s.points.at(1).y);
        return path;
    }

    for (int i = 1; i < n - 1; ++i) {
        const StrokePoint &c = s.points.at(i);
        const StrokePoint &next = s.points.at(i + 1);
        path.quadTo(c.x, c.y, (c.x + next.x) / 2, (c.y + next.y) / 2);
    }
    path.lineTo(s.points.at(n - 1).x, s.points.at(n - 1).y);
    return path;
}

static qreal signedArea(const QPolygonF &poly)
{
    qreal a = 0;
    for (int i = 0; i < poly.size(); ++i) {
        const QPointF &p = poly.at(i);
        const QPointF &q = poly.at((i + 1) % poly.size());
        a += p.x() * q.y() - q.x() * p.y();
    }
    return a / 2;
}

static void addOriented(QPainterPath *path, QPolygonF poly)
{
    if (poly.size() < 3)
        return;
    if (signedArea(poly) < 0) {
        QPolygonF reversed;
        reversed.reserve(poly.size());
        for (int i = poly.size() - 1; i >= 0; --i)
            reversed << poly.at(i);
        poly = reversed;
    }
    path->addPolygon(poly);
    path->closeSubpath();
}

static QPolygonF discPolygon(const QPointF &center, qreal radius, int steps = 16)
{
    QPolygonF poly;
    poly.reserve(steps);
    for (int i = 0; i < steps; ++i) {
        const qreal a = 2 * M_PI * i / steps;
        poly << QPointF(center.x() + radius * qCos(a), center.y() + radius * qSin(a));
    }
    return poly;
}

QPainterPath outline(const Stroke &s)
{
    QPainterPath path;
    const int n = s.points.size();
    if (n == 0)
        return path;

    path.setFillRule(Qt::WindingFill);

    if (n == 1) {
        addOriented(&path, discPolygon(QPointF(s.points.at(0).x, s.points.at(0).y),
                                       s.widthAt(0) / 2));
        return path;
    }

    for (int i = 0; i < n - 1; ++i) {
        const StrokePoint &a = s.points.at(i);
        const StrokePoint &b = s.points.at(i + 1);
        const qreal ra = s.widthAt(i) / 2;
        const qreal rb = s.widthAt(i + 1) / 2;

        const QLineF seg(a.x, a.y, b.x, b.y);
        const qreal len = seg.length();
        if (len > 0.0001) {
            const qreal nx = -(b.y - a.y) / len;
            const qreal ny = (b.x - a.x) / len;

            QPolygonF quad;
            quad << QPointF(a.x + nx * ra, a.y + ny * ra)
                 << QPointF(b.x + nx * rb, b.y + ny * rb)
                 << QPointF(b.x - nx * rb, b.y - ny * rb)
                 << QPointF(a.x - nx * ra, a.y - ny * ra);
            addOriented(&path, quad);
        }

        addOriented(&path, discPolygon(QPointF(a.x, a.y), ra));
    }

    addOriented(&path, discPolygon(QPointF(s.points.at(n - 1).x, s.points.at(n - 1).y),
                                   s.widthAt(n - 1) / 2));
    return path;
}

QString toSvgPathData(const QPainterPath &path)
{
    QString d;
    d.reserve(path.elementCount() * 16);

    for (int i = 0; i < path.elementCount(); ++i) {
        const QPainterPath::Element &e = path.elementAt(i);
        switch (e.type) {
        case QPainterPath::MoveToElement:
            d += QStringLiteral("M%1 %2").arg(num(e.x), num(e.y));
            break;
        case QPainterPath::LineToElement:
            d += QStringLiteral("L%1 %2").arg(num(e.x), num(e.y));
            break;
        case QPainterPath::CurveToElement: {
            const QPainterPath::Element &c2 = path.elementAt(i + 1);
            const QPainterPath::Element &to = path.elementAt(i + 2);
            d += QStringLiteral("C%1 %2 %3 %4 %5 %6")
                    .arg(num(e.x), num(e.y), num(c2.x), num(c2.y), num(to.x), num(to.y));
            i += 2;
            break;
        }
        case QPainterPath::CurveToDataElement:
            break;
        }
        d += QLatin1Char(' ');
    }
    return d.trimmed();
}
}
}
