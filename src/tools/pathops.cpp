#include "tools/pathops.h"

#include "model/element.h"
#include "model/path.h"
#include "model/stroke.h"
#include "model/textitem.h"

#include <QFontMetricsF>
#include <QStringList>

namespace xn {
static void copyStyle(Path *target, const Element *source)
{
    if (!source)
        return;

    if (source->type() == Element::PathType) {
        const Path *p = static_cast<const Path *>(source);
        target->color = p->color;
        target->width = p->width;
        target->lineStyle = p->lineStyle;
        target->fill = p->fill;
        target->fillColor = p->fillColor;
        target->gradient = p->gradient;
        target->gradientAngle = p->gradientAngle;
    } else if (source->type() == Element::StrokeType) {
        const Stroke *s = static_cast<const Stroke *>(source);
        target->color = s->color;
        target->width = s->width;
        target->lineStyle = s->lineStyle;
        target->fill = s->fill;
        target->fillColor = s->fillColor;
        target->gradient = s->gradient;
        target->gradientAngle = s->gradientAngle;
    } else if (source->type() == Element::TextType) {
        const TextItem *t = static_cast<const TextItem *>(source);
        target->color = t->color;
        // fill is an alpha, not a flag: 0 would be an invisible fill.
        target->fill = t->style == TextItem::Outline ? -1 : 255;
        target->fillColor = t->color;
    }
}

bool PathOps::isConvertible(const Element *e)
{
    return e && (e->type() == Element::PathType
                 || e->type() == Element::StrokeType
                 || e->type() == Element::TextType);
}

QPainterPath PathOps::toPainterPath(const Element *e)
{
    QPainterPath out;
    if (!e)
        return out;

    if (e->type() == Element::PathType)
        return static_cast<const Path *>(e)->painterPath();

    if (e->type() == Element::StrokeType) {
        const Stroke *s = static_cast<const Stroke *>(e);
        if (s->points.isEmpty())
            return out;
        out.moveTo(s->points.at(0).x, s->points.at(0).y);
        for (int i = 1; i < s->points.size(); ++i)
            out.lineTo(s->points.at(i).x, s->points.at(i).y);
        if (s->shape == Stroke::RectShape || s->shape == Stroke::EllipseShape
                || s->shape == Stroke::PolygonShape || s->shape == Stroke::StarShape)
            out.closeSubpath();
        return out;
    }

    const TextItem *t = static_cast<const TextItem *>(e);
    const QFontMetricsF fm(t->font());
    const QStringList lines = t->text.split(QLatin1Char('\n'));
    for (int i = 0; i < lines.size(); ++i)
        out.addText(t->pos.x(), t->pos.y() + fm.ascent() + i * fm.height(),
                    t->font(), lines.at(i));
    return out;
}

// Walk the QPainterPath once and hand back its contours in order.
static QVector<SubPath> contoursOf(const QPainterPath &path)
{
    QVector<SubPath> out;
    SubPath current;
    bool open = false;

    for (int i = 0; i < path.elementCount(); ++i) {
        const QPainterPath::Element &e = path.elementAt(i);

        if (e.type == QPainterPath::MoveToElement) {
            if (open && !current.segments.isEmpty())
                out << current;
            current = SubPath();
            current.start = QPointF(e.x, e.y);
            open = true;
            continue;
        }

        if (!open)
            continue;

        const QPointF from = current.segments.isEmpty() ? current.start
                                                        : current.segments.last().to;

        if (e.type == QPainterPath::LineToElement) {
            const QPointF to(e.x, e.y);
            current.segments << CubicSegment(from + (to - from) / 3.0,
                                             from + (to - from) * 2.0 / 3.0, to);
        } else if (e.type == QPainterPath::CurveToElement && i + 2 < path.elementCount()) {
            const QPainterPath::Element &c2 = path.elementAt(i + 1);
            const QPainterPath::Element &to = path.elementAt(i + 2);
            current.segments << CubicSegment(QPointF(e.x, e.y), QPointF(c2.x, c2.y),
                                             QPointF(to.x, to.y));
            i += 2;
        }
    }

    if (open && !current.segments.isEmpty())
        out << current;
    return out;
}

static Path *pathFromContour(const SubPath &contour, const Element *style)
{
    Path *p = new Path;
    copyStyle(p, style);
    p->setSegments(contour.start, contour.segments);
    p->closed = QLineF(contour.start, contour.segments.last().to).length() < 0.01;
    return p;
}

Path *PathOps::fromPainterPath(const QPainterPath &path, const Element *style)
{
    const QVector<SubPath> contours = contoursOf(path);
    if (contours.isEmpty())
        return 0;

    Path *out = pathFromContour(contours.first(), style);
    for (int i = 1; i < contours.size(); ++i)
        out->extra << contours.at(i);
    if (contours.size() > 1)
        out->closed = true;
    out->invalidate();
    return out;
}

QVector<Path *> PathOps::splitSubpaths(const QPainterPath &path, const Element *style)
{
    QVector<Path *> out;
    const QVector<SubPath> contours = contoursOf(path);
    for (int i = 0; i < contours.size(); ++i)
        out << pathFromContour(contours.at(i), style);
    return out;
}

static QPainterPath filledUnion(const QVector<Element *> &elements, int from)
{
    QPainterPath combined;
    for (int i = from; i < elements.size(); ++i) {
        if (!PathOps::isConvertible(elements.at(i)))
            continue;
        QPainterPath one = PathOps::toPainterPath(elements.at(i));
        if (one.isEmpty())
            continue;
        one.closeSubpath();
        combined = combined.isEmpty() ? one : combined.united(one);
    }
    return combined;
}

QVector<Path *> PathOps::unite(const QVector<Element *> &elements)
{
    if (elements.size() < 2)
        return QVector<Path *>();

    const QPainterPath combined = filledUnion(elements, 0);
    if (combined.isEmpty())
        return QVector<Path *>();

    QVector<Path *> out;
    // Separate islands stay separate elements; holes stay inside their island.
    const QList<QPolygonF> islands = combined.simplified().toSubpathPolygons();
    if (islands.size() > 1)
        return splitSubpaths(combined.simplified(), elements.first());
    if (Path *one = fromPainterPath(combined.simplified(), elements.first()))
        out << one;
    return out;
}

QVector<Path *> PathOps::subtract(const QVector<Element *> &elements)
{
    if (elements.size() < 2 || !isConvertible(elements.first()))
        return QVector<Path *>();

    QPainterPath base = toPainterPath(elements.first());
    if (base.isEmpty())
        return QVector<Path *>();
    base.closeSubpath();

    const QPainterPath cutter = filledUnion(elements, 1);
    if (cutter.isEmpty())
        return QVector<Path *>();

    const QPainterPath result = base.subtracted(cutter);
    if (result.isEmpty())
        return QVector<Path *>();

    QVector<Path *> out;
    if (Path *one = fromPainterPath(result.simplified(), elements.first()))
        out << one;
    return out;
}

QVector<Path *> PathOps::breakApart(const Element *element)
{
    if (!isConvertible(element))
        return QVector<Path *>();

    const QVector<Path *> parts = splitSubpaths(toPainterPath(element), element);
    if (parts.size() < 2) {
        qDeleteAll(parts);
        return QVector<Path *>();
    }
    return parts;
}

// One path per character, so the counter of an e stays a hole in that e.
QVector<Path *> PathOps::textToPath(const TextItem *text)
{
    QVector<Path *> out;
    if (!text || text->text.isEmpty())
        return out;

    const QFontMetricsF fm(text->font());
    const QStringList lines = text->text.split(QLatin1Char('\n'));

    for (int line = 0; line < lines.size(); ++line) {
        const qreal baseline = text->pos.y() + fm.ascent() + line * fm.height();
        qreal x = text->pos.x();

        for (int i = 0; i < lines.at(line).size(); ++i) {
            const QString glyph = lines.at(line).mid(i, 1);
            const qreal advance = fm.width(glyph);
            if (!glyph.trimmed().isEmpty()) {
                QPainterPath one;
                one.addText(x, baseline, text->font(), glyph);
                if (Path *p = fromPainterPath(one, text))
                    out << p;
            }
            x += advance;
        }
    }
    return out;
}

Path *PathOps::join(const QVector<Element *> &elements)
{
    QPointF start;
    QVector<CubicSegment> segments;
    bool haveStart = false;
    const Element *style = 0;

    for (int i = 0; i < elements.size(); ++i) {
        if (!isConvertible(elements.at(i)))
            continue;

        const QVector<Path *> parts = splitSubpaths(toPainterPath(elements.at(i)),
                                                    elements.at(i));
        for (int j = 0; j < parts.size(); ++j) {
            const Path *p = parts.at(j);
            if (!haveStart) {
                start = p->start;
                haveStart = true;
                style = elements.at(i);
            } else {
                // Bridge the gap so the pieces really become one outline.
                const QPointF from = segments.isEmpty() ? start : segments.last().to;
                const QPointF to = p->start;
                if (QLineF(from, to).length() > 0.01)
                    segments << CubicSegment(from + (to - from) / 3.0,
                                             from + (to - from) * 2.0 / 3.0, to);
            }
            segments += p->segments;
        }
        qDeleteAll(parts);
    }

    if (!haveStart || segments.isEmpty())
        return 0;

    Path *out = new Path;
    copyStyle(out, style);
    out->setSegments(start, segments);
    out->closed = QLineF(start, segments.last().to).length() < 0.01;
    return out;
}
}
