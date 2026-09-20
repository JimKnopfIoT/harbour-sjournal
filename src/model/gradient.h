#ifndef XN_GRADIENT_H
#define XN_GRADIENT_H

#include <QColor>
#include <QLinearGradient>
#include <QRectF>
#include <QString>
#include <QVector>

namespace xn {
struct GradientStop
{
    GradientStop(): at(0) {}
    GradientStop(qreal position, const QColor &c): at(position), color(c) {}

    bool operator==(const GradientStop &o) const { return at == o.at && color == o.color; }
    bool operator!=(const GradientStop &o) const { return !(*this == o); }

    qreal at;
    QColor color;
};

typedef QVector<GradientStop> GradientStops;

// A fill is a gradient only once it has two stops to run between.
class Gradient
{
public:
    static bool isReal(const GradientStops &stops) { return stops.size() >= 2; }

    static QLinearGradient forRect(const GradientStops &stops, const QRectF &box,
                                   int angleDegrees, int alpha);

    static GradientStops evenly(const QVector<QColor> &colors);

    static QString toString(const GradientStops &stops);
    static GradientStops fromString(const QString &text);
};
}

#endif
