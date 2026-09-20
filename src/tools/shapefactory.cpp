#include "tools/shapefactory.h"

#include <qmath.h>

namespace xn {
namespace ShapeFactory {
static const qreal kPointsPerMm = 72.0 / 25.4;

const qreal kSnapStep = 5.0 * kPointsPerMm;

static const qreal kSnapFraction = 0.3;

int clampCorners(int corners)
{
    return qBound(3, corners, 12);
}

static qreal snapped(qreal value, qreal step)
{
    if (step <= 0)
        return value;
    const qreal nearest = qRound(value / step) * step;
    return qAbs(value - nearest) <= step * kSnapFraction ? nearest : value;
}

QRectF dragRect(const QPointF &origin, const QPointF &current,
                bool equalSides, qreal snapStep)
{
    qreal w = qAbs(current.x() - origin.x());
    qreal h = qAbs(current.y() - origin.y());

    if (equalSides)
        w = h = qMax(w, h);

    w = snapped(w, snapStep);
    h = snapped(h, snapStep);

    const qreal x = current.x() >= origin.x() ? origin.x() : origin.x() - w;
    const qreal y = current.y() >= origin.y() ? origin.y() : origin.y() - h;
    return QRectF(x, y, w, h);
}

QString describeSize(const QRectF &rect)
{
    return QStringLiteral("%1 × %2 mm")
            .arg(qRound(rect.width() / kPointsPerMm))
            .arg(qRound(rect.height() / kPointsPerMm));
}

static qreal starInnerRatio(int n)
{
    if (n <= 3)
        return 0.20;
    if (n == 4)
        return 0.30;
    return qBound(qreal(0.3), qCos(2 * M_PI / n) / qCos(M_PI / n), qreal(0.75));
}

QVector<StrokePoint> build(Stroke::Shape shape, const QPointF &origin,
                           const QPointF &current, int polygonCorners,
                           bool equalSides, qreal snapStep)
{
    QVector<StrokePoint> pts;
    const QRectF rect = dragRect(origin, current, equalSides, snapStep);

    switch (shape) {
    case Stroke::LineShape: {
        QPointF end = current;
        if (equalSides) {
            const qreal dx = current.x() - origin.x();
            const qreal dy = current.y() - origin.y();
            const qreal len = qSqrt(dx * dx + dy * dy);
            if (len > 1e-6) {
                const qreal step = M_PI / 12;
                const qreal angle = qRound(qAtan2(dy, dx) / step) * step;
                end = QPointF(origin.x() + len * qCos(angle),
                              origin.y() + len * qSin(angle));
            }
        }
        pts << StrokePoint(origin.x(), origin.y())
            << StrokePoint(end.x(), end.y());
        break;
    }

    case Stroke::ArrowShape: {
        QPointF tip = current;
        qreal dx = tip.x() - origin.x();
        qreal dy = tip.y() - origin.y();
        qreal len = qSqrt(dx * dx + dy * dy);
        if (len < 1e-6) {
            pts << StrokePoint(origin.x(), origin.y());
            break;
        }
        if (equalSides) {
            const qreal step = M_PI / 12;
            const qreal angle = qRound(qAtan2(dy, dx) / step) * step;
            tip = QPointF(origin.x() + len * qCos(angle), origin.y() + len * qSin(angle));
            dx = tip.x() - origin.x();
            dy = tip.y() - origin.y();
            len = qSqrt(dx * dx + dy * dy);
        }
        const qreal headLen = qBound(qreal(6), qreal(len * 0.22), qreal(len * 0.35));
        const qreal rad = qDegreesToRadians(qreal(25));
        const qreal ux = dx / len, uy = dy / len;

        const QPointF barb1(tip.x() - headLen * (ux * qCos(rad) - uy * qSin(rad)),
                            tip.y() - headLen * (ux * qSin(rad) + uy * qCos(rad)));
        const QPointF barb2(tip.x() - headLen * (ux * qCos(-rad) - uy * qSin(-rad)),
                            tip.y() - headLen * (ux * qSin(-rad) + uy * qCos(-rad)));

        pts << StrokePoint(origin.x(), origin.y())
            << StrokePoint(tip.x(), tip.y())
            << StrokePoint(barb1.x(), barb1.y())
            << StrokePoint(tip.x(), tip.y())
            << StrokePoint(barb2.x(), barb2.y());
        break;
    }

    case Stroke::RectShape:
        pts << StrokePoint(rect.left(), rect.top())
            << StrokePoint(rect.right(), rect.top())
            << StrokePoint(rect.right(), rect.bottom())
            << StrokePoint(rect.left(), rect.bottom())
            << StrokePoint(rect.left(), rect.top());
        break;

    case Stroke::EllipseShape: {
        const qreal cx = rect.center().x(), cy = rect.center().y();
        const qreal rx = rect.width() / 2, ry = rect.height() / 2;
        const int steps = 64;
        for (int i = 0; i <= steps; ++i) {
            const qreal a = 2 * M_PI * i / steps;
            pts << StrokePoint(cx + rx * qCos(a), cy + ry * qSin(a));
        }
        break;
    }

    case Stroke::PolygonShape: {
        const int n = clampCorners(polygonCorners);
        const qreal cx = rect.center().x(), cy = rect.center().y();
        const qreal rx = rect.width() / 2, ry = rect.height() / 2;
        const qreal start = -M_PI / 2;
        for (int i = 0; i <= n; ++i) {
            const qreal a = start + 2 * M_PI * i / n;
            pts << StrokePoint(cx + rx * qCos(a), cy + ry * qSin(a));
        }
        break;
    }

    case Stroke::StarShape: {
        const int n = clampCorners(polygonCorners);
        const qreal cx = rect.center().x(), cy = rect.center().y();
        const qreal rx = rect.width() / 2, ry = rect.height() / 2;
        const qreal inner = starInnerRatio(n);
        const qreal start = -M_PI / 2;
        for (int i = 0; i <= 2 * n; ++i) {
            const qreal a = start + M_PI * i / n;
            const qreal f = (i % 2 == 0) ? qreal(1.0) : inner;
            pts << StrokePoint(cx + rx * f * qCos(a), cy + ry * f * qSin(a));
        }
        break;
    }

    case Stroke::PolylineShape:
    case Stroke::FreeShape:
        break;
    }

    return pts;
}
}
}
