#ifndef XN_STROKEBUILDER_H
#define XN_STROKEBUILDER_H

#include "model/stroke.h"

#include <QColor>
#include <QPointF>

namespace xn {
class StrokeBuilder
{
public:
    StrokeBuilder();
    ~StrokeBuilder();

    void setTool(Stroke::Tool t) { m_tool = t; }
    void setColor(const QColor &c) { m_color = c; }
    void setBaseWidth(qreal w) { m_baseWidth = w; }

    void setDynamicWidth(bool on) { m_dynamicWidth = on; }

    void setWidthSpread(qreal spread) { m_widthSpread = spread; }

    void setPressureEnabled(bool on) { m_pressureEnabled = on; }

    bool pressureIsUsable() const;

    qreal observedPressureMin() const { return m_pressureMin; }
    qreal observedPressureMax() const { return m_pressureMax; }

    void setMinDistance(qreal d) { m_minDistance = d; }

    bool isActive() const { return m_stroke != 0; }
    const Stroke *peek() const { return m_stroke; }

    void begin(const QPointF &pagePos, qreal pressure, qint64 timeMs);

    bool extend(const QPointF &pagePos, qreal pressure, qint64 timeMs);

    Stroke *take();

    void cancel();

private:
    qreal widthFor(const QPointF &pos, qreal pressure, qint64 timeMs);

    Stroke *m_stroke;
    Stroke::Tool m_tool;
    QColor m_color;
    qreal m_baseWidth;
    bool m_dynamicWidth;
    qreal m_widthSpread;
    bool m_pressureEnabled;
    qreal m_minDistance;

    QPointF m_lastPos;
    qint64 m_lastTime;
    qreal m_smoothedWidth;

    qreal m_pressureMin;
    qreal m_pressureMax;
};
}

#endif
