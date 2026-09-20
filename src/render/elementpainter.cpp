#include "render/elementpainter.h"

#include "model/imageitem.h"
#include "model/page.h"
#include "model/path.h"
#include "model/stroke.h"
#include "model/textitem.h"
#include "render/strokegeometry.h"

#include <QFont>
#include <QFontMetricsF>
#include <QPainterPath>
#include <QStringList>
#include <QPainter>

namespace xn {
namespace ElementPainter {
static Qt::PenCapStyle capStyle(Stroke::Cap c)
{
    switch (c) {
    case Stroke::ButtCap: return Qt::FlatCap;
    case Stroke::SquareCap: return Qt::SquareCap;
    case Stroke::RoundCap: break;
    }
    return Qt::RoundCap;
}

static Qt::PenStyle penStyle(const QString &lineStyle)
{
    if (lineStyle == QLatin1String("dash")) return Qt::DashLine;
    if (lineStyle == QLatin1String("dot")) return Qt::DotLine;
    if (lineStyle == QLatin1String("dashdot")) return Qt::DashDotLine;
    return Qt::SolidLine;
}

void drawBackground(QPainter *p, const Page *page)
{
    if (!page)
        return;

    const QRectF r(0, 0, page->width, page->height);
    p->fillRect(r, page->background.color);

    const QString style = page->background.style;
    if (style.isEmpty() || style == QLatin1String("plain"))
        return;

    const qreal spacing = 14.17;
    QPen rule(QColor(0xbf, 0xcf, 0xdf));
    rule.setWidthF(0.5);
    p->setPen(rule);

    if (style == QLatin1String("lined") || style == QLatin1String("ruled")) {
        for (qreal y = spacing * 5; y < page->height; y += spacing)
            p->drawLine(QPointF(0, y), QPointF(page->width, y));
        if (style == QLatin1String("ruled")) {
            QPen margin(QColor(0xff, 0x80, 0x80));
            margin.setWidthF(0.5);
            p->setPen(margin);
            p->drawLine(QPointF(spacing * 5, 0), QPointF(spacing * 5, page->height));
        }
    } else if (style == QLatin1String("graph")) {
        for (qreal x = spacing; x < page->width; x += spacing)
            p->drawLine(QPointF(x, 0), QPointF(x, page->height));
        for (qreal y = spacing; y < page->height; y += spacing)
            p->drawLine(QPointF(0, y), QPointF(page->width, y));
    } else if (style == QLatin1String("dotted")) {
        p->setBrush(QColor(0xbf, 0xcf, 0xdf));
        p->setPen(Qt::NoPen);
        for (qreal x = spacing; x < page->width; x += spacing) {
            for (qreal y = spacing; y < page->height; y += spacing)
                p->drawEllipse(QPointF(x, y), 0.6, 0.6);
        }
        p->setBrush(Qt::NoBrush);
    }
}

static const QColor kOutlineColor(0x14, 0x1c, 0x28);

void drawStroke(QPainter *p, const Stroke *s, bool outlineOnly)
{
    if (!s || s->points.isEmpty())
        return;

    if (outlineOnly) {
        p->save();
        p->setRenderHint(QPainter::Antialiasing, false);
        QPen hairline(kOutlineColor);
        hairline.setWidth(0);
        hairline.setCosmetic(true);
        p->setPen(hairline);
        p->setBrush(Qt::NoBrush);
        p->drawPath(StrokeGeometry::centerline(*s));
        p->restore();
        return;
    }

    QColor color = s->color;
    p->save();
    if (s->tool == Stroke::Highlighter) {
        color.setAlpha(0x7f);
        p->setCompositionMode(QPainter::CompositionMode_Multiply);
    }

    if (s->fill >= 0) {
        QPainterPath fillPath = StrokeGeometry::centerline(*s);
        fillPath.closeSubpath();
        if (Gradient::isReal(s->gradient)) {
            p->fillPath(fillPath, Gradient::forRect(s->gradient, s->bounds(),
                                                    s->gradientAngle, s->fill));
        } else {
            QColor fill = s->fillColor.isValid() ? s->fillColor : color;
            fill.setAlpha(s->fill);
            p->fillPath(fillPath, fill);
        }
    }

    if (StrokeGeometry::isUniformWidth(*s)) {
        QPen pen(color);
        pen.setWidthF(s->width);
        pen.setCapStyle(capStyle(s->cap));
        pen.setJoinStyle(Qt::RoundJoin);
        pen.setStyle(penStyle(s->lineStyle));
        p->setPen(pen);
        p->setBrush(Qt::NoBrush);
        p->drawPath(StrokeGeometry::centerline(*s));
    } else {
        QPen pen(color);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        p->setBrush(Qt::NoBrush);
        for (int i = 0; i < s->points.size() - 1; ++i) {
            const StrokePoint &a = s->points.at(i);
            const StrokePoint &b = s->points.at(i + 1);
            pen.setWidthF((s->widthAt(i) + s->widthAt(i + 1)) / 2);
            p->setPen(pen);
            p->drawLine(QPointF(a.x, a.y), QPointF(b.x, b.y));
        }
        if (s->points.size() == 1) {
            const StrokePoint &a = s->points.first();
            p->setPen(Qt::NoPen);
            p->setBrush(color);
            p->drawEllipse(QPointF(a.x, a.y), s->widthAt(0) / 2, s->widthAt(0) / 2);
        }
    }

    p->restore();
}

void drawPath(QPainter *p, const Path *path, bool outlineOnly)
{
    if (!path || path->segments.isEmpty())
        return;

    const QPainterPath geometry = path->painterPath();
    p->save();

    if (outlineOnly) {
        p->setRenderHint(QPainter::Antialiasing, false);
        QPen hairline(kOutlineColor);
        hairline.setWidth(0);
        hairline.setCosmetic(true);
        p->setPen(hairline);
        p->setBrush(Qt::NoBrush);
        p->drawPath(geometry);
        p->restore();
        return;
    }

    if (path->fill >= 0) {
        if (Gradient::isReal(path->gradient)) {
            p->fillPath(geometry, Gradient::forRect(path->gradient, path->bounds(),
                                                    path->gradientAngle, path->fill));
        } else {
            QColor fill = path->fillColor.isValid() ? path->fillColor : path->color;
            fill.setAlpha(path->fill);
            p->fillPath(geometry, fill);
        }
    }

    QPen pen(path->color);
    pen.setWidthF(path->width);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setStyle(penStyle(path->lineStyle));
    p->setPen(pen);
    p->setBrush(Qt::NoBrush);
    p->drawPath(geometry);

    p->restore();
}

void drawElement(QPainter *p, const Element *e, bool outlineOnly)
{
    if (!e)
        return;

    switch (e->type()) {
    case Element::StrokeType:
        drawStroke(p, static_cast<const Stroke *>(e), outlineOnly);
        break;
    case Element::PathType:
        drawPath(p, static_cast<const Path *>(e), outlineOnly);
        break;
    case Element::TextType: {
        const TextItem *t = static_cast<const TextItem *>(e);
        p->save();
        p->setFont(t->font());
        if (outlineOnly || t->style == TextItem::Filled) {
            p->setPen(outlineOnly ? kOutlineColor : t->color);
            p->drawText(QRectF(t->pos, QSizeF(10000, 10000)),
                        Qt::AlignLeft | Qt::AlignTop, t->text);
        } else {
            const QFontMetricsF fm(t->font());
            const QStringList lines = t->text.split(QLatin1Char('\n'));
            QPainterPath glyphs;
            for (int i = 0; i < lines.size(); ++i) {
                glyphs.addText(t->pos.x(), t->pos.y() + fm.ascent() + i * fm.height(),
                               t->font(), lines.at(i));
            }
            QPen pen(t->style == TextItem::Inline ? Qt::white : t->color);
            pen.setWidthF(qMax(qreal(0.4), qreal(t->fontSize * 0.055)));
            pen.setJoinStyle(Qt::RoundJoin);
            p->setPen(pen);
            p->setBrush(t->style == TextItem::Inline ? QBrush(t->color) : Qt::NoBrush);
            p->drawPath(glyphs);
        }
        p->restore();
        break;
    }
    case Element::ImageType: {
        const ImageItem *i = static_cast<const ImageItem *>(e);
        if (i->image.isNull())
            break;
        if (outlineOnly) {
            p->save();
            p->setRenderHint(QPainter::Antialiasing, false);
            QPen hairline(kOutlineColor);
            hairline.setWidth(0);
            hairline.setCosmetic(true);
            p->setPen(hairline);
            p->setBrush(Qt::NoBrush);
            p->drawRect(i->rect);
            p->drawLine(i->rect.topLeft(), i->rect.bottomRight());
            p->drawLine(i->rect.topRight(), i->rect.bottomLeft());
            p->restore();
        } else {
            p->drawImage(i->rect, i->image);
        }
        break;
    }
    }
}
}
}
