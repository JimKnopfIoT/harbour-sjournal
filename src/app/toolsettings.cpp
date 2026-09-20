#include "app/toolsettings.h"

#include "model/stroke.h"
#include "tools/shapefactory.h"

namespace xn {
ToolSettings::ToolSettings(QObject *parent)
    : QObject(parent)
    , m_tool(PenTool)
    , m_color(Qt::black)
    , m_penWidth(2.4)
    , m_dynamicWidth(true)
    , m_fontSize(12)
    , m_fontBold(false)
    , m_fontItalic(false)
    , m_fontUnderline(false)
    , m_textStyle(0)
    , m_selectShape(RectSelect)
    , m_shapeRecognition(false)
    , m_outlineView(false)
    , m_shapeKind(EllipseKind)
    , m_polygonCorners(3)
    , m_equalSides(false)
    , m_fillEnabled(false)
    , m_fillColor(QColor(0x1a, 0x72, 0xd0))
    , m_fillAlpha(120)
    , m_snapSizes(true)
    , m_snapToPoints(true)
    , m_pressureSeen(false)
{
}

void ToolSettings::setTool(Tool t)
{
    if (m_tool == t)
        return;
    m_tool = t;
    if (t != ShapeTool)
        resetCorners();
    Q_EMIT toolChanged();
}

void ToolSettings::setColor(const QColor &c)
{
    if (m_color == c)
        return;
    m_color = c;
    Q_EMIT colorChanged();
}

void ToolSettings::setPenWidth(qreal w)
{
    const qreal clamped = qBound(qreal(0.2), w, qreal(40.0));
    if (qFuzzyCompare(m_penWidth, clamped))
        return;
    m_penWidth = clamped;
    Q_EMIT penWidthChanged();
}

void ToolSettings::setDynamicWidth(bool on)
{
    if (m_dynamicWidth == on)
        return;
    m_dynamicWidth = on;
    Q_EMIT dynamicWidthChanged();
}

void ToolSettings::setFontSize(qreal size)
{
    const qreal clamped = qBound(qreal(4), size, qreal(200));
    if (qFuzzyCompare(m_fontSize, clamped))
        return;
    m_fontSize = clamped;
    Q_EMIT fontSizeChanged();
}

void ToolSettings::setFontBold(bool on)
{
    if (m_fontBold == on)
        return;
    m_fontBold = on;
    Q_EMIT fontFormChanged();
}

void ToolSettings::setFontItalic(bool on)
{
    if (m_fontItalic == on)
        return;
    m_fontItalic = on;
    Q_EMIT fontFormChanged();
}

void ToolSettings::setFontUnderline(bool on)
{
    if (m_fontUnderline == on)
        return;
    m_fontUnderline = on;
    Q_EMIT fontFormChanged();
}

void ToolSettings::setTextStyle(int style)
{
    const int clamped = qBound(0, style, 2);
    if (m_textStyle == clamped)
        return;
    m_textStyle = clamped;
    Q_EMIT textStyleChanged();
}

void ToolSettings::setShapeRecognition(bool on)
{
    if (m_shapeRecognition == on)
        return;
    m_shapeRecognition = on;
    Q_EMIT shapeRecognitionChanged();
}

void ToolSettings::setOutlineView(bool on)
{
    if (m_outlineView == on)
        return;
    m_outlineView = on;
    Q_EMIT outlineViewChanged();
}

void ToolSettings::setShapeKind(ShapeKind kind)
{
    if (m_shapeKind == kind)
        return;
    m_shapeKind = kind;
    resetCorners();
    Q_EMIT shapeKindChanged();
}

void ToolSettings::resetCorners()
{
    if (m_polygonCorners == 3)
        return;
    m_polygonCorners = 3;
    Q_EMIT polygonCornersChanged();
}

void ToolSettings::setPolygonCorners(int corners)
{
    const int clamped = ShapeFactory::clampCorners(corners);
    if (m_polygonCorners == clamped)
        return;
    m_polygonCorners = clamped;
    Q_EMIT polygonCornersChanged();
}

void ToolSettings::setEqualSides(bool on)
{
    if (m_equalSides == on)
        return;
    m_equalSides = on;
    Q_EMIT equalSidesChanged();
}

int ToolSettings::currentShape() const
{
    switch (m_shapeKind) {
    case RectKind:         return Stroke::RectShape;
    case PolygonKind:      return Stroke::PolygonShape;
    case StarKind:         return Stroke::StarShape;
    case LineKind:         return Stroke::LineShape;
    case ArrowKind:        return Stroke::ArrowShape;
    case PolylineKind:     return Stroke::PolylineShape;
    case SplineKind:
    case ClosedSplineKind:
    case EllipseKind:      break;
    }
    return Stroke::EllipseShape;
}

bool ToolSettings::placesPoints() const
{
    if (m_tool == BezierTool)
        return true;
    if (m_tool != ShapeTool)
        return false;
    return m_shapeKind == PolylineKind || m_shapeKind == SplineKind
            || m_shapeKind == ClosedSplineKind;
}

void ToolSettings::setFillEnabled(bool on)
{
    if (m_fillEnabled == on)
        return;
    m_fillEnabled = on;
    Q_EMIT fillChanged();
}

void ToolSettings::setFillColor(const QColor &c)
{
    if (m_fillColor == c)
        return;
    m_fillColor = c;
    Q_EMIT fillChanged();
}

void ToolSettings::setFillAlpha(int alpha)
{
    const int clamped = qBound(0, alpha, 255);
    if (m_fillAlpha == clamped)
        return;
    m_fillAlpha = clamped;
    Q_EMIT fillChanged();
}

void ToolSettings::setLineStyle(const QString &style)
{
    if (m_lineStyle == style)
        return;
    m_lineStyle = style;
    Q_EMIT lineStyleChanged();
}

void ToolSettings::setSnapSizes(bool on)
{
    if (m_snapSizes == on)
        return;
    m_snapSizes = on;
    Q_EMIT snapSizesChanged();
}

void ToolSettings::setSnapToPoints(bool on)
{
    if (m_snapToPoints == on)
        return;
    m_snapToPoints = on;
    Q_EMIT snapToPointsChanged();
}

qreal ToolSettings::snapStep() const
{
    return m_snapSizes ? ShapeFactory::kSnapStep : 0.0;
}

void ToolSettings::setSelectShape(SelectShape s)
{
    if (m_selectShape == s)
        return;
    m_selectShape = s;
    Q_EMIT selectShapeChanged();
}

void ToolSettings::reportHint(const QString &text)
{
    if (m_hintText == text)
        return;
    m_hintText = text;
    Q_EMIT hintTextChanged();
}

void ToolSettings::reportPressure(qreal minSeen, qreal maxSeen)
{
    const QString range = QStringLiteral("%1 – %2")
            .arg(minSeen, 0, 'f', 2).arg(maxSeen, 0, 'f', 2);
    if (m_pressureSeen && m_pressureRange == range)
        return;
    m_pressureSeen = true;
    m_pressureRange = range;
    Q_EMIT pressureSeenChanged();
}
}
