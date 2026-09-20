#include "tools/strokebuilder.h"

#include <QLineF>

namespace xn {
static const qreal kReferenceSpeed = 1.6;

static const qreal kDefaultSpread = 0.55;

static const qreal kWidthSmoothing = 0.35;

StrokeBuilder::StrokeBuilder()
    : m_stroke(0)
    , m_tool(Stroke::Pen)
    , m_color(Qt::black)
    , m_baseWidth(1.41)
    , m_dynamicWidth(true)
    , m_widthSpread(kDefaultSpread)
    , m_pressureEnabled(true)
    , m_minDistance(0.8)
    , m_lastTime(0)
    , m_smoothedWidth(1.41)
    , m_pressureMin(1.0)
    , m_pressureMax(0.0)
{
}

bool StrokeBuilder::pressureIsUsable() const
{
    return m_pressureMax > m_pressureMin + 0.02;
}

StrokeBuilder::~StrokeBuilder()
{
    delete m_stroke;
}

void StrokeBuilder::begin(const QPointF &pagePos, qreal pressure, qint64 timeMs)
{
    delete m_stroke;
    m_stroke = new Stroke;
    m_stroke->tool = m_tool;
    m_stroke->color = m_color;
    m_stroke->width = m_baseWidth;
    m_stroke->cap = Stroke::RoundCap;

    m_lastPos = pagePos;
    m_lastTime = timeMs;
    m_smoothedWidth = m_baseWidth;

    if (pressure > 0.0 && pressure < 1.0) {
        m_pressureMin = qMin(m_pressureMin, pressure);
        m_pressureMax = qMax(m_pressureMax, pressure);
    }
    m_stroke->addPoint(StrokePoint(pagePos.x(), pagePos.y(), m_baseWidth));
}

bool StrokeBuilder::extend(const QPointF &pagePos, qreal pressure, qint64 timeMs)
{
    if (!m_stroke)
        return false;

    const qreal moved = QLineF(m_lastPos, pagePos).length();
    if (moved < m_minDistance)
        return false;

    const qreal w = widthFor(pagePos, pressure, timeMs);
    m_stroke->addPoint(StrokePoint(pagePos.x(), pagePos.y(), w));

    m_lastPos = pagePos;
    m_lastTime = timeMs;
    return true;
}

qreal StrokeBuilder::widthFor(const QPointF &pos, qreal pressure, qint64 timeMs)
{
    qreal target = m_baseWidth;

    if (pressure > 0.0 && pressure < 1.0) {
        m_pressureMin = qMin(m_pressureMin, pressure);
        m_pressureMax = qMax(m_pressureMax, pressure);
    }

    if (m_pressureEnabled && pressureIsUsable() && pressure > 0 && pressure < 1) {
        const qreal span = m_pressureMax - m_pressureMin;
        const qreal normalized = qBound(qreal(0), (pressure - m_pressureMin) / span, qreal(1));
        target = m_baseWidth * (0.4 + 0.6 * normalized);
    } else if (m_dynamicWidth) {
        const qreal dt = qreal(qMax<qint64>(timeMs - m_lastTime, 1));
        const qreal speed = QLineF(m_lastPos, pos).length() / dt;
        const qreal norm = qBound(qreal(0), speed / kReferenceSpeed, qreal(1));
        target = m_baseWidth * (1.0 - m_widthSpread * norm);
    }

    m_smoothedWidth = m_smoothedWidth * (1 - kWidthSmoothing) + target * kWidthSmoothing;
    return m_smoothedWidth;
}

Stroke *StrokeBuilder::take()
{
    Stroke *s = m_stroke;
    m_stroke = 0;

    if (!s)
        return 0;

    if (s->points.isEmpty()) {
        delete s;
        return 0;
    }

    if (s->points.size() == 1) {
        const StrokePoint p = s->points.first();
        s->addPoint(StrokePoint(p.x + 0.01, p.y, p.width));
    }

    return s;
}

void StrokeBuilder::cancel()
{
    delete m_stroke;
    m_stroke = 0;
}
}
