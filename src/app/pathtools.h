#ifndef XN_PATHTOOLS_H
#define XN_PATHTOOLS_H

#include <QObject>
#include <QVector>

namespace xn {
class Document;
class Element;
class Layer;
class Path;
class Selection;
class UndoStack;

class PathTools : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool canConvertText READ canConvertText NOTIFY changed)
    Q_PROPERTY(bool canCombine READ canCombine NOTIFY changed)
    Q_PROPERTY(bool canBreakApart READ canBreakApart NOTIFY changed)

public:
    PathTools(Document *document, Selection *selection, UndoStack *undo, QObject *parent = 0);

    void setDocument(Document *document) { m_document = document; }

    bool canConvertText() const;
    bool canCombine() const;
    bool canBreakApart() const;

    Q_INVOKABLE bool textToPath();
    Q_INVOKABLE bool join();
    Q_INVOKABLE bool breakApart();
    Q_INVOKABLE bool unite();
    Q_INVOKABLE bool subtract();

Q_SIGNALS:
    void changed();
    void contentChanged(int page);
    void historyChanged();
    void error(const QString &message);

private:
    Layer *targetLayer() const;
    bool replace(const QVector<Element *> &removed, const QVector<Path *> &added);

    Document *m_document;
    Selection *m_selection;
    UndoStack *m_undo;
};
}

#endif
