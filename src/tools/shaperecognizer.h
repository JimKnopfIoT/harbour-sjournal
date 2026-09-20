#ifndef XN_SHAPERECOGNIZER_H
#define XN_SHAPERECOGNIZER_H

#include "model/stroke.h"

#include <QString>
#include <QVector>

namespace xn {
class ShapeRecognizer
{
public:
    struct Candidate
    {
        Candidate(): shape(Stroke::FreeShape), score(0) {}

        Stroke::Shape shape;
        QVector<StrokePoint> points;
        qreal score;
        QString label;                 // for the UI: "Rectangle", "Arrow", …
    };

    ShapeRecognizer();

    QVector<Candidate> candidates(const Stroke &s) const;

    Candidate best(const Stroke &s) const;

    bool apply(Stroke *s) const;

    void setThreshold(qreal t) { m_threshold = t; }

    void setAngleSnap(qreal degrees) { m_angleSnap = degrees; }

    void setSquareSnap(qreal fraction) { m_squareSnap = fraction; }

private:
    QVector<Candidate> lineCandidates(const QVector<QPointF> &pts, qreal diag) const;
    QVector<Candidate> closedCandidates(const QVector<QPointF> &pts, qreal diag) const;
    Candidate arrowCandidate(const QVector<QPointF> &pts, qreal diag) const;

    qreal m_threshold;
    qreal m_angleSnap;
    qreal m_squareSnap;
};
}

#endif
