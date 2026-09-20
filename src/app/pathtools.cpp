#include "app/pathtools.h"

#include "app/selection.h"
#include "app/undostack.h"
#include "model/document.h"
#include "model/element.h"
#include "model/layer.h"
#include "model/page.h"
#include "model/path.h"
#include "model/textitem.h"
#include "tools/pathops.h"

namespace xn {
PathTools::PathTools(Document *document, Selection *selection, UndoStack *undo, QObject *parent)
    : QObject(parent)
    , m_document(document)
    , m_selection(selection)
    , m_undo(undo)
{
    connect(m_selection, &Selection::changed, this, &PathTools::changed);
}

Layer *PathTools::targetLayer() const
{
    Page *page = m_document ? m_document->pageAt(m_selection->page()) : 0;
    if (!page)
        return 0;

    const QVector<Element *> &items = m_selection->elements();
    if (items.isEmpty())
        return 0;

    for (int l = 0; l < page->layerCount(); ++l) {
        Layer *layer = page->layers.at(l);
        if (layer->indexOf(items.first()) >= 0)
            return layer->locked ? 0 : layer;
    }
    return 0;
}

bool PathTools::canConvertText() const
{
    const QVector<Element *> &items = m_selection->elements();
    for (int i = 0; i < items.size(); ++i)
        if (items.at(i)->type() == Element::TextType)
            return true;
    return false;
}

bool PathTools::canCombine() const
{
    int usable = 0;
    const QVector<Element *> &items = m_selection->elements();
    for (int i = 0; i < items.size(); ++i)
        if (PathOps::isConvertible(items.at(i)))
            ++usable;
    return usable > 1;
}

bool PathTools::canBreakApart() const
{
    const QVector<Element *> &items = m_selection->elements();
    return items.size() == 1 && PathOps::isConvertible(items.first());
}

bool PathTools::replace(const QVector<Element *> &removed, const QVector<Path *> &added)
{
    Layer *layer = targetLayer();
    if (!layer) {
        qDeleteAll(added);
        Q_EMIT error(tr("This layer is locked"));
        return false;
    }
    if (removed.isEmpty() || added.isEmpty()) {
        qDeleteAll(added);
        return false;
    }

    QVector<Element *> gone;
    QVector<int> at;
    for (int i = removed.size() - 1; i >= 0; --i) {
        const int index = layer->indexOf(removed.at(i));
        if (index < 0)
            continue;
        layer->take(index);
        gone << removed.at(i);
        at << index;
    }

    if (gone.isEmpty()) {
        qDeleteAll(added);
        return false;
    }

    QVector<Element *> fresh;
    for (int i = 0; i < added.size(); ++i) {
        layer->append(added.at(i));
        fresh << added.at(i);
    }

    m_undo->pushWithoutRedo(new ReplaceElementsCommand(layer, gone, at, fresh));
    m_selection->clear();

    Q_EMIT contentChanged(m_selection->page());
    Q_EMIT historyChanged();
    Q_EMIT changed();
    return true;
}

bool PathTools::textToPath()
{
    const QVector<Element *> items = m_selection->elements();
    QVector<Element *> removed;
    QVector<Path *> added;

    for (int i = 0; i < items.size(); ++i) {
        if (items.at(i)->type() != Element::TextType)
            continue;
        const QVector<Path *> parts =
                PathOps::textToPath(static_cast<const TextItem *>(items.at(i)));
        if (parts.isEmpty())
            continue;
        removed << items.at(i);
        added += parts;
    }

    if (removed.isEmpty()) {
        Q_EMIT error(tr("Select a text first"));
        return false;
    }
    return replace(removed, added);
}

bool PathTools::join()
{
    const QVector<Element *> items = m_selection->elements();
    if (!canCombine())
        return false;

    Path *one = PathOps::join(items);
    if (!one)
        return false;

    QVector<Path *> added;
    added << one;
    return replace(items, added);
}

bool PathTools::breakApart()
{
    const QVector<Element *> items = m_selection->elements();
    if (!canBreakApart())
        return false;

    const QVector<Path *> parts = PathOps::breakApart(items.first());
    if (parts.isEmpty()) {
        Q_EMIT error(tr("Nothing to break apart"));
        return false;
    }
    return replace(items, parts);
}

bool PathTools::unite()
{
    const QVector<Element *> items = m_selection->elements();
    if (!canCombine())
        return false;

    const QVector<Path *> result = PathOps::unite(items);
    if (result.isEmpty())
        return false;
    return replace(items, result);
}

bool PathTools::subtract()
{
    const QVector<Element *> items = m_selection->elements();
    if (!canCombine())
        return false;

    const QVector<Path *> result = PathOps::subtract(items);
    if (result.isEmpty()) {
        Q_EMIT error(tr("Nothing left after subtracting"));
        return false;
    }
    return replace(items, result);
}
}
