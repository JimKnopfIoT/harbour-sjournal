#ifndef XN_TEXTITEM_H
#define XN_TEXTITEM_H

#include "model/element.h"

#include <QColor>
#include <QFont>
#include <QPointF>
#include <QString>

namespace xn {
class TextItem : public Element
{
public:
    enum Style { Filled, Outline, Inline };

    TextItem();

    Type type() const { return TextType; }
    QRectF bounds() const;
    Element *clone() const;
    void translate(qreal dx, qreal dy) { pos += QPointF(dx, dy); }
    void scale(const QPointF &origin, qreal sx, qreal sy);

    QString text;
    QPointF pos;
    QString fontName;
    qreal fontSize;
    bool bold;
    bool italic;
    bool underline;
    Style style;
    QColor color;

    QFont font() const;
};
}

#endif
