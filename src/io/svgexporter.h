#ifndef XN_SVGEXPORTER_H
#define XN_SVGEXPORTER_H

#include "model/gradient.h"

#include <QByteArray>
#include <QHash>
#include <QRectF>
#include <QString>

class QXmlStreamWriter;

namespace xn {
class Element;
class Page;
class Layer;
class Path;
class Stroke;
class TextItem;
class ImageItem;

class SvgExporter
{
public:
    SvgExporter();

    bool exportPage(const QString &path, const Page *page, bool withBackground = true);
    QByteArray toSvg(const Page *page, bool withBackground = true);

    QString errorString() const { return m_error; }

private:
    void writeBackground(QXmlStreamWriter &xml, const Page *page);
    void writeLayer(QXmlStreamWriter &xml, const Layer *layer, int index);
    void writeStroke(QXmlStreamWriter &xml, const Stroke *s);
    void writeShapeStroke(QXmlStreamWriter &xml, const Stroke *s);
    void writePath(QXmlStreamWriter &xml, const Path *p);
    void writeText(QXmlStreamWriter &xml, const TextItem *t);
    void writeImage(QXmlStreamWriter &xml, const ImageItem *i);

    void collectGradients(const Page *page);
    void writeGradientDefs(QXmlStreamWriter &xml);
    QString fillPaint(const Element *e, const QString &flat) const;

    QString m_error;
    QHash<QString, QString> m_gradientIds;
    QVector<QString> m_gradientOrder;
};
}

#endif
