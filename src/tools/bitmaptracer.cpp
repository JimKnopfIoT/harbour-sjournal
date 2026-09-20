#include "tools/bitmaptracer.h"

#include "model/stroke.h"

#include <QPainterPath>
#include <QPolygonF>

extern "C" {
#include "potracelib.h"
}

namespace xn {
static const int kWordBits = int(sizeof(potrace_word) * 8);

BitmapTracer::Options::Options()
    : threshold(128)
    , invert(false)
    , filled(false)
    , speckSize(4)
    , cornerThreshold(1.0)
    , optimizeTolerance(0.2)
{
}

BitmapTracer::BitmapTracer()
{
}

static potrace_bitmap_t *makeBitmap(const QImage &image, const QRect &rect,
                                    const BitmapTracer::Options &opt)
{
    const int w = rect.width();
    const int h = rect.height();
    if (w <= 0 || h <= 0)
        return 0;

    potrace_bitmap_t *bm = new potrace_bitmap_t;
    bm->w = w;
    bm->h = h;
    bm->dy = (w + kWordBits - 1) / kWordBits;
    bm->map = new potrace_word[size_t(bm->dy) * size_t(h)]();

    for (int y = 0; y < h; ++y) {
        const int srcY = rect.top() + (h - 1 - y);
        potrace_word *row = bm->map + size_t(y) * size_t(bm->dy);

        for (int x = 0; x < w; ++x) {
            const QRgb pixel = image.pixel(rect.left() + x, srcY);
            bool ink;
            if (qAlpha(pixel) < 128) {
                ink = false;
            } else {
                ink = qGray(pixel) < opt.threshold;
                if (opt.invert)
                    ink = !ink;
            }
            if (ink)
                row[x / kWordBits] |= potrace_word(1) << (kWordBits - 1 - (x % kWordBits));
        }
    }
    return bm;
}

static void freeBitmap(potrace_bitmap_t *bm)
{
    if (!bm)
        return;
    delete[] bm->map;
    delete bm;
}

QList<Stroke *> BitmapTracer::trace(const QImage &image, const QRect &sourceRect,
                                    const QRectF &target, const Options &options,
                                    const QColor &color, qreal strokeWidth) const
{
    QList<Stroke *> result;
    m_error.clear();

    if (image.isNull()) {
        m_error = QStringLiteral("No image");
        return result;
    }

    QRect rect = sourceRect.isNull() ? image.rect() : sourceRect.intersected(image.rect());
    if (rect.width() < 2 || rect.height() < 2) {
        m_error = QStringLiteral("Region is too small to trace");
        return result;
    }

    const QImage source = image.format() == QImage::Format_ARGB32
            ? image : image.convertToFormat(QImage::Format_ARGB32);

    potrace_bitmap_t *bm = makeBitmap(source, rect, options);
    if (!bm) {
        m_error = QStringLiteral("Cannot build the trace bitmap");
        return result;
    }

    potrace_param_t *param = potrace_param_default();
    if (!param) {
        freeBitmap(bm);
        m_error = QStringLiteral("Cannot set up potrace");
        return result;
    }
    param->turdsize = options.speckSize;
    param->alphamax = options.cornerThreshold;
    param->opticurve = options.optimizeTolerance > 0 ? 1 : 0;
    param->opttolerance = options.optimizeTolerance;

    potrace_state_t *state = potrace_trace(param, bm);
    potrace_param_free(param);
    freeBitmap(bm);

    if (!state || state->status != POTRACE_STATUS_OK) {
        if (state)
            potrace_state_free(state);
        m_error = QStringLiteral("Tracing failed");
        return result;
    }

    const qreal sx = target.width() / rect.width();
    const qreal sy = target.height() / rect.height();
    const qreal originX = target.left();
    const qreal originY = target.top() + target.height();

    for (potrace_path_t *path = state->plist; path; path = path->next) {
        const potrace_curve_t &curve = path->curve;
        if (curve.n <= 0)
            continue;

        QPainterPath painterPath;
        const potrace_dpoint_t &start = curve.c[curve.n - 1][2];
        painterPath.moveTo(originX + start.x * sx, originY - start.y * sy);

        for (int i = 0; i < curve.n; ++i) {
            const potrace_dpoint_t *c = curve.c[i];
            if (curve.tag[i] == POTRACE_CORNER) {
                painterPath.lineTo(originX + c[1].x * sx, originY - c[1].y * sy);
                painterPath.lineTo(originX + c[2].x * sx, originY - c[2].y * sy);
            } else {
                painterPath.cubicTo(originX + c[0].x * sx, originY - c[0].y * sy,
                                    originX + c[1].x * sx, originY - c[1].y * sy,
                                    originX + c[2].x * sx, originY - c[2].y * sy);
            }
        }
        painterPath.closeSubpath();

        const QList<QPolygonF> polygons = painterPath.toSubpathPolygons();
        for (int p = 0; p < polygons.size(); ++p) {
            const QPolygonF &poly = polygons.at(p);
            if (poly.size() < 3)
                continue;

            Stroke *stroke = new Stroke;
            stroke->color = color;
            stroke->width = strokeWidth;
            stroke->cap = Stroke::RoundCap;

            QVector<StrokePoint> pts;
            pts.reserve(poly.size() + 1);
            for (int i = 0; i < poly.size(); ++i)
                pts.append(StrokePoint(poly.at(i).x(), poly.at(i).y()));
            if (pts.first().x != pts.last().x || pts.first().y != pts.last().y)
                pts.append(pts.first());
            stroke->setPoints(pts);

            if (options.filled && path->sign == '+') {
                stroke->fill = 255;
                stroke->fillColor = color;
            }

            result.append(stroke);
        }
    }

    potrace_state_free(state);

    if (result.isEmpty())
        m_error = QStringLiteral("Nothing found at this threshold");
    return result;
}
}
