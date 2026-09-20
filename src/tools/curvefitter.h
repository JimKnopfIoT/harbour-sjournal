#ifndef XN_CURVEFITTER_H
#define XN_CURVEFITTER_H

#include "model/path.h"

#include <QPointF>
#include <QVector>

namespace xn {
namespace CurveFitter {
QVector<CubicSegment> through(const QVector<QPointF> &anchors, bool closed);
}
}

#endif
