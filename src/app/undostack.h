#ifndef XN_UNDOSTACK_H
#define XN_UNDOSTACK_H

#include "model/gradient.h"

#include <QColor>
#include <QList>
#include <QPointF>
#include <QVector>
#include <QtGlobal>

namespace xn {
class Element;
class Layer;
class Path;
class Stroke;
class TextItem;
struct CubicSegment;
struct StrokePoint;

class UndoCommand
{
public:
    virtual ~UndoCommand() {}
    virtual void undo() = 0;
    virtual void redo() = 0;
};

class AddElementCommand : public UndoCommand
{
public:
    AddElementCommand(Layer *layer, Element *element);
    ~AddElementCommand();

    void undo();
    void redo();

private:
    Layer *m_layer;
    Element *m_element;
    int m_index;
    bool m_ownsElement;
};

class RemoveElementsCommand : public UndoCommand
{
public:
    RemoveElementsCommand(Layer *layer, const QVector<Element *> &elements);

    RemoveElementsCommand(Layer *layer, const QVector<Element *> &elements,
                          const QVector<int> &indexes);

    ~RemoveElementsCommand();

    void undo();
    void redo();

    bool isEmpty() const { return m_elements.isEmpty(); }

private:
    Layer *m_layer;
    QVector<Element *> m_elements;
    QVector<int> m_indexes;
    bool m_ownsElements;
};

class ChangeTextCommand : public UndoCommand
{
public:
    ChangeTextCommand(TextItem *item, const QString &before, const QString &after);

    void undo();
    void redo();

private:
    TextItem *m_item;
    QString m_before;
    QString m_after;
};

struct TextFormat
{
    qreal size;
    bool bold;
    bool italic;
    bool underline;
    int style;
};

bool operator==(const TextFormat &a, const TextFormat &b);
inline bool operator!=(const TextFormat &a, const TextFormat &b) { return !(a == b); }

class ChangeTextFormatCommand : public UndoCommand
{
public:
    ChangeTextFormatCommand(const QVector<TextItem *> &items,
                            const QVector<TextFormat> &before,
                            const TextFormat &after);

    void undo();
    void redo();

private:
    QVector<TextItem *> m_items;
    QVector<TextFormat> m_before;
    TextFormat m_after;
};

// Node edits rewrite a whole list, so both sides are snapshotted.
class ChangeGeometryCommand : public UndoCommand
{
public:
    ChangeGeometryCommand(Stroke *stroke, const QVector<StrokePoint> &before,
                          const QVector<StrokePoint> &after);
    ChangeGeometryCommand(Path *path, const QPointF &beforeStart,
                          const QVector<CubicSegment> &before,
                          const QPointF &afterStart,
                          const QVector<CubicSegment> &after);

    void undo();
    void redo();

private:
    void apply(bool after);

    Stroke *m_stroke;
    Path *m_path;
    QVector<StrokePoint> m_pointsBefore;
    QVector<StrokePoint> m_pointsAfter;
    QPointF m_startBefore;
    QPointF m_startAfter;
    QVector<CubicSegment> m_segmentsBefore;
    QVector<CubicSegment> m_segmentsAfter;
};

// One step: a remove plus an add would undo in two.
struct ElementStyle
{
    ElementStyle(): width(-1), fill(-1), gradientAngle(0), textStyle(-1) {}

    QColor color;
    qreal width;
    int fill;
    QColor fillColor;
    GradientStops gradient;
    int gradientAngle;
    int textStyle;
};

class ChangeStyleCommand : public UndoCommand
{
public:
    ChangeStyleCommand(const QVector<Element *> &elements,
                       const QVector<ElementStyle> &before,
                       const QVector<ElementStyle> &after);

    void undo();
    void redo();

    static ElementStyle styleOf(const Element *e);
    static void applyStyle(Element *e, const ElementStyle &style);

private:
    QVector<Element *> m_elements;
    QVector<ElementStyle> m_before;
    QVector<ElementStyle> m_after;
};

class ReplaceElementsCommand : public UndoCommand
{
public:
    ReplaceElementsCommand(Layer *layer, const QVector<Element *> &removed,
                           const QVector<int> &indexes, const QVector<Element *> &added);
    ~ReplaceElementsCommand();

    void undo();
    void redo();

private:
    Layer *m_layer;
    QVector<Element *> m_removed;
    QVector<int> m_indexes;
    QVector<Element *> m_added;
    bool m_ownsRemoved;
};

class ScaleElementsCommand : public UndoCommand
{
public:
    ScaleElementsCommand(const QVector<Element *> &elements, const QPointF &origin,
                         qreal sx, qreal sy);

    void undo();
    void redo();

private:
    QVector<Element *> m_elements;
    QPointF m_origin;
    qreal m_sx;
    qreal m_sy;
};

class MoveElementsCommand : public UndoCommand
{
public:
    MoveElementsCommand(const QVector<Element *> &elements, qreal dx, qreal dy);

    void undo();
    void redo();

private:
    QVector<Element *> m_elements;
    qreal m_dx;
    qreal m_dy;
};

class ReparentElementsCommand : public UndoCommand
{
public:
    ReparentElementsCommand(const QVector<Element *> &elements, Layer *source, Layer *target);

    void undo();
    void redo();

private:
    QVector<Element *> m_elements;
    QVector<int> m_indexes;
    Layer *m_source;
    Layer *m_target;
};

class GroupElementsCommand : public UndoCommand
{
public:
    GroupElementsCommand(Layer *layer, const QVector<Element *> &elements, int group);

    void undo();
    void redo();

private:
    Layer *m_layer;
    QVector<Element *> m_elements;
    QVector<int> m_oldIndexes;
    QVector<int> m_oldGroups;
    int m_group;
    int m_insertAt;
};

class UndoStack
{
public:
    UndoStack();
    ~UndoStack();

    void push(UndoCommand *cmd);
    void pushWithoutRedo(UndoCommand *cmd);

    bool canUndo() const { return !m_undo.isEmpty(); }
    bool canRedo() const { return !m_redo.isEmpty(); }

    void undo();
    void redo();
    void clear();

    void setLimit(int n) { m_limit = n; }

private:
    void trim();

    QList<UndoCommand *> m_undo;
    QList<UndoCommand *> m_redo;
    int m_limit;
};
}

#endif
