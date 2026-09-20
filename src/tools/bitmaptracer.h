#ifndef XN_BITMAPTRACER_H
#define XN_BITMAPTRACER_H

#include <QColor>
#include <QImage>
#include <QList>
#include <QRect>
#include <QRectF>
#include <QString>

namespace xn {
class Stroke;

class BitmapTracer
{
public:
    struct Options
    {
        Options();

        int threshold;
        bool invert;
        bool filled;
        int speckSize;
        qreal cornerThreshold;
        qreal optimizeTolerance;
    };

    BitmapTracer();

    QList<Stroke *> trace(const QImage &image, const QRect &sourceRect,
                          const QRectF &target, const Options &options,
                          const QColor &color = Qt::black,
                          qreal strokeWidth = 1.0) const;

    QString errorString() const { return m_error; }

private:
    mutable QString m_error;
};
}

#endif
