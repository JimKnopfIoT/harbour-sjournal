#ifndef XN_SELECTION_H
#define XN_SELECTION_H

#include <QObject>
#include <QPointF>
#include <QPolygonF>
#include <QRectF>
#include <QVector>

namespace xn {
class Document;
class Element;
class Layer;
class UndoStack;

class Selection : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int count READ count NOTIFY changed)
    Q_PROPERTY(QRectF bounds READ bounds NOTIFY changed)
    Q_PROPERTY(QRectF rect READ rect NOTIFY changed)
    Q_PROPERTY(bool hasImage READ hasImage NOTIFY changed)
    Q_PROPERTY(int clipboardCount READ clipboardCount NOTIFY clipboardChanged)

public:
    Selection(Document *document, UndoStack *undo, QObject *parent = 0);
    ~Selection();

    void setDocument(Document *document) { m_document = document; forget(); }
    void setPage(int page) { m_page = page; }
    void setLayer(int layer) { m_layer = layer; }

    int count() const { return m_elements.size(); }
    int page() const { return m_page; }
    QRectF bounds() const;
    QRectF rect() const { return m_rect; }
    bool hasImage() const;
    int clipboardCount() const { return m_clipboard.size(); }
    const QVector<Element *> &elements() const { return m_elements; }

    void setRect(int page, const QRectF &rect);
    void setLasso(int page, const QPolygonF &lasso);
    Q_INVOKABLE void clear();

    bool selectAt(int page, const QPointF &pos, qreal radius);
    Element *elementAt(int page, const QPointF &pos, qreal radius) const;
    bool toggleAt(int page, const QPointF &pos, qreal radius);
    bool isOn(int page, const QPointF &pos, qreal radius) const;

    void beginMove();
    void moveBy(qreal dx, qreal dy);
    void endMove();

    Q_INVOKABLE int copy();
    Q_INVOKABLE int cut();
    Q_INVOKABLE int remove();
    Q_INVOKABLE int paste();
    Q_INVOKABLE int moveToLayer(int layerIndex);

    Q_PROPERTY(bool canGroup READ canGroup NOTIFY changed)
    Q_PROPERTY(bool canUngroup READ canUngroup NOTIFY changed)
    bool canGroup() const;
    bool canUngroup() const;
    Q_INVOKABLE int group();
    Q_INVOKABLE int ungroup();

    void forget();

Q_SIGNALS:
    void changed();
    void clipboardChanged();
    void contentChanged(int page);
    void historyChanged();
    void layerRequested(int layerIndex);
    void error(const QString &message);

private:
    Layer *layerOf(int page, int index) const;
    void expandToGroups(Layer *layer);

    Document *m_document;
    UndoStack *m_undo;
    QVector<Element *> m_elements;
    QVector<Element *> m_clipboard;
    QRectF m_rect;
    QPointF m_moveTotal;
    int m_page;
    int m_layer;
    bool m_moving;
};
}

#endif
