#ifndef XN_IMAGETOOLS_H
#define XN_IMAGETOOLS_H

#include <QObject>
#include <QRectF>
#include <QString>

namespace xn {
class Document;
class UndoStack;

class ImageTools : public QObject
{
    Q_OBJECT

public:
    ImageTools(Document *document, UndoStack *undo, QObject *parent = 0);

    void setDocument(Document *document) { m_document = document; }
    void setPage(int page) { m_page = page; }
    void setLayer(int layer) { m_layer = layer; }

    bool insertImage(const QString &fileUrl, bool ownLayer);
    bool insertLatestScreenshot();
    bool traceRegion(const QRectF &region, int threshold, bool invert, bool filled,
                     int speckSize, const QColor &color, qreal strokeWidth);

Q_SIGNALS:
    void layersChanged();
    void contentChanged(int page);
    void layerRequested(int layerIndex);
    void historyCleared();
    void error(const QString &message);

private:
    Document *m_document;
    UndoStack *m_undo;
    int m_page;
    int m_layer;
};
}

#endif
