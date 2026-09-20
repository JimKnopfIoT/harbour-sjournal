#ifndef XN_NODEEDITOR_H
#define XN_NODEEDITOR_H

#include "model/path.h"
#include "model/stroke.h"

#include <QObject>
#include <QPointF>
#include <QVector>

namespace xn {
class Element;
class UndoStack;

class NodeEditor : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int count READ count NOTIFY changed)
    Q_PROPERTY(bool active READ isActive NOTIFY changed)
    Q_PROPERTY(bool busy READ isBusy NOTIFY changed)
    Q_PROPERTY(bool hasNode READ hasNode NOTIFY changed)
    Q_PROPERTY(qreal nodeX READ nodeX NOTIFY changed)
    Q_PROPERTY(qreal nodeY READ nodeY NOTIFY changed)

public:
    enum Kind { Anchor, ControlIn, ControlOut };

    struct Node
    {
        Node(): kind(Anchor), anchor(-1) {}
        Node(const QPointF &p, Kind k, int a): pos(p), kind(k), anchor(a) {}

        QPointF pos;
        Kind kind;
        int anchor;
    };

    explicit NodeEditor(UndoStack *undo, QObject *parent = 0);

    Element *target() const { return m_target; }
    void setTarget(Element *e);
    void forget();
    Q_INVOKABLE void clear();

    int page() const { return m_page; }
    void setPage(int page) { m_page = page; }

    const QVector<Node> &nodes() const { return m_nodes; }
    int count() const { return m_nodes.size(); }
    bool isActive() const { return m_target != 0; }
    bool isDragging() const { return m_dragging >= 0; }

    bool isBusy() const;
    int activeAnchor() const { return m_activeAnchor; }

    bool hasNode() const { return m_selected >= 0 && m_selected < m_nodes.size(); }
    qreal nodeX() const { return hasNode() ? m_nodes.at(m_selected).pos.x() : 0; }
    qreal nodeY() const { return hasNode() ? m_nodes.at(m_selected).pos.y() : 0; }
    Q_INVOKABLE void moveNodeTo(qreal x, qreal y);

    int hit(const QPointF &pos, qreal radius) const;
    void select(int node);
    void beginDrag(int node);
    void dragTo(const QPointF &pos);
    void endDrag();

    Q_INVOKABLE int simplify();

Q_SIGNALS:
    void changed();
    void contentChanged(int page);
    void historyChanged();

private:
    void rebuild();
    void snapshot();
    void moveAnchor(int anchor, const QPointF &delta);

    Stroke *stroke() const;
    Path *path() const;

    UndoStack *m_undo;
    Element *m_target;
    int m_page;

    QVector<Node> m_nodes;
    int m_activeAnchor;
    int m_selected;
    int m_dragging;

    QVector<StrokePoint> m_strokeBefore;
    QPointF m_startBefore;
    QVector<CubicSegment> m_segmentsBefore;
    bool m_haveSnapshot;
};
}

#endif
