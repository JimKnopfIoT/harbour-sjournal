#include "io/svgexporter.h"

#include "model/imageitem.h"
#include "model/page.h"
#include "model/path.h"
#include "model/stroke.h"
#include "model/element.h"
#include "model/layer.h"
#include "model/textitem.h"
#include "render/strokegeometry.h"

#include <QBuffer>
#include <QFile>
#include <QFontMetricsF>
#include <QLineF>
#include <QStringList>
#include <qmath.h>
#include <QXmlStreamWriter>

namespace xn {
static const char *SVG_NS = "http://www.w3.org/2000/svg";
static const char *XLINK_NS = "http://www.w3.org/1999/xlink";
static const char *INKSCAPE_NS = "http://www.inkscape.org/namespaces/inkscape";
static const char *SODIPODI_NS = "http://sodipodi.sourceforge.net/DTD/sodipodi-0.0.dtd";

static QString num(qreal v)
{
    return QString::number(v, 'g', 6);
}

static QString rgb(const QColor &c)
{
    return QString(QStringLiteral("#%1%2%3"))
            .arg(c.red(), 2, 16, QLatin1Char('0'))
            .arg(c.green(), 2, 16, QLatin1Char('0'))
            .arg(c.blue(), 2, 16, QLatin1Char('0'));
}

static QRectF pointBounds(const Stroke *s)
{
    if (s->points.isEmpty())
        return QRectF();
    qreal minX = s->points.first().x, maxX = minX;
    qreal minY = s->points.first().y, maxY = minY;
    for (int i = 1; i < s->points.size(); ++i) {
        const StrokePoint &p = s->points.at(i);
        minX = qMin(minX, p.x); maxX = qMax(maxX, p.x);
        minY = qMin(minY, p.y); maxY = qMax(maxY, p.y);
    }
    return QRectF(minX, minY, maxX - minX, maxY - minY);
}

SvgExporter::SvgExporter()
{
}

bool SvgExporter::exportPage(const QString &path, const Page *page, bool withBackground)
{
    m_error.clear();
    if (!page) {
        m_error = QStringLiteral("No page");
        return false;
    }

    const QByteArray svg = toSvg(page, withBackground);

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        m_error = f.errorString();
        return false;
    }
    if (f.write(svg) != svg.size()) {
        m_error = f.errorString();
        return false;
    }
    f.close();
    return true;
}

QByteArray SvgExporter::toSvg(const Page *page, bool withBackground)
{
    QByteArray out;
    QXmlStreamWriter xml(&out);
    xml.setAutoFormatting(true);
    xml.setAutoFormattingIndent(2);

    xml.writeStartDocument(QStringLiteral("1.0"));
    xml.writeDefaultNamespace(QLatin1String(SVG_NS));
    xml.writeNamespace(QLatin1String(XLINK_NS), QStringLiteral("xlink"));
    xml.writeNamespace(QLatin1String(INKSCAPE_NS), QStringLiteral("inkscape"));
    xml.writeNamespace(QLatin1String(SODIPODI_NS), QStringLiteral("sodipodi"));
    xml.writeStartElement(QLatin1String(SVG_NS), QStringLiteral("svg"));

    xml.writeAttribute(QStringLiteral("width"), num(page->width) + QStringLiteral("pt"));
    xml.writeAttribute(QStringLiteral("height"), num(page->height) + QStringLiteral("pt"));
    xml.writeAttribute(QStringLiteral("viewBox"),
                       QStringLiteral("0 0 %1 %2").arg(num(page->width), num(page->height)));
    xml.writeAttribute(QStringLiteral("version"), QStringLiteral("1.1"));

    collectGradients(page);
    writeGradientDefs(xml);

    if (withBackground)
        writeBackground(xml, page);

    for (int i = 0; i < page->layers.size(); ++i)
        writeLayer(xml, page->layers.at(i), i);

    xml.writeEndElement();
    xml.writeEndDocument();
    return out;
}

// SVG needs gradients declared before use, so they are gathered in one pass.
static QString gradientKey(const Element *e)
{
    GradientStops stops;
    int angle = 0;
    int alpha = -1;
    QRectF box;

    if (e->type() == Element::StrokeType) {
        const Stroke *s = static_cast<const Stroke *>(e);
        stops = s->gradient;
        angle = s->gradientAngle;
        alpha = s->fill;
        box = s->bounds();
    } else if (e->type() == Element::PathType) {
        const Path *p = static_cast<const Path *>(e);
        stops = p->gradient;
        angle = p->gradientAngle;
        alpha = p->fill;
        box = p->bounds();
    }

    if (!Gradient::isReal(stops) || alpha < 0)
        return QString();

    return QStringLiteral("%1|%2|%3,%4,%5,%6|%7")
            .arg(Gradient::toString(stops)).arg(angle)
            .arg(num(box.x()), num(box.y()), num(box.width()), num(box.height()))
            .arg(alpha);
}

void SvgExporter::collectGradients(const Page *page)
{
    m_gradientIds.clear();
    m_gradientOrder.clear();

    for (int l = 0; l < page->layers.size(); ++l) {
        const Layer *layer = page->layers.at(l);
        for (int i = 0; i < layer->elements.size(); ++i) {
            const QString key = gradientKey(layer->elements.at(i));
            if (key.isEmpty() || m_gradientIds.contains(key))
                continue;
            m_gradientIds.insert(key, QStringLiteral("sjgrad%1").arg(m_gradientIds.size()));
            m_gradientOrder << key;
        }
    }
}

void SvgExporter::writeGradientDefs(QXmlStreamWriter &xml)
{
    if (m_gradientOrder.isEmpty())
        return;

    xml.writeStartElement(QStringLiteral("defs"));
    for (int i = 0; i < m_gradientOrder.size(); ++i) {
        const QString key = m_gradientOrder.at(i);
        const QStringList parts = key.split(QLatin1Char('|'));
        if (parts.size() != 4)
            continue;

        const GradientStops stops = Gradient::fromString(parts.at(0));
        const int angle = parts.at(1).toInt();
        const QStringList boxParts = parts.at(2).split(QLatin1Char(','));
        const int alpha = parts.at(3).toInt();
        if (boxParts.size() != 4)
            continue;

        const QRectF box(boxParts.at(0).toDouble(), boxParts.at(1).toDouble(),
                         boxParts.at(2).toDouble(), boxParts.at(3).toDouble());
        const QLinearGradient g = Gradient::forRect(stops, box, angle, alpha);

        xml.writeStartElement(QStringLiteral("linearGradient"));
        xml.writeAttribute(QStringLiteral("id"), m_gradientIds.value(key));
        xml.writeAttribute(QStringLiteral("gradientUnits"), QStringLiteral("userSpaceOnUse"));
        xml.writeAttribute(QStringLiteral("x1"), num(g.start().x()));
        xml.writeAttribute(QStringLiteral("y1"), num(g.start().y()));
        xml.writeAttribute(QStringLiteral("x2"), num(g.finalStop().x()));
        xml.writeAttribute(QStringLiteral("y2"), num(g.finalStop().y()));

        for (int k = 0; k < stops.size(); ++k) {
            xml.writeStartElement(QStringLiteral("stop"));
            xml.writeAttribute(QStringLiteral("offset"), num(stops.at(k).at));
            xml.writeAttribute(QStringLiteral("stop-color"), rgb(stops.at(k).color));
            if (alpha < 255)
                xml.writeAttribute(QStringLiteral("stop-opacity"), num(alpha / 255.0));
            xml.writeEndElement();
        }
        xml.writeEndElement();
    }
    xml.writeEndElement();
}

QString SvgExporter::fillPaint(const Element *e, const QString &flat) const
{
    const QString key = gradientKey(e);
    if (key.isEmpty() || !m_gradientIds.contains(key))
        return flat;
    return QStringLiteral("url(#%1)").arg(m_gradientIds.value(key));
}

void SvgExporter::writeBackground(QXmlStreamWriter &xml, const Page *page)
{
    const Background &bg = page->background;

    xml.writeStartElement(QStringLiteral("g"));
    xml.writeAttribute(QStringLiteral("id"), QStringLiteral("background"));
    xml.writeAttribute(QLatin1String(INKSCAPE_NS), QStringLiteral("groupmode"), QStringLiteral("layer"));
    xml.writeAttribute(QLatin1String(INKSCAPE_NS), QStringLiteral("label"), QStringLiteral("Background"));

    xml.writeEmptyElement(QStringLiteral("rect"));
    xml.writeAttribute(QStringLiteral("x"), QStringLiteral("0"));
    xml.writeAttribute(QStringLiteral("y"), QStringLiteral("0"));
    xml.writeAttribute(QStringLiteral("width"), num(page->width));
    xml.writeAttribute(QStringLiteral("height"), num(page->height));
    xml.writeAttribute(QStringLiteral("fill"), rgb(bg.color));

    const QString style = bg.style;
    const qreal spacing = 14.17;
    const QString ruleColor = QStringLiteral("#bfcfdf");

    if (style == QLatin1String("lined") || style == QLatin1String("ruled")) {
        for (qreal y = spacing * 5; y < page->height; y += spacing) {
            xml.writeEmptyElement(QStringLiteral("line"));
            xml.writeAttribute(QStringLiteral("x1"), QStringLiteral("0"));
            xml.writeAttribute(QStringLiteral("y1"), num(y));
            xml.writeAttribute(QStringLiteral("x2"), num(page->width));
            xml.writeAttribute(QStringLiteral("y2"), num(y));
            xml.writeAttribute(QStringLiteral("stroke"), ruleColor);
            xml.writeAttribute(QStringLiteral("stroke-width"), QStringLiteral("0.5"));
        }
        if (style == QLatin1String("ruled")) {
            xml.writeEmptyElement(QStringLiteral("line"));
            xml.writeAttribute(QStringLiteral("x1"), num(spacing * 5));
            xml.writeAttribute(QStringLiteral("y1"), QStringLiteral("0"));
            xml.writeAttribute(QStringLiteral("x2"), num(spacing * 5));
            xml.writeAttribute(QStringLiteral("y2"), num(page->height));
            xml.writeAttribute(QStringLiteral("stroke"), QStringLiteral("#ff8080"));
            xml.writeAttribute(QStringLiteral("stroke-width"), QStringLiteral("0.5"));
        }
    } else if (style == QLatin1String("graph")) {
        for (qreal x = spacing; x < page->width; x += spacing) {
            xml.writeEmptyElement(QStringLiteral("line"));
            xml.writeAttribute(QStringLiteral("x1"), num(x));
            xml.writeAttribute(QStringLiteral("y1"), QStringLiteral("0"));
            xml.writeAttribute(QStringLiteral("x2"), num(x));
            xml.writeAttribute(QStringLiteral("y2"), num(page->height));
            xml.writeAttribute(QStringLiteral("stroke"), ruleColor);
            xml.writeAttribute(QStringLiteral("stroke-width"), QStringLiteral("0.5"));
        }
        for (qreal y = spacing; y < page->height; y += spacing) {
            xml.writeEmptyElement(QStringLiteral("line"));
            xml.writeAttribute(QStringLiteral("x1"), QStringLiteral("0"));
            xml.writeAttribute(QStringLiteral("y1"), num(y));
            xml.writeAttribute(QStringLiteral("x2"), num(page->width));
            xml.writeAttribute(QStringLiteral("y2"), num(y));
            xml.writeAttribute(QStringLiteral("stroke"), ruleColor);
            xml.writeAttribute(QStringLiteral("stroke-width"), QStringLiteral("0.5"));
        }
    } else if (style == QLatin1String("dotted")) {
        for (qreal x = spacing; x < page->width; x += spacing) {
            for (qreal y = spacing; y < page->height; y += spacing) {
                xml.writeEmptyElement(QStringLiteral("circle"));
                xml.writeAttribute(QStringLiteral("cx"), num(x));
                xml.writeAttribute(QStringLiteral("cy"), num(y));
                xml.writeAttribute(QStringLiteral("r"), QStringLiteral("0.6"));
                xml.writeAttribute(QStringLiteral("fill"), ruleColor);
            }
        }
    }

    xml.writeEndElement();
}

void SvgExporter::writeLayer(QXmlStreamWriter &xml, const Layer *layer, int index)
{
    xml.writeStartElement(QStringLiteral("g"));
    xml.writeAttribute(QStringLiteral("id"), QStringLiteral("layer%1").arg(index + 1));
    xml.writeAttribute(QLatin1String(INKSCAPE_NS), QStringLiteral("groupmode"), QStringLiteral("layer"));
    xml.writeAttribute(QLatin1String(INKSCAPE_NS), QStringLiteral("label"),
                       layer->name.isEmpty() ? QStringLiteral("Layer %1").arg(index + 1) : layer->name);
    if (!layer->visible)
        xml.writeAttribute(QStringLiteral("style"), QStringLiteral("display:none"));
    if (layer->locked)
        xml.writeAttribute(QLatin1String(SODIPODI_NS), QStringLiteral("insensitive"),
                           QStringLiteral("true"));
    if (layer->opacity < 1.0)
        xml.writeAttribute(QStringLiteral("opacity"), num(layer->opacity));

    int openGroup = 0;
    for (int i = 0; i < layer->elements.size(); ++i) {
        const Element *e = layer->elements.at(i);

        if (e->group != openGroup) {
            if (openGroup != 0)
                xml.writeEndElement();
            openGroup = e->group;
            if (openGroup != 0) {
                xml.writeStartElement(QStringLiteral("g"));
                xml.writeAttribute(QStringLiteral("id"),
                                   QStringLiteral("group%1").arg(openGroup));
            }
        }

        switch (e->type()) {
        case Element::StrokeType: {
            const Stroke *s = static_cast<const Stroke *>(e);
            if (s->shape == Stroke::FreeShape)
                writeStroke(xml, s);
            else
                writeShapeStroke(xml, s);
            break;
        }
        case Element::PathType:
            writePath(xml, static_cast<const Path *>(e));
            break;
        case Element::TextType:
            writeText(xml, static_cast<const TextItem *>(e));
            break;
        case Element::ImageType:
            writeImage(xml, static_cast<const ImageItem *>(e));
            break;
        }
    }

    xml.writeEndElement();
}

void SvgExporter::writePath(QXmlStreamWriter &xml, const Path *p)
{
    if (p->segments.isEmpty())
        return;

    QString d = StrokeGeometry::toSvgPathData(p->painterPath());
    if (p->closed && p->extra.isEmpty())
        d += QStringLiteral(" Z");

    xml.writeEmptyElement(QStringLiteral("path"));
    xml.writeAttribute(QStringLiteral("d"), d);
    // Qt fills odd-even, SVG defaults to nonzero; the counters would close up.
    if (!p->extra.isEmpty())
        xml.writeAttribute(QStringLiteral("fill-rule"), QStringLiteral("evenodd"));

    if (p->fill >= 0) {
        const QColor fill = p->fillColor.isValid() ? p->fillColor : p->color;
        xml.writeAttribute(QStringLiteral("fill"), fillPaint(p, rgb(fill)));
        if (p->fill < 255 && !Gradient::isReal(p->gradient))
            xml.writeAttribute(QStringLiteral("fill-opacity"), num(p->fill / 255.0));
    } else {
        xml.writeAttribute(QStringLiteral("fill"), QStringLiteral("none"));
    }

    xml.writeAttribute(QStringLiteral("stroke"), rgb(p->color));
    xml.writeAttribute(QStringLiteral("stroke-width"), num(p->width));
    xml.writeAttribute(QStringLiteral("stroke-linecap"), QStringLiteral("round"));
    xml.writeAttribute(QStringLiteral("stroke-linejoin"), QStringLiteral("round"));
    if (p->color.alphaF() < 1.0)
        xml.writeAttribute(QStringLiteral("stroke-opacity"), num(p->color.alphaF()));
}

void SvgExporter::writeStroke(QXmlStreamWriter &xml, const Stroke *s)
{
    if (s->points.isEmpty() || s->tool == Stroke::Eraser)
        return;

    const bool highlighter = s->tool == Stroke::Highlighter;
    const qreal alpha = highlighter ? 0.5 : s->color.alphaF();

    if (StrokeGeometry::isUniformWidth(*s)) {
        const QPainterPath path = StrokeGeometry::centerline(*s);
        xml.writeEmptyElement(QStringLiteral("path"));
        xml.writeAttribute(QStringLiteral("d"), StrokeGeometry::toSvgPathData(path));
        xml.writeAttribute(QStringLiteral("fill"), QStringLiteral("none"));
        xml.writeAttribute(QStringLiteral("stroke"), rgb(s->color));
        xml.writeAttribute(QStringLiteral("stroke-width"), num(s->width));
        xml.writeAttribute(QStringLiteral("stroke-linecap"),
                           s->cap == Stroke::ButtCap ? QStringLiteral("butt")
                         : s->cap == Stroke::SquareCap ? QStringLiteral("square")
                                                       : QStringLiteral("round"));
        xml.writeAttribute(QStringLiteral("stroke-linejoin"), QStringLiteral("round"));
        if (alpha < 1.0)
            xml.writeAttribute(QStringLiteral("stroke-opacity"), num(alpha));
        if (highlighter)
            xml.writeAttribute(QStringLiteral("style"), QStringLiteral("mix-blend-mode:multiply"));
    } else {
        const QPainterPath path = StrokeGeometry::outline(*s);
        xml.writeEmptyElement(QStringLiteral("path"));
        xml.writeAttribute(QStringLiteral("d"), StrokeGeometry::toSvgPathData(path));
        xml.writeAttribute(QStringLiteral("fill"), rgb(s->color));
        xml.writeAttribute(QStringLiteral("fill-rule"), QStringLiteral("nonzero"));
        xml.writeAttribute(QStringLiteral("stroke"), QStringLiteral("none"));
        if (alpha < 1.0)
            xml.writeAttribute(QStringLiteral("fill-opacity"), num(alpha));
        if (highlighter)
            xml.writeAttribute(QStringLiteral("style"), QStringLiteral("mix-blend-mode:multiply"));
    }
}

void SvgExporter::writeShapeStroke(QXmlStreamWriter &xml, const Stroke *s)
{
    const QRectF r = pointBounds(s);
    const QString stroke = rgb(s->color);
    const QColor fillSource = s->fillColor.isValid() ? s->fillColor : s->color;
    const bool gradientFill = Gradient::isReal(s->gradient) && s->fill >= 0;
    const QString fill = s->fill >= 0 ? fillPaint(s, rgb(fillSource))
                                      : QStringLiteral("none");
    const qreal fillOpacity = s->fill >= 0 && !gradientFill ? s->fill / 255.0 : 1.0;

    switch (s->shape) {
    case Stroke::LineShape: {
        if (s->points.size() < 2)
            return;
        xml.writeEmptyElement(QStringLiteral("line"));
        xml.writeAttribute(QStringLiteral("x1"), num(s->points.first().x));
        xml.writeAttribute(QStringLiteral("y1"), num(s->points.first().y));
        xml.writeAttribute(QStringLiteral("x2"), num(s->points.last().x));
        xml.writeAttribute(QStringLiteral("y2"), num(s->points.last().y));
        break;
    }
    case Stroke::PolylineShape: {
        if (s->points.size() < 2)
            return;
        QString coords;
        for (int i = 0; i < s->points.size(); ++i) {
            if (i)
                coords += QLatin1Char(' ');
            coords += num(s->points.at(i).x) + QLatin1Char(',') + num(s->points.at(i).y);
        }
        xml.writeEmptyElement(QStringLiteral("polyline"));
        xml.writeAttribute(QStringLiteral("points"), coords);
        xml.writeAttribute(QStringLiteral("fill"), fill);
        break;
    }
    case Stroke::RectShape: {
        if (s->points.size() < 4) {
            xml.writeEmptyElement(QStringLiteral("rect"));
            xml.writeAttribute(QStringLiteral("x"), num(r.x()));
            xml.writeAttribute(QStringLiteral("y"), num(r.y()));
            xml.writeAttribute(QStringLiteral("width"), num(r.width()));
            xml.writeAttribute(QStringLiteral("height"), num(r.height()));
            xml.writeAttribute(QStringLiteral("fill"), fill);
            break;
        }

        const QPointF c0(s->points.at(0).x, s->points.at(0).y);
        const QPointF c1(s->points.at(1).x, s->points.at(1).y);
        const QPointF c2(s->points.at(2).x, s->points.at(2).y);
        const QPointF c3(s->points.at(3).x, s->points.at(3).y);

        const qreal w = QLineF(c0, c1).length();
        const qreal h = QLineF(c1, c2).length();
        const QPointF center((c0.x() + c2.x()) / 2, (c0.y() + c2.y()) / 2);
        const qreal angle = qRadiansToDegrees(qAtan2(c1.y() - c0.y(), c1.x() - c0.x()));

        xml.writeEmptyElement(QStringLiteral("rect"));
        xml.writeAttribute(QStringLiteral("x"), num(center.x() - w / 2));
        xml.writeAttribute(QStringLiteral("y"), num(center.y() - h / 2));
        xml.writeAttribute(QStringLiteral("width"), num(w));
        xml.writeAttribute(QStringLiteral("height"), num(h));
        if (qAbs(angle) > 0.05) {
            xml.writeAttribute(QStringLiteral("transform"),
                               QStringLiteral("rotate(%1 %2 %3)")
                               .arg(num(angle), num(center.x()), num(center.y())));
        }
        xml.writeAttribute(QStringLiteral("fill"), fill);
        break;
    }
    case Stroke::EllipseShape: {
        const qreal rx = r.width() / 2;
        const qreal ry = r.height() / 2;
        if (qAbs(rx - ry) < qMax(rx, ry) * 0.02) {
            xml.writeEmptyElement(QStringLiteral("circle"));
            xml.writeAttribute(QStringLiteral("cx"), num(r.center().x()));
            xml.writeAttribute(QStringLiteral("cy"), num(r.center().y()));
            xml.writeAttribute(QStringLiteral("r"), num((rx + ry) / 2));
        } else {
            xml.writeEmptyElement(QStringLiteral("ellipse"));
            xml.writeAttribute(QStringLiteral("cx"), num(r.center().x()));
            xml.writeAttribute(QStringLiteral("cy"), num(r.center().y()));
            xml.writeAttribute(QStringLiteral("rx"), num(rx));
            xml.writeAttribute(QStringLiteral("ry"), num(ry));
        }
        xml.writeAttribute(QStringLiteral("fill"), fill);
        break;
    }
    case Stroke::ArrowShape:
    case Stroke::PolygonShape:
    case Stroke::StarShape:
    default: {
        QString pts;
        for (int i = 0; i < s->points.size(); ++i) {
            if (i)
                pts += QLatin1Char(' ');
            pts += num(s->points.at(i).x) + QLatin1Char(',') + num(s->points.at(i).y);
        }
        const bool closed = s->shape == Stroke::PolygonShape || s->shape == Stroke::StarShape;
        xml.writeEmptyElement(closed ? QStringLiteral("polygon")
                                     : QStringLiteral("polyline"));
        xml.writeAttribute(QStringLiteral("points"), pts);
        xml.writeAttribute(QStringLiteral("fill"), closed ? fill : QStringLiteral("none"));
        break;
    }
    }

    if (s->fill >= 0 && fillOpacity < 1.0)
        xml.writeAttribute(QStringLiteral("fill-opacity"), num(fillOpacity));
    xml.writeAttribute(QStringLiteral("stroke"), stroke);
    xml.writeAttribute(QStringLiteral("stroke-width"), num(s->width));
    xml.writeAttribute(QStringLiteral("stroke-linecap"), QStringLiteral("round"));
    xml.writeAttribute(QStringLiteral("stroke-linejoin"), QStringLiteral("round"));
    if (!s->lineStyle.isEmpty() && s->lineStyle != QLatin1String("plain")) {
        const qreal w = qMax(s->width, qreal(0.2));
        QString dashes;
        if (s->lineStyle == QLatin1String("dash"))
            dashes = QStringLiteral("%1,%2").arg(num(w * 3), num(w * 3));
        else if (s->lineStyle == QLatin1String("dot"))
            dashes = QStringLiteral("%1,%2").arg(num(w * 0.5), num(w * 2.5));
        else if (s->lineStyle == QLatin1String("dashdot"))
            dashes = QStringLiteral("%1,%2,%3,%4")
                    .arg(num(w * 3), num(w * 2), num(w * 0.5), num(w * 2));
        if (!dashes.isEmpty())
            xml.writeAttribute(QStringLiteral("stroke-dasharray"), dashes);
    }
    if (s->color.alphaF() < 1.0)
        xml.writeAttribute(QStringLiteral("stroke-opacity"), num(s->color.alphaF()));
}

// .xopp carries Pango's family names; SVG needs the CSS spelling.
static QString cssFamily(const QString &name)
{
    const QString lower = name.trimmed().toLower();
    if (lower.isEmpty() || lower == QLatin1String("sans") || lower == QLatin1String("sans serif"))
        return QStringLiteral("sans-serif");
    if (lower == QLatin1String("serif"))
        return QStringLiteral("serif");
    if (lower == QLatin1String("mono") || lower == QLatin1String("monospace"))
        return QStringLiteral("monospace");
    return QStringLiteral("'%1', sans-serif").arg(name);
}

void SvgExporter::writeText(QXmlStreamWriter &xml, const TextItem *t)
{
    const QStringList lines = t->text.split(QLatin1Char('\n'));

    // Baselines come from the metrics ElementPainter draws with, not a 1.2 em guess.
    const QFontMetricsF fm(t->font());

    xml.writeStartElement(QStringLiteral("text"));
    xml.writeAttribute(QStringLiteral("x"), num(t->pos.x()));
    xml.writeAttribute(QStringLiteral("y"), num(t->pos.y() + fm.ascent()));
    xml.writeAttribute(QStringLiteral("font-family"), cssFamily(t->fontName));
    xml.writeAttribute(QStringLiteral("font-size"), num(t->fontSize));
    if (t->bold)
        xml.writeAttribute(QStringLiteral("font-weight"), QStringLiteral("bold"));
    if (t->italic)
        xml.writeAttribute(QStringLiteral("font-style"), QStringLiteral("italic"));
    if (t->underline)
        xml.writeAttribute(QStringLiteral("text-decoration"), QStringLiteral("underline"));
    if (t->style == TextItem::Outline) {
        xml.writeAttribute(QStringLiteral("fill"), QStringLiteral("none"));
        xml.writeAttribute(QStringLiteral("stroke"), rgb(t->color));
        xml.writeAttribute(QStringLiteral("stroke-width"), num(qMax(qreal(0.4), qreal(t->fontSize * 0.055))));
    } else if (t->style == TextItem::Inline) {
        xml.writeAttribute(QStringLiteral("stroke"), QStringLiteral("#ffffff"));
        xml.writeAttribute(QStringLiteral("stroke-width"), num(qMax(qreal(0.4), qreal(t->fontSize * 0.055))));
        xml.writeAttribute(QStringLiteral("paint-order"), QStringLiteral("fill stroke"));
    }
    xml.writeAttribute(QStringLiteral("fill"), rgb(t->color));

    for (int i = 0; i < lines.size(); ++i) {
        xml.writeStartElement(QStringLiteral("tspan"));
        xml.writeAttribute(QStringLiteral("x"), num(t->pos.x()));
        xml.writeAttribute(QStringLiteral("y"),
                           num(t->pos.y() + fm.ascent() + i * fm.height()));
        xml.writeCharacters(lines.at(i));
        xml.writeEndElement();
    }

    xml.writeEndElement();
}

void SvgExporter::writeImage(QXmlStreamWriter &xml, const ImageItem *i)
{
    if (i->image.isNull())
        return;

    QByteArray png;
    QBuffer buf(&png);
    buf.open(QIODevice::WriteOnly);
    if (!i->image.save(&buf, "PNG"))
        return;

    xml.writeEmptyElement(QStringLiteral("image"));
    xml.writeAttribute(QStringLiteral("x"), num(i->rect.x()));
    xml.writeAttribute(QStringLiteral("y"), num(i->rect.y()));
    xml.writeAttribute(QStringLiteral("width"), num(i->rect.width()));
    xml.writeAttribute(QStringLiteral("height"), num(i->rect.height()));
    xml.writeAttribute(QStringLiteral("preserveAspectRatio"), QStringLiteral("none"));
    xml.writeAttribute(QLatin1String(XLINK_NS), QStringLiteral("href"),
                       QStringLiteral("data:image/png;base64,") + QString::fromLatin1(png.toBase64()));
}
}
