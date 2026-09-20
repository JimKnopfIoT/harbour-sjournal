#include "io/xoppreader.h"

#include "io/gzfile.h"
#include "model/document.h"
#include "model/imageitem.h"
#include "model/path.h"
#include "model/stroke.h"
#include "model/textitem.h"

#include <QColor>
#include <QFileInfo>
#include <QStringList>
#include <QXmlStreamReader>

namespace xn {
static QColor parseColor(const QString &s, const QColor &fallback = Qt::black)
{
    if (s.isEmpty())
        return fallback;

    if (s.startsWith(QLatin1Char('#')) && s.length() == 9) {
        bool ok = false;
        const uint rgba = s.mid(1).toUInt(&ok, 16);
        if (ok) {
            return QColor(int((rgba >> 24) & 0xff), int((rgba >> 16) & 0xff),
                          int((rgba >> 8) & 0xff), int(rgba & 0xff));
        }
    }

    const QColor named(s);
    return named.isValid() ? named : fallback;
}

static QStringList splitNumbers(const QString &s)
{
    return s.split(QRegExp(QStringLiteral("\\s+")), QString::SkipEmptyParts);
}

static qreal attrReal(const QXmlStreamAttributes &a, const QString &name, qreal fallback)
{
    if (!a.hasAttribute(name))
        return fallback;
    bool ok = false;
    const qreal v = a.value(name).toString().toDouble(&ok);
    return ok ? v : fallback;
}

XoppReader::XoppReader()
{
}

bool XoppReader::read(const QString &path, Document *doc)
{
    if (!doc)
        return false;

    m_error.clear();
    const QByteArray data = gz::readAll(path, &m_error);
    if (data.isEmpty()) {
        if (m_error.isEmpty())
            m_error = QStringLiteral("Empty file");
        return false;
    }

    doc->clear();
    doc->filePath = path;

    QXmlStreamReader xml(data);
    while (!xml.atEnd()) {
        if (xml.readNext() == QXmlStreamReader::StartElement) {
            const QStringRef name = xml.name();
            if (name == QLatin1String("xournal") || name == QLatin1String("MrWriter")) {
                readXournal(xml, doc);
                break;
            }
        }
    }

    if (xml.hasError()) {
        m_error = xml.errorString();
        return false;
    }

    if (doc->pages.isEmpty()) {
        m_error = QStringLiteral("No pages in file");
        return false;
    }

    if (doc->title.isEmpty())
        doc->title = QFileInfo(path).completeBaseName();

    return true;
}

bool XoppReader::readSummary(const QString &path, QString *title, int *pageCount, QString *text)
{
    const QByteArray data = gz::readAll(path);
    if (data.isEmpty())
        return false;

    QXmlStreamReader xml(data);
    int pages = 0;
    QString t;
    QString body;
    while (!xml.atEnd()) {
        if (xml.readNext() != QXmlStreamReader::StartElement)
            continue;
        if (xml.name() == QLatin1String("title")) {
            t = xml.readElementText().trimmed();
        } else if (xml.name() == QLatin1String("page")) {
            ++pages;
        } else if (text && xml.name() == QLatin1String("text")) {
            body += xml.readElementText();
            body += QLatin1Char('\n');
        } else if (xml.name() == QLatin1String("preview")
                   || xml.name() == QLatin1String("image")
                   || xml.name() == QLatin1String("stroke")) {
            xml.skipCurrentElement();
        }
    }
    if (xml.hasError())
        return false;

    if (title)
        *title = t.isEmpty() ? QFileInfo(path).completeBaseName() : t;
    if (pageCount)
        *pageCount = pages;
    if (text)
        *text = body;
    return true;
}

void XoppReader::readXournal(QXmlStreamReader &xml, Document *doc)
{
    while (!xml.atEnd()) {
        const QXmlStreamReader::TokenType t = xml.readNext();
        if (t == QXmlStreamReader::EndElement)
            return;
        if (t != QXmlStreamReader::StartElement)
            continue;

        if (xml.name() == QLatin1String("title")) {
            doc->title = xml.readElementText().trimmed();
        } else if (xml.name() == QLatin1String("page")) {
            const QXmlStreamAttributes a = xml.attributes();
            Page *page = new Page(QSizeF(attrReal(a, QStringLiteral("width"), Page::a4().width()),
                                         attrReal(a, QStringLiteral("height"), Page::a4().height())));
            qDeleteAll(page->layers);
            page->layers.clear();
            readPage(xml, page);
            if (page->layers.isEmpty())
                page->addLayer();
            doc->pages.append(page);
        } else {
            xml.skipCurrentElement();
        }
    }
}

void XoppReader::readPage(QXmlStreamReader &xml, Page *page)
{
    while (!xml.atEnd()) {
        const QXmlStreamReader::TokenType t = xml.readNext();
        if (t == QXmlStreamReader::EndElement)
            return;
        if (t != QXmlStreamReader::StartElement)
            continue;

        if (xml.name() == QLatin1String("background")) {
            const QXmlStreamAttributes a = xml.attributes();
            const QString type = a.value(QStringLiteral("type")).toString();
            if (type == QLatin1String("pdf"))
                page->background.type = Background::Pdf;
            else if (type == QLatin1String("pixmap"))
                page->background.type = Background::Pixmap;
            else
                page->background.type = Background::Solid;

            page->background.color = parseColor(a.value(QStringLiteral("color")).toString(), Qt::white);
            page->background.style = a.value(QStringLiteral("style")).toString();
            if (page->background.style.isEmpty())
                page->background.style = QStringLiteral("plain");
            page->background.filename = a.value(QStringLiteral("filename")).toString();
            page->background.domain = a.value(QStringLiteral("domain")).toString();
            page->background.config = a.value(QStringLiteral("config")).toString();
            page->background.pageNo = int(attrReal(a, QStringLiteral("pageno"), 1)) - 1;
            xml.skipCurrentElement();
        } else if (xml.name() == QLatin1String("layer")) {
            Layer *layer = new Layer;
            const QXmlStreamAttributes a = xml.attributes();
            layer->name = a.value(QStringLiteral("name")).toString();
            if (layer->name.isEmpty())
                layer->name = QStringLiteral("Layer %1").arg(page->layers.size() + 1);
            if (a.hasAttribute(QStringLiteral("visible")))
                layer->visible = a.value(QStringLiteral("visible")).toString() != QLatin1String("false");
            layer->locked = a.value(QStringLiteral("locked")).toString() == QLatin1String("true");
            layer->opacity = attrReal(a, QStringLiteral("opacity"), 1.0);
            readLayer(xml, layer);
            page->layers.append(layer);
        } else {
            xml.skipCurrentElement();
        }
    }
}

void XoppReader::readLayer(QXmlStreamReader &xml, Layer *layer)
{
    while (!xml.atEnd()) {
        const QXmlStreamReader::TokenType t = xml.readNext();
        if (t == QXmlStreamReader::EndElement)
            return;
        if (t != QXmlStreamReader::StartElement)
            continue;

        if (xml.name() == QLatin1String("stroke"))
            readStroke(xml, layer);
        else if (xml.name() == QLatin1String("text"))
            readText(xml, layer);
        else if (xml.name() == QLatin1String("image") || xml.name() == QLatin1String("teximage"))
            readImage(xml, layer);
        else
            xml.skipCurrentElement();
    }
}

static Path *pathFromCurve(const QXmlStreamAttributes &a)
{
    const QStringList n = splitNumbers(a.value(QStringLiteral("curve")).toString());
    if (n.size() < 8 || (n.size() - 2) % 6 != 0)
        return 0;

    Path *p = new Path;
    p->start = QPointF(n.at(0).toDouble(), n.at(1).toDouble());
    for (int i = 2; i + 5 < n.size(); i += 6) {
        p->segments << CubicSegment(QPointF(n.at(i).toDouble(), n.at(i + 1).toDouble()),
                                    QPointF(n.at(i + 2).toDouble(), n.at(i + 3).toDouble()),
                                    QPointF(n.at(i + 4).toDouble(), n.at(i + 5).toDouble()));
    }
    p->closed = a.value(QStringLiteral("curveClosed")).toString() == QLatin1String("true");

    if (a.hasAttribute(QStringLiteral("curveHoles"))) {
        const QStringList holes =
                a.value(QStringLiteral("curveHoles")).toString().split(QLatin1Char(';'),
                                                                       QString::SkipEmptyParts);
        for (int k = 0; k < holes.size(); ++k) {
            const QStringList h = splitNumbers(holes.at(k));
            if (h.size() < 8 || (h.size() - 2) % 6 != 0)
                continue;
            SubPath sub;
            sub.start = QPointF(h.at(0).toDouble(), h.at(1).toDouble());
            for (int i = 2; i + 5 < h.size(); i += 6) {
                sub.segments << CubicSegment(
                        QPointF(h.at(i).toDouble(), h.at(i + 1).toDouble()),
                        QPointF(h.at(i + 2).toDouble(), h.at(i + 3).toDouble()),
                        QPointF(h.at(i + 4).toDouble(), h.at(i + 5).toDouble()));
            }
            if (!sub.segments.isEmpty())
                p->extra << sub;
        }
        p->invalidate();
    }
    p->color = parseColor(a.value(QStringLiteral("color")).toString(), Qt::black);
    p->width = attrReal(a, QStringLiteral("width"), 1.41);
    p->lineStyle = a.value(QStringLiteral("style")).toString();
    if (a.hasAttribute(QStringLiteral("fill")))
        p->fill = int(attrReal(a, QStringLiteral("fill"), -1));
    if (a.hasAttribute(QStringLiteral("fillColor")))
        p->fillColor = parseColor(a.value(QStringLiteral("fillColor")).toString(), QColor());
    if (a.hasAttribute(QStringLiteral("gradient"))) {
        p->gradient = Gradient::fromString(a.value(QStringLiteral("gradient")).toString());
        p->gradientAngle = int(attrReal(a, QStringLiteral("gradientAngle"), 0));
    }
    p->group = int(attrReal(a, QStringLiteral("group"), 0));
    p->invalidate();
    return p;
}

void XoppReader::readStroke(QXmlStreamReader &xml, Layer *layer)
{
    const QXmlStreamAttributes a = xml.attributes();

    if (a.hasAttribute(QStringLiteral("curve"))) {
        Path *p = pathFromCurve(a);
        if (p) {
            xml.skipCurrentElement();
            layer->append(p);
            return;
        }
    }

    Stroke *s = new Stroke;

    const QString tool = a.value(QStringLiteral("tool")).toString();
    if (tool == QLatin1String("highlighter"))
        s->tool = Stroke::Highlighter;
    else if (tool == QLatin1String("eraser"))
        s->tool = Stroke::Eraser;
    else
        s->tool = Stroke::Pen;

    s->color = parseColor(a.value(QStringLiteral("color")).toString(), Qt::black);

    const QString capStyle = a.value(QStringLiteral("capStyle")).toString();
    if (capStyle == QLatin1String("butt"))
        s->cap = Stroke::ButtCap;
    else if (capStyle == QLatin1String("square"))
        s->cap = Stroke::SquareCap;
    else
        s->cap = Stroke::RoundCap;

    s->lineStyle = a.value(QStringLiteral("style")).toString();
    if (a.hasAttribute(QStringLiteral("fill")))
        s->fill = int(attrReal(a, QStringLiteral("fill"), -1));
    if (a.hasAttribute(QStringLiteral("fillColor")))
        s->fillColor = parseColor(a.value(QStringLiteral("fillColor")).toString(), QColor());
    if (a.hasAttribute(QStringLiteral("gradient"))) {
        s->gradient = Gradient::fromString(a.value(QStringLiteral("gradient")).toString());
        s->gradientAngle = int(attrReal(a, QStringLiteral("gradientAngle"), 0));
    }

    const QString shape = a.value(QStringLiteral("shape")).toString();
    if (shape == QLatin1String("line")) s->shape = Stroke::LineShape;
    else if (shape == QLatin1String("rect")) s->shape = Stroke::RectShape;
    else if (shape == QLatin1String("ellipse")) s->shape = Stroke::EllipseShape;
    else if (shape == QLatin1String("arrow")) s->shape = Stroke::ArrowShape;
    else if (shape == QLatin1String("polygon")) s->shape = Stroke::PolygonShape;
    else if (shape == QLatin1String("star")) s->shape = Stroke::StarShape;
    else if (shape == QLatin1String("polyline")) s->shape = Stroke::PolylineShape;

    const QStringList widths = splitNumbers(a.value(QStringLiteral("width")).toString());
    if (!widths.isEmpty())
        s->width = widths.first().toDouble();

    s->group = int(attrReal(a, QStringLiteral("group"), 0));

    const QStringList coords = splitNumbers(xml.readElementText());
    const int n = coords.size() / 2;
    QVector<StrokePoint> pts;
    pts.reserve(n);
    for (int i = 0; i < n; ++i) {
        StrokePoint p(coords.at(2 * i).toDouble(), coords.at(2 * i + 1).toDouble());
        if (widths.size() > i + 1)
            p.width = widths.at(i + 1).toDouble();
        pts.append(p);
    }

    if (pts.isEmpty()) {
        delete s;
        return;
    }
    s->setPoints(pts);
    layer->append(s);
}

void XoppReader::readText(QXmlStreamReader &xml, Layer *layer)
{
    const QXmlStreamAttributes a = xml.attributes();
    TextItem *t = new TextItem;
    t->fontName = a.value(QStringLiteral("font")).toString();
    if (t->fontName.isEmpty())
        t->fontName = QStringLiteral("Sans");
    t->fontSize = attrReal(a, QStringLiteral("size"), 12);
    t->pos = QPointF(attrReal(a, QStringLiteral("x"), 0), attrReal(a, QStringLiteral("y"), 0));
    t->color = parseColor(a.value(QStringLiteral("color")).toString(), Qt::black);
    t->bold = a.value(QStringLiteral("bold")).toString() == QLatin1String("true");
    t->italic = a.value(QStringLiteral("italic")).toString() == QLatin1String("true");
    t->underline = a.value(QStringLiteral("underline")).toString() == QLatin1String("true");
    const QString textStyle = a.value(QStringLiteral("textStyle")).toString();
    if (textStyle == QLatin1String("outline"))
        t->style = TextItem::Outline;
    else if (textStyle == QLatin1String("inline"))
        t->style = TextItem::Inline;
    t->group = int(attrReal(a, QStringLiteral("group"), 0));
    t->text = xml.readElementText();

    if (t->text.isEmpty()) {
        delete t;
        return;
    }
    layer->append(t);
}

void XoppReader::readImage(QXmlStreamReader &xml, Layer *layer)
{
    const QXmlStreamAttributes a = xml.attributes();
    const qreal left = attrReal(a, QStringLiteral("left"), 0);
    const qreal top = attrReal(a, QStringLiteral("top"), 0);
    const qreal right = attrReal(a, QStringLiteral("right"), 0);
    const qreal bottom = attrReal(a, QStringLiteral("bottom"), 0);

    const QByteArray raw = QByteArray::fromBase64(xml.readElementText().toLatin1());
    QImage img;
    if (raw.isEmpty() || !img.loadFromData(raw))
        return;

    ImageItem *item = new ImageItem;
    item->group = int(attrReal(a, QStringLiteral("group"), 0));
    item->image = img;
    item->rect = QRectF(QPointF(left, top), QPointF(right, bottom)).normalized();
    layer->append(item);
}
}
