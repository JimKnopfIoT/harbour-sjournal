#ifndef XN_SHAPEFACTORY_H
#define XN_SHAPEFACTORY_H

#include "model/stroke.h"

#include <QPointF>
#include <QRectF>
#include <QString>
#include <QVector>

namespace xn {
namespace ShapeFactory {
extern const qreal kSnapStep;

QRectF dragRect(const QPointF &origin, const QPointF &current,
                bool equalSides, qreal snapStep);

QVector<StrokePoint> build(Stroke::Shape shape, const QPointF &origin,
                           const QPointF &current, int polygonCorners = 3,
                           bool equalSides = false, qreal snapStep = 0);

int clampCorners(int corners);

QString describeSize(const QRectF &rect);
}
}

#endif
