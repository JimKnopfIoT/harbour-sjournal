#include "io/xoppwriter.h"

#include "io/gzfile.h"
#include "model/document.h"
#include "model/imageitem.h"
#include "model/path.h"
#include "model/stroke.h"
#include "model/textitem.h"

#include <QBuffer>
#include <QStringList>
#include <QXmlStreamWriter>

namespace xn {
static QString num(qreal v)
{
    return QString::number(v, 'g', 8);
}

static QString colorString(const QColor &c, int forcedAlpha = -1)
{
    const int a = forcedAlpha >= 0 ? forcedAlpha : c.alpha();
    return QString(QStringLiteral("#%1%2%3%4"))
            .arg(c.red(), 2, 16, QLatin1Char('0'))
            .arg(c.green(), 2, 16, QLatin1Char('0'))
            .arg(c.blue(), 2, 16, QLatin1Char('0'))
            .arg(a, 2, 16, QLatin1Char('0'));
}

static QString shapeString(Stroke::Shape s)
{
    switch (s) {
    case Stroke::LineShape: return QStringLiteral("line");
    case Stroke::RectShape: return QStringLiteral("rect");
    case Stroke::EllipseShape: return QStringLiteral("ellipse");
    case Stroke::ArrowShape: return QStringLiteral("arrow");
    case Stroke::PolygonShape: return QStringLiteral("polygon");
    case Stroke::StarShape: return QStringLiteral("star");
    case Stroke::PolylineShape: return QStringLiteral("polyline");
    case Stroke::FreeShape: break;
    }
    return QString();
}

XoppWriter::XoppWriter()
{
}

bool XoppWriter::write(const QString &path, const Document *doc, const QImage &preview)
{
    m_error.clear();
    if (!doc) {
        m_error = QStringLiteral("No document");
        return false;
    }
    return gz::writeAll(path, toXml(doc, preview), &m_error);
}

QByteArray XoppWriter::toXml(const Document *doc, const QImage &preview)
{
    QByteArray out;
    QXmlStreamWriter xml(&out);
    xml.setAutoFormatting(true);
    xml.setAutoFormattingIndent(2);

    xml.writeStartDocument(QStringLiteral("1.0"));
    xml.writeStartElement(QStringLiteral("xournal"));
    xml.writeAttribute(QStringLiteral("creator"), QStringLiteral("SJournal 0.1.0"));
    xml.writeAttribute(QStringLiteral("fileversion"), QStringLiteral("4"));

    xml.writeTextElement(QStringLiteral("title"),
                         doc->title.isEmpty() ? QStringLiteral("SJournal note") : doc->title);

    if (!preview.isNull()) {
        QByteArray png;
        QBuffer buf(&png);
        buf.open(QIODevice::WriteOnly);
        if (preview.save(&buf, "PNG"))
            xml.writeTextElement(QStringLiteral("preview"), QString::fromLatin1(png.toBase64()));
    }

    for (int i = 0; i < doc->pages.size(); ++i)
        writePage(xml, doc->pages.at(i));

    xml.writeEndElement();
    xml.writeEndDocument();
    return out;
}

void XoppWriter::writePage(QXmlStreamWriter &xml, const Page *page)
{
    xml.writeStartElement(QStringLiteral("page"));
    xml.writeAttribute(QStringLiteral("width"), num(page->width));
    xml.writeAttribute(QStringLiteral("height"), num(page->height));

    const Background &bg = page->background;
    xml.writeStartElement(QStringLiteral("background"));
    switch (bg.type) {
    case Background::Pdf:
        xml.writeAttribute(QStringLiteral("type"), QStringLiteral("pdf"));
        if (!bg.domain.isEmpty())
            xml.writeAttribute(QStringLiteral("domain"), bg.domain);
        if (!bg.filename.isEmpty())
            xml.writeAttribute(QStringLiteral("filename"), bg.filename);
        xml.writeAttribute(QStringLiteral("pageno"), QString::number(bg.pageNo + 1));
        break;
    case Background::Pixmap:
        xml.writeAttribute(QStringLiteral("type"), QStringLiteral("pixmap"));
        if (!bg.domain.isEmpty())
            xml.writeAttribute(QStringLiteral("domain"), bg.domain);
        if (!bg.filename.isEmpty())
            xml.writeAttribute(QStringLiteral("filename"), bg.filename);
        break;
    case Background::Solid:
        xml.writeAttribute(QStringLiteral("type"), QStringLiteral("solid"));
        xml.writeAttribute(QStringLiteral("color"), colorString(bg.color));
        xml.writeAttribute(QStringLiteral("style"),
                           bg.style.isEmpty() ? QStringLiteral("plain") : bg.style);
        if (!bg.config.isEmpty())
            xml.writeAttribute(QStringLiteral("config"), bg.config);
        break;
    }
    xml.writeEndElement();

    for (int i = 0; i < page->layers.size(); ++i)
        writeLayer(xml, page->layers.at(i));

    xml.writeEndElement();
}

void XoppWriter::writeLayer(QXmlStreamWriter &xml, const Layer *layer)
{
    xml.writeStartElement(QStringLiteral("layer"));
    if (!layer->name.isEmpty())
        xml.writeAttribute(QStringLiteral("name"), layer->name);
    if (!layer->visible)
        xml.writeAttribute(QStringLiteral("visible"), QStringLiteral("false"));
    if (layer->locked)
        xml.writeAttribute(QStringLiteral("locked"), QStringLiteral("true"));
    if (layer->opacity < 1.0)
        xml.writeAttribute(QStringLiteral("opacity"), num(layer->opacity));

    for (int i = 0; i < layer->elements.size(); ++i) {
        const Element *e = layer->elements.at(i);
        switch (e->type()) {
        case Element::StrokeType:
            writeStroke(xml, static_cast<const Stroke *>(e));
            break;
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

void XoppWriter::writeStroke(QXmlStreamWriter &xml, const Stroke *s)
{
    if (s->points.isEmpty())
        return;

    xml.writeStartElement(QStringLiteral("stroke"));

    switch (s->tool) {
    case Stroke::Highlighter: xml.writeAttribute(QStringLiteral("tool"), QStringLiteral("highlighter")); break;
    case Stroke::Eraser:      xml.writeAttribute(QStringLiteral("tool"), QStringLiteral("eraser")); break;
    case Stroke::Pen:         xml.writeAttribute(QStringLiteral("tool"), QStringLiteral("pen")); break;
    }

    xml.writeAttribute(QStringLiteral("color"),
                       colorString(s->color, s->tool == Stroke::Highlighter ? 0x7f : -1));

    if (s->hasPressure()) {
        QString widths = num(s->width);
        for (int i = 0; i < s->points.size() - 1; ++i)
            widths += QLatin1Char(' ') + num(s->widthAt(i));
        xml.writeAttribute(QStringLiteral("width"), widths);
    } else {
        xml.writeAttribute(QStringLiteral("width"), num(s->width));
    }

    if (s->fill >= 0) {
        xml.writeAttribute(QStringLiteral("fill"), QString::number(s->fill));
        if (s->fillColor.isValid())
            xml.writeAttribute(QStringLiteral("fillColor"), colorString(s->fillColor));
        if (Gradient::isReal(s->gradient)) {
            xml.writeAttribute(QStringLiteral("gradient"), Gradient::toString(s->gradient));
            xml.writeAttribute(QStringLiteral("gradientAngle"),
                               QString::number(s->gradientAngle));
        }
    }

    switch (s->cap) {
    case Stroke::ButtCap:   xml.writeAttribute(QStringLiteral("capStyle"), QStringLiteral("butt")); break;
    case Stroke::SquareCap: xml.writeAttribute(QStringLiteral("capStyle"), QStringLiteral("square")); break;
    case Stroke::RoundCap:  xml.writeAttribute(QStringLiteral("capStyle"), QStringLiteral("round")); break;
    }

    if (!s->lineStyle.isEmpty() && s->lineStyle != QLatin1String("plain"))
        xml.writeAttribute(QStringLiteral("style"), s->lineStyle);

    const QString shape = shapeString(s->shape);
    if (!shape.isEmpty())
        xml.writeAttribute(QStringLiteral("shape"), shape);
    if (s->group != 0)
        xml.writeAttribute(QStringLiteral("group"), QString::number(s->group));

    QString coords;
    coords.reserve(s->points.size() * 16);
    for (int i = 0; i < s->points.size(); ++i) {
        if (i)
            coords += QLatin1Char(' ');
        coords += num(s->points.at(i).x);
        coords += QLatin1Char(' ');
        coords += num(s->points.at(i).y);
    }
    xml.writeCharacters(coords);

    xml.writeEndElement();
}

void XoppWriter::writePath(QXmlStreamWriter &xml, const Path *p)
{
    if (p->segments.isEmpty())
        return;

    xml.writeStartElement(QStringLiteral("stroke"));
    xml.writeAttribute(QStringLiteral("tool"), QStringLiteral("pen"));
    xml.writeAttribute(QStringLiteral("color"), colorString(p->color));
    xml.writeAttribute(QStringLiteral("width"), num(p->width));
    xml.writeAttribute(QStringLiteral("capStyle"), QStringLiteral("round"));

    if (p->fill >= 0) {
        xml.writeAttribute(QStringLiteral("fill"), QString::number(p->fill));
        if (p->fillColor.isValid())
            xml.writeAttribute(QStringLiteral("fillColor"), colorString(p->fillColor));
        if (Gradient::isReal(p->gradient)) {
            xml.writeAttribute(QStringLiteral("gradient"), Gradient::toString(p->gradient));
            xml.writeAttribute(QStringLiteral("gradientAngle"),
                               QString::number(p->gradientAngle));
        }
    }
    if (!p->lineStyle.isEmpty() && p->lineStyle != QLatin1String("plain"))
        xml.writeAttribute(QStringLiteral("style"), p->lineStyle);
    if (p->group != 0)
        xml.writeAttribute(QStringLiteral("group"), QString::number(p->group));

    QString curve = num(p->start.x()) + QLatin1Char(' ') + num(p->start.y());
    for (int i = 0; i < p->segments.size(); ++i) {
        const CubicSegment &s = p->segments.at(i);
        const QPointF pts[3] = { s.c1, s.c2, s.to };
        for (int k = 0; k < 3; ++k) {
            curve += QLatin1Char(' ') + num(pts[k].x());
            curve += QLatin1Char(' ') + num(pts[k].y());
        }
    }
    xml.writeAttribute(QStringLiteral("curve"), curve);
    if (p->closed)
        xml.writeAttribute(QStringLiteral("curveClosed"), QStringLiteral("true"));

    // Inner contours ride along in their own attribute, separated by ';'.
    if (!p->extra.isEmpty()) {
        QStringList holes;
        for (int k = 0; k < p->extra.size(); ++k) {
            const SubPath &sub = p->extra.at(k);
            QString one = num(sub.start.x()) + QLatin1Char(' ') + num(sub.start.y());
            for (int i = 0; i < sub.segments.size(); ++i) {
                const CubicSegment &s = sub.segments.at(i);
                const QPointF pts[3] = { s.c1, s.c2, s.to };
                for (int m = 0; m < 3; ++m) {
                    one += QLatin1Char(' ') + num(pts[m].x());
                    one += QLatin1Char(' ') + num(pts[m].y());
                }
            }
            holes << one;
        }
        xml.writeAttribute(QStringLiteral("curveHoles"), holes.join(QLatin1Char(';')));
    }

    const QVector<QPointF> flat = p->flattened();
    QString coords;
    coords.reserve(flat.size() * 16);
    for (int i = 0; i < flat.size(); ++i) {
        if (i)
            coords += QLatin1Char(' ');
        coords += num(flat.at(i).x());
        coords += QLatin1Char(' ');
        coords += num(flat.at(i).y());
    }
    xml.writeCharacters(coords);

    xml.writeEndElement();
}

void XoppWriter::writeText(QXmlStreamWriter &xml, const TextItem *t)
{
    xml.writeStartElement(QStringLiteral("text"));
    xml.writeAttribute(QStringLiteral("font"), t->fontName);
    xml.writeAttribute(QStringLiteral("size"), num(t->fontSize));
    xml.writeAttribute(QStringLiteral("x"), num(t->pos.x()));
    xml.writeAttribute(QStringLiteral("y"), num(t->pos.y()));
    xml.writeAttribute(QStringLiteral("color"), colorString(t->color));
    if (t->bold)
        xml.writeAttribute(QStringLiteral("bold"), QStringLiteral("true"));
    if (t->italic)
        xml.writeAttribute(QStringLiteral("italic"), QStringLiteral("true"));
    if (t->underline)
        xml.writeAttribute(QStringLiteral("underline"), QStringLiteral("true"));
    if (t->style == TextItem::Outline)
        xml.writeAttribute(QStringLiteral("textStyle"), QStringLiteral("outline"));
    else if (t->style == TextItem::Inline)
        xml.writeAttribute(QStringLiteral("textStyle"), QStringLiteral("inline"));
    if (t->group != 0)
        xml.writeAttribute(QStringLiteral("group"), QString::number(t->group));
    xml.writeCharacters(t->text);
    xml.writeEndElement();
}

void XoppWriter::writeImage(QXmlStreamWriter &xml, const ImageItem *i)
{
    if (i->image.isNull())
        return;

    QByteArray png;
    QBuffer buf(&png);
    buf.open(QIODevice::WriteOnly);
    if (!i->image.save(&buf, "PNG"))
        return;

    xml.writeStartElement(QStringLiteral("image"));
    xml.writeAttribute(QStringLiteral("left"), num(i->rect.left()));
    xml.writeAttribute(QStringLiteral("top"), num(i->rect.top()));
    xml.writeAttribute(QStringLiteral("right"), num(i->rect.right()));
    xml.writeAttribute(QStringLiteral("bottom"), num(i->rect.bottom()));
    if (i->group != 0)
        xml.writeAttribute(QStringLiteral("group"), QString::number(i->group));
    xml.writeCharacters(QString::fromLatin1(png.toBase64()));
    xml.writeEndElement();
}
}
