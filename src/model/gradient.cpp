#include "model/gradient.h"

#include <QStringList>
#include <qmath.h>

namespace xn {
QLinearGradient Gradient::forRect(const GradientStops &stops, const QRectF &box,
                                  int angleDegrees, int alpha)
{
    const QPointF centre = box.center();
    const qreal radians = qDegreesToRadians(qreal(angleDegrees));
    const qreal dx = qCos(radians);
    const qreal dy = qSin(radians);

    // Reach far enough that the run covers the box at any angle.
    const qreal half = qSqrt(box.width() * box.width() + box.height() * box.height()) / 2;

    QLinearGradient g(centre - QPointF(dx, dy) * half, centre + QPointF(dx, dy) * half);
    for (int i = 0; i < stops.size(); ++i) {
        QColor c = stops.at(i).color;
        if (alpha >= 0)
            c.setAlpha(alpha);
        g.setColorAt(qBound(qreal(0), stops.at(i).at, qreal(1)), c);
    }
    return g;
}

GradientStops Gradient::evenly(const QVector<QColor> &colors)
{
    GradientStops out;
    if (colors.size() < 2)
        return out;

    for (int i = 0; i < colors.size(); ++i)
        out << GradientStop(qreal(i) / (colors.size() - 1), colors.at(i));
    return out;
}

QString Gradient::toString(const GradientStops &stops)
{
    QStringList parts;
    for (int i = 0; i < stops.size(); ++i)
        parts << QStringLiteral("%1,%2")
                 .arg(stops.at(i).at, 0, 'g', 4)
                 .arg(stops.at(i).color.name(QColor::HexRgb));
    return parts.join(QStringLiteral(" "));
}

GradientStops Gradient::fromString(const QString &text)
{
    GradientStops out;
    const QStringList parts = text.split(QLatin1Char(' '), QString::SkipEmptyParts);
    for (int i = 0; i < parts.size(); ++i) {
        const QStringList one = parts.at(i).split(QLatin1Char(','));
        if (one.size() != 2)
            continue;
        bool ok = false;
        const qreal at = one.at(0).toDouble(&ok);
        const QColor c(one.at(1));
        if (ok && c.isValid())
            out << GradientStop(at, c);
    }
    return out;
}
}
