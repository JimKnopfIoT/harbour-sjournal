#ifndef XN_SELECTIONTRANSFORM_H
#define XN_SELECTIONTRANSFORM_H

#include <QObject>
#include <QPointF>
#include <QRectF>

namespace xn {
class Selection;
class UndoStack;

class SelectionTransform : public QObject
{
    Q_OBJECT

    Q_PROPERTY(Mode mode READ mode WRITE setMode NOTIFY modeChanged)

public:
    enum Mode { Move, Resize, FlipHorizontal, FlipVertical };
    Q_ENUM(Mode)

    SelectionTransform(Selection *selection, UndoStack *undo, QObject *parent = 0);

    Mode mode() const { return m_mode; }
    void setMode(Mode m);

    Q_PROPERTY(QRectF box READ box NOTIFY boxChanged)

    QRectF box() const;
    Q_INVOKABLE bool moveTo(qreal x, qreal y);
    Q_INVOKABLE bool resizeTo(qreal w, qreal h);

    Q_INVOKABLE bool flipHorizontal();
    Q_INVOKABLE bool flipVertical();

    bool isScaling() const { return m_scaling; }
    int grabHandle(const QPointF &pos, qreal radius) const;
    void beginScale(int handle);
    void scaleTo(const QPointF &pos, bool keepAspect);
    void endScale();

Q_SIGNALS:
    void modeChanged();
    void boxChanged();
    void contentChanged(int page);
    void historyChanged();

private:
    bool applyScale(const QPointF &origin, qreal sx, qreal sy);

    Selection *m_selection;
    UndoStack *m_undo;
    Mode m_mode;

    bool m_scaling;
    int m_handle;
    QRectF m_startRect;
    QPointF m_anchor;
    qreal m_totalX;
    qreal m_totalY;
};
}

#endif
