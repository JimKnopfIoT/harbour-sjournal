#include "app/selection.h"

#include "app/undostack.h"
#include "model/document.h"
#include "model/path.h"
#include "model/stroke.h"

#include <QLineF>

namespace xn {
static const qreal kPasteOffset = 4.0 * 72.0 / 25.4;

static bool elementHit(const Element *e, const QPointF &pos, qreal radius)
{
    if (!e->bounds().adjusted(-radius, -radius, radius, radius).contains(pos))
        return false;

    if (e->type() == Element::PathType)
        return static_cast<const Path *>(e)->hits(pos, radius);

    if (e->type() != Element::StrokeType)
        return e->bounds().contains(pos);

    const Stroke *s = static_cast<const Stroke *>(e);
    if (s->fill >= 0 && e->bounds().contains(pos))
        return true;

    for (int i = 0; i + 1 < s->points.size(); ++i) {
        const StrokePoint &a = s->points.at(i);
        const StrokePoint &b = s->points.at(i + 1);
        const qreal dx = b.x - a.x;
        const qreal dy = b.y - a.y;
        const qreal lenSq = dx * dx + dy * dy;
        qreal t = 0;
        if (lenSq > 1e-9)
            t = qBound(qreal(0), ((pos.x() - a.x) * dx + (pos.y() - a.y) * dy) / lenSq, qreal(1));
        if (QLineF(pos, QPointF(a.x + t * dx, a.y + t * dy)).length() <= radius + s->widthAt(i) / 2)
            return true;
    }
    return false;
}

Selection::Selection(Document *document, UndoStack *undo, QObject *parent)
    : QObject(parent)
    , m_document(document)
    , m_undo(undo)
    , m_page(-1)
    , m_layer(0)
    , m_moving(false)
{
}

Selection::~Selection()
{
    qDeleteAll(m_clipboard);
}

Layer *Selection::layerOf(int page, int index) const
{
    Page *p = m_document->pageAt(page);
    return p ? p->layerAt(index) : 0;
}

void Selection::expandToGroups(Layer *layer)
{
    if (!layer)
        return;
    QVector<int> groups;
    for (int i = 0; i < m_elements.size(); ++i) {
        const int g = m_elements.at(i)->group;
        if (g != 0 && !groups.contains(g))
            groups.append(g);
    }
    if (groups.isEmpty())
        return;

    for (int i = 0; i < layer->count(); ++i) {
        Element *e = layer->elements.at(i);
        if (groups.contains(e->group) && !m_elements.contains(e))
            m_elements.append(e);
    }
}

bool Selection::canGroup() const
{
    return m_elements.size() > 1;
}

bool Selection::canUngroup() const
{
    for (int i = 0; i < m_elements.size(); ++i) {
        if (m_elements.at(i)->group != 0)
            return true;
    }
    return false;
}

int Selection::group()
{
    Layer *layer = layerOf(m_page, m_layer);
    if (!layer || m_elements.size() < 2)
        return 0;

    int highest = 0;
    for (int i = 0; i < layer->count(); ++i)
        highest = qMax(highest, layer->elements.at(i)->group);

    m_undo->push(new GroupElementsCommand(layer, m_elements, highest + 1));
    Q_EMIT historyChanged();
    Q_EMIT changed();
    Q_EMIT contentChanged(m_page);
    return m_elements.size();
}

int Selection::ungroup()
{
    Layer *layer = layerOf(m_page, m_layer);
    if (!layer || m_elements.isEmpty())
        return 0;

    m_undo->push(new GroupElementsCommand(layer, m_elements, 0));
    Q_EMIT historyChanged();
    Q_EMIT changed();
    Q_EMIT contentChanged(m_page);
    return m_elements.size();
}

QRectF Selection::bounds() const
{
    QRectF box;
    for (int i = 0; i < m_elements.size(); ++i)
        box = box.isNull() ? m_elements.at(i)->bounds() : box.united(m_elements.at(i)->bounds());
    return box;
}

bool Selection::hasImage() const
{
    Page *page = m_document->pageAt(m_page);
    if (!page || m_rect.isEmpty())
        return false;

    for (int l = page->layerCount() - 1; l >= 0; --l) {
        const Layer *layer = page->layers.at(l);
        if (!layer->visible)
            continue;
        for (int i = layer->count() - 1; i >= 0; --i) {
            const Element *e = layer->elements.at(i);
            if (e->type() == Element::ImageType && e->bounds().intersects(m_rect))
                return true;
        }
    }
    return false;
}

// Same rule as the rubber band: picked only when it lies wholly inside.
void Selection::setLasso(int page, const QPolygonF &lasso)
{
    m_page = page;
    m_rect = lasso.boundingRect();
    m_elements.clear();

    Layer *layer = layerOf(page, m_layer);
    if (layer && !layer->locked && lasso.size() > 2) {
        for (int i = 0; i < layer->count(); ++i) {
            const QRectF b = layer->elements.at(i)->bounds();
            if (lasso.containsPoint(b.topLeft(), Qt::OddEvenFill)
                    && lasso.containsPoint(b.topRight(), Qt::OddEvenFill)
                    && lasso.containsPoint(b.bottomLeft(), Qt::OddEvenFill)
                    && lasso.containsPoint(b.bottomRight(), Qt::OddEvenFill))
                m_elements.append(layer->elements.at(i));
        }
        expandToGroups(layer);
    }
    Q_EMIT changed();
}

void Selection::setRect(int page, const QRectF &rect)
{
    m_page = page;
    m_rect = rect;
    m_elements.clear();

    Layer *layer = layerOf(page, m_layer);
    if (layer && !layer->locked) {
        for (int i = 0; i < layer->count(); ++i) {
            if (rect.contains(layer->elements.at(i)->bounds()))
                m_elements.append(layer->elements.at(i));
        }
        expandToGroups(layer);
    }
    Q_EMIT changed();
}

void Selection::clear()
{
    if (m_rect.isEmpty() && m_elements.isEmpty())
        return;
    m_rect = QRectF();
    m_elements.clear();
    m_page = -1;
    Q_EMIT changed();
}

void Selection::forget()
{
    m_rect = QRectF();
    m_elements.clear();
    m_page = -1;
}

Element *Selection::elementAt(int page, const QPointF &pos, qreal radius) const
{
    Page *p = m_document->pageAt(page);
    if (!p)
        return 0;

    for (int l = p->layerCount() - 1; l >= 0; --l) {
        Layer *layer = p->layers.at(l);
        if (!layer->visible || layer->locked)
            continue;
        for (int i = layer->count() - 1; i >= 0; --i) {
            Element *e = layer->elements.at(i);
            if (elementHit(e, pos, radius))
                return e;
        }
    }
    return 0;
}

bool Selection::selectAt(int page, const QPointF &pos, qreal radius)
{
    Page *p = m_document->pageAt(page);
    if (!p)
        return false;

    for (int l = p->layerCount() - 1; l >= 0; --l) {
        Layer *layer = p->layers.at(l);
        if (!layer->visible || layer->locked)
            continue;
        for (int i = layer->count() - 1; i >= 0; --i) {
            Element *e = layer->elements.at(i);
            if (!elementHit(e, pos, radius))
                continue;

            m_elements.clear();
            m_elements.append(e);
            expandToGroups(layer);
            m_page = page;
            m_rect = QRectF();
            if (m_layer != l)
                Q_EMIT layerRequested(l);
            Q_EMIT changed();
            return true;
        }
    }

    if (!m_elements.isEmpty() || !m_rect.isEmpty())
        clear();
    return false;
}

bool Selection::toggleAt(int page, const QPointF &pos, qreal radius)
{
    Page *p = m_document->pageAt(page);
    if (!p)
        return false;

    if (m_page != page)
        m_elements.clear();

    for (int l = p->layerCount() - 1; l >= 0; --l) {
        Layer *layer = p->layers.at(l);
        if (!layer->visible || layer->locked)
            continue;
        for (int i = layer->count() - 1; i >= 0; --i) {
            Element *e = layer->elements.at(i);
            if (!elementHit(e, pos, radius))
                continue;

            const int at = m_elements.indexOf(e);
            if (at >= 0)
                m_elements.remove(at);
            else
                m_elements.append(e);
            expandToGroups(layer);

            m_page = page;
            m_rect = QRectF();
            Q_EMIT changed();
            return true;
        }
    }
    return false;
}

bool Selection::isOn(int page, const QPointF &pos, qreal radius) const
{
    if (m_page != page || m_elements.isEmpty())
        return false;
    return bounds().adjusted(-radius, -radius, radius, radius).contains(pos);
}

void Selection::beginMove()
{
    m_moveTotal = QPointF();
    m_moving = !m_elements.isEmpty();
}

void Selection::moveBy(qreal dx, qreal dy)
{
    if (!m_moving || m_elements.isEmpty())
        return;
    for (int i = 0; i < m_elements.size(); ++i)
        m_elements.at(i)->translate(dx, dy);
    m_moveTotal += QPointF(dx, dy);
    Q_EMIT changed();
    Q_EMIT contentChanged(m_page);
}

void Selection::endMove()
{
    if (!m_moving)
        return;
    m_moving = false;
    if (qAbs(m_moveTotal.x()) < 0.01 && qAbs(m_moveTotal.y()) < 0.01)
        return;

    m_undo->pushWithoutRedo(new MoveElementsCommand(m_elements, m_moveTotal.x(), m_moveTotal.y()));
    m_moveTotal = QPointF();
    Q_EMIT historyChanged();
}

int Selection::copy()
{
    if (m_elements.isEmpty())
        return 0;

    qDeleteAll(m_clipboard);
    m_clipboard.clear();
    for (int i = 0; i < m_elements.size(); ++i)
        m_clipboard.append(m_elements.at(i)->clone());

    Q_EMIT clipboardChanged();
    return m_clipboard.size();
}

int Selection::remove()
{
    Layer *layer = layerOf(m_page, m_layer);
    if (!layer || m_elements.isEmpty())
        return 0;

    const QVector<Element *> doomed = m_elements;
    m_elements.clear();
    m_undo->push(new RemoveElementsCommand(layer, doomed));

    Q_EMIT historyChanged();
    Q_EMIT changed();
    Q_EMIT contentChanged(m_page);
    return doomed.size();
}

int Selection::cut()
{
    const int copied = copy();
    if (copied == 0)
        return 0;
    remove();
    return copied;
}

int Selection::paste()
{
    if (m_clipboard.isEmpty())
        return 0;

    Layer *layer = layerOf(m_page >= 0 ? m_page : 0, m_layer);
    if (!layer || layer->locked) {
        Q_EMIT error(tr("This layer is locked"));
        return 0;
    }

    const int target = m_page >= 0 ? m_page : 0;
    QRectF pasted;
    for (int i = 0; i < m_clipboard.size(); ++i) {
        Element *copy = m_clipboard.at(i)->clone();
        copy->translate(kPasteOffset, kPasteOffset);
        pasted = pasted.isNull() ? copy->bounds() : pasted.united(copy->bounds());
        m_undo->push(new AddElementCommand(layer, copy));
    }
    for (int i = 0; i < m_clipboard.size(); ++i)
        m_clipboard.at(i)->translate(kPasteOffset, kPasteOffset);

    Q_EMIT historyChanged();
    Q_EMIT contentChanged(target);
    if (!pasted.isNull())
        setRect(target, pasted.adjusted(-2, -2, 2, 2));
    return m_clipboard.size();
}

int Selection::moveToLayer(int layerIndex)
{
    Layer *source = layerOf(m_page, m_layer);
    Layer *target = layerOf(m_page, layerIndex);
    if (!source || !target || source == target || m_elements.isEmpty())
        return 0;
    if (target->locked) {
        Q_EMIT error(tr("That layer is locked"));
        return 0;
    }

    const int moved = m_elements.size();
    m_undo->push(new ReparentElementsCommand(m_elements, source, target));

    Q_EMIT layerRequested(layerIndex);
    Q_EMIT historyChanged();
    Q_EMIT changed();
    Q_EMIT contentChanged(m_page);
    return moved;
}
}
