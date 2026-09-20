#include "app/styletools.h"

#include "app/selection.h"
#include "app/undostack.h"
#include "model/element.h"

namespace xn {
StyleTools::StyleTools(Selection *selection, UndoStack *undo, QObject *parent)
    : QObject(parent)
    , m_selection(selection)
    , m_undo(undo)
{
    connect(m_selection, &Selection::changed, this, &StyleTools::changed);
}

bool StyleTools::canRestyle() const
{
    return m_selection->count() > 0;
}

bool StyleTools::canFill() const
{
    const QVector<Element *> &items = m_selection->elements();
    for (int i = 0; i < items.size(); ++i) {
        const Element::Type t = items.at(i)->type();
        if (t == Element::StrokeType || t == Element::PathType)
            return true;
    }
    return false;
}

bool StyleTools::isFilled() const
{
    const QVector<Element *> &items = m_selection->elements();
    for (int i = 0; i < items.size(); ++i)
        if (ChangeStyleCommand::styleOf(items.at(i)).fill >= 0)
            return true;
    return false;
}

QColor StyleTools::fillColor() const
{
    const QVector<Element *> &items = m_selection->elements();
    if (items.isEmpty())
        return QColor();
    const ElementStyle s = ChangeStyleCommand::styleOf(items.first());
    return s.fillColor.isValid() ? s.fillColor : s.color;
}

int StyleTools::fillAlpha() const
{
    const QVector<Element *> &items = m_selection->elements();
    if (items.isEmpty())
        return 255;
    const int a = ChangeStyleCommand::styleOf(items.first()).fill;
    return a < 0 ? 255 : a;
}

qreal StyleTools::strokeWidth() const
{
    const QVector<Element *> &items = m_selection->elements();
    if (items.isEmpty())
        return 0;
    return ChangeStyleCommand::styleOf(items.first()).width;
}

QVariantList StyleTools::gradientColors() const
{
    QVariantList out;
    const QVector<Element *> &items = m_selection->elements();
    if (items.isEmpty())
        return out;

    const GradientStops stops = ChangeStyleCommand::styleOf(items.first()).gradient;
    for (int i = 0; i < stops.size(); ++i)
        out << QVariant(stops.at(i).color);
    return out;
}

int StyleTools::gradientAngle() const
{
    const QVector<Element *> &items = m_selection->elements();
    if (items.isEmpty())
        return 0;
    return ChangeStyleCommand::styleOf(items.first()).gradientAngle;
}

QColor StyleTools::color() const
{
    const QVector<Element *> &items = m_selection->elements();
    if (items.isEmpty())
        return QColor();
    return ChangeStyleCommand::styleOf(items.first()).color;
}

bool StyleTools::apply(Field field, const QColor &color, qreal width, int alpha,
                       const GradientStops &stops, int angle)
{
    const QVector<Element *> &items = m_selection->elements();
    if (items.isEmpty())
        return false;

    QVector<ElementStyle> before;
    QVector<ElementStyle> after;
    bool differs = false;

    for (int i = 0; i < items.size(); ++i) {
        const ElementStyle was = ChangeStyleCommand::styleOf(items.at(i));
        ElementStyle now = was;

        if (field == Colour) {
            now.color = color;
        } else if (field == Width) {
            now.width = width;
        } else {
            const bool fillable = items.at(i)->type() == Element::StrokeType
                    || items.at(i)->type() == Element::PathType;
            if (!fillable) {
                before << was;
                after << was;
                continue;
            }
            if (field == Gradient) {
                now.gradient = stops;
                now.gradientAngle = angle;
                now.fill = alpha;
            } else {
                now.gradient = GradientStops();
                now.fill = alpha;
                if (alpha >= 0)
                    now.fillColor = color;
            }
        }

        if (now.color != was.color || now.fill != was.fill
                || now.fillColor != was.fillColor || now.width != was.width
                || now.gradient != was.gradient || now.gradientAngle != was.gradientAngle)
            differs = true;

        before << was;
        after << now;
    }

    if (!differs)
        return false;

    for (int i = 0; i < items.size(); ++i)
        ChangeStyleCommand::applyStyle(items.at(i), after.at(i));

    m_undo->pushWithoutRedo(new ChangeStyleCommand(items, before, after));
    Q_EMIT contentChanged(m_selection->page());
    Q_EMIT historyChanged();
    Q_EMIT changed();
    return true;
}

bool StyleTools::applyColor(const QColor &color)
{
    return apply(Colour, color, -1, -1);
}

bool StyleTools::applyWidth(qreal width)
{
    return apply(Width, QColor(), width, -1);
}

bool StyleTools::applyFill(const QColor &color, int alpha)
{
    return apply(Fill, color, -1, qBound(0, alpha, 255));
}

bool StyleTools::removeFill()
{
    return apply(Fill, QColor(), -1, -1);
}

bool StyleTools::toggleFill(const QColor &color, int alpha)
{
    return isFilled() ? removeFill() : applyFill(color, alpha);
}

bool StyleTools::applyGradient(const QVariantList &colors, int angle, int alpha)
{
    QVector<QColor> wanted;
    for (int i = 0; i < colors.size(); ++i) {
        const QColor c = colors.at(i).value<QColor>();
        if (c.isValid())
            wanted << c;
    }

    const GradientStops stops = xn::Gradient::evenly(wanted);
    if (!xn::Gradient::isReal(stops))
        return false;

    return apply(Gradient, QColor(), -1, qBound(0, alpha, 255), stops, angle);
}

bool StyleTools::clearGradient()
{
    return apply(Fill, fillColor(), -1, fillAlpha());
}
}
