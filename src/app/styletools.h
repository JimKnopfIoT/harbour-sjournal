#ifndef XN_STYLETOOLS_H
#define XN_STYLETOOLS_H

#include "model/gradient.h"

#include <QColor>
#include <QObject>
#include <QVariantList>

namespace xn {
class Selection;
class UndoStack;

// ToolSettings styles the next element; this one restyles what is already drawn.
class StyleTools : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool canRestyle READ canRestyle NOTIFY changed)
    Q_PROPERTY(bool canFill READ canFill NOTIFY changed)
    Q_PROPERTY(bool filled READ isFilled NOTIFY changed)
    Q_PROPERTY(QColor color READ color NOTIFY changed)
    Q_PROPERTY(QColor fillColor READ fillColor NOTIFY changed)
    Q_PROPERTY(int fillAlpha READ fillAlpha NOTIFY changed)
    Q_PROPERTY(qreal strokeWidth READ strokeWidth NOTIFY changed)
    Q_PROPERTY(QVariantList gradientColors READ gradientColors NOTIFY changed)
    Q_PROPERTY(int gradientAngle READ gradientAngle NOTIFY changed)

public:
    StyleTools(Selection *selection, UndoStack *undo, QObject *parent = 0);

    bool canRestyle() const;
    bool canFill() const;
    bool isFilled() const;
    QColor color() const;
    QColor fillColor() const;
    int fillAlpha() const;
    qreal strokeWidth() const;
    QVariantList gradientColors() const;
    int gradientAngle() const;

    Q_INVOKABLE bool applyColor(const QColor &color);
    Q_INVOKABLE bool applyWidth(qreal width);
    Q_INVOKABLE bool applyFill(const QColor &color, int alpha = 255);
    Q_INVOKABLE bool removeFill();
    Q_INVOKABLE bool toggleFill(const QColor &color, int alpha = 255);
    Q_INVOKABLE bool applyGradient(const QVariantList &colors, int angle, int alpha = 255);
    Q_INVOKABLE bool clearGradient();

Q_SIGNALS:
    void changed();
    void contentChanged(int page);
    void historyChanged();

private:
    enum Field { Colour, Width, Fill, Gradient };
    bool apply(Field field, const QColor &color, qreal width, int alpha,
               const GradientStops &stops = GradientStops(), int angle = 0);

    Selection *m_selection;
    UndoStack *m_undo;
};
}

#endif
