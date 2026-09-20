#include "tools/shaperecognizer.h"

#include <QLineF>
#include <QPointF>
#include <QRectF>
#include <qmath.h>

#include <algorithm>

namespace xn {
static QRectF boundsOf(const QVector<QPointF> &pts)
{
    if (pts.isEmpty())
        return QRectF();
    qreal minX = pts.first().x(), maxX = minX;
    qreal minY = pts.first().y(), maxY = minY;
    for (int i = 1; i < pts.size(); ++i) {
        minX = qMin(minX, pts.at(i).x()); maxX = qMax(maxX, pts.at(i).x());
        minY = qMin(minY, pts.at(i).y()); maxY = qMax(maxY, pts.at(i).y());
    }
    return QRectF(minX, minY, maxX - minX, maxY - minY);
}

static qreal pathLength(const QVector<QPointF> &pts)
{
    qreal len = 0;
    for (int i = 1; i < pts.size(); ++i)
        len += QLineF(pts.at(i - 1), pts.at(i)).length();
    return len;
}

static qreal distanceToSegment(const QPointF &p, const QPointF &a, const QPointF &b)
{
    const qreal dx = b.x() - a.x();
    const qreal dy = b.y() - a.y();
    const qreal lenSq = dx * dx + dy * dy;
    if (lenSq < 1e-9)
        return QLineF(p, a).length();
    qreal t = ((p.x() - a.x()) * dx + (p.y() - a.y()) * dy) / lenSq;
    t = qBound(qreal(0), t, qreal(1));
    return QLineF(p, QPointF(a.x() + t * dx, a.y() + t * dy)).length();
}

static qreal distanceToPolygon(const QPointF &p, const QVector<QPointF> &poly)
{
    qreal best = -1;
    for (int i = 0; i < poly.size(); ++i) {
        const qreal d = distanceToSegment(p, poly.at(i), poly.at((i + 1) % poly.size()));
        if (best < 0 || d < best)
            best = d;
    }
    return best < 0 ? 0 : best;
}

static QVector<QPointF> resample(const QVector<StrokePoint> &src, int count)
{
    QVector<QPointF> in;
    in.reserve(src.size());
    for (int i = 0; i < src.size(); ++i) {
        const QPointF p(src.at(i).x, src.at(i).y);
        if (in.isEmpty() || QLineF(in.last(), p).length() > 0.01)
            in.append(p);
    }
    if (in.size() < 2)
        return in;

    const qreal total = pathLength(in);
    if (total < 1e-6)
        return in;

    const qreal step = total / (count - 1);
    QVector<QPointF> out;
    out.reserve(count);
    out.append(in.first());

    qreal carried = 0;
    for (int i = 1; i < in.size(); ++i) {
        QPointF a = in.at(i - 1);
        const QPointF b = in.at(i);
        qreal segment = QLineF(a, b).length();

        while (carried + segment >= step && out.size() < count - 1) {
            const qreal t = (step - carried) / segment;
            const QPointF p(a.x() + t * (b.x() - a.x()), a.y() + t * (b.y() - a.y()));
            out.append(p);
            a = p;
            segment = QLineF(a, b).length();
            carried = 0;
        }
        carried += segment;
    }
    while (out.size() < count)
        out.append(in.last());
    return out;
}

static QVector<QPointF> convexHull(QVector<QPointF> pts)
{
    if (pts.size() < 3)
        return pts;

    struct Less {
        bool operator()(const QPointF &a, const QPointF &b) const
        {
            return a.x() < b.x() || (a.x() == b.x() && a.y() < b.y());
        }
    };
    std::sort(pts.begin(), pts.end(), Less());

    QVector<QPointF> hull(2 * pts.size());
    int k = 0;
    for (int i = 0; i < pts.size(); ++i) {
        while (k >= 2) {
            const QPointF &o = hull.at(k - 2);
            const QPointF &a = hull.at(k - 1);
            const qreal cross = (a.x() - o.x()) * (pts.at(i).y() - o.y())
                              - (a.y() - o.y()) * (pts.at(i).x() - o.x());
            if (cross > 0)
                break;
            --k;
        }
        hull[k++] = pts.at(i);
    }
    for (int i = pts.size() - 2, t = k + 1; i >= 0; --i) {
        while (k >= t) {
            const QPointF &o = hull.at(k - 2);
            const QPointF &a = hull.at(k - 1);
            const qreal cross = (a.x() - o.x()) * (pts.at(i).y() - o.y())
                              - (a.y() - o.y()) * (pts.at(i).x() - o.x());
            if (cross > 0)
                break;
            --k;
        }
        hull[k++] = pts.at(i);
    }
    hull.resize(qMax(0, k - 1));
    return hull;
}

static QVector<QPointF> simplify(const QVector<QPointF> &pts, qreal epsilon)
{
    if (pts.size() < 3)
        return pts;

    QVector<bool> keep(pts.size(), false);
    keep[0] = true;
    keep[pts.size() - 1] = true;

    QVector<QPair<int, int> > stack;
    stack.append(qMakePair(0, pts.size() - 1));

    while (!stack.isEmpty()) {
        const QPair<int, int> range = stack.takeLast();
        qreal maxDist = 0;
        int maxIndex = -1;
        for (int i = range.first + 1; i < range.second; ++i) {
            const qreal d = distanceToSegment(pts.at(i), pts.at(range.first), pts.at(range.second));
            if (d > maxDist) {
                maxDist = d;
                maxIndex = i;
            }
        }
        if (maxIndex >= 0 && maxDist > epsilon) {
            keep[maxIndex] = true;
            stack.append(qMakePair(range.first, maxIndex));
            stack.append(qMakePair(maxIndex, range.second));
        }
    }

    QVector<QPointF> out;
    for (int i = 0; i < pts.size(); ++i) {
        if (keep.at(i))
            out.append(pts.at(i));
    }
    return out;
}

static qreal scoreFor(qreal rmsError, qreal tolerance)
{
    if (tolerance <= 0)
        return 0;
    return qBound(qreal(0), qreal(1.0 - rmsError / tolerance), qreal(1));
}

static QVector<StrokePoint> toStrokePoints(const QVector<QPointF> &pts, bool close)
{
    QVector<StrokePoint> out;
    out.reserve(pts.size() + (close ? 1 : 0));
    for (int i = 0; i < pts.size(); ++i)
        out.append(StrokePoint(pts.at(i).x(), pts.at(i).y()));
    if (close && !pts.isEmpty())
        out.append(StrokePoint(pts.first().x(), pts.first().y()));
    return out;
}

ShapeRecognizer::ShapeRecognizer()
    : m_threshold(0.4)
    , m_angleSnap(7.0)
    , m_squareSnap(0.18)
{
}

QVector<ShapeRecognizer::Candidate> ShapeRecognizer::lineCandidates(
        const QVector<QPointF> &pts, qreal diag) const
{
    QVector<Candidate> out;
    if (pts.size() < 2)
        return out;

    QPointF a = pts.first();
    QPointF b = pts.last();

    QLineF line(a, b);
    const qreal angle = line.angle();
    for (int k = 0; k < 8; ++k) {
        const qreal target = k * 45.0;
        qreal delta = angle - target;
        while (delta > 180) delta -= 360;
        while (delta < -180) delta += 360;
        if (qAbs(delta) <= m_angleSnap) {
            const QPointF mid((a.x() + b.x()) / 2, (a.y() + b.y()) / 2);
            const qreal half = line.length() / 2;
            const qreal rad = qDegreesToRadians(target);
            const QPointF dir(qCos(rad), -qSin(rad));
            a = mid - dir * half;
            b = mid + dir * half;
            break;
        }
    }

    qreal sum = 0;
    for (int i = 0; i < pts.size(); ++i) {
        const qreal d = distanceToSegment(pts.at(i), a, b);
        sum += d * d;
    }

    Candidate c;
    c.shape = Stroke::LineShape;
    c.points << StrokePoint(a.x(), a.y()) << StrokePoint(b.x(), b.y());
    c.score = scoreFor(qSqrt(sum / pts.size()), diag * 0.06);
    c.label = QStringLiteral("Line");
    out.append(c);
    return out;
}

QVector<ShapeRecognizer::Candidate> ShapeRecognizer::closedCandidates(
        const QVector<QPointF> &pts, qreal diag) const
{
    QVector<Candidate> out;
    const QRectF bbox = boundsOf(pts);
    const qreal tolerance = diag * 0.06;

    {
        qreal sx = 0, sy = 0, sxx = 0, syy = 0, sxy = 0, sz = 0, sxz = 0, syz = 0;
        const int n = pts.size();
        for (int i = 0; i < n; ++i) {
            const qreal x = pts.at(i).x(), y = pts.at(i).y();
            const qreal z = x * x + y * y;
            sx += x; sy += y; sxx += x * x; syy += y * y; sxy += x * y;
            sz += z; sxz += x * z; syz += y * z;
        }
        const qreal m11 = 2 * (sxx - sx * sx / n);
        const qreal m12 = 2 * (sxy - sx * sy / n);
        const qreal m22 = 2 * (syy - sy * sy / n);
        const qreal b1 = sxz - sx * sz / n;
        const qreal b2 = syz - sy * sz / n;
        const qreal det = m11 * m22 - m12 * m12;

        if (qAbs(det) > 1e-9) {
            const qreal cx = (b1 * m22 - b2 * m12) / det;
            const qreal cy = (m11 * b2 - m12 * b1) / det;
            qreal radius = 0;
            for (int i = 0; i < n; ++i)
                radius += QLineF(pts.at(i), QPointF(cx, cy)).length();
            radius /= n;

            qreal sum = 0;
            for (int i = 0; i < n; ++i) {
                const qreal d = QLineF(pts.at(i), QPointF(cx, cy)).length() - radius;
                sum += d * d;
            }

            Candidate c;
            c.shape = Stroke::EllipseShape;
            const int steps = 64;
            for (int i = 0; i <= steps; ++i) {
                const qreal t = 2 * M_PI * i / steps;
                c.points << StrokePoint(cx + radius * qCos(t), cy + radius * qSin(t));
            }
            c.score = scoreFor(qSqrt(sum / n), tolerance);
            c.label = QStringLiteral("Circle");
            out.append(c);
        }

        if (bbox.width() > 1 && bbox.height() > 1) {
            const qreal cx = bbox.center().x(), cy = bbox.center().y();
            const qreal rx = bbox.width() / 2, ry = bbox.height() / 2;
            qreal sum = 0;
            for (int i = 0; i < n; ++i) {
                const qreal nx = (pts.at(i).x() - cx) / rx;
                const qreal ny = (pts.at(i).y() - cy) / ry;
                const qreal dev = (qSqrt(nx * nx + ny * ny) - 1.0) * qMin(rx, ry);
                sum += dev * dev;
            }

            Candidate c;
            c.shape = Stroke::EllipseShape;
            const int steps = 64;
            for (int i = 0; i <= steps; ++i) {
                const qreal t = 2 * M_PI * i / steps;
                c.points << StrokePoint(cx + rx * qCos(t), cy + ry * qSin(t));
            }
            c.score = scoreFor(qSqrt(sum / n), tolerance) * 0.98;
            c.label = QStringLiteral("Ellipse");
            out.append(c);
        }
    }

    {
        const QVector<QPointF> hull = convexHull(pts);
        if (hull.size() >= 3) {
            qreal bestArea = -1;
            QVector<QPointF> bestCorners;

            for (int i = 0; i < hull.size(); ++i) {
                const QPointF &p0 = hull.at(i);
                const QPointF &p1 = hull.at((i + 1) % hull.size());
                const qreal edgeLen = QLineF(p0, p1).length();
                if (edgeLen < 1e-6)
                    continue;

                const QPointF ux((p1.x() - p0.x()) / edgeLen, (p1.y() - p0.y()) / edgeLen);
                const QPointF uy(-ux.y(), ux.x());

                qreal minU = 1e18, maxU = -1e18, minV = 1e18, maxV = -1e18;
                for (int j = 0; j < hull.size(); ++j) {
                    const QPointF d = hull.at(j) - p0;
                    const qreal u = d.x() * ux.x() + d.y() * ux.y();
                    const qreal v = d.x() * uy.x() + d.y() * uy.y();
                    minU = qMin(minU, u); maxU = qMax(maxU, u);
                    minV = qMin(minV, v); maxV = qMax(maxV, v);
                }

                const qreal area = (maxU - minU) * (maxV - minV);
                if (bestArea < 0 || area < bestArea) {
                    bestArea = area;
                    bestCorners.clear();
                    const QPointF corners[4] = {
                        QPointF(minU, minV), QPointF(maxU, minV),
                        QPointF(maxU, maxV), QPointF(minU, maxV)
                    };
                    for (int c = 0; c < 4; ++c) {
                        bestCorners << QPointF(
                            p0.x() + corners[c].x() * ux.x() + corners[c].y() * uy.x(),
                            p0.y() + corners[c].x() * ux.y() + corners[c].y() * uy.y());
                    }
                }
            }

            if (bestCorners.size() == 4) {
                const QLineF edge(bestCorners.at(0), bestCorners.at(1));
                qreal angle = edge.angle();
                while (angle >= 90) angle -= 90;
                const bool upright = angle <= m_angleSnap || angle >= 90 - m_angleSnap;
                if (upright) {
                    bestCorners.clear();
                    bestCorners << bbox.topLeft() << bbox.topRight()
                                << bbox.bottomRight() << bbox.bottomLeft();
                }

                qreal w = QLineF(bestCorners.at(0), bestCorners.at(1)).length();
                qreal h = QLineF(bestCorners.at(1), bestCorners.at(2)).length();
                bool square = false;
                if (qMax(w, h) > 0 && qAbs(w - h) / qMax(w, h) <= m_squareSnap) {
                    const qreal side = (w + h) / 2;
                    const QPointF centre((bestCorners.at(0).x() + bestCorners.at(2).x()) / 2,
                                         (bestCorners.at(0).y() + bestCorners.at(2).y()) / 2);
                    const QLineF e0(bestCorners.at(0), bestCorners.at(1));
                    const qreal len0 = qMax(e0.length(), qreal(1e-6));
                    const QPointF ux((e0.dx()) / len0, (e0.dy()) / len0);
                    const QPointF uy(-ux.y(), ux.x());
                    const qreal half = side / 2;

                    bestCorners.clear();
                    const qreal signs[4][2] = { {-1, -1}, {1, -1}, {1, 1}, {-1, 1} };
                    for (int k = 0; k < 4; ++k) {
                        bestCorners << QPointF(
                            centre.x() + signs[k][0] * half * ux.x() + signs[k][1] * half * uy.x(),
                            centre.y() + signs[k][0] * half * ux.y() + signs[k][1] * half * uy.y());
                    }
                    square = true;
                }

                qreal sum = 0;
                for (int i = 0; i < pts.size(); ++i) {
                    const qreal d = distanceToPolygon(pts.at(i), bestCorners);
                    sum += d * d;
                }

                Candidate c;
                c.shape = Stroke::RectShape;
                c.points = toStrokePoints(bestCorners, true);
                c.score = scoreFor(qSqrt(sum / pts.size()), tolerance * 1.35);
                c.label = square ? QStringLiteral("Square") : QStringLiteral("Rectangle");
                out.append(c);
            }
        }
    }

    {
        const qreal epsilon = qMax(qreal(2.0), qreal(diag * 0.05));
        QVector<QPointF> corners = simplify(pts, epsilon);

        if (corners.size() >= 2
                && QLineF(corners.first(), corners.last()).length() < epsilon * 1.5) {
            corners.removeLast();
        }

        if (corners.size() >= 3 && corners.size() <= 8) {
            qreal sum = 0;
            for (int i = 0; i < pts.size(); ++i) {
                const qreal d = distanceToPolygon(pts.at(i), corners);
                sum += d * d;
            }

            static const qreal cornerPenalty[9] = {
                1.0, 1.0, 1.0,
                0.95,
                0.78,
                0.72, 0.66, 0.60, 0.55
            };

            Candidate c;
            c.shape = Stroke::PolygonShape;
            c.points = toStrokePoints(corners, true);
            c.score = scoreFor(qSqrt(sum / pts.size()), tolerance)
                    * cornerPenalty[qBound(0, corners.size(), 8)];
            c.label = corners.size() == 3 ? QStringLiteral("Triangle")
                                          : QStringLiteral("Polygon");
            out.append(c);
        }
    }

    return out;
}

ShapeRecognizer::Candidate ShapeRecognizer::arrowCandidate(
        const QVector<QPointF> &pts, qreal diag) const
{
    Candidate c;

    const qreal epsilon = qMax(qreal(2.0), qreal(diag * 0.05));
    const QVector<QPointF> corners = simplify(pts, epsilon);
    if (corners.size() != 3)
        return c;

    const QPointF a = corners.at(0);
    const QPointF b = corners.at(1);
    const QPointF cc = corners.at(2);

    const qreal shaft = QLineF(a, b).length();
    const qreal barb = QLineF(b, cc).length();
    if (shaft < 20 || barb < shaft * 0.1 || barb > shaft * 0.55)
        return c;

    const QPointF v1(a.x() - b.x(), a.y() - b.y());
    const QPointF v2(cc.x() - b.x(), cc.y() - b.y());
    const qreal dot = v1.x() * v2.x() + v1.y() * v2.y();
    const qreal turn = qRadiansToDegrees(qAcos(qBound(qreal(-1),
            dot / (shaft * barb), qreal(1))));
    if (turn > 75)
        return c;

    const qreal headLen = qBound(qreal(shaft * 0.12), barb, qreal(shaft * 0.3));
    const qreal rad = qDegreesToRadians(qreal(25));
    const QPointF dir((b.x() - a.x()) / shaft, (b.y() - a.y()) / shaft);

    const QPointF barb1(b.x() - headLen * (dir.x() * qCos(rad) - dir.y() * qSin(rad)),
                        b.y() - headLen * (dir.x() * qSin(rad) + dir.y() * qCos(rad)));
    const QPointF barb2(b.x() - headLen * (dir.x() * qCos(-rad) - dir.y() * qSin(-rad)),
                        b.y() - headLen * (dir.x() * qSin(-rad) + dir.y() * qCos(-rad)));

    qreal sum = 0;
    for (int i = 0; i < pts.size(); ++i) {
        const qreal d = qMin(distanceToSegment(pts.at(i), a, b),
                             distanceToSegment(pts.at(i), b, cc));
        sum += d * d;
    }

    c.shape = Stroke::ArrowShape;
    c.points << StrokePoint(a.x(), a.y())
             << StrokePoint(b.x(), b.y())
             << StrokePoint(barb1.x(), barb1.y())
             << StrokePoint(b.x(), b.y())
             << StrokePoint(barb2.x(), barb2.y());
    c.score = scoreFor(qSqrt(sum / pts.size()), diag * 0.06);
    c.label = QStringLiteral("Arrow");
    return c;
}

QVector<ShapeRecognizer::Candidate> ShapeRecognizer::candidates(const Stroke &s) const
{
    QVector<Candidate> out;
    if (s.points.size() < 4)
        return out;

    const QVector<QPointF> pts = resample(s.points, 96);
    if (pts.size() < 4)
        return out;

    const QRectF bbox = boundsOf(pts);
    const qreal diag = qSqrt(bbox.width() * bbox.width() + bbox.height() * bbox.height());

    if (diag < 20.0)
        return out;

    const qreal gap = QLineF(pts.first(), pts.last()).length();
    const qreal length = pathLength(pts);
    const bool closed = gap < diag * 0.3 && length > diag * 1.2;

    if (closed) {
        out += closedCandidates(pts, diag);
    } else {
        out += lineCandidates(pts, diag);
        const Candidate arrow = arrowCandidate(pts, diag);
        if (arrow.shape != Stroke::FreeShape)
            out.append(arrow);
    }

    QVector<Candidate> kept;
    for (int i = 0; i < out.size(); ++i) {
        if (out.at(i).score >= m_threshold)
            kept.append(out.at(i));
    }

    struct Better {
        bool operator()(const Candidate &a, const Candidate &b) const
        {
            return a.score > b.score;
        }
    };
    std::sort(kept.begin(), kept.end(), Better());
    return kept;
}

ShapeRecognizer::Candidate ShapeRecognizer::best(const Stroke &s) const
{
    const QVector<Candidate> all = candidates(s);
    return all.isEmpty() ? Candidate() : all.first();
}

bool ShapeRecognizer::apply(Stroke *s) const
{
    if (!s)
        return false;

    const Candidate c = best(*s);
    if (c.shape == Stroke::FreeShape || c.points.isEmpty())
        return false;

    s->shape = c.shape;
    s->setPoints(c.points);
    s->cap = Stroke::RoundCap;
    return true;
}
}
