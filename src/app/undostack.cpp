#include "app/undostack.h"

#include "model/element.h"
#include "model/layer.h"
#include "model/path.h"
#include "model/stroke.h"
#include "model/textitem.h"
#include "model/textitem.h"

#include <algorithm>

namespace xn {
AddElementCommand::AddElementCommand(Layer *layer, Element *element)
    : m_layer(layer)
    , m_element(element)
    , m_index(-1)
    , m_ownsElement(true)
{
}

AddElementCommand::~AddElementCommand()
{
    if (m_ownsElement)
        delete m_element;
}

void AddElementCommand::redo()
{
    if (!m_layer || !m_element || !m_ownsElement)
        return;
    if (m_index < 0 || m_index > m_layer->count())
        m_index = m_layer->count();
    m_layer->insert(m_index, m_element);
    m_ownsElement = false;
}

void AddElementCommand::undo()
{
    if (!m_layer || m_ownsElement)
        return;
    const int at = m_layer->indexOf(m_element);
    if (at < 0)
        return;
    m_index = at;
    m_layer->take(at);
    m_ownsElement = true;
}

RemoveElementsCommand::RemoveElementsCommand(Layer *layer, const QVector<Element *> &elements)
    : m_layer(layer)
    , m_ownsElements(false)
{
    if (!layer)
        return;

    for (int i = 0; i < elements.size(); ++i) {
        const int at = layer->indexOf(elements.at(i));
        if (at >= 0) {
            m_elements.append(elements.at(i));
            m_indexes.append(at);
        }
    }

    for (int i = 0; i < m_indexes.size(); ++i) {
        for (int j = i + 1; j < m_indexes.size(); ++j) {
            if (m_indexes.at(j) < m_indexes.at(i)) {
                qSwap(m_indexes[i], m_indexes[j]);
                qSwap(m_elements[i], m_elements[j]);
            }
        }
    }
}

RemoveElementsCommand::RemoveElementsCommand(Layer *layer, const QVector<Element *> &elements,
                                             const QVector<int> &indexes)
    : m_layer(layer)
    , m_elements(elements)
    , m_indexes(indexes)
    , m_ownsElements(true)
{
}

RemoveElementsCommand::~RemoveElementsCommand()
{
    if (m_ownsElements)
        qDeleteAll(m_elements);
}

void RemoveElementsCommand::redo()
{
    if (!m_layer || m_ownsElements)
        return;
    for (int i = m_indexes.size() - 1; i >= 0; --i) {
        const int at = m_layer->indexOf(m_elements.at(i));
        if (at >= 0)
            m_layer->take(at);
    }
    m_ownsElements = true;
}

void RemoveElementsCommand::undo()
{
    if (!m_layer || !m_ownsElements)
        return;
    for (int i = 0; i < m_indexes.size(); ++i)
        m_layer->insert(m_indexes.at(i), m_elements.at(i));
    m_ownsElements = false;
}

ReparentElementsCommand::ReparentElementsCommand(const QVector<Element *> &elements,
                                                 Layer *source, Layer *target)
    : m_source(source)
    , m_target(target)
{
    if (!source)
        return;
    for (int i = 0; i < elements.size(); ++i) {
        const int at = source->indexOf(elements.at(i));
        if (at >= 0) {
            m_elements.append(elements.at(i));
            m_indexes.append(at);
        }
    }
    for (int i = 0; i < m_indexes.size(); ++i) {
        for (int j = i + 1; j < m_indexes.size(); ++j) {
            if (m_indexes.at(j) < m_indexes.at(i)) {
                qSwap(m_indexes[i], m_indexes[j]);
                qSwap(m_elements[i], m_elements[j]);
            }
        }
    }
}

void ReparentElementsCommand::redo()
{
    if (!m_source || !m_target)
        return;
    for (int i = m_indexes.size() - 1; i >= 0; --i) {
        const int at = m_source->indexOf(m_elements.at(i));
        if (at >= 0)
            m_source->take(at);
    }
    for (int i = 0; i < m_elements.size(); ++i)
        m_target->append(m_elements.at(i));
}

void ReparentElementsCommand::undo()
{
    if (!m_source || !m_target)
        return;
    for (int i = m_elements.size() - 1; i >= 0; --i) {
        const int at = m_target->indexOf(m_elements.at(i));
        if (at >= 0)
            m_target->take(at);
    }
    for (int i = 0; i < m_indexes.size(); ++i)
        m_source->insert(m_indexes.at(i), m_elements.at(i));
}

ChangeTextCommand::ChangeTextCommand(TextItem *item, const QString &before, const QString &after)
    : m_item(item)
    , m_before(before)
    , m_after(after)
{
}

void ChangeTextCommand::undo()
{
    m_item->text = m_before;
}

void ChangeTextCommand::redo()
{
    m_item->text = m_after;
}

static void applyFormat(TextItem *item, const TextFormat &f)
{
    item->fontSize = f.size;
    item->bold = f.bold;
    item->italic = f.italic;
    item->underline = f.underline;
    item->style = TextItem::Style(f.style);
}

bool operator==(const TextFormat &a, const TextFormat &b)
{
    return qFuzzyCompare(a.size, b.size) && a.bold == b.bold && a.italic == b.italic
            && a.underline == b.underline && a.style == b.style;
}

ChangeTextFormatCommand::ChangeTextFormatCommand(const QVector<TextItem *> &items,
                                                 const QVector<TextFormat> &before,
                                                 const TextFormat &after)
    : m_items(items)
    , m_before(before)
    , m_after(after)
{
}

void ChangeTextFormatCommand::undo()
{
    for (int i = 0; i < m_items.size() && i < m_before.size(); ++i)
        applyFormat(m_items.at(i), m_before.at(i));
}

void ChangeTextFormatCommand::redo()
{
    for (int i = 0; i < m_items.size(); ++i)
        applyFormat(m_items.at(i), m_after);
}

ChangeGeometryCommand::ChangeGeometryCommand(Stroke *stroke,
                                             const QVector<StrokePoint> &before,
                                             const QVector<StrokePoint> &after)
    : m_stroke(stroke)
    , m_path(0)
    , m_pointsBefore(before)
    , m_pointsAfter(after)
{
}

ChangeGeometryCommand::ChangeGeometryCommand(Path *path, const QPointF &beforeStart,
                                             const QVector<CubicSegment> &before,
                                             const QPointF &afterStart,
                                             const QVector<CubicSegment> &after)
    : m_stroke(0)
    , m_path(path)
    , m_startBefore(beforeStart)
    , m_startAfter(afterStart)
    , m_segmentsBefore(before)
    , m_segmentsAfter(after)
{
}

void ChangeGeometryCommand::apply(bool after)
{
    if (m_stroke)
        m_stroke->setPoints(after ? m_pointsAfter : m_pointsBefore);
    else if (m_path)
        m_path->setSegments(after ? m_startAfter : m_startBefore,
                            after ? m_segmentsAfter : m_segmentsBefore);
}

void ChangeGeometryCommand::undo() { apply(false); }
void ChangeGeometryCommand::redo() { apply(true); }

ElementStyle ChangeStyleCommand::styleOf(const Element *e)
{
    ElementStyle s;
    if (!e)
        return s;

    if (e->type() == Element::StrokeType) {
        const Stroke *v = static_cast<const Stroke *>(e);
        s.color = v->color;
        s.width = v->width;
        s.fill = v->fill;
        s.fillColor = v->fillColor;
        s.gradient = v->gradient;
        s.gradientAngle = v->gradientAngle;
    } else if (e->type() == Element::PathType) {
        const Path *v = static_cast<const Path *>(e);
        s.color = v->color;
        s.width = v->width;
        s.fill = v->fill;
        s.fillColor = v->fillColor;
        s.gradient = v->gradient;
        s.gradientAngle = v->gradientAngle;
    } else if (e->type() == Element::TextType) {
        const TextItem *v = static_cast<const TextItem *>(e);
        s.color = v->color;
        s.textStyle = int(v->style);
    }
    return s;
}

void ChangeStyleCommand::applyStyle(Element *e, const ElementStyle &style)
{
    if (!e)
        return;

    if (e->type() == Element::StrokeType) {
        Stroke *v = static_cast<Stroke *>(e);
        v->color = style.color;
        if (style.width > 0)
            v->width = style.width;
        v->fill = style.fill;
        v->fillColor = style.fillColor;
        v->gradient = style.gradient;
        v->gradientAngle = style.gradientAngle;
        v->invalidate();
    } else if (e->type() == Element::PathType) {
        Path *v = static_cast<Path *>(e);
        v->color = style.color;
        if (style.width > 0)
            v->width = style.width;
        v->fill = style.fill;
        v->fillColor = style.fillColor;
        v->gradient = style.gradient;
        v->gradientAngle = style.gradientAngle;
        v->invalidate();
    } else if (e->type() == Element::TextType) {
        TextItem *v = static_cast<TextItem *>(e);
        v->color = style.color;
        if (style.textStyle >= 0)
            v->style = TextItem::Style(style.textStyle);
    }
}

ChangeStyleCommand::ChangeStyleCommand(const QVector<Element *> &elements,
                                       const QVector<ElementStyle> &before,
                                       const QVector<ElementStyle> &after)
    : m_elements(elements)
    , m_before(before)
    , m_after(after)
{
}

void ChangeStyleCommand::undo()
{
    for (int i = 0; i < m_elements.size() && i < m_before.size(); ++i)
        applyStyle(m_elements.at(i), m_before.at(i));
}

void ChangeStyleCommand::redo()
{
    for (int i = 0; i < m_elements.size() && i < m_after.size(); ++i)
        applyStyle(m_elements.at(i), m_after.at(i));
}

ReplaceElementsCommand::ReplaceElementsCommand(Layer *layer,
                                               const QVector<Element *> &removed,
                                               const QVector<int> &indexes,
                                               const QVector<Element *> &added)
    : m_layer(layer)
    , m_removed(removed)
    , m_indexes(indexes)
    , m_added(added)
    , m_ownsRemoved(true)
{
}

ReplaceElementsCommand::~ReplaceElementsCommand()
{
    if (m_ownsRemoved)
        qDeleteAll(m_removed);
    else
        qDeleteAll(m_added);
}

void ReplaceElementsCommand::redo()
{
    if (!m_layer)
        return;
    for (int i = 0; i < m_added.size(); ++i)
        if (m_layer->indexOf(m_added.at(i)) < 0)
            m_layer->append(m_added.at(i));
    m_ownsRemoved = true;
}

void ReplaceElementsCommand::undo()
{
    if (!m_layer)
        return;

    for (int i = 0; i < m_added.size(); ++i) {
        const int at = m_layer->indexOf(m_added.at(i));
        if (at >= 0)
            m_layer->take(at);
    }
    for (int i = 0; i < m_indexes.size() && i < m_removed.size(); ++i)
        m_layer->insert(qMin(m_indexes.at(i), m_layer->count()), m_removed.at(i));

    m_ownsRemoved = false;
}

ScaleElementsCommand::ScaleElementsCommand(const QVector<Element *> &elements,
                                           const QPointF &origin, qreal sx, qreal sy)
    : m_elements(elements)
    , m_origin(origin)
    , m_sx(sx)
    , m_sy(sy)
{
}

void ScaleElementsCommand::undo()
{
    if (qFuzzyIsNull(m_sx) || qFuzzyIsNull(m_sy))
        return;
    for (int i = 0; i < m_elements.size(); ++i)
        m_elements.at(i)->scale(m_origin, 1.0 / m_sx, 1.0 / m_sy);
}

void ScaleElementsCommand::redo()
{
    for (int i = 0; i < m_elements.size(); ++i)
        m_elements.at(i)->scale(m_origin, m_sx, m_sy);
}

MoveElementsCommand::MoveElementsCommand(const QVector<Element *> &elements, qreal dx, qreal dy)
    : m_elements(elements)
    , m_dx(dx)
    , m_dy(dy)
{
}

void MoveElementsCommand::redo()
{
    for (int i = 0; i < m_elements.size(); ++i)
        m_elements.at(i)->translate(m_dx, m_dy);
}

void MoveElementsCommand::undo()
{
    for (int i = 0; i < m_elements.size(); ++i)
        m_elements.at(i)->translate(-m_dx, -m_dy);
}

GroupElementsCommand::GroupElementsCommand(Layer *layer, const QVector<Element *> &elements,
                                           int group)
    : m_layer(layer)
    , m_group(group)
    , m_insertAt(0)
{
    if (!layer)
        return;
    for (int i = 0; i < elements.size(); ++i) {
        const int at = layer->indexOf(elements.at(i));
        if (at >= 0) {
            m_elements.append(elements.at(i));
            m_oldIndexes.append(at);
            m_oldGroups.append(elements.at(i)->group);
        }
    }
    for (int i = 0; i < m_oldIndexes.size(); ++i) {
        for (int j = i + 1; j < m_oldIndexes.size(); ++j) {
            if (m_oldIndexes.at(j) < m_oldIndexes.at(i)) {
                qSwap(m_oldIndexes[i], m_oldIndexes[j]);
                qSwap(m_elements[i], m_elements[j]);
                qSwap(m_oldGroups[i], m_oldGroups[j]);
            }
        }
    }
    m_insertAt = m_oldIndexes.isEmpty() ? 0 : m_oldIndexes.first();
}

void GroupElementsCommand::redo()
{
    if (!m_layer)
        return;
    for (int i = m_elements.size() - 1; i >= 0; --i) {
        const int at = m_layer->indexOf(m_elements.at(i));
        if (at >= 0)
            m_layer->take(at);
    }
    for (int i = 0; i < m_elements.size(); ++i) {
        m_elements.at(i)->group = m_group;
        m_layer->insert(m_insertAt + i, m_elements.at(i));
    }
}

void GroupElementsCommand::undo()
{
    if (!m_layer)
        return;
    for (int i = m_elements.size() - 1; i >= 0; --i) {
        const int at = m_layer->indexOf(m_elements.at(i));
        if (at >= 0)
            m_layer->take(at);
    }
    for (int i = 0; i < m_elements.size(); ++i) {
        m_elements.at(i)->group = m_oldGroups.at(i);
        m_layer->insert(m_oldIndexes.at(i), m_elements.at(i));
    }
}

UndoStack::UndoStack()
    : m_limit(80)
{
}

UndoStack::~UndoStack()
{
    clear();
}

void UndoStack::push(UndoCommand *cmd)
{
    if (!cmd)
        return;
    cmd->redo();
    pushWithoutRedo(cmd);
}

void UndoStack::pushWithoutRedo(UndoCommand *cmd)
{
    if (!cmd)
        return;
    qDeleteAll(m_redo);
    m_redo.clear();
    m_undo.append(cmd);
    trim();
}

void UndoStack::undo()
{
    if (m_undo.isEmpty())
        return;
    UndoCommand *cmd = m_undo.takeLast();
    cmd->undo();
    m_redo.append(cmd);
}

void UndoStack::redo()
{
    if (m_redo.isEmpty())
        return;
    UndoCommand *cmd = m_redo.takeLast();
    cmd->redo();
    m_undo.append(cmd);
}

void UndoStack::clear()
{
    qDeleteAll(m_undo);
    m_undo.clear();
    qDeleteAll(m_redo);
    m_redo.clear();
}

void UndoStack::trim()
{
    while (m_undo.size() > m_limit)
        delete m_undo.takeFirst();
}
}
