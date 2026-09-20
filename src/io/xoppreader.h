#ifndef XN_XOPPREADER_H
#define XN_XOPPREADER_H

#include <QString>

class QXmlStreamReader;

namespace xn {
class Document;
class Page;
class Layer;

class XoppReader
{
public:
    XoppReader();

    bool read(const QString &path, Document *doc);
    QString errorString() const { return m_error; }

    static bool readSummary(const QString &path, QString *title, int *pageCount,
                            QString *text = 0);

private:
    void readXournal(QXmlStreamReader &xml, Document *doc);
    void readPage(QXmlStreamReader &xml, Page *page);
    void readLayer(QXmlStreamReader &xml, Layer *layer);
    void readStroke(QXmlStreamReader &xml, Layer *layer);
    void readText(QXmlStreamReader &xml, Layer *layer);
    void readImage(QXmlStreamReader &xml, Layer *layer);

    QString m_error;
};
}

#endif
