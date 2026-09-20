#include "app/selectiontransform.h"

#include "app/selection.h"
#include "app/undostack.h"
#include "model/element.h"

#include <QLineF>

namespace xn {
static const qreal kMinScale = 0.02;

SelectionTransform::SelectionTransform(Selection *selection, UndoStack *undo, QObject *parent)
    : QObject(parent)
    , m_selection(selection)
    , m_undo(undo)
    , m_mode(Move)
    , m_scaling(false)
    , m_handle(-1)
    , m_totalX(1)
    , m_totalY(1)
{
}

void SelectionTransform::setMode(Mode m)
{
    if (m_mode == m)
        return;
    m_mode = m;
    Q_EMIT modeChanged();
}

bool SelectionTransform::applyScale(const QPointF &origin, qreal sx, qreal sy)
{
    const QVector<Element *> &items = m_selection->elements();
    if (items.isEmpty())
        return false;

    for (int i = 0; i < items.size(); ++i)
        items.at(i)->scale(origin, sx, sy);
    return true;
}

QRectF SelectionTransform::box() const
{
    return m_selection->bounds();
}

bool SelectionTransform::moveTo(qreal x, qreal y)
{
    const QRectF b = m_selection->bounds();
    if (b.isNull())
        return false;

    const qreal dx = x - b.x();
    const qreal dy = y - b.y();
    if (qFuzzyIsNull(dx) && qFuzzyIsNull(dy))
        return false;

    const QVector<Element *> &items = m_selection->elements();
    for (int i = 0; i < items.size(); ++i)
        items.at(i)->translate(dx, dy);

    m_undo->pushWithoutRedo(new MoveElementsCommand(items, dx, dy));
    Q_EMIT contentChanged(m_selection->page());
    Q_EMIT historyChanged();
    Q_EMIT boxChanged();
    return true;
}

bool SelectionTransform::resizeTo(qreal w, qreal h)
{
    const QRectF b = m_selection->bounds();
    if (b.isNull() || b.width() <= 0 || b.height() <= 0 || w <= 0 || h <= 0)
        return false;

    const qreal sx = w / b.width();
    const qreal sy = h / b.height();
    if (qFuzzyCompare(sx, qreal(1)) && qFuzzyCompare(sy, qreal(1)))
        return false;

    if (!applyScale(b.topLeft(), sx, sy))
        return false;

    m_undo->pushWithoutRedo(new ScaleElementsCommand(m_selection->elements(),
                                                     b.topLeft(), sx, sy));
    Q_EMIT contentChanged(m_selection->page());
    Q_EMIT historyChanged();
    Q_EMIT boxChanged();
    return true;
}

bool SelectionTransform::flipHorizontal()
{
    const QRectF box = m_selection->bounds();
    if (box.isNull() || !applyScale(box.center(), -1, 1))
        return false;

    m_undo->pushWithoutRedo(new ScaleElementsCommand(m_selection->elements(),
                                                     box.center(), -1, 1));
    Q_EMIT contentChanged(m_selection->page());
    Q_EMIT historyChanged();
    Q_EMIT boxChanged();
    return true;
}

bool SelectionTransform::flipVertical()
{
    const QRectF box = m_selection->bounds();
    if (box.isNull() || !applyScale(box.center(), 1, -1))
        return false;

    m_undo->pushWithoutRedo(new ScaleElementsCommand(m_selection->elements(),
                                                     box.center(), 1, -1));
    Q_EMIT contentChanged(m_selection->page());
    Q_EMIT historyChanged();
    Q_EMIT boxChanged();
    return true;
}

// Corners, clockwise from top left; the opposite corner stays put while dragging.
int SelectionTransform::grabHandle(const QPointF &pos, qreal radius) const
{
    const QRectF box = m_selection->bounds();
    if (box.isNull())
        return -1;

    const QPointF corners[4] = { box.topLeft(), box.topRight(),
                                 box.bottomRight(), box.bottomLeft() };
    for (int i = 0; i < 4; ++i)
        if (QLineF(pos, corners[i]).length() <= radius)
            return i;
    return -1;
}

void SelectionTransform::beginScale(int handle)
{
    if (handle < 0 || handle > 3)
        return;

    m_startRect = m_selection->bounds();
    if (m_startRect.isNull())
        return;

    const QPointF corners[4] = { m_startRect.topLeft(), m_startRect.topRight(),
                                 m_startRect.bottomRight(), m_startRect.bottomLeft() };
    m_handle = handle;
    m_anchor = corners[(handle + 2) % 4];
    m_scaling = true;
    m_totalX = 1;
    m_totalY = 1;
}

void SelectionTransform::scaleTo(const QPointF &pos, bool keepAspect)
{
    if (!m_scaling)
        return;

    const QPointF corners[4] = { m_startRect.topLeft(), m_startRect.topRight(),
                                 m_startRect.bottomRight(), m_startRect.bottomLeft() };
    const QPointF grabbed = corners[m_handle];

    const qreal spanX = grabbed.x() - m_anchor.x();
    const qreal spanY = grabbed.y() - m_anchor.y();
    if (qAbs(spanX) < 1e-6 || qAbs(spanY) < 1e-6)
        return;

    qreal wantX = (pos.x() - m_anchor.x()) / spanX;
    qreal wantY = (pos.y() - m_anchor.y()) / spanY;
    if (keepAspect) {
        const qreal both = qMax(qAbs(wantX), qAbs(wantY));
        wantX = wantX < 0 ? -both : both;
        wantY = wantY < 0 ? -both : both;
    }

    if (qAbs(wantX) < kMinScale || qAbs(wantY) < kMinScale)
        return;

    const qreal stepX = wantX / m_totalX;
    const qreal stepY = wantY / m_totalY;
    if (!applyScale(m_anchor, stepX, stepY))
        return;

    m_totalX = wantX;
    m_totalY = wantY;
    Q_EMIT contentChanged(m_selection->page());
    Q_EMIT boxChanged();
}

void SelectionTransform::endScale()
{
    if (!m_scaling)
        return;

    m_scaling = false;
    m_handle = -1;
    if (qFuzzyCompare(m_totalX, qreal(1)) && qFuzzyCompare(m_totalY, qreal(1)))
        return;

    m_undo->pushWithoutRedo(new ScaleElementsCommand(m_selection->elements(),
                                                     m_anchor, m_totalX, m_totalY));
    Q_EMIT historyChanged();
}
}
