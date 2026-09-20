#ifndef XN_XOPPWRITER_H
#define XN_XOPPWRITER_H

#include <QImage>
#include <QString>

class QXmlStreamWriter;

namespace xn {
class Document;
class Page;
class Layer;
class Path;
class Stroke;
class TextItem;
class ImageItem;

class XoppWriter
{
public:
    XoppWriter();

    bool write(const QString &path, const Document *doc, const QImage &preview = QImage());

    QByteArray toXml(const Document *doc, const QImage &preview = QImage());

    QString errorString() const { return m_error; }

private:
    void writePage(QXmlStreamWriter &xml, const Page *page);
    void writeLayer(QXmlStreamWriter &xml, const Layer *layer);
    void writeStroke(QXmlStreamWriter &xml, const Stroke *s);
    void writePath(QXmlStreamWriter &xml, const Path *p);
    void writeText(QXmlStreamWriter &xml, const TextItem *t);
    void writeImage(QXmlStreamWriter &xml, const ImageItem *i);

    QString m_error;
};
}

#endif
