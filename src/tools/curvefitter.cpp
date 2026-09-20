#include "tools/curvefitter.h"

namespace xn {
namespace CurveFitter {
static QPointF at(const QVector<QPointF> &p, int i, bool closed)
{
    const int n = p.size();
    if (closed)
        return p.at(((i % n) + n) % n);
    return p.at(qBound(0, i, n - 1));
}

QVector<CubicSegment> through(const QVector<QPointF> &anchors, bool closed)
{
    QVector<CubicSegment> out;
    const int n = anchors.size();
    if (n < 2)
        return out;

    const int last = closed ? n : n - 1;
    out.reserve(last);

    for (int i = 0; i < last; ++i) {
        const QPointF before = at(anchors, i - 1, closed);
        const QPointF from = at(anchors, i, closed);
        const QPointF to = at(anchors, i + 1, closed);
        const QPointF after = at(anchors, i + 2, closed);

        out << CubicSegment(from + (to - before) / 6.0,
                            to - (after - from) / 6.0,
                            to);
    }
    return out;
}
}
}
