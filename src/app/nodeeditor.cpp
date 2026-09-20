#include "app/nodeeditor.h"

#include "app/undostack.h"
#include "model/element.h"

#include <QLineF>

namespace xn {
static const int kBusyNodeCount = 60;

static bool samePoints(const QVector<StrokePoint> &a, const QVector<StrokePoint> &b)
{
    if (a.size() != b.size())
        return false;
    for (int i = 0; i < a.size(); ++i)
        if (a.at(i).x != b.at(i).x || a.at(i).y != b.at(i).y)
            return false;
    return true;
}

NodeEditor::NodeEditor(UndoStack *undo, QObject *parent)
    : QObject(parent)
    , m_undo(undo)
    , m_target(0)
    , m_page(0)
    , m_activeAnchor(-1)
    , m_selected(-1)
    , m_dragging(-1)
    , m_haveSnapshot(false)
{
}

Stroke *NodeEditor::stroke() const
{
    return m_target && m_target->type() == Element::StrokeType
            ? static_cast<Stroke *>(m_target) : 0;
}

Path *NodeEditor::path() const
{
    return m_target && m_target->type() == Element::PathType
            ? static_cast<Path *>(m_target) : 0;
}

void NodeEditor::setTarget(Element *e)
{
    if (m_target == e)
        return;
    m_target = e;
    m_activeAnchor = -1;
    m_selected = -1;
    m_dragging = -1;
    m_haveSnapshot = false;
    rebuild();
    Q_EMIT changed();
}

void NodeEditor::forget()
{
    m_target = 0;
    m_activeAnchor = -1;
    m_selected = -1;
    m_dragging = -1;
    m_haveSnapshot = false;
    m_nodes.clear();
}

void NodeEditor::clear()
{
    if (!m_target)
        return;
    forget();
    Q_EMIT changed();
}

bool NodeEditor::isBusy() const
{
    const Stroke *s = stroke();
    return s && s->points.size() > kBusyNodeCount;
}

void NodeEditor::rebuild()
{
    m_nodes.clear();
    if (!m_target)
        return;

    if (const Stroke *s = stroke()) {
        if (s->points.size() > kBusyNodeCount)
            return;
        for (int i = 0; i < s->points.size(); ++i)
            m_nodes << Node(QPointF(s->points.at(i).x, s->points.at(i).y), Anchor, i);
        return;
    }

    if (const Path *p = path()) {
        const QVector<QPointF> anchors = p->anchors();
        for (int i = 0; i < anchors.size(); ++i)
            m_nodes << Node(anchors.at(i), Anchor, i);

        // Only the touched anchor shows handles; all of them at once is unreadable.
        const int a = m_activeAnchor;
        if (a >= 0 && a < anchors.size()) {
            if (a >= 1 && a - 1 < p->segments.size())
                m_nodes << Node(p->segments.at(a - 1).c2, ControlIn, a);
            if (a < p->segments.size())
                m_nodes << Node(p->segments.at(a).c1, ControlOut, a);
        }
    }
}

int NodeEditor::hit(const QPointF &pos, qreal radius) const
{
    int best = -1;
    qreal bestDistance = radius;

    // Handles win ties against the anchor they belong to: they sit on top.
    for (int i = m_nodes.size() - 1; i >= 0; --i) {
        const qreal d = QLineF(pos, m_nodes.at(i).pos).length();
        if (d < bestDistance) {
            bestDistance = d;
            best = i;
        }
    }
    return best;
}

void NodeEditor::snapshot()
{
    if (Stroke *s = stroke()) {
        m_strokeBefore = s->points;
        m_haveSnapshot = true;
    } else if (Path *p = path()) {
        m_startBefore = p->start;
        m_segmentsBefore = p->segments;
        m_haveSnapshot = true;
    }
}

void NodeEditor::select(int node)
{
    if (node == m_selected)
        return;
    m_selected = node >= 0 && node < m_nodes.size() ? node : -1;
    Q_EMIT changed();
}

void NodeEditor::moveNodeTo(qreal x, qreal y)
{
    if (!hasNode())
        return;

    m_dragging = m_selected;
    snapshot();
    dragTo(QPointF(x, y));
    endDrag();
}

void NodeEditor::beginDrag(int node)
{
    if (node < 0 || node >= m_nodes.size())
        return;

    m_dragging = node;
    m_selected = node;
    if (m_nodes.at(node).kind == Anchor && path()) {
        m_activeAnchor = m_nodes.at(node).anchor;
        rebuild();
        // Handles are appended after the anchors, so the dragged index still holds.
        Q_EMIT changed();
    }
    snapshot();
}

void NodeEditor::moveAnchor(int anchor, const QPointF &delta)
{
    Path *p = path();
    if (!p)
        return;

    if (anchor == 0) {
        p->start += delta;
        if (!p->segments.isEmpty())
            p->segments[0].c1 += delta;
    } else {
        const int seg = anchor - 1;
        if (seg >= p->segments.size())
            return;
        p->segments[seg].to += delta;
        p->segments[seg].c2 += delta;
        if (anchor < p->segments.size())
            p->segments[anchor].c1 += delta;
    }
    p->invalidate();
}

void NodeEditor::dragTo(const QPointF &pos)
{
    if (m_dragging < 0 || m_dragging >= m_nodes.size())
        return;

    const Node node = m_nodes.at(m_dragging);

    if (Stroke *s = stroke()) {
        if (node.anchor < 0 || node.anchor >= s->points.size())
            return;
        QVector<StrokePoint> pts = s->points;
        pts[node.anchor].x = pos.x();
        pts[node.anchor].y = pos.y();
        s->setPoints(pts);
    } else if (Path *p = path()) {
        if (node.kind == Anchor) {
            moveAnchor(node.anchor, pos - node.pos);
        } else if (node.kind == ControlIn) {
            const int seg = node.anchor - 1;
            if (seg < 0 || seg >= p->segments.size())
                return;
            p->segments[seg].c2 = pos;
            p->invalidate();
        } else {
            if (node.anchor >= p->segments.size())
                return;
            p->segments[node.anchor].c1 = pos;
            p->invalidate();
        }
    } else {
        return;
    }

    rebuild();
    if (m_selected >= m_nodes.size())
        m_selected = -1;
    Q_EMIT changed();
    Q_EMIT contentChanged(m_page);
}

void NodeEditor::endDrag()
{
    if (m_dragging < 0) {
        m_haveSnapshot = false;
        return;
    }

    m_dragging = -1;
    if (!m_haveSnapshot)
        return;
    m_haveSnapshot = false;

    if (Stroke *s = stroke()) {
        if (samePoints(s->points, m_strokeBefore))
            return;
        m_undo->pushWithoutRedo(new ChangeGeometryCommand(s, m_strokeBefore, s->points));
    } else if (Path *p = path()) {
        if (p->start == m_startBefore && p->segments.size() == m_segmentsBefore.size()) {
            bool same = true;
            for (int i = 0; i < p->segments.size() && same; ++i) {
                same = p->segments.at(i).c1 == m_segmentsBefore.at(i).c1
                        && p->segments.at(i).c2 == m_segmentsBefore.at(i).c2
                        && p->segments.at(i).to == m_segmentsBefore.at(i).to;
            }
            if (same)
                return;
        }
        m_undo->pushWithoutRedo(new ChangeGeometryCommand(p, m_startBefore, m_segmentsBefore,
                                                          p->start, p->segments));
    } else {
        return;
    }

    Q_EMIT historyChanged();
    Q_EMIT changed();
}

static void douglasPeucker(const QVector<StrokePoint> &pts, int first, int last,
                           qreal tolerance, QVector<bool> &keep)
{
    if (last <= first + 1)
        return;

    const QPointF a(pts.at(first).x, pts.at(first).y);
    const QPointF b(pts.at(last).x, pts.at(last).y);
    const QLineF span(a, b);
    const qreal lenSq = span.dx() * span.dx() + span.dy() * span.dy();

    qreal worst = -1;
    int worstAt = -1;
    for (int i = first + 1; i < last; ++i) {
        const QPointF p(pts.at(i).x, pts.at(i).y);
        QPointF foot = a;
        if (lenSq > 1e-9) {
            const qreal t = qBound(qreal(0),
                                   ((p.x() - a.x()) * span.dx() + (p.y() - a.y()) * span.dy())
                                   / lenSq, qreal(1));
            foot = a + t * QPointF(span.dx(), span.dy());
        }
        const qreal d = QLineF(p, foot).length();
        if (d > worst) {
            worst = d;
            worstAt = i;
        }
    }

    if (worst <= tolerance || worstAt < 0)
        return;

    keep[worstAt] = true;
    douglasPeucker(pts, first, worstAt, tolerance, keep);
    douglasPeucker(pts, worstAt, last, tolerance, keep);
}

int NodeEditor::simplify()
{
    Stroke *s = stroke();
    if (!s || s->points.size() < 3)
        return 0;

    const QVector<StrokePoint> before = s->points;

    // Loosen until a handful of handles remain; this wants an editable path, not a copy.
    QVector<StrokePoint> result;
    for (qreal tolerance = 0.5; tolerance <= 32.0; tolerance *= 1.6) {
        QVector<bool> keep(before.size(), false);
        keep[0] = true;
        keep[before.size() - 1] = true;
        douglasPeucker(before, 0, before.size() - 1, tolerance, keep);

        result.clear();
        for (int i = 0; i < before.size(); ++i)
            if (keep.at(i))
                result << before.at(i);

        if (result.size() <= 24)
            break;
    }

    if (result.size() < 2 || result.size() == before.size())
        return 0;

    s->setPoints(result);
    m_undo->pushWithoutRedo(new ChangeGeometryCommand(s, before, result));

    rebuild();
    Q_EMIT contentChanged(m_page);
    Q_EMIT historyChanged();
    Q_EMIT changed();
    return result.size();
}
}
