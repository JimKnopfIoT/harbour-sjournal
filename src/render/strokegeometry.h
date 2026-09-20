#ifndef XN_STROKEGEOMETRY_H
#define XN_STROKEGEOMETRY_H

#include <QPainterPath>
#include <QString>

namespace xn {
class Stroke;

namespace StrokeGeometry {
bool isUniformWidth(const Stroke &s);

QPainterPath centerline(const Stroke &s);

QPainterPath outline(const Stroke &s);

QString toSvgPathData(const QPainterPath &path);
}
}

#endif
